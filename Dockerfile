# Copyright 2022 Primihub
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


FROM ubuntu:20.04 as builder

ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN  apt update \
  && apt install -y python3 python3-dev gcc-8 g++-8 python-dev libgmp-dev cmake \
  && apt install -y automake ca-certificates git libtool m4 patch pkg-config unzip make wget curl zip ninja-build npm \
  && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8 \
  && rm -rf /var/lib/apt/lists/*

# install  bazelisk
RUN npm install -g @bazel/bazelisk

# Install keyword PIR dependencies
WORKDIR /opt
RUN wget https://github.com/zeromq/libzmq/archive/refs/tags/v4.3.4.tar.gz \
  && tar -zxf v4.3.4.tar.gz && mkdir libzmq-4.3.4/build && cd libzmq-4.3.4/build \
  && cmake .. && make && make install

RUN wget https://github.com/zeromq/cppzmq/archive/refs/tags/v4.9.0.tar.gz \
  && tar -zxf v4.9.0.tar.gz && mkdir cppzmq-4.9.0/build && cd cppzmq-4.9.0/build \
  && cmake .. && make && make install

RUN wget https://github.com/google/flatbuffers/archive/refs/tags/v2.0.0.tar.gz \
  && tar -zxf v2.0.0.tar.gz && mkdir flatbuffers-2.0.0/build && cd flatbuffers-2.0.0/build \
  && cmake .. && make && make install

RUN wget https://sourceforge.net/projects/tclap/files/tclap-1.2.5.tar.gz \
  && tar -zxvf tclap-1.2.5.tar.gz && cd tclap-1.2.5 && ./configure \
  && make && make install

WORKDIR /src
ADD . /src

# Bazel build primihub-node & primihub-cli & paillier shared library
RUN bash pre_build.sh \
  && bazel build --cxxopt=-D_AMD64_ --config=linux --define microsoft-apsi=true :node :cli :opt_paillier_c2py

FROM ubuntu:20.04 as runner

# Install python3 and GCC openmp (Depends with cryptFlow2 library)
RUN apt-get update \
  && apt-get install -y python3 python3-dev libgomp1 python3-pip libzmq5 \
  && rm -rf /var/lib/apt/lists/*

ARG TARGET_PATH=/root/.cache/bazel/_bazel_root/f8087e59fd95af1ae29e8fcb7ff1a3dc/execroot/primihub/bazel-out/k8-fastbuild/bin
WORKDIR $TARGET_PATH

# Copy binaries to TARGET_PATH
COPY --from=builder $TARGET_PATH ./
# Copy test data files to /tmp/
COPY --from=builder /src/data/ /tmp/

# Change WorkDir to /app
WORKDIR /app

# Make symlink to primihub-node & primihub-cli
RUN ln -s $TARGET_PATH/node /app/primihub-node && ln -s $TARGET_PATH/cli /app/primihub-cli

# Copy all test config files to /app/config
COPY --from=builder /src/config ./config

# Copy primihub python sources to /app and setup to system python3
RUN mkdir -p src/primihub/protos data log
COPY --from=builder /src/python ./python
COPY --from=builder /src/src/primihub/protos/ ./src/primihub/protos/

WORKDIR /app/python
RUN python3 -m pip install --upgrade pip \
  && python3 -m pip install -r requirements.txt \
  && python3 setup.py install \
  && python3 setup.py solib --solib-path $TARGET_PATH 

WORKDIR /app

# gRPC server port
EXPOSE 50050
# Cryptool port
EXPOSE 12120
EXPOSE 12121

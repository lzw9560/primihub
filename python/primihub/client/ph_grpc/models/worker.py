"""
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """
import sys
from os import path
from typing import Type
from protobuf_to_pydantic import msg_to_pydantic_model
from pydantic import BaseModel

here = path.abspath(path.join(path.dirname(__file__), ".."))
sys.path.append(here)
from src.primihub.protos import worker_pb2, worker_pb2_grpc  # noqa

PushTaskRequestModel: Type[BaseModel] = msg_to_pydantic_model(worker_pb2.PushTaskRequest)
PushTaskReplyModel: Type[BaseModel] = msg_to_pydantic_model(worker_pb2.PushTaskReply)

TaskRequestModel: Type[BaseModel] = msg_to_pydantic_model(worker_pb2.TaskRequest)
TaskResponseModel: Type[BaseModel] = msg_to_pydantic_model(worker_pb2.TaskResponse)

print(
    PushTaskRequestModel.__name__,
    {
        k: v.field_info
        for k, v in PushTaskRequestModel.__fields__.items()
    }
)

print(
    PushTaskReplyModel.__name__,
    {
        k: v.field_info
        for k, v in PushTaskRequestModel.__fields__.items()
    }
)

print(
    TaskRequestModel.__name__,
    {
        k: v.field_info
        for k, v in PushTaskRequestModel.__fields__.items()
    }
)

print(
    TaskResponseModel.__name__,
    {
        k: v.field_info
        for k, v in PushTaskRequestModel.__fields__.items()
    }
)

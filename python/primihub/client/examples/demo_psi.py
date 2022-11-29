"""
Copyright 2022 Primihub

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     https: //www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

https://docs.primihub.com/docs/advance-usage/create-tasks/pir-task

"""
import logging

from primihub.client import primihub_cli as cli

# client init
# cli.init(config={"node": "127.0.0.1:50050", "cert": ""})
# cli.init(config={"node": "192.168.99.23:8050", "cert": ""})
cli.init(config={"node": "192.168.99.26:50050", "cert": ""})


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT,
                        datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


# arbiter_info, guest_info, host_info, task_type, task_params = load_info()
task_params = {}
logger = get_logger("psi")

"""
./bazel-bin/cli 
--server="你的IP:50050" 
--task_type=3 
--params="clientData:STRING:0:psi_client_data,serverData:STRING:0:psi_server_data,
clientIndex:INT32:0:0,serverIndex:INT32:0:1,
psiType:INT32:0:0,psiTag:INT32:0:0,outputFullFilename:STRING:0:/data/result/psi_result.csv" 
--input_datasets="clientData,serverData"
"""

params = {
    "clientData": "psi_client_data",
    "serverData": "psi_server_data",
    "clientIndex": 0,
    "serverIndex": 1,
    "psiType": 0,
    "psiTag": 0,
    "outputFullFilename": "/data/result/cli/psi_result.csv",

}

cli.async_remote_execute(
    task_type=3,
    name="psi task",
    language=3,
    params=params,
    node_map={},
    input_datasets=["clientData", "serverData"],
)

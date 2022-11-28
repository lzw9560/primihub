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
import random

from primihub.client.ph_grpc.event import listener
from primihub.client.ph_grpc.models.common import TaskModel
from primihub.client.ph_grpc.models.worker import PushTaskRequestModel
from primihub.client.ph_grpc.worker import WorkerClient


class GrpcClient(object):
    """primihub grpc client"""

    def __init__(self, node, cert):
        self.node = node
        self.cert = cert
        self.listener = listener

    # def submit_task(self, code: str, job_id: str, work_id: str, client_id: str):
    def submit_task(self, task: TaskModel, client_id: str):
        """submit task


        :return: _description_
        :rtype: _type_
        """
        client = WorkerClient(self.node, self.cert)
        # task = client.set_task(code=code.encode('utf-8'),
        #                            job_id=bytes(str(job_id), "utf-8"),
        #                            work_id=bytes(str(work_id), "utf-8"))
        request = client.push_task_request(intended_worker_id=b'1',
                                           task=task,
                                           sequence_number=random.randint(0, 9999),
                                           client_processed_up_to=random.randint(0, 9999),
                                           submit_client_id=bytes(str(client_id), "utf-8")
                                           )
        res = client.submit_task(request)
        return res

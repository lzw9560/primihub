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
import uuid
import grpc

from .connect import GRPCConnect
from src.primihub.protos import common_pb2, worker_pb2, worker_pb2_grpc  # noqa
from .models.common import TaskModel, ParamsModel, ParamValueModel
from .models.worker import PushTaskRequestModel

from primihub.utils.logger_util import logger
from .src.primihub.protos.common_pb2 import ParamValue, Params


class WorkerClient(GRPCConnect):
    """primihub gRPC worker client

    :return: A primihub gRPC worker client.
    """

    def __init__(self, node, cert) -> None:
        """Constructor
        """
        super(WorkerClient, self).__init__(node, cert)
        self.channel = grpc.insecure_channel(self.node)
        self.request_data = None
        self.stub = worker_pb2_grpc.VMNodeStub(self.channel)

    @staticmethod
    def set_task_model(task_type: common_pb2.TaskType = 0,
                       name: str = "",
                       language: common_pb2.Language = 0,
                       params: common_pb2.Params = None,
                       code: bytes = None,
                       node_map: common_pb2.Task.NodeMapEntry = None,
                       input_datasets: str = None,
                       job_id: bytes = None,
                       task_id: bytes = None,
                       ):
        """set task map

        :param task_type: {}
        :param name: str
        :param language: {}
        :param params: `dict`
        :param code: bytes
        :param node_map: `dict`
        :param input_datasets:
        :param job_id: bytes
        :param task_id: bytes

        :return: `dict`
        """

        logger.debug(f"params: {params}")

        params_map = {}
        for k, v in params.items():
            if k == "pirType":
                p = common_pb2.ParamValue()
                p.var_type = 0
                p.is_array = False
                p.value_int32 = v
                params_map[k] = p

            elif k == "clientData":
                p = common_pb2.ParamValue()
                p.var_type = 2
                p.is_array = True
                p.value_string = v
                params_map[k] = p
            elif k == "serverData":
                p = common_pb2.ParamValue()
                p.var_type = 2
                p.is_array = False
                p.value_string = v
                params_map[k] = p
            elif k == "outputFullFilename":
                p = common_pb2.ParamValue()
                p.var_type = 2
                p.is_array = False
                p.value_string = v
                params_map[k] = p

        params_obj = common_pb2.Params(param_map=params_map)

        task = common_pb2.Task(type=task_type,
                               name=name,
                               language=language,
                               params=params_obj,
                               code=code,
                               node_map=node_map,
                               input_datasets=input_datasets,
                               job_id=job_id or bytes(str(uuid.uuid1().hex), "utf-8"),
                               task_id=task_id or bytes(str(uuid.uuid1().hex), "utf-8"))

        logger.debug(f"task: {task}")
        return task

    @staticmethod
    def push_task_request(intended_worker_id=b'1',
                          task=TaskModel,
                          sequence_number=11,
                          client_processed_up_to=22,
                          submit_client_id=b""
                          ):
        # request_data = {
        #     "intended_worker_id": intended_worker_id,
        #     "task": task,
        #     "sequence_number": sequence_number,
        #     "client_processed_up_to": client_processed_up_to,
        #     "submit_client_id": submit_client_id
        # }

        request = worker_pb2.PushTaskRequest(
            intended_worker_id=intended_worker_id,
            task=task,
            sequence_number=sequence_number,
            client_processed_up_to=client_processed_up_to,
            submit_client_id=submit_client_id
        )
        # request = worker_pb2.PushTaskRequest(**request_data)
        # request = worker_pb2.PushTaskRequest(request_data)
        return request

    def submit_task(self, request: worker_pb2.PushTaskRequest) -> worker_pb2.PushTaskReply:
        """gRPC submit task

        :returns: gRPC reply
        :rtype: :obj:`worker_pb2.PushTaskReply`
        """
        # print(type(request_data), request_data)
        with self.channel:
            reply = self.stub.SubmitTask(request)
            print("return code: %s, job id: %s" % (reply.ret_code, reply.job_id))  # noqa
            return reply

    # def execute(self, request) -> None:
    #     pass

    # def remote_execute(self, *args):
    #     """Send local functions and parameters to the remote side

    #     :param args: `list` [`tuple`] (`function`, `args`)
    #     """
    #     for arg in args:
    #         # TODO
    #         print(arg)

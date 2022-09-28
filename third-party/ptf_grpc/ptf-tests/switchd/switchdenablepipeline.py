"""
Copyright(c) 2022 Intel Corporation.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

"""
    This test case enables the pipeline

"""
import struct
import traceback
import os
import sys
import json
from ptf.testutils import *
from ptf.packet import *
from utils import REPORTING
from switchd_base_test import *
import ptf_grpc.ptfRpc_pb2_grpc as ptfrpc_pb2_grpc
import ptf_grpc.ptfRpc_pb2 as ptfrpc_pb2

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(THIS_DIR, '..'))
test_name = os.path.splitext(os.path.basename(__file__))[0]

class switchdEnablePipeline(SwitchdHelper):
    def setup(self):
        super(switchdEnablePipeline, self).setup()
        
    def runTest(self):
        id = 0
        print("Started running pipeline enable test.")
        try:
            print("We are going to enable the pipeline.")
            status = self.GrpcClient.enable_pipeline(ptfrpc_pb2.device_id(dev_id = id))
            if(status.responsem != 0):
                raise Exception("Pipeline can't be enabled failed with status",status.responsem)

            print("Pipeline has been enabled successfully.")
            REPORTING.update_results(REPORTING.get_json(), test_name, "SUCCESS")
        except Exception as e:
            print(traceback.print_exc())
            REPORTING.update_results(REPORTING.get_json(), test_name, "FAILURE")
        finally:
            pass

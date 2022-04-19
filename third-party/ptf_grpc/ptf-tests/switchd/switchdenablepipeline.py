"""
    This test case enables the pipeline

"""

import struct
import traceback
import os
import sys

from ptf.testutils import *
from ptf.packet import *

from switchd_base_test import *
import ptf_grpc.ptfRpc_pb2_grpc as ptfrpc_pb2_grpc
import ptf_grpc.ptfRpc_pb2 as ptfrpc_pb2

from utils import RESULT_STATUS

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(THIS_DIR, '..'))

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
            print(RESULT_STATUS.SUCCESS)
        except Exception as e:
            print("Exception occurred ")
            print(e)
            print(traceback.print_exc())
            print(RESULT_STATUS.FAILURE)
        finally:
            pass

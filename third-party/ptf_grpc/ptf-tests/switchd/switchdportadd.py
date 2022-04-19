"""
This test case add 4 TAP ports
"""

import ptf_grpc.ptfRpc_pb2 as ptfrpc_pb2
import ptf_grpc.ptfRpc_pb2_grpc as ptfrpc_pb2_grpc

import traceback
from ptf.testutils import *
from ptf.packet import *
import os
import sys

from utils import RESULT_STATUS

from switchd_base_test import *

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(THIS_DIR, '..'))

class switchdPortAdd(SwitchdHelper):
    def setup(self):
        super(switchdPortAdd, self).setup()

    def runTest(self):
        port =0
        num_ports=4
        try:
            print("We are going to add the ports.")
            while (port < num_ports):
                print("Calling driver api to add port %d" % port)
                status = self.GrpcClient.port_add(ptfrpc_pb2.switchd_app_int_t(dev_id = port, port_name = "TAP", mempool_name = "MEMPOOL0", pipe_name="pipe"))
                os.popen('ip link set TAP%d up' % port)
                port+=1
                if(status.responsem!=0):
                    raise Exception(port_name, port, " port is not added check bfshell logs")
            print("All %d ports have been added" % num_ports)
            print(RESULT_STATUS.SUCCESS)

        except Exception as e:
            print("Exception occurred ")
            print(e)
            print(traceback.print_exc())
            print(RESULT_STATUS.FAILURE)
        finally:
            pass

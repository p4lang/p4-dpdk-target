"""
Base classes for ptf test cases

The individual test case will inherit this class for
setting up grpc channel, dataplane
"""

from __future__ import print_function

import grpc
import ptf_grpc.ptfRpc_pb2 as ptfrpc_pb2
import ptf_grpc.ptfRpc_pb2_grpc as ptfrpc_pb2_grpc

import os
import ptf
from ptf import config
from ptf.base_tests import BaseTest
import ptf.testutils as testutils
import logging

class GrpcInterface(BaseTest):
    """
    This class:
    Create gRPC channel
    Removes objects created in setup
    """

    def create_grpc_client(self):
        if "grpc_server" in self.test_params:
            grpc_addr = self.test_params['grpc_server']
        else:
            grpc_addr = "localhost:50051"
        print("Going to Connect with",grpc_addr)
        self.channel = grpc.insecure_channel(grpc_addr , options=(('grpc.enable_http_proxy', 0),) )
        self.GrpcClient = ptfrpc_pb2_grpc.switchd_applicationStub(self.channel)
        print("gRPC channel created")
        return

    def setUp(self):
        self.interface_to_front_mapping = {}
        self.port_map_loaded = False
        BaseTest.setUp(self)
        self.test_params = testutils.test_params_get()
        #self.loadPortMap()
        self.create_grpc_client()
        return

    def tearDown(self):
        BaseTest.tearDown(self)
        self.channel.close()

class GrpcInterfaceDataPlane(GrpcInterface):
    """
    Root class that sets up the GRPC interface and dataplane
    """
    def setUp(self):
        GrpcInterface.setUp(self)
        self.dataplane = ptf.dataplane_instance
        if self.dataplane is not None:
            self.dataplane.flush()
            if config["log_dir"] is not None:
                filename = os.path.join(config["log_dir"], str(self)) + ".pcap"
                self.dataplane.start_pcap(filename)

    def tearDown(self):
        if config["log_dir"] is not None:
            self.dataplane.stop_pcap()
        GrpcInterface.tearDown(self)

class SwitchdHelperBase(GrpcInterfaceDataPlane):
    def setUp(self):
        GrpcInterfaceDataPlane.setUp(self)

class SwitchdHelper(SwitchdHelperBase):
    def setUp(self):
        SwitchdHelperBase.setUp(self)

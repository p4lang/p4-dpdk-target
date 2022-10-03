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

import struct
import traceback
import json
import os
import sys
from ctypes import *
from ptf.testutils import *
from ptf.packet import *
from ipaddress import IPv4Address
from switchd_base_test import *
import ptf_grpc.ptfRpc_pb2 as ptfrpc_pb2
import ptf_grpc.ptfRpc_pb2_grpc as ptfrpc_pb2_grpc
from utils import REPORTING

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(THIS_DIR, '..'))
test_name = os.path.splitext(os.path.basename(__file__))[0]

class switchdAddTableEntry(SwitchdHelper):
    def setup(self):
        super(switchdAddTableEntry, self).setup()

    def get_table_name(self, bfrt_json):
        if 'tables' in bfrt_json:
            return bfrt_json['tables'][-1].get('name').split(".")[0] + ".ingress.ipv4_wcm"
        else:
            raise Exception("Table Not found in bf-rt.json")

    def get_p4_name(self, context_json):
        if 'program_name' in context_json:
            return context_json['program_name']
        else:
            raise Exception("Program name not found in context.json")

    def get_id(self, bfrt_json):
        key = bfrt_json['tables'][-1].get('key')[0].get('id')
        action = bfrt_json['tables'][-1].get('action_specs')[-2].get('id')
        data =  bfrt_json['tables'][-1].get('action_specs')[-2].get('data')[0].get('id')
        return key, action, data

    def entry(self, ipaddr, mask, portout, oper, p4_name, table_name, key, action, data):
        print("adding IP %s with mask %s to be sent to port %d" % (str(IPv4Address(ipaddr)), str(IPv4Address(mask)), portout))
        pn = ptfrpc_pb2.prog_name.simple_l2l3_wcm
        status = self.GrpcClient.wcm_add_or_del_table_entry(ptfrpc_pb2.wcm_table(ipaddr = ipaddr, mask = mask, port = portout, op = oper, p4_name = p4_name, table_name = table_name, field_id = key, action_id = action, data_field = data, pn = pn))
        return status

    def runTest(self):
        dev_id = 0
        print("Start running the test.")
        try:
            install_dir = os.environ.get("SDE_INSTALL")
            if install_dir is None:
                raise Exception("Please export SDE & SDE_INSTALL in the env.")
            bfrt = open(os.path.join(install_dir,"sample_app/simple_l2l3_wcm/bf-rt.json"))
            context = open(os.path.join(install_dir,"sample_app/simple_l2l3_wcm/pipe/context.json"))
            bf_rt = json.load(bfrt)
            table_name = self.get_table_name(bf_rt)
            context_json = json.load(context)
            p4_name = self.get_p4_name(context_json)
            key, action, data = self.get_id(bf_rt)
            print("We are going to to add an entry to the WCM table.")
            oper = ptfrpc_pb2.table_operation.addition
            status = self.entry(int(IPv4Address("192.168.1.100")), int(IPv4Address("255.255.255.0")), 1, oper, p4_name, table_name, key, action, data)
            status = self.entry(int(IPv4Address("192.168.100.2")), int(IPv4Address("255.255.0.255")), 2, oper, p4_name, table_name, key, action, data)
            status = self.entry(int(IPv4Address("192.100.168.3")), int(IPv4Address("255.0.255.255")), 3, oper, p4_name, table_name, key, action, data)
            if(status.responsem != 0):
                raise Exception("Entries addition failed with status ", status.responsem)
            print("All the rules have been successfully added to the wcm table.")
            REPORTING.update_results(REPORTING.get_json(), test_name, "SUCCESS")
        except Exception as e:
            print(traceback.print_exc())
            REPORTING.update_results(REPORTING.get_json(), test_name, "FAILURE")
        finally:
            pass

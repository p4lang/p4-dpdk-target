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
import random
# import scapy.all as scapy
import scapy.layers.l2 as l2
import scapy.layers.inet as inet
from ctypes import *
from ptf.testutils import *
from ptf.packet import *
from ipaddress import IPv4Address
from switchd_base_test import *
from utils import REPORTING

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(THIS_DIR, '..'))
test_name = os.path.splitext(os.path.basename(__file__))[0]

class switchdVerifyWcmTraffic(SwitchdHelper):
    def setup(self):
        super(switchdVerifyWcmTraffic, self).setup()

    def form_packet(self, dst_mac=None, src_mac=None, dst_ip=None, src_ip=None, dst_port=None, src_port=None, payload=None):
        pkt = l2.Ether(dst=dst_mac, src=src_mac)/\
                inet.IP(dst=dst_ip, src=src_ip)/\
                inet.TCP(dport=dst_port, sport=src_port)
        pkt.add_payload(payload)
        return pkt

    def runTest(self):
        dst_mac = "00:00:00:00:03:14"
        src_mac = "9e:ba:ce:98:d9:d3"
        src_ip = "192.168.1.10"
        dst_port = 8888
        src_port = 7777
        #As per the rule config, we send traffic on port TAP0 and receive it on:
        #TAP1 for 192.168.1.X
        #TAP2 for 192.168.X.2
        #TAP3 for 192.X.168.3
        print("Start running the test.")
        try:
            print("We are going to to send the pkts.")
            for index in range(0, 3):
                counter = index + 1
                if counter == 1:
                    pkt = self.form_packet(dst_mac, src_mac, "192.168.1." + str(random.randrange(255)), src_ip, dst_port, src_port, bytes(str(counter), 'utf-8') * 50)
                elif counter == 2:
                    pkt = self.form_packet(dst_mac, src_mac, "192.168." + str(random.randrange(255)) + ".2", src_ip, dst_port, src_port, bytes(str(counter), 'utf-8') * 50)
                elif counter == 3:
                    pkt = self.form_packet(dst_mac, src_mac, "192." + str(random.randrange(255)) + ".168.3", src_ip, dst_port, src_port, bytes(str(counter), 'utf-8') * 50)
                print("sending traffic on port %d, should be received on port %d" % (0, counter))
                send(self, (0, 0), pkt)
                verify_packet(self, port_id=(0, counter), pkt=pkt)
            print("Traffic verification is successful.")
            REPORTING.update_results(REPORTING.get_json(), test_name, "SUCCESS")
        except Exception as e:
            print(traceback.print_exc())
            REPORTING.update_results(REPORTING.get_json(), test_name, "FAILURE")
        finally:
            pass

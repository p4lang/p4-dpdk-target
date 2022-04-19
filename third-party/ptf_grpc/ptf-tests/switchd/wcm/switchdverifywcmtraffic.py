import struct
import traceback

from ctypes import *

from ptf.testutils import *
from ptf.packet import *

from ipaddress import IPv4Address
import json

import scapy.all as scapy

from switchd_base_test import *
from utils import RESULT_STATUS
import os
import sys
import random

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(THIS_DIR, '..'))

class switchdVerifyWcmTraffic(SwitchdHelper):
    def setup(self):
        super(switchdVerifyWcmTraffic, self).setup()

    def form_packet(self, dst_mac=None, src_mac=None, dst_ip=None, src_ip=None, dst_port=None, src_port=None, payload=None):

        pkt = scapy.Ether(dst=dst_mac, src=src_mac)/\
                scapy.IP(dst=dst_ip, src=src_ip)/\
                scapy.TCP(dport=dst_port, sport=src_port)

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
            print(RESULT_STATUS.SUCCESS)

        except Exception as e:
            print("Exception occurred ")
            print(e)
            print(traceback.print_exc())
            print(RESULT_STATUS.FAILURE)
        finally:
            pass

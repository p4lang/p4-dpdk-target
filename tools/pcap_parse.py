#
# Copyright(c) 2021 Intel Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import sys
from scapy.all import *

def parse_single_packet(packet):
    ether_pkt = packet[Ether]
    print ("Source \t\t  ---> Destination")
    print (ether_pkt.src+  " ---> "+ ether_pkt.dst)
    print (ether_pkt.payload.src+":"+str(ether_pkt.sport)+" ---> "+ether_pkt.payload.dst+":"+str(ether_pkt.dport))
    print (ether_pkt.aliastypes)
    #print (packet[Ether].src, packet[Ether].dst)
    #print (packet[Ether].payload.dst,packet[Ether].payload.src)
    print (packet[TCP].load)
    #print (packet[TCP].sport,packet[TCP].dport)

def process_pcap(path):
    packets = scapy.utils.rdpcap(path)
    for packet in packets:
        if packet.haslayer(Raw):
            parse_single_packet(packet)
    return len(packets)

failed_test = 0
passed_test = 0
if process_pcap("tap1_recv.pcap") == 1:
    print ("TAP1 received one packet, Test Passed \n")
    passed_test += 1
else:
    print ("No Packet received, Test Failed \n")
    failed_test += 1
if process_pcap("tap2_recv.pcap") == 1:
    print ("TAP2 received one packet, Test Passed \n")
    passed_test += 1
else:
    print ("No Packet received, Test Failed \n")
    failed_test += 1
if process_pcap("tap3_drop.pcap") == 0:
    print ("TAP3 dropped the packets, Test Passed \n")
    passed_test += 1
else:
    print ("Oh No, TAP3 should not receive any packets;  Test Failed \n")
    failed_test += 1

if failed_test > 0:
    print ("One or more tests have failed, Please check the logs above")
    sys.exit(1)

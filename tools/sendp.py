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
from scapy.sendrecv import sniff, sendp
from scapy.all import *

class Port():
    def __init__(self, smac="9e:ba:ce:98:d9:d3", 
                       dmac="00:00:00:00:03:14", 
                       sip = "192.168.1.10", 
                       dip = "192.168.1.100",
                       sport = 7777, 
                       dport = 8888,
                       rawdata="0"*50
                       ):
        #self.ptype = ptype
        self.smac = smac
        self.dmac = dmac
        self.sip = sip
        self.dip = dip
        self.sport = sport
        self.dport = dport
        self.rawdata= rawdata
        self.ethport = "TAP0"

    def create_send_packet(self):
        print (self.__dict__)
        sendp(Ether(dst=self.dmac, src=self.smac)/IP(src=self.sip, dst=self.dip)/TCP(sport=self.sport, dport=self.dport)/Raw(load=self.rawdata), iface=self.ethport)
port = Port()
port.create_send_packet()
port = Port (smac="9e:ba:ce:98:d9:d3",sip = "192.168.1.10",sport = 7777,dport = 8888,rawdata="5"*50,dip = "192.168.1.101",dmac="00:00:00:00:03:16")
port.create_send_packet()
port = Port (smac="9e:ba:ce:98:d9:d3",sip = "192.168.1.10",sport = 7777,dport = 8888,rawdata="5"*50,dip = "192.168.1.200",dmac="00:00:00:00:03:18")
port.create_send_packet()
#sendp(Ether(dst="00:00:00:00:03:14", src="9e:ba:ce:98:d9:d3")/IP(src="192.168.1.10", dst="192.168.1.100")/TCP(sport=7777, dport=8888)/Raw(load="0"*50), iface='TAP0')

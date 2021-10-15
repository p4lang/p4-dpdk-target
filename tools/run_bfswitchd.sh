#!/bin/bash
##
## Copyright(c) 2021 Intel Corporation.
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
## http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##
if [[ "$#" -ne 1 ]]; then
	echo "Usage: $0 <SDE_PATH>" >&2
	exit 1
fi
#set -x
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    VER=$VERSION_ID
fi
curl https://bootstrap.pypa.io/pip/2.7/get-pip.py -o get-pip.py
python get-pip.py
export SDE=$1
export SDE_INSTALL=$SDE/install
#export PYTHONPATH=$SDE_INSTALL/lib/python3.4/:$SDE_INSTALL/lib/python3.4/lib-dynload
#export PYTHONHOME=$SDE_INSTALL/lib/python3.4/
echo "running for $OS"
if [[ "$OS" == "ubuntu" ]]; then
	export PKG_CONFIG_PATH="$SDE_INSTALL/lib/x86_64-linux-gnu/pkgconfig"
	export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SDE_INSTALL/lib/x86_64-linux-gnu/:$SDE_INSTALL/lib"

elif [[ "$OS" == "fedora" ]]; then
	export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SDE_INSTALL/lib64:$SDE_INSTALL/lib"
	export PKG_CONFIG_PATH=$SDE_INSTALL/lib/pkgconfig
fi
echo 16 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
l3_demo_conf=$SDE/p4_sde-nat-p4-driver/NAT_SDE_examples/DPDK/simple_l3/simple_l3_demo.conf
cp -r $SDE/p4_sde-nat-p4-driver/tools/p4testutils $SDE_INSTALL/lib/python2.7/site-packages/p4testutils
echo "Killing bf_switchd if alredy running..."
ps aux  |  grep -i bf_switchd  |  awk '{print $2}'  |  xargs kill -9
sleep 5
sed -i "s=share/dpdk=$SDE/p4_sde-nat-p4-driver/NAT_SDE_examples/DPDK=" $l3_demo_conf
$SDE_INSTALL/bin/bf_switchd --install-dir $SDE_INSTALL --conf-file $l3_demo_conf --init-mode=cold --status-port 7777 --skip-hld mkt &
./run_bfshell.sh -b $SDE/p4_sde-nat-p4-driver/tools/simple_l3.py
echo "Bringing up the created TAP ports..."
ip link set TAP0 up
ip link set TAP1 up
ip link set TAP2 up
ip link set TAP3 up
echo ""
echo "cleaning up the existing pcap files.."
rm *.pcap -v
#IP traffic only
tcpdump ip -s 0 -i TAP1 -w tap1_recv.pcap &
tcpdump ip -s 0 -i TAP2 -w tap2_recv.pcap &
tcpdump ip -s 0 -i TAP3 -w tap3_drop.pcap &
sleep 3
python3 sendp.py
sleep 2
echo "stopping the tcpdump sessions..."
ps aux  |  grep -i tcpdump  |  awk '{print $2}'  |  xargs kill
tshark -F pcap -r tap1_recv.pcap -w tap1_recv.pcap
tshark -F pcap -r tap2_recv.pcap -w tap2_recv.pcap
tshark -F pcap -r tap3_drop.pcap -w tap3_drop.pcap
echo "Analysing the packet captures"
python3 pcap_parse.py

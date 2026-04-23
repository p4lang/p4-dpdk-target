# SPDX-FileCopyrightText: 2021 Intel Corporation
# Copyright (C) 2021 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
from netaddr import IPAddress
port_table = bfrt.port.port
port_table.add(DEV_PORT=0, PORT_TYPE="BF_DPDK_TAP", PORT_DIR="PM_PORT_DIR_DEFAULT", PORT_IN_ID=0, PORT_OUT_ID =0, PIPE="pipe", MEMPOOL="MEMPOOL0", PORT_NAME="TAP0", MTU=1500)
port_table.add(DEV_PORT=1, PORT_TYPE="BF_DPDK_TAP", PORT_DIR="PM_PORT_DIR_DEFAULT", PORT_IN_ID=1, PORT_OUT_ID =1, PIPE="pipe", MEMPOOL="MEMPOOL0", PORT_NAME="TAP1", MTU=1500)
port_table.add(DEV_PORT=2, PORT_TYPE="BF_DPDK_TAP", PORT_DIR="PM_PORT_DIR_DEFAULT", PORT_IN_ID=2, PORT_OUT_ID =2,PIPE="pipe", MEMPOOL="MEMPOOL0", PORT_NAME="TAP2", MTU=1500)
port_table.add(DEV_PORT=3, PORT_TYPE="BF_DPDK_TAP", PORT_DIR="PM_PORT_DIR_DEFAULT", PORT_IN_ID=3, PORT_OUT_ID =3,PIPE="pipe", MEMPOOL="MEMPOOL0", PORT_NAME="TAP3", MTU=1500)
port_table.add(DEV_PORT=4, PORT_TYPE="BF_DPDK_SINK", PORT_DIR="PM_PORT_DIR_TX_ONLY", PORT_OUT_ID=4, PIPE="pipe",  PORT_NAME="SINK4",  FILE_NAME="none")

bfrt.simple_l3_demo.enable()
import sys
p4 = bfrt.simple_l3_demo.pipe
table = p4.ipv4_host
table.add_with_send(dst_addr=IPAddress('192.168.1.100'),   port=1)
table.get(dst_addr=IPAddress('192.168.1.100'))
table.add_with_send(dst_addr=IPAddress('192.168.1.101'),   port=2)
table.add_with_drop(dst_addr=IPAddress('192.168.1.200'))
bfrt.complete_operations()


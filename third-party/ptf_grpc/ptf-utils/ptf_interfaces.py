################################################################################
# BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
#
# Copyright (c) 2019-present Barefoot Networks, Inc.
#
# All Rights Reserved.
#
# NOTICE: All information contained herein is, and remains the property of
# Barefoot Networks, Inc. and its suppliers, if any. The intellectual and
# technical concepts contained herein are proprietary to Barefoot Networks, Inc.
# and its suppliers and may be covered by U.S. and Foreign Patents, patents in
# process, and are protected by trade secret or copyright law.  Dissemination of
# this information or reproduction of this material is strictly forbidden unless
# prior written permission is obtained from Barefoot Networks, Inc.
#
# No warranty, explicit or implicit is provided, unless granted under a written
# agreement with Barefoot Networks, Inc.
#
################################################################################

import sys
import os
import json
import subprocess
import sys
import six
cur_dir = os.path.dirname(os.path.realpath(__file__))

def check_and_execute_program(command):
    program = command[0]
    for path in os.environ["PATH"].split(os.pathsep):
        path = path.strip('"')
        exe_file = os.path.join(path, program)
        if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
            return subprocess.call(command)
    six.print_("Cannot find %s in PATH(%s)" % (program,
                                                         os.environ["PATH"]), file=sys.stderr)
    sys.exit(1)

def port_cfg_to_str(config):
    port_cfg_str = ""
    for k in config:
        port_cfg_str += ";" + str(k) + "=" + str(config[k])
    return port_cfg_str

def interface_list(args):
    intf_list = []
    port_list = {}
    device=0

    cleanscript = 'port_mapping_clean'
    check_and_execute_program([cleanscript])

    if "port_info" in args and os.path.isfile(args.port_info):
        connNode = 'PortConn'
        mapNode ='PortToVeth'
        ifMapNode ='PortToIf'
        port_connections = {}
        json_file = open(args.port_info)
        json_data = json.load(json_file)

        if (json_data.get(connNode)):
            noOfConn = len(json_data[connNode])
        else:
            noOfConn = 0

        if (json_data.get(mapNode)):
            noOfPortToVeth = len(json_data[mapNode])
        else:
             noOfPortToVeth = 0

        if (json_data.get(ifMapNode)):
            noOfIfMap = len(json_data[ifMapNode])
        else:
             noOfIfMap = 0

        for count in range(0,noOfPortToVeth):
            port_cfg = json_data[mapNode][count]
            port = port_cfg['device_port']
            veth1 = port_cfg['veth1']
            veth2 = port_cfg['veth2']
            port_list[port] = "veth%d" % veth2
            six.print_(port_list[port])
            port_list[port] += port_cfg_to_str(port_cfg)

        for count in range(0,noOfIfMap):
            port_cfg = json_data[ifMapNode][count]
            port = port_cfg['device_port']
            ifname = port_cfg['if']
            port_list[port] = ifname
            six.print_(port_list[port])
            port_list[port] += port_cfg_to_str(port_cfg)

        if noOfConn != 0:
            for count in range(0, noOfConn):
                port1 = json_data[connNode][count]['device_port1']
                port2 = json_data[connNode][count]['device_port2']
                port_connections[port1] = port2
                port_connections[port2] = port1
                veth_pair1 = port_list[port1]
                veth_pair2 = port_list[port2]
                setupscript = 'port_mapping_setup'
                check_and_execute_program([setupscript, str(port1), str(port2),
                                           veth_pair1, veth_pair2])

        for port in port_list:
            intf_str = "%d-%d@%s" % (device, port, port_list[port])
            if intf_str not in intf_list:
                intf_list += ["--interface", intf_str]
    else:
        base_port = 0
        for n_veth in range(int(args.max_ports)):
            port = n_veth
            intf_list += ["--interface", "%d-%d@veth%d" % (device, port+base_port, 2 * n_veth + 1)]

    if args.cpu_port != "None" and args.cpu_veth != "None":
        intf_str = "%d-%s@veth%s" % (device, args.cpu_port, args.cpu_veth)
        if intf_str not in intf_list:
            intf_list += ["--interface", intf_str]

    return intf_list

#!/usr/bin/env python

# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import os
import subprocess
import argparse
from distutils.spawn import find_executable

root_dir = '@abs_srcdir@'

parser = argparse.ArgumentParser()
parser.add_argument("--test-dir", required=True,
                    help="directory containing the tests")
parser.add_argument("--p4-name", required=False,
                    help="name of the P4 program")
parser.add_argument("--ptf", required=False,
                    default="ptf",
                    help="PTF executable (default: ptf)")
parser.add_argument("--db-prefix", required=False,
                    default="switch",
                    help="database prefix to use for test names")
parser.add_argument("--target", required=False, default="asic-model",
                    help="Target (asic-model or hw)")
parser.add_argument("--port-info", required=False,
                    default="None",
                    help="json file containing the port mapping")
parser.add_argument("--max-ports", required=False,
                    default="9",
                    help="max ports")
parser.add_argument("--cpu-port", required=False,
                    default="64",
                    help="cpu port")
parser.add_argument("--cpu-veth", required=False,
                    default="251",
                    help="cpu veth")
parser.add_argument("--drivers-test-info", required=False,
                    default="None",
                    help="json file containing test parameters for drivers-test")
parser.add_argument("--seed", required=False,
                    default="None",
                    help="Seed value to use for tests")
parser.add_argument("--num-pipes", required=False,
                    default=4,
                    help="Num of logical pipes to be used for testing")
parser.add_argument("--port-mode", required=False,
                    default="100G",
                    help="Port Speed")
parser.add_argument("--config-file", required=False,
                    default="filename",
                    help="config filename")
parser.add_argument("--thrift-server", required=False, default="localhost",
                    help="Thrift server IP address")
parser.add_argument("--grpc-server", required=False, default="localhost:50052",
                    help="GRPC server IP address")
parser.add_argument("--counter-byte-adjust", required=False, default=0,
                    help="Byte adjustment per packet for byte counters")
parser.add_argument("--use-pi", required=False,
                    default=False,
                    help="Use PI")
parser.add_argument("--setup", required=False, action="store_true",
                    help="Run only setUp for test")
parser.add_argument("--cleanup", required=False, action="store_true",
                    help="Run only teardown for test")
parser.add_argument("--traffic-gen", required=False,
                    default="None",
                    help="Traffic generator (ixia, scapy)")
parser.add_argument("--socket-recv-size", required=False, default="10240",
                    help="socket recv buffer size for ptf scapy verification")
parser.add_argument("--test-params", required=False, default="",
                    help="test params str")
parser.add_argument("--p4c", required=False, default="",
                    help="compiler version")
parser.add_argument("--reboot", required=False, default="None",
                    help="Type - hitless/fastreconfig")
parser.add_argument("--default-negative-timeout", required=False,
                    help="Timeout in seconds for negative checks")
parser.add_argument("--default-timeout", required=False,
                       help="Timeout in seconds for most operations")
parser.add_argument("--profile", action="store_true", required=False, default=False, help="Enable Python profiling")
args, unknown_args = parser.parse_known_args()

if __name__ == "__main__":
    import ptf_interfaces

    new_args = unknown_args
    my_dir = os.path.dirname(os.path.realpath(__file__))
    # Add the install location of this file to the import path.
    new_args += ["--pypath", my_dir]
    # Add the install location of python packages. This is where P4
    # program-specific PD Thrift files are installed.
    new_args += ["--pypath", os.path.join(my_dir, '..')]
    new_args += ["--test-dir", args.test_dir]
    cmd = ("set -e;"
           "prefix=/root/ws/p4sde/p4factory/install;"
           "echo /root/ws/p4sde/p4factory/install/lib/python3.6/site-packages;")
    site_packages_path = subprocess.check_output(cmd, shell=True).strip()

    # Adding bf-pktpy ptf folder to pythonpath to allow coexistence of scapy and bf-pktpy
    # After successful bf-pktpy transition only one PTF version will be kept
    if args.ptf == "bf-ptf":
        new_args += ["--pypath", os.path.join(site_packages_path.decode('UTF-8'),"bf-ptf")]

    new_args += ["--pypath", site_packages_path]

    if "--openflow" in args:
        new_args +=  ["-S 127.0.0.1", "-V1.3"]

    if "--failfast" in args:
        new_args += ["--failfast"]

    if args.profile is True:
        new_args += ["--profile"]

    #Example: 1@eth1 or 0-1@eth2 (use eth2 as port 1 of device 0)
    if args.port_info is not None and args.port_info != "None":
        import json
        file_handler = open(args.port_info)
        port_map = json.load(file_handler)
        ports = port_map['port_map']
        for key in ports.keys():
            new_args += ["-i", str(ports[key][0]) + "-" + str(ports[key][1]) + "@" + str(key)]
        #new_args += ["--interface", json.dumps(port_map['port_map'])]
    if args.port_info is None or args.port_info == "None":
        new_args += ["-i", "0-0@lo"]

    if args.default_negative_timeout is not None:
        new_args += ["--default-negative-timeout", args.default_negative_timeout]

    if args.default_timeout is not None:
        new_args += ["--default-timeout", args.default_timeout]

    #new_args += ptf_interfaces.interface_list(args)
    new_args += ["--socket-recv-size", args.socket_recv_size]

    kvp = "target='%s';" % (args.target.lower())
    kvp += "use_pi='%s';" % (args.use_pi)
    kvp += "traffic_gen='%s';" % (args.traffic_gen)
    if args.config_file != "":
        kvp += "config_file='%s';" % (args.config_file)
    kvp += "drivers_test_info='%s';" % (args.drivers_test_info)
    kvp += "test_seed='%s';" % (args.seed)
    kvp += "num_pipes='%s';" % (args.num_pipes)
    kvp += "port_mode='%s';" % (args.port_mode.lower())
    kvp += "counter_byte_adjust='%s';" % (args.counter_byte_adjust)
    kvp += "setup=%s;" % args.setup
    kvp += "cleanup=%s;" % args.cleanup
    kvp += "p4c='%s';" % args.p4c
    if args.test_params != "":
        kvp += args.test_params + ";"
    kvp += "thrift_server='%s';" % (args.thrift_server)
    kvp += "grpc_server='%s';" % (args.grpc_server)
    kvp += "reboot='%s';" % (args.reboot.lower())
    new_args += ["--test-params=%s" % (kvp)]

    if os.environ.get('GEN_XML_OUTPUT') == "1":
        script_dir = args.db_prefix
        xml_log_path = os.path.join(root_dir, 'xml_out', script_dir)
        new_args += ["--xunit", "--xunit-dir", xml_log_path]

    env = os.environ.copy()
    env["PYTHONPATH"] = site_packages_path.decode('UTF-8') + \
                        os.pathsep + os.environ.get("PYTHONPATH", "")

    # Adding bf-pktpy ptf folder to pythonpath to allow coexistence of scapy and bf-pktpy
    # After successful bf-pktpy transition only one PTF version will be kept
    if args.ptf == "bf-ptf":
        env["PYTHONPATH"] = os.path.join(site_packages_path.decode('UTF-8'),"bf-ptf") + \
                        os.pathsep + os.environ.get("PYTHONPATH", "")

    #ptf_path = find_executable(args.ptf)
    #child = subprocess.Popen([sys.executable, ptf_path] + new_args, env=env)
    child = subprocess.Popen([args.ptf] + new_args, env=env)
    child.wait()
    sys.exit(child.returncode)

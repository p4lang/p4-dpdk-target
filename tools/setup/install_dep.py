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
from sysutils import Platforms
import subprocess
import os
from typing import List, Tuple
from logger import initialize_logger
log = initialize_logger('setup')

def execute_system_command(command: List[str],
                           timeout: int = None,
                           stdin=None,
                           env=None,
                           cwd=None,
                           logs_size: int = 0) -> Tuple[str, int, str]:
    """
    Executes system's command
    :param command: command to be exeucted
    :param timeout: timeout of execution, when timeout pass - command is interrupted
    :param stdin: stream with input data for command
    :param env: environment within which command is run
    :param cwd: command working directory
    :param logs_size: if other than 0 - system sends to logger logs_size last characters
    :return: output - output of the command
             exit_code - exit code returned by a command
             log_output - output that should be passed to logs. If a real output contains
             special characters that are not present in a current system's encoding, this
             attribute contains information about a need of changing system's encoding
    """
    try:
        output = subprocess.check_output(  # type: ignore
            command,
            timeout=timeout,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            stdin=stdin,
            env=env,
            cwd=cwd,
            encoding='utf-8')
        encoded_output = output[-logs_size:].encode('utf-8')
        log.debug(f'COMMAND: {command} RESULT: {encoded_output}'.replace('\n', '\\n'))
    except subprocess.CalledProcessError as ex:
        log.exception(f'COMMAND: {command} RESULT: {ex.output}'.replace('\n', '\\n'))
        return ex.output, ex.returncode, ex.output
    else:
        return output, 0, encoded_output

ip_command = ["ip", "link", "show"]
lp = Platforms()
lp.get_all_details()

# git should be level 0 requirement. without git the server cant fetch the repo itself

apt_packages = ['git',
                'unifdef',
                #'python',
                'curl',
                #'python-setuptools',
                'python3-setuptools',
                'python3-pip',
                'python3-wheel',
                'python3-cffi',
                'libconfig-dev',
                'libunwind-dev',
                'libffi-dev',
                'zlib1g-dev',
                'libedit-dev',
                'libexpat1-dev',
                'clang',
                'ninja-build',
                'gcc',
                'libstdc++6',
                'autotools-dev',
                'autoconf',
                'autoconf-archive',
                'libtool',
                'meson',
                'google-perftools',
                'connect-proxy',
                'tshark',
                'cmake'
                ]

pip_packages = ['thrift', 'protobuf','pyelftools','scapy','six']

dnf_pkg = [ "clang", #libstdc++ is part of clang
            "gcc",
            "meson", #ninja build is included in meson
            "libtool", #autoconf, automake are part of it
            "google-perftools", # libunwind part of google perftools
            "libconfig",
            "unifdef",
            #"python",
            "curl",
            'libffi-devel',
            'zlib-devel',
            'libedit-devel',
            'expat-devel',
            "python3-setuptools",
            #"python-setuptools",
            "python3-pip",
            "python3-wheel",
            "python3-cffi",
            "autoconf-archive",
            "connect-proxy",
            "wireshark",
            "cmake",
            "libunwind-devel"
]


if lp.pkgmgr == "apt-get":
    for item in apt_packages:
        install_command = ["sudo", "-E", lp.pkgmgr, "install",  "-y", item]
        print (execute_system_command (install_command)[0])
if lp.pkgmgr == "dnf" or lp.pkgmgr == "yum":
    for item in dnf_pkg:
        install_command = ["sudo", "-E", lp.pkgmgr, "install",  "-y", item]
        print (execute_system_command (install_command)[0])

## setup proxies for pip to run
for item in pip_packages:
    pip3_install_command = ["pip3", "install",  item]
    #pip_install_command = ["pip", "install",  item]
    #pip2_install_command = ["pip2", "install",  item]
    print (execute_system_command (pip3_install_command)[0])

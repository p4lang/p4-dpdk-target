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
import os
from enum import Enum
import platform
from distutils.version import LooseVersion

try:
    from typing import List, Tuple
except ImportError:
    pass

using_distro = False
try:
    import distro
    using_distro = True
except ImportError:
    pass

class Platforms():
    def __init__(self, platform="linux"):

        self.platform = platform
        

    def get_python_versions(self):
        self.python_major = sys.version_info.major
        self.python_minor = sys.version_info.minor

    def get_os_version(self):
        system_str = self.platform
        if system_str == "linux":
            if using_distro:
                os_info = distro.info()
                self.linux_distro = distro.id().lower()
                self.os_version = LooseVersion(os_info["version"])
            else:
                os_info = platform.linux_distribution()
                self.linux_distro = platform.linux_distribution()[0].lower()
                self.os_version = LooseVersion(os_info[1])
        else:
            print ("UNSUPPORTED OS")
            sys.exit(1)

    def get_package_manager(self, linux_distribution):
        pkg_mgr = {
                    "fedora":"dnf", 
                    "centos": "yum",
                    "ubuntu" : "apt-get",
                    "debian" : "apt-get",
                    "redhat" : "yum"
                }
        self.pkgmgr= pkg_mgr[linux_distribution]

    def get_all_details(self):
        self.get_python_versions()
        self.get_os_version()
        self.get_package_manager(self.linux_distro)

if __name__ == "__main__":
    lp = Platforms("linux")
    lp.get_all_details()
    print (lp.__dict__)

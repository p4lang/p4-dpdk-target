#
# Copyright(c) 2022 Intel Corporation.
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
#!/bin/bash

# Set up hugepages used for DMA buffer allocation
ID=$(id -u)
if [ $ID != 0 ]; then
  echo "ERROR: Run this script as root or with sudo"
  exit 1
else
  sh -c 'echo "#Enable huge pages support for DMA purposes" >> /etc/sysctl.conf'
  sh -c 'echo "vm.nr_hugepages = 128" >> /etc/sysctl.conf'
  sysctl -p /etc/sysctl.conf
  mkdir /mnt/huge
  mount -t hugetlbfs nodev /mnt/huge
fi

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
#!/usr/bin/env bash

noOfVeths=64
if [ $# -eq 1 ]; then
    noOfVeths=$1
fi
echo "No of Veths is $noOfVeths"

set -e

idx=0
while [ $idx -lt $noOfVeths ]
do
    intf="veth$(($idx*2))"
    if ip link show $intf &> /dev/null; then
        ip link delete $intf type veth
    fi
    idx=$((idx + 1))
done
echo "Removing CPU veth"
idx=125
intf="veth$(($idx*2))"
if ip link show $intf &> /dev/null; then
    ip link delete $intf type veth
fi

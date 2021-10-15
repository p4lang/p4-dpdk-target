# Copyright (c) 2021 Intel Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#! /bin/bash
set -e

if [ -z "$1" ]
then
    echo "- Missing mandatory argument:"
    echo " - Usage: source p4sde_env_setup.sh <SDE - top level directory with syslibs, utils and this p4-dpdk-target repo>"
    exit 1
fi

export SDE=$1
export SDE_INSTALL=$SDE/install
export LD_LIBRARY_PATH=$SDE_INSTALL/lib:$SDE_INSTALL/lib64:$SDE_INSTALL/lib/x86_64-linux-gnu/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib64
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

echo ""
echo "Updated Environment Variables ..."
echo "SDE: $SDE"
echo "SDE_INSTALL: $SDE_INSTALL"
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
echo ""

set +e

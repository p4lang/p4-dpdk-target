# Copyright (c) 2022 Intel Corporation.
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

DISABLE_PARALLEL_COMPILE=""
SDE_INSTALL_PATH=""

usage() {
    echo " - Usage: ./build-p4sde.sh -s <SDE_INSTALL_PATH> [-d <DISABLE_PARALLEL_COMPILE>]"
    echo "Example:"
    echo "dpdk:        ./build-p4sde.sh -s /root/p4sde/install/"
    exit 1
}

while getopts ':s:d' OPTION; do
    case "$OPTION" in
        s)
            SDE_INSTALL_PATH="$OPTARG"
            echo "SDE Install path is $SDE_INSTALL_PATH"
            ;;
        d)
            DISABLE_PARALLEL_COMPILE=1
            echo "Disabled parallel compilation"
            ;;
        :)
            echo "Error: -${OPTARG} requires an argument."
            usage
            ;;
        *)
            usage
            ;;
    esac
done

if [ -z $SDE_INSTALL_PATH ]
then
    echo "Missing mandatory argument -s"
    usage
fi

#Read the number of CPUs in a system and derive the NUM threads
if [ ! -z "$DISABLE_PARALLEL_COMPILE" ]
then
    NUM_THREADS=""
else
    NUM_THREADS="-j";
    echo "Number of Parallel threads used: $NUM_THREADS"
fi
export ENABLE_PARALLEL_COMPILE="$NUM_THREADS"

# p4sde build process starts here
./autogen.sh
./configure --prefix=$SDE_INSTALL_PATH

make $NUM_THREADS || exit 1
make $NUM_THREADS install || exit 1

set +e

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
if [[ "$@" == *--help* ]]; then
    cat<<EOF
 =================== Build script for P4 SDE============================
 This script builds the resultant binaries and libs for the P4-SDE
 including its dependencies
 ==================== Help End =========================================
EOF
    exit 0;
fi

build_utils() {
    cd $SDE
    tar -C $SDE -czf $SDE/$1.tar.gz $1
    cd $SDE/$1
    rm -rf build
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=$SDE_INSTALL -DCPYTHON=1 -DSTANDALONE=ON ..
    make -j4
    make install
}

build_syslibs() {
    cd $SDE
    tar -C $SDE -czf $SDE/$1.tar.gz $1
    cd $SDE/$1
    rm -rf build
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=$SDE_INSTALL ..
    make -j4
}

build_driver() {
    cd $SDE
    tar -C $SDE -czf $SDE/$1.tar.gz $1
    cd $SDE/$1
    ./autogen.sh
    ./configure --prefix=$SDE_INSTALL
    make -j4
    make install
}

main() {
    PWD=`pwd`
    export SDE=$PWD/p4-sde
    mkdir -p $SDE
    export SDE_INSTALL=$SDE/install
    mkdir -p $SDE_INSTALL
    build_syslibs syslibs
    build_utils utils
    build_driver p4dpdk-driver
    cd $SDE
    tar -C $SDE -czf $SDE/install.tar.gz install
}
main

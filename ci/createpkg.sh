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
SYSLIBS="bf-syslibs"
UTILS="bf-utils"
P4DRIVER="p4_sde-nat-p4-driver"
REL_VER="9.7.0"
clean_up() {
    cd $1
    git clean -xfd
    git submodule update
    for sub in $(git submodule | cut -d' ' -f3); do
        ( cd $sub; git clean -xfd )
    done
    rm -rf .git
    cd ..
}

clean_up ${SYSLIBS}
clean_up ${UTILS}
clean_up ${P4DRIVER}

package_name=$1
mkdir -p p4-sde
tar -czvf "bf-syslibs-$REL_VER.tar.gz" $SYSLIBS
tar -czvf "bf-utils-$REL_VER.tar.gz" $UTILS
tar -czvf "p4-driver-$REL_VER.tar.gz" $P4DRIVER
mv *tar.gz p4-sde/.
tar -czvf "${package_name}.tar.gz" p4-sde


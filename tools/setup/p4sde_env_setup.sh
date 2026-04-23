# SPDX-FileCopyrightText: 2021 Intel Corporation
# Copyright (C) 2021 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
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

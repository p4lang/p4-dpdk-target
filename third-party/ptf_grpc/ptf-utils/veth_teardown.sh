# Copyright (C) 2022 Intel Corporation.
# SPDX-FileCopyrightText: 2022 Riyaz Ahmad
#
# SPDX-License-Identifier: Apache-2.0

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

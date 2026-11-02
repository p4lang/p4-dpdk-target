#!/bin/bash

# Copyright (C) 2021 Intel Corporation.
# SPDX-FileCopyrightText: 2021 James Choi
#
# SPDX-License-Identifier: Apache-2.0
for idx in 0 1 2 3; do
    intf="veth$(($idx*2))"
    if ip link show $intf &> /dev/null; then
        ip link delete $intf type veth
    fi
done

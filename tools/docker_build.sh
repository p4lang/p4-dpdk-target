#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2023 Bili Dong
#
# SPDX-License-Identifier: Apache-2.0

#
# Build a Docker image for the P4 DPDK stack.

IMAGE_NAME=p4lang/p4-dpdk-target

docker build -t $IMAGE_NAME .

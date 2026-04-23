#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2023 Bili Dong
#
# SPDX-License-Identifier: Apache-2.0

#
# Run a new bash in a running container.

CONTAINER_NAME=p4-dpdk-target

docker exec -it $CONTAINER_NAME bash

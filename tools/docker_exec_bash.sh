#!/usr/bin/env bash
#
# Run a new bash in a running container.

CONTAINER_NAME=p4-dpdk-target

docker exec -it $CONTAINER_NAME bash

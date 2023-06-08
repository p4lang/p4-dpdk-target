#!/usr/bin/env bash
#
# Build a Docker image for the P4 DPDK stack.

IMAGE_NAME=p4lang/p4-dpdk-target

docker build -t $IMAGE_NAME .

#!/usr/bin/env bash

IMAGE_NAME=p4lang/p4-dpdk-target
CONTAINER_NAME=p4-dpdk-target
MOUNT_DIR=/home/work

docker run -it --rm --privileged \
    --name $CONTAINER_NAME \
    -v /dev/hugepages:/dev/hugepages \
    -v "$PWD":$MOUNT_DIR -w $MOUNT_DIR \
    $IMAGE_NAME bash

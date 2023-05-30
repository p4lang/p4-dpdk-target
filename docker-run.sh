#!/usr/bin/env bash

DOCKER_IMAGE=p4-dpdk-target
DOCKER_MOUNTDIR=/home/work

docker run -it --rm --privileged \
    -v /dev/hugepages:/dev/hugepages \
    -v "$PWD":$DOCKER_MOUNTDIR -w $DOCKER_MOUNTDIR \
    $DOCKER_IMAGE bash

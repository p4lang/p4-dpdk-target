#!/usr/bin/env bash

DOCKER_IMAGE=p4-dpdk-target

docker build -t $DOCKER_IMAGE .

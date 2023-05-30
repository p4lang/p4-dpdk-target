#!/usr/bin/env bash

OUTPUT_DIR=pipe

rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

p4c-dpdk pipe.p4 \
    --arch pna \
    --p4runtime-files $OUTPUT_DIR/p4info.txt \
    --bf-rt-schema $OUTPUT_DIR/bfrt.json \
    --tdi $OUTPUT_DIR/tdi.json \
    --context $OUTPUT_DIR/context.json \
    -o $OUTPUT_DIR/config.spec

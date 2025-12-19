
#!/bin/bash
p4c-dpdk simple_l2l3_lpm.p4 \
    -o simple_l2l3_lpm.spec \
    --arch psa \
    --bf-rt-schema simple_l2l3_lpm.bfrt.json \
    --context context.json \
    --p4runtime-files simple_l2l3_lpm.pb.txt \
    --p4runtime-format text

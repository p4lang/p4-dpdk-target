tdi_python

# Add ports

for port_id in range(4):
    tdi.port.port.add(
        DEV_PORT=port_id,
        PORT_TYPE="BF_DPDK_TAP",
        PORT_DIR="PM_PORT_DIR_DEFAULT",
        PORT_IN_ID=port_id,
        PORT_OUT_ID =port_id,
        PIPE_IN="pipe",
        PIPE_OUT="pipe",
        MEMPOOL="MEMPOOL0",
        PORT_NAME=f"TAP{port_id}",
        MTU=1500
    )

# Add entries

from netaddr import IPAddress

control = tdi.main.pipe.MainControl
table = control.forwarding
table.add_with_mark_and_forward(
    dst_addr=IPAddress('192.168.1.101'),
    marker=0x01,
    port=1
)
table.add_with_mark_and_forward(
    dst_addr=IPAddress('192.168.1.102'),
    marker=0x02,
    port=2
)

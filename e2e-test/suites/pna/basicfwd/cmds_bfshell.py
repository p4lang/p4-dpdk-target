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

# Enable TDI program
tdi.main.enable()

# Add entries
control = tdi.main.pipe.MainControl
table = control.forwarding
table.add_with_forward(input_port=0, output_port=1)
table.add_with_forward(input_port=1, output_port=0)
table.add_with_forward(input_port=2, output_port=3)
table.add_with_forward(input_port=3, output_port=2)

# TODO: Replace the following hack with something better.
foo = "fini"
bar = "shed"
print(foo + bar)

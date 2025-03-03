# Makefile for P4 DPDK MAC Modifier example

# Define variables
P4C = p4c-dpdk
P4_SRC = mac_modifier.p4
P4_JSON = mac_modifier.json
P4_CLI = mac_modifier_cli.txt
P4_DPDK_TARGET = mac_modifier.spec

CC = gcc
CFLAGS = -Wall -Werror $(shell pkg-config --cflags libp4rt)
LDFLAGS = -lrte_eal -lrte_mempool -lrte_mbuf -lrte_ethdev $(shell pkg-config --libs libp4rt)
CONTROL_SRC = table_manager.c
CONTROL_BIN = table_manager

# Default target
all: p4_compile control_compile

# Compile P4 program
p4_compile:
	$(P4C) --arch psa --target dpdk -o $(P4_DPDK_TARGET) $(P4_SRC)

# Compile control plane program
control_compile:
	$(CC) $(CFLAGS) -o $(CONTROL_BIN) $(CONTROL_SRC) $(LDFLAGS)

# Clean generated files
clean:
	rm -f $(P4_JSON) $(P4_DPDK_TARGET) $(CONTROL_BIN)

.PHONY: all p4_compile control_compile clean
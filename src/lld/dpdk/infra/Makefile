# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2020 Intel Corporation

# library name
LIB_NAME = dpdk_infra

# all source are stored in SRCS-y
SRCS-y += dpdk_cli.c
SRCS-y += dpdk_infra.c
SRCS-y += dpdk_obj.c
SRCS-y += dpdk_thread.c
SRCS-y += dpdk_conn.c
PC_FILE := $(shell find $(install_dir) -name libdpdk.pc)
PKG_CONFIG_PATH := $(shell dirname $(PC_FILE))

PKGCONF ?= pkg-config

CFLAGS += -O3 -fPIC $(shell PKG_CONFIG_PATH='$(PKG_CONFIG_PATH)' $(PKGCONF) --cflags libdpdk)
LDFLAGS = $(shell PKG_CONFIG_PATH='$(PKG_CONFIG_PATH)' $(PKGCONF) --libs libdpdk)
#This is needed as we are compiling a library on top of shared library
#as-needed will strip of all unused libraries
STRING = -Wl,--as-needed
LDFLAGS := $(filter-out $(STRING),$(LDFLAGS))

CFLAGS += -I. -DALLOW_EXPERIMENTAL_API -D_GNU_SOURCE

OBJS := $(patsubst %.c,build/%.o,$(SRCS-y))

build/%.o: %.c Makefile $(PC_FILE)
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean all
all: $(OBJS)
	$(CC) $(OBJS) -shared -o build/$(LIB_NAME).so $(LDFLAGS)

clean:
	rm -f build/$(LIB_NAME)* build/*.o
	test -d build && rmdir -p build || true

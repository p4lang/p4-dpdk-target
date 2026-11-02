/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*!
 * @file lld_dpdk_lib.h
 * @date
 *
 * Definitions for interfaces to DPDK model
 */

#ifndef _LLD_DPDK_LIB_H
#define _LLD_DPDK_LIB_H

#include <osdep/p4_sde_osdep.h>
#include <dvm/bf_drv_intf.h>

enum mat_type {
	VXLAN_ENCAP = 1,
	SIMPLE_L3   = 2,
};

enum dpdk_init_err {
	SOCK_CREATE_FAIL = -2,
	SOCK_CONNECT_FAIL,
	SOCK_CONNECT_SUCCESS,
};

struct simple_l3_mav {
	u32 match_key;
	u32 fwd_port;
};

#define PORT_MASK_DPDK 0x1FF
#define UINT32_MAX_VAL 0xFFFFFFFF
#define BUFF_SIZE 1024

int lld_dpdk_init(bf_device_profile_t *profile);
int lld_dpdk_pipeline_enable(void);
int lld_dpdk_pipeline_disable(void);
void lld_dpdk_table_rule_dump(char *pipeline, char *table, char *file_name);
void lld_dpdk_table_stats(char *pipeline, char *table);
void lld_dpdk_pipeline_build(char *pipeline, char *specfile);
#endif

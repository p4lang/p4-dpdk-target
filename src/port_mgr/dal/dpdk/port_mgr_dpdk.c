/*
 * Copyright (C) 2021 Intel Corporation
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*!
 * @file port_mgr_dpdk.c
 *
 *
 * Definitions for DPDK Port Manager APIs.
 */

#include <stdio.h>

void port_mgr_platform_init(void)
{
	printf("STUB:%s DPDK\n", __func__);
}

void port_mgr_platform_cleanup(void)
{
	printf("STUB:%s DPDK\n", __func__);
}

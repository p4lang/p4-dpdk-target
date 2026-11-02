/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bf_types/bf_types.h>
#include <port_mgr/port_mgr_intf.h>
/**
 * @file bf_port_if.c
 * \brief Details Port-level APIs.
 *
 */

/*****************************************************************************
 * bf_port_mgr_init
 *****************************************************************************/
bf_status_t bf_port_mgr_init(void)
{
	port_mgr_init();
	return BF_SUCCESS;
}

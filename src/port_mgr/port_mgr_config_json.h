/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*!
 * @file port_mgr_config_json.h
 *
 *
 * Functions for DPDK Port Manager Config Parsing
 */

#ifndef __PORT_MGR_CONFIG_JSON_H__
#define __PORT_MGR_CONFIG_JSON_H__
#include <dvm/bf_drv_profile.h>

int port_mgr_config_import(int dev_id,
			   struct bf_device_profile *dev_profile);
#endif /*__PORT_MGR_CONFIG_JSON_H__*/


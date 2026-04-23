/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*!
 * @file port_mgr_dev.c
 *
 *
 * Functions for Port Manager Device actions
 */

#include "port_mgr/port_mgr_dev.h"
#include "port_mgr/port_mgr_config_json.h"
#include "port_mgr/port_mgr_log.h"
#include "port_mgr/port_mgr_intf.h"

bf_status_t port_mgr_dev_add(bf_dev_id_t dev_id,
			     bf_dev_family_t dev_family,
			     bf_device_profile_t *profile,
			     bf_dev_init_mode_t warm_init_mode)
{
	int status = BF_SUCCESS;

	port_mgr_log_trace("Entering %s", __func__);

	status = port_mgr_config_import(dev_id, profile);
	if (status != BF_SUCCESS) {
		port_mgr_log_trace("Exiting %s", __func__);
		return status;
	}

	return status;
}

bf_status_t port_mgr_dev_remove(bf_dev_id_t dev_id)
{
	port_mgr_log_trace("Entering %s", __func__);
	return BF_SUCCESS;
}

/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "port_mgr/dal/dal_port_mgr.h"
#include "port_mgr/port_mgr_log.h"

bf_status_t dal_port_cfg_table_add(bf_dev_id_t dev_id,
                                   struct fixed_function_key_spec *key,
                                   struct fixed_function_data_spec *data,
                                   struct fixed_function_table_ctx *tbl_ctx)
{
	port_mgr_log_trace("STUB: %s", __func__);
	return BF_SUCCESS;
}

bf_status_t dal_port_stats_get(bf_dev_id_t dev_id,
                               struct fixed_function_key_spec *key,
                               struct fixed_function_data_spec *data,
                               struct fixed_function_table_ctx *tbl_ctx)
{
	port_mgr_log_trace("STUB: %s", __func__);
	return BF_SUCCESS;
}

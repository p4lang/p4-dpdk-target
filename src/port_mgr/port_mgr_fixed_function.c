/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "port_mgr/dal/dal_port_mgr.h"
#include "port_mgr/port_mgr_log.h"

bf_status_t port_cfg_table_add(bf_dev_id_t dev_id,
                               struct fixed_function_key_spec *key,
                               struct fixed_function_data_spec *data)
{
	struct fixed_function_table_ctx *tbl_ctx = NULL;
	struct fixed_function_mgr_ctx *ff_ctx = NULL;

	ff_ctx = get_fixed_function_mgr_ctx(FF_MGR_PORT);
	if (!ff_ctx)
		return BF_INVALID_ARG;

	tbl_ctx = get_fixed_function_table_ctx(ff_ctx,
				FIXED_FUNCTION_TABLE_TYPE_CONFIG);
	if (!tbl_ctx)
		return BF_INVALID_ARG;

	return dal_port_cfg_table_add(dev_id, key, data, tbl_ctx);
}

bf_status_t port_all_stats_get(bf_dev_id_t dev_id,
			       struct fixed_function_key_spec *key,
			       struct fixed_function_data_spec *data)
{
	struct fixed_function_table_ctx *tbl_ctx = NULL;
	struct fixed_function_mgr_ctx   *ff_ctx  = NULL;

	/* Get the Port fixed function context Info from Hash Map */
	ff_ctx = get_fixed_function_mgr_ctx(FF_MGR_PORT);
	if (!ff_ctx)
		return BF_INVALID_ARG;

	/* Get relevant table context info */
	tbl_ctx = get_fixed_function_table_ctx(ff_ctx,
				               FIXED_FUNCTION_TABLE_TYPE_STATE);

	if (!tbl_ctx)
		return BF_INVALID_ARG;

	return dal_port_stats_get(dev_id, key, data, tbl_ctx);
}

/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*!
 * @ file dal_parse.h
 * date
 *
 * parsing functions to target specific attributes.
 */
#ifndef __DAL_PARSE_H__
#define __DAL_PARSE_H__

#include <ctx_json/ctx_json_utils.h>

int dal_parse_ctx_json_parse_stage_tables
	(int dev_id, int prof_id,
	cJSON *stage_table_list_cjson,
	void **stage_table,
	int *stage_table_count,
	struct pipe_mgr_mat_ctx *mat_ctx);

int dal_parse_ctx_json_parse_value_lookup_stage_tables(int dev_id, int prof_id,
						       cJSON *stage_table_list_cjson,
						       void **stage_table,
						       int *stage_table_count,
						       struct pipe_mgr_value_lookup_ctx *val_lookup_ctx);

int dal_ctx_json_parse_global_config(int dev_id, int prof_id,
		cJSON *root, void **dal_global_config);

int dal_post_parse_processing(int dev_id, int prof_id,
		struct pipe_mgr_p4_pipeline *ctx);

int dal_ctx_json_parse_extern(int dev_id,
			      int prof_id,
			      cJSON *root,
			      struct pipe_mgr_p4_pipeline *ctx);
#endif

/*
 * Copyright(c) 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

int dal_ctx_json_parse_global_config(int dev_id, int prof_id,
		cJSON *root, void **dal_global_config);

int dal_post_parse_processing(int dev_id, int prof_id,
		struct pipe_mgr_p4_pipeline *ctx);

#endif

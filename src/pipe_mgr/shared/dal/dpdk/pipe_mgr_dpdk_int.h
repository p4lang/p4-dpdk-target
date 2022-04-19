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
 * @ file pipe_mgr_dpdk_int.h
 * date
 *
 * Internal definitions of DPDK for pipe_mgr
 */
#ifndef __PIPE_MGR_DPDK_INT_H__
#define __PIPE_MGR_DPDK_INT_H__

#include <rte_swx_pipeline.h>
#include <rte_swx_ctl.h>

#include "pipe_mgr/shared/pipe_mgr_mat.h"
#include "pipe_mgr/shared/infra/pipe_mgr_int.h"

#define PIPE_MGR_DPDK_STR_SIZE 1024

struct pipe_mgr_dpdk_immediate_fields {
	char    param_name[P4_SDE_NAME_LEN];
	char    param_type[P4_SDE_NAME_LEN];
	uint32_t    param_shift;
	uint32_t    dest_start;
	uint32_t    dest_width;
	struct pipe_mgr_dpdk_immediate_fields *next;
};

struct  pipe_mgr_dpdk_action_format {
	char    action_name[P4_SDE_NAME_LEN];
	char    target_action_name[P4_SDE_NAME_LEN];
	uint32_t    action_handle;
	int     immediate_fields_count;
	struct pipe_mgr_dpdk_immediate_fields *immediate_field;
	/* dpdk action information */
	uint32_t action_id;
	struct rte_swx_ctl_action_info n_args;
	/* dpdk action arg info array*/
	struct rte_swx_ctl_action_arg_info *arg_info;
	uint32_t arg_nbits;
	struct  pipe_mgr_dpdk_action_format *next;
};

struct dal_dpdk_table_metadata {
	struct pipeline *pipe;
	uint32_t table_id;
	struct rte_swx_ctl_table_info dpdk_table_info;
	/* table match field info array */
	struct rte_swx_ctl_table_match_field_info *mf;
	enum pipe_mgr_match_type match_type;
	uint32_t match_field_nbits;
	uint32_t action_data_size;
};

struct pipe_mgr_dpdk_stage_table {
	int     action_format_count;
	struct  pipe_mgr_dpdk_action_format *act_fmt;
	/* dpdk table metadata to store table info */
	struct dal_dpdk_table_metadata *table_meta;
	struct pipe_mgr_dpdk_stage_table *next;
};


int dal_dpdk_table_metadata_get(struct pipe_mgr_mat_ctx *mat_ctx,
		char *pipeline_name, char *table_name, bool adt_table);

int table_entry_alloc(struct rte_swx_table_entry **ent,
		struct dal_dpdk_table_metadata *meta,
		int match_type);
#endif

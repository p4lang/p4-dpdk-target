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
 * @ file pipe_mgr_dpdk_ctx_util.h
 * date
 *
 *  Utilities to retrieve DPDK specific ctx objects
 */
#ifndef __PIPE_MGR_DPDK_CTX_UTIL_H__
#define __PIPE_MGR_DPDK_CTX_UTIL_H__

#include <infra/dpdk_infra.h>
#include "pipe_mgr_dpdk_int.h"
#include "../../infra/pipe_mgr_int.h"

int pipe_mgr_ctx_dpdk_get_act_fmt(
		struct pipe_mgr_dpdk_stage_table *stage_tbl,
		u32 act_fn_hdl,
		struct pipe_mgr_dpdk_action_format **act_fmt);
int pipe_mgr_dpdk_encode_match_key_and_mask(
		struct pipe_mgr_mat_ctx *mat_ctx,
		struct pipe_tbl_match_spec *match_spec,
		struct rte_swx_table_entry *entry);
int pipe_mgr_dpdk_encode_action_data(
		struct  pipe_mgr_dpdk_action_format *act_fmt,
		u8 *action_data_bits,
		struct rte_swx_table_entry *entry);
int pipe_mgr_dpdk_encode_adt_action_data(
		struct  pipe_mgr_dpdk_action_format *act_fmt,
		u8 *action_data_bits,
		struct rte_swx_table_entry *entry);
int pipe_mgr_dpdk_encode_sel_action(char *table_name,
		struct pipeline *pipe,
		struct pipe_action_spec *act_data_spec,
		struct rte_swx_table_entry *entry);
int pipe_mgr_dpdk_encode_member_id(char *table_name,
		struct pipeline *pipe,
		struct pipe_action_spec *act_data_spec,
		struct rte_swx_table_entry *entry);
#endif

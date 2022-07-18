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
#include "pipe_mgr/shared/pipe_mgr_value_lookup.h"

#define PIPE_MGR_DPDK_FILL_DATA_FROM_IMMEDIATE_FIELD_AND_DATA_SPEC(im, data, field)              \
	do {                                                                                     \
		if (im) {                                                                        \
			status = pipe_mgr_dpdk_encode_param_from_data_spec(im, data, &val_ptr,   \
									   &num_bytes);          \
			if (status)                                                              \
				goto cleanup;                                                    \
			switch (num_bytes) {                                                     \
			case 1:                                                                  \
				memcpy(&val8, val_ptr, num_bytes);                               \
				field = val8;                                                    \
				val8 = 0;                                                        \
				break;                                                           \
			case 2:                                                                  \
				memcpy(&val16, val_ptr, num_bytes);                              \
				val16 = BE16_TO_CPU(val16);                                      \
				field = val16;                                                   \
				val16 = 0;                                                       \
				break;                                                           \
			case 4:                                                                  \
				memcpy(&val32, val_ptr, num_bytes);                              \
				val32 = BE32_TO_CPU(val32);                                      \
				field = val32;                                                   \
				val32 = 0;                                                       \
				break;                                                           \
			case 8:                                                                  \
				memcpy(&val64, val_ptr, num_bytes);                              \
				val64 = BE64_TO_CPU(val64);                                      \
				field = val64;                                                   \
				val64 = 0;                                                       \
				break;                                                           \
			default:                                                                 \
				LOG_ERROR("Does not support %d bytes.", num_bytes);              \
				P4_SDE_FREE(val_ptr);                                            \
				status = BF_NOT_SUPPORTED;                                       \
				goto cleanup;                                                    \
			}                                                                        \
			P4_SDE_FREE(val_ptr);                                                    \
			im = im->next;                                                           \
		} else {                                                                         \
			LOG_DBG("No immediate fields to decode and fill values.");               \
		}                                                                                \
	} while (0);                                                                             \

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
int pipe_mgr_dpdk_encode_param_from_data_spec(struct pipe_mgr_dpdk_immediate_fields *immediate_field,
					      struct pipe_data_spec *data_spec,
					      u8 **param_ptr,
					      u16 *num_bytes);
int pipe_mgr_dpdk_encode_key_from_match_spec(struct pipe_tbl_match_spec *match_spec,
					     u8 **param_ptr,
					     u16 *num_bytes);

#endif

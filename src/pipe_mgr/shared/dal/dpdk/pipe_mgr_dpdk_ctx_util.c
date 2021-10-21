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
#include <rte_swx_pipeline.h>
#include <rte_swx_ctl.h>
#include <bf_types/bf_types.h>
#include <osdep/p4_sde_osdep.h>

#include "../../../core/pipe_mgr_log.h"

#include "pipe_mgr_dpdk_ctx_util.h"

#include "../dal_mat.h"
#include <lld_dpdk_lib.h>
#include <infra/dpdk_infra.h>
#include "pipe_mgr_dpdk_int.h"

int pipe_mgr_dpdk_encode_match_key_and_mask(
		struct pipe_mgr_mat_ctx *mat_ctx,
		struct pipe_tbl_match_spec *match_spec,
		struct rte_swx_table_entry *entry)
{

	struct rte_swx_ctl_table_match_field_info *mf;
	struct pipe_mgr_dpdk_stage_table *stage_table;
	struct pipe_mgr_match_key_fields *match_fields;
	struct dal_dpdk_table_metadata *meta;
	int status = BF_SUCCESS;
	uint32_t offset;
	int match_bytes;
	uint64_t val;
	int n_match;
	int index_2;
	int index;
	int i;

	stage_table = mat_ctx->match_attr.stage_table;
	if (!stage_table) {
		LOG_ERROR("dpdk encode match key and mask error stage table"
				" is null");
		return BF_UNEXPECTED;
	}

	meta = stage_table->table_meta;
	if (!meta) {
		LOG_ERROR("dpdk encode match key and mask error meta is null");
		return BF_UNEXPECTED;
	}

	match_fields = mat_ctx->mat_key_fields;

	n_match = meta->dpdk_table_info.n_match_fields;
	if (n_match != mat_ctx->mat_key_fields_count) {
		LOG_ERROR("dpdk encode match key and mask error meta n_match"
				" and match key fields count don't match");
		return BF_UNEXPECTED;
	}

	index_2 = 0;
	index = 0;
	for (i = 0; i < n_match; i++) {
		if (!match_fields) {
			LOG_ERROR("dpdk encode match key and mask error"
					" match_fields is null");
			return BF_UNEXPECTED;
		}
		mf = &meta->mf[i];
		offset = (mf->offset - meta->mf[0].offset) >> 3;
		match_bytes = (mf->n_bits >> 3) + (mf->n_bits % 8 != 0);

		/* encode key value */
		memcpy(&val, &(match_spec->match_value_bits[index]),
					sizeof(uint64_t));
		index += match_bytes;

		if (!mf->is_header)
			val = dpdk_field_hton(val, mf->n_bits);

		/* Copy to entry. */
		memcpy(&entry->key[offset], (uint8_t *)&val,
				match_bytes);

		if (match_fields->match_type != PIPE_MGR_MATCH_TYPE_EXACT) {
			/* encode key mask */
			memcpy(&val, &(match_spec->match_mask_bits[index_2]),
					sizeof(uint64_t));
			index_2 += sizeof(uint64_t);
			/* Copy to entry. */
			memcpy(&entry->key_mask[offset], (uint8_t *)&val,
					match_bytes);
		}
		match_fields = match_fields->next;
	}

	entry->key_priority = match_spec->priority;

	return status;
}

int pipe_mgr_dpdk_encode_adt_action_data(
		struct  pipe_mgr_dpdk_action_format *act_fmt,
		u8 *action_data_bits,
		struct rte_swx_table_entry *entry)
{
	struct rte_swx_ctl_action_arg_info *arg;
	uint32_t arg_offset = 0;;
	int status = BF_SUCCESS;
	uint32_t nbytes;
	uint64_t val;
	uint32_t i;

	entry->action_id = act_fmt->action_id;

	for (i = 0; i < act_fmt->n_args.n_args; i++) {
		arg = &act_fmt->arg_info[i];
		nbytes = (arg->n_bits >> 3) + (arg->n_bits % 8 != 0);

		val = 0;
		memcpy(&val, &(action_data_bits[arg_offset]),
				nbytes);

		val = dpdk_field_hton(val, arg->n_bits);

		memcpy(&entry->action_data[arg_offset],
				(uint8_t *)&val,
				nbytes);

		arg_offset += nbytes;
	}

	return status;
}

int pipe_mgr_dpdk_encode_sel_action(char *table_name,
		struct pipeline *pipe,
		struct pipe_action_spec *act_data_spec,
		struct rte_swx_table_entry *entry)
{
	char action_name[P4_SDE_NAME_LEN + P4_SDE_NAME_SUFFIX];
	int val;

	/* FIXME: this is at runtime, move this logic to
	 * init time */
	action_name[0] = 0;
	strncat(action_name, table_name,
		(sizeof(action_name) - strlen(action_name) -1));
	strcat(action_name, "_set_group_id");

	entry->action_id = get_action_id(pipe, action_name);
	if (entry->action_id == UINT64_MAX) {
		LOG_ERROR("dpdk action id get failed for action %s",
				action_name);
		return BF_UNEXPECTED;
	}

	val = 0;

	memcpy(&val, &(act_data_spec->sel_grp_hdl),
			sizeof(u32));

	memcpy(&entry->action_data[0],
			(uint8_t *)&val,
			sizeof(u32));
	return BF_SUCCESS;
}

int pipe_mgr_dpdk_encode_member_id(char *table_name,
		struct pipeline *pipe,
		struct pipe_action_spec *act_data_spec,
		struct rte_swx_table_entry *entry)
{
	char action_name[P4_SDE_NAME_LEN + P4_SDE_NAME_SUFFIX];
	int val;

	/* FIXME: this is at runtime, move this logic to
	 * init time */
	action_name[0] = 0;
	strncat(action_name, table_name,
		(sizeof(action_name) - strlen(action_name) -1));
	strcat(action_name, "_set_member_id");

	entry->action_id = get_action_id(pipe, action_name);
	if (entry->action_id == UINT64_MAX) {
		LOG_ERROR("dpdk action id get failed for action %s",
				action_name);
		return BF_UNEXPECTED;
	}

	val = 0;

	memcpy(&val, &(act_data_spec->adt_ent_hdl),
			sizeof(u32));

	memcpy(&entry->action_data[0],
			(uint8_t *)&val,
			sizeof(u32));
	return BF_SUCCESS;
}

int pipe_mgr_dpdk_encode_action_data(
		struct  pipe_mgr_dpdk_action_format *act_fmt,
		u8 *action_data_bits,
		struct rte_swx_table_entry *entry)
{
	struct pipe_mgr_dpdk_immediate_fields *immediate_field;
	struct rte_swx_ctl_action_arg_info *arg;
	uint32_t arg_offset = 0;;
	int status = BF_SUCCESS;
	int name_match = -1;
	uint32_t dest_start;
	uint32_t num_bytes;
	uint32_t nbytes;
	uint64_t val;
	uint32_t i;

	entry->action_id = act_fmt->action_id;

	for (i = 0; i < act_fmt->n_args.n_args; i++) {
		arg = &act_fmt->arg_info[i];
		immediate_field = act_fmt->immediate_field;
		/*  TODO:
		 *  comipler has to place the immediate_field list in the same
		 *  order as dpdk arg array so that this list traverse can be
		 *  removed */
		while (immediate_field) {
			name_match = strcmp(immediate_field->param_name,
					arg->name);
			if (!name_match)
				break;
			immediate_field = immediate_field->next;
		}

		if (name_match || !immediate_field) {
			LOG_ERROR("immediate_field not found param_name %s",
					arg->name);
			return BF_UNEXPECTED;
		}

		dest_start = immediate_field->dest_start;
		num_bytes = (immediate_field->dest_width >> 3) +
			(immediate_field->dest_width % 8 != 0);

		nbytes = (arg->n_bits >> 3) + (arg->n_bits % 8 != 0);

		if (num_bytes != nbytes) {
			LOG_ERROR("context immediate_field num_bytes "
				"mismatch with dpdk arg %s", arg->name);
			return BF_UNEXPECTED;
		}

		val = 0;
		memcpy(&val, &(action_data_bits[dest_start]),
				num_bytes);

		val = dpdk_field_hton(val, arg->n_bits);

		memcpy(&entry->action_data[arg_offset],
				(uint8_t *)&val,
				num_bytes);

		arg_offset += num_bytes;
	}

	return status;
}

/* Retrieve Action Format associated with act_fn_hdl from the given
 * stage table.
 */
int pipe_mgr_ctx_dpdk_get_act_fmt(
		struct pipe_mgr_dpdk_stage_table *stage_tbl,
		u32 act_fn_hdl,
		struct pipe_mgr_dpdk_action_format **act_fmt)
{
	struct  pipe_mgr_dpdk_action_format *cur_action;

	cur_action = stage_tbl->act_fmt;
	while (cur_action) {
		if (cur_action->action_handle == act_fn_hdl) {
			*act_fmt = cur_action;
			return BF_SUCCESS;
		}
		cur_action = cur_action->next;
	}
	return BF_OBJECT_NOT_FOUND;
}

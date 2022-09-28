/*
 * Copyright(c) 2022 Intel Corporation.
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
 * @file dal_tbl.c (DPDK)
 *
 * @Description Definitions for interfaces for tables (DPDK).
 */

#include <osdep/p4_sde_osdep.h>

#include <infra/dpdk_infra.h>

#include "../../../core/pipe_mgr_log.h"
#include "pipe_mgr_dpdk_int.h"
#include "dal_tbl.h"

static int table_match_field_info(char *table_name,
				  struct dal_dpdk_table_metadata *meta)
{
	int status = BF_SUCCESS;
	int n_match;
	int i;

	n_match = meta->dpdk_table_info.n_match_fields;

	meta->mf = P4_SDE_CALLOC(n_match,
			sizeof(struct rte_swx_ctl_table_match_field_info));
	if (!meta->mf) {
		LOG_ERROR("dpdk table %s match_field get failed", table_name);
		return BF_NO_SPACE;
	}

	meta->match_field_nbits = 0;
	for (i=0; i < n_match; i++) {
		status = rte_swx_ctl_table_match_field_info_get(meta->pipe->p,
				meta->table_id, i, &(meta->mf[i]));
		if (status) {
			P4_SDE_FREE(meta->mf);
			return BF_UNEXPECTED;
		}
		meta->match_field_nbits += meta->mf[i].n_bits;
	}
	return status;
}

static int adt_action_args_info(struct dal_dpdk_table_metadata *meta,
				struct pipe_mgr_mat_ctx *mat_ctx)
{
	struct pipe_mgr_dpdk_stage_table *stage_table = mat_ctx->stage_table;
	struct pipe_mgr_actions_list *action;
	struct rte_swx_ctl_action_arg_info *action_arg_info;
	struct pipe_mgr_dpdk_action_format *act_fmt = NULL;
	int status = BF_SUCCESS;
	uint64_t action_id;
	uint32_t index;
	int n_action;
	int i;
	void *ptr;

	n_action = meta->dpdk_table_info.n_actions;
	action = mat_ctx->actions;

	meta->action_data_size = 0;
	for (i=0; i < n_action; i++) {
		if(!action) {
			LOG_ERROR("reached end of action in context");
			status = BF_UNEXPECTED;
			goto cleanup;
		}
		act_fmt = P4_SDE_CALLOC(1, sizeof(*act_fmt));
		if (!act_fmt) {
			LOG_ERROR("dpdk act_fmt alloc failed");
			status = BF_NO_SPACE;
			goto cleanup;
		}

		action_id = get_action_id(meta->pipe, action->target_action_name);
		if (action_id == UINT64_MAX) {
			LOG_ERROR("dpdk action id get failed for action %s",
					act_fmt->action_name);
			status = BF_UNEXPECTED;
			goto cleanup;
		}

		act_fmt->action_id = action_id;
		act_fmt->action_handle = action->handle;

		status = rte_swx_ctl_action_info_get(meta->pipe->p, action_id,
				&(act_fmt->n_args));
		if (status) {
			LOG_ERROR("dpdk action info get failed for action %s",
					 act_fmt->action_name);
			status = BF_UNEXPECTED;
			goto cleanup;
		}

		action_arg_info = P4_SDE_CALLOC(act_fmt->n_args.n_args,
				sizeof(struct rte_swx_ctl_action_arg_info));

		if (!action_arg_info) {
			LOG_ERROR("dpdk action arg info alloc failed");
			status = BF_NO_SPACE;
			goto cleanup;
		}

		act_fmt->arg_info = action_arg_info;

		act_fmt->arg_nbits = 0;

		for (index = 0; index < act_fmt->n_args.n_args; index++) {
			status = rte_swx_ctl_action_arg_info_get(meta->pipe->p,
					action_id, index,
					&action_arg_info[index]);
			if (status) {
				LOG_ERROR("dpdk action arg info get failed");
				status = BF_UNEXPECTED;
				goto cleanup;
			}

			act_fmt->arg_nbits += action_arg_info[index].n_bits;
		}

		if (act_fmt->arg_nbits > meta->action_data_size)
			meta->action_data_size = act_fmt->arg_nbits;

		action = action->next;
		if (stage_table->act_fmt)
			act_fmt->next = stage_table->act_fmt;
		stage_table->act_fmt = act_fmt;
	}

	return status;

cleanup:
	if (act_fmt) {
		if (act_fmt->arg_info)
			P4_SDE_FREE(act_fmt->arg_info);
		P4_SDE_FREE(act_fmt);
	}
	act_fmt = stage_table->act_fmt;
	while(act_fmt) {
		P4_SDE_FREE(act_fmt->arg_info);
		ptr = (void *)act_fmt;
		act_fmt = act_fmt->next;
		P4_SDE_FREE(ptr);
	}
	return status;
}

static int mat_action_args_info(struct dal_dpdk_table_metadata *meta,
				struct pipe_mgr_mat_ctx *mat_ctx)
{
	struct rte_swx_ctl_action_arg_info *action_arg_info;
	struct pipe_mgr_dpdk_action_format *act_fmt;
	struct pipe_mgr_dpdk_stage_table *stage_table;
	struct pipe_mgr_actions_list *action = NULL;
	int status = BF_SUCCESS;
	uint64_t action_id;
	uint32_t index;
	int n_action;
	int i;

	stage_table = mat_ctx->match_attr.stage_table;
	n_action = meta->dpdk_table_info.n_actions;
	action = mat_ctx->actions;

	meta->action_data_size = 0;
	act_fmt = stage_table->act_fmt;
	/* Indirect table will not have action format
	 * Allocate u32 action data space for <table_name>_set_group_id
	 * action's data*/
	if (mat_ctx->adt_count || mat_ctx->sel_tbl_count) {
		meta->action_data_size = sizeof(u32) * 8;
		return status;
	}

	for (i=0; i < n_action; i++) {
		if(!act_fmt) {
			LOG_ERROR("reached end of action format in context");
			return BF_UNEXPECTED;
		}

		if(!action) {
			LOG_ERROR("reached end of action in context");
			return BF_UNEXPECTED;
		}

		action_id = get_action_id(meta->pipe, action->target_action_name);
		if (action_id == UINT64_MAX) {
			LOG_ERROR("dpdk action id get failed for action %s",
					act_fmt->action_name);
			return BF_UNEXPECTED;
		}

		act_fmt->action_id = action_id;

		status = rte_swx_ctl_action_info_get(meta->pipe->p, action_id,
				&(act_fmt->n_args));
		if (status) {
			LOG_ERROR("dpdk action info get failed for action %s",
					 act_fmt->action_name);
			return BF_UNEXPECTED;
		}

		action_arg_info = P4_SDE_CALLOC(act_fmt->n_args.n_args,
				sizeof(struct rte_swx_ctl_action_arg_info));

		if (!action_arg_info) {
			LOG_ERROR("dpdk action arg info alloc failed");
			return BF_NO_SPACE;
		}

		act_fmt->arg_info = action_arg_info;

		act_fmt->arg_nbits = 0;

		for (index = 0; index < act_fmt->n_args.n_args; index++) {
			status = rte_swx_ctl_action_arg_info_get(meta->pipe->p,
					action_id, index,
					&action_arg_info[index]);
			if (status) {
				LOG_ERROR("dpdk action arg info get failed");
				act_fmt = stage_table->act_fmt;
				while(act_fmt) {
					P4_SDE_FREE(act_fmt->arg_info);
					act_fmt = act_fmt->next;
				}
				return BF_UNEXPECTED;
			}

			act_fmt->arg_nbits += action_arg_info[index].n_bits;
		}

		if (act_fmt->arg_nbits > meta->action_data_size)
			meta->action_data_size = act_fmt->arg_nbits;

		act_fmt = act_fmt->next;
		action = action->next;
	}
	return status;
}

int dal_dpdk_table_entry_alloc(struct rte_swx_table_entry **ent,
			       struct dal_dpdk_table_metadata *meta,
			       int match_type)
{
	struct rte_swx_table_entry *entry;
	int status = BF_SUCCESS;
	uint32_t act_data_bytes;
	uint32_t mf_bytes;

	entry = P4_SDE_CALLOC(1, sizeof(struct rte_swx_table_entry));
	if (!entry) {
		LOG_ERROR("dpdk table entry calloc failed");
		return BF_NO_SPACE;
	}

	/* key, key_mask. */
	if (meta->match_field_nbits) {
		mf_bytes = (meta->match_field_nbits >> 3) +
			   (meta->match_field_nbits % 8 != 0);
		entry->key = P4_SDE_CALLOC(1, mf_bytes);
		if (!entry->key) {
			LOG_ERROR("dpdk entry key alloc failed");
			status = BF_NO_SPACE;
			goto error;
		}

		if (match_type != PIPE_MGR_MATCH_TYPE_EXACT) {
			entry->key_mask = P4_SDE_CALLOC(1, mf_bytes);
			if (!entry->key_mask) {
				LOG_ERROR("dpdk entry key_mask alloc failed");
				status = BF_NO_SPACE;
				goto error_key;
			}
		}
	}

	if (meta->action_data_size) {
		act_data_bytes = (meta->action_data_size >> 3) +
				 (meta->action_data_size % 8 != 0);
		entry->action_data = P4_SDE_CALLOC(1, act_data_bytes);
		if (!entry->action_data) {
			LOG_ERROR("dpdk entry action data alloc failed");
			status = BF_NO_SPACE;
			goto error_mask;
		}
	}

	*ent = entry;
	return status;
error_mask:
	P4_SDE_FREE(entry->key_mask);
error_key:
	P4_SDE_FREE(entry->key);
error:
	P4_SDE_FREE(entry);
	return status;
}

static int
dal_dpdk_value_lookup_metadata_get(void *tbl, char *pipeline_name, char *table_name)
{
	struct pipe_mgr_dpdk_stage_table *stage_table;
	int status = BF_SUCCESS;

	stage_table = ((struct pipe_mgr_value_lookup_ctx *)tbl)->match_attr.stage_table;

	if (stage_table->val_lookup_meta) {
		LOG_ERROR("table meta for table %s exists", table_name);
		return BF_UNEXPECTED;
	}

	stage_table->val_lookup_meta = P4_SDE_CALLOC(1, sizeof(*stage_table->val_lookup_meta));
	if (!stage_table->val_lookup_meta) {
		LOG_ERROR("table meta alloc failed for table %s", table_name);
		return BF_NO_SPACE;
	}

	/* get dpdk pipeline info */
	stage_table->val_lookup_meta->pipe = pipeline_find(pipeline_name);
	if (!stage_table->val_lookup_meta->pipe) {
		LOG_ERROR("dpdk pipeline %s get failed", pipeline_name);
		status = BF_OBJECT_NOT_FOUND;
		goto cleanup;
	}
	return status;
cleanup:
	P4_SDE_FREE(stage_table->val_lookup_meta);
	return status;
}

int dal_dpdk_table_metadata_get(void *tbl, enum pipe_mgr_table_type tbl_type,
				char *pipeline_name, char *table_name)
{
	struct pipe_mgr_match_key_fields *match_fields;
	struct pipe_mgr_dpdk_stage_table *stage_table;
	struct dal_dpdk_table_metadata *meta;
	uint32_t key_fields_count;
	int status = BF_SUCCESS;

	switch (tbl_type) {
	case PIPE_MGR_TABLE_TYPE_ADT:
		stage_table = ((struct pipe_mgr_mat_ctx *)tbl)->stage_table;
		match_fields = ((struct pipe_mgr_mat_ctx *)tbl)->mat_key_fields;
		key_fields_count = ((struct pipe_mgr_mat_ctx *)tbl)->mat_key_fields_count;
		break;
	case PIPE_MGR_TABLE_TYPE_MAT:
		stage_table = ((struct pipe_mgr_mat_ctx *)tbl)->match_attr.stage_table;
		match_fields = ((struct pipe_mgr_mat_ctx *)tbl)->mat_key_fields;
		key_fields_count = ((struct pipe_mgr_mat_ctx *)tbl)->mat_key_fields_count;
		break;
	case PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP:
		return dal_dpdk_value_lookup_metadata_get(tbl, pipeline_name, table_name);
	default:
		LOG_ERROR("Table type %d is not supported", tbl_type);
		return BF_INVALID_ARG;
	}

	meta = stage_table->table_meta;
	if (meta) {
		LOG_ERROR("table meta for table %s exists", table_name);
		return BF_UNEXPECTED;
	}

	meta = P4_SDE_CALLOC(1, sizeof(*meta));
	if (!meta) {
		LOG_ERROR("table meta alloc failed for table %s", table_name);
		return BF_NO_SPACE;
	}
	/* get dpdk pipeline, table and action info */
	meta->pipe = pipeline_find(pipeline_name);
	if (!meta->pipe) {
		LOG_ERROR("dpdk pipeline %s get failed", pipeline_name);
		status = BF_OBJECT_NOT_FOUND;
		goto error;
	}

	/* get dpdk table id */
	meta->table_id = get_table_id(meta->pipe, table_name);
	if (meta->table_id == UINT32_MAX) {
		LOG_ERROR("dpdk table id get for table %s failed", table_name);
		status = BF_OBJECT_NOT_FOUND;
		goto error;
	}

	/* get the rte_swx_ctl_table_info for the table. */
	status = rte_swx_ctl_table_info_get(meta->pipe->p, meta->table_id,
					    &(meta->dpdk_table_info));
	if (status) {
		LOG_ERROR("dpdk table info get failed with error %d for table %s",
			  status, table_name);
		status = BF_OBJECT_NOT_FOUND;
		goto error;
	}

	status = table_match_field_info(table_name, meta);
	if (status) {
		LOG_ERROR("dpdk table match field info failed");
		goto error;
	}

	if (tbl_type != PIPE_MGR_TABLE_TYPE_ADT) {
		if (meta->dpdk_table_info.n_match_fields != key_fields_count) {
			LOG_ERROR(" dpdk num of match field mismatch");
			status = BF_UNEXPECTED;
			goto error;
		}

		/* if any one if the match field is of type ternary or LPM
		   then table is not exact match kind */
		meta->match_type = match_fields->match_type;
		while (match_fields) {
			if (match_fields->match_type == PIPE_MGR_MATCH_TYPE_LPM
					|| match_fields->match_type
					== PIPE_MGR_MATCH_TYPE_TERNARY) {
				meta->match_type = match_fields->match_type;
				break;
			}
			match_fields = match_fields->next;
		}
	}

	if (tbl_type == PIPE_MGR_TABLE_TYPE_ADT)
		status = adt_action_args_info(meta, (struct pipe_mgr_mat_ctx *)tbl);
	else if (tbl_type == PIPE_MGR_TABLE_TYPE_MAT)
		status = mat_action_args_info(meta, (struct pipe_mgr_mat_ctx *)tbl);
	if (status) {
		LOG_ERROR("dpdk table match field info failed");
		goto error_mf;
	}

	stage_table->table_meta = meta;
	return status;

error_mf:
	P4_SDE_FREE(meta->mf);
error:
	P4_SDE_FREE(meta);
	return status;
}

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
#include <arpa/inet.h>
#include <rte_swx_pipeline.h>
#include <rte_swx_ctl.h>

#include "../dal_mat.h"
#include "../../../core/pipe_mgr_log.h"
#include <lld_dpdk_lib.h>
#include <infra/dpdk_infra.h>
#include "../../infra/pipe_mgr_dbg.h"
#include "pipe_mgr_dpdk_int.h"
#include "pipe_mgr_dpdk_ctx_util.h"
#include "../../infra/pipe_mgr_int.h"

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
	struct pipe_mgr_dpdk_action_format *act_fmt;
	int status = BF_SUCCESS;
	uint64_t action_id;
	uint32_t index;
	int n_action;
	int i;

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

		action_id = get_action_id(meta->pipe, action->name);
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
				act_fmt = stage_table->act_fmt;
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
	act_fmt = stage_table->act_fmt;
	while(act_fmt) {
		P4_SDE_FREE(act_fmt->arg_info);
		act_fmt = act_fmt->next;
	}
	return status;
}

static int mat_action_args_info(struct dal_dpdk_table_metadata *meta,
		struct pipe_mgr_mat_ctx *mat_ctx)
{
	struct rte_swx_ctl_action_arg_info *action_arg_info;
	struct pipe_mgr_dpdk_action_format *act_fmt;
	struct pipe_mgr_dpdk_stage_table *stage_table;
	int status = BF_SUCCESS;
	uint64_t action_id;
	uint32_t index;
	int n_action;
	int i;

	stage_table = mat_ctx->match_attr.stage_table;
	n_action = meta->dpdk_table_info.n_actions;

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
			LOG_ERROR("reached end of action in context");
			return BF_UNEXPECTED;
		}

		action_id = get_action_id(meta->pipe, act_fmt->action_name);
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
	}
	return status;
}

int table_entry_alloc(struct rte_swx_table_entry **ent,
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

int dal_dpdk_table_metadata_get(struct pipe_mgr_mat_ctx *mat_ctx,
		char *pipeline_name, char *table_name, bool adt_table)
{
	struct pipe_mgr_match_key_fields *match_fields;
	struct pipe_mgr_dpdk_stage_table *stage_table;
	struct dal_dpdk_table_metadata *meta;
	int status = BF_SUCCESS;

	if (adt_table)
		stage_table = mat_ctx->stage_table;
	else
		stage_table = mat_ctx->match_attr.stage_table;

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
		LOG_ERROR("dpdk pipeline %s get failed",
				pipeline_name);
		status = BF_OBJECT_NOT_FOUND;
		goto error;
	}

	/* get dpdk table id */
	meta->table_id = get_table_id(meta->pipe, table_name);
	if (meta->table_id == UINT32_MAX) {
		LOG_ERROR("dpdk table id get for table %s failed",
				 table_name);
		status = BF_OBJECT_NOT_FOUND;
		goto error;
	}

	/* get the rte_swx_ctl_table_info for the table. */
	status = rte_swx_ctl_table_info_get(meta->pipe->p, meta->table_id,
			&(meta->dpdk_table_info));
	if (status) {
		LOG_ERROR("dpdk table info get failed with error %d"
				"for table %s", status, table_name);
		status = BF_OBJECT_NOT_FOUND;
		goto error;
	}

	match_fields = mat_ctx->mat_key_fields;
	status = table_match_field_info(table_name, meta);
	if (status) {
		LOG_ERROR("dpdk table match field info failed");
		goto error;
	}

	if (!adt_table) {
		if (meta->dpdk_table_info.n_match_fields
				!= (u32)mat_ctx->mat_key_fields_count) {
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

	if (adt_table)
		status = adt_action_args_info(meta, mat_ctx);
	else
		status = mat_action_args_info(meta, mat_ctx);
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
int dal_table_ent_add(u32 sess_hdl,
		      struct bf_dev_target_t dev_tgt,
		      u32 mat_tbl_hdl,
		      struct pipe_tbl_match_spec *match_spec,
		      u32 act_fn_hdl,
		      struct pipe_action_spec *act_data_spec,
		      u32 ttl,
		      u32 pipe_api_flags,
		      struct pipe_mgr_mat_ctx *mat_ctx,
		      u32 tbl_ent_hdl,
		      void **dal_data)
{
	struct pipe_action_data_spec *data_spec =
					&act_data_spec->act_data;
	u8 *action_data_bits = data_spec->action_data_bits;
	struct pipe_mgr_dpdk_stage_table *stage_table = NULL;
	struct pipe_mgr_dpdk_action_format *act_fmt = NULL;
	struct pipe_mgr_profile *profile = NULL;
	struct rte_swx_table_entry *entry = NULL;
	struct rte_swx_ctl_pipeline *ctl = NULL;
	struct pipeline *pipe = NULL;
	int status = BF_SUCCESS;

	LOG_TRACE("Entering %s", __func__);
#ifdef PIPE_MGR_DEBUG
	pipe_mgr_print_match_spec(match_spec);
	pipe_mgr_print_action_spec(act_data_spec);
#endif

	if (mat_ctx->match_attr.stage_table_count > 1) {
		LOG_ERROR("1 P4 to many stage tables is not supported yet");
		LOG_TRACE("Exiting %s", __func__);
		return BF_NOT_SUPPORTED;
	}

	stage_table = mat_ctx->match_attr.stage_table;
	if (act_data_spec->pipe_action_datatype_bmap ==
			PIPE_ACTION_DATA_TYPE) {
		status = pipe_mgr_ctx_dpdk_get_act_fmt(stage_table, act_fn_hdl,
				&act_fmt);
		if (status) {
			LOG_ERROR("not able find action format with id %d",
					act_fn_hdl);
			return BF_OBJECT_NOT_FOUND;
		}
	}

	status = pipe_mgr_get_profile(dev_tgt.device_id, &profile);
	if (status) {
		LOG_ERROR("not able find profile with device_id  %d",
				dev_tgt.device_id);
		return BF_OBJECT_NOT_FOUND;
	}

	if (!stage_table->table_meta) {
		status = dal_dpdk_table_metadata_get(mat_ctx,
				profile->pipeline_name,
				mat_ctx->name, 0);
		if (status) {
			LOG_ERROR("not able get table metadata for table %s",
					mat_ctx->name);
			return BF_OBJECT_NOT_FOUND;
		}
	}
	pipe = stage_table->table_meta->pipe;
	ctl = stage_table->table_meta->pipe->ctl;
	if (!ctl) {
		LOG_ERROR("dpdk pipeline %s ctl is null",
				profile->pipeline_name);
		return BF_OBJECT_NOT_FOUND;
	}

	status = table_entry_alloc(&entry, stage_table->table_meta,
			(int)stage_table->table_meta->match_type);
	if (status) {
		LOG_ERROR("dpdk table entry alloc failed");
		return BF_NO_SPACE;
	}


	/* encode the key, key mask and action expected as
	 * rte calls
	 */
	status = pipe_mgr_dpdk_encode_match_key_and_mask(
			mat_ctx,
			match_spec,
			entry);

	if (status) {
		LOG_ERROR("dpdk table entry key/key_mask encoding failed");
		status = BF_UNEXPECTED;
		goto error;
	}

	if (act_data_spec->pipe_action_datatype_bmap ==
			PIPE_SEL_GRP_HDL_TYPE) {
		/* encode the action set_group_id and group id */
		status = pipe_mgr_dpdk_encode_sel_action(mat_ctx->name, pipe,
				act_data_spec, entry);
		if (status) {
			LOG_ERROR("dpdk table entry action set_group_id"
					" encoding failed");
			goto error;
		}
	} else if (act_data_spec->pipe_action_datatype_bmap ==
			PIPE_ACTION_DATA_HDL_TYPE) {
		/* encode the action set_member_id*/
		status = pipe_mgr_dpdk_encode_member_id(mat_ctx->name, pipe,
				act_data_spec, entry);
		if (status) {
			LOG_ERROR("dpdk table entry action set_member_id"
					" encoding failed");
			goto error;
		}
	} else if (act_data_spec->pipe_action_datatype_bmap ==
		   PIPE_ACTION_DATA_TYPE) {
		/* encode the action parameters name and values */
		status = pipe_mgr_dpdk_encode_action_data(act_fmt,
				action_data_bits,
				entry);

		if (status) {
			LOG_ERROR("dpdk table entry action "
					"data encoding failed");
			goto error;
		}
	}

	status = rte_swx_ctl_pipeline_table_entry_add(pipe->ctl, mat_ctx->name,
						      entry);
	if (status) {
		LOG_ERROR("rte_swx_ctl_pipeline_table_entry_add");
		status = BF_UNEXPECTED;
		goto error;
	}

	status = rte_swx_ctl_pipeline_commit(ctl, 1);
	if (status) {
		LOG_ERROR("rte_swx_ctl_pipeline_commit failed");
		status = BF_UNEXPECTED;
		goto error;
	}

	return status;
error:
	table_entry_free(entry);
	return status;
}

int dal_table_ent_del_by_match_spec(u32 sess_hdl,
				    struct bf_dev_target_t dev_tgt,
				    u32 mat_tbl_hdl,
				    struct pipe_tbl_match_spec *match_spec,
				    u32 pipe_api_flags,
				    struct pipe_mgr_mat_ctx *mat_ctx,
				    u32 tbl_ent_hdl, void *dal_data)
{
	struct pipe_mgr_dpdk_stage_table *stage_table;
	struct rte_swx_table_entry *entry;
	struct rte_swx_ctl_pipeline *ctl;
	int status = BF_SUCCESS;

	LOG_TRACE("Entering %s", __func__);
#ifdef PIPE_MGR_DEBUG
	pipe_mgr_print_match_spec(match_spec);
#endif
	if (mat_ctx->match_attr.stage_table_count > 1) {
		LOG_ERROR("1 P4 to many stage tables is not supported yet");
		LOG_TRACE("Exiting %s", __func__);
		return BF_NOT_SUPPORTED;
	}

	stage_table = mat_ctx->match_attr.stage_table;
	if (!stage_table->table_meta) {
		LOG_ERROR("not able get table metadata for table %s",
				mat_ctx->name);
		return BF_OBJECT_NOT_FOUND;
	}

	ctl = stage_table->table_meta->pipe->ctl;
	if (!ctl) {
		LOG_ERROR("dpdk pipeline ctl is null");
		return BF_OBJECT_NOT_FOUND;
	}

	status = table_entry_alloc(&entry, stage_table->table_meta,
			(int)stage_table->table_meta->match_type);
	if (status) {
		LOG_ERROR("dpdk table entry alloc failed");
		return BF_NO_SPACE;
	}

	/* encode the key, key mask and action expected as
	 * rte calls
	 */
	status = pipe_mgr_dpdk_encode_match_key_and_mask(
			mat_ctx,
			match_spec,
			entry);

	if (status) {
		LOG_ERROR("dpdk table entry key/key_mask encoding failed");
		status = BF_UNEXPECTED;
		goto exit;
	}

	status = rte_swx_ctl_pipeline_table_entry_delete(ctl, mat_ctx->name,
							 entry);
	if (status) {
		LOG_ERROR("rte_swx_ctl_pipeline_table_entry_add");
		status = BF_UNEXPECTED;
		goto exit;
	}
	status = rte_swx_ctl_pipeline_commit(ctl, 1);
	if (status) {
		LOG_ERROR("rte_swx_ctl_pipeline_commit failed");
		status = BF_UNEXPECTED;
		goto exit;
	}

exit:
	table_entry_free(entry);
	return status;
}

/* currently no DPDK dal data struct for table entries,
   if we have in future then this function should be written */
void dal_delete_table_entry_data(void *dal_data)
{
	return;
}

void dal_delete_adt_table_entry_data(void *dal_adt_data)
{
	return;
}

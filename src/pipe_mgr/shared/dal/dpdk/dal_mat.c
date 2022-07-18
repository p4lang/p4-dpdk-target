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
#include "../../infra/pipe_mgr_dbg.h"
#include "pipe_mgr_dpdk_int.h"
#include "pipe_mgr_dpdk_ctx_util.h"
#include "../../infra/pipe_mgr_int.h"
#include "dal_tbl.h"

static int is_match_value_present(struct pipe_tbl_match_spec *match_spec)
{
	int i;

	for (i = 0; i < match_spec->num_match_bytes; i++)
		if (match_spec->match_value_bits[i] ||
		    match_spec->match_mask_bits[i])
			return 1;
	return 0;
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

	status = pipe_mgr_get_profile(dev_tgt.device_id,
				      dev_tgt.dev_pipe_id, &profile);
	if (status) {
		LOG_ERROR("not able find profile with device_id  %d",
				dev_tgt.device_id);
		return BF_OBJECT_NOT_FOUND;
	}

	if (!stage_table->table_meta) {
		status = dal_dpdk_table_metadata_get((void *)mat_ctx,
						     PIPE_MGR_TABLE_TYPE_MAT,
						     profile->pipeline_name,
						     mat_ctx->target_table_name);
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

	status = dal_dpdk_table_entry_alloc(&entry, stage_table->table_meta,
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

	if (is_match_value_present(match_spec))
		status = rte_swx_ctl_pipeline_table_entry_add
				(pipe->ctl, mat_ctx->name, entry);
	else
		status = rte_swx_ctl_pipeline_table_default_entry_add
				(pipe->ctl, mat_ctx->name, entry);

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
				    void *dal_data)
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

	status = dal_dpdk_table_entry_alloc(&entry, stage_table->table_meta,
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
int dal_unpack_dal_data(void *dal_data)
{
	return BF_SUCCESS;
}

void dal_delete_table_entry_data(void *dal_data)
{
	return;
}

void dal_delete_adt_table_entry_data(void *dal_adt_data)
{
	return;
}

bool dal_mat_store_entries(struct pipe_mgr_mat_ctx *mat_ctx)
{
	if (mat_ctx->match_attr.match_type == PIPE_MGR_MATCH_TYPE_EXACT &&
	    mat_ctx->add_on_miss) {
		return false;
	}

	return true;
}


int dal_mat_get_first_entry(u32 sess_hdl,
			    struct bf_dev_target_t dev_tgt,
			    u32 mat_tbl_hdl,
			    struct pipe_tbl_match_spec *match_spec,
			    struct pipe_action_spec *act_data_spec,
			    u32 *act_fn_hdl,
		            struct pipe_mgr_mat_ctx *mat_ctx)
{
	return BF_NOT_SUPPORTED;
}

int dal_mat_get_next_n_by_key(u32 sess_hdl,
			       struct bf_dev_target_t dev_tgt,
			       u32 mat_tbl_hdl,
			       struct pipe_tbl_match_spec *cur_match_spec,
			       int n,
			       struct pipe_tbl_match_spec *match_specs,
			       struct pipe_action_spec **act_specs,
			       u32 *act_fn_hdls,
			       u32 *num,
		               struct pipe_mgr_mat_ctx *mat_ctx)
{
	return BF_NOT_SUPPORTED;
}

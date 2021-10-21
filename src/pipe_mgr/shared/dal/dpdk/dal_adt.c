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

int dal_table_adt_ent_add(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 adt_tbl_hdl,
		u32 act_fn_hdl,
		struct pipe_action_spec *action_spec,
		u32 pipe_api_flags,
		struct pipe_mgr_mat_ctx *mat_ctx,
		u32 tbl_ent_hdl,
		void **dal_data)
{
	struct pipe_action_data_spec *data_spec =
					&action_spec->act_data;
	u8 *action_data_bits = data_spec->action_data_bits;
	struct pipe_mgr_dpdk_stage_table *stage_table;
	struct pipe_mgr_dpdk_action_format *act_fmt;
	struct rte_swx_table_entry *entry;
	struct pipe_mgr_profile *profile;
	struct rte_swx_ctl_pipeline *ctl;
	/*uint64_t val;*/
	int status = BF_SUCCESS;

	LOG_TRACE("Entering %s", __func__);
#ifdef PIPE_MGR_DEBUG
	printf("MEMBER ID handle = %02x\n", tbl_ent_hdl);
	pipe_mgr_print_action_spec(action_spec);
#endif
	if (mat_ctx->stage_table_count > 1) {
		LOG_ERROR("1 P4 to many stage tables is not supported yet");
		LOG_TRACE("Exiting %s", __func__);
		return BF_NOT_SUPPORTED;
	}

	status = pipe_mgr_get_profile(dev_tgt.device_id, &profile);
	if (status) {
		LOG_ERROR("not able find profile with device_id  %d",
				dev_tgt.device_id);
		return BF_OBJECT_NOT_FOUND;
	}

	if (!mat_ctx->stage_table) {

		stage_table = P4_SDE_CALLOC(1, sizeof(*stage_table));
		if (!stage_table) {
			LOG_ERROR("not able to alloc adt stage table");
			return BF_NO_SPACE;
		}
		mat_ctx->stage_table = stage_table;
		status = dal_dpdk_table_metadata_get(mat_ctx,
				profile->pipeline_name,
				mat_ctx->name, 1);
		if (status) {
			LOG_ERROR("not able get table metadata for table %s",
					mat_ctx->name);
			P4_SDE_FREE(stage_table);
			mat_ctx->stage_table = NULL;
			return BF_OBJECT_NOT_FOUND;
		}
	}

	stage_table = mat_ctx->stage_table;
	ctl = stage_table->table_meta->pipe->ctl;
	if (!ctl) {
		LOG_ERROR("dpdk pipeline %s ctl is null",
				profile->pipeline_name);
		return BF_OBJECT_NOT_FOUND;
	}

	status = pipe_mgr_ctx_dpdk_get_act_fmt(stage_table, act_fn_hdl,
						&act_fmt);
	if (status) {
		LOG_ERROR("not able find action format  with id %d",
				act_fn_hdl);
		return BF_OBJECT_NOT_FOUND;
	}

	status = table_entry_alloc(&entry, stage_table->table_meta,
			(int) PIPE_MGR_MATCH_TYPE_EXACT);
	if (status) {
		LOG_ERROR("dpdk table entry alloc failed");
		return BF_NO_SPACE;
	}

	/*encode the match key*/
	memcpy(&entry->key[0], (uint8_t *)&tbl_ent_hdl, sizeof(tbl_ent_hdl));

	/* encode the action parameters name and values */
	status = pipe_mgr_dpdk_encode_adt_action_data(act_fmt,
						  action_data_bits,
						  entry);

	if (status) {
		LOG_ERROR("dpdk table entry action data encoding failed");
		goto error;
	}

	status = rte_swx_ctl_pipeline_table_entry_add(ctl, mat_ctx->name,
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


int dal_table_adt_ent_del
	(u32 sess_hdl,
	 struct bf_dev_target_t dev_tgt,
	 u32 adt_tbl_hdl,
	 u32 pipe_api_flags,
	 struct pipe_mgr_mat_ctx *mat_ctx,
	 u32 tbl_ent_hdl,
	 void **dal_data)
{
	struct pipe_mgr_dpdk_stage_table *stage_table;
	struct rte_swx_table_entry *entry;
	struct pipe_mgr_profile *profile;
	struct rte_swx_ctl_pipeline *ctl;
	int status = BF_SUCCESS;

	LOG_TRACE("Entering %s", __func__);

	if (mat_ctx->stage_table_count > 1) {
		LOG_ERROR("1 P4 to many stage tables is not supported yet");
		LOG_TRACE("Exiting %s", __func__);
		return BF_NOT_SUPPORTED;
	}

	status = pipe_mgr_get_profile(dev_tgt.device_id, &profile);
	if (status) {
		LOG_ERROR("not able find profile with device_id  %d",
				dev_tgt.device_id);
		return BF_OBJECT_NOT_FOUND;
	}

	stage_table = mat_ctx->stage_table;
	ctl = stage_table->table_meta->pipe->ctl;
	if (!ctl) {
		LOG_ERROR("dpdk pipeline %s ctl is null",
				profile->pipeline_name);
		return BF_OBJECT_NOT_FOUND;
	}

	status = table_entry_alloc(&entry, stage_table->table_meta,
			(int) PIPE_MGR_MATCH_TYPE_EXACT);
	if (status) {
		LOG_ERROR("dpdk table entry alloc failed");
		return BF_NO_SPACE;
	}

	/*encode the match key*/
	memcpy(&entry->key[0], (uint8_t *)&tbl_ent_hdl, sizeof(tbl_ent_hdl));

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

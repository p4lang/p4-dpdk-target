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

#include "../dal_mat.h"
#include "../../../core/pipe_mgr_log.h"
#include <lld_dpdk_lib.h>
#include <infra/dpdk_infra.h>
#include "../../infra/pipe_mgr_dbg.h"
#include "pipe_mgr_dpdk_int.h"
#include "pipe_mgr_dpdk_ctx_util.h"

int dal_table_sel_ent_add_del(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 sel_tbl_hdl,
		u32 pipe_api_flags,
		struct pipe_mgr_mat_ctx *mat_ctx,
		u32 *tbl_ent_hdl,
		bool del_grp)
{
	struct pipe_mgr_profile *profile;
	struct rte_swx_ctl_pipeline *ctl;
	int status = BF_SUCCESS;
	struct pipeline *pipe;

	status = pipe_mgr_get_profile(dev_tgt.device_id, &profile);
	if (status) {
		LOG_ERROR("not able find profile with device_id  %d",
				dev_tgt.device_id);
		return BF_OBJECT_NOT_FOUND;
	}

	/* get dpdk pipeline, table and action info */
	pipe = pipeline_find(profile->pipeline_name);
	if (!pipe) {
		LOG_ERROR("dpdk pipeline %s get failed",
				profile->pipeline_name);
		return BF_OBJECT_NOT_FOUND;
	}

	if (!pipe->ctl) {
		LOG_ERROR("dpdk pipeline %s ctl is null",
				profile->pipeline_name);
		return BF_OBJECT_NOT_FOUND;
	}

	ctl = pipe->ctl;

	if (!del_grp)
		status =
		rte_swx_ctl_pipeline_selector_group_add(ctl, mat_ctx->name,
			tbl_ent_hdl);
	else
		status =
		rte_swx_ctl_pipeline_selector_group_delete(ctl, mat_ctx->name,
			*tbl_ent_hdl);

	if (status) {
		if (!del_grp)
			LOG_ERROR(
			"rte_swx_ctl_pipeline_selector_group_add failed");
		else
			LOG_ERROR(
			"rte_swx_ctl_pipeline_selector_group_delete failed");

		return BF_UNEXPECTED;
	}

	status = rte_swx_ctl_pipeline_commit(ctl, 1);
	if (status) {
		LOG_ERROR("rte_swx_ctl_pipeline_commit failed");
		return BF_UNEXPECTED;
	}

	return status;
}

int dal_table_sel_member_add_del(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 sel_tbl_hdl,
		u32 sel_grp_hdl,
		uint32_t num_mbrs,
		u32 *mbrs,
		u32 pipe_api_flags,
		struct pipe_mgr_mat_ctx *mat_ctx,
		bool delete_members)
{
	struct pipe_mgr_profile *profile;
	struct rte_swx_ctl_pipeline *ctl;
	int status = BF_SUCCESS;
	struct pipeline *pipe;
	uint32_t i;

	status = pipe_mgr_get_profile(dev_tgt.device_id, &profile);
	if (status) {
		LOG_ERROR("not able find profile with device_id  %d",
				dev_tgt.device_id);
		return BF_OBJECT_NOT_FOUND;
	}

	/* get dpdk pipeline, table and action info */
	pipe = pipeline_find(profile->pipeline_name);
	if (!pipe) {
		LOG_ERROR("dpdk pipeline %s get failed",
				profile->pipeline_name);
		return BF_OBJECT_NOT_FOUND;
	}

	if (!pipe->ctl) {
		LOG_ERROR("dpdk pipeline %s ctl is null",
				profile->pipeline_name);
		return BF_OBJECT_NOT_FOUND;
	}

	ctl = pipe->ctl;
	for (i = 0; i < num_mbrs; i++) {
		if (!delete_members)
			status =
			rte_swx_ctl_pipeline_selector_group_member_add(ctl,
					mat_ctx->name, sel_grp_hdl, mbrs[i],
					(uint32_t)1);
		else
			status =
			rte_swx_ctl_pipeline_selector_group_member_delete(ctl,
					mat_ctx->name, sel_grp_hdl, mbrs[i]);

		if (status) {
			if (!delete_members)
				LOG_ERROR(
				"rte_swx_ctl_pipeline_selector_group_add"
				" failed %d", status);
			else
				LOG_ERROR(
				"rte_swx_ctl_pipeline_selector_"
				"group_member_delete"
				" failed %d", status);
			return BF_UNEXPECTED;
		}
	}
	status = rte_swx_ctl_pipeline_commit(ctl, 1);
	if (status) {
		LOG_ERROR("rte_swx_ctl_pipeline_commit failed");
		return BF_UNEXPECTED;
	}

	return status;
}

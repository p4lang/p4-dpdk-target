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
 * @file dal_value_lookup.c (DPDK)
 *
 * @Description Definitions for interfaces to value lookup table.
 */

#include <pipe_mgr/shared/pipe_mgr_value_lookup.h>

#include "../../../core/pipe_mgr_log.h"
#include "../../infra/pipe_mgr_int.h"
#include "../../infra/pipe_mgr_dbg.h"
#include "../dal_mirror.h"
#include "../dal_mat.h"
#include "pipe_mgr_dpdk_ctx_util.h"
#include "pipe_mgr_dpdk_int.h"
#include "dal_tbl.h"

static int
dal_mirror_get_mirror_session_id(struct pipe_tbl_match_spec *match_spec,
				 uint8_t *mirror_id)
{
	int status = BF_SUCCESS;
	uint8_t *val_ptr = NULL;
	uint16_t num_bytes;
	uint8_t val = 0;

	status = pipe_mgr_dpdk_encode_key_from_match_spec(match_spec, &val_ptr, &num_bytes);
	if (status)
		return status;
	memcpy(&val, val_ptr, num_bytes);
	P4_SDE_FREE(val_ptr);
	*mirror_id = val;
	return status;
}

static int
dal_mirror_fill_mirror_session_params(struct pipe_mgr_dpdk_immediate_fields *immediate_fields,
				      int immediate_fields_count,
				      struct pipe_data_spec *data_spec,
				      struct pipe_mgr_mir_prof *mir_params,
				      int fast_clone)
{
	struct pipe_mgr_dpdk_immediate_fields *im;
	int status = BF_SUCCESS;
	uint16_t num_bytes;
	uint64_t val64 = 0;
	uint32_t val32 = 0;
	uint16_t val16 = 0;
	uint8_t val8 = 0;
	uint8_t *val_ptr;

        im = immediate_fields;

	mir_params->fast_clone = fast_clone;
	PIPE_MGR_DPDK_FILL_DATA_FROM_IMMEDIATE_FIELD_AND_DATA_SPEC(im, data_spec, mir_params->port_id);
	PIPE_MGR_DPDK_FILL_DATA_FROM_IMMEDIATE_FIELD_AND_DATA_SPEC(im, data_spec, mir_params->truncate_length);

cleanup:
	return status;
}

int
dal_value_lookup_ent_add(uint32_t sess_hdl,
			 struct bf_dev_target_t dev_tgt,
			 uint32_t tbl_hdl,
			 struct pipe_tbl_match_spec *match_spec,
			 struct pipe_data_spec *data_spec,
			 struct pipe_mgr_value_lookup_ctx *tbl_ctx)
{
	struct pipe_mgr_dpdk_stage_table *stage_table = NULL;
	struct pipe_mgr_mir_prof mir_params = {0};
	struct pipe_mgr_profile *profile = NULL;
	struct rte_swx_ctl_pipeline *ctl = NULL;
	struct pipeline *pipe = NULL;
	int status = BF_SUCCESS;
	uint8_t mirror_id = 0;

	LOG_TRACE("Entering %s", __func__);

#ifdef PIPE_MGR_DEBUG
	pipe_mgr_print_match_spec(match_spec);
	pipe_mgr_print_data_spec(data_spec);
#endif
	if (tbl_ctx->match_attr.stage_table_count > 1) {
		LOG_ERROR("1 P4 to many stage tables is not supported yet");
		LOG_TRACE("Exiting %s", __func__);
		return BF_NOT_SUPPORTED;
	}
	stage_table = tbl_ctx->match_attr.stage_table;
	if (!stage_table) {
		LOG_ERROR("Could not find stage tables information");
		LOG_TRACE("Exiting %s", __func__);
		return BF_OBJECT_NOT_FOUND;
	}

	status = pipe_mgr_get_profile(dev_tgt.device_id, dev_tgt.dev_pipe_id, &profile);
	if (status) {
		LOG_ERROR("not able find profile with device_id  %d", dev_tgt.device_id);
		LOG_TRACE("Exiting %s", __func__);
		return BF_OBJECT_NOT_FOUND;
	}

	if (!stage_table->val_lookup_meta) {
		status = dal_dpdk_table_metadata_get((void *)tbl_ctx,
						     PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
						     profile->pipeline_name,
						     tbl_ctx->target_table_name);
		if (status) {
			LOG_ERROR("not able get table metadata for table %s", tbl_ctx->name);
			LOG_TRACE("Exiting %s", __func__);
			return BF_OBJECT_NOT_FOUND;
		}
	}

	pipe = stage_table->val_lookup_meta->pipe;
	ctl = stage_table->val_lookup_meta->pipe->ctl;
	if (!ctl) {
		LOG_ERROR("dpdk pipeline %s ctl is null", profile->pipeline_name);
		LOG_TRACE("Exiting %s", __func__);
		return BF_OBJECT_NOT_FOUND;
	}

	switch (stage_table->resource_id) {
	case PIPE_MGR_DPDK_RES_MIR_SESSION:
		status = dal_mirror_get_mirror_session_id(match_spec, &mirror_id);
		if (status) {
			LOG_ERROR("Could not fill the mirror session id.");
			goto cleanup;
		}
		status = dal_mirror_fill_mirror_session_params(stage_table->immediate_field,
							       stage_table->immediate_fields_count,
							       data_spec, &mir_params,
							       profile->fast_clone);
		if (status) {
			LOG_ERROR("Could not fill the mirror session params.");
			goto cleanup;
		}
		status = dal_mirror_session_set(mirror_id, &mir_params, pipe->p);
		if (status) {
			LOG_ERROR("Could not configure the mirror session.");
			goto cleanup;
		}
		break;
	default:
		LOG_ERROR("Invalid resource id %d for configuration.", stage_table->resource_id);
		status = BF_NOT_SUPPORTED;
		goto cleanup;
	}

	status = rte_swx_ctl_pipeline_commit(ctl, 1);
	if (status) {
		LOG_ERROR("rte_swx_ctl_pipeline_commit failed");
		status = BF_UNEXPECTED;
	}

cleanup:
	LOG_TRACE("Exiting %s", __func__);
	return status;
}

int
dal_value_lookup_ent_del(uint32_t sess_hdl,
			 struct bf_dev_target_t dev_tgt,
			 uint32_t tbl_hdl,
			 struct pipe_tbl_match_spec *match_spec,
			 struct pipe_mgr_value_lookup_ctx *tbl_ctx)
{
	struct pipe_mgr_dpdk_stage_table *stage_table = NULL;
	struct rte_swx_ctl_pipeline *ctl;
	struct pipeline *pipe = NULL;
	int status = BF_SUCCESS;
	uint8_t mirror_id = 0;

	LOG_TRACE("Entering %s", __func__);

#ifdef PIPE_MGR_DEBUG
	pipe_mgr_print_match_spec(match_spec);
#endif
	if (tbl_ctx->match_attr.stage_table_count > 1) {
		LOG_ERROR("1 P4 to many stage tables is not supported yet");
		LOG_TRACE("Exiting %s", __func__);
		return BF_NOT_SUPPORTED;
	}

	stage_table = tbl_ctx->match_attr.stage_table;
	if (!stage_table->val_lookup_meta) {
		LOG_ERROR("not able get table metadata for table %s", tbl_ctx->name);
		LOG_TRACE("Exiting %s", __func__);
		return BF_OBJECT_NOT_FOUND;
	}

	pipe = stage_table->val_lookup_meta->pipe;
	if (!pipe) {
		LOG_ERROR("dpdk pipeline pipe is null");
		LOG_TRACE("Exiting %s", __func__);
		return BF_OBJECT_NOT_FOUND;
	}

	ctl = stage_table->val_lookup_meta->pipe->ctl;
	if (!ctl) {
		LOG_ERROR("dpdk pipeline ctl is null");
		LOG_TRACE("Exiting %s", __func__);
		return BF_OBJECT_NOT_FOUND;
	}

	switch (stage_table->resource_id) {
	case PIPE_MGR_DPDK_RES_MIR_SESSION:
		status = dal_mirror_get_mirror_session_id(match_spec, &mirror_id);
		if (status) {
			LOG_ERROR("Could not fill the mirror session id.");
			goto cleanup;
		}
		status = dal_mirror_session_clear(mirror_id, pipe->p);
		if (status) {
			LOG_ERROR("Could not clear the mirror session.");
			goto cleanup;
		}
		break;
	default:
		LOG_ERROR("Invalid resource id %d for configuration.", stage_table->resource_id);
		status = BF_NOT_SUPPORTED;
		goto cleanup;
	}

	status = rte_swx_ctl_pipeline_commit(ctl, 1);
	if (status) {
		LOG_ERROR("rte_swx_ctl_pipeline_commit failed");
		status = BF_UNEXPECTED;
	}

cleanup:
	LOG_TRACE("Exiting %s", __func__);
	return BF_SUCCESS;
}

int
dal_value_lookup_ent_get_first(void)
{
	LOG_TRACE("Entering %s", __func__);
	LOG_TRACE("Exiting %s", __func__);
	return BF_NOT_SUPPORTED;
}

int
dal_value_lookup_ent_get_next_n_by_key(void)
{
	LOG_TRACE("Entering %s", __func__);
	LOG_TRACE("Exiting %s", __func__);
	return BF_NOT_SUPPORTED;
}

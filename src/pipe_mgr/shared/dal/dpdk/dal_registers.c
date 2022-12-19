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
 * @file dal_registers.c
 *
 * @description Utilities for registers
 */
#include <infra/dpdk_infra.h>
#include "../dal_registers.h"
#include "../../pipe_mgr_shared_intf.h"
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/shared/pipe_mgr_mat.h>
#include <pipe_mgr/core/pipe_mgr_ctx_json.h>

bf_status_t
dal_reg_read_indirect_register_set(bf_dev_target_t dev_tgt,
				   const char *table_name,
				   int id,
				   pipe_stful_mem_query_t *stful_query)
{
	struct pipe_mgr_externs_ctx *externs_entry = NULL;
	struct pipe_mgr_p4_pipeline *ctx_obj = NULL;
	char key_name[P4_SDE_TABLE_NAME_LEN] = {0};
	struct pipe_mgr_profile *profile = NULL;
	bf_status_t status = BF_SUCCESS;
	struct pipeline *pipe = NULL;
	const char *ptr = NULL;
	uint64_t value = 0;

	status = pipe_mgr_get_profile(dev_tgt.device_id,
				      dev_tgt.dev_pipe_id, &profile);
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

	/* retrieve context json object associated with dev_tgt */
	status = pipe_mgr_get_profile_ctx(dev_tgt, &ctx_obj);

	if (status) {
		LOG_ERROR("context object not found for a profile");
		return BF_OBJECT_NOT_FOUND;
	}
	/* extract table name which is used as a key in hash map */
	ptr = trim_classifier_str((char *)table_name);

	if (!ptr)
		return BF_OBJECT_NOT_FOUND;

	strncpy(key_name, ptr, P4_SDE_TABLE_NAME_LEN - 1);

	externs_entry = bf_hashtbl_search(ctx_obj->bf_externs_htbl, key_name);

	if (!externs_entry) {
		LOG_ERROR("externs object/entry get for table \"%s\" failed",
			  table_name);
		return BF_OBJECT_NOT_FOUND;
	}

	/* read register value from dpdk pipeline */
	status = rte_swx_ctl_pipeline_regarray_read(pipe->p,
						    externs_entry->target_name,
						    id,
						    &value);

	if (status) {
		LOG_ERROR("%s:Register read failed for Name[%s][%d]\n"
				, __func__,
				externs_entry->target_name,
				id);

		return BF_OBJECT_NOT_FOUND;
	}

	stful_query->data->dbl = value;

	return BF_SUCCESS;
}

bf_status_t
dal_reg_write_assignable_register_set(bf_dev_target_t dev_tgt,
				      const char *name,
				      int id,
				      pipe_stful_mem_spec_t *stful_spec)
{
	struct  pipe_mgr_externs_ctx *externs_entry = NULL;
	char    key_name[P4_SDE_TABLE_NAME_LEN]     = {0};
	struct  pipe_mgr_p4_pipeline *ctx_obj       = NULL;
	struct  pipe_mgr_profile *profile           = NULL;
	bf_status_t  status = BF_SUCCESS;
	struct  pipeline *pipe;
	const char  *ptr    = NULL;

	status = pipe_mgr_get_profile(dev_tgt.device_id,
				      dev_tgt.dev_pipe_id, &profile);
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

	/* retrieve context json object associated with dev_tgt */
	status = pipe_mgr_get_profile_ctx(dev_tgt, &ctx_obj);

	if (status) {
		LOG_ERROR("context object not found for a profile");
		return BF_OBJECT_NOT_FOUND;
	}

	/* extract table name which is used as a key in hash map */
	ptr = trim_classifier_str((char *)name);
	strncpy(key_name, ptr, P4_SDE_TABLE_NAME_LEN - 1);
	externs_entry = bf_hashtbl_search(ctx_obj->bf_externs_htbl, key_name);

	if (!externs_entry) {
		LOG_ERROR("externs object/entry get for table \"%s\" failed",
			  name);
		return BF_OBJECT_NOT_FOUND;
	}
	status = rte_swx_ctl_pipeline_regarray_write(pipe->p,
						     externs_entry->target_name,
						     id,
						     stful_spec->dbl);
	if (status) {
		LOG_ERROR("%s:Register write failed for Name[%s][%d]\n",
			  __func__, name, id);
		return BF_OBJECT_NOT_FOUND;
	}

	return BF_SUCCESS;
}

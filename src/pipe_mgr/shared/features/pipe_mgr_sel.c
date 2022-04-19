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
#include <osdep/p4_sde_osdep_utils.h>
#include "pipe_mgr/shared/pipe_mgr_infra.h"
#include "pipe_mgr/shared/pipe_mgr_mat.h"
#include  "../dal/dal_mat.h"
#include "../../core/pipe_mgr_log.h"
#include "../infra/pipe_mgr_ctx_util.h"
#include "../infra/pipe_mgr_tbl.h"

static int pipe_mgr_sel_delete_entry_data(struct pipe_mgr_sel_entry_info *
		entry)
{
	P4_SDE_FREE(entry->mbrs);
	P4_SDE_FREE(entry);
	return BF_SUCCESS;
}

static int pipe_mgr_sel_pack_entry_data(u32 sel_tbl_hdl,
		u32 sel_grp_hdl,
		uint32_t num_mbrs,
		u32 *mbrs,
		struct pipe_mgr_sel_entry_info **entry_info)
{
	struct pipe_mgr_sel_entry_info *entry;
	int status;

	entry = P4_SDE_CALLOC(1, sizeof(*entry));
	if (!entry)
		return BF_NO_SYS_RESOURCES;

	entry->sel_tbl_hdl = sel_tbl_hdl;
	entry->sel_grp_hdl = sel_grp_hdl;
	entry->num_mbrs = num_mbrs;

	entry->mbrs = P4_SDE_CALLOC(num_mbrs, sizeof(*mbrs));
	if (!entry->mbrs) {
		status = BF_NO_SYS_RESOURCES;
		goto error;
	}
	memcpy(entry->mbrs, mbrs, num_mbrs*sizeof(*mbrs));

	*entry_info = entry;
	return BF_SUCCESS;
error:
	P4_SDE_FREE(entry);
	*entry_info = NULL;
	return status;
}

int pipe_mgr_sel_grp_add(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 sel_tbl_hdl,
		uint32_t max_grp_size,
		u32 *sel_grp_hdl_p,
		uint32_t pipe_api_flags)
{
	struct pipe_mgr_mat_state *tbl_state;
	struct pipe_mgr_mat *tbl;
	u32 new_ent_hdl;
	int status;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_is_pipe_valid(dev_tgt.device_id, dev_tgt.dev_pipe_id);
	if (status) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_ctx_get_tbl(dev_tgt, sel_tbl_hdl, &tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  sel_tbl_hdl);
		goto cleanup;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  sel_tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	status = dal_table_sel_ent_add_del(sess_hdl, dev_tgt, sel_tbl_hdl,
				   pipe_api_flags, &tbl->ctx, &new_ent_hdl, 0);

	if (status) {
		LOG_ERROR("dal_table_sel_ent_add_del add failed");
		goto cleanup_tbl_unlock;
	}

	*sel_grp_hdl_p = new_ent_hdl;

	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed", sel_tbl_hdl);
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}

	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;

cleanup_tbl_unlock:
	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
		LOG_ERROR("Unlock of table %d failed", sel_tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

int pipe_mgr_sel_grp_mbrs_set(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 sel_tbl_hdl,
		u32 sel_grp_hdl,
		uint32_t num_mbrs,
		u32 *mbrs,
		bool *enable,
		uint32_t pipe_api_flags)
{
	struct pipe_mgr_mat_state *adt_tbl_state;
	struct pipe_mgr_sel_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct pipe_mgr_mat *adt_tbl;
	struct pipe_mgr_mat *tbl;
	int status;
	u64 key;
	u32 i;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_is_pipe_valid(dev_tgt.device_id, dev_tgt.dev_pipe_id);
	if (status) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_ctx_get_tbl(dev_tgt, sel_tbl_hdl, &tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  sel_tbl_hdl);
		goto cleanup;
	}

	/* get the ADT table handle*/
	status = pipe_mgr_ctx_get_tbl(dev_tgt,
			tbl->ctx.adt_handle, &adt_tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  sel_tbl_hdl);
		goto cleanup;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  sel_tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	/*Acquiring the mutux for the ADT table associated with this sel tbl*/
	adt_tbl_state = adt_tbl->state;
	status = P4_SDE_MUTEX_LOCK(&adt_tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  sel_tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	status = pipe_mgr_sel_pack_entry_data(sel_tbl_hdl, sel_grp_hdl,
			num_mbrs, mbrs, &entry);
	if (status) {
		LOG_ERROR("Entry encoding failed");
		goto cleanup_tbl_unlock;
	}

	status = dal_table_sel_member_add_del(sess_hdl, dev_tgt, sel_tbl_hdl,
				   sel_grp_hdl, num_mbrs, mbrs,
				   pipe_api_flags, &tbl->ctx, 0);

	/* sel_grp_hdl to entry map */
	if (status == BF_SUCCESS) {
		key = (u64) sel_grp_hdl;
		/* Insert into the entry_handle-entry map */
		map_sts = P4_SDE_MAP_ADD(&tbl_state->entry_info_htbl, key,
					 (void *)entry);
		if (map_sts != BF_MAP_OK) {
			LOG_ERROR("Error in inserting entry info");
			status = BF_NO_SYS_RESOURCES;
			goto cleanup_map_add;
		}
	} else {
		LOG_ERROR("dal_table_sel_member_add_del add failed");
		goto cleanup_map_add;
	}

	/* incrementing the members reference count */
	for (i = 0; i < num_mbrs; i++) {
		status = pipe_mgr_adt_member_reference_add_delete(dev_tgt,
				tbl->ctx.adt_handle,
				mbrs[i], 0);
		if (status) {
			LOG_ERROR("ADT member reference add failed");
			status = BF_UNEXPECTED;
			goto cleanup_map_add;
		}
	}

	if (P4_SDE_MUTEX_UNLOCK(&adt_tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed",
				tbl->ctx.adt_handle);
		status = BF_UNEXPECTED;
		goto cleanup_map_add;
	}

	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed", sel_tbl_hdl);
		status = BF_UNEXPECTED;
		goto cleanup_map_add;
	}

	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;

cleanup_map_add:
	pipe_mgr_sel_delete_entry_data(entry);

cleanup_tbl_unlock:
	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
		LOG_ERROR("Unlock of table %d failed", sel_tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);
		LOG_ERROR("Unexpected error");

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

int pipe_mgr_get_first_group_member(u32 sess_hdl,
		u32 tbl_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 sel_grp_hdl,
		u32 *mbr_hdl)
{
	struct pipe_mgr_sel_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct pipe_mgr_mat *tbl;
	int status;
	u64 key;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_is_pipe_valid(dev_tgt.device_id, dev_tgt.dev_pipe_id);
	if (status) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_ctx_get_tbl(dev_tgt, tbl_hdl, &tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  tbl_hdl);
		goto cleanup;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	key = (u64) sel_grp_hdl;
	map_sts = P4_SDE_MAP_GET(&tbl_state->entry_info_htbl, key,
			(void **)&entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in inserting entry info");
		status = BF_NO_SYS_RESOURCES;
		goto cleanup_tbl_unlock;
	}

	if (entry->num_mbrs) {
		*mbr_hdl = entry->mbrs[0];
	}

	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed", tbl_hdl);
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}

	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;

cleanup_tbl_unlock:
	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
		LOG_ERROR("Unlock of table %d failed", tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);
		LOG_ERROR("Unexpected error");

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

int pipe_mgr_get_sel_grp_mbr_count(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 tbl_hdl,
		u32 grp_hdl,
		uint32_t *count)
{
	struct pipe_mgr_sel_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct pipe_mgr_mat *tbl;
	int status;
	u64 key;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_is_pipe_valid(dev_tgt.device_id, dev_tgt.dev_pipe_id);
	if (status) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_ctx_get_tbl(dev_tgt, tbl_hdl, &tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  tbl_hdl);
		goto cleanup;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	key = (u64) grp_hdl;
	map_sts = P4_SDE_MAP_GET(&tbl_state->entry_info_htbl, key,
			(void **)&entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in inserting entry info");
		status = BF_NO_SYS_RESOURCES;
		goto cleanup_tbl_unlock;
	}

	*count = entry->num_mbrs;

	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed", tbl_hdl);
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}

	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;

cleanup_tbl_unlock:
	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
		LOG_ERROR("Unlock of table %d failed", tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);
		LOG_ERROR("Unexpected error");

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

int pipe_mgr_sel_grp_mbrs_get(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 tbl_hdl,
		u32 sel_grp_hdl,
		uint32_t mbrs_size,
		u32 *mbrs,
		bool *enable,
		u32 *mbrs_populated)
{
	struct pipe_mgr_sel_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct pipe_mgr_mat *tbl;
	int status;
	u64 key;
	u32 i;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_is_pipe_valid(dev_tgt.device_id, dev_tgt.dev_pipe_id);
	if (status) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_ctx_get_tbl(dev_tgt, tbl_hdl, &tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  tbl_hdl);
		goto cleanup;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	key = (u64) sel_grp_hdl;
	map_sts = P4_SDE_MAP_GET(&tbl_state->entry_info_htbl, key,
			(void **)&entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in inserting entry info");
		status = BF_NO_SYS_RESOURCES;
		goto cleanup_tbl_unlock;
	}

	if (entry->num_mbrs) {
		for (i = 0; i < entry->num_mbrs && i < mbrs_size; i++) {
			mbrs[i] = entry->mbrs[i];
			enable[i] = 1;
			(*mbrs_populated)++;
		}
	}

	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed", tbl_hdl);
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}

	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;

cleanup_tbl_unlock:
	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
		LOG_ERROR("Unlock of table %d failed", tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

int pipe_mgr_sel_grp_del(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 tbl_hdl,
		u32 grp_hdl,
		uint32_t pipe_api_flags)
{
	struct pipe_mgr_mat_state *adt_tbl_state;
	struct pipe_mgr_sel_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct pipe_mgr_mat *adt_tbl;
	struct pipe_mgr_mat *tbl;
	int status;
	u64 key;
	u32 i;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_is_pipe_valid(dev_tgt.device_id, dev_tgt.dev_pipe_id);
	if (status) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_ctx_get_tbl(dev_tgt, tbl_hdl, &tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  tbl_hdl);
		goto cleanup;
	}

	/* get the ADT table handle*/
	status = pipe_mgr_ctx_get_tbl(dev_tgt,
			tbl->ctx.adt_handle, &adt_tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  tbl->ctx.adt_handle);
		goto cleanup;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	/*Acquiring the mutux for the ADT table associated with this sel tbl*/
	adt_tbl_state = adt_tbl->state;
	status = P4_SDE_MUTEX_LOCK(&adt_tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  tbl->ctx.adt_handle, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	key = (u64) grp_hdl;
	map_sts = P4_SDE_MAP_GET(&tbl_state->entry_info_htbl, key,
			(void **)&entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in inserting entry info");
		status = BF_NO_SYS_RESOURCES;
		goto cleanup_tbl_unlock;
	}

	if (entry->num_mbrs) {
		status = dal_table_sel_member_add_del(sess_hdl,
				dev_tgt, tbl_hdl,
				grp_hdl, entry->num_mbrs,
				entry->mbrs,
				pipe_api_flags, &tbl->ctx,
				1);

		if (status != BF_SUCCESS) {
			status = BF_UNEXPECTED;
			goto cleanup_tbl_unlock;
		}

		for (i = 0; i < entry->num_mbrs; i++) {
			status = pipe_mgr_adt_member_reference_add_delete(
					dev_tgt, tbl->ctx.adt_handle,
					entry->mbrs[i], 1);
			if (status) {
				LOG_ERROR("ADT member reference add failed");
				status = BF_UNEXPECTED;
				goto cleanup_tbl_unlock;
			}
		}
	}

	status = dal_table_sel_ent_add_del(sess_hdl, dev_tgt, tbl_hdl,
				   pipe_api_flags, &tbl->ctx, &grp_hdl, 1);

	/* sel_grp_hdl to entry map */
	if (status == BF_SUCCESS) {
		P4_SDE_MAP_RMV(&tbl_state->entry_info_htbl, key);
		pipe_mgr_sel_delete_entry_data(entry);
	} else {
		LOG_ERROR("dal_table_sel_member_add_del del failed");
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}

	if (P4_SDE_MUTEX_UNLOCK(&adt_tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed",
				tbl->ctx.adt_handle);
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}

	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed", tbl_hdl);
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}

	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;

cleanup_tbl_unlock:
	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
		LOG_ERROR("Unlock of table %d failed", tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

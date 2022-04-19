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

static void pipe_mgr_adt_delete_entry_data(
		struct pipe_mgr_adt_entry_info *entry)
{
	pipe_mgr_delete_act_data_spec(entry->act_data_spec);
	dal_delete_adt_table_entry_data(entry->dal_data);
	P4_SDE_FREE(entry);
}

static int pipe_mgr_adt_pack_entry_data(struct bf_dev_target_t dev_tgt,
		u32 act_fn_hdl,
		struct pipe_action_spec *act_data_spec,
		u32 ent_hdl,
		struct pipe_mgr_mat *tbl,
		struct pipe_mgr_adt_entry_info **entry_info)
{
	struct pipe_mgr_adt_entry_info *entry;
	int status;

	entry = P4_SDE_CALLOC(1, sizeof(*entry));
	if (!entry)
		return BF_NO_SYS_RESOURCES;
	entry->adt_ent_hdl = ent_hdl;
	entry->act_fn_hdl = act_fn_hdl;

	status = pipe_mgr_mat_pack_act_spec(&(entry->act_data_spec),
					    act_data_spec);
	if (status)
		goto error_act_spec;

	*entry_info = entry;
	return BF_SUCCESS;

error_act_spec:
	P4_SDE_FREE(entry);
	*entry_info = NULL;
	return status;
}

int pipe_mgr_adt_ent_add(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 adt_tbl_hdl,
		u32 act_fn_hdl,
		struct pipe_action_spec *action_spec,
		u32 *adt_ent_hdl_p,
		uint32_t pipe_api_flags)
{
	struct pipe_mgr_adt_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct pipe_mgr_mat *tbl;
	u32 new_ent_hdl;
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

	status = pipe_mgr_ctx_get_tbl(dev_tgt, adt_tbl_hdl, &tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  adt_tbl_hdl);
		goto cleanup;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  adt_tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	new_ent_hdl = P4_SDE_ID_ALLOC(tbl_state->entry_handle_array);
	if (new_ent_hdl == ENTRY_HANDLE_ARRAY_SIZE) {
		LOG_ERROR("entry handle allocator failed");
		status = BF_NO_SPACE;
		goto cleanup_tbl_unlock;
	}

	status = pipe_mgr_adt_pack_entry_data(dev_tgt, act_fn_hdl, action_spec,
			new_ent_hdl, tbl, &entry);
	if (status) {
		LOG_ERROR("Entry encoding failed");
		goto cleanup_id;
	}

	status = dal_table_adt_ent_add(sess_hdl, dev_tgt, adt_tbl_hdl,
				   act_fn_hdl, action_spec,
				   pipe_api_flags, &tbl->ctx, new_ent_hdl,
				   &(entry->dal_data));

	/* allocate entry handle and map the entry*/
	/* insert the match_key/entry handle mapping in to the hash */
	if (status == BF_SUCCESS) {
		key = (u64) new_ent_hdl;
		/* Insert into the entry_handle-entry map */
		map_sts = P4_SDE_MAP_ADD(&tbl_state->entry_info_htbl, key,
					 (void *)entry);
		if (map_sts != BF_MAP_OK) {
			LOG_ERROR("Error in inserting entry info");
			status = BF_NO_SYS_RESOURCES;
			goto cleanup_map_add;
		}
	} else {
		LOG_ERROR("dal_table_adt_ent_add failed");
		goto cleanup_map_add;
	}

	*adt_ent_hdl_p = new_ent_hdl;

	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed", adt_tbl_hdl);
		status = BF_UNEXPECTED;
		goto cleanup_map_add;
	}

	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;

cleanup_map_add:
	pipe_mgr_adt_delete_entry_data(entry);

cleanup_id:
	P4_SDE_ID_FREE(tbl_state->entry_handle_array, new_ent_hdl);

cleanup_tbl_unlock:
	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
		LOG_ERROR("Unlock of table %d failed", adt_tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

int pipe_mgr_get_action_data_entry(u32 sess_hdl,
		u32 tbl_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 entry_hdl,
		struct pipe_action_data_spec *pipe_action_data_spec,
		u32 *act_fn_hdl,
		bool from_hw)
{
	struct pipe_mgr_adt_entry_info *entry = NULL;
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

	key = (u64) entry_hdl;

	map_sts = P4_SDE_MAP_GET(&tbl_state->entry_info_htbl, key,
			(void **) &entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in getting entry info");
		status = BF_NO_SYS_RESOURCES;
		goto cleanup_tbl_unlock;
	}

	pipe_action_data_spec->num_valid_action_data_bits =
		entry->act_data_spec->act_data.num_valid_action_data_bits;
	pipe_action_data_spec->num_action_data_bytes =
		entry->act_data_spec->act_data.num_action_data_bytes;
	memcpy(pipe_action_data_spec->action_data_bits,
			entry->act_data_spec->act_data.action_data_bits,
			entry->act_data_spec->act_data.num_action_data_bytes);
	*act_fn_hdl = entry->act_fn_hdl;

	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed", tbl_hdl);
		status = BF_UNEXPECTED;
		goto cleanup;
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

int pipe_mgr_adt_ent_del(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 adt_tbl_hdl,
		u32 adt_ent_hdl,
		uint32_t pipe_api_flags)
{
	struct pipe_mgr_adt_entry_info *entry;
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

	status = pipe_mgr_ctx_get_tbl(dev_tgt, adt_tbl_hdl, &tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  adt_tbl_hdl);
		goto cleanup;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  adt_tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	key = (u64) adt_ent_hdl;

	map_sts = P4_SDE_MAP_GET(&tbl_state->entry_info_htbl, key,
				(void **) &entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in getting entry info");
		status = BF_NO_SYS_RESOURCES;
		goto cleanup_tbl_unlock;
	}

	status = dal_table_adt_ent_del(sess_hdl, dev_tgt, adt_tbl_hdl,
				   pipe_api_flags, &tbl->ctx, adt_ent_hdl,
				   &(entry->dal_data));
	if (status == BF_SUCCESS) {
		P4_SDE_ID_FREE(tbl->state->entry_handle_array, key);
		P4_SDE_MAP_RMV(&tbl_state->entry_info_htbl, key);
		pipe_mgr_adt_delete_entry_data(entry);
	} else {
		LOG_ERROR("pipe_mgr_adt_ent_del failed");
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}

	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed", adt_tbl_hdl);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);
	if (status) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	LOG_TRACE("Exiting %s", __func__);
	return status;

cleanup_tbl_unlock:
	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
		LOG_ERROR("Unlock of table %d failed", adt_tbl_hdl);
cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

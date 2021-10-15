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

void pipe_mgr_delete_act_data_spec(struct pipe_action_spec *ads)
{
	P4_SDE_FREE(ads->act_data.action_data_bits);
	P4_SDE_FREE(ads);
}

int pipe_mgr_mat_unpack_act_spec(
			struct pipe_action_spec *ads,
			struct pipe_action_spec *act_data_spec)
{
	act_data_spec->pipe_action_datatype_bmap =
		ads->pipe_action_datatype_bmap;
	act_data_spec->adt_ent_hdl = ads->adt_ent_hdl;
	act_data_spec->act_data.num_valid_action_data_bits =
		ads->act_data.num_valid_action_data_bits;
	act_data_spec->act_data.num_action_data_bytes =
		ads->act_data.num_action_data_bytes;
	memcpy(act_data_spec->act_data.action_data_bits,
			ads->act_data.action_data_bits,
			ads->act_data.num_action_data_bytes);

	return BF_SUCCESS;
}

int pipe_mgr_mat_pack_act_spec(
			struct pipe_action_spec **act_data_spec_out,
			struct pipe_action_spec *ads)
{
	struct pipe_action_spec *act_data_spec;
	act_data_spec = P4_SDE_CALLOC(1, sizeof(*act_data_spec));
	if (!act_data_spec)
		return BF_NO_SYS_RESOURCES;
	act_data_spec->pipe_action_datatype_bmap =
		ads->pipe_action_datatype_bmap;
	act_data_spec->adt_ent_hdl = ads->adt_ent_hdl;
	act_data_spec->act_data.num_valid_action_data_bits =
		ads->act_data.num_valid_action_data_bits;
	act_data_spec->act_data.num_action_data_bytes =
		ads->act_data.num_action_data_bytes;
	act_data_spec->act_data.action_data_bits =
		P4_SDE_CALLOC(ads->act_data.num_action_data_bytes,
				sizeof(u8));
	if (!act_data_spec->act_data.action_data_bits)
		goto error_action_data_bits;
	memcpy(act_data_spec->act_data.action_data_bits,
			ads->act_data.action_data_bits,
			ads->act_data.num_action_data_bytes);

	*act_data_spec_out = act_data_spec;
	return BF_SUCCESS;


error_action_data_bits:
	P4_SDE_FREE(act_data_spec);
	return BF_NO_SYS_RESOURCES;
}

static void pipe_mgr_delete_match_spec(struct pipe_tbl_match_spec *match_spec)
{
	P4_SDE_FREE(match_spec->match_mask_bits);
	P4_SDE_FREE(match_spec->match_value_bits);
	P4_SDE_FREE(match_spec);
}

static int pipe_mgr_mat_unpack_match_spec(
			struct pipe_tbl_match_spec *ms,
			struct pipe_tbl_match_spec *match_spec)
{
	match_spec->num_valid_match_bits = ms->num_valid_match_bits;
	match_spec->num_match_bytes = ms->num_match_bytes;
	match_spec->priority = ms->priority;
	memcpy(match_spec->match_value_bits, ms->match_value_bits,
			ms->num_match_bytes);

	memcpy(match_spec->match_mask_bits, ms->match_mask_bits,
			ms->num_match_bytes);

	return BF_SUCCESS;
}

static int pipe_mgr_mat_pack_match_spec(
			struct pipe_tbl_match_spec **match_spec_out,
			struct pipe_tbl_match_spec *ms)
{
	struct pipe_tbl_match_spec *match_spec;
	match_spec = P4_SDE_CALLOC(1, sizeof(*match_spec));
	if (!match_spec)
		return BF_NO_SYS_RESOURCES;
	match_spec->num_valid_match_bits = ms->num_valid_match_bits;
	match_spec->num_match_bytes = ms->num_match_bytes;
	match_spec->priority = ms->priority;
	match_spec->match_value_bits = P4_SDE_CALLOC(ms->num_match_bytes,
							sizeof(u8));
	if (!match_spec->match_value_bits)
		goto error_match_value;
	memcpy(match_spec->match_value_bits, ms->match_value_bits,
			ms->num_match_bytes);

	match_spec->match_mask_bits = P4_SDE_CALLOC(ms->num_match_bytes,
							sizeof(u8));
	if (!match_spec->match_mask_bits)
		goto error_match_mask;
	memcpy(match_spec->match_mask_bits, ms->match_mask_bits,
			ms->num_match_bytes);

	*match_spec_out = match_spec;
	return BF_SUCCESS;

error_match_mask:
	P4_SDE_FREE(match_spec->match_value_bits);
error_match_value:
	P4_SDE_FREE(match_spec);
	return BF_NO_SYS_RESOURCES;
}

static int pipe_mgr_mat_unpack_entry_data(struct bf_dev_target_t dev_tgt,
		struct pipe_tbl_match_spec *match_spec,
		u32 *act_fn_hdl,
		struct pipe_action_spec *act_data_spec,
		u32 ent_hdl,
		struct pipe_mgr_mat *tbl,
		struct pipe_mgr_mat_entry_info *entry)
{
	int status;

	if (ent_hdl != entry->mat_ent_hdl) {
		LOG_ERROR("entry unpack error");
		return BF_UNEXPECTED;
	}

	*act_fn_hdl = entry->act_fn_hdl;

	status = pipe_mgr_mat_unpack_match_spec(entry->match_spec,
			match_spec);
	if (status) {
		LOG_ERROR("match_spec unpack failed");
		return status;
	}

	status = pipe_mgr_mat_unpack_act_spec(entry->act_data_spec,
					    act_data_spec);
	if (status) {
		LOG_ERROR("act_spec unpack failed");
		return status;
	}

	return BF_SUCCESS;
}

static int pipe_mgr_mat_pack_entry_data(struct bf_dev_target_t dev_tgt,
		struct pipe_tbl_match_spec *match_spec,
		u32 act_fn_hdl,
		struct pipe_action_spec *act_data_spec,
		u32 ent_hdl,
		struct pipe_mgr_mat *tbl,
		struct pipe_mgr_mat_entry_info **entry_info)
{
	struct pipe_mgr_mat_entry_info *entry;
	int status;

	entry = P4_SDE_CALLOC(1, sizeof(*entry));
	if (!entry)
		return BF_NO_SYS_RESOURCES;
	entry->mat_ent_hdl = ent_hdl;
	entry->act_fn_hdl = act_fn_hdl;

	status = pipe_mgr_mat_pack_match_spec(&(entry->match_spec),
			match_spec);
	if (status)
		goto error_match_spec;

	status = pipe_mgr_mat_pack_act_spec(&(entry->act_data_spec),
					    act_data_spec);
	if (status)
		goto error_act_spec;

	*entry_info = entry;
	return BF_SUCCESS;
error_act_spec:
	pipe_mgr_delete_match_spec(entry->match_spec);
error_match_spec:
	P4_SDE_FREE(entry);
	*entry_info = NULL;
	return status;
}

static void pipe_mgr_mat_delete_entry_data(
		struct pipe_mgr_mat_entry_info *entry)
{
	pipe_mgr_delete_match_spec(entry->match_spec);
	pipe_mgr_delete_act_data_spec(entry->act_data_spec);
	dal_delete_table_entry_data(entry->dal_data);
	P4_SDE_FREE(entry);
}

int pipe_mgr_match_spec_to_ent_hdl
	(u32 sess_hdl,
	 struct bf_dev_target_t dev_tgt,
	 u32 mat_tbl_hdl,
	 struct pipe_tbl_match_spec *match_spec,
	 u32 *ent_hdl_p)
{
	struct pipe_mgr_mat *tbl;
	bool exists;
	int status;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_ctx_get_tbl(dev_tgt, mat_tbl_hdl, &tbl);
	if (status) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_mat_tbl_key_exists(tbl, match_spec,
					     dev_tgt.dev_pipe_id,
					     &exists, ent_hdl_p);
	if (status) {
		LOG_ERROR("pipe_mgr_mat_tbl_key_exists failed");
		return status;
	}

	if (!exists) {
		LOG_TRACE("entry not found");
		return BF_OBJECT_NOT_FOUND;
	}

	return status;
}

int pipe_mgr_get_entry(u32 sess_hdl,
		u32 mat_tbl_hdl,
		int dev_id,
		u32 entry_hdl,
		struct pipe_tbl_match_spec *match_spec,
		struct pipe_action_spec *act_data_spec,
		u32 *act_fn_hdl,
		bool from_hw,
		uint32_t res_get_flags,
		void *res_data)
{
	struct pipe_mgr_mat_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct bf_dev_target_t dev_tgt;
	struct pipe_mgr_mat *tbl;
	unsigned long key;
	int status;

	LOG_TRACE("Entering %s", __func__);
	dev_tgt.device_id = dev_id;
	/* TODO: Why does this API (many other pipe_mgr_get_* API) use
	 *	 only dev_id and not dev_tgt which has pipe_id as well?
	 */
	dev_tgt.dev_pipe_id = PIPE_MGR_DEFAULT_PIPE_ID;

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_ctx_get_tbl(dev_tgt, mat_tbl_hdl, &tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  mat_tbl_hdl);
		goto cleanup;
	}

	tbl_state = tbl->state;
	key = (unsigned long) entry_hdl;

	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  mat_tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	map_sts = P4_SDE_MAP_GET(&tbl_state->entry_info_htbl, key,
				 (void **)&entry);
	if (map_sts != BF_MAP_OK) {
		if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
			LOG_ERROR("Unlock of table %d failed", mat_tbl_hdl);
		LOG_TRACE("Exiting %s", __func__);
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}
	status = pipe_mgr_mat_unpack_entry_data(dev_tgt, match_spec,
			act_fn_hdl, act_data_spec,
			entry_hdl, tbl, entry);
	if (status) {
		LOG_ERROR("Unpacking enty data failed for entry hdl %d\n",
			  entry_hdl);
		goto cleanup_tbl_unlock;
	}

	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed", mat_tbl_hdl);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;

cleanup_tbl_unlock:
	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
		LOG_ERROR("Unlock of table %d failed", mat_tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

int pipe_mgr_mat_ent_add(u32 sess_hdl,
			 struct bf_dev_target_t dev_tgt,
			 u32 mat_tbl_hdl,
			 struct pipe_tbl_match_spec *match_spec,
			 u32 act_fn_hdl,
			 struct pipe_action_spec *act_data_spec,
			 u32 ttl, u32 pipe_api_flags,
			 u32 *ent_hdl_p)
{
	struct pipe_mgr_mat_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	struct pipe_mgr_mat *tbl;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	u32 new_ent_hdl;
	int status;
	u64 key;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_ctx_get_tbl(dev_tgt, mat_tbl_hdl, &tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  mat_tbl_hdl);
		goto cleanup;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  mat_tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	new_ent_hdl = P4_SDE_ID_ADD(tbl_state->entry_handle_array);
	if (new_ent_hdl == ENTRY_HANDLE_ARRAY_SIZE) {
		LOG_ERROR("entry handle allocator failed");
		status = BF_NO_SPACE;
		goto cleanup_tbl_unlock;
	}

	status = pipe_mgr_mat_pack_entry_data(dev_tgt, match_spec,
			act_fn_hdl, act_data_spec,
			new_ent_hdl, tbl, &entry);
	if (status) {
		LOG_ERROR("Entry encoding failed");
		goto cleanup_id;
	}

	status = dal_table_ent_add(sess_hdl, dev_tgt, mat_tbl_hdl, match_spec,
				   act_fn_hdl, act_data_spec, ttl,
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
		/* Insert the match_spec-entry_handle hash*/
		status = pipe_mgr_mat_tbl_key_insert(dev_tgt,
				tbl,
				entry->match_spec,
				new_ent_hdl);
		if (status) {
			LOG_ERROR("Error in inserting match_spec-entry_handle"
					" hash");
			P4_SDE_MAP_RMV(&tbl_state->entry_info_htbl, key);
			goto cleanup_map_add;
		}
	} else {
		LOG_ERROR("dal_table_ent_add failed");
		goto cleanup_map_add;
	}

	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock)) {
		LOG_ERROR("Unlock of table %d failed", mat_tbl_hdl);
		status = BF_UNEXPECTED;
		goto cleanup_map_add;
	}

	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;

cleanup_map_add:
	pipe_mgr_mat_delete_entry_data(entry);

cleanup_id:
	P4_SDE_ID_RMV(tbl_state->entry_handle_array, new_ent_hdl);

cleanup_tbl_unlock:
	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
		LOG_ERROR("Unlock of table %d failed", mat_tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

int pipe_mgr_mat_ent_del_by_match_spec(u32 sess_hdl,
				       struct bf_dev_target_t dev_tgt,
				       u32 mat_tbl_hdl,
				       struct pipe_tbl_match_spec *match_spec,
				       u32 pipe_api_flags)
{
	struct pipe_mgr_mat_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct pipe_mgr_mat *tbl;
	unsigned long key;
	u32 mat_ent_hdl;
	bool exists;
	int status;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_ctx_get_tbl(dev_tgt, mat_tbl_hdl, &tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  mat_tbl_hdl);
		goto cleanup;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  mat_tbl_hdl, status);
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	status = pipe_mgr_mat_tbl_key_exists(tbl, match_spec,
					     dev_tgt.dev_pipe_id,
					     &exists, &mat_ent_hdl);
	if (status) {
		LOG_ERROR("pipe_mgr_mat_tbl_key_exists failed");
		goto cleanup_tbl_unlock;
	}

	if (!exists) {
		LOG_TRACE("entry not found");
		goto cleanup_tbl_unlock;
	}

	key = (unsigned long) mat_ent_hdl;

	map_sts = P4_SDE_MAP_GET(&tbl->state->entry_info_htbl, key,
				 (void **)&entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("table entry handle/entry map get failed");
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}

	status = dal_table_ent_del_by_match_spec(sess_hdl, dev_tgt, mat_tbl_hdl,
						 match_spec, pipe_api_flags,
						 &tbl->ctx, mat_ent_hdl,
						 entry->dal_data);

	if (status) {
		LOG_ERROR("dal table entry del failed");
		goto cleanup_tbl_unlock;
	}

	map_sts = P4_SDE_MAP_RMV(&tbl->state->entry_info_htbl, key);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("table entry handle/entry map del failed");
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}

	status = pipe_mgr_mat_tbl_key_delete(dev_tgt,
					tbl,
					match_spec,
					mat_ent_hdl);
	if (status) {
		LOG_ERROR("table entry match_spec/entry_handle del failed");
		goto cleanup_tbl_unlock;
	}

	P4_SDE_ID_RMV(tbl->state->entry_handle_array, mat_ent_hdl);

	pipe_mgr_mat_delete_entry_data(entry);

cleanup_tbl_unlock:
	if (P4_SDE_MUTEX_UNLOCK(&tbl_state->lock))
		LOG_ERROR("Unlock of table %d failed", mat_tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

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
#include "../infra/pipe_mgr_int.h"

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

/* Supports fetching either entry index or handle in a single call - not both.
 */
static pipe_status_t pipe_mgr_adt_get_mbr_id_hdl_int(
    bf_dev_target_t dev_tgt,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_mbr_id_t mbr_id,
    pipe_adt_ent_hdl_t ent_hdl,
    pipe_adt_ent_hdl_t *adt_ent_hdl,
    pipe_adt_mbr_id_t *adt_mbr_id,
    struct pipe_mgr_adt_ent_data *adt_ent_data) {

	struct pipe_mgr_adt_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct pipe_mgr_mat *tbl;
	int status;
	u64 key;

	if (adt_ent_hdl == NULL && adt_mbr_id == NULL &&
	    adt_ent_data == NULL) {
		LOG_ERROR("Ivalid output arguments passed");
		return BF_INVALID_ARG;
	}

	if (ent_hdl == 0 && mbr_id == 0) {
		LOG_ERROR("Ivalid input arguments passed");
		return BF_INVALID_ARG;
	}

	status = pipe_mgr_ctx_get_table(dev_tgt, adt_tbl_hdl,
					PIPE_MGR_TABLE_TYPE_ADT, (void *)&tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  adt_tbl_hdl);
		return BF_OBJECT_NOT_FOUND;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  adt_tbl_hdl, status);
		return BF_UNEXPECTED;
	}

	if (ent_hdl) {
		key = (u64) ent_hdl;
		map_sts = P4_SDE_MAP_GET(&tbl_state->entry_info_htbl, key,
				(void **) &entry);
	} else if (mbr_id) {
		key = (u64) mbr_id;
		map_sts = P4_SDE_MAP_GET(&tbl_state->mbr_id_htbl, key,
				(void **) &entry);
	}

	P4_SDE_MUTEX_UNLOCK(&tbl_state->lock);

	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in getting entry info");
		return BF_NO_SYS_RESOURCES;
	}

	if (adt_ent_hdl)
		*adt_ent_hdl = entry->adt_ent_hdl;
	if (adt_mbr_id)
		*adt_mbr_id = entry->mbr_id;
	if (adt_ent_data)
		*adt_ent_data = *entry->entry_data;

  return BF_SUCCESS;
}

static pipe_status_t pipe_mgr_adt_default_get_mbr_id_hdl_int (bf_dev_target_t
							      dev_tgt,
							      pipe_adt_tbl_hdl_t
							      adt_tbl_hdl,
							      pipe_adt_mbr_id_t
							      mbr_id,
							      pipe_adt_ent_hdl_t
							      *adt_ent_hdl,
							      pipe_adt_mbr_id_t
							      *adt_mbr_id,
							      struct pipe_mgr_adt_ent_data
							      *adt_ent_data)
{
	struct pipe_mgr_adt_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct pipe_mgr_mat *tbl;
	int status;
	u64 key;

	if (!adt_ent_hdl && !adt_mbr_id  && !adt_ent_data) {
		LOG_ERROR("Ivalid output arguments passed");
		return BF_INVALID_ARG;
	}

	status = pipe_mgr_ctx_get_table(dev_tgt, adt_tbl_hdl,
					PIPE_MGR_TABLE_TYPE_ADT, (void *)&tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  adt_tbl_hdl);
		return BF_OBJECT_NOT_FOUND;
	}

	tbl_state = tbl->state;
	status = P4_SDE_MUTEX_LOCK(&tbl_state->lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d",
			  adt_tbl_hdl, status);
		return BF_UNEXPECTED;
	}

	key = (u64)mbr_id;
	map_sts = P4_SDE_MAP_GET(&tbl_state->mbr_id_htbl, key,
				 (void **)&entry);

	P4_SDE_MUTEX_UNLOCK(&tbl_state->lock);

	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in getting entry info");
		return BF_NO_SYS_RESOURCES;
	}

	if (adt_ent_hdl)
		*adt_ent_hdl = entry->adt_ent_hdl;
	if (adt_mbr_id)
		*adt_mbr_id = entry->mbr_id;
	if (adt_ent_data)
		*adt_ent_data = *entry->entry_data;

	return BF_SUCCESS;
}

/*!
 * Used to get handle for specified entry member index.
 */
int pipe_mgr_adt_ent_hdl_get(pipe_sess_hdl_t shdl,
                                       bf_dev_target_t dev_tgt,
                                       pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                       pipe_adt_mbr_id_t mbr_id,
                                       pipe_adt_ent_hdl_t *adt_ent_hdl) {
	int status = PIPE_SUCCESS;
	LOG_TRACE("Entering %s", __func__);

        status = pipe_mgr_is_pipe_valid(dev_tgt.device_id, dev_tgt.dev_pipe_id);
        if (status) {
		LOG_TRACE("Exiting %s", __func__);
                return status;
	}

	status = pipe_mgr_api_prologue(shdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_adt_get_mbr_id_hdl_int(
		dev_tgt, adt_tbl_hdl, mbr_id, 0, adt_ent_hdl, NULL, NULL);
	pipe_mgr_api_epilogue(shdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

int pipe_mgr_adt_default_ent_hdl_get(pipe_sess_hdl_t shdl,
				     bf_dev_target_t dev_tgt,
				     pipe_adt_tbl_hdl_t adt_tbl_hdl,
				     pipe_adt_mbr_id_t mbr_id,
				     pipe_adt_ent_hdl_t *adt_ent_hdl)
{
	int status = PIPE_SUCCESS;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_is_pipe_valid(dev_tgt.device_id, dev_tgt.dev_pipe_id);
	if (status) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_api_prologue(shdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_adt_default_get_mbr_id_hdl_int(dev_tgt, adt_tbl_hdl,
							 mbr_id, adt_ent_hdl,
							 NULL, NULL);
	pipe_mgr_api_epilogue(shdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/*!
 * Used to get entry member index for specified entry handle.
 */
int pipe_mgr_adt_mbr_id_get(pipe_sess_hdl_t shdl,
                                      bf_dev_target_t dev_tgt,
                                      pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                      pipe_adt_ent_hdl_t ent_hdl,
                                      pipe_adt_mbr_id_t *adt_mbr_id) {
	int status = PIPE_SUCCESS;

	LOG_TRACE("Entering %s", __func__);

        status = pipe_mgr_is_pipe_valid(dev_tgt.device_id, dev_tgt.dev_pipe_id);
        if (status) {
		LOG_TRACE("Exiting %s", __func__);
                return status;
	}

	status = pipe_mgr_api_prologue(shdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

  	status = pipe_mgr_adt_get_mbr_id_hdl_int(
      		dev_tgt, adt_tbl_hdl, 0, ent_hdl, NULL, adt_mbr_id, NULL);
	pipe_mgr_api_epilogue(shdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/*!
 * Used to get entry data and handle for specified member id.
 */
int pipe_mgr_adt_ent_data_get(pipe_sess_hdl_t shdl,
                                        bf_dev_target_t dev_tgt,
                                        pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                        pipe_adt_mbr_id_t mbr_id,
                                        pipe_adt_ent_hdl_t *adt_ent_hdl,
                                        pipe_mgr_adt_ent_data_t *ent_data) {
	int status = PIPE_SUCCESS;

	LOG_TRACE("Entering %s", __func__);

        status = pipe_mgr_is_pipe_valid(dev_tgt.device_id, dev_tgt.dev_pipe_id);
        if (status) {
		LOG_TRACE("Exiting %s", __func__);
                return status;
	}

	status = pipe_mgr_api_prologue(shdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}
	status = pipe_mgr_adt_get_mbr_id_hdl_int(
      		dev_tgt, adt_tbl_hdl, mbr_id, 0, adt_ent_hdl, NULL, ent_data);
	pipe_mgr_api_epilogue(shdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/* Packed format:
 *   tag, num_resources, act_fn_hdl, action_data.num_action_data_bytes,
 *   action_data.num_valid_action_data_bits, action data, resource specs */
static inline struct pipe_mgr_adt_ent_data *make_adt_ent_data(
	struct pipe_action_data_spec *action_data,
	u32 act_fn_hdl,
	int num_resources,
	struct adt_data_resources_ *resources) {

  	struct pipe_mgr_adt_ent_data *adt_data;
  	uint32_t as_data_offset;
  	size_t alloc_sz = 0;
  	void *data;

  	/* Compute required allocation size to hold all fields. */
  	alloc_sz += sizeof(struct pipe_mgr_adt_ent_data);
  	alloc_sz += action_data ? action_data->num_action_data_bytes : 0;
  	data = P4_SDE_MALLOC(alloc_sz);
  	if (!data) return NULL;

	adt_data = data;
  	adt_data->act_fn_hdl = act_fn_hdl;
  	adt_data->action_data.num_valid_action_data_bits =
      		action_data ? action_data->num_valid_action_data_bits : 0;
  	adt_data->action_data.num_action_data_bytes =
      		action_data ? action_data->num_action_data_bytes : 0;
  	if (adt_data->action_data.num_action_data_bytes) {
  		as_data_offset = sizeof(struct pipe_mgr_adt_ent_data);
    		adt_data->action_data.action_data_bits = (uint8_t *)data + as_data_offset;
    		memcpy(adt_data->action_data.action_data_bits,
                    action_data->action_data_bits,
                    action_data->num_action_data_bytes);
  	} else {
    		adt_data->action_data.action_data_bits = NULL;
  	}
  	adt_data->num_resources = num_resources;
  	if (adt_data->num_resources) {
    		memcpy(adt_data->adt_data_resources, resources,
                       num_resources * sizeof(struct adt_data_resources_));
  	}
  	adt_data->pad = 0;
  	return adt_data;
}

int pipe_mgr_adt_ent_add(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 adt_tbl_hdl,
		u32 act_fn_hdl,
    		u32 mbr_id,
		struct pipe_action_spec *action_spec,
		u32 *adt_ent_hdl_p,
		uint32_t pipe_api_flags)
{
	struct adt_data_resources_ *resources = NULL;
	struct pipe_action_data_spec *act_data_spec;
	struct pipe_mgr_adt_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct pipe_mgr_mat *tbl;
	u32 new_ent_hdl;
	int status;
	u64 key;
    	int i;

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

	status = pipe_mgr_ctx_get_table(dev_tgt, adt_tbl_hdl,
					PIPE_MGR_TABLE_TYPE_ADT, (void *)&tbl);
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
	entry->mbr_id = mbr_id;
	status = dal_table_adt_ent_add(sess_hdl, dev_tgt, adt_tbl_hdl,
				   act_fn_hdl, action_spec,
				   pipe_api_flags, &tbl->ctx, new_ent_hdl,
				   &(entry->dal_data));
	if (status) {
		LOG_ERROR("dal_table_adt_ent_add failed");
		goto cleanup_map_add;
	}

	if (action_spec->resource_count) {
		resources = P4_SDE_CALLOC(action_spec->resource_count,
					  sizeof(adt_data_resources_t));
		if (resources == NULL) {
			LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
			goto cleanup_map_add;
		}
    		for (i = 0; i < action_spec->resource_count; i++) {
      			resources[i].tbl_hdl = action_spec->resources[i].tbl_hdl;
      			resources[i].tbl_idx = action_spec->resources[i].tbl_idx;
    		}
  	}
	act_data_spec = &action_spec->act_data;
	entry->entry_data = make_adt_ent_data(act_data_spec, act_fn_hdl, action_spec->resource_count, resources);
  	if (entry->entry_data == NULL) {
    		LOG_ERROR(
        	"%s:%d Error in getting adt entry data for entry hdl %d, tbl 0x%x, "
        	"device id %d",
        	__func__,
        	__LINE__,
        	new_ent_hdl,
        	adt_tbl_hdl,
        	dev_tgt.device_id);
		goto cleanup_map_add;
  	}

	/* allocate entry handle and map the entry*/
	/* insert the match_key/entry handle mapping in to the hash */
	key = (u64) new_ent_hdl;
	/* Insert into the entry_handle-entry map */
	map_sts = P4_SDE_MAP_ADD(&tbl_state->entry_info_htbl, key,
				 (void *)entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in inserting entry info to entry htbl");
		status = BF_NO_SYS_RESOURCES;
		goto cleanup_map_add;
	}
	key = (u64) mbr_id;
	/* Insert into the entry_handle-entry map */
	map_sts = P4_SDE_MAP_ADD(&tbl_state->mbr_id_htbl, key,
				 (void *)entry);
	if (map_sts != BF_MAP_OK) {
		key = (u64) new_ent_hdl;
		P4_SDE_MAP_RMV(&tbl_state->entry_info_htbl, key);
		LOG_ERROR("Error in inserting entry info to mbrid htbl");
		status = BF_NO_SYS_RESOURCES;
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
	if(resources)
		P4_SDE_FREE(resources);

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

int pipe_mgr_adt_default_ent_add(u32 sess_hdl,
				 struct bf_dev_target_t dev_tgt,
				 u32 adt_tbl_hdl,
				 u32 act_fn_hdl,
				 u32 mbr_id,
				 struct pipe_action_spec *action_spec,
				 u32 *adt_ent_hdl_p,
				 uint32_t pipe_api_flags)
{
	struct adt_data_resources_ *resources = NULL;
	struct pipe_action_data_spec *act_data_spec;
	struct pipe_mgr_adt_entry_info *entry;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	struct pipe_mgr_mat *tbl;
	u32 new_ent_hdl;
	int status;
	u64 key;
	int i;

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

	status = pipe_mgr_ctx_get_table(dev_tgt, adt_tbl_hdl,
					PIPE_MGR_TABLE_TYPE_ADT, (void *)&tbl);
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
	entry->mbr_id = mbr_id;
	status = dal_table_adt_default_ent_add(sess_hdl, dev_tgt, adt_tbl_hdl,
					       act_fn_hdl, action_spec,
					       pipe_api_flags, &tbl->ctx,
					       new_ent_hdl, &entry->dal_data);
	if (status) {
		LOG_ERROR("dal_table_adt_default_ent_add failed");
		goto cleanup_map_add;
	}

	if (action_spec->resource_count) {
		resources = P4_SDE_CALLOC(action_spec->resource_count,
					  sizeof(adt_data_resources_t));
		if (!resources) {
			LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
			goto cleanup_map_add;
		}
		for (i = 0; i < action_spec->resource_count; i++) {
			resources[i].tbl_hdl = action_spec->resources[i].tbl_hdl;
			resources[i].tbl_idx = action_spec->resources[i].tbl_idx;
		}
	}
	act_data_spec = &action_spec->act_data;
	entry->entry_data = make_adt_ent_data(act_data_spec, act_fn_hdl,
					      action_spec->resource_count,
					      resources);
	if (!entry->entry_data) {
		LOG_ERROR("%s:%d Error in getting adt entry data for entry "
			  "hdl %d, tbl 0x%x, device id %d",
			  __func__,
			  __LINE__,
			  new_ent_hdl,
			  adt_tbl_hdl,
			  dev_tgt.device_id);
		goto cleanup_map_add;
	}

	/* allocate entry handle and map the entry*/
	/* insert the match_key/entry handle mapping in to the hash */
	key = (u64)new_ent_hdl;
	/* Insert into the entry_handle-entry map */
	map_sts = P4_SDE_MAP_ADD(&tbl_state->entry_info_htbl, key,
				 (void *)entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in inserting entry info to entry htbl");
		status = BF_NO_SYS_RESOURCES;
		goto cleanup_map_add;
	}
	key = (u64)mbr_id;
	/* Insert into the entry_handle-entry map */
	map_sts = P4_SDE_MAP_ADD(&tbl_state->mbr_id_htbl, key,
				 (void *)entry);
	if (map_sts != BF_MAP_OK) {
		key = (u64)new_ent_hdl;
		P4_SDE_MAP_RMV(&tbl_state->entry_info_htbl, key);
		LOG_ERROR("Error in inserting entry info to mbrid htbl");
		status = BF_NO_SYS_RESOURCES;
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
	if (resources)
		P4_SDE_FREE(resources);

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

int pipe_mgr_adt_member_reference_add_delete(struct bf_dev_target_t dev_tgt,
		u32 adt_tbl_hdl,
                u32 adt_ent_hdl,
                int op)
{
	struct pipe_mgr_adt_entry_info *entry = NULL;
	struct pipe_mgr_mat *adt_tbl;
	struct pipe_mgr_mat_state *tbl_state;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	int status;
	u64 key;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_ctx_get_table(dev_tgt, adt_tbl_hdl,
					PIPE_MGR_TABLE_TYPE_ADT, (void *)&adt_tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
			  adt_tbl_hdl);
		return BF_UNEXPECTED;
	}
	tbl_state = adt_tbl->state;

	key = (u64) adt_ent_hdl;

	map_sts = P4_SDE_MAP_GET(&tbl_state->entry_info_htbl, key,
			(void **) &entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in getting entry info");
		return BF_NO_SYS_RESOURCES;
	}

	if(op)
		entry->reference_count--;
	else
		entry->reference_count++;
	LOG_TRACE("Exiting %s", __func__);
	return BF_SUCCESS;
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

	status = pipe_mgr_ctx_get_table(dev_tgt, tbl_hdl,
					PIPE_MGR_TABLE_TYPE_ADT, (void *)&tbl);
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

	status = pipe_mgr_ctx_get_table(dev_tgt, adt_tbl_hdl,
					PIPE_MGR_TABLE_TYPE_ADT, (void *)&tbl);
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

	if (entry->reference_count) {
		LOG_ERROR("member id is referenced in selector group, delete"
			" selector group to delete this member");
		status = BF_UNEXPECTED;
		goto cleanup_tbl_unlock;
	}

	status = dal_table_adt_ent_del(sess_hdl, dev_tgt, adt_tbl_hdl,
				   pipe_api_flags, &tbl->ctx, adt_ent_hdl,
				   &(entry->dal_data));
	if (status == BF_SUCCESS) {
		P4_SDE_ID_FREE(tbl->state->entry_handle_array, key);
		P4_SDE_MAP_RMV(&tbl_state->entry_info_htbl, key);
		key = (u64) entry->mbr_id;
		P4_SDE_MAP_RMV(&tbl_state->mbr_id_htbl, key);
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

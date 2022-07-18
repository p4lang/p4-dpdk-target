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

/* P4 SDE Headers */
#include <osdep/p4_sde_osdep.h>
#include <pipe_mgr/shared/pipe_mgr_infra.h>
#include <dvm/bf_drv_intf.h>

/* Local module headers */
#include "../../core/pipe_mgr_log.h"
#include "pipe_mgr/shared/pipe_mgr_infra.h"
#include "pipe_mgr/shared/pipe_mgr_mat.h"
#include "../dal/dal_init.h"
#include "../pipe_mgr_shared_intf.h"

#define GET_TBL_INFO_FOR_DEL(tbl_struct, tbl)                                   \
	do {                                                                    \
		key_htbl = ((tbl_struct *)tbl)->state->key_htbl[pipe_idx];      \
		match_type = ((tbl_struct *)tbl)->ctx.match_attr.match_type;    \
		ent_hdl_arr = ((tbl_struct *)tbl)->state->entry_handle_array;   \
		ent_info_htbl = &((tbl_struct *)tbl)->state->entry_info_htbl;   \
	} while (0)                                                             \

#define GET_TBL_INFO_FOR_KEY_EXIST(tbl_struct, tbl)                             \
	do {                                                                    \
		key_htbl = ((tbl_struct *)tbl)->state->key_htbl[pipe_idx];      \
		match_type = ((tbl_struct *)tbl)->ctx.match_attr.match_type;    \
		dup_ent_chk = ((tbl_struct *)tbl)->ctx.duplicate_entry_check;   \
		ent_info_htbl = &((tbl_struct *)tbl)->state->entry_info_htbl;   \
	} while (0)                                                             \

#define GET_TBL_INFO_FOR_INSERT(tbl_struct, tbl)                                \
	do {                                                                    \
		ent_hdl_arr = ((tbl_struct *)tbl)->state->entry_handle_array;   \
		ent_info_htbl = &((tbl_struct *)tbl)->state->entry_info_htbl;   \
	} while (0)                                                             \

#define GET_TBL_INFO_FOR_INSERT_INTERNAL(tbl_struct, tbl)                       \
	do {                                                                    \
		handle = ((tbl_struct *)tbl)->ctx.handle;                       \
		key_htbl = &((tbl_struct *)tbl)->state->key_htbl[pipe_idx];     \
		match_type = ((tbl_struct *)tbl)->ctx.match_attr.match_type;    \
	} while (0)                                                             \

#define GET_TBL_INFO_FOR_GET(tbl_struct, tbl)                                   \
	do {                                                                    \
		ent_info_htbl = &((tbl_struct *)tbl)->state->entry_info_htbl;   \
	} while (0)                                                             \

#define GET_TBL_HDL_LOCK(tbl_struct, tbl)                                       \
	do {                                                                    \
		handle = ((tbl_struct *)tbl)->ctx.handle;                       \
		lock = &((tbl_struct *)tbl)->state->lock;                       \
	} while (0)                                                             \

#define GET_TBL_DATA_INFO_FOR_INSERT(data_struct, data)                         \
	do {                                                                    \
		match_spec = (struct pipe_tbl_match_spec *)                     \
		              ((data_struct *)entry)->match_spec;               \
		entry_hdl = &((data_struct *)entry)->mat_ent_hdl;               \
	} while (0)                                                             \

static int pipe_mgr_form_htbl_key(
		uint32_t key_sz,
		const struct pipe_tbl_match_spec *match_spec,
		uint8_t **key_p)
{
	uint32_t spec_size;
	(*key_p) = (uint8_t *)P4_SDE_CALLOC(key_sz, sizeof(uint8_t));
	if (NULL == (*key_p)) {
		LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
		return BF_NO_SYS_RESOURCES;
	}

	spec_size = match_spec->num_match_bytes;

	memcpy(*key_p,
		&(match_spec->match_field_validity),
		sizeof(match_spec->match_field_validity));

	if (match_spec->num_match_bytes) {
		memcpy(*key_p, match_spec->match_value_bits,
				match_spec->num_match_bytes);
	}

	/* key_sz == spec_size then it has both match key and key mask */
	if (key_sz == 2 * spec_size + sizeof(match_spec->priority)) {
		memcpy(&((*key_p)[match_spec->num_match_bytes]),
			match_spec->match_mask_bits,
			match_spec->num_match_bytes);
		memcpy(&((*key_p)[2 * spec_size]),
				&(match_spec->priority),
				sizeof(match_spec->priority));
	}

	return BF_SUCCESS;
}

static int pipe_mgr_cmp_match_value_bits(
    const void *key1,
    const struct pipe_tbl_match_spec *match_spec2,
    bool is_tern)
{
	uint8_t *key2 = NULL;
	uint32_t key_sz;
	int ret = 0;

	if (!key1) {
		return -1;
	}

	key_sz = match_spec2->num_match_bytes;
	if (is_tern) {
		key_sz *= 2;
		key_sz += sizeof(match_spec2->priority);
	}

	if ((pipe_mgr_form_htbl_key(key_sz, match_spec2, &key2) ==
				BF_NO_SYS_RESOURCES) ||
			!key2) {
		ret = -1;
		goto error;
	}

	ret = memcmp((void *)key1, (void *)key2, key_sz);
	if (ret != 0) {
		ret = -1;
		goto error;
	}
error:
	if (key2) {
		P4_SDE_FREE(key2);
	}
	return ret;
}

/* This is the compare function which is invoked by the hashtbl library to
 * compare the key found at a certain hash location. The first argument to the
 * function is an argument that is passed in when initiating the search and
 * the
 * the second argument is the hash table node.
 */
static int pipe_mgr_mat_exm_key_cmp_fn(const void *arg, const void *key1)
{
	void *key2;
	struct pipe_tbl_match_spec *match_spec2;
	if (key1 == NULL || arg == NULL) {
		return -1;
	}

	key2 = bf_hashtbl_get_cmp_data(key1);
	if (key1 == NULL || key2 == NULL) {
		return -1;
	}
	match_spec2 =
		(((struct pipe_mgr_mat_key_htbl_node *)key2)->match_spec);

	return pipe_mgr_cmp_match_value_bits(arg, match_spec2, false);
}

static int pipe_mgr_mat_tern_key_cmp_fn(const void *arg, const void *key1)
{
	void *key2;
	struct pipe_tbl_match_spec *match_spec2;
	if (key1 == NULL || arg == NULL) {
		return -1;
	}

	key2 = bf_hashtbl_get_cmp_data(key1);
	if (key1 == NULL || key2 == NULL) {
		return -1;
	}
	match_spec2 =
		(((struct pipe_mgr_mat_key_htbl_node *)key2)->match_spec);

	return pipe_mgr_cmp_match_value_bits(arg, match_spec2, true);
}

void pipe_mgr_free_key_htbl_node(void *node)
{
	struct pipe_mgr_mat_key_htbl_node *htbl_node = node;
	if (htbl_node == NULL) {
		return;
	}
	P4_SDE_FREE(htbl_node);
	return;
}

/* TODO: TBD stardard match value bits */
#define MAX_RAM_WORDS_IN_EXM_TBL_WORD 8
#define BYTES_IN_RAM_WORD 16
static int pipe_mgr_table_key_insert_internal(void *tbl,
					      enum pipe_mgr_table_type tbl_type,
					      struct pipe_tbl_match_spec *ms,
					      u32 mat_ent_hdl,
					      bf_dev_pipe_t pipe_id)
{
	struct pipe_mgr_mat_key_htbl_node *htbl_node = NULL;
	struct pipe_tbl_match_spec *match_spec;
	bf_hashtbl_sts_t htbl_sts = BF_HASHTBL_OK;
	enum pipe_mgr_match_type match_type;
	bf_hashtable_t **key_htbl;
	uint8_t *key_p = NULL;
	uint8_t pipe_idx = 0;
	int ret = BF_SUCCESS;
	uint32_t key_sz = 0;
	int handle;

	/* TBD: do we need to support BF_DEV_PIPE_ALL */
	/*if (pipe_id == BF_DEV_PIPE_ALL) {
		pipe_idx = 0;
	} else {
		pipe_idx = pipe_id;
	} */

	pipe_idx = pipe_id;
	match_spec = ms;
	/*TODO: support for dynamic key mask */

	switch (tbl_type) {
		case PIPE_MGR_TABLE_TYPE_MAT:
			GET_TBL_INFO_FOR_INSERT_INTERNAL(struct pipe_mgr_mat, tbl);
			break;
		case PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP:
			GET_TBL_INFO_FOR_INSERT_INTERNAL(struct pipe_mgr_value_lookup, tbl);
			break;
		default:
			LOG_ERROR("Invalid table type, table type %d does not exist.", tbl_type);
			return BF_INVALID_ARG;
	}

	if (!*key_htbl) {
		/* Hash table is not initialized, may be
		 * the first entry that is getting added.
		 */
		*key_htbl = (bf_hashtable_t *)P4_SDE_CALLOC(1, sizeof(bf_hashtable_t));
		if (*key_htbl == NULL) {
			LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
			return BF_NO_SYS_RESOURCES;
		}

		key_sz = match_spec->num_match_bytes;
		if (match_type != PIPE_MGR_MATCH_TYPE_EXACT) {
			key_sz *= 2;
			key_sz += sizeof(match_spec->priority);
		}

		if (match_type == PIPE_MGR_MATCH_TYPE_EXACT) {
			htbl_sts = bf_hashtbl_init(*key_htbl,
						   pipe_mgr_mat_exm_key_cmp_fn,
						   pipe_mgr_free_key_htbl_node,
						   key_sz,
						   sizeof(struct pipe_mgr_mat_key_htbl_node),
						   0x98733423);
		} else {
			htbl_sts = bf_hashtbl_init(*key_htbl,
						   pipe_mgr_mat_tern_key_cmp_fn,
						   pipe_mgr_free_key_htbl_node,
						   key_sz,
						   sizeof(struct pipe_mgr_mat_key_htbl_node),
						   0x98733423);
		}

		if (htbl_sts != BF_HASHTBL_OK) {
			LOG_ERROR("%s:%d Error in initializing hashtable"
				  "for match table key table 0x%x",
				  __func__, __LINE__, handle);
			return BF_UNEXPECTED;
		}
	}

	htbl_node = (struct pipe_mgr_mat_key_htbl_node *)P4_SDE_CALLOC(1, sizeof(*htbl_node));
	if (htbl_node == NULL) {
		LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
		return BF_NO_SYS_RESOURCES;
	}
	htbl_node->match_spec = match_spec;
	htbl_node->mat_ent_hdl = mat_ent_hdl;
	key_sz = match_spec->num_match_bytes;
	if (match_type != PIPE_MGR_MATCH_TYPE_EXACT) {
		key_sz *= 2;
		key_sz += sizeof(match_spec->priority);
	}

	if (pipe_mgr_form_htbl_key(key_sz, match_spec, &key_p) == BF_NO_SYS_RESOURCES) {
		P4_SDE_FREE(htbl_node);
		return BF_NO_SYS_RESOURCES;
	}

	htbl_sts = bf_hashtbl_insert(*key_htbl, htbl_node, key_p);
	if (htbl_sts != BF_HASHTBL_OK) {
		LOG_ERROR("%s:%d Error in inserting match spec into the"
				"key hash tbl for tbl 0x%x",
				__func__, __LINE__, handle);
		ret =  BF_UNEXPECTED;
		P4_SDE_FREE(htbl_node);
		goto error;
	}

error:
	if (key_p) {
		P4_SDE_FREE(key_p);
	}
	return ret;
}

static int mat_tbl_key_exists(void *tbl,
			      enum pipe_mgr_table_type tbl_type,
			      struct pipe_tbl_match_spec *ms,
			      bf_dev_pipe_t pipe_id,
			      bool *exists,
			      u32 *mat_ent_hdl,
			      void **entry)
{
	struct pipe_mgr_mat_key_htbl_node *htbl_node = NULL;
	struct pipe_tbl_match_spec *match_spec;
	enum pipe_mgr_match_type match_type;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	p4_sde_map *ent_info_htbl;
	bf_hashtable_t *key_htbl;
	uint8_t *key_p = NULL;
	uint8_t pipe_idx = 0;
	bool dup_ent_chk;
	uint32_t key_sz;
	u64 key;

	/* TBD: do we need to support BF_DEV_PIPE_ALL */
	/*if (pipe_id == BF_DEV_PIPE_ALL)
		pipe_idx = 0;
	else
		pipe_idx = pipe_id; */

	pipe_idx = pipe_id;

	/* Get the handles and values for tables as per table type */
	switch (tbl_type) {
		case PIPE_MGR_TABLE_TYPE_MAT:
			GET_TBL_INFO_FOR_KEY_EXIST(struct pipe_mgr_mat, tbl);
			break;
		case PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP:
			GET_TBL_INFO_FOR_KEY_EXIST(struct pipe_mgr_value_lookup, tbl);
			break;
		default:
			LOG_ERROR("Invalid table type, table type %d does not exist.", tbl_type);
			return BF_INVALID_ARG;
	}

	/* The very first entry getting added into this table
	 * or duplicate_entry_check is disabled for this table */
	if (!dup_ent_chk ||
	    !key_htbl) {
		*exists = false;
		return BF_SUCCESS;
	}

	match_spec = ms;

	key_sz = match_spec->num_match_bytes;
	if (match_type != PIPE_MGR_MATCH_TYPE_EXACT) {
		key_sz *= 2;
		key_sz += sizeof(match_spec->priority);
	}

	if (pipe_mgr_form_htbl_key(key_sz, match_spec, &key_p) ==
			BF_NO_SYS_RESOURCES) {
		return BF_NO_SYS_RESOURCES;
	}

	htbl_node = bf_hashtbl_search(key_htbl, key_p);

	if (htbl_node == NULL) {
		*exists = false;
		if (key_p)
			P4_SDE_FREE(key_p);
		return BF_SUCCESS;
	} else {
		*mat_ent_hdl = htbl_node->mat_ent_hdl;
		*exists = true;
	}
	if (key_p) {
		P4_SDE_FREE(key_p);
	}

	if (!entry)
		return BF_SUCCESS;

	key = (unsigned long)*mat_ent_hdl;

	map_sts = P4_SDE_MAP_GET(ent_info_htbl, key,
				 (void **)entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("table entry handle/entry map get failed");
		return BF_UNEXPECTED;
	}
	return BF_SUCCESS;
}

int pipe_mgr_table_key_exists(void *tbl,
			      enum pipe_mgr_table_type tbl_type,
			      struct pipe_tbl_match_spec *ms,
			      bf_dev_pipe_t pipe_id,
			      bool *exists,
			      u32 *ent_hdl,
			      void **entry)
{
	int status = BF_SUCCESS;
	p4_sde_mutex *lock;
	int handle;

	switch (tbl_type) {
		case PIPE_MGR_TABLE_TYPE_MAT:
			GET_TBL_HDL_LOCK(struct pipe_mgr_mat, tbl);
			break;
		case PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP:
			GET_TBL_HDL_LOCK(struct pipe_mgr_value_lookup, tbl);
			break;
		default:
			LOG_ERROR("Invalid table type, table type %d does not exist.", tbl_type);
			return BF_INVALID_ARG;
	}

	status = P4_SDE_MUTEX_LOCK(lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d", handle, status);
		return BF_UNEXPECTED;
	}

	status = mat_tbl_key_exists((void *)tbl, tbl_type,
				    ms, pipe_id, exists,
				    ent_hdl, (void **)entry);

	if (P4_SDE_MUTEX_UNLOCK(lock))
		LOG_ERROR("Unlock of table %d failed", handle);

	return status;
}

int pipe_mgr_table_key_delete_internal(struct bf_dev_target_t dev_tgt,
				       void *tbl,
				       enum pipe_mgr_table_type tbl_type,
				       struct pipe_tbl_match_spec *match_spec)
{
	struct pipe_mgr_mat_key_htbl_node *htbl_node = NULL;
	enum pipe_mgr_match_type match_type;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	p4_sde_map *ent_info_htbl;
	bf_hashtable_t *key_htbl;
	int status = BF_SUCCESS;
	p4_sde_id *ent_hdl_arr;
	uint8_t *key_p = NULL;
	uint8_t pipe_idx = 0;
	uint32_t key_sz = 0;
	p4_sde_mutex *lock;
	u32 mat_ent_hdl;
	bool exists;
	int handle;
	u64 key;

	pipe_idx = dev_tgt.dev_pipe_id;

	/* Get the handles and values for tables as per table type */
	switch (tbl_type) {
		case PIPE_MGR_TABLE_TYPE_MAT:
			GET_TBL_INFO_FOR_DEL(struct pipe_mgr_mat, tbl);
			GET_TBL_HDL_LOCK(struct pipe_mgr_mat, tbl);
			break;
		case PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP:
			GET_TBL_INFO_FOR_DEL(struct pipe_mgr_value_lookup, tbl);
			GET_TBL_HDL_LOCK(struct pipe_mgr_value_lookup, tbl);
			break;
		default:
			LOG_ERROR("Invalid table type, table type %d does not exist.", tbl_type);
			return BF_INVALID_ARG;
	}

	key_sz = match_spec->num_match_bytes;
	if (match_type != PIPE_MGR_MATCH_TYPE_EXACT) {
		key_sz *= 2;
		key_sz += sizeof(match_spec->priority);
	}

	if (pipe_mgr_form_htbl_key(key_sz, match_spec, &key_p) ==
			BF_NO_SYS_RESOURCES) {
		return BF_NO_SYS_RESOURCES;
	}

	status = P4_SDE_MUTEX_LOCK(lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d", handle, status);
		if (key_p)
			P4_SDE_FREE(key_p);

		return BF_UNEXPECTED;
	}

	status = mat_tbl_key_exists(tbl, tbl_type, match_spec, dev_tgt.dev_pipe_id,
				    &exists, &mat_ent_hdl, NULL);
	if (status) {
		LOG_ERROR("Failure in searching key in table");
		status = BF_UNEXPECTED;
		goto cleanup_key;
	}

	if (!exists)
		goto cleanup_key;

	key = (u64)mat_ent_hdl;
	map_sts = P4_SDE_MAP_RMV(ent_info_htbl, key);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("table entry handle/entry map del failed");
		status = BF_UNEXPECTED;
	}

	P4_SDE_ID_FREE(ent_hdl_arr, mat_ent_hdl);

	htbl_node = bf_hashtbl_get_remove(key_htbl, key_p);
	if (htbl_node == NULL) {
		LOG_ERROR("Not found in match spec based hash table");
		status = (status == BF_SUCCESS) ? BF_OBJECT_NOT_FOUND :
			 status;
	}

cleanup_key:
	if (key_p)
		P4_SDE_FREE(key_p);

	if (P4_SDE_MUTEX_UNLOCK(lock))
		LOG_ERROR("Unlock of table %d failed", handle);

	return status;
}

int pipe_mgr_table_key_delete(struct bf_dev_target_t dev_tgt,
			      void *tbl,
			      enum pipe_mgr_table_type tbl_type,
			      struct pipe_tbl_match_spec *match_spec)
{
	return pipe_mgr_table_key_delete_internal(dev_tgt, tbl,
						  tbl_type, match_spec);
}

int pipe_mgr_table_key_insert(struct bf_dev_target_t dev_tgt,
			      void *tbl,
			      enum pipe_mgr_table_type tbl_type,
			      void *entry,
			      u32 *ent_hdl)
{
	struct pipe_tbl_match_spec *match_spec;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	p4_sde_map *ent_info_htbl;
	p4_sde_id *ent_hdl_arr;
	p4_sde_mutex *lock;
	u32 new_ent_hdl;
	uint32_t *entry_hdl;
	int handle;
	int status;
	u64 key;

	switch (tbl_type) {
		case PIPE_MGR_TABLE_TYPE_MAT:
			GET_TBL_INFO_FOR_INSERT(struct pipe_mgr_mat, tbl);
			GET_TBL_DATA_INFO_FOR_INSERT(struct pipe_mgr_mat_entry_info, entry);
			GET_TBL_HDL_LOCK(struct pipe_mgr_mat, tbl);
			break;
		case PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP:
			GET_TBL_INFO_FOR_INSERT(struct pipe_mgr_value_lookup, tbl);
			GET_TBL_DATA_INFO_FOR_INSERT(struct pipe_mgr_value_lookup_entry_info, entry);
			GET_TBL_HDL_LOCK(struct pipe_mgr_value_lookup, tbl);
			break;
		default:
			LOG_ERROR("Invalid table type, table type %d does not exist.", tbl_type);
			return BF_INVALID_ARG;
	}

	status = P4_SDE_MUTEX_LOCK(lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d", handle, status);
		return BF_UNEXPECTED;
	}

	new_ent_hdl = P4_SDE_ID_ALLOC(ent_hdl_arr);
	if (new_ent_hdl == ENTRY_HANDLE_ARRAY_SIZE) {
		LOG_ERROR("entry handle allocator failed");
		status = BF_NO_SPACE;
		goto cleanup;
	}
	*entry_hdl = new_ent_hdl;

	key = (u64)new_ent_hdl;
	/* Insert into the entry_handle-entry map */
	map_sts = P4_SDE_MAP_ADD(ent_info_htbl, key, (void *)entry);
	if (map_sts != BF_MAP_OK) {
		LOG_ERROR("Error in inserting entry info");
		status = BF_NO_SYS_RESOURCES;
		goto cleanup_id;
	}

	status = pipe_mgr_table_key_insert_internal(tbl, tbl_type,
						    match_spec,
						    new_ent_hdl,
						    dev_tgt.dev_pipe_id);
	if (status) {
		LOG_ERROR("Error in inserting in hash table");
		goto cleanup_map_add;
	}

	*ent_hdl = new_ent_hdl;
	if (P4_SDE_MUTEX_UNLOCK(lock)) {
		LOG_ERROR("Unlock of table %d failed", handle);
		status = BF_UNEXPECTED;
	}

	return status;

cleanup_map_add:
	P4_SDE_MAP_RMV(ent_info_htbl, key);

cleanup_id:
	P4_SDE_ID_FREE(ent_hdl_arr, new_ent_hdl);

cleanup:
	P4_SDE_MUTEX_UNLOCK(lock);
	return status;
}

int pipe_mgr_table_get_first(void *tbl,
			     enum pipe_mgr_table_type tbl_type,
			     bf_dev_pipe_t pipe_id,
			     u32 *ent_hdl)
{
	void **entry = NULL;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	p4_sde_map *ent_info_htbl;
	p4_sde_mutex *lock;
	unsigned long key;
	int handle;
	int status;

	if (!ent_hdl)
		return BF_INVALID_ARG;

	switch (tbl_type) {
		case PIPE_MGR_TABLE_TYPE_MAT:
			GET_TBL_INFO_FOR_GET(struct pipe_mgr_mat, tbl);
			GET_TBL_HDL_LOCK(struct pipe_mgr_mat, tbl);
			break;
		case PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP:
			GET_TBL_INFO_FOR_GET(struct pipe_mgr_value_lookup, tbl);
			GET_TBL_HDL_LOCK(struct pipe_mgr_value_lookup, tbl);
			break;
		default:
			LOG_ERROR("Invalid table type, table type %d does not exist.", tbl_type);
			return BF_INVALID_ARG;
	}

	status = P4_SDE_MUTEX_LOCK(lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d", handle, status);
		return BF_UNEXPECTED;
	}
	status = BF_SUCCESS;

	map_sts = P4_SDE_MAP_GET_FIRST(ent_info_htbl, &key, (void **)&entry);
	if (map_sts == BF_MAP_NO_KEY) {
		key = -1;
		status = BF_OBJECT_NOT_FOUND;
	} else if (map_sts) {
		LOG_ERROR("Retrieving handle for table %d failed", handle);
		status = BF_UNEXPECTED;
	}

	*ent_hdl = key;

	if (P4_SDE_MUTEX_UNLOCK(lock))
		LOG_ERROR("Unlock of table %d failed", handle);

	return status;
}

int pipe_mgr_table_get_next_n(void *tbl,
			      enum pipe_mgr_table_type tbl_type,
			      bf_dev_pipe_t pipe_id,
			      u32 ent_hdl,
			      int n,
			      u32 *next_ent_hdls)
{
	void **entry = NULL;
	p4_sde_map_sts map_sts = BF_MAP_OK;
	p4_sde_map *ent_info_htbl;
	p4_sde_mutex *lock;
	unsigned long key;
	int handle;
	int status;
	int i;

	if (!next_ent_hdls)
		return BF_INVALID_ARG;

	switch (tbl_type) {
		case PIPE_MGR_TABLE_TYPE_MAT:
			GET_TBL_INFO_FOR_GET(struct pipe_mgr_mat, tbl);
			GET_TBL_HDL_LOCK(struct pipe_mgr_mat, tbl);
			break;
		case PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP:
			GET_TBL_INFO_FOR_GET(struct pipe_mgr_value_lookup, tbl);
			GET_TBL_HDL_LOCK(struct pipe_mgr_value_lookup, tbl);
			break;
		default:
			LOG_ERROR("Invalid table type, table type %d does not exist.", tbl_type);
			return BF_INVALID_ARG;
	}

	status = P4_SDE_MUTEX_LOCK(lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d", handle, status);
		return BF_UNEXPECTED;
	}

	key = ent_hdl;
	i = 0;
	while (i < n) {
		map_sts = P4_SDE_MAP_GET_NEXT(ent_info_htbl, &key, (void **)&entry);
		if (map_sts != BF_MAP_OK) {
			next_ent_hdls[i] = -1;
			break;
		}

		next_ent_hdls[i] = key;
		i++;
	}

	if (i < n)
		next_ent_hdls[i] = -1;

	if (map_sts == BF_MAP_OK)
		status = BF_SUCCESS;
	else if (map_sts == BF_MAP_NO_KEY)
		status = (i == 0) ? BF_OBJECT_NOT_FOUND : BF_SUCCESS;
	else
		status = BF_UNEXPECTED;

	if (P4_SDE_MUTEX_UNLOCK(lock))
		LOG_ERROR("Unlock of table %d failed", handle);

	return status;
}

int pipe_mgr_table_get(void *tbl,
		       enum pipe_mgr_table_type tbl_type,
		       bf_dev_pipe_t pipe_id,
		       u32 ent_hdl,
		       void **entry)
{
	p4_sde_map_sts map_sts = BF_MAP_OK;
	p4_sde_map *ent_info_htbl;
	p4_sde_mutex *lock;
	unsigned long key;
	int handle;
	int status;

	if (!entry)
		return BF_INVALID_ARG;

	switch (tbl_type) {
		case PIPE_MGR_TABLE_TYPE_MAT:
			GET_TBL_INFO_FOR_GET(struct pipe_mgr_mat, tbl);
			GET_TBL_HDL_LOCK(struct pipe_mgr_mat, tbl);
			break;
		case PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP:
			GET_TBL_INFO_FOR_GET(struct pipe_mgr_value_lookup, tbl);
			GET_TBL_HDL_LOCK(struct pipe_mgr_value_lookup, tbl);
			break;
		default:
			LOG_ERROR("Invalid table type, table type %d does not exist.", tbl_type);
			return BF_INVALID_ARG;
	}

	status = P4_SDE_MUTEX_LOCK(lock);
	if (status) {
		LOG_ERROR("Acquiring lock for table %d failed with err: %d", handle, status);
		return BF_UNEXPECTED;
	}

	key = ent_hdl;

	map_sts = P4_SDE_MAP_GET(ent_info_htbl, key, (void **)entry);
	if (map_sts == BF_MAP_OK)
		status = BF_SUCCESS;
	else if (map_sts == BF_MAP_NO_KEY)
		status = BF_OBJECT_NOT_FOUND;
	else
		status = BF_UNEXPECTED;

	if (P4_SDE_MUTEX_UNLOCK(lock))
		LOG_ERROR("Unlock of table %d failed", handle);

	return status;
}

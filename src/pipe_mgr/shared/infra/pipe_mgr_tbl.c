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
#include "pipe_mgr_int.h"
#include "pipe_mgr/shared/pipe_mgr_infra.h"
#include "pipe_mgr/shared/pipe_mgr_mat.h"
#include "../dal/dal_init.h"
#include "../pipe_mgr_shared_intf.h"

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
static int pipe_mgr_mat_tbl_key_insert_internal(
		struct pipe_mgr_mat *tbl,
		struct pipe_tbl_match_spec *ms,
		u32 mat_ent_hdl,
		bf_dev_pipe_t pipe_id)
{
	struct pipe_tbl_match_spec *match_spec;
	struct pipe_mgr_mat_key_htbl_node *htbl_node = NULL;
	bf_hashtbl_sts_t htbl_sts = BF_HASHTBL_OK;
	struct pipe_mgr_mat_state *tbl_state;
	struct pipe_mgr_mat_ctx *tbl_ctx;
	uint8_t *key_p = NULL;
	uint8_t pipe_idx = 0;
	uint32_t key_sz = 0;
	int ret = BF_SUCCESS;

	/* TBD: do we need to support BF_DEV_PIPE_ALL */
	/*if (pipe_id == BF_DEV_PIPE_ALL) {
		pipe_idx = 0;
	} else {
		pipe_idx = pipe_id;
	} */

	tbl_ctx = &tbl->ctx;
	tbl_state = tbl->state;
	match_spec = ms;
	/*TODO: support for dynamic key mask */

	if (!tbl_state->key_htbl[pipe_idx]) {
		/* Hash table is not initialized, may be
		 * the first entry that is getting added.
		 */
		tbl_state->key_htbl[pipe_idx] =
			(bf_hashtable_t *)P4_SDE_CALLOC(1,
					sizeof(bf_hashtable_t));

		key_sz = match_spec->num_match_bytes;
		if (tbl_ctx->match_attr.match_type !=
				PIPE_MGR_MATCH_TYPE_EXACT) {
			key_sz *= 2;
			key_sz += sizeof(match_spec->priority);
		}

		if (tbl_ctx->match_attr.match_type ==
				PIPE_MGR_MATCH_TYPE_EXACT) {
			htbl_sts = bf_hashtbl_init(
					tbl_state->key_htbl[pipe_idx],
					pipe_mgr_mat_exm_key_cmp_fn,
					pipe_mgr_free_key_htbl_node,
					key_sz,
					sizeof
					(struct pipe_mgr_mat_key_htbl_node),
					0x98733423);
		} else {
			htbl_sts = bf_hashtbl_init(
					tbl_state->key_htbl[pipe_idx],
					pipe_mgr_mat_tern_key_cmp_fn,
					pipe_mgr_free_key_htbl_node,
					key_sz,
					sizeof
					(struct pipe_mgr_mat_key_htbl_node),
					0x98733423);
		}

		if (htbl_sts != BF_HASHTBL_OK) {
			LOG_ERROR(
					"%s:%d Error in initializing hashtable"
					"for match table key"
					" table 0x%x",
					__func__,
					__LINE__,
					tbl_ctx->handle);
			return BF_UNEXPECTED;
		}
	}

	htbl_node = (struct pipe_mgr_mat_key_htbl_node *)P4_SDE_CALLOC(
			1, sizeof(*htbl_node));
	if (htbl_node == NULL) {
		LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
		return BF_NO_SYS_RESOURCES;
	}
	htbl_node->match_spec = match_spec;
	htbl_node->mat_ent_hdl = mat_ent_hdl;

	key_sz = match_spec->num_match_bytes;
	if (tbl_ctx->match_attr.match_type != PIPE_MGR_MATCH_TYPE_EXACT) {
		key_sz *= 2;
		key_sz += sizeof(match_spec->priority);
	}

	if (pipe_mgr_form_htbl_key(key_sz, match_spec, &key_p) ==
			BF_NO_SYS_RESOURCES) {
		return BF_NO_SYS_RESOURCES;
	}
	htbl_sts =
		bf_hashtbl_insert(tbl_state->key_htbl[pipe_idx],
				  htbl_node, key_p);

	if (htbl_sts != BF_HASHTBL_OK) {
		LOG_ERROR(
				"%s:%d Error in inserting match spec into the"
				"key hash tbl"
				" for tbl 0x%x",
				__func__,
				__LINE__,
				tbl_ctx->handle);
		ret =  BF_UNEXPECTED;
		goto error;
	}

error:
	if (key_p) {
		P4_SDE_FREE(key_p);
	}
	return ret;
}

int pipe_mgr_mat_tbl_key_exists(struct pipe_mgr_mat *tbl,
				struct pipe_tbl_match_spec *ms,
				bf_dev_pipe_t pipe_id,
				bool *exists,
				u32 *mat_ent_hdl)
{
	struct pipe_mgr_mat_key_htbl_node *htbl_node = NULL;
	struct pipe_tbl_match_spec *match_spec;
	uint8_t *key_p = NULL;
	uint8_t pipe_idx = 0;
	uint32_t key_sz;

	/* TBD: do we need to support BF_DEV_PIPE_ALL */
	/*if (pipe_id == BF_DEV_PIPE_ALL)
		pipe_idx = 0;
	else
		pipe_idx = pipe_id; */

	/* The very first entry getting added into this table
	 * or duplicate_entry_check is disabled for this table */
	if (!tbl->ctx.duplicate_entry_check ||
	    !tbl->state->key_htbl) {
		*exists = false;
		return BF_SUCCESS;
	}

	match_spec = ms;

/*if ((pipe_mgr_get_exm_key_with_dkm_mask(tbl_state, ms, dkm_match_spec)) ==
      PIPE_SUCCESS) {
    // EXM table has dynamic key mask property. Change key to include bits as
    // per key-mask.
    match_spec = dkm_match_spec;
  } */

	key_sz = match_spec->num_match_bytes;
	if (tbl->ctx.match_attr.match_type != PIPE_MGR_MATCH_TYPE_EXACT) {
		key_sz *= 2;
		key_sz += sizeof(match_spec->priority);
	}

	if (pipe_mgr_form_htbl_key(key_sz, match_spec, &key_p) ==
			BF_NO_SYS_RESOURCES) {
		return BF_NO_SYS_RESOURCES;
	}

	htbl_node = bf_hashtbl_search(tbl->state->key_htbl[pipe_idx], key_p);

	if (htbl_node == NULL) {
		*exists = false;
	} else {
		*mat_ent_hdl = htbl_node->mat_ent_hdl;
		*exists = true;
	}
	if (key_p) {
		P4_SDE_FREE(key_p);
	}

	return BF_SUCCESS;
}

int pipe_mgr_mat_tbl_key_delete_internal(struct bf_dev_target_t dev_tgt,
		struct pipe_mgr_mat *tbl,
		struct pipe_tbl_match_spec *match_spec,
		u32 mat_ent_hdl)
{
	struct pipe_mgr_mat_key_htbl_node *htbl_node = NULL;
	int status = BF_SUCCESS;
	uint8_t *key_p = NULL;
	uint8_t pipe_idx = 0;
	uint32_t key_sz = 0;

	key_sz = match_spec->num_match_bytes;
	if (tbl->ctx.match_attr.match_type != PIPE_MGR_MATCH_TYPE_EXACT) {
		key_sz *= 2;
		key_sz += sizeof(match_spec->priority);
	}

	if (pipe_mgr_form_htbl_key(key_sz, match_spec, &key_p) ==
			BF_NO_SYS_RESOURCES) {
		return BF_NO_SYS_RESOURCES;
	}

	htbl_node = bf_hashtbl_get_remove(tbl->state->key_htbl[pipe_idx],
					  key_p);
	if (htbl_node == NULL) {
		LOG_ERROR("Not found in match spec based hash table");
		status = BF_OBJECT_NOT_FOUND;
	}

	if (key_p) {
		P4_SDE_FREE(key_p);
	}

	return status;
}

int pipe_mgr_mat_tbl_key_delete(struct bf_dev_target_t dev_tgt,
		struct pipe_mgr_mat *tbl,
		struct pipe_tbl_match_spec *match_spec,
		u32 mat_ent_hdl)
{
	return pipe_mgr_mat_tbl_key_delete_internal(
			dev_tgt, tbl,
			match_spec, mat_ent_hdl);
}

int pipe_mgr_mat_tbl_key_insert(struct bf_dev_target_t dev_tgt,
		struct pipe_mgr_mat *tbl,
		struct pipe_tbl_match_spec *match_spec,
		u32 mat_ent_hdl)
{
	uint32_t num_pipelines = 0;
	int status;

	/* If the hash tables are not allocated, allocate it */
	if (!tbl->state->key_htbl) {
		num_pipelines =
			pipe_mgr_get_num_active_pipes(dev_tgt.device_id);
		tbl->state->key_htbl = (bf_hashtable_t **)P4_SDE_CALLOC
				       (num_pipelines,
					sizeof(bf_hashtable_t *));

		if (!tbl->state->key_htbl) {
			LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
			return BF_NO_SYS_RESOURCES;
		}
	}

	status = pipe_mgr_mat_tbl_key_insert_internal
			(tbl, match_spec, mat_ent_hdl,
			 dev_tgt.dev_pipe_id);

	return status;
}

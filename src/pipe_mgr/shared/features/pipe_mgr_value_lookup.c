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
 * @file pipe_mgr_value_lookup.c
 *
 * @Description Definitions for interfaces to value lookup table.
 */

#include <osdep/p4_sde_osdep_utils.h>

#include "pipe_mgr/shared/pipe_mgr_value_lookup.h"
#include "../infra/pipe_mgr_ctx_util.h"
#include "../dal/dal_value_lookup.h"
#include "../pipe_mgr_shared_intf.h"
#include "../../core/pipe_mgr_log.h"
#include "../infra/pipe_mgr_tbl.h"

/*
 * Method to pack the value lookup table match spec
 */
static int pipe_mgr_value_lookup_pack_match_spec(struct pipe_tbl_match_spec *match_spec,
						 struct pipe_tbl_match_spec **match_spec_out)
{
	struct pipe_tbl_match_spec *ms;

	ms = P4_SDE_CALLOC(1, sizeof(*ms));
	if (!ms)
		return BF_NO_SYS_RESOURCES;

	ms->num_valid_match_bits = match_spec->num_valid_match_bits;
	ms->num_match_bytes = match_spec->num_match_bytes;
	ms->priority = match_spec->priority;
	ms->match_value_bits = P4_SDE_CALLOC(match_spec->num_match_bytes, sizeof(uint8_t));
	if (!ms->match_value_bits)
		goto cleanup_match_value;
	memcpy(ms->match_value_bits, match_spec->match_value_bits, match_spec->num_match_bytes);
	ms->match_mask_bits = P4_SDE_CALLOC(match_spec->num_match_bytes, sizeof(uint8_t));
	if (!ms->match_mask_bits)
		goto cleanup_match_mask;
	memcpy(ms->match_mask_bits, match_spec->match_mask_bits, match_spec->num_match_bytes);

	*match_spec_out = ms;
	return BF_SUCCESS;

cleanup_match_mask:
	P4_SDE_FREE(ms->match_value_bits);
cleanup_match_value:
	P4_SDE_FREE(ms);
	return BF_NO_SYS_RESOURCES;
}

/*
 * Method to unpack the value lookup table match spec
 */
static int pipe_mgr_value_lookup_unpack_match_spec(struct pipe_tbl_match_spec *match_spec,
						   struct pipe_tbl_match_spec *match_spec_out)
{
	match_spec_out->num_valid_match_bits = match_spec->num_valid_match_bits;
	match_spec_out->num_match_bytes = match_spec->num_match_bytes;
	match_spec_out->priority = match_spec->priority;
	memcpy(match_spec_out->match_value_bits, match_spec->match_value_bits, match_spec->num_match_bytes);
	memcpy(match_spec_out->match_mask_bits, match_spec->match_mask_bits, match_spec->num_match_bytes);

	return BF_SUCCESS;
}

/*
 * Method to delete the value lookup table match spec
 */
static void pipe_mgr_value_lookup_del_match_spec(struct pipe_tbl_match_spec *match_spec)
{
        P4_SDE_FREE(match_spec->match_mask_bits);
        P4_SDE_FREE(match_spec->match_value_bits);
        P4_SDE_FREE(match_spec);
}

/*
 * Method to pack the value lookup table data spec
 */
static int pipe_mgr_value_lookup_pack_data_spec(struct pipe_data_spec *data_spec,
						struct pipe_data_spec **data_spec_out)
{
	struct pipe_data_spec *ds;

	ds = P4_SDE_CALLOC(1, sizeof(*ds));
	if (!ds)
		return BF_NO_SYS_RESOURCES;

	ds->num_data_bytes = data_spec->num_data_bytes;
	ds->data_bytes = P4_SDE_CALLOC(data_spec->num_data_bytes, sizeof(uint8_t));
	if (!ds->data_bytes)
		goto cleanup;
	memcpy(ds->data_bytes, data_spec->data_bytes, data_spec->num_data_bytes);

	*data_spec_out = ds;

	return BF_SUCCESS;
cleanup:
	P4_SDE_FREE(ds);
	return BF_NO_SYS_RESOURCES;
}

/*
 * Method to unpack the value lookup table data spec
 */
static int pipe_mgr_value_lookup_unpack_data_spec(struct pipe_data_spec *data_spec,
						  struct pipe_data_spec *data_spec_out)
{
	data_spec_out->num_data_bytes = data_spec->num_data_bytes;
	memcpy(data_spec_out->data_bytes, data_spec->data_bytes, data_spec->num_data_bytes);

	return BF_SUCCESS;
}

/*
 * Method to delete the value lookup table data spec
 */
static void pipe_mgr_value_lookup_del_data_spec(struct pipe_data_spec *data_spec)
{
	P4_SDE_FREE(data_spec->data_bytes);
	P4_SDE_FREE(data_spec);
}

/*
 * Method to pack the value lookup table entry info (match_spec and data_spec)
 */
static int pipe_mgr_value_lookup_pack_entry_info(struct bf_dev_target_t dev_tgt,
						 struct pipe_mgr_value_lookup *tbl,
						 struct pipe_tbl_match_spec *match_spec,
						 struct pipe_data_spec *data_spec,
						 struct pipe_mgr_value_lookup_entry_info **entry_out)
{
	struct pipe_mgr_value_lookup_entry_info *entry;
	int status;

	entry = P4_SDE_CALLOC(1, sizeof(*entry));
	if (!entry)
		return BF_NO_SYS_RESOURCES;

	status = pipe_mgr_value_lookup_pack_match_spec(match_spec, &(entry->match_spec));
        if (status)
                goto cleanup_entry;

	status = pipe_mgr_value_lookup_pack_data_spec(data_spec, &(entry->data_spec));
        if (status)
                goto cleanup_match_spec;

        *entry_out = entry;

        return BF_SUCCESS;

cleanup_match_spec:
	pipe_mgr_value_lookup_del_match_spec(entry->match_spec);
cleanup_entry:
        P4_SDE_FREE(entry);
        *entry_out = NULL;
	return status;
}

/*
 * Method to unpack the value lookup table entry info (match_spec and data_spec)
 */
static int pipe_mgr_value_lookup_unpack_entry_info(struct bf_dev_target_t dev_tgt,
						   struct pipe_mgr_value_lookup *tbl,
						   struct pipe_tbl_match_spec *match_spec,
						   struct pipe_data_spec *data_spec,
						   uint32_t ent_hdl,
						   struct pipe_mgr_value_lookup_entry_info *entry)
{
	int status;

	if (ent_hdl != entry->mat_ent_hdl) {
		LOG_ERROR("lookup table entry unpack error");
		return BF_UNEXPECTED;
	}

	status = pipe_mgr_value_lookup_unpack_match_spec(entry->match_spec, match_spec);
	if (status) {
		LOG_ERROR("lookup table match_spec unpack failed");
		return status;
	}

	status = pipe_mgr_value_lookup_unpack_data_spec(entry->data_spec, data_spec);
	if (status) {
		LOG_ERROR("lookup table data_spec unpack failed");
		return status;
	}

	return BF_SUCCESS;
}

/*
 * Method to delete the value lookup table entry info (match_spec and data_spec)
 */
static void pipe_mgr_value_lookup_del_entry(struct pipe_mgr_value_lookup_entry_info *entry)
{
	pipe_mgr_value_lookup_del_match_spec(entry->match_spec);
	pipe_mgr_value_lookup_del_data_spec(entry->data_spec);
	P4_SDE_FREE(entry);
}

/*
 * Method to add entries to value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param match_spec: key spec for table.
 * @param data_spec: data to be stored in table.
 * @param ent_hdl_p: entry handle ptr.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_ent_add(uint32_t sess_hdl,
				  struct bf_dev_target_t dev_tgt,
				  uint32_t tbl_hdl,
				  struct pipe_tbl_match_spec *match_spec,
				  struct pipe_data_spec *data_spec,
				  uint32_t *ent_hdl_p)
{
	struct pipe_mgr_value_lookup_entry_info *entry;
	struct pipe_mgr_value_lookup *tbl;
	int status = BF_SUCCESS;
	bool exists;

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
					PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
					(void *)&tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed", tbl_hdl);
		goto cleanup;
	}

	if (tbl->ctx.store_entries) {
		status = pipe_mgr_table_key_exists((void *)tbl, PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
						   match_spec, dev_tgt.dev_pipe_id,
						   &exists, ent_hdl_p,
						   NULL);
		if (status) {
			LOG_ERROR("pipe_mgr_table_key_exists failed");
			goto cleanup;
		}
		if (exists) {
			LOG_ERROR("duplicate entry found in table = %s", tbl->ctx.name);
			status = BF_UNEXPECTED;
			goto cleanup;
		}
	}

	status = pipe_mgr_value_lookup_pack_entry_info(dev_tgt, tbl, match_spec,
						       data_spec, &entry);
	if (status) {
		LOG_ERROR("entry encoding failed");
		goto cleanup;
	}

	status = dal_value_lookup_ent_add(sess_hdl, dev_tgt, tbl_hdl, match_spec,
					  data_spec, &tbl->ctx);
	if (status) {
		LOG_ERROR("dal_value_lookup_ent_add failed");
		goto cleanup_entry;
	}

	if (tbl->ctx.store_entries) {
		status = pipe_mgr_table_key_insert(dev_tgt, (void *)tbl,
						   PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
						   (void *)entry, ent_hdl_p);
		if (status) {
			LOG_ERROR("Error in inserting entry in table");
			goto cleanup_entry;
		}
	}

	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;

cleanup_entry:
	pipe_mgr_value_lookup_del_entry(entry);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/*
 * Method to delete entries from value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param match_spec: key spec for table.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_ent_del(uint32_t sess_hdl,
				  struct bf_dev_target_t dev_tgt,
				  uint32_t tbl_hdl,
				  struct pipe_tbl_match_spec *match_spec)
{
	struct pipe_mgr_value_lookup_entry_info *entry;
	struct pipe_mgr_value_lookup *tbl;
	int status = BF_SUCCESS;
	uint32_t ent_hdl;
	bool exists;

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
					PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
					(void *)&tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed", tbl_hdl);
		goto cleanup;
	}

	if (tbl->ctx.store_entries) {
		status = pipe_mgr_table_key_exists((void *)tbl, PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
						   match_spec, dev_tgt.dev_pipe_id,
						   &exists, &ent_hdl,
						   (void **)&entry);
		if (status) {
			LOG_ERROR("pipe_mgr_table_key_exists failed");
			goto cleanup;
		}

		if (!exists) {
			LOG_ERROR("entry not found in table = %s", tbl->ctx.name);
			status = BF_UNEXPECTED;
			goto cleanup;
		}
	}

	status = dal_value_lookup_ent_del(sess_hdl, dev_tgt, tbl_hdl, match_spec,
					  &tbl->ctx);
	if (status) {
		LOG_ERROR("dal table entry del failed");
		goto cleanup;
	}

	if (tbl->ctx.store_entries) {
		status = pipe_mgr_table_key_delete(dev_tgt, (void *)tbl,
						   PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
						   match_spec);
		if (status) {
			LOG_ERROR("table entry del failed");
			goto cleanup;
		}
		pipe_mgr_value_lookup_del_entry(entry);
	}

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/*
 * Method to get entries from value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param ent_hdl: entry handle.
 * @param match_spec: key spec for table.
 * @param data_spec: data to be fetched from table.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_ent_get(uint32_t sess_hdl,
				  struct bf_dev_target_t dev_tgt,
				  uint32_t tbl_hdl,
				  uint32_t ent_hdl,
				  struct pipe_tbl_match_spec *match_spec,
				  struct pipe_data_spec *data_spec)
{
	struct pipe_mgr_value_lookup_entry_info *entry;
	struct pipe_mgr_value_lookup *tbl;
	int status = BF_SUCCESS;

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
					PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
					(void *)&tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed",
				tbl_hdl);
		goto cleanup;
	}

	if (!tbl->ctx.store_entries) {
		LOG_ERROR("Not supported. Rule entries are not stored");
		status = BF_NOT_SUPPORTED;
		goto cleanup;
	}

	status = pipe_mgr_table_get(tbl, PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
				    dev_tgt.dev_pipe_id, ent_hdl,
				    (void **)&entry);
	if (status) {
		LOG_ERROR("Failed to retrieve entry for hdl: %d", ent_hdl);
		goto cleanup;
	}

	status = pipe_mgr_value_lookup_unpack_entry_info(dev_tgt, tbl, match_spec,
							 data_spec, ent_hdl, entry);

	if (status)
		LOG_ERROR("Unpacking enty data failed for entry hdl %d\n", ent_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/*
 * Method to get first entry from value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param match_spec: key spec for table.
 * @param data_spec: data to be fetched from table.
 * @param ent_hdl_p: entry handle ptr.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_ent_get_first(uint32_t sess_hdl,
					struct bf_dev_target_t dev_tgt,
					uint32_t tbl_hdl,
					struct pipe_tbl_match_spec *match_spec,
					struct pipe_data_spec *data_spec,
					uint32_t *ent_hdl_p)
{
	struct pipe_mgr_value_lookup *tbl;
	int status = BF_SUCCESS;

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
					PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
					(void *)&tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed", tbl_hdl);
		goto cleanup;
	}

	status = dal_value_lookup_ent_get_first();
	if (status)
		LOG_ERROR("Getting first entry failed for table %d", tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/*
 * Method to get next n entries from value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param cur_match_spec: current key spec.
 * @param n: number of entries to fetch.
 * @param match_specs: keys spec for table.
 * @param data_spec: data to be fetched from table.
 * @param num: number of entries fetched from table.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_ent_get_next_n_by_key(uint32_t sess_hdl,
						struct bf_dev_target_t dev_tgt,
						uint32_t tbl_hdl,
						struct pipe_tbl_match_spec *cur_match_spec,
						uint32_t n,
						struct pipe_tbl_match_spec *match_specs,
						struct pipe_data_spec **data_specs,
						uint32_t *num)
{
	struct pipe_mgr_value_lookup *tbl;
	int status = BF_SUCCESS;

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
					PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
					(void *)&tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed", tbl_hdl);
		goto cleanup;
	}

	status = dal_value_lookup_ent_get_next_n_by_key();
	if (status)
		LOG_ERROR("Getting next n entries failed for table %d", tbl_hdl);

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/*
 * Method to get first entry handle for value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param ent_hdl_p: entry handle ptr.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_get_first_ent_handle(uint32_t sess_hdl,
					       struct bf_dev_target_t dev_tgt,
					       uint32_t tbl_hdl,
					       uint32_t *ent_hdl_p)
{
	struct pipe_mgr_value_lookup *tbl;
	int status = BF_SUCCESS;

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
					PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
					(void *)&tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed", tbl_hdl);
		goto cleanup;
	}

	if (tbl->ctx.store_entries) {
		status = pipe_mgr_table_get_first((void *)tbl, PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
						  dev_tgt.dev_pipe_id, ent_hdl_p);
	} else {
		LOG_ERROR("Not supported. Entries are not stored in SDE");
		status = BF_NOT_SUPPORTED;
	}

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/*
 * Method to get next n entry handles for value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param ent_hdl: entry handle.
 * @param num_hdl: num of next n handles to get.
 * @param next_ent_hdl: next n entry handles.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_get_next_n_ent_handle(uint32_t sess_hdl,
						struct bf_dev_target_t dev_tgt,
						uint32_t tbl_hdl,
						uint32_t ent_hdl,
						uint32_t num_hdl,
						uint32_t *next_ent_hdl)
{
	struct pipe_mgr_value_lookup *tbl;
	int status = BF_SUCCESS;

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
					PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
					(void *)&tbl);
	if (status) {
		LOG_ERROR("Retrieving context json object for table %d failed", tbl_hdl);
		goto cleanup;
	}

	if (tbl->ctx.store_entries) {
		status = pipe_mgr_table_get_next_n((void *)tbl, PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
						   dev_tgt.dev_pipe_id,
						   ent_hdl, num_hdl,
						   next_ent_hdl);
	} else {
		LOG_ERROR("Not supported. Entries are not stored in SDE");
		next_ent_hdl[0] = -1;
		status = BF_NOT_SUPPORTED;
	}

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/*
 * Method to map match spec with entry handle for value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param match_spec: key spec for table.
 * @param ent_hdl_p: entry handle ptr.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_match_spec_to_ent_hdl(uint32_t sess_hdl,
						struct bf_dev_target_t dev_tgt,
						uint32_t tbl_hdl,
						struct pipe_tbl_match_spec *match_spec,
						uint32_t *ent_hdl_p)
{
	struct pipe_mgr_value_lookup *tbl;
	bool exists;
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

	status = pipe_mgr_ctx_get_table(dev_tgt, tbl_hdl,
					PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP, (void *)&tbl);
	if (status)
		goto cleanup;

	if (!tbl->ctx.store_entries) {
		LOG_ERROR("Not supported. Rule entries are not stored");
		status = BF_NOT_SUPPORTED;
		goto cleanup;
	}

	status = pipe_mgr_table_key_exists((void *)tbl, PIPE_MGR_TABLE_TYPE_VALUE_LOOKUP,
					   match_spec, dev_tgt.dev_pipe_id,
					   &exists, ent_hdl_p, NULL);
	if (status) {
		LOG_ERROR("pipe_mgr_table_key_exists failed");
		goto cleanup;
	}

	if (!exists) {
		LOG_TRACE("entry not found");
		status = BF_OBJECT_NOT_FOUND;
	}

cleanup:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);
	LOG_TRACE("Exiting %s", __func__);
	return status;
}

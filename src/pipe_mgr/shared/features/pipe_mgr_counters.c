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

/*!
 * @file pipe_mgr_counters.c
 *
 * @description Utilities for counters
 */
#include <pipe_mgr/pipe_mgr_intf.h>
#include "../infra/pipe_mgr_int.h"
#include "pipe_mgr_counters.h"
#include "../dal/dal_counters.h"
#include "../../shared/pipe_mgr_shared_intf.h"
#include "../../core/pipe_mgr_ctx_json.h"

/*!
 * fetch counter id from global counter pool.
 *
 * @param counter_id allocate a counter and return
 * @param pool_id to be used to allocate the counter from
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_allocate_counter_id(uint32_t *counter_id, uint8_t pool_id)
{
	return dal_cnt_allocate_counter_id(counter_id, pool_id);
}

/*!
 * Returns back the counter ID to the global counter pool.
 *
 * @param counter_id counter that needs to be returned to the pool.
 * @param pool_id validate the counter pool to be used.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_free_counter_id(uint32_t counter_id, uint8_t pool_id)
{
	return dal_cnt_free_counter_id(counter_id, pool_id);
}

/*!
 * Reads DDR to get the counter pair value.
 *
 * @param abs_id absolute counter id to be read
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_read_counter_pair(int abs_id, void *stats)
{
	return dal_cnt_read_counter_pair(abs_id, stats);
}

/*!
 * Reads DDR to get the flow counter pair value.
 *
 * @param id flow counter id to be read for stats
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_read_flow_counter_pair(uint32_t id, void *stats)
{
	return dal_cnt_read_flow_counter_pair(id, stats);
}

/*!
 * Reads DDR to get the flow counter pair value.
 *
 * @param id assignable counter id to be read for stats
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_read_assignable_counter_set(dev_target_t dev_tgt,
				         const char *name,
					 int id,
					 void *stats)
{
	return dal_cnt_read_assignable_counter_set(dev_tgt, name, id, stats);
}

/*!
 * Write flow counter pair value for a specific index.
 *
 * @param dev_tgt device target
 * @param table name
 * @param id counter id to write stats
 * @param value to be updated
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_mod_assignable_counter_set(dev_target_t dev_tgt,
					const char *name,
					int id,
					uint64_t value)
{
	return dal_cnt_write_assignable_counter_set(dev_tgt, name, id, value);
}


/*!
 * Clears the flow counter pair (pkts and bytes).
 *
 * @param counter_id flow counter id that needs cleared.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_clear_flow_counter_pair(uint32_t counter_id)
{
	return dal_cnt_clear_flow_counter_pair(counter_id);
}

/*!
 * Clears the assignable counter set (set of pkts and bytes).
 *
 * @param counter_id assignable counter id that needs cleared.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_clear_assignable_counter_set(uint32_t counter_id)
{
	return dal_cnt_clear_assignable_counter_set(counter_id);
}

/*!
 * Displays the flow counter pair (pair of pkts and bytes).
 *
 * @param stats flow counter stats that needs to be displayed.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_display_flow_counter_pair(void *stats)
{
	return dal_cnt_display_flow_counter_pair(stats);
}

/*!
 * Displays the assignable counter set (set of pkts and bytes).
 *
 * @param stats assignable stats that needs to be displayed.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_display_assignable_counter_set(void *stats)
{
	return dal_cnt_display_assignable_counter_set(stats);
}

static void pipe_mgr_cnt_set_value(pipe_stat_data_t *stat_data,
				   enum externs_attr_type attr_type,
				   uint64_t value)
{
	switch (attr_type) {
	  case EXTERNS_ATTR_TYPE_BYTES:
		stat_data->bytes  = value;
		break;
	  case EXTERNS_ATTR_TYPE_PACKETS:
		stat_data->packets = value;
		break;
	  default:
		break;
	}
}

/*!
 * routine to query a stats entry.
 */
bf_status_t pipe_mgr_stat_ent_query(pipe_sess_hdl_t sess_hdl,
				    dev_target_t dev_tgt,
				    const char *table_name,
				    pipe_stat_ent_idx_t stat_ent_idx,
				    pipe_stat_data_t *stat_data)
{
	const char  *ptr    = NULL;
	uint64_t     value  = 0;
	bf_status_t  status = BF_SUCCESS;
	struct pipe_mgr_p4_pipeline *ctx_obj = NULL;
	struct pipe_mgr_externs_ctx *externs_entry = NULL;
	char key_name[P4_SDE_TABLE_NAME_LEN] = {0};
	char target_name[P4_SDE_TABLE_NAME_LEN + 10]  = {0};

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);

	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	/* retrieve context json object associated with dev_tgt */
	status = pipe_mgr_get_profile_ctx(dev_tgt, &ctx_obj);

	if (status)
		goto error;

	/* extract table name which is used as a key in hash map */
	ptr = trim_classifier_str((char*)table_name);
	strncpy(key_name, ptr, P4_SDE_TABLE_NAME_LEN-1);

	externs_entry = bf_hashtbl_search(ctx_obj->bf_externs_htbl, key_name);

	if (!externs_entry) {
		LOG_ERROR("externs object/entry get for table \"%s\" failed",
			   table_name);
		status = BF_OBJECT_NOT_FOUND;
		goto error;
	}

	switch (externs_entry->attr_type) {
	  case EXTERNS_ATTR_TYPE_BYTES:
	  case EXTERNS_ATTR_TYPE_PACKETS:
		status = pipe_mgr_cnt_read_assignable_counter_set(
				dev_tgt,
				externs_entry->target_name,
				stat_ent_idx,
				(void *)&value);
		if (status)
			goto error;

		pipe_mgr_cnt_set_value(stat_data,
				       externs_entry->attr_type,
				       value);
		break;
	  case EXTERNS_ATTR_TYPE_PACKETS_AND_BYTES:
		/* for packet and bytes, fetch counter values for
		 * bytes and packets in saparate calls by appending
		 * target name with respective attribute type */
		snprintf(target_name, sizeof(target_name), "%s_%s",
				externs_entry->target_name, "packets");

		status = pipe_mgr_cnt_read_assignable_counter_set(
				dev_tgt,
				target_name,
				stat_ent_idx,
				(void *)&value);

		if (status)
			goto error;

		pipe_mgr_cnt_set_value(stat_data,
				       EXTERNS_ATTR_TYPE_PACKETS,
				       value);

		memset(target_name, 0x0, sizeof(target_name));
		snprintf(target_name, sizeof(target_name), "%s_%s",
				externs_entry->target_name, "bytes");

		status = pipe_mgr_cnt_read_assignable_counter_set(
				dev_tgt,
				target_name,
				stat_ent_idx,
				(void *)&value);

		if (status)
			goto error;

		pipe_mgr_cnt_set_value(stat_data,
				       EXTERNS_ATTR_TYPE_BYTES,
				       value);
		break;
	  default:
		status = BF_INTERNAL_ERROR;
	}
error:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/*!
 * routine to write flow counter index
 */
bf_status_t pipe_mgr_stat_ent_set(pipe_sess_hdl_t sess_hdl,
				  dev_target_t dev_tgt,
				  const char *table_name,
				  pipe_stat_ent_idx_t stat_ent_idx,
				  pipe_stat_data_t *stat_data)
{
	const char  *ptr    = NULL;
	uint64_t     value  = 0;
	bf_status_t  status = BF_SUCCESS;
	struct pipe_mgr_p4_pipeline *ctx_obj = NULL;
	struct pipe_mgr_externs_ctx *externs_entry = NULL;
	char key_name[P4_SDE_TABLE_NAME_LEN] = {0};
	char target_name[P4_SDE_TABLE_NAME_LEN + 10]  = {0};

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);

	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	/* retrieve context json object associated with dev_tgt */
	status = pipe_mgr_get_profile_ctx(dev_tgt, &ctx_obj);

	if (status)
		goto error;

	/* extract table name which is used as a key in hash map */
	ptr = trim_classifier_str((char*)table_name);
	strncpy(key_name, ptr, P4_SDE_TABLE_NAME_LEN - 1);

	externs_entry = bf_hashtbl_search(ctx_obj->bf_externs_htbl, key_name);

	if (!externs_entry) {
		LOG_ERROR("externs object/entry get for table \"%s\" failed",
			   table_name);
		status = BF_OBJECT_NOT_FOUND;
		goto error;
	}

	switch (externs_entry->attr_type) {
	  case EXTERNS_ATTR_TYPE_BYTES:
	  case EXTERNS_ATTR_TYPE_PACKETS:
		status = pipe_mgr_cnt_mod_assignable_counter_set(
				dev_tgt,
				externs_entry->target_name,
				stat_ent_idx,
				value);
		if (status)
			goto error;
		break;
	  case EXTERNS_ATTR_TYPE_PACKETS_AND_BYTES:
		/* for packet and bytes, update counter values for
		 * bytes and packets in saparate calls by appending
		 * target name with respective attribute type */
		snprintf(target_name, sizeof(target_name), "%s_%s",
				externs_entry->target_name, "packets");

		status = pipe_mgr_cnt_mod_assignable_counter_set(
				dev_tgt,
				target_name,
				stat_ent_idx,
				value);

		if (status)
			goto error;

		memset(target_name, 0x0, sizeof(target_name));
		snprintf(target_name, sizeof(target_name), "%s_%s",
				externs_entry->target_name, "bytes");

		status = pipe_mgr_cnt_mod_assignable_counter_set(
				dev_tgt,
				target_name,
				stat_ent_idx,
				value);

		if (status)
			goto error;

		break;
	  default:
		status = BF_INTERNAL_ERROR;
	}
error:
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

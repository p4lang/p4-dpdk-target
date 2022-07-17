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
 * @file dal_counters.c
 *
 * @description Utilities for counters
 */
#include <infra/dpdk_infra.h>
#include "../dal_counters.h"
#include "../../pipe_mgr_shared_intf.h"
#include <pipe_mgr/shared/pipe_mgr_mat.h>
#include <pipe_mgr/core/pipe_mgr_ctx_json.h>
/*!
 * Initialize the global counter pool.
 *
 * @param pipe_ctx pipe mgr pipline ctx info.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_init(struct pipe_mgr_p4_pipeline *pipe_ctx)
{
	LOG_TRACE("STUB:%s\n", __func__);
	return BF_SUCCESS;
}

/*!
 * Unnitialize the global counter pool.
 *
 * @param pipe_ctx pipe mgr pipline ctx info.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_destroy(struct pipe_mgr_p4_pipeline *pipe_ctx)
{
	LOG_TRACE("STUB:%s\n", __func__);
	return BF_SUCCESS;
}

/*!
 * fetch counter id from global counter pool.
 *
 * @param counter_id allocate a counter and return
 * @param pool_id to be used to allocate the counter from
 * @return Status of the API call
 */
bf_status_t
dal_cnt_allocate_counter_id(uint32_t *counter_id, uint8_t pool_id)
{
	LOG_TRACE("STUB:%s\n", __func__);
	return BF_SUCCESS;
}

/*!
 * Returns back the counter ID to the global counter pool.
 *
 * @param counter_id counter that needs to be returned to the pool.
 * @param pool_id validate the counter pool to be used.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_free_counter_id(uint32_t counter_id, uint8_t pool_id)
{
	LOG_TRACE("STUB:%s\n", __func__);
	return BF_SUCCESS;
}

/*!
 * Reads DDR to get the counter pair value.
 *
 * @param abs_id absolute counter id to be read
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
dal_cnt_read_counter_pair(int abs_id, void *stats)
{
	LOG_TRACE("STUB:%s\n", __func__);
	return BF_SUCCESS;
}

/*!
 * Reads DDR to get the flow counter pair value.
 *
 * @param id flow counter id to be read for stats
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
dal_cnt_read_flow_counter_pair(uint32_t id, void *stats)
{
	LOG_TRACE("STUB:%s\n", __func__);
	return BF_SUCCESS;
}

static void dal_cnt_set_value(pipe_stat_data_t *stat_data,
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
 * Reads DDR to get the flow counter pair value.
 *
 * @param id assignable counter id to be read for stats
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
dal_cnt_read_assignable_counter_set(bf_dev_target_t dev_tgt,
                                    const char *name,
                                    int id,
                                    void *stats)
{
	bf_status_t status = BF_SUCCESS;
	const char  *ptr   = NULL;
	uint64_t     value = 0;
	struct pipeline *pipe = NULL;
	struct pipe_mgr_profile *profile            = NULL;
	struct pipe_mgr_p4_pipeline *ctx_obj        = NULL;
	struct pipe_mgr_externs_ctx *externs_entry  = NULL;
	char   key_name[P4_SDE_TABLE_NAME_LEN]        = {0};
	char   target_name[P4_SDE_COUNTER_TARGET_LEN] = {0};

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
	ptr = trim_classifier_str((char*)name);
	strncpy(key_name, ptr, P4_SDE_TABLE_NAME_LEN-1);

	externs_entry = bf_hashtbl_search(ctx_obj->bf_externs_htbl, key_name);

	if (!externs_entry) {
		LOG_ERROR("externs object/entry get for table \"%s\" failed",
				name);
		return BF_OBJECT_NOT_FOUND;
	}

	switch (externs_entry->attr_type) {
	  case EXTERNS_ATTR_TYPE_BYTES:
	  case EXTERNS_ATTR_TYPE_PACKETS:
		  //read counter stats from dpdk pipeline
		  status = rte_swx_ctl_pipeline_regarray_read(
				  pipe->p,
				  externs_entry->target_name,
				  id,
				  &value);

		  if (status) {
			  LOG_ERROR("%s:Counter read failed for Name[%s][%d]\n"
					  , __func__,
					  externs_entry->target_name,
					  id);

			  return BF_OBJECT_NOT_FOUND;
		  }

		  dal_cnt_set_value(stats, externs_entry->attr_type, value);
		  break;
	  case EXTERNS_ATTR_TYPE_PACKETS_AND_BYTES:
		  /* for packet and bytes, fetch counter values for
		   * bytes and packets in saparate calls by appending
		   * target name with respective attribute type */
		  snprintf(target_name, sizeof(target_name), "%s_%s",
				  externs_entry->target_name, "packets");
		  //read counter stats from dpdk pipeline
		  status = rte_swx_ctl_pipeline_regarray_read(
				  pipe->p,
				  target_name,
				  id,
				  &value);

		  if (status) {
			  LOG_ERROR("%s:Counter read failed for Name[%s][%d]\n"
					  , __func__,
					  externs_entry->target_name,
					  id);

			  return BF_OBJECT_NOT_FOUND;
		  }

		  dal_cnt_set_value(stats, EXTERNS_ATTR_TYPE_PACKETS, value);

		  memset(target_name, 0x0, sizeof(target_name));
		  snprintf(target_name, sizeof(target_name), "%s_%s",
				  externs_entry->target_name, "bytes");

		  //read counter stats from dpdk pipeline
		  status = rte_swx_ctl_pipeline_regarray_read(
				  pipe->p,
				  target_name,
				  id,
				  &value);

		  if (status) {
			  LOG_ERROR("%s:Counter read failed for Name[%s][%d]\n"
					  , __func__,
					  externs_entry->target_name,
					  id);
			  return BF_OBJECT_NOT_FOUND;
		  }

		  dal_cnt_set_value(stats, EXTERNS_ATTR_TYPE_BYTES, value);
		  break;
		default:
		  LOG_ERROR("invalid attribute type for counter table %s",
				  externs_entry->target_name);
		  return (BF_INTERNAL_ERROR);
	}

	return BF_SUCCESS;
}

/*!
 * Write flow counter value for a specific index.
 *
 * @param dev_tgt device target
 * @param table name
 * @param id counter id to write stats
 * @param value to be updated
 * @return Status of the API call
 */
bf_status_t
dal_cnt_write_assignable_counter_set(bf_dev_target_t dev_tgt,
                                     const char *name,
                                     int id,
                                     void *stats)
{
	bf_status_t  status = BF_SUCCESS;
	const char  *ptr    = NULL;
	uint64_t     value  = 0;
	struct  pipeline *pipe;
	struct  pipe_mgr_profile *profile           = NULL;
	struct  pipe_mgr_p4_pipeline *ctx_obj       = NULL;
	struct  pipe_mgr_externs_ctx *externs_entry = NULL;
	char    key_name[P4_SDE_TABLE_NAME_LEN]     = {0};
	char    target_name[P4_SDE_COUNTER_TARGET_LEN] = {0};

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
	ptr = trim_classifier_str((char*)name);
	strncpy(key_name, ptr, P4_SDE_TABLE_NAME_LEN - 1);

	externs_entry = bf_hashtbl_search(ctx_obj->bf_externs_htbl, key_name);

	if (!externs_entry) {
		LOG_ERROR("externs object/entry get for table \"%s\" failed",
				name);
		return BF_OBJECT_NOT_FOUND;
	}

	switch (externs_entry->attr_type) {
	  case EXTERNS_ATTR_TYPE_BYTES:
	  case EXTERNS_ATTR_TYPE_PACKETS:
		  status = rte_swx_ctl_pipeline_regarray_write(
				  pipe->p,
				  externs_entry->target_name,
				  id,
				  value);
		  if (status) {
			LOG_ERROR("%s:Counter write failed for Name[%s][%d]\n",
					  __func__, name, id);
			return BF_OBJECT_NOT_FOUND;
		  }
		  break;
	  case EXTERNS_ATTR_TYPE_PACKETS_AND_BYTES:
		  /* for packet and bytes, update counter values for
		   * bytes and packets in saparate calls by appending
		   * target name with respective attribute type */
		  snprintf(target_name, sizeof(target_name), "%s_%s",
				  externs_entry->target_name, "packets");

		  status = rte_swx_ctl_pipeline_regarray_write(
				  pipe->p,
				  target_name,
				  id,
				  value);
		  if (status) {
			LOG_ERROR("%s:Counter write failed for Name[%s][%d]\n",
					  __func__, name, id);
			return BF_OBJECT_NOT_FOUND;
		  }
		  memset(target_name, 0x0, sizeof(target_name));
		  snprintf(target_name, sizeof(target_name), "%s_%s",
				  externs_entry->target_name, "bytes");

		  status = rte_swx_ctl_pipeline_regarray_write(
				  pipe->p,
				  target_name,
				  id,
				  value);
		  if (status) {
			LOG_ERROR("%s:Counter write failed for Name[%s][%d]\n",
					  __func__, name, id);
			return BF_OBJECT_NOT_FOUND;
		  }
		  break;
	  default:
		  LOG_ERROR("invalid attribute type for counter table %s",
				  externs_entry->target_name);
		  return (BF_INTERNAL_ERROR);
	}
	return BF_SUCCESS;
}

/*!
 * Clears the flow counter pair (pkts and bytes).
 *
 * @param counter_id flow counter id that needs cleared.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_clear_flow_counter_pair(uint32_t counter_id)
{
	LOG_TRACE("STUB:%s\n", __func__);
	return BF_SUCCESS;
}

/*!
 * Clears the assignable counter set (set of pkts and bytes).
 *
 * @param counter_id assignable counter id that needs cleared.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_clear_assignable_counter_set(uint32_t counter_id)
{
	LOG_TRACE("STUB:%s\n", __func__);
	return BF_SUCCESS;
}

/*!
 * Displays the flow counter pair (pair of pkts and bytes).
 *
 * @param stats flow counter stats that needs to be displayed.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_display_flow_counter_pair(void *stats)
{
	LOG_TRACE("STUB:%s\n", __func__);
	return BF_SUCCESS;
}

/*!
 * Displays the assignable counter set (set of pkts and bytes).
 *
 * @param stats assignable stats that needs to be displayed.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_display_assignable_counter_set(void *stats)
{
	LOG_TRACE("STUB:%s\n", __func__);
	return BF_SUCCESS;
}

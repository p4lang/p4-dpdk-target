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

#include "../dal_counters.h"

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

/*!
 * Reads DDR to get the flow counter pair value.
 *
 * @param id assignable counter id to be read for stats
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
dal_cnt_read_assignable_counter_set(int id, void *stats)
{
	LOG_TRACE("STUB:%s\n", __func__);
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

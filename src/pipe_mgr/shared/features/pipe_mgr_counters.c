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
pipe_mgr_cnt_read_assignable_counter_set(int id, void *stats)
{
	return dal_cnt_read_assignable_counter_set(id, stats);
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

bf_status_t pipe_mgr_stat_ent_query(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                      pipe_stat_ent_idx_t stat_ent_idx,
                                      pipe_stat_data_t *stat_data)
{
        int status;

        LOG_TRACE("Entering %s", __func__);

        status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
        if (status) {
                LOG_ERROR("API prologue failed with err: %d", status);
                LOG_TRACE("Exiting %s", __func__);
                return status;
        }

        status = pipe_mgr_cnt_read_assignable_counter_set(stat_ent_idx,
                                                          (void *)stat_data);

        pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

        LOG_TRACE("Exiting %s", __func__);
        return status;
}

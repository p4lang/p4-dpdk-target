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
 * @file pipe_mgr_counters.h
 *
 * @description Utilities for counters
 */

#ifndef __PIPE_MGR_COUNTERS_H__
#define __PIPE_MGR_COUNTERS_H__

#include <bf_types/bf_types.h>

/*!
 * fetch counter id from global counter pool.
 *
 * @param counter_id allocate a counter and return
 * @param pool_id to be used to allocate the counter from
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_allocate_counter_id(uint32_t *counter_id, uint8_t pool_id);

/*!
 * Returns back the counter ID to the global counter pool.
 *
 * @param counter_id counter that needs to be returned to the pool.
 * @param pool_id validate the counter pool to be used.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_free_counter_id(uint32_t counter_id, uint8_t pool_id);

/*!
 * Reads DDR to get the counter pair value.
 *
 * @param abs_id absolute counter id to be read
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_read_counter_pair(int abs_id, void *stats);

/*!
 * Reads DDR to get the flow counter pair value.
 *
 * @param id flow counter id to be read for stats
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_read_flow_counter_pair(uint32_t id, void *stats);

/*!
 * Reads DDR to get the flow counter pair value.
 *
 * @param id assignable counter id to be read for stats
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_read_flow_indirect_counter_set(bf_dev_target_t dev_tgt,
					    const char *name,
					    int id,
					    void *stats);

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
pipe_mgr_cnt_mod_assignable_counter_set(bf_dev_target_t dev_tgt,
                                        const char *name,
                                        int id,
                                        void *stats);

/*!
 * Clears the flow counter pair (pkts and bytes).
 *
 * @param counter_id flow counter id that needs cleared.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_clear_flow_counter_pair(uint32_t counter_id);

/*!
 * Clears the assignable counter set (set of pkts and bytes).
 *
 * @param counter_id assignable counter id that needs cleared.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_clear_assignable_counter_set(uint32_t counter_id);

/*!
 * Displays the flow counter pair (pair of pkts and bytes).
 *
 * @param stats flow counter stats that needs to be displayed.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_display_flow_counter_pair(void *stats);

/*!
 * Displays the assignable counter set (set of pkts and bytes).
 *
 * @param stats assignable stats that needs to be displayed.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_cnt_display_assignable_counter_set(void *stats);

#endif /* __PIPE_MGR_COUNTERS_H__ */

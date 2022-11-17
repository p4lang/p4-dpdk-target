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
 * @file dal_counters.h
 *
 * @description Utilities for counters
 */

#ifndef __DAL_COUNTERS_H__
#define __DAL_COUNTERS_H__

#include <bf_types/bf_types.h>
#include "../../core/pipe_mgr_log.h"
#include "../infra/pipe_mgr_int.h"
/*
 * up to 6 general independent count actions per packet (two per SEM, WCM, LEM)
 */
#define MAX_COUNT_PER_BLOCK             2
/*
 * This needs to be same as defined in src/lld/mev/cp/nd_linux-mev_cp/src/cp_counters.h
 */
#define CP_MAX_ASSIGN_CNT_SET_CNT           2048

/*!
 * Initialize the global counter pool.
 *
 * @param pipe_ctx pipe mgr pipline ctx info.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_init(struct pipe_mgr_p4_pipeline *pipe_ctx);

/*!
 * Destroy the global counter pool.
 *
 * @param pipe_ctx pipe mgr pipline ctx info.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_destroy(struct pipe_mgr_p4_pipeline *pipe_ctx);

/*!
 * fetch a counter id from global counter pool.
 *
 * @param counter_id allocate a counter and return
 * @param pool_id to be used to allocate the counter from
 * @return Status of the API call
 */
bf_status_t
dal_cnt_allocate_counter_id(uint32_t *counter_id, uint8_t pool_id);

/*!
 * Returns back the counter ID to the free pool.
 *
 * @param counter_id counter that needs to be returned to the pool.
 * @param pool_id validate the counter pool to be used.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_free_counter_id(uint32_t counter_id, uint8_t pool_id);

/*!
 * Reads DDR to get the counter pair value.
 *
 * @param abs_id absolute counter id to be read
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
dal_cnt_read_counter_pair(int abs_id, void *stats);

/*!
 * Reads DDR to get the flow counter pair value.
 *
 * @param id flow counter id to be read for stats
 * @param address of stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
dal_cnt_read_flow_counter_pair(uint32_t id, void **stats);

/*!
 * Reads DDR to get the flow indirect counter pair value.
 *
 * @param dev_tgt device target
 * @param table_name table name
 * @param id indirect counter id to be read for stats
 * @param stats buffer to fill stats
 * @return Status of the API call
 */
bf_status_t
dal_cnt_read_flow_indirect_counter_set(bf_dev_target_t dev_tgt,
				       const char *table_name,
				       int id,
				       void *stats);

/*!
 * Reads DDR to get the flow direct counter pair value.
 *
 * @param  dal_data              Pointer to Dal layer data.
 * @param  res_data              Pointer to res data to be filled with stats.
 * @param  dev_tgt               device target.
 * @param  match_spec            Match spec to populate.
 * @return                       Status of the API call
 */
bf_status_t
dal_cnt_read_flow_direct_counter_set(void *dal_data, void *res_data,
                                     bf_dev_target_t dev_tgt,
                                     struct pipe_tbl_match_spec *match_spec);

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
dal_cnt_write_assignable_counter_set(bf_dev_target_t dev_tgt,
                                     const char *table_name,
                                     int id,
                                     void *stats);

/*!
 * Clears the flow counter pair (pkts and bytes).
 *
 * @param counter_id flow counter id that needs cleared.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_clear_flow_counter_pair(uint32_t counter_id);

/*!
 * Clears the assignable counter set (set of pkts and bytes).
 *
 * @param counter_id assignable counter id that needs cleared.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_clear_assignable_counter_set(uint32_t counter_id);

/*!
 * Displays the flow counter pair (pair of pkts and bytes).
 *
 * @param stats flow counter stats that needs to be displayed.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_display_flow_counter_pair(void *stats);

/*!
 * Displays the assignable counter set (set of pkts and bytes).
 *
 * @param stats assignable stats that needs to be displayed.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_display_assignable_counter_set(void *stats);

/*!
 * Free extern objects hash map
 *
 * @param pipe_ctx pipe mgr pipline ctx info.
 * @return Status of the API call
 */
bf_status_t
dal_cnt_free_externs(struct pipe_mgr_p4_pipeline *pipe_ctx);
#endif /* __DAL_COUNTERS_H__ */

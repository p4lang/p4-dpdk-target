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
/** @file bf_rt_table_operations.h
 *
 * @brief Contains BF-RT Table operations APIs
 */
#ifndef _BF_RT_TABLE_OPERATIONS_H
#define _BF_RT_TABLE_OPERATIONS_H

#include <bf_rt/bf_rt_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Table Operations Type
 */
typedef enum bf_rt_table_operations_mode_ {
  /** Update sw value of all counters with the value in hw.
     Applicable on Counters or MATs with direct counters */
  BFRT_COUNTER_SYNC = 0,
  /** Update sw value of all registers with the value in hw.
     Applicable on Registers or MATs with direct registers */
  BFRT_REGISTER_SYNC = 1,
  /** Update sw value of all hit state o entries with the actual
     hw status. Applicable MATs with idletimeout POLL mode*/
  BFRT_HIT_STATUS_UPDATE = 2,
  BFRT_INVALID
} bf_rt_table_operations_mode_t;

/**
 * @brief Register Sync Callback
 * @param[in] dev_tgt Device target
 * @param[in] cookie User registered optional cookie
 */
typedef void (*bf_rt_register_sync_cb)(bf_rt_target_t *, void *);

/**
 * @brief Counter Sync Callback
 * @param[in] dev_tgt Device target
 * @param[in] cookie User registered optional cookie
 */
typedef void (*bf_rt_counter_sync_cb)(bf_rt_target_t *, void *);

/**
 * @brief Hit State Update Callback
 * @param[in] dev_tgt Device target
 * @param[in] cookie User registered optional cookie
 */
typedef void (*bf_rt_hit_state_update_cb)(bf_rt_target_t *, void *);

/**
 * @brief Set Register sync callback.
 *
 * @param[in] tbl_ops Table operations handle
 * @param[in] session Session Object
 * @param[in] dev_tgt Device target
 * @param[in] callback Register sync callback
 * @param[in] cookie User cookie
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_operations_register_sync_set(
    bf_rt_table_operations_hdl *tbl_ops,
    const bf_rt_session_hdl *session,
    const bf_rt_target_t *dev_tgt,
    const bf_rt_register_sync_cb callback,
    const void *cookie);

/**
 * @brief Set Counter sync callback.
 *
 * @param[in] tbl_ops Table operations handle
 * @param[in] session Session Object
 * @param[in] dev_tgt Device target
 * @param[in] callback Counter sync callback
 * @param[in] cookie User cookie
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_operations_counter_sync_set(
    bf_rt_table_operations_hdl *tbl_ops,
    const bf_rt_session_hdl *session,
    const bf_rt_target_t *dev_tgt,
    const bf_rt_counter_sync_cb callback,
    const void *cookie);

/**
 * @brief Set Hit State Update callback.
 *
 * @param[in] tbl_ops Table operations handle
 * @param[in] session Session Object
 * @param[in] dev_tgt Device target
 * @param[in] callback Hit State update sync callback
 * @param[in] cookie User cookie
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_operations_hit_state_update_set(
    bf_rt_table_operations_hdl *tbl_ops,
    const bf_rt_session_hdl *session,
    const bf_rt_target_t *dev_tgt,
    const bf_rt_hit_state_update_cb callback,
    const void *cookie);

#ifdef __cplusplus
}
#endif

#endif  // _BF_RT_TABLE_OPERATIONS_H

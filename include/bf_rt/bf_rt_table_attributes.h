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
/** @file bf_rt_table_attributes.h
 *
 *  @brief Contains BF-RT Table Attribute APIs
 */
#ifndef _BF_RT_TABLE_ATTRIBUTES_H
#define _BF_RT_TABLE_ATTRIBUTES_H

#include <bf_rt/bf_rt_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Attribute types. Table attributes properties on an entire
 * table. Main difference between Attributes and Operations is that
 * Attributes deal with some state but operations don't.
 */
typedef enum bf_rt_table_attributes_type_ {
  /** Pipe scope of all entries. Applicable to all Match Action
       Tables(MAT) */
  ENTRY_SCOPE = 0,
  /** Idle table on MATs if applicable*/
  IDLE_TABLE_RUNTIME = 1,
  /** Meter byte count asjust. Applicable to meter
                               tables and MATs if they have direct meters*/
  METER_BYTE_COUNT_ADJ = 3,
  /** Port status change cb set attribute. Applicable to Port table */
  PORT_STATUS_NOTIF = 4,
  /** Selector update CB*/
  SELECTOR_UPDATE_CALLBACK = 6
} bf_rt_table_attributes_type_t;

/**
 * @brief IdleTable Mode
 */
typedef enum attributes_idle_table_mode {
  /** Idle poll mode. When set, entry_hit_state on MAT entries can be
     queried to check idle time */
  BFRT_POLL_MODE,
  /** Idle notify mode. Can be used to set CB for idletimeout on a MAT */
  BFRT_NOTIFY_MODE,
  BFRT_INVALID_MODE
} bf_rt_attributes_idle_table_mode_t;

/**
 * @brief Pipe Entry scope
 */
typedef enum table_entry_scope {
  /** Set scope to all pipelines of current profile for this table. Turns
     table to symmetric. Default mode of tables */
  ENTRY_SCOPE_ALL_PIPELINES = 0,
  /** Set scope to a single logical pipe in this profile for this table.
      Turns table to assymmetric */
  ENTRY_SCOPE_SINGLE_PIPELINE = 1,
  /** Set scope to user defined scope in this profile for this table.
      Turns table to assymmetric but can be used to group some pipes
      together and hence can be used differently from single scope.
      Doesn't work for C-frontend currently */
  ENTRY_SCOPE_USER_DEFINED = 2
} bf_rt_table_entry_scope_t;

/**
 * @brief IdleTimeout Callback
 * @param[in] dev_tgt Device target
 * @param[in] key Table Key
 * @param[in] cookie User provided cookie during cb registration
 */
typedef bf_status_t (*bf_rt_idle_tmo_expiry_cb)(bf_rt_target_t *dev_tgt,
                                                bf_rt_table_key_hdl *key,
                                                void *cookie);

/**
 * @brief PortStatusChange Callback
 * @param[in] dev_tgt Device target
 * @param[in] key Port Table Key hdl
 * @param[in] port_up If port is up
 * @param[in] cookie User provided cookie during cb registration
 */
typedef bf_status_t (*bf_rt_port_status_chg_cb)(bf_rt_target_t *dev_tgt,
                                                bf_rt_table_key_hdl *key,
                                                bool port_up,
                                                void *cookie);

/**
 * @brief Selector Table Update Callback. This can be used to get notification
 * of data plane triggered Sel table update
 *
 * @param[in] session Session
 * @param[in] dev_tgt Device target
 * @param[in] cookie User provided cookie during cb registration
 * @param[in] sel_grp_id Selector-grp ID which was updated
 * @param[in] act_mbr_id Action-mbr ID which was updated
 * @param[in] logical_entry_index Table logical entry index
 * @param[in] is_add If the operation was add or del
 */
typedef void (*bf_rt_selector_table_update_cb)(const bf_rt_session_hdl *session,
                                               const bf_rt_target_t *dev_tgt,
                                               const void *cookie,
                                               const bf_rt_id_t sel_grp_id,
                                               const bf_rt_id_t act_mbr_id,
                                               const int logical_table_index,
                                               const bool is_add);

/**
 * @brief Set IdleTable Poll Mode options in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] enable            Flag to enable IdleTable
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_attributes_idle_table_poll_mode_set(
    bf_rt_table_attributes_hdl *tbl_attr, const bool enable);

/**
 * @brief Set IdleTable Notify Mode options in the Attributes Object
 *
 * @param[in] tbl_attr            Table attribute object handle
 * @param[in] enable              Enable IdleTimeout table
 * @param[in] callback            Will be called when entry expires
 * @param[in] ttl_query_interval  Inverval for querying entry TTL
 * @param[in] max_ttl             Max. allowed entry TTL value - not used
 * @param[in] min_ttl             Min. allowed entry TTL value - not used
 * @param[in] cookie              Used with callbacks
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_attributes_idle_table_notify_mode_set(
    bf_rt_table_attributes_hdl *tbl_attr,
    const bool enable,
    const bf_rt_idle_tmo_expiry_cb callback,
    const uint32_t ttl_query_interval,
    const uint32_t max_ttl,
    const uint32_t min_ttl,
    const void *cookie);

/**
 * @brief Get IdleTable configuration options in the Attributes Object
 * Only mode and enable params apply to poll mode.
 *
 * @param[in] tbl_attr             Table attribute object handle
 * @param[out] mode                IdleTable mode
 * @param[out] enable              IdleTimeout table enable
 * @param[out] callback            Will be called when entry expires
 * @param[out] ttl_query_interval  Inverval for querying entry TTL
 * @param[out] max_ttl             Max. allowed entry TTL value - not used
 * @param[out] min_ttl             Min. allowed entry TTL value - not used
 * @param[out] cookie              Used with callbacks
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_attributes_idle_table_get(
    const bf_rt_table_attributes_hdl *tbl_attr,
    bf_rt_attributes_idle_table_mode_t *mode,
    bool *enable,
    bf_rt_idle_tmo_expiry_cb *callback,
    uint32_t *ttl_query_interval,
    uint32_t *max_ttl,
    uint32_t *min_ttl,
    void **cookie);

/**
 * @brief Set Port status notificaiton options in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] enable            Port status notification enable
 * @param[in] callback          Will be called when port status chagnes
 * @param[in] cookie            Used with callbacks
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_attributes_port_status_notify_set(
    bf_rt_table_attributes_hdl *tbl_attr,
    const bool enable,
    const bf_rt_port_status_chg_cb callback,
    const void *cookie);

/**
 * @brief Get Port status notificaiton options in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] enable           Port status notification enable
 * @param[out] callback         Will be called when port status chagnes
 * @param[out] cookie           Used with callbacks
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_attributes_port_status_notify_get(
    const bf_rt_table_attributes_hdl *tbl_attr,
    bool *enable,
    bf_rt_port_status_chg_cb *callback,
    void **cookie);

/**
 * @brief Set Port Stat Poll Interval in Millisecond in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] poll_intvl_ms     Poll interval
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_attributes_port_stats_poll_intv_set(
    bf_rt_table_attributes_hdl *tbl_attr, const uint32_t poll_intv_ms);

/**
 * @brief Get Port Stat Poll Interval in Millisecond in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] poll_intvl_ms    Poll interval
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_attributes_port_stats_poll_intv_get(
    bf_rt_table_attributes_hdl *tbl_attr, uint32_t *poll_intv_ms);

/**
 * @brief Set Meter Byte Count Adjust in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] byte_count_adj    Number of adjust bytes for meter tables
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_attributes_meter_byte_count_adjust_set(
    bf_rt_table_attributes_hdl *tbl_attr, const int byte_count_adj);

/**
 * @brief Get Meter Byte Count Adjust in the Attributes Object
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] byte_count_adj   Number of adjust bytes for meter tables
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_attributes_meter_byte_count_adjust_get(
    bf_rt_table_attributes_hdl *tbl_attr, int *byte_count_adj);

/**
 * @brief Set Selector Update Notification Callback
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[in] enable            Flag to enable selector update notifications
 * @param[in] session           Session handle
 * @param[in] callback_fn       Callback on Selector table update
 * @param[in] cookie            User cookie
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_attributes_selector_table_update_cb_set(
    bf_rt_table_attributes_hdl *tbl_attr,
    const bool enable,
    const bf_rt_session_hdl *session,
    const bf_rt_selector_table_update_cb callback,
    const void *cookie);

/**
 * @brief Get Selector Update Notification Callback
 *
 * @param[in] tbl_attr          Table attribute object handle
 * @param[out] enable           Enable Flag
 * @param[out] session          Session handle
 * @param[out] callback_fn      Callback fn set for Selector table update
 * @param[out] cookie           User cookie
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_attributes_selector_table_update_cb_get(
    const bf_rt_table_attributes_hdl *tbl_attr,
    bool *enable,
    bf_rt_session_hdl **session,
    bf_rt_selector_table_update_cb *callback,
    void **cookie);

#ifdef __cplusplus
}
#endif

#endif  // _BF_RT_TABLE_ATTRIBUTES_H

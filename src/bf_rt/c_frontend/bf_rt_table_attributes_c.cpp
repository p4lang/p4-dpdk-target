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

#include "bf_rt/bf_rt_table_attributes.hpp"
#include <bf_rt/bf_rt_table_attributes.h>
#include <bf_rt_common/bf_rt_table_attributes_impl.hpp>
#include <bf_rt_common/bf_rt_utils.hpp>
#include "bf_rt_state_c.hpp"

bf_status_t bf_rt_attributes_idle_table_poll_mode_set(
    bf_rt_table_attributes_hdl *tbl_attr, const bool enable) {
  auto table_attributes =
      reinterpret_cast<bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  return table_attributes->idleTablePollModeSet(enable);
}

bf_status_t bf_rt_attributes_idle_table_notify_mode_set(
    bf_rt_table_attributes_hdl *tbl_attr,
    const bool enable,
    const bf_rt_idle_tmo_expiry_cb callback,
    const uint32_t ttl_query_interval,
    const uint32_t max_ttl,
    const uint32_t min_ttl,
    const void *cookie) {
  auto table_attributes =
      reinterpret_cast<bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  return table_attributes->idleTableNotifyModeSetCFrontend(
      enable, callback, ttl_query_interval, max_ttl, min_ttl, cookie);
}

bf_status_t bf_rt_attributes_idle_table_get(
    const bf_rt_table_attributes_hdl *tbl_attr,
    bf_rt_attributes_idle_table_mode_t *mode,
    bool *enable,
    bf_rt_idle_tmo_expiry_cb *callback,
    uint32_t *ttl_query_interval,
    uint32_t *max_ttl,
    uint32_t *min_ttl,
    void **cookie) {
  auto table_attributes =
      reinterpret_cast<const bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  bfrt::TableAttributesIdleTableMode cpp_mode;
  auto bf_status = table_attributes->idleTableGetCFrontend(&cpp_mode,
                                                           enable,
                                                           callback,
                                                           ttl_query_interval,
                                                           max_ttl,
                                                           min_ttl,
                                                           cookie);
  if (bf_status != BF_SUCCESS) {
    return bf_status;
  }
  // Convert the mode from cpp enum class to c enum
  switch (cpp_mode) {
    case bfrt::TableAttributesIdleTableMode::POLL_MODE:
      *mode = BFRT_POLL_MODE;
      break;
    case bfrt::TableAttributesIdleTableMode::NOTIFY_MODE:
      *mode = BFRT_NOTIFY_MODE;
      break;
    case bfrt::TableAttributesIdleTableMode::INVALID_MODE:
      *mode = BFRT_INVALID_MODE;
      break;
    default:
      LOG_ERROR("%s:%d Invalid Idle Table Mode found set", __func__, __LINE__);
      BF_RT_ASSERT(0);
  }
  return bf_status;
}

bf_status_t bf_rt_attributes_port_status_notify_set(
    bf_rt_table_attributes_hdl *tbl_attr,
    const bool enable,
    const bf_rt_port_status_chg_cb callback,
    const void *cookie) {
  auto table_attributes =
      reinterpret_cast<bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  return table_attributes->portStatusChangeNotifSetCFrontend(
      enable, callback, cookie);
}

bf_status_t bf_rt_attributes_port_status_notify_get(
    const bf_rt_table_attributes_hdl *tbl_attr,
    bool *enable,
    bf_rt_port_status_chg_cb *callback,
    void **cookie) {
  auto table_attributes =
      reinterpret_cast<const bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  return table_attributes->portStatusChangeNotifGetCFrontend(
      enable, callback, cookie);
}

bf_status_t bf_rt_attributes_port_stats_poll_intv_set(
    bf_rt_table_attributes_hdl *tbl_attr, const uint32_t poll_intv_ms) {
  auto table_attributes =
      reinterpret_cast<bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  return table_attributes->portStatPollIntvlMsSet(poll_intv_ms);
}

bf_status_t bf_rt_attributes_port_stats_poll_intv_get(
    bf_rt_table_attributes_hdl *tbl_attr, uint32_t *poll_intv_ms) {
  auto table_attributes =
      reinterpret_cast<const bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  return table_attributes->portStatPollIntvlMsGet(poll_intv_ms);
}

bf_status_t bf_rt_attributes_entry_scope_symmetric_mode_set(
    bf_rt_table_attributes_hdl *tbl_attr, const bool symmetric_mode) {
  auto table_attributes =
      reinterpret_cast<bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  if (symmetric_mode) {
    return table_attributes->entryScopeParamsSet(
        bfrt::TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES);
  }
  return table_attributes->entryScopeParamsSet(
      bfrt::TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE);
}

bf_status_t bf_rt_attributes_entry_scope_symmetric_mode_get(
    bf_rt_table_attributes_hdl *tbl_attr, bool *is_symmetric_mode) {
  if (is_symmetric_mode == NULL) {
    return BF_INVALID_ARG;
  }
  auto table_attributes =
      reinterpret_cast<bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  bfrt::TableEntryScope mode;
  bf_status_t sts = table_attributes->entryScopeParamsGet(&mode, NULL);
  if (mode == bfrt::TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES) {
    *is_symmetric_mode = true;
  } else {
    *is_symmetric_mode = false;
  }
  return sts;
}

bf_status_t bf_rt_attributes_dyn_hashing_get(
    bf_rt_table_attributes_hdl *tbl_attr, uint32_t *alg_hdl, uint64_t *seed) {
  auto table_attributes =
      reinterpret_cast<bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  return table_attributes->dynHashingGet(alg_hdl, seed);
}

bf_status_t bf_rt_attributes_dyn_hashing_set(
    bf_rt_table_attributes_hdl *tbl_attr,
    const uint32_t alg_hdl,
    const uint64_t seed) {
  auto table_attributes =
      reinterpret_cast<bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  return table_attributes->dynHashingSet(alg_hdl, seed);
}

bf_status_t bf_rt_attributes_meter_byte_count_adjust_get(
    bf_rt_table_attributes_hdl *tbl_attr, int *byte_count_adj) {
  auto table_attributes =
      reinterpret_cast<bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  return table_attributes->meterByteCountAdjGet(byte_count_adj);
}

bf_status_t bf_rt_attributes_meter_byte_count_adjust_set(
    bf_rt_table_attributes_hdl *tbl_attr, const int byte_count_adj) {
  auto table_attributes =
      reinterpret_cast<bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  return table_attributes->meterByteCountAdjSet(byte_count_adj);
}

bf_status_t bf_rt_attributes_selector_table_update_cb_set(
    bf_rt_table_attributes_hdl *tbl_attr,
    const bool enable,
    const bf_rt_session_hdl *session,
    const bf_rt_selector_table_update_cb callback,
    const void *cookie) {
  // Get the shared_ptr for the corresponding raw pointer from the state
  auto &c_state = bfrt::bfrt_c::BfRtCFrontEndSessionState::getInstance();
  auto cpp_session = c_state.getSharedPtr(
      reinterpret_cast<const bfrt::BfRtSession *>(session));
  if (cpp_session == nullptr) {
    return BF_INVALID_ARG;
  }
  auto table_attributes =
      reinterpret_cast<bfrt::BfRtTableAttributesImpl *>(tbl_attr);
  return table_attributes->selectorUpdateCbSetCFrontend(
      enable, cpp_session, callback, cookie);
}

bf_status_t bf_rt_attributes_selector_table_update_cb_get(
    const bf_rt_table_attributes_hdl *tbl_attr,
    bool *enable,
    bf_rt_session_hdl **session,
    bf_rt_selector_table_update_cb *callback,
    void **cookie) {
  auto table_attributes =
      reinterpret_cast<const bfrt::BfRtTableAttributesImpl *>(tbl_attr);

  return table_attributes->selectorUpdateCbGetCFrontend(
      enable,
      reinterpret_cast<bfrt::BfRtSession **>(session),
      callback,
      cookie);
}

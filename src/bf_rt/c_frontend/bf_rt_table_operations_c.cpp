// SPDX-FileCopyrightText: 2021 Intel Corporation
// Copyright (C) 2021 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0

#include <bf_rt_common/bf_rt_table_operations_impl.hpp>
#include <bf_rt_common/bf_rt_utils.hpp>

bf_status_t bf_rt_operations_register_sync_set(
    bf_rt_table_operations_hdl *tbl_ops,
    const bf_rt_session_hdl *session,
    const bf_rt_target_t *dev_tgt,
    const bf_rt_register_sync_cb callback,
    const void *cookie) {
  auto table_operations =
      reinterpret_cast<bfrt::BfRtTableOperationsImpl *>(tbl_ops);
  return table_operations->registerSyncSetCFrontend(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      callback,
      cookie);
}

bf_status_t bf_rt_operations_counter_sync_set(
    bf_rt_table_operations_hdl *tbl_ops,
    const bf_rt_session_hdl *session,
    const bf_rt_target_t *dev_tgt,
    const bf_rt_counter_sync_cb callback,
    const void *cookie) {
  auto table_operations =
      reinterpret_cast<bfrt::BfRtTableOperationsImpl *>(tbl_ops);
  return table_operations->counterSyncSetCFrontend(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      callback,
      cookie);
}

bf_status_t bf_rt_operations_hit_state_update_set(
    bf_rt_table_operations_hdl *tbl_ops,
    const bf_rt_session_hdl *session,
    const bf_rt_target_t *dev_tgt,
    const bf_rt_hit_state_update_cb callback,
    const void *cookie) {
  auto table_operations =
      reinterpret_cast<bfrt::BfRtTableOperationsImpl *>(tbl_ops);
  return table_operations->hitStateUpdateSetCFrontend(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      callback,
      cookie);
}

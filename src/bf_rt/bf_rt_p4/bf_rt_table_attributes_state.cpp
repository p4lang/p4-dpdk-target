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

#include <unistd.h>

#include "bf_rt_table_attributes_state.hpp"
#include "bf_rt_p4_table_impl.hpp"
#include "bf_rt_p4_table_key_impl.hpp"
#include <bf_rt_common/bf_rt_init_impl.hpp>
#include <bf_rt_common/bf_rt_utils.hpp>

namespace bfrt {

namespace {
// This variable determines the max number of outstanding tasks in the thread
// pool queue. Everytime, before we submit tasks to the thread pool, we check
// the size of the queue in the thread pool. We only submit the task if the
// size is less than this threshold. The reason for this limit is to guard
// against the case of the worker thread/threads of the thread pool getting
// held up for some reason (think the worker thread calls the user registered
// aging cb and that cb never returns) and the queue growing indefinitely
// to a point that we run out of system memory
const size_t idle_timeout_max_queue_occupancy = 500000;

// This variable determines the maximum number of times we will try to get the
// entry from the entry_hdl before we give up and return an error. This is
// required because we don't want to hold up a worker thread indefinitely for
// processing a single task when it could rather perform other useful tasks.
const int idle_timeout_retry_limit = 100;

// This variable determines how long the worker thread will wait before
// attempting to read the entry again
const int idle_timeout_sleep_betweem_retries = 100;
}  // anonymous namespace

// match_spec is now "owned" by the bfRtIdleTmoExpiryTaskExecute.
// The bloew function is responsible to free the spec
void bfRtIdleTmoExpiryTaskExecute(
    bf_dev_id_t dev_id,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    std::shared_ptr<BfRtStateTableAttributesAging> attributes_aging_sp) {
  // Get details from age state
  auto t = attributes_aging_sp->stateTableAttributesAgingGet();
  auto enabled = std::get<0>(t);
  auto callback_cpp = std::get<1>(t);
  auto callback_c = std::get<2>(t);
  auto table = std::get<3>(t);
  auto cookie_actual = std::get<4>(t);

  // If not enabled, then the idle tmo was disabled
  // before the cb was scheduled to be called. Let's cancel
  if (!enabled) {
    LOG_WARN("%s:%d Idle tmo expiry cb was disabled. cancelling",
             __func__,
             __LINE__);
    return;
  }
  // we do not care about action_spec and action hdl
  // Warning: if you want to use these, allocate them to
  // avoid deep problems
  auto max_data_bytes = table->getMaxdataSz();
  pipe_action_spec_t pipe_action_spec;
  std::memset(&pipe_action_spec, 0, sizeof(pipe_action_spec_t));
  std::vector<uint8_t> action_data_bits_vec(max_data_bytes);
  pipe_action_spec.act_data.action_data_bits = action_data_bits_vec.data();

  // get the match_spec from the entry hdl and pipe_mgr API
  auto pipe_mgr = PipeMgrIntf::getInstance();
  // construct a BfRtTableKey object from the pipe_match_spec
  std::unique_ptr<BfRtTableKey> newKey;
  auto status = table->keyAllocate(&newKey);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d Failed to allocate key", __func__, __LINE__);
    return;
  }
  // dynamic downcast to BfRtMatchActionKey
  auto matchKey = dynamic_cast<BfRtMatchActionKey *>(newKey.get());
  if (matchKey == nullptr) {
    LOG_ERROR(
        "%s:%d The key allocated wasn't matchActionKey", __func__, __LINE__);
    return;
  }
  matchKey->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  pipe_mgr->pipeMgrMatchSpecFree(pipe_match_spec);
  // call the registered callback from the state and send the cookie
  // from the state
  bf_rt_target_t dev_tgt{0};
  dev_tgt.dev_id = dev_id;
  // The below are just defaults. They don't have any meaning for the table
  // entry as such
  dev_tgt.direction = BF_DEV_DIR_ALL;
  dev_tgt.prsr_id = PIPE_MGR_PVS_PARSER_ALL;

  // Pipe-id of the received is retrieved based on the following
  // 1. If the table is symmetric, the pipe-id is PIPE_ALL
  // 2. If the table is asymmetric or user-defined scope, we decode the pipe
  // from the pipe-mgr entry handle

  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(
      dev_tgt.dev_id, table->programNameGet());
  if (device_state == nullptr) {
    LOG_ERROR("%s:%d ERROR device state for device id %d, program %s not found",
              __func__,
              __LINE__,
              dev_tgt.dev_id,
              table->programNameGet().c_str());
    BF_RT_DBGCHK(0);
    return;
  }
  // Get ENTRY_SCOPE of the table
  auto attributes_state =
      device_state->attributesState.getObjState(table->table_id_get());
  auto entry_scope = attributes_state->getEntryScope();

  if (entry_scope == TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES) {
    dev_tgt.pipe_id = BF_DEV_PIPE_ALL;
  } else {
    dev_tgt.pipe_id = pipe_mgr->pipeGetHdlPipe(entry_hdl);
  }

  if (callback_cpp) {
    callback_cpp(dev_tgt, std::move(matchKey), cookie_actual);
  }
  if (callback_c) {
    newKey.release();
    callback_c(&dev_tgt,
               reinterpret_cast<bf_rt_table_key_hdl *>(matchKey),
               cookie_actual);
  }

  // No need to delete match_mask_bits and match_value_bits since they are
  // being destroyed by ~BfRtMatchActionKey
  // TODO: Find a better ownership model here
  return;
}

void bfRtIdleTmoExpiryInternalCb(bf_dev_id_t dev_id,
                                 pipe_mat_ent_hdl_t entry_hdl,
                                 pipe_tbl_match_spec_t *match_spec_allocated,
                                 void *cookie) {
  // Get the age state from the cookie
  auto attributes_aging_state =
      static_cast<BfRtStateTableAttributesAging *>(cookie);

  // Get details from age state
  auto t = attributes_aging_state->stateTableAttributesAgingGet();
  auto enabled = std::get<0>(t);
  auto callback_cpp = std::get<1>(t);
  auto callback_c = std::get<2>(t);
  auto table = std::get<3>(t);
  auto cookie_actual = std::get<4>(t);

  BfRtTable::TableType table_type;
  table->tableTypeGet(&table_type);

  // This memory is allocated here but de-allocated by the thread in the thread
  // pool
  // Here we need to create a new object instead of just enqueuing the
  // retrieved aging_state because, there is a chance that that state
  // might get cleared in the context of some other thread (currently
  // this is device wide global state per table and not thread safe).
  // If it is indeed cleared, then the pointer that we would have enqueued
  // would no longer be valid and we might end up dereferencing an invalid/
  // null ptr
  // TODO Make the global device state thread safe or pull in all the state
  // within the table objects. Then we don't need to create a new object
  // everytime and we can simply enqueue the pointer that we retrieve
  auto attributes_aging_sp = std::make_shared<BfRtStateTableAttributesAging>(
      enabled, callback_cpp, callback_c, table, cookie_actual);

  // Retrieve the thread pool for this table.
  BfRtThreadPool *idle_tmo_thread_pool = nullptr;
  switch (table_type) {
    case BfRtTable::TableType::MATCH_DIRECT:
      idle_tmo_thread_pool = (static_cast<const BfRtMatchActionTable *>(table))
                                 ->idletimeCbThreadPoolGet();
      break;
    case BfRtTable::TableType::MATCH_INDIRECT:
    case BfRtTable::TableType::MATCH_INDIRECT_SELECTOR:
      idle_tmo_thread_pool =
          (static_cast<const BfRtMatchActionIndirectTable *>(table))
              ->idletimeCbThreadPoolGet();
      break;
    default:
      LOG_ERROR(
          "%s:%d %s ERROR Invalid table type encountered for entry %d dev %d",
          __func__,
          __LINE__,
          table->table_name_get().c_str(),
          entry_hdl,
          dev_id);
      BF_RT_DBGCHK(0);
      return;
  }
  if (idle_tmo_thread_pool == nullptr) {
    // This indicates a serious error, as we should have had created a thread
    // pool for this table when we enabled idletime on this table.
    LOG_ERROR(
        "%s:%d %s ERROR Thread pool does not exist. Thus ignoring idle timeout "
        "cb for entry %d dev %d",
        __func__,
        __LINE__,
        table->table_name_get().c_str(),
        entry_hdl,
        dev_id);
    BF_RT_DBGCHK(0);
    return;
  }

  // Check the size of the queue and dont add any more tasks if the size of
  // the queue is more than the allowed threshold
  if (idle_tmo_thread_pool->getQueueSize() >=
      idle_timeout_max_queue_occupancy) {
    LOG_ERROR(
        "%s:%d %s ERROR Thread pool queue size is greater than the allowed "
        "limit of %zd, hence aborting handling of entry %d",
        __func__,
        __LINE__,
        table->table_name_get().c_str(),
        idle_timeout_max_queue_occupancy,
        entry_hdl);
    return;
  }

  // Put the task in the thread pool queue
  idle_tmo_thread_pool->submitTask(bfRtIdleTmoExpiryTaskExecute,
                                   dev_id,
                                   entry_hdl,
                                   match_spec_allocated,
                                   attributes_aging_sp);
}

pipe_status_t selUpdatePipeMgrInternalCb(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         void *cookie,
                                         pipe_sel_grp_hdl_t sel_grp_hdl,
                                         pipe_adt_ent_hdl_t adt_ent_hdl,
                                         int logical_entry_index,
                                         bool is_add) {
  auto cb_state = static_cast<BfRtStateSelUpdateCb *>(cookie);
  auto t = cb_state->stateGet();

  // auto enable = std::get<0>(t);
  auto table = std::get<1>(t);
  auto session_obj = std::get<2>(t).lock();
  auto cpp_callback_fn = std::get<3>(t);
  auto c_callback_fn = std::get<4>(t);
  auto original_cookie = std::get<5>(t);

  bf_rt_id_t sel_grp_id = 0;
  bf_rt_id_t act_mbr_id = 0;

  if (session_obj == nullptr) {
    LOG_ERROR(
        "%s:%d ERROR Session which was passed during selector update callback "
        "registration has been destroyed",
        __func__,
        __LINE__);
    return PIPE_SUCCESS;
  }

  if (sess_hdl != session_obj->sessHandleGet()) {
    LOG_ERROR("%s:%d ERROR session handle was changed from %d to %d",
              __func__,
              __LINE__,
              sess_hdl,
              session_obj->sessHandleGet());
    return PIPE_SESSION_NOT_FOUND;
  }

  // From sel_grp_hdl get sel grp id
  auto sel_table = static_cast<const BfRtSelectorTable *>(table);
  auto bf_status =
      sel_table->getGrpIdFromHndl(dev_tgt.device_id, sel_grp_hdl, &sel_grp_id);
  if (bf_status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Group Id not found for group hndl %d",
              __func__,
              __LINE__,
              sel_table->table_name_get().c_str(),
              sel_grp_hdl);
    // We always return SUCCESS to pipe-mgr in the context of this callback
    return PIPE_SUCCESS;
  }

  // From adt_ent_hdl get, act_mbr_id
  bf_status = sel_table->getActMbrIdFromHndl(
      dev_tgt.device_id, adt_ent_hdl, &act_mbr_id);

  if (bf_status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Action member id not found for entry hndl %d",
              __func__,
              __LINE__,
              sel_table->table_name_get().c_str(),
              adt_ent_hdl);
    return PIPE_SUCCESS;
  }

  const bf_rt_target_t bf_rt_tgt = {
      dev_tgt.device_id,
      dev_tgt.dev_pipe_id,
      BF_DEV_DIR_INGRESS /* direction : don't care */,
      0xff /* parser_id : don't care */};

  // Now, invoke the user registered callback
  if (cpp_callback_fn) {
    cpp_callback_fn(session_obj,
                    bf_rt_tgt,
                    original_cookie,
                    sel_grp_id,
                    act_mbr_id,
                    logical_entry_index,
                    is_add);
  }
  if (c_callback_fn) {
    c_callback_fn(
        reinterpret_cast<const bf_rt_session_hdl *>(session_obj.get()),
        &bf_rt_tgt,
        original_cookie,
        sel_grp_id,
        act_mbr_id,
        logical_entry_index,
        is_add);
  }
  return PIPE_SUCCESS;
}

void BfRtStateTableAttributesAging::stateTableAttributesAgingSet(
    bool enabled,
    BfRtIdleTmoExpiryCb callback_cpp,
    bf_rt_idle_tmo_expiry_cb callback_c,
    const BfRtTableObj *table,
    void *cookie) {
  enabled_ = enabled;
  callback_cpp_ = callback_cpp;
  callback_c_ = callback_c;
  table_ = const_cast<BfRtTableObj *>(table);
  cookie_ = cookie;
}

void BfRtStateTableAttributesAging::stateTableAttributesAgingReset() {
  enabled_ = false;
  callback_cpp_ = nullptr;
  callback_c_ = nullptr;
  table_ = nullptr;
  cookie_ = nullptr;
}

std::tuple<bool,
           BfRtIdleTmoExpiryCb,
           bf_rt_idle_tmo_expiry_cb,
           const BfRtTableObj *,
           void *>
BfRtStateTableAttributesAging::stateTableAttributesAgingGet() {
  return std::make_tuple(enabled_, callback_cpp_, callback_c_, table_, cookie_);
}

void BfRtStateSelUpdateCb::reset() {
  enable_ = false;
  sel_table_ = nullptr;
  cookie_ = nullptr;
  return;
}

std::tuple<bool,
           const BfRtTableObj *,
           const std::weak_ptr<BfRtSession>,
           selUpdateCb,
           bf_rt_selector_table_update_cb,
           const void *>
BfRtStateSelUpdateCb::stateGet() {
  return std::make_tuple(enable_,
                         sel_table_,
                         session_obj_,
                         cpp_callback_fn_,
                         c_callback_fn_,
                         cookie_);
}

}  // bfrt

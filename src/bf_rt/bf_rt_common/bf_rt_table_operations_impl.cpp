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

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#ifdef __cplusplus
}

#endif
#include <bf_rt/bf_rt_session.hpp>
#include <bf_rt/bf_rt_table_operations.h>
#include <bf_rt/bf_rt_table_operations.hpp>
#include "bf_rt_init_impl.hpp"
#include "bf_rt_table_operations_impl.hpp"
#include "bf_rt_table_operations_state.hpp"
#include "bf_rt_pipe_mgr_intf.hpp"

namespace bfrt {
namespace {

template <typename T, typename U>
void bfRtOperationsInternalCallback(bf_dev_id_t device_id, void *cb_cookie) {
  // get information from the state_object sent in the cookie
  auto operations_state =
      static_cast<BfRtStateTableOperations<T, U> *>(cb_cookie);
  auto t = operations_state->stateTableOperationsGet();
  auto callback_cpp = std::get<0>(t);
  auto callback_c = std::get<1>(t);
  auto cookie = std::get<2>(t);
  if (!callback_cpp && !callback_c) {
    LOG_WARN("%s:%d No Callback function found", __func__, __LINE__);
    return;
  }
  if (callback_cpp) {
    // TODO figure out how to populate pipe
    bf_rt_target_t dev_tgt = {device_id, BF_DEV_PIPE_ALL};
    callback_cpp(dev_tgt, cookie);
  } else {
    bf_rt_target_t dev_tgt = {device_id, BF_DEV_PIPE_ALL};
    callback_c(&dev_tgt, cookie);
  }
}

}  // anonymous namespace

TableOperationsType BfRtTableOperationsImpl::getType(
    const std::string &op_name, const BfRtTable::TableType &object_type) {
  TableOperationsType ret_val = TableOperationsType::INVALID;
  switch (object_type) {
    case BfRtTable::TableType::REGISTER:
      if (op_name == "Sync") {
        ret_val = TableOperationsType::REGISTER_SYNC;
      }
      break;
    case BfRtTable::TableType::COUNTER:
      if (op_name == "Sync") {
        ret_val = TableOperationsType::COUNTER_SYNC;
      }
      break;
    case BfRtTable::TableType::MATCH_DIRECT:
    case BfRtTable::TableType::MATCH_INDIRECT:
    case BfRtTable::TableType::MATCH_INDIRECT_SELECTOR:
      if (op_name == "SyncRegisters") {
        ret_val = TableOperationsType::REGISTER_SYNC;
      } else if (op_name == "SyncCounters") {
        ret_val = TableOperationsType::COUNTER_SYNC;
      } else if (op_name == "UpdateHitState") {
        ret_val = TableOperationsType::HIT_STATUS_UPDATE;
      }
      break;
    default:
      break;
  }
  return ret_val;
}

// Works on Match Action tables and register tables only
bf_status_t BfRtTableOperationsImpl::registerSyncSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtRegisterSyncCb &callback,
    const void *cookie) {
  return registerSyncInternal(session, dev_tgt, callback, nullptr, cookie);
}

// Works on Match Action tables and counter tables only
bf_status_t BfRtTableOperationsImpl::counterSyncSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtCounterSyncCb &callback,
    const void *cookie) {
  return counterSyncInternal(session, dev_tgt, callback, nullptr, cookie);
}

bf_status_t BfRtTableOperationsImpl::hitStateUpdateSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtHitStateUpdateCb &callback,
    const void *cookie) {
  return hitStateUpdateInternal(session, dev_tgt, callback, nullptr, cookie);
}

// Works on Match Action tables and register tables only
bf_status_t BfRtTableOperationsImpl::registerSyncSetCFrontend(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const bf_rt_register_sync_cb &callback,
    const void *cookie) {
  return registerSyncInternal(session, dev_tgt, nullptr, callback, cookie);
}

// Works on Match Action tables and counter tables only
bf_status_t BfRtTableOperationsImpl::counterSyncSetCFrontend(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const bf_rt_counter_sync_cb &callback,
    const void *cookie) {
  return counterSyncInternal(session, dev_tgt, nullptr, callback, cookie);
}

bf_status_t BfRtTableOperationsImpl::hitStateUpdateSetCFrontend(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const bf_rt_hit_state_update_cb &callback,
    const void *cookie) {
  return hitStateUpdateInternal(session, dev_tgt, nullptr, callback, cookie);
}

bf_status_t BfRtTableOperationsImpl::registerSyncInternal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtRegisterSyncCb &callback,
    const bf_rt_register_sync_cb &callback_c,
    const void *cookie) {
  session_ = &session;
  dev_tgt_ = dev_tgt;
  register_cpp_ = callback;
  register_c_ = callback_c;
  cookie_ = cookie;
  return BF_SUCCESS;
}

bf_status_t BfRtTableOperationsImpl::counterSyncInternal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtCounterSyncCb &callback,
    const bf_rt_counter_sync_cb &callback_c,
    const void *cookie) {
  session_ = &session;
  dev_tgt_ = dev_tgt;
  counter_cpp_ = callback;
  counter_c_ = callback_c;
  cookie_ = cookie;
  return BF_SUCCESS;
}

bf_status_t BfRtTableOperationsImpl::hitStateUpdateInternal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtHitStateUpdateCb &callback,
    const bf_rt_hit_state_update_cb &callback_c,
    const void *cookie) {
  session_ = &session;
  dev_tgt_ = dev_tgt;
  hit_state_cpp_ = callback;
  hit_state_c_ = callback_c;
  cookie_ = cookie;
  return BF_SUCCESS;
}

bf_status_t BfRtTableOperationsImpl::registerSyncExecute() const {
  // If table is MAT then call direct sync else if register
  // table then indirect sync
  BfRtTable::TableType table_type;
  table_->tableTypeGet(&table_type);
  bool direct = false;
  if (table_type == BfRtTable::TableType::MATCH_DIRECT ||
      table_type == BfRtTable::TableType::MATCH_INDIRECT ||
      table_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR) {
    direct = true;
  } else if (table_type == BfRtTable::TableType::REGISTER) {
    direct = false;
  } else {
    LOG_ERROR("%s:%d %s Invalid function for this table",
              __func__,
              __LINE__,
              table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  // get the state and update it
  auto prog_name = table_->programNameGet();
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt_.dev_id, prog_name);
  if (device_state == nullptr) {
    LOG_ERROR("%s:%d device state not found for device %d",
              __func__,
              __LINE__,
              dev_tgt_.dev_id);
    return BF_OBJECT_NOT_FOUND;
  }
  auto register_state =
      device_state->operationsRegisterState.getObjState(table_->table_id_get());

  register_state->stateTableOperationsSet(register_cpp_, register_c_, cookie_);

  // If no cb has been registered, then dont register BFRT cb with
  // pipe_mgr
  bool register_cb = true;
  if (!register_cpp_ && !register_c_) {
    register_cb = false;
  }

  // call pipe_mgr function to register internal counter-callback
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t dev_tgt_pipe_mgr = {dev_tgt_.dev_id, dev_tgt_.pipe_id};
  if (direct) {
    return pipeMgr->pipeStfulDirectDatabaseSync(
        session_->sessHandleGet(),
        dev_tgt_pipe_mgr,
        table_->tablePipeHandleGet(),
        register_cb == true
            ? bfRtOperationsInternalCallback<BfRtRegisterSyncCb,
                                             bf_rt_register_sync_cb>
            : nullptr,
        register_state.get());
  } else {
    return pipeMgr->pipeStfulDatabaseSync(
        session_->sessHandleGet(),
        dev_tgt_pipe_mgr,
        table_->tablePipeHandleGet(),
        register_cb == true
            ? bfRtOperationsInternalCallback<BfRtRegisterSyncCb,
                                             bf_rt_register_sync_cb>
            : nullptr,
        register_state.get());
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTableOperationsImpl::counterSyncExecute() const {
  // If table is MAT then call direct sync else if counter
  // table then idirect sync
  BfRtTable::TableType table_type;
  table_->tableTypeGet(&table_type);
  bool direct = false;
  if (table_type == BfRtTable::TableType::MATCH_DIRECT ||
      table_type == BfRtTable::TableType::MATCH_INDIRECT ||
      table_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR) {
    direct = true;
  } else if (table_type == BfRtTable::TableType::COUNTER) {
    direct = false;
  } else {
    LOG_ERROR("%s:%d %s Invalid function for this table",
              __func__,
              __LINE__,
              table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  // get the state and update it
  auto prog_name = table_->programNameGet();
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt_.dev_id, prog_name);
  if (device_state == nullptr) {
    LOG_ERROR("%s:%d device state not found for device %d",
              __func__,
              __LINE__,
              dev_tgt_.dev_id);
    return BF_OBJECT_NOT_FOUND;
  }
  auto counter_state =
      device_state->operationsCounterState.getObjState(table_->table_id_get());

  counter_state->stateTableOperationsSet(counter_cpp_, counter_c_, cookie_);

  // If no cb has been registered, then dont register BFRT cb with
  // pipe_mgr
  bool register_cb = true;
  if (!counter_cpp_ && !counter_c_) {
    register_cb = false;
  }
  // call pipe_mgr function to counter internal counter-callback
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t dev_tgt_pipe_mgr = {dev_tgt_.dev_id, dev_tgt_.pipe_id};
  if (direct) {
    return pipeMgr->pipeMgrDirectStatDatabaseSync(
        session_->sessHandleGet(),
        dev_tgt_pipe_mgr,
        table_->tablePipeHandleGet(),
        register_cb == true
            ? bfRtOperationsInternalCallback<BfRtCounterSyncCb,
                                             bf_rt_counter_sync_cb>
            : nullptr,
        counter_state.get());
  } else {
    return pipeMgr->pipeMgrStatDatabaseSync(
        session_->sessHandleGet(),
        dev_tgt_pipe_mgr,
        table_->tablePipeHandleGet(),
        register_cb == true
            ? bfRtOperationsInternalCallback<BfRtCounterSyncCb,
                                             bf_rt_counter_sync_cb>
            : nullptr,
        counter_state.get());
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTableOperationsImpl::hitStateUpdateExecute() const {
  // If table is MAT then call else error
  BfRtTable::TableType table_type;
  table_->tableTypeGet(&table_type);
  if (!(table_type == BfRtTable::TableType::MATCH_DIRECT ||
        table_type == BfRtTable::TableType::MATCH_INDIRECT ||
        table_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR)) {
    LOG_ERROR("%s:%d %s Invalid function for this table",
              __func__,
              __LINE__,
              table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  // get the state and update it
  auto prog_name = table_->programNameGet();
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt_.dev_id, prog_name);
  if (device_state == nullptr) {
    LOG_ERROR("%s:%d device state not found for device %d",
              __func__,
              __LINE__,
              dev_tgt_.dev_id);
    return BF_OBJECT_NOT_FOUND;
  }
  auto hit_state_state =
      device_state->operationsHitStateState.getObjState(table_->table_id_get());

  hit_state_state->stateTableOperationsSet(
      hit_state_cpp_, hit_state_c_, cookie_);

  // call pipe_mgr function to hitState internal hitState-callback
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t dev_tgt_pipe_mgr = {dev_tgt_.dev_id, dev_tgt_.pipe_id};
  return pipeMgr->pipeMgrIdleTimeUpdateHitState(
      session_->sessHandleGet(),
      dev_tgt_pipe_mgr.device_id,
      table_->tablePipeHandleGet(),
      bfRtOperationsInternalCallback<BfRtHitStateUpdateCb,
                                     bf_rt_hit_state_update_cb>,
      hit_state_state.get());
}

}  // bfrt

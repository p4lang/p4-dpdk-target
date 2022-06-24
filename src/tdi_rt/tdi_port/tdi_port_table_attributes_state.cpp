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

#include "tdi_port_table_attributes_state.hpp"
#include "tdi_port_table_key_impl.hpp"
namespace tdi {
int PortStatusChgInternalCb(bf_dev_id_t dev_id,
                                int dev_port,
                                bool port_up,
                                void *cookie) {
  // get callback and cookie_actual
  auto attributes_state = static_cast<StateTableAttributesPort *>(cookie);
  auto t = attributes_state->stateTableAttributesPortGet();
  auto enabled = std::get<0>(t);
  auto callback = std::get<1>(t);
  auto callback_c = std::get<2>(t);
  auto table = std::get<3>(t);
  auto cookie_actual = std::get<4>(t);
  // If not enabled, then the port status change was disabled
  // before the cb was scheduled to be called. Let's cancel
  if (!enabled) {
    LOG_WARN("%s:%d Port status change cb was disabled. cancelling",
             __func__,
             __LINE__);
    return -1;
  }

  // call callback
  std::unique_ptr<TableKey> newKey;
  auto status = table->keyAllocate(&newKey);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d Failed to allocate key", __func__, __LINE__);
    return -1;
  }
  // cast to PortCfgTableKey
  auto matchKey = dynamic_cast<PortCfgTableKey *>(newKey.get());
  if (matchKey == nullptr) {
    LOG_ERROR(
        "%s:%d The key allocated wasn't matchActionKey", __func__, __LINE__);
    return -1;
  }
  matchKey->setId(dev_port);  // set dev port
  if (callback != nullptr) {
    callback(dev_id, matchKey, port_up, cookie_actual);
  }
  if (callback_c != nullptr) {
    newKey.release();
    tdi_target_t target;
    target.dev_id = dev_id;
    callback_c(&target,
               reinterpret_cast<tdi_table_key_hdl *>(matchKey),
               port_up,
               cookie_actual);
  }
  return 0;
}

void StateTableAttributesPort::stateTableAttributesPortSet(
    bool enabled,
    PortStatusNotifCb callback_fn,
    tdi_port_status_chg_cb callback_fn_c,
    const TableObj *table,
    void *cookie) {
  std::lock_guard<std::mutex> lock(state_lock);
  enabled_ = enabled;
  callback_ = callback_fn;
  callback_c_ = callback_fn_c;
  if (callback_ && callback_c_) {
    LOG_ERROR(
        "%s:%d Not allow to set c and c++ callback functions at the same time",
        __func__,
        __LINE__);
  }
  table_ = const_cast<TableObj *>(table);
  cookie_ = cookie;
}

void StateTableAttributesPort::stateTableAttributesPortReset() {
  enabled_ = false;
  callback_ = nullptr;
  callback_c_ = nullptr;
  table_ = nullptr;
  cookie_ = nullptr;
}

std::tuple<bool,
           PortStatusNotifCb,
           tdi_port_status_chg_cb,
           const TableObj *,
           void *>
StateTableAttributesPort::stateTableAttributesPortGet() {
  std::lock_guard<std::mutex> lock(state_lock);
  return std::make_tuple(enabled_, callback_, callback_c_, table_, cookie_);
}

}  // tdi

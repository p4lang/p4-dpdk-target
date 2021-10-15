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
#ifndef _BF_RT_TBL_ATTRIBUTES_STATE_HPP
#define _BF_RT_TBL_ATTRIBUTES_STATE_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include "pipe_mgr/pipe_mgr_intf.h"
#include <bf_rt/bf_rt_common.h>
#ifdef __cplusplus
}
#endif

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <tuple>

#include <bf_rt/bf_rt_table_attributes.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>
namespace bfrt {

class BfRtTableObj;

void bfRtIdleTmoExpiryInternalCb(bf_dev_id_t dev_id,
                                 pipe_mat_ent_hdl_t entry_hdl,
                                 pipe_tbl_match_spec_t *match_spec_allocated,
                                 void *cookie);

pipe_status_t selUpdatePipeMgrInternalCb(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         void *cookie,
                                         pipe_sel_grp_hdl_t sel_grp_hdl,
                                         pipe_adt_ent_hdl_t adt_ent_hdl,
                                         int logical_table_index,
                                         bool is_add);

class BfRtStateTableAttributesAging {
 public:
  BfRtStateTableAttributesAging()
      : enabled_(false),
        callback_c_((bf_rt_idle_tmo_expiry_cb) nullptr),
        table_(nullptr),
        cookie_(nullptr){};
  BfRtStateTableAttributesAging(bool enable,
                                BfRtIdleTmoExpiryCb callback_cpp,
                                bf_rt_idle_tmo_expiry_cb callback_c,
                                const BfRtTableObj *table,
                                void *cookie)
      : enabled_(enable),
        callback_cpp_(callback_cpp),
        callback_c_(callback_c),
        table_(table),
        cookie_(cookie){};
  void stateTableAttributesAgingSet(bool enabled,
                                    BfRtIdleTmoExpiryCb callback_cpp,
                                    bf_rt_idle_tmo_expiry_cb callback_c,
                                    const BfRtTableObj *table,
                                    void *cookie);
  void stateTableAttributesAgingReset();
  std::tuple<bool,
             BfRtIdleTmoExpiryCb,
             bf_rt_idle_tmo_expiry_cb,
             const BfRtTableObj *,
             void *>
  stateTableAttributesAgingGet();

 private:
  bool enabled_;
  BfRtIdleTmoExpiryCb callback_cpp_;
  bf_rt_idle_tmo_expiry_cb callback_c_;
  const BfRtTableObj *table_;
  void *cookie_;
};

class BfRtStateSelUpdateCb {
 public:
  BfRtStateSelUpdateCb()
      : enable_(false),
        sel_table_(nullptr),
        c_callback_fn_((bf_rt_selector_table_update_cb) nullptr),
        cookie_(nullptr){};
  BfRtStateSelUpdateCb(bool enable,
                       const BfRtTableObj *sel_table,
                       std::weak_ptr<BfRtSession> session_obj,
                       const selUpdateCb &cpp_callback_fn,
                       const bf_rt_selector_table_update_cb &c_callback_fn,
                       const void *cookie)
      : enable_(enable),
        sel_table_(sel_table),
        session_obj_(session_obj),
        cpp_callback_fn_(cpp_callback_fn),
        c_callback_fn_(c_callback_fn),
        cookie_(cookie){};

  void reset();

  std::tuple<bool,
             const BfRtTableObj *,
             const std::weak_ptr<BfRtSession>,
             selUpdateCb,
             bf_rt_selector_table_update_cb,
             const void *>
  stateGet();

 private:
  bool enable_;
  const BfRtTableObj *sel_table_;
  std::weak_ptr<BfRtSession> session_obj_;
  selUpdateCb cpp_callback_fn_;
  bf_rt_selector_table_update_cb c_callback_fn_;
  const void *cookie_;
};

class BfRtStateTableAttributes {
 public:
  BfRtStateTableAttributes(bf_rt_id_t id)
      : table_id_{id},
        entry_scope(TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES){};
  BfRtStateTableAttributesAging &getAgingAttributeObj() {
    std::lock_guard<std::mutex> lock(state_lock);
    return aging_attributes;
  }

  BfRtStateSelUpdateCb &getSelUpdateCbObj() {
    std::lock_guard<std::mutex> lock(state_lock);
    return sel_update_cb;
  }

  void setAgingAttributeObj(const BfRtStateTableAttributesAging &aging) {
    std::lock_guard<std::mutex> lock(state_lock);
    aging_attributes = aging;
  }

  void setSelUpdateCbObj(const BfRtStateSelUpdateCb &sel_update_cb_) {
    std::lock_guard<std::mutex> lock(state_lock);
    sel_update_cb = sel_update_cb_;
  }

  void getSelUpdateCbObj(BfRtStateSelUpdateCb *sel_update_cb_) {
    std::lock_guard<std::mutex> lock(state_lock);
    *sel_update_cb_ = sel_update_cb;
  }

  void resetSelUpdateCbObj() {
    std::lock_guard<std::mutex> lock(state_lock);
    sel_update_cb.reset();
  }

  void agingAttributesReset() {
    std::lock_guard<std::mutex> lock(state_lock);
    aging_attributes.stateTableAttributesAgingReset();
  }

  void setEntryScope(const TableEntryScope &scope) {
    std::lock_guard<std::mutex> lock(state_lock);
    entry_scope = scope;
  }

  TableEntryScope getEntryScope() {
    std::lock_guard<std::mutex> lock(state_lock);
    return entry_scope;
  }

 private:
  bf_rt_id_t table_id_;
  std::mutex state_lock;
  BfRtStateTableAttributesAging aging_attributes;
  BfRtStateSelUpdateCb sel_update_cb;
  TableEntryScope entry_scope;
};
}  // bfrt
#endif  // _BF_RT_TBL_ATTRIBUTES_STATE_HPP

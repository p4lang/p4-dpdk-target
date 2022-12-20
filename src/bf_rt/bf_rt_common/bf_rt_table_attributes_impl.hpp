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
#ifndef _BF_RT_TABLE_ATTRIBUTES_IMPL_HPP
#define _BF_RT_TABLE_ATTRIBUTES_IMPL_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#ifdef __cplusplus
}
#endif

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <array>
#include <unordered_map>
#include <functional>

#include <bf_rt/bf_rt_table_attributes.hpp>
#include <bf_rt/bf_rt_table_attributes.h>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_key.hpp>
#include "bf_rt_pipe_mgr_intf.hpp"

namespace bfrt {

class BfRtTableObj;

class BfRtTableEntryScopeArgumentsImpl : public BfRtTableEntryScopeArguments {
 public:
  BfRtTableEntryScopeArgumentsImpl(const std::bitset<32> &val);
  BfRtTableEntryScopeArgumentsImpl(
      const std::array<std::bitset<8>, 4> &val_arr);
  bf_status_t setValue(const std::bitset<32> &val);
  bf_status_t setValue(const std::array<std::bitset<8>, 4> &val_arr);
  bf_status_t getValue(std::bitset<32> *val) const;
  bf_status_t getValue(std::array<std::bitset<8>, 4> *val_arr) const;

 private:
  // scope_value_ is a 32 bit unsigned int. The least significant byte (byte 0)
  // of which indicates the pipes present in scope 0, byte 1 indicates the
  // pipes in scope 1, and so on. Thus we can have a maximum of 4 scope. Thus
  // if the scope_value_ is set to 0xc03, pipes 0 and 1 are in scope 0 and
  // pipes 2 and 3 are in scope 1
  uint32_t scope_value_;
};

// This class is responsible for handling everything related to idle params
// and is called by the parent class (BfRtTableAttributesImpl)
class BfRtTableAttributesIdleTable {
 public:
  BfRtTableAttributesIdleTable();
  bf_status_t idleTablePollModeSet(const bool &enable);
  bf_status_t idleTableNotifyModeSet(const bool &enable,
                                     const BfRtIdleTmoExpiryCb &callback,
                                     const bf_rt_idle_tmo_expiry_cb &callback_c,
                                     const uint32_t &ttl_query_interval,
                                     const uint32_t &max_ttl,
                                     const uint32_t &min_ttl,
                                     const void *cookie);

  bf_status_t idleTableGet(TableAttributesIdleTableMode *mode,
                           bool *enable,
                           BfRtIdleTmoExpiryCb *callback,
                           bf_rt_idle_tmo_expiry_cb *callback_c,
                           uint32_t *ttl_query_interval,
                           uint32_t *max_ttl,
                           uint32_t *min_ttl,
                           void **cookie) const;

  const pipe_idle_time_params_t &getTtlParamsInternal() const {
    return idle_time_param_;
  };
  const BfRtIdleTmoExpiryCb &getCallback() const { return callback_cpp_; };
  const bf_rt_idle_tmo_expiry_cb &getCallbackC() const { return callback_c_; };
  const bool &getEnabled() const { return enable_; };
  bf_status_t idleTableModeSet(const TableAttributesIdleTableMode &table_type);
  TableAttributesIdleTableMode idleTableModeGet();
  void idleTableAllParamsClear();

 private:
  bool enable_;
  BfRtIdleTmoExpiryCb callback_cpp_{nullptr};
  bf_rt_idle_tmo_expiry_cb callback_c_{nullptr};
  pipe_idle_time_params_t idle_time_param_;
};

// This class is responsible for handling everything related to entry scope
// and is called by the parent class (BfRtTableAttributesImpl)
class BfRtTableAttributesEntryScope {
 public:
  BfRtTableAttributesEntryScope();
  bf_status_t entryScopeArgumentsAllocate(
      std::unique_ptr<BfRtTableEntryScopeArguments> *scope_args_ret) const;
  bf_status_t entryScopeParamsSet(
      const TableEntryScope &entry_scope,
      const BfRtTableEntryScopeArguments &scope_args);
  bf_status_t entryScopeParamsSet(const TableEntryScope &entry_scope);
  bf_status_t entryScopeParamsGet(
      TableEntryScope *entry_scope,
      BfRtTableEntryScopeArguments *scope_args) const;

  bf_status_t gressScopeParamsSet(const TableGressScope &gress_scope) {
    gress_scope_ = gress_scope;
    return BF_SUCCESS;
  };
  bf_status_t gressScopeParamsGet(TableGressScope *gress_scope) const {
    *gress_scope = gress_scope_;
    return BF_SUCCESS;
  };

  bf_status_t prsrScopeParamsSet(const TablePrsrScope &prsr_scope,
                                 const GressTarget &prsr_gress);
  bf_status_t prsrScopeParamsGet(TablePrsrScope *prsr_scope,
                                 GressTarget *prsr_gress) const;
  void entryScopeAllParamsClear();

 private:
  TableEntryScope entry_scope_{TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES};
  TableGressScope gress_scope_{TableGressScope::GRESS_SCOPE_ALL_GRESS};
  TablePrsrScope prsr_scope_{TablePrsrScope::PRSR_SCOPE_ALL_PRSRS_IN_PIPE};
  // Property arguments
  uint32_t property_args_value_{0};
  uint32_t prsr_args_value_{0};
};

// This class is responsible for handling everything related to dynamic hashing,
// algorithms and seed,
// and is called by the parent class (BfRtTableAttributesImpl)
class BfRtTableAttributesDynHashing {
 public:
  BfRtTableAttributesDynHashing();

  void dynHashingAllParamsClear();
  bf_status_t dynHashingParamsSet(const uint32_t &alg_hdl,
                                  const uint64_t &seed);
  bf_status_t dynHashingParamsGet(uint32_t *alg_hdl, uint64_t *seed) const;

 private:
  uint32_t alg_hdl_;
  uint64_t seed_;
};

// This class is responsible for handling everything related to meter byte count
// adjust,
// and is called by the parent class (BfRtTableAttributesImpl)
class BfRtTableAttributesMeterByteCountAdj {
 public:
  BfRtTableAttributesMeterByteCountAdj();

  void byteCountAdjClear();
  bf_status_t byteCountAdjSet(const int &byte_count_adj);
  bf_status_t byteCountAdjGet(int *byte_count_adj) const;

 private:
  int byte_count_adj_;
};

// This class is responsible for handling everything related to port status
// change callback function
// and is called by the parent class (BfRtTableAttributesImpl)
class BfRtTableAttributePortStatusChangeReg {
 public:
  BfRtTableAttributePortStatusChangeReg();

  bf_status_t portStatusChgCbSet(const bool enable,
                                 const BfRtPortStatusNotifCb &cb,
                                 const bf_rt_port_status_chg_cb &cb_c,
                                 const void *cookie);
  bf_status_t portStatusChgCbGet(bool *enable,
                                 BfRtPortStatusNotifCb *cb,
                                 bf_rt_port_status_chg_cb *cb_c,
                                 void **cookie) const;
  void portStatusChgParamsClear();

 private:
  bool enable_;
  BfRtPortStatusNotifCb callback_func_;
  bf_rt_port_status_chg_cb callback_func_c_;
  void *client_data_;
};

// This class is responsible for handling everything related to port stat poll
// intvl
// and is called by the parent class (BfRtTableAttributesImpl)
class BfRtTableAttributesPortStatPollIntvl {
 public:
  BfRtTableAttributesPortStatPollIntvl();

  bf_status_t portStatPollIntvlParamSet(const uint32_t &poll_intvl);
  bf_status_t portStatPollIntvlParamGet(uint32_t *poll_intvl) const;
  void portStatPollIntvlParamClear();

 private:
  uint32_t intvl_;
};

class BfRtTableAttributesSelectorUpdateCallback {
 public:
  BfRtTableAttributesSelectorUpdateCallback();

  void paramSet(const bool &enable,
                const std::shared_ptr<BfRtSession> session,
                const selUpdateCb &cpp_callback_fn,
                const bf_rt_selector_table_update_cb &c_callback_fn,
                const void *cookie);

  void paramGet(bool *enable,
                BfRtSession **session,
                selUpdateCb *cpp_callback_fn,
                bf_rt_selector_table_update_cb *c_callback_fn,
                void **cookie) const;

  void paramsClear();

  void paramSetInternal(const std::tuple<bool,
                                         std::weak_ptr<BfRtSession>,
                                         selUpdateCb,
                                         bf_rt_selector_table_update_cb,
                                         const void *> &t);

  std::tuple<bool,
             std::weak_ptr<BfRtSession>,
             selUpdateCb,
             bf_rt_selector_table_update_cb,
             const void *>
  paramGetInternal() const;

 private:
  bool enable_{false};
  std::weak_ptr<BfRtSession> session_;
  selUpdateCb cpp_callback_fn_{nullptr};
  bf_rt_selector_table_update_cb c_callback_fn_{nullptr};
  const void *cookie_{nullptr};
};

// This class is the implementation of the BfRtTableAttributes interface
// and is internally composed of multiple classes (components) with
// each sub class being responsible for one attribute type
class BfRtTableAttributesImpl : public BfRtTableAttributes {
 public:
  BfRtTableAttributesImpl(const BfRtTableObj *table,
                          const TableAttributesType &attr_type)
      : table_(table), attr_type_(attr_type) {}
  BfRtTableAttributesImpl(const BfRtTableObj *table,
                          const TableAttributesType &attr_type,
                          const TableAttributesIdleTableMode &idle_table_type);

  bf_status_t idleTablePollModeSet(const bool &enable);
  bf_status_t idleTableNotifyModeSet(const bool &enable,
                                     const BfRtIdleTmoExpiryCb &callback,
                                     const uint32_t &ttl_query_interval,
                                     const uint32_t &max_ttl,
                                     const uint32_t &min_ttl,
                                     const void *cookie);

  bf_status_t idleTableGet(TableAttributesIdleTableMode *mode,
                           bool *enable,
                           BfRtIdleTmoExpiryCb *callback,
                           uint32_t *ttl_query_interval,
                           uint32_t *max_ttl,
                           uint32_t *min_ttl,
                           void **cookie) const;

  bf_status_t entryScopeArgumentsAllocate(
      std::unique_ptr<BfRtTableEntryScopeArguments> *scope_args_ret) const;
  bf_status_t entryScopeParamsSet(
      const TableEntryScope &entry_scope,
      const BfRtTableEntryScopeArguments &scope_args);
  bf_status_t entryScopeParamsSet(const TableEntryScope &entry_scope);
  bf_status_t entryScopeParamsGet(
      TableEntryScope *entry_scope,
      BfRtTableEntryScopeArguments *scope_args) const;

  bf_status_t entryScopeParamsSet(const TableGressScope &gress,
                                  const TableEntryScope &pipe,
                                  const BfRtTableEntryScopeArguments &pipe_args,
                                  const TablePrsrScope &prsr,
                                  const GressTarget &prsr_args);
  bf_status_t entryScopeParamsGet(TableGressScope *gress,
                                  TableEntryScope *pipe,
                                  BfRtTableEntryScopeArguments *pipe_args,
                                  TablePrsrScope *prsr,
                                  GressTarget *prsr_gress) const;

  bf_status_t dynHashingSet(const uint32_t &alg_hdl, const uint64_t &seed);

  bf_status_t dynHashingGet(uint32_t *alg_hdl, uint64_t *seed) const;

  bf_status_t meterByteCountAdjSet(const int &byte_count);
  bf_status_t meterByteCountAdjGet(int *byte_count) const;
  bf_status_t portStatusChangeNotifSet(
      const bool &enable,
      const BfRtPortStatusNotifCb &callback,
      const bf_rt_port_status_chg_cb &callback_c,
      const void *cookie);

  bf_status_t portStatusChangeNotifGet(bool *enable,
                                       BfRtPortStatusNotifCb *callback,
                                       bf_rt_port_status_chg_cb *callback_c,
                                       void **cookie) const;

  bf_status_t portStatPollIntvlMsSet(const uint32_t &poll_intvl_ms);

  bf_status_t portStatPollIntvlMsGet(uint32_t *poll_intvl_ms) const;

  bf_status_t selectorUpdateCbSet(const bool &enable,
                                  const std::shared_ptr<BfRtSession> session,
                                  const selUpdateCb &callback_fn,
                                  const void *cookie) override final;

  bf_status_t selectorUpdateCbGet(bool *enable,
                                  BfRtSession **session,
                                  selUpdateCb *callback_fn,
                                  void **cookie) const override final;

  // Hidden functions
  TableAttributesType getAttributeType() const { return attr_type_; }
  bf_status_t resetAttributeType(const TableAttributesType &attr);
  bf_status_t resetAttributeType(const TableAttributesType &attr,
                                 const TableAttributesIdleTableMode &idle_mode);

  const pipe_idle_time_params_t &getIdleTableTtlParamsInternal() const {
    return idle_table_.getTtlParamsInternal();
  };
  const BfRtIdleTmoExpiryCb &getIdleTableCallback() const {
    return idle_table_.getCallback();
  };
  const bf_rt_idle_tmo_expiry_cb &getIdleTableCallbackC() const {
    return idle_table_.getCallbackC();
  };
  bool getIdleTableEnabled() const { return idle_table_.getEnabled(); };
  bf_status_t idleTableModeSet(const TableAttributesIdleTableMode &table_type) {
    return idle_table_.idleTableModeSet(table_type);
  }
  TableAttributesIdleTableMode idleTableModeGet() {
    return idle_table_.idleTableModeGet();
  }
  bf_status_t idleTableNotifyModeSetCFrontend(
      const bool &enable,
      bf_rt_idle_tmo_expiry_cb callback_c,
      const uint32_t &ttl_query_interval,
      const uint32_t &max_ttl,
      const uint32_t &min_ttl,
      const void *cookie);
  bf_status_t idleTableGetCFrontend(TableAttributesIdleTableMode *mode,
                                    bool *enable,
                                    bf_rt_idle_tmo_expiry_cb *callback_c,
                                    uint32_t *ttl_query_interval,
                                    uint32_t *max_ttl,
                                    uint32_t *min_ttl,
                                    void **cookie) const;
  bf_status_t portStatusChangeNotifSetCFrontend(
      const bool &enable,
      bf_rt_port_status_chg_cb callback_c,
      const void *cookie);

  bf_status_t portStatusChangeNotifGetCFrontend(
      bool *enable, bf_rt_port_status_chg_cb *callback_c, void **cookie) const;

  void selectorUpdateCbInternalSet(
      const std::tuple<bool,
                       std::weak_ptr<BfRtSession>,
                       selUpdateCb,
                       bf_rt_selector_table_update_cb,
                       const void *> &t);

  std::tuple<bool,
             std::weak_ptr<BfRtSession>,
             selUpdateCb,
             bf_rt_selector_table_update_cb,
             const void *>
  selectorUpdateCbInternalGet() const;

  bf_status_t selectorUpdateCbSetCFrontend(
      const bool &enable,
      const std::shared_ptr<BfRtSession> session,
      const bf_rt_selector_table_update_cb &callback_fn,
      const void *cookie);

  bf_status_t selectorUpdateCbGetCFrontend(
      bool *enable,
      BfRtSession **session,
      bf_rt_selector_table_update_cb *callback_fn,
      void **cookie) const;

 private:
  // backpointer to table
  const BfRtTableObj *table_;
  // Attibute Type
  TableAttributesType attr_type_;

  // Idle Params Attribute class object
  BfRtTableAttributesIdleTable idle_table_;
  // Entry Scope Attribute class object
  BfRtTableAttributesEntryScope entry_scope_;
  // Dyn Hashing class object
  BfRtTableAttributesDynHashing dyn_hashing_;
  // Meter Byte Count Adjust Attribute class object
  BfRtTableAttributesMeterByteCountAdj meter_bytecount_adj_;
  // Port Cfg class object
  BfRtTableAttributePortStatusChangeReg port_status_chg_reg_;
  // Port Stat class object
  BfRtTableAttributesPortStatPollIntvl port_stat_poll_;
  // Selector Update callback class object
  BfRtTableAttributesSelectorUpdateCallback selector_update_callback_;
  // Other Attribute class objects will go here
};

}  // bfrt

#endif  // _BF_RT_TABLE_ATTRIBUTES_IMPL_HPP

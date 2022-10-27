/*******************************************************************************
 *  INTEL CONFIDENTIAL
 *
 *  Copyright (c) 2021 Intel Corporation
 *  All Rights Reserved.
 *
 *  This software and the related documents are Intel copyrighted materials,
 *  and your use of them is governed by the express license under which they
 *  were provided to you ("License"). Unless the License provides otherwise,
 *  you may not use, modify, copy, publish, distribute, disclose or transmit
 *  this software or the related documents without Intel's prior written
 *  permission.
 *
 *  This software and the related documents are provided as is, with no express
 *  or implied warranties, other than those that are expressly stated in the
 *  License.
 ******************************************************************************/
#ifndef _TDI_TABLE_ATTRIBUTES_IMPL_HPP
#define _TDI_TABLE_ATTRIBUTES_IMPL_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <functional>

#include <tdi/common/tdi_attributes.hpp>
#include <tdi/common/c_frontend/tdi_attributes.h>
#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_table_key.hpp>

#include <tdi_rt/tdi_rt_defs.h>
#include <tdi_rt/tdi_rt_init.hpp>
#include <tdi_rt/tdi_rt_attributes.hpp>
#include <tdi_rt/c_frontend/tdi_rt_attributes.h>
#include "tdi_pipe_mgr_intf.hpp"

namespace tdi {
namespace pna {
namespace rt {

// This class is responsible for handling everything related to idle params
// and is called by the parent class (TableAttributesImpl)
class TableAttributesIdleTable {
 public:
  TableAttributesIdleTable();
  tdi_status_t idleTablePollModeSet(const bool &enable);
  tdi_status_t idleTableNotifyModeSet(const bool &enable,
                                      const IdleTmoExpiryCb &callback,
                                      const tdi_idle_tmo_expiry_cb &callback_c,
                                      const uint32_t &ttl_query_interval,
                                      const uint32_t &max_ttl,
                                      const uint32_t &min_ttl,
                                      const void *cookie);

  tdi_status_t idleTableGet(tdi_rt_attributes_idle_table_mode_e *mode,
                            bool *enable,
                            IdleTmoExpiryCb *callback,
                            tdi_idle_tmo_expiry_cb *callback_c,
                            uint32_t *ttl_query_interval,
                            uint32_t *max_ttl,
                            uint32_t *min_ttl,
                            void **cookie) const;

  const pipe_idle_time_params_t &getTtlParamsInternal() const {
    return idle_time_param_;
  };
  const IdleTmoExpiryCb &getCallback() const { return callback_cpp_; };
  const tdi_idle_tmo_expiry_cb &getCallbackC() const { return callback_c_; };
  const bool &getEnabled() const { return enable_; };
  tdi_status_t idleTableModeSet(
      const tdi_rt_attributes_idle_table_mode_e &table_type);
  tdi_rt_attributes_idle_table_mode_e idleTableModeGet();
  void idleTableAllParamsClear();

 private:
  bool enable_;
  IdleTmoExpiryCb callback_cpp_{nullptr};
  tdi_idle_tmo_expiry_cb callback_c_{nullptr};
  pipe_idle_time_params_t idle_time_param_;
};

// This class is responsible for handling everything related to crypto status
// change callback function
// and is called by the parent class (TableAttributesImpl)
class TableAttributesIpsecSadbExpireReg {
 public:
  TableAttributesIpsecSadbExpireReg();

  tdi_status_t ipsecSadbExpireCbSet(const bool enable,
                                    const IpsecSadbExpireNotifCb &cb,
                                    const tdi_ipsec_sadb_expire_cb &cb_c,
                                    const void *cookie);
  tdi_status_t ipsecSadbExpireCbGet(bool *enable,
                                    IpsecSadbExpireNotifCb *cb_cpp,
                                    tdi_ipsec_sadb_expire_cb *cb_c,
                                    void **cookie) const;
  void ipsecSadbExpireParamsClear();
  const bool &getEnabled() const { return enable_; };
  const IpsecSadbExpireNotifCb &getCallbackCpp() const { return callback_cpp_; };
  const tdi_ipsec_sadb_expire_cb &getCallbackC() const { return callback_c_; };
  const bool &getEnable() const { return enable_; };
  void *getCookie() { return cookie_; };
  void setEnable(const bool enable) { enable_ = enable; };
  void setCallbackCpp(const IpsecSadbExpireNotifCb &cb_cpp) { callback_cpp_ = cb_cpp; };
  void setCallbackC(const tdi_ipsec_sadb_expire_cb &cb_c) { callback_c_ = cb_c; };
  void setCookie(void *cookie) { cookie_ = cookie; };

  bool enable_;
  IpsecSadbExpireNotifCb callback_cpp_{nullptr};
  tdi_ipsec_sadb_expire_cb callback_c_{nullptr};
  void *cookie_;                         // client_data
};

// This class is the implementation of the TableAttributes interface
// and is internally composed of multiple classes (components) with
// each sub class being responsible for one attribute type
class TableAttributesImpl : public tdi::TableAttributes {
 public:
  TableAttributesImpl(const Table *table, const tdi_attributes_type_e &attr_type)
      : tdi::TableAttributes(table, attr_type){};
  tdi_status_t setValue(const tdi_attributes_field_type_e &type,
                        const uint64_t &value) override final;
  tdi_status_t getValue(const tdi_attributes_field_type_e &type,
                        uint64_t *value) const override;
  // Hidden functions
  tdi_status_t resetAttributeType(const tdi_attributes_type_e &attr);
  const IpsecSadbExpireNotifCb &getIpsecSadbExpireTableCallback() const {
    return ipsec_sadb_expire_.getCallbackCpp();
  };
  const tdi_ipsec_sadb_expire_cb &getIpsecSadbExpireTableCallbackC() const {
    return ipsec_sadb_expire_.getCallbackC();
  };
  bool getIpsecSadbExpireTableEnabled() { return ipsec_sadb_expire_.getEnable(); };

  // IpsecSadbExpire ipsec_sadb_expire_
  TableAttributesIpsecSadbExpireReg ipsec_sadb_expire_;
};

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  // _TDI_TABLE_ATTRIBUTES_IMPL_HPP

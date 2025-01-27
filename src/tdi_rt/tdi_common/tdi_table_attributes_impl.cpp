/*******************************************************************************
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/
#include "tdi_table_attributes_impl.hpp"
#include <iostream>
#include <string>
#include "tdi_fixed_mgr_intf.hpp"
namespace tdi {
namespace pna {
namespace rt {

#define CALLBACK_NOTIF_TEST 0

/**
 * Fixed function manager auto-notifacation Callback.
 */
tdi_status_t FixedFunctionMgrInternalCb(
		dev_target_t dev_tgt,
		tdi_rt_attributes_type_e tdi_rt_attr_type,
		void *cb_cookie,
		void *notif_data)
{
  fixed_function_notif_params_t *ff_mgr_notif_param =
	  static_cast <fixed_function_notif_params_t *>(cb_cookie);
  auto table = reinterpret_cast<const tdi::Table *>(ff_mgr_notif_param->table_);

  switch (tdi_rt_attr_type) {
    case TDI_RT_ATTRIBUTES_TYPE_IPSEC_SADB_EXPIRE_NOTIF: {

	if (ff_mgr_notif_param->enable && ff_mgr_notif_param->callback_c) {
		//invoke C callback
	}
    }
    break;
    default:
      LOG_TRACE(
          "%s:%d Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes for table %s",
          __func__,
          __LINE__,
          static_cast<int>(tdi_rt_attr_type),
          table->tableInfoGet()->nameGet().c_str());
  }

  return TDI_SUCCESS;
}

tdi_status_t TableAttributesImpl::setValue(const tdi_attributes_field_type_e &type,
                                           const uint64_t &value) {
  auto tdi_rt_attr_type =
      static_cast<tdi_rt_attributes_type_e>(this->attributeTypeGet());
  switch (tdi_rt_attr_type) {
    case TDI_RT_ATTRIBUTES_TYPE_IPSEC_SADB_EXPIRE_NOTIF: {
      auto ipsec_expire_sadb_field_type =
          static_cast<tdi_rt_attributes_ipsec_sadb_expire_table_field_type_e>(type);
      switch (ipsec_expire_sadb_field_type) {
        case TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_ENABLE:
          ipsec_sadb_expire_.enable_ = value;
          break;
        case TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_CALLBACK_CPP:
          ipsec_sadb_expire_.callback_cpp_ = reinterpret_cast<IpsecSadbExpireNotifCb>(value);
          break;
        case TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_CALLBACK_C:
          ipsec_sadb_expire_.callback_c_ = reinterpret_cast<tdi_ipsec_sadb_expire_cb>(value);
          break;
        case TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_COOKIE:
          ipsec_sadb_expire_.cookie_= reinterpret_cast<void *>(value);

#if CALLBACK_NOTIF_TEST
          // test with callback
          char test_str[]="192.168.1.10";
          ipsec_sadb_expire_.callback_c_(1,1,1,1,test_str,1,ipsec_sadb_expire_.cookie_);
          //ipsec_sadb_expire_.callback_c_c(1,1,1,1,test_str,1,(void *)ipsec_sadb_expire_.cookie_);
#endif
          break;
      }
      break;
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableGet()->tableInfoGet()->nameGet().c_str(),
          static_cast<int>(tdi_rt_attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t TableAttributesImpl::getValue(const tdi_attributes_field_type_e &type,
                                       uint64_t *value) const {
  auto tdi_rt_attr_type =
      static_cast<tdi_rt_attributes_type_e>(this->attributeTypeGet());
  switch (tdi_rt_attr_type) {
    case TDI_RT_ATTRIBUTES_TYPE_IPSEC_SADB_EXPIRE_NOTIF: {
      auto ipsec_expire_sadb_field_type =
          static_cast<tdi_rt_attributes_ipsec_sadb_expire_table_field_type_e>(type);
      switch (ipsec_expire_sadb_field_type) {
        case TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_ENABLE:
          *value = static_cast<uint64_t>(ipsec_sadb_expire_.enable_);
          break;
        case TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_CALLBACK_CPP:
          *value = reinterpret_cast<uint64_t>(ipsec_sadb_expire_.callback_cpp_);
          break;
        case TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_CALLBACK_C:
          *value = reinterpret_cast<uint64_t>(ipsec_sadb_expire_.callback_c_);
          break;
        case TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_COOKIE:
          *value = reinterpret_cast<uint64_t>(ipsec_sadb_expire_.cookie_);
#if CALLBACK_NOTIF_TEST
          // test with callback
          //ipsec_sadb_expire_.callback_c_(1,1,1,1,test_str,1,ipsec_sadb_expire_.cookie_);
          if (ipsec_sadb_expire_.callback_c_ == nullptr) {
              printf("skip the callback testing since ipsec_sadb_expire_.callback_c_ == nullptr\n");
          } else {
              char test_str[]="192.168.1.100";
              ipsec_sadb_expire_.callback_c_(1,1,1,1,test_str,1, (void *)1);
          }
#endif
          break;
      }
      break;
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableGet()->tableInfoGet()->nameGet().c_str(),
          static_cast<int>(tdi_rt_attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

TableAttributesIpsecSadbExpireReg::TableAttributesIpsecSadbExpireReg() {
  ipsecSadbExpireParamsClear();
}

tdi_status_t TableAttributesIpsecSadbExpireReg::ipsecSadbExpireCbSet(
    const bool enable,
    const IpsecSadbExpireNotifCb &cb,
    const tdi_ipsec_sadb_expire_cb &cb_c,
    const void *cookie) {
  if (cb && cb_c) {
    LOG_ERROR(
        "%s:%d Not allow to set both c and c++ callback functions at the same "
        "time",
        __func__,
        __LINE__);
    return TDI_INVALID_ARG;
  }
  enable_ = enable;
  callback_cpp_ = cb;
  callback_c_ = cb_c;
  cookie_ = const_cast<void *>(cookie);
  return TDI_SUCCESS;
}

tdi_status_t TableAttributesIpsecSadbExpireReg::ipsecSadbExpireCbGet(
    bool *enable,
    IpsecSadbExpireNotifCb *cb,
    tdi_ipsec_sadb_expire_cb *cb_c,
    void **cookie) const {
  if (cb) {
    *cb = callback_cpp_;
  }
  if (cb_c) {
    *cb_c = callback_c_;
  }
  *enable = enable_;
  if (cookie && *cookie) *cookie = cookie;
  return TDI_SUCCESS;
}

void TableAttributesIpsecSadbExpireReg::ipsecSadbExpireParamsClear() {
  enable_ = false;
  callback_c_ = nullptr;
  callback_cpp_ = nullptr;
  cookie_ = nullptr;
}

#if 0
tdi_status_t TableAttributes::resetAttributeType(
    const tdi_rt_attributes_type_e &attr) {
  switch (attr) {
    case TDI_RT_ATTRIBUTES_TYPE_IPSEC_SADB_EXPIRE_NOTIF:
      port_status_chg_reg_.ipsecSadbExpireParamsClear();
      break;
    default:
      LOG_ERROR("%s:%d Invalid Attribute Type %d",
                __func__,
                __LINE__,
                static_cast<int>(attr));
      return TDI_INVALID_ARG;
  }
  attributeTypeGet() = attr;
  return TDI_SUCCESS;
}

#endif
}  // namespace rt
}  // namespace pna
}  // namespace tdi

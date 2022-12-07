/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) 2017-2018 Barefoot Networks, Inc.

 * All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of
 * Barefoot Networks, Inc. and its suppliers, if any. The intellectual and
 * technical concepts contained herein are proprietary to Barefoot Networks,
 * Inc.
 * and its suppliers and may be covered by U.S. and Foreign Patents, patents in
 * process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material is
 * strictly forbidden unless prior written permission is obtained from
 * Barefoot Networks, Inc.
 *
 * No warranty, explicit or implicit is provided, unless granted under a
 * written agreement with Barefoot Networks, Inc.
 *
 * $Id: $
 *
 ******************************************************************************/

/** @file tdi_rt_info.hpp
 *
 *  @brief Contains TDI Table enums for rt and mappers
 *
 */
#ifndef _TDI_RT_INFO_HPP_
#define _TDI_RT_INFO_HPP_

#include <tdi/common/tdi_table.hpp>

#include <tdi/arch/pna/pna_info.hpp>

#include <tdi_rt/tdi_rt_defs.h>

#include "tdi_port_table_impl.hpp"
#include "../tdi_p4/tdi_p4_table_impl.hpp"
#include "../tdi_fixed/tdi_fixed_table_impl.hpp"
/**
 * @brief Namespace for TDI
 */
namespace tdi {

namespace tdi_json {
namespace values {
namespace rt {}  // namespace rt
}  // namespace values
}  // namespace tdi_json

namespace {
const std::map<std::string, tdi_rt_attributes_type_e> rt_attributes_type_map =
    {
        {"EntryScope", TDI_RT_ATTRIBUTES_TYPE_ENTRY_SCOPE},
        {"DynamicKeyMask", TDI_RT_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK},
        {"IdleTimeout", TDI_RT_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME},
        {"MeterByteCountAdjust", TDI_RT_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ},
        {"port_status_notif_cb", TDI_RT_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF},
        {"poll_intvl_ms", TDI_RT_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS},
        {"pre_device_config", TDI_RT_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG},
        {"SelectorUpdateCb", TDI_RT_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK},
        {"ipsec_sadb_expire_notif", TDI_RT_ATTRIBUTES_TYPE_IPSEC_SADB_EXPIRE_NOTIF},
};
const std::map<std::string, tdi_rt_operations_type_e> rt_operations_type_map =
    {
      {"SyncCounters", TDI_RT_OPERATIONS_TYPE_COUNTER_SYNC},
      {"SyncRegisters", TDI_RT_OPERATIONS_TYPE_REGISTER_SYNC},
      {"UpdateHitState", TDI_RT_OPERATIONS_TYPE_HIT_STATUS_UPDATE},
      {"Sync", TDI_RT_OPERATIONS_TYPE_SYNC},
};

const std::map<std::string, tdi_rt_table_type_e> rt_table_type_map = {
    {"MatchAction_Direct", TDI_RT_TABLE_TYPE_MATCH_DIRECT},
    {"MatchAction_Indirect", TDI_RT_TABLE_TYPE_MATCH_INDIRECT},
    {"MatchAction_Indirect_Selector",
     TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR},
    {"Action", TDI_RT_TABLE_TYPE_ACTION_PROFILE},
    {"Selector", TDI_RT_TABLE_TYPE_SELECTOR},
    {"SelectorGetMember", TDI_RT_TABLE_TYPE_SELECTOR_GET_MEMBER},
    {"Meter", TDI_RT_TABLE_TYPE_METER},
    {"Counter", TDI_RT_TABLE_TYPE_COUNTER},
    {"Register", TDI_RT_TABLE_TYPE_REGISTER},
    {"PortConfigure", TDI_RT_TABLE_TYPE_PORT_CFG},
    {"PortStat", TDI_RT_TABLE_TYPE_PORT_STAT},
    {"DevConfigure", TDI_RT_TABLE_TYPE_DEV_CFG},
    {"MatchValueLookupTable", TDI_RT_TABLE_TYPE_VALUE_LOOKUP},
    {"FixedFunctionConfig", TDI_RT_TABLE_TYPE_FIXED_FUNC},
    {"FixedFunctionState", TDI_RT_TABLE_TYPE_FIXED_FUNC_STATE}
};
}
namespace pna {
namespace rt {


class TdiInfoMapper : public tdi::pna::TdiInfoMapper {
 public:
  TdiInfoMapper() {
    // table types
    for (const auto &kv: rt_table_type_map) {
      tableEnumMapAdd(kv.first, static_cast<tdi_table_type_e>(kv.second));
    }
    for (const auto &kv : rt_attributes_type_map) {
      attributesEnumMapAdd(kv.first,
                           static_cast<tdi_attributes_type_e>(kv.second));
    }
    for (const auto &kv : rt_operations_type_map) {
      operationsEnumMapAdd(kv.first,
                           static_cast<tdi_operations_type_e>(kv.second));
    }
  }
};

/**
 * @brief Class to help create the correct Table object with the
 * help of a map. Targets should override
 */
class TableFactory : public tdi::pna::TableFactory {
 public:
  virtual std::unique_ptr<tdi::Table> makeTable(
      const TdiInfo *tdi_info,
      const tdi::TableInfo *table_info) const override {
    if (!table_info) {
      LOG_ERROR("%s:%d No table info received", __func__, __LINE__);
      return nullptr;
    }
    auto table_type = static_cast<tdi_rt_table_type_e>(table_info->tableTypeGet());
    LOG_DBG("%s:%d table info received type 0x%x name %s", __func__, __LINE__,
		    (int)table_type, table_info->nameGet().c_str());
    switch(table_type) {
      case TDI_RT_TABLE_TYPE_MATCH_DIRECT:
        return std::unique_ptr<tdi::Table>(new MatchActionDirect(tdi_info, table_info));
      case TDI_RT_TABLE_TYPE_MATCH_INDIRECT:
      case TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR:
        return std::unique_ptr<tdi::Table>(new MatchActionIndirect(tdi_info, table_info));
      case TDI_RT_TABLE_TYPE_ACTION_PROFILE:
        return std::unique_ptr<tdi::Table>(new ActionProfile(tdi_info, table_info));
      case TDI_RT_TABLE_TYPE_SELECTOR:
        return std::unique_ptr<tdi::Table>(new Selector(tdi_info, table_info));
      case TDI_RT_TABLE_TYPE_COUNTER:
        return std::unique_ptr<tdi::Table>(new CounterIndirect(tdi_info, table_info));
      case TDI_RT_TABLE_TYPE_REGISTER:
	return std::unique_ptr<tdi::Table>(new RegisterTable(tdi_info, table_info));
      case TDI_RT_TABLE_TYPE_METER:
        return std::unique_ptr<tdi::Table>(new MeterIndirect(tdi_info, table_info));
      case TDI_RT_TABLE_TYPE_PORT_CFG:
	return std::unique_ptr<tdi::Table>(new tdi::PortCfgTable(tdi_info, table_info));
      case TDI_RT_TABLE_TYPE_PORT_STAT:
        return std::unique_ptr<tdi::Table>(new tdi::PortStatTable(tdi_info, table_info));
      case TDI_RT_TABLE_TYPE_VALUE_LOOKUP:
        return std::unique_ptr<tdi::Table>(new MatchValueLookupTable(tdi_info, table_info));
      case TDI_RT_TABLE_TYPE_FIXED_FUNC:
        return std::unique_ptr<tdi::Table>(new FixedFunctionConfigTable(tdi_info, table_info));
      case TDI_RT_TABLE_TYPE_FIXED_FUNC_STATE:
        return std::unique_ptr<tdi::Table>(new FixedFunctionStateTable(tdi_info, table_info));
      default:
        LOG_DBG("%s:%d table info received for other", __func__, __LINE__);
	return nullptr;
    }
    return nullptr;
  };
};

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif

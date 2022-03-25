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

#include "tdi_rt_table.hpp"
#include "../tdi_port/dpdk/tdi_port_table_impl.hpp"
/**
 * @brief Namespace for TDI
 */
namespace tdi {

namespace tdi_json {
namespace values {
namespace rt {

}  // namespace pna
}  // namespace values
}  // namespace tdi_json

namespace {
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
    {"PortMetadata", TDI_RT_TABLE_TYPE_PORT_METADATA},
    {"PortConfigure", TDI_RT_TABLE_TYPE_PORT_CFG},
    {"PortStat", TDI_RT_TABLE_TYPE_PORT_STAT},
    {"PortHdlInfo", TDI_RT_TABLE_TYPE_PORT_HDL_INFO},
    {"PortStrInfo", TDI_RT_TABLE_TYPE_PORT_STR_INFO},
    {"PortFpIdxInfo", TDI_RT_TABLE_TYPE_PORT_FRONT_PANEL_IDX_INFO},
};
}
namespace pna {
namespace rt {


class TdiInfoMapper : public tdi::pna::TdiInfoMapper {
 public:

  TdiInfoMapper() {
    // table types
    for (const auto &kv: rt_table_type_map) {
      tableEnumMapAdd(kv.first,
                    static_cast<tdi_table_type_e>(kv.second));
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
        return std::unique_ptr<tdi::Table>(new MatchActionTable(table_info));
      case TDI_RT_TABLE_TYPE_PORT_CFG:
	LOG_DBG("%s:%d table info received for PORT_CFG", __func__, __LINE__);
        return std::unique_ptr<tdi::Table>(new tdi::PortCfgTable(table_info));
      case TDI_RT_TABLE_TYPE_PORT_STAT:
	LOG_DBG("%s:%d table info received for PORT_STAT", __func__, __LINE__);
        return std::unique_ptr<tdi::Table>(new tdi::PortStatTable(table_info));
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

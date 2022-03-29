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
#ifndef _TDI_RT_TABLE_HPP
#define _TDI_RT_TABLE_HPP

#include <tdi/common/tdi_table.hpp>

namespace tdi {
namespace pna {
namespace rt {

class MatchActionDirect : public tdi::Table {
 public:
  MatchActionDirect(const tdi::TableInfo *table_info) : tdi::Table(table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
  bool actionIdApplicable() const override { return false; };
};

class MatchActionIndirect : public tdi::Table {
 public:
  MatchActionIndirect(const tdi::TableInfo *table_info)
      : tdi::Table(table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

class ActionProfile : public tdi::Table {
 public:
  ActionProfile(const tdi::TableInfo *table_info) : tdi::Table(table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

class Selector : public tdi::Table {
 public:
  Selector(const tdi::TableInfo *table_info) : tdi::Table(table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

class CounterIndirect : public tdi::Table {
 public:
  CounterIndirect(const tdi::TableInfo *table_info) : tdi::Table(table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

class MeterIndirect : public tdi::Table {
 public:
  MeterIndirect(const tdi::TableInfo *table_info) : tdi::Table(table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

}
}
}  // tdi

#endif  //

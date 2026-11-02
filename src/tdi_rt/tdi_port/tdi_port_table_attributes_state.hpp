// SPDX-FileCopyrightText: 2021 Intel Corporation
// Copyright (C) 2021 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0
#ifndef _TDI_PORT_TBL_ATTRIBUTES_STATE_HPP
#define _TDI_PORT_TBL_ATTRIBUTES_STATE_HPP

#include <mutex>
#include <unordered_map>

#include <tdi_rt/tdi_table_attributes.hpp>
#include <tdi_rt_common/tdi_table_impl.hpp>

namespace tdi {

class Table;

int portStatusChgInternalCb(bf_dev_id_t dev_id,
                                int dev_port,
                                bool port_up,
                                void *cookie);

class StateTableAttributesPort {
 public:
  StateTableAttributesPort(tdi_id_t id) : table_id_(id){};

  void stateTableAttributesPortSet(bool enabled,
                                   PortStatusNotifCb callback_fn,
                                   tdi_port_status_chg_cb callback_c,
                                   const Table *table,
                                   void *cookie);

  void stateTableAttributesPortReset();
  std::tuple<bool,
             PortStatusNotifCb,
             tdi_port_status_chg_cb,
             const Table *,
             void *>
  stateTableAttributesPortGet();

 private:
  std::mutex state_lock;
  tdi_id_t table_id_;
  bool enabled_;
  PortStatusNotifCb callback_;
  tdi_port_status_chg_cb callback_c_;
  Table *table_;
  void *cookie_;
};

}  // tdi
#endif  // _TDI_PORT_TBL_ATTRIBUTES_STATE_HPP

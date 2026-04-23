// SPDX-FileCopyrightText: 2021 Intel Corporation
// Copyright (C) 2021 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0
extern "C" {
#include <bf_rt/bf_rt_common.h>
#include <bf_pal/bf_pal_port_intf.h>
}

/* bf_rt_includes */
#include <bf_rt/bf_rt_info.hpp>
#include "bf_rt_port_mgr_intf.hpp"

namespace bfrt {
std::unique_ptr<IPortMgrIntf> IPortMgrIntf::instance = nullptr;
std::once_flag IPortMgrIntf::m_onceFlag;

bf_status_t PortMgrIntf::portMgrPortAdd(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        struct port_attributes_t *port_attrib) {
  return bf_pal_port_add(dev_id, dev_port, port_attrib);
}

// Port Stats
bf_status_t PortMgrIntf::portMgrPortAllStatsGet(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint64_t *stats) {
  return bf_pal_port_all_stats_get(dev_id, dev_port, stats);
}

}  // namespace bfrt

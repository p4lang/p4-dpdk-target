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

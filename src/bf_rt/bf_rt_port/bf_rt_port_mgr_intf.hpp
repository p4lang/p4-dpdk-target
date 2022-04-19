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

#ifndef _BF_RT_PORT_MGR_INTERFACE_HPP
#define _BF_RT_PORT_MGR_INTERFACE_HPP

extern "C" {
#include <bf_types/bf_types.h>
#include <target-sys/bf_sal/bf_sys_mem.h>
#include <port_mgr/bf_port_if.h>
}

#include <map>
#include <iostream>
#include <mutex>
#include <memory>

namespace bfrt {

class IPortMgrIntf {
 public:
  virtual ~IPortMgrIntf() = default;
  // Library init API

  virtual bf_status_t portMgrPortAdd(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     struct port_attributes_t *port_attrib) = 0;
  virtual bf_status_t portMgrPortAllStatsGet(
      bf_dev_id_t dev_id,
      bf_dev_port_t dev_port,
      uint64_t *stats) = 0;

 protected:
  static std::unique_ptr<IPortMgrIntf> instance;
  static std::once_flag m_onceFlag;
};

class PortMgrIntf : public IPortMgrIntf {
 public:
  virtual ~PortMgrIntf() = default;
  PortMgrIntf() = default;
  static IPortMgrIntf *getInstance() {
    std::call_once(m_onceFlag, [] { instance.reset(new PortMgrIntf()); });
    return IPortMgrIntf::instance.get();
  }
  bf_status_t portMgrPortAdd(bf_dev_id_t dev_id,
                             bf_dev_port_t dev_port,
                             struct port_attributes_t *port_attrib);
  bf_status_t portMgrPortAllStatsGet(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint64_t *stats);
};
}  // namespace bfrt

#endif  // _BF_RT_PORT_MGR_INTERFACE_HPP

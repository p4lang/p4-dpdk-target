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
// bf_rt includes
#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_init.hpp>

// local includes
#include "bf_rt_init_impl.hpp"

namespace bfrt {

BfRtDevMgr *BfRtDevMgr::dev_mgr_instance = nullptr;
std::mutex BfRtDevMgr::dev_mgr_instance_mutex;

bf_status_t BfRtInit::bfRtModuleInit(bool port_mgr_skip) {
  // Just call the corresponding implementation class function
  return BfRtInitImpl::bfRtModuleInit(port_mgr_skip);
}

BfRtDevMgr::BfRtDevMgr() {
  dev_mgr_impl_ = std::unique_ptr<BfRtDevMgrImpl>(new BfRtDevMgrImpl());
}

BfRtDevMgr &BfRtDevMgr::getInstance() {
  if (dev_mgr_instance == nullptr) {
    dev_mgr_instance_mutex.lock();
    if (dev_mgr_instance == nullptr) {
      dev_mgr_instance = new BfRtDevMgr();
    }
    dev_mgr_instance_mutex.unlock();
  }
  return *(BfRtDevMgr::dev_mgr_instance);
}

bf_status_t BfRtDevMgr::bfRtInfoP4NamesGet(
    const bf_dev_id_t &dev_id,
    std::vector<std::reference_wrapper<const std::string>> &p4_names) {
  return devMgrImpl()->bfRtInfoP4NamesGet(dev_id, p4_names);
}

bf_status_t BfRtDevMgr::fixedFilePathsGet(
    const bf_dev_id_t &dev_id,
    std::vector<std::reference_wrapper<const std::string>> &fixed_file_vec) {
  return devMgrImpl()->fixedFilePathsGet(dev_id, fixed_file_vec);
}

bf_status_t BfRtDevMgr::bfRtInfoGet(const bf_dev_id_t &dev_id,
                                    const std::string &prog_name,
                                    const BfRtInfo **ret_obj) const {
  // Just call the corresponding implementation class function
  return devMgrImpl()->bfRtInfoGet(dev_id, prog_name, ret_obj);
}

bf_status_t BfRtDevMgr::bfRtDeviceIdListGet(
    std::set<bf_dev_id_t> *device_id_list) const {
  return devMgrImpl()->bfRtDeviceIdListGet(device_id_list);
}

}  // bfrt

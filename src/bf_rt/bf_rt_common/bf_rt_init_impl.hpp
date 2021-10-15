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
#ifndef _BF_RT_INIT_IMPL_HPP_
#define _BF_RT_INIT_IMPL_HPP_

extern "C" {
#include <bf_types/bf_types.h>
}

#include <string>
#include <unordered_map>
#include <functional>

#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_init.hpp>

#include "bf_rt_state.hpp"

namespace bfrt {

struct PairHash {
  template <class T1, class T2>
  bool operator()(const std::pair<T1, T2> &left,
                  const std::pair<T1, T2> &right) const {
    if (left.first < right.first) {
      return true;
    } else if (left.first == right.first) {
      if (left.second < right.second) {
        return true;
      }
    }
    return false;
  }
};

// This class holds the implementation for BfRtDevMgr class
class BfRtDevMgrImpl {
 public:
  bf_status_t bfRtInfoP4NamesGet(
      const bf_dev_id_t &dev_id,
      std::vector<std::reference_wrapper<const std::string>> &p4_names);

  bf_status_t fixedFilePathsGet(
      const bf_dev_id_t &dev_id,
      std::vector<std::reference_wrapper<const std::string>> &fixed_file_vec);

  bf_status_t bfRtInfoGet(const bf_dev_id_t &dev_id,
                          const std::string &prog_name,
                          const bfrt::BfRtInfo **) const;

  bf_status_t bfRtDeviceIdListGet(std::set<bf_dev_id_t> *device_id_list) const;

  // Hidden APIs
  bf_status_t bfRtDeviceAdd(const bf_dev_id_t &dev_id,
                            const bf_dev_family_t &dev_family,
                            const bf_device_profile_t *dont_care,
                            const bf_dev_init_mode_t warm_init_mode);

  bf_status_t bfRtDeviceRemove(const bf_dev_id_t &dev_id);

  static void bfRtDeviceConfigSet(bool port_mgr_skip) {
    port_mgr_skip_ = port_mgr_skip;
  }

  static void bfRtDeviceConfigGet(bool *port_mgr_skip) {
    *port_mgr_skip = port_mgr_skip_;
  }

  std::shared_ptr<BfRtDeviceState> static bfRtDeviceStateGet(
      const bf_dev_id_t &dev_id, const std::string &prog_name);

  static void initReservedSession() {
    // Reserve a session for bfrt
    // Here the shared pointer itself is allocated on heap. This is being done
    // so as to eliminate the dependencies of the order of destruction of
    // static objects. When static objects are allocated in the data segment,
    // they are destroyed after main exits. However the order of destruction of
    // static objects is non deterministic and hence there is a chance that
    // the static objects that the destructor of BfRtSession references might
    // already have been destroyed. This would lead to a seg fault. Thus to
    // overcome this, we are allocating this on the heap itself. As a result,
    // this will never be destroyed and the memory would simply be reclaimed
    // by the OS upon program termination. Since our intention here is to
    // simply have this object for the entire lifetime of Bf-Rt module, this
    // scheme works well
    static std::shared_ptr<BfRtSession> *temp_bfrt_session =
        new std::shared_ptr<BfRtSession>(BfRtSession::sessionCreate());
    reserved_bfrt_session = temp_bfrt_session;
  }

  static const std::shared_ptr<BfRtSession> &bfRtReservedSessionGet() {
    return (*reserved_bfrt_session);
  }

 private:
  // Map of (devid, program name) to (bfrt info object)
  std::map<std::pair<bf_dev_id_t, std::string>,
           std::unique_ptr<const BfRtInfo>,
           PairHash> bfRtInfoObjMap;
  bf_status_t duplicateEntryCheckEnable(const BfRtInfo *bfRtInfo,
                                        const bf_dev_id_t &dev_id);
  static std::map<std::pair<bf_dev_id_t, std::string>,
                  std::shared_ptr<BfRtDeviceState>,
                  PairHash> bfRtDeviceStateMap;

  // This reserved session will be used exclusively by Bf-Rt especially
  // during callbacks (aging) or other instances when Bf-Rt does not have
  // access to the session being used by the user but wants to communicate
  // with pipe mgr. This avoids Bf-Rt from unnecessarily creating and
  // destroying temporary sessions objects which can be expensive
  static std::shared_ptr<BfRtSession> *reserved_bfrt_session;

  static bool port_mgr_skip_;

  // Stores the path names for fixed table jsons
  std::map<bf_dev_id_t, std::vector<std::string>> bf_rt_json_map_;
};  // BfRtDevMgrImpl

// This class holds the implementation for BfRtInit class
class BfRtInitImpl {
 public:
  static bf_status_t bfRtModuleInit(bool port_mgr_skip);

 private:
  // Bf Rt Module registered with the DVM
  static bf_drv_client_handle_t bf_rt_drv_hdl;

  // Bf Rt Module name used while registering with DVM
  static std::string bf_rt_module_name;
};  // BfRtInitImpl

}  // bfrt

#endif  // _BF_RT_INIT_IMPL_HPP_

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
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <bf_rt/bf_rt_init.h>
#include <bf_rt/bf_rt_info.h>

#ifdef __cplusplus
}
#endif

// bf_rt includes
#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_info.hpp>

// local includes
#include <bf_rt_common/bf_rt_utils.hpp>
#include <bf_rt_common/bf_rt_pipe_mgr_intf.hpp>

bf_status_t bf_rt_info_get(const bf_dev_id_t dev_id,
                           const char *prog_name,
                           const bf_rt_info_hdl **info_hdl_ret) {
  bf_status_t sts = BF_SUCCESS;
  const bfrt::BfRtInfo *bfRtInfo = nullptr;
  std::string program_name(prog_name);
  bfrt::BfRtDevMgr &devMgrObj = bfrt::BfRtDevMgr::getInstance();
  sts = devMgrObj.bfRtInfoGet(dev_id, program_name, &bfRtInfo);
  if (sts != BF_SUCCESS) {
    return sts;
  }

  *info_hdl_ret = reinterpret_cast<const bf_rt_info_hdl *>(bfRtInfo);
  return BF_SUCCESS;
}

bf_status_t bf_rt_num_device_id_list_get(uint32_t *num) {
  std::set<bf_dev_id_t> device_id_list;
  bf_status_t sts = BF_SUCCESS;
  bfrt::BfRtDevMgr &devMgrObj = bfrt::BfRtDevMgr::getInstance();
  sts = devMgrObj.bfRtDeviceIdListGet(&device_id_list);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  *num = device_id_list.size();
  return BF_SUCCESS;
}

bf_status_t bf_rt_device_id_list_get(bf_dev_id_t *device_id_list_out) {
  std::set<bf_dev_id_t> device_id_list;
  bf_status_t sts = BF_SUCCESS;
  bfrt::BfRtDevMgr &devMgrObj = bfrt::BfRtDevMgr::getInstance();
  sts = devMgrObj.bfRtDeviceIdListGet(&device_id_list);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  int i = 0;
  for (const auto &id : device_id_list) {
    device_id_list_out[i++] = id;
  }
  return BF_SUCCESS;
}

bf_status_t bf_rt_num_p4_names_get(const bf_dev_id_t dev_id, int *num_names) {
  std::vector<std::reference_wrapper<const std::string>> p4_names;
  bfrt::BfRtDevMgr &devMgrObj = bfrt::BfRtDevMgr::getInstance();
  bf_status_t sts = devMgrObj.bfRtInfoP4NamesGet(dev_id, p4_names);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  *num_names = static_cast<int>(p4_names.size());
  return sts;
}

bf_status_t bf_rt_p4_names_get(const bf_dev_id_t dev_id,
                               const char **prog_names) {
  std::vector<std::reference_wrapper<const std::string>> p4_names;
  bfrt::BfRtDevMgr &devMgrObj = bfrt::BfRtDevMgr::getInstance();
  bf_status_t sts = devMgrObj.bfRtInfoP4NamesGet(dev_id, p4_names);

  /*
   * Iterate through the p4 names and fill the name string array.
   * We use iterators rather than auto so we can use iterator
   * arithmetic to increment our array index rather than a variable.
   * We use the reference_wrapper because we iterate through a list
   * of string references, not string values.
   */
  std::vector<std::reference_wrapper<const std::string>>::iterator it;
  for (it = p4_names.begin(); it != p4_names.end(); ++it) {
    prog_names[it - p4_names.begin()] = it->get().c_str();
  }
  return sts;
}

bf_status_t bf_rt_module_init(bool port_mgr_skip){
  return bfrt::BfRtInit::bfRtModuleInit(port_mgr_skip);
}

bf_status_t bf_rt_enable_pipeline(const bf_dev_id_t dev_id)
{
    bf_status_t sts = BF_SUCCESS;
    auto *pipeMgr = bfrt::PipeMgrIntf::getInstance();
    sts = pipeMgr->pipeMgrEnablePipeline(dev_id);
    if (sts != BF_SUCCESS)
        LOG_ERROR("%s:%d failed", __func__, __LINE__);

    return sts;
}

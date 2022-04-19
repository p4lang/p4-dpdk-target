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
#include <dirent.h>

#ifdef __cplusplus
}
#endif

#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>
#include <bf_rt/bf_rt_init.hpp>

// local includes
#include "bf_rt_init_impl.hpp"
#include "bf_rt_utils.hpp"
#include "bf_rt_pipe_mgr_intf.hpp"
#include "bf_rt_table_impl.hpp"
#include "bf_rt_info_impl.hpp"

namespace {
template <typename T>
void removeItemFromMap(T *map, const bf_dev_id_t &dev_id) {
  for (auto item = map->begin(); item != map->end();) {
    if (item->first.first == dev_id) {
      item = map->erase(item);
    } else {
      item++;
    }
  }
}
}  // Anonymous namespace

namespace bfrt {

std::map<std::pair<bf_dev_id_t, std::string>,
         std::shared_ptr<BfRtDeviceState>,
         PairHash> BfRtDevMgrImpl::bfRtDeviceStateMap;

std::string BfRtInitImpl::bf_rt_module_name = "bf_rt";
bf_drv_client_handle_t BfRtInitImpl::bf_rt_drv_hdl;

std::shared_ptr<BfRtSession> *BfRtDevMgrImpl::reserved_bfrt_session = nullptr;
bool BfRtDevMgrImpl::port_mgr_skip_ = false;

bf_status_t bf_rt_device_add(bf_dev_id_t dev_id,
                             bf_dev_family_t dev_family,
                             bf_device_profile_t *dev_profile,
                             bf_dev_init_mode_t warm_init_mode) {
  auto &dev_mgr_obj = bfrt::BfRtDevMgr::getInstance();
  return dev_mgr_obj.devMgrImpl()->bfRtDeviceAdd(
      dev_id, dev_family, dev_profile, warm_init_mode);
}

bf_status_t bf_rt_device_remove(bf_dev_id_t dev_id) {
  auto &dev_mgr_obj = bfrt::BfRtDevMgr::getInstance();
  return dev_mgr_obj.devMgrImpl()->bfRtDeviceRemove(dev_id);
}

bf_status_t BfRtInitImpl::bfRtModuleInit(bool port_mgr_skip) {
  bf_status_t sts = BF_SUCCESS;
  bf_drv_client_callbacks_t callbacks = {0};

  // Register the Device Add/ Remove functions with the DVM
  sts = bf_drv_register(bf_rt_module_name.c_str(), &bf_rt_drv_hdl);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d : Unable to register bf_rt module with DVM : %s (%d)",
              __func__,
              __LINE__,
              bf_err_str(sts),
              sts);
    return sts;
  }

  callbacks.device_add = bf_rt_device_add;
  callbacks.device_del = bf_rt_device_remove;
  sts = bf_drv_client_register_callbacks(
      bf_rt_drv_hdl, &callbacks, BF_CLIENT_PRIO_0);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d : Unable to register bf_rt callbacks with DVM : %s (%d)",
              __func__,
              __LINE__,
              bf_err_str(sts),
              sts);
    return sts;
  }

  // Initialize the reserved session for bfrt
  BfRtDevMgrImpl::bfRtDeviceConfigSet(port_mgr_skip);
  BfRtDevMgrImpl::initReservedSession();

  return sts;
}

bf_status_t BfRtDevMgrImpl::bfRtInfoP4NamesGet(
    const bf_dev_id_t &dev_id,
    std::vector<std::reference_wrapper<const std::string>> &p4_names) {
  for (auto &it : bfRtInfoObjMap) {
    if (dev_id == it.first.first) {
      p4_names.push_back(std::cref(it.first.second));
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtDevMgrImpl::fixedFilePathsGet(
    const bf_dev_id_t &dev_id,
    std::vector<std::reference_wrapper<const std::string>> &fixed_file_vec) {
  if (bf_rt_json_map_.find(dev_id) == bf_rt_json_map_.end()) {
    LOG_ERROR("%s:%d Device ID %d not found", __func__, __LINE__, dev_id);
    return BF_OBJECT_NOT_FOUND;
  }
  for (const auto &fixed_path : this->bf_rt_json_map_.at(dev_id)) {
    fixed_file_vec.push_back(std::cref(fixed_path));
  }
  return BF_SUCCESS;
}

bf_status_t BfRtDevMgrImpl::bfRtInfoGet(const bf_dev_id_t &dev_id,
                                        const std::string &prog_name,
                                        const BfRtInfo **ret_obj) const {
  if (bfRtInfoObjMap.find(std::pair<bf_dev_id_t, std::string>(
          dev_id, prog_name)) == bfRtInfoObjMap.end()) {
    LOG_ERROR("%s:%d BfRtInfo Object not found for dev : %d",
              __func__,
              __LINE__,
              dev_id);
    return BF_OBJECT_NOT_FOUND;
  }

  *ret_obj = static_cast<const bfrt::BfRtInfo *>(
      bfRtInfoObjMap.at(std::pair<bf_dev_id_t, std::string>(dev_id, prog_name))
          .get());
  return BF_SUCCESS;
}

bf_status_t BfRtDevMgrImpl::bfRtDeviceIdListGet(
    std::set<bf_dev_id_t> *device_id_list) const {
  if (device_id_list == nullptr) {
    LOG_ERROR("%s:%d Please allocate space for out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  for (const auto &pair : bfRtInfoObjMap) {
    if ((*device_id_list).find(pair.first.first) == (*device_id_list).end()) {
      (*device_id_list).insert(pair.first.first);
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtDevMgrImpl::duplicateEntryCheckEnable(
    const BfRtInfo *bfRtInfo, const bf_dev_id_t &dev_id) {
  // Iterate over all tables and turn on the duplicate entry check. This is
  // done only for match tables
  bf_status_t status = BF_SUCCESS;
  std::vector<const BfRtTable *> tables;
  status = bfRtInfo->bfrtInfoGetTables(&tables);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d ERROR in getting tables, err %d", __func__, __LINE__, status);
    return status;
  }

  auto *pipeMgr = PipeMgrIntf::getInstance();
  pipe_mgr_tbl_prop_value_t prop_value;
  prop_value.duplicate_check = PIPE_MGR_DUPLICATE_ENTRY_CHECK_ENABLE;
  pipe_mgr_tbl_prop_args_t args = {0};

  // Create a pipe-mgr Session and destroy it after we enable the duplicate
  // entry check
  auto session = BfRtSession::sessionCreate();
  if (!session) {
    LOG_ERROR("%s:%d Failed to create session", __func__, __LINE__);
    return BF_NO_SYS_RESOURCES;
  }
  auto sess_hdl = session->sessHandleGet();

  for (auto &each_table : tables) {
    const BfRtTableObj *table =
        reinterpret_cast<const BfRtTableObj *>(each_table);
    BfRtTable::TableType table_type;
    table->tableTypeGet(&table_type);
    if (table_type == BfRtTable::TableType::MATCH_DIRECT ||
        table_type == BfRtTable::TableType::MATCH_INDIRECT ||
        table_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR) {
      status = pipeMgr->pipeMgrTblSetProperty(sess_hdl,
                                              dev_id,
                                              table->tablePipeHandleGet(),
                                              PIPE_MGR_DUPLICATE_ENTRY_CHECK,
                                              prop_value,
                                              args);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Error in turning on duplicate entry check, err %d",
                  __func__,
                  __LINE__,
                  table->table_name_get().c_str(),
                  status);
        return status;
      }
    }
  }

  status = session->sessionCompleteOperations();
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d Error in completing session operations, err %d",
              __func__,
              __LINE__,
              status);
    return status;
  }
  status = session->sessionDestroy();
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d Error in destroying session, err %d",
              __func__,
              __LINE__,
              status);
    return status;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtDevMgrImpl::bfRtDeviceAdd(
    const bf_dev_id_t &dev_id,
    const bf_dev_family_t &dev_family,
    const bf_device_profile_t *dev_profile,
    const bf_dev_init_mode_t warm_init_mode) {
  LOG_DBG("%s:%d BfRt Device Add called for dev : %d : warm init mode : %d",
          __func__,
          __LINE__,
          dev_id,
          warm_init_mode);
  // Load fixed bf_rt json files.
  // Fixed bf_rt json files would be presented in both fixed bfrt info obj and
  // p4 bfrt info obj

  // Old platform suffixes tof.json and tof2.json are denied in case these files
  // are still present in the same directory.
  std::vector<std::string> excluded_suffixes = {"tof.json", "tof2.json"};
  // map of dev_id -> vector of all bf-rt.json files
  if (this->bf_rt_json_map_.find(dev_id) == this->bf_rt_json_map_.end()) {
    this->bf_rt_json_map_[dev_id] = std::vector<std::string>();
  } else {
    this->bf_rt_json_map_[dev_id].clear();
  }
  if (dev_profile->bfrt_non_p4_json_dir_path != NULL) {
    std::string bf_rt_fixed_json_path(dev_profile->bfrt_non_p4_json_dir_path);
    DIR *dir = opendir(bf_rt_fixed_json_path.c_str());
    if (dir != NULL) {
      std::string common_string("json");
      struct dirent *pdir;
      while ((pdir = readdir(dir)) != NULL) {
        std::string file_name(pdir->d_name);
        if (file_name.find(common_string) == std::string::npos) {
          continue;  // ignore non json files
        }
        bool is_excluded = false;
        for (auto &excluded_str : excluded_suffixes) {
          if (file_name.find(excluded_str) != std::string::npos) {
            // The json file name is not belonging to the current dev_family,
            // or it has an old name suffix.
            is_excluded = true;
            break;
          }
        }
        if (is_excluded) {
          continue;
        }
        // The json file is either with the current platform suffix,
        // or it has no explicit platform suffix.
        file_name.insert(0, bf_rt_fixed_json_path);
        this->bf_rt_json_map_[dev_id].push_back(file_name);
      }
      int sts = closedir(dir);
      if (0 != sts) {
        LOG_ERROR("%s:%d Trying to close the bf_rt directory %s fail, dev %d",
                  __func__,
                  __LINE__,
                  bf_rt_fixed_json_path.c_str(),
                  dev_id);
        return sts;
      }
    }
  }
  // tracks P4 programs with valid bf-rt.json of their own
  uint32_t num_valid_p4_programs = 0;

  // Find the first P4 program and track its name
  std::string first_p4_name = "";

  // Load p4 bf_rt json and context json
  for (int i = 0; i < dev_profile->num_p4_programs; i++) {
    // Initialize bf_rt_p4_json_vect to a vector containing all fixed json paths
    std::vector<std::string> bf_rt_p4_json_vect(this->bf_rt_json_map_[dev_id]);
    // If bf-rt.json doesn't exist, then continue the loop since we don't want
    // to continue making a BfRtInfo Object but still want to continue with the
    // device_add
    if (dev_profile->p4_programs[i].bfrt_json_file == nullptr) {
      LOG_TRACE(
          "%s:%d No BF-RT json file found for program %s"
          " Not adding BF-RT Info object for it",
          __func__,
          __LINE__,
          dev_profile->p4_programs[i].prog_name);
      continue;
    } else {
      num_valid_p4_programs++;
    }
    std::string prog_name(dev_profile->p4_programs[i].prog_name);
    if (i == 0) {
      // If it is the first program, track name
      first_p4_name = prog_name;
    }
    LOG_DBG("%s:%d Adding program %s on device %d",
            __func__,
            __LINE__,
            prog_name.c_str(),
            dev_id);
    if (bfRtInfoObjMap.find(std::pair<bf_dev_id_t, std::string>(
            dev_id, prog_name)) != bfRtInfoObjMap.end()) {
      LOG_ERROR(
          "%s:%d Trying to add a new BfRt Info object for the same "
          "dev : %d program %s pair",
          __func__,
          __LINE__,
          dev_id,
          dev_profile->p4_programs[i].prog_name);
      return BF_ALREADY_EXISTS;
    }

    std::vector<ProgramConfig::P4Pipeline> p4_pipelines;
    for (int j = 0; j < dev_profile->p4_programs[i].num_p4_pipelines; j++) {
      std::string profile_name =
          dev_profile->p4_programs[i].p4_pipelines[j].p4_pipeline_name;
      std::string context_json_path =
          dev_profile->p4_programs[i].p4_pipelines[j].runtime_context_file;
      std::string binary_path =
          dev_profile->p4_programs[i].p4_pipelines[j].cfg_file;

      std::vector<bf_dev_pipe_t> pipe_scope;
      if (dev_profile->p4_programs[i].p4_pipelines[j].num_pipes_in_scope == 0) {
        uint32_t num_pipes;
        PipeMgrIntf::getInstance()->pipeMgrGetNumPipelines(dev_id, &num_pipes);
        for (uint32_t k = 0; k < num_pipes; k++) {
          pipe_scope.push_back(k);
        }
      } else {
        pipe_scope.assign(
            dev_profile->p4_programs[i].p4_pipelines[j].pipe_scope,
            dev_profile->p4_programs[i].p4_pipelines[j].pipe_scope +
                dev_profile->p4_programs[i].p4_pipelines[j].num_pipes_in_scope);
      }
      p4_pipelines.emplace_back(
          profile_name, context_json_path, binary_path, pipe_scope);
    }
    bf_rt_p4_json_vect.push_back(dev_profile->p4_programs[i].bfrt_json_file);
    ProgramConfig program_config(prog_name, bf_rt_p4_json_vect, p4_pipelines);
    // Create a new BfRtInfo object and insert in the map
    bfRtInfoObjMap[std::pair<bf_dev_id_t, std::string>(dev_id, prog_name)] =
        std::move(BfRtInfoImpl::makeBfRtInfo(dev_id, program_config));
    // If BfRtInfo creation had failed for this program, then try and continue
    // the process for other programs. This way, even if none of the P4
    // bf-rt.json could be added, then we would still have $SHARED tables to
    // work with.
    if (bfRtInfoObjMap[std::pair<bf_dev_id_t, std::string>(
            dev_id, prog_name)] == nullptr) {
      continue;
    }
    // Turn on duplicate entry checks for all tables since BfRt deals with match
    // tables using keys
    bf_status_t status = duplicateEntryCheckEnable(
        bfRtInfoObjMap[std::pair<bf_dev_id_t, std::string>(dev_id, prog_name)]
            .get(),
        dev_id);
    if (status != BF_SUCCESS) {
      return status;
    }

    if (bfRtDeviceStateMap.find(std::pair<bf_dev_id_t, std::string>(
            dev_id, prog_name)) != bfRtDeviceStateMap.end()) {
      LOG_ERROR(
          "%s:%d Trying to create a new Device state object for the same dev : "
          "%d program : %s pair",
          __func__,
          __LINE__,
          dev_id,
          dev_profile->p4_programs[i].prog_name);
      return BF_ALREADY_EXISTS;
    }
    // Create device state for this (dev_id, program_name) and insert it in
    // the map
    auto this_prog_state = std::make_shared<BfRtDeviceState>(dev_id, prog_name);
    bfRtDeviceStateMap[std::pair<bf_dev_id_t, std::string>(dev_id, prog_name)] =
        this_prog_state;
  }

  if (num_valid_p4_programs == 0) {
    // If there is no p4_program, load bf_rt fixed tables only with a fixed
    // fake p4 program name "$SHARED".
    std::vector<ProgramConfig::P4Pipeline> dummy;
    std::string prog_name = "$SHARED";
    ProgramConfig program_config_fixed(
        prog_name, this->bf_rt_json_map_[dev_id], dummy);
    // As all init related msgs require a p4_program name, we make up a
    // fixed,
    // shared name "$SHARED" for all the bf_rt fixed feature.
    bfRtInfoObjMap[std::pair<bf_dev_id_t, std::string>(dev_id, prog_name)] =
        std::move(BfRtInfoImpl::makeBfRtInfo(dev_id, program_config_fixed));
    // We need to create device state for $SHARED too
    bfRtDeviceStateMap[std::pair<bf_dev_id_t, std::string>(dev_id, prog_name)] =
        std::make_shared<BfRtDeviceState>(dev_id, prog_name);
  }

  // Set the pipe mgr to always return network ordered byte data for all
  // learn data. If this is not set, pipe mgr will return hostorder data
  // for data of size <= 4 and network ordered data for data of size > 4
  auto pipe_sts =
      PipeMgrIntf::getInstance()->pipeMgrFlowLrnSetNetworkOrderDigest(dev_id,
                                                                      true);
  if (pipe_sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Unable to set learn digest network order in pipemgr for dev %d",
        __func__,
        __LINE__,
        dev_id);
  }

  return BF_SUCCESS;
}

bf_status_t BfRtDevMgrImpl::bfRtDeviceRemove(const bf_dev_id_t &dev_id) {
  LOG_DBG("%s:%d BfRt Device Remove called for dev : %d",
          __func__,
          __LINE__,
          dev_id);
  removeItemFromMap(&bfRtInfoObjMap, dev_id);
  removeItemFromMap(&bfRtDeviceStateMap, dev_id);

  return BF_SUCCESS;
}

std::shared_ptr<BfRtDeviceState> BfRtDevMgrImpl::bfRtDeviceStateGet(
    const bf_dev_id_t &dev_id, const std::string &prog_name) {
  if (bfRtDeviceStateMap.find(std::pair<bf_dev_id_t, std::string>(
          dev_id, prog_name)) == bfRtDeviceStateMap.end()) {
    // State does not exist for this <dev_id, prog_name> pair
    LOG_ERROR("%s:%d Unable to find Device state for dev_id : %d program : %s",
              __func__,
              __LINE__,
              dev_id,
              prog_name.c_str());
    return nullptr;
  }
  return bfRtDeviceStateMap.at(
      std::pair<bf_dev_id_t, std::string>(dev_id, prog_name));
}
}  // bfrt

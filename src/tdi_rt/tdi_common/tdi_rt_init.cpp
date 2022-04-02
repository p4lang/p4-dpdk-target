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
#ifdef __cplusplus
extern "C" {
#endif
#include <dirent.h>

#ifdef __cplusplus
}
#endif
// tdi includes
#include <tdi/common/tdi_utils.hpp>

// target include
#include <tdi_rt/c_frontend/tdi_rt_init.h>
#include <tdi_rt/tdi_rt_init.hpp>
#include "tdi_rt_info.hpp"
#include "tdi_context_info.hpp"
#include "tdi_pipe_mgr_intf.hpp"

namespace tdi {
namespace pna {
namespace rt {

class Device;

std::string Init::tdi_module_name = "tdi";
bf_drv_client_handle_t Init::tdi_drv_hdl;

namespace {
std::vector<std::string> tdiFixedJsonFilePathsGet(
    bf_dev_family_t dev_family, const char *non_p4_json_path) {
  std::vector<std::string> tdi_json_fixed_vec;

  // map of dev_id -> vector of all tdi.json files
  std::string tdi_fixed_json_path(non_p4_json_path);
  DIR *dir = opendir(tdi_fixed_json_path.c_str());
  if (dir != NULL) {
    std::string common_string("json");
    struct dirent *pdir;
    while ((pdir = readdir(dir)) != NULL) {
      std::string file_name(pdir->d_name);
      if (file_name.find(common_string) == std::string::npos) {
        continue;  // ignore non json files
      }
      // The json file is either with the current platform suffix,
      // or it has no explicit platform suffix.
      file_name.insert(0, tdi_fixed_json_path);
      tdi_json_fixed_vec.push_back(file_name);
    }
    int sts = closedir(dir);
    if (0 != sts) {
      LOG_ERROR("%s:%d Trying to close the tdi directory %s fail",
                __func__,
                __LINE__,
                tdi_fixed_json_path.c_str());
      return {};
    }
  }
  return tdi_json_fixed_vec;
}

std::vector<tdi::ProgramConfig> convertDevProfileToDeviceConfig(
    const bf_dev_id_t & /*dev_id*/,
    const bf_device_profile_t *dev_profile,
    const std::vector<std::string> &tdi_fixed_json_path_vec) {
  // tracks P4 programs with valid bf-rt.json of their own
  uint32_t num_valid_p4_programs = 0;

  std::vector<tdi::ProgramConfig> program_config_vec;
  for (int i = 0; i < dev_profile->num_p4_programs; i++) {
    std::string prog_name(dev_profile->p4_programs[i].prog_name);
    std::vector<std::string> tdi_p4_json_vect(tdi_fixed_json_path_vec);
    std::vector<tdi::P4Pipeline> p4_pipelines;
    // If bf-rt.json doesn't exist, then continue the loop
    if (dev_profile->p4_programs[i].bfrt_json_file == nullptr) {
      LOG_ERROR(
          "%s:%d No TDI json file found for program %s"
          " Not adding TDI Info object for it",
          __func__,
          __LINE__,
          dev_profile->p4_programs[i].prog_name);
      continue;
    } else {
      num_valid_p4_programs++;
    }
    for (int j = 0; j < dev_profile->p4_programs[i].num_p4_pipelines; j++) {
      std::string profile_name =
          dev_profile->p4_programs[i].p4_pipelines[j].p4_pipeline_name;
      std::string context_json_path =
          dev_profile->p4_programs[i].p4_pipelines[j].runtime_context_file;
      std::string binary_path =
          dev_profile->p4_programs[i].p4_pipelines[j].cfg_file;

      std::vector<uint32_t> pipe_scope;
      if (dev_profile->p4_programs[i].p4_pipelines[j].num_pipes_in_scope == 0) {
        // number of p4 programs and p4 pipelines should be both 1 in this case
        // because we expect pipe_scope to be specified in this scenario
        TDI_ASSERT(dev_profile->num_p4_programs == 1);
        TDI_ASSERT(dev_profile->p4_programs[i].num_p4_pipelines == 1);
        uint32_t num_pipes = 4;
#if 0
        PipeMgrIntf::getInstance()->pipeMgrGetNumPipelines(dev_id, &num_pipes);
#endif
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
    tdi_p4_json_vect.push_back(dev_profile->p4_programs[i].bfrt_json_file);
    program_config_vec.emplace_back(prog_name, tdi_p4_json_vect, p4_pipelines);
  }
  if (num_valid_p4_programs == 0) {
    // If there is no p4_program, load TDI fixed tables only with a fixed
    // fake p4 program name "$SHARED".
    std::vector<tdi::P4Pipeline> dummy;
    std::string prog_name = "$SHARED";
    program_config_vec.emplace_back(prog_name, tdi_fixed_json_path_vec, dummy);
  }
  return program_config_vec;
}

}  // namespace

tdi_status_t tdi_device_add(bf_dev_id_t dev_id,
                            bf_dev_family_t dev_family,
                            bf_device_profile_t *dev_profile,
                            bf_dev_init_mode_t warm_init_mode) {
  auto &dev_mgr_obj = DevMgr::getInstance();

  LOG_ERROR("%s:%d TDI Device Add called for dev : %d : warm init mode : %d",
            __func__,
            __LINE__,
            dev_id,
            warm_init_mode);
  // Load fixed tdi json files.
  // Fixed tdi json files would be present in both fixed tdi info obj and
  // p4 tdi info obj
  std::vector<std::string> tdi_fixed_json_path_vec;
  if (dev_profile->bfrt_non_p4_json_dir_path != NULL) {
    tdi_fixed_json_path_vec = tdiFixedJsonFilePathsGet(
        dev_family, dev_profile->bfrt_non_p4_json_dir_path);
  }
  return dev_mgr_obj.deviceAdd<tdi::pna::rt::Device>(
      dev_id,
      TDI_ARCH_TYPE_PNA,
      convertDevProfileToDeviceConfig(
          dev_id, dev_profile, tdi_fixed_json_path_vec),
      {},  // Figure out a way to get mgr_list through the dev_add cb
      nullptr);
}

tdi_status_t tdi_device_remove(bf_dev_id_t dev_id) {
  auto &dev_mgr_obj = tdi::DevMgr::getInstance();
  return dev_mgr_obj.deviceRemove(dev_id);
}

Device::Device(const tdi_dev_id_t &device_id,
               const tdi_arch_type_e &arch_type,
               const std::vector<tdi::ProgramConfig> &device_config,
               const std::vector<tdi_mgr_type_e> mgr_type_list,
               void *cookie)
    : tdi::pna::Device(
          device_id, arch_type, device_config, mgr_type_list, cookie) {
  // Parse tdi json for every program
  for (const auto &program_config : device_config) {
    LOG_ERROR("%s:%d parsing %s",
              __func__,
              __LINE__,
              program_config.prog_name_.c_str());
    auto tdi_info_mapper = std::unique_ptr<tdi::TdiInfoMapper>(
        new tdi::pna::rt::TdiInfoMapper());
    auto table_factory = std::unique_ptr<tdi::TableFactory>(
        new tdi::pna::rt::TableFactory());

    auto tdi_info_parser = std::unique_ptr<TdiInfoParser>(
        new TdiInfoParser(std::move(tdi_info_mapper)));
    tdi_info_parser->parseTdiInfo(program_config.tdi_info_file_paths_);
    auto tdi_info = tdi::TdiInfo::makeTdiInfo(program_config.prog_name_,
                                              std::move(tdi_info_parser),
                                              table_factory.get());
    // Parse context json
    auto status = parseContextJson(tdi_info.get(), device_id, program_config);
    if (status) {
      LOG_ERROR("%s:%d Failed to parse context.json", __func__, __LINE__);
    }
    tdi_info_map_.insert({program_config.prog_name_, std::move(tdi_info)});
  }
}

tdi_status_t Init::tdiModuleInit(
    const std::vector<tdi_mgr_type_e> /*mgr_type_list*/) {
  tdi_status_t sts = TDI_SUCCESS;
  bf_drv_client_callbacks_t callbacks = {0};

  // Register the Device Add/ Remove functions with the DVM
  sts = bf_drv_register(tdi_module_name.c_str(), &tdi_drv_hdl);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d : Unable to register tdi module with DVM : %s (%d)",
              __func__,
              __LINE__,
              bf_err_str(sts),
              sts);
    return sts;
  }

  callbacks.device_add = tdi_device_add;
  callbacks.device_del = tdi_device_remove;
  sts = bf_drv_client_register_callbacks(
      tdi_drv_hdl, &callbacks, BF_CLIENT_PRIO_0);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d : Unable to register tdi callbacks with DVM : %s (%d)",
              __func__,
              __LINE__,
              bf_err_str(sts),
              sts);
    return sts;
  }

  return sts;
}

}  // namespace rt
}  // namespace pna
}  // namespace tdi

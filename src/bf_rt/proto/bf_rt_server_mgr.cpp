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
#include <time.h>
#include <future>
#include <thread>
#include <sstream>
#include <iostream>
#include <ostream>
#include <fstream>

#ifdef BFRT_PERF_TEST
#include <gperftools/profiler.h>
#endif

#include "bf_rt_server_impl.hpp"
#include <bf_rt_common/bf_rt_utils.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>
#include <bf_rt_common/bf_rt_table_data_impl.hpp>
#include <bf_rt_common/bf_rt_cjson.hpp>

namespace bfrt {
namespace server {

BfRtServer *BfRtServer::bfrt_server_obj = nullptr;

namespace {

const std::string BF_RT_FILE_NAME = "bf-rt.json";
const std::string PIPELINE_CFG_FILE_NAME = "pipeline.spec";
const std::string CONTEXT_FILE_NAME = "context.json";

std::once_flag bfrt_server_once_flag;

/**
 * @brief Creates an entire path of directories. Errors out if
 * it is unable to create the directory path.
 * if a/b/c/d is to be created and a/b/c already exist, then d
 * will be created. if a/b/c/d exist, then also this func will
 * return success
 *
 * @param[in] mode
 * @param[in] root_path base path to put at
 * @param[in] path_to_put path to create
 *
 * @return grpc status
 */
Status createDirectory(const mode_t &mode,
                       const std::string &root_path,
                       const std::string &path_to_put) {
  struct stat st;
  std::string path = path_to_put;
  for (std::string::iterator iter = path.begin(); iter != path.end();) {
    std::string::iterator new_iter = std::find(iter, path.end(), '/');
    std::string new_path =
        root_path + "/" + std::string(path.begin(), new_iter);

    if (stat(new_path.c_str(), &st) == 0) {
      // stat succeeded. clearly something fishy. The path exists. In case of
      // a directory log and move on to next. Error out in case of a file
      if (S_ISDIR(st.st_mode)) {
        LOG_DBG("%s:%d path %s already exists",
                __func__,
                __LINE__,
                new_path.c_str());
      } else {
        check_and_return(BF_INVALID_ARG,
                         "%s:%d path %s not a dir",
                         __func__,
                         __LINE__,
                         new_path.c_str());
      }
    } else {
      if (mkdir(new_path.c_str(), mode) != 0 && errno != EEXIST) {
        check_and_return(BF_INVALID_ARG,
                         "%s:%d cannot create dir [ %s ]",
                         __func__,
                         __LINE__,
                         new_path.c_str());
      }
    }

    iter = new_iter;
    if (new_iter != path.end()) {
      iter++;
    }
  }
  return Status();
}

/**
 * @brief Creates a file at file_path and puts content. All previous
 * data is truncated
 *
 * @return grpc Status
 */
Status createFile(const std::string &file_path, const std::string &content) {
  if (file_path.empty() || file_path.back() == '/') {
    check_and_return(BF_INVALID_ARG, "Trying to make an invalid file");
  }
  auto dir_path = file_path.substr(0, file_path.find_last_of("/"));
  Status grpc_status = Status();
  // Check if the path is absolute or relative. Root-path will change
  // accordingly
  if (file_path[0] == '/') {
    grpc_status = createDirectory(0777, "/", dir_path);
  } else {
    grpc_status = createDirectory(0777, ".", dir_path);
  }
  grpc_check_and_return(grpc_status, "Failed to create directory");

  std::ofstream file_stream;
  file_stream.open(file_path, std::ofstream::out | std::ofstream::trunc);
  file_stream << content;
  file_stream.close();
  return Status();
}

std::string readAndSquashBfrtJsons(
    const std::vector<std::string> &bfrt_json_paths) {
  Cjson final_file;
  Cjson current_file;
  bool is_it_first = true;
  std::vector<Cjson> bfrt_cjson_vec;
  for (const auto &bfrt_path : bfrt_json_paths) {
    // Read this file and make a cjson object
    LOG_DBG("%s:%d Reading %s file", __func__, __LINE__, bfrt_path.c_str());
    std::ifstream bf_rt_stream(bfrt_path);
    if (bf_rt_stream.fail()) {
      LOG_ERROR(
          "Error opening file %s errno:%s", bfrt_path.c_str(), strerror(errno));
      return "";
    }
    std::string bfrt_content((std::istreambuf_iterator<char>(bf_rt_stream)),
                             std::istreambuf_iterator<char>());
    bf_rt_stream.close();
    current_file = Cjson::createCjsonFromFile(bfrt_content);

    if (!current_file.exists()) {
      LOG_ERROR("%s:%d Failed to read file %s into a json object",
                __func__,
                __LINE__,
                bfrt_path.c_str());
      BF_RT_DBGCHK(0);
      continue;
    }
    bfrt_cjson_vec.push_back(current_file);
  }
  for (const auto &top_cjson : bfrt_cjson_vec) {
    // Concatenate all tables of all json files to the first one's cjson. Hence
    // skipping the first one
    if (!is_it_first) {
      for (const auto &table_cjson : top_cjson["tables"].getCjsonChildVec()) {
        final_file["tables"] += *table_cjson;
      }
    } else {
      final_file = top_cjson;
    }
    is_it_first = false;
  }
  // If learn_filter exists in the last file, then add it too. Only the last
  // file is expected to have learn_filters since it is the p4_file.
  // current_file
  // will contain the last file
  current_file = bfrt_cjson_vec.back();
  if (current_file["learn_filters"].exists()) {
    final_file.addObject("learn_filters", current_file["learn_filters"]);
  }
  // the outstream operator is overloaded for cjson. use it to make a string
  std::stringstream ss;
  ss << final_file;
  return ss.str();
}

}  // anonymous namespace

std::unique_ptr<StreamWrapper> StreamWrapper::make(
    StreamChannelReaderWriter *stream) {
  return std::unique_ptr<StreamWrapper>(new StreamWrapper(stream));
}

// The comparator operator helps with creating a set
bool DeviceForwardingConfig::ProgramConfig::P4Pipeline::operator<(
    const DeviceForwardingConfig::ProgramConfig::P4Pipeline &anotherPipeline)
    const {
  std::vector<int> v_intersection;
  std::set_intersection(pipe_scope_.begin(),
                        pipe_scope_.end(),
                        anotherPipeline.pipe_scope_.begin(),
                        anotherPipeline.pipe_scope_.end(),
                        std::back_inserter(v_intersection));
  // if there is a profile_name clash OR the intersection set of the 2
  // pipe_scopes is not empty, then it is deemed to be equal. So returning false
  // from here.
  // The way operator< is used in creating a set is that both a < b and b < a
  // are checked. If it is false for both, then it is equal.
  if (profile_name_ == anotherPipeline.profile_name_ ||
      !v_intersection.empty()) {
    return false;
  }
  return (*pipe_scope_.begin()) < (*anotherPipeline.pipe_scope_.begin());
}

// The comparator operator helps with creating a set
bool DeviceForwardingConfig::ProgramConfig::operator<(
    const DeviceForwardingConfig::ProgramConfig &anotherProgram) const {
  return prog_name_ < anotherProgram.prog_name_;
}

DeviceForwardingConfig::DeviceForwardingConfig(
    const bfrt_proto::SetForwardingPipelineConfigRequest &request)
    : device_id_(request.device_id()) {
  std::string install_path;
  if (request.base_path().size() > 0) {
    install_path = request.base_path();
    if (install_path.back() == '/') {
      install_path.pop_back();
    }
  } else {
    install_path = ".";
  }
  // In case there is no config present like in COMMIT msg, then this
  // section won't be entered. So we are good.
  // For each program
  for (const auto &config : request.config()) {
    const auto &p4_name = config.p4_name();
    if (p4_name.size() >= PROG_NAME_LEN) {
      LOG_ERROR("%s:%d Program name %s is longer than allowed = %d",
                __func__,
                __LINE__,
                p4_name.c_str(),
                PROG_NAME_LEN - 1);
      throw fwdConfigError();
    }
    // Construct Program path
    std::string program_path = install_path + "/" + p4_name;
    // Get the bf-rt.json path
    std::string bf_rt_path = program_path + "/" + BF_RT_FILE_NAME;

    std::set<DeviceForwardingConfig::ProgramConfig::P4Pipeline> p4_pipelines;
    for (const auto &profile : config.profiles()) {
      if (profile.profile_name().size() >= PROG_NAME_LEN) {
        LOG_ERROR("%s:%d Profile name %s is longer than allowed = %d",
                  __func__,
                  __LINE__,
                  profile.profile_name().c_str(),
                  PROG_NAME_LEN - 1);
        throw fwdConfigError();
      }
      // config json path
      std::string config_path =
          program_path + "/" + profile.profile_name() + "/" + CONTEXT_FILE_NAME;
      // binary path
      std::string binary_path =
          program_path + "/" + profile.profile_name() + "/" + PIPELINE_CFG_FILE_NAME;
      // pipe_scope
      if (profile.pipe_scope().size() > MAX_P4_PIPELINES) {
        LOG_ERROR("%s:%d pipe_scope array size %d is longer than allowed = %d",
                  __func__,
                  __LINE__,
                  profile.pipe_scope().size(),
                  MAX_P4_PIPELINES);
        throw fwdConfigError();
      }
      auto ret_val = p4_pipelines.emplace(
          profile.profile_name(),
          config_path,
          binary_path,
          std::set<bf_dev_pipe_t>(
              {profile.pipe_scope().begin(), profile.pipe_scope().end()}));
      if (!ret_val.second) {
        LOG_ERROR("%s:%d Cannot insert profile since overlapping pipe_scopes",
                  __func__,
                  __LINE__);
        throw fwdConfigError();
      }
    }
    auto ret_val = programs_.emplace(p4_name, bf_rt_path, p4_pipelines);
    if (!ret_val.second) {
      LOG_ERROR("%s:%d Cannot insert program since this name already exists",
                __func__,
                __LINE__);
      throw fwdConfigError();
    }
  }
  // Get all the fixed json paths and convert the ref vector to string vec
  std::vector<std::reference_wrapper<const std::string>> fixed_path_vec;
  auto bf_status = BfRtDevMgr::getInstance().fixedFilePathsGet(
      request.device_id(), fixed_path_vec);
  if (BF_SUCCESS != bf_status) {
    LOG_ERROR("%s:%d Cannot insert program since file path is not found",
              __func__,
              __LINE__);
    throw fwdConfigError();
  }
  for (const auto &file_path : fixed_path_vec) {
    this->fixed_info_path_vec_.push_back(file_path.get());
  }
}

DeviceForwardingConfig::DeviceForwardingConfig(const bf_dev_id_t &device_id)
    : device_id_(device_id) {
  // Get the device from the device mgr
  std::vector<std::reference_wrapper<const std::string>> p4_names;
  auto bf_status =
      BfRtDevMgr::getInstance().bfRtInfoP4NamesGet(device_id, p4_names);
  // For each program
  for (const auto &p4_name : p4_names) {
    const BfRtInfo *bfrt_info;
    bf_status =
        BfRtDevMgr::getInstance().bfRtInfoGet(device_id, p4_name, &bfrt_info);

    // Get the bf-rt json path
    std::vector<std::reference_wrapper<const std::string>> bfrt_info_path_vec;
    bf_status = bfrt_info->bfRtInfoFilePathGet(&bfrt_info_path_vec);

    PipelineProfInfoVec pipe_info;
    bf_status = bfrt_info->bfRtInfoPipelineInfoGet(&pipe_info);
    std::set<DeviceForwardingConfig::ProgramConfig::P4Pipeline> p4_pipelines;

    std::vector<std::reference_wrapper<const std::string>> context_file_name_v;
    bf_status = bfrt_info->contextFilePathGet(&context_file_name_v);

    std::vector<std::reference_wrapper<const std::string>> binary_file_name_v;
    bf_status = bfrt_info->binaryFilePathGet(&binary_file_name_v);

    uint32_t i = 0;
    for (const auto &profile_pair : pipe_info) {
      // Construct program path
      // Assumption -> this loop won't be entered by $SHARED since
      // pipe_info_vec size will be 0
      const std::string &profile_name = profile_pair.first;
      // config json path
      std::string config_path = context_file_name_v[i].get();
      // binary path
      std::string binary_path = binary_file_name_v[i].get();
      // pipe_scope
      p4_pipelines.emplace(
          profile_name,
          config_path,
          binary_path,
          std::set<bf_dev_pipe_t>({profile_pair.second.get().begin(),
                                   profile_pair.second.get().end()}));
      i++;
    }
    programs_.emplace(p4_name, bfrt_info_path_vec.back().get(), p4_pipelines);
  }
  // Get all the fixed json paths and convert the ref vector to string vec
  std::vector<std::reference_wrapper<const std::string>> fixed_path_vec;
  bf_status =
      BfRtDevMgr::getInstance().fixedFilePathsGet(device_id, fixed_path_vec);
  if (BF_SUCCESS != bf_status) {
    LOG_ERROR("%s:%d Cannot insert program since file path is not found",
              __func__,
              __LINE__);
    throw fwdConfigError();
  }
  for (const auto &file_path : fixed_path_vec) {
    this->fixed_info_path_vec_.push_back(file_path.get());
  }
}

Status DeviceForwardingConfig::getDeviceProfile(
    bf_device_profile_t *device_profile) {
  device_profile->num_p4_programs = programs_.size();
  int program_index = 0;
  for (const auto &program : programs_) {
    std::snprintf(device_profile->p4_programs[program_index].prog_name,
                  PROG_NAME_LEN,
                  "%s",
                  program.prog_name_.c_str());
    device_profile->p4_programs[program_index].bfrt_json_file =
        const_cast<char *>(program.p4_bfrt_path_.c_str());
    device_profile->p4_programs[program_index].num_p4_pipelines =
        program.p4_pipelines_.size();
    int profile_index = 0;
    for (const auto &profile : program.p4_pipelines_) {
      auto device_pipe_prof = &(device_profile->p4_programs[program_index]
                                    .p4_pipelines[profile_index]);
      if (profile.profile_name_.size() >= PROG_NAME_LEN) {
        check_and_return(BF_INVALID_ARG,
                         "Profile name %s is longer than allowed = %d",
                         profile.profile_name_.c_str(),
                         PROG_NAME_LEN - 1);
      }
      std::snprintf(device_pipe_prof->p4_pipeline_name,
                    PROG_NAME_LEN,
                    "%s",
                    profile.profile_name_.c_str());
      device_pipe_prof->cfg_file =
          const_cast<char *>(profile.binary_path_.c_str());
      device_pipe_prof->runtime_context_file =
          const_cast<char *>(profile.context_json_path_.c_str());
      device_pipe_prof->pi_config_file = nullptr;
      device_pipe_prof->num_pipes_in_scope = profile.pipe_scope_.size();
      std::copy(profile.pipe_scope_.begin(),
                profile.pipe_scope_.end(),
                device_pipe_prof->pipe_scope);
      profile_index++;
    }
    program_index++;
  }
  return Status();
}

Status DeviceForwardingConfig::saveToDisk(
    const bfrt_proto::SetForwardingPipelineConfigRequest &request) {
  // Go over each program in the repeated field
  std::string install_path = ".";
  if (request.base_path().size() > 0) {
    install_path = request.base_path();
    if (install_path.back() == '/') {
      install_path.pop_back();
    }
  }
  Status grpc_status = Status();
  for (const auto &config : request.config()) {
    // save all the files in the mentioned location
    const std::string &p4_name = config.p4_name();
    std::string program_path = install_path + "/" + p4_name;
    grpc_status = createFile(program_path + "/" + BF_RT_FILE_NAME,
                             config.bfruntime_info());
    grpc_check_and_return(grpc_status, "Failed to create BF-RT file");

    for (const auto &profile : config.profiles()) {
      grpc_status = createFile(
          program_path + "/" + profile.profile_name() + "/" + CONTEXT_FILE_NAME,
          profile.context());
      grpc_check_and_return(grpc_status, "Failed to create context json file");

      grpc_status = createFile(
          program_path + "/" + profile.profile_name() + "/" + PIPELINE_CFG_FILE_NAME,
          profile.binary());
      grpc_check_and_return(grpc_status, "Failed to create binary file");
    }
  }
  return grpc_status;
}

Status DeviceForwardingConfig::readFromDisk(
    bfrt_proto::GetForwardingPipelineConfigResponse *response) {
  auto non_p4 = response->mutable_non_p4_config();
  std::string non_p4_content =
      readAndSquashBfrtJsons(this->fixed_info_path_vec_);
  non_p4->set_bfruntime_info(non_p4_content);
  // Loop all the programs
  for (const auto &program : programs_) {
    auto config = response->add_config();
    // Read from bf-rt files
    std::ifstream bfrt_stream(program.p4_bfrt_path_);
    if (bfrt_stream.fail()) {
      check_and_return(BF_OBJECT_NOT_FOUND,
                       "Error opening file %s",
                       program.p4_bfrt_path_.c_str());
    }
    std::string bfrt_content((std::istreambuf_iterator<char>(bfrt_stream)),
                             std::istreambuf_iterator<char>());

    // Set P4 name and bf-rt info in the response
    config->set_p4_name(program.prog_name_);
    config->set_bfruntime_info(bfrt_content);

    for (const auto &profile : program.p4_pipelines_) {
      // Construct program path
      // Assumption -> this loop won't be entered by $SHARED since
      // program.p4_pipelines_ will be empty
      std::string program_path = program.p4_bfrt_path_;
      program_path.erase(program_path.size() - BF_RT_FILE_NAME.size() - 1,
                         BF_RT_FILE_NAME.size() + 1);

      auto response_profile = config->add_profiles();
      // Get profile directory path
      auto profile_dir = program_path + "/" + profile.profile_name_;
      response_profile->set_profile_name(profile.profile_name_);

      // Read context.json
      std::ifstream context_stream(profile.context_json_path_);
      if (context_stream.fail()) {
        LOG_WARN("Error opening file %s", profile.context_json_path_.c_str());
      } else {
        std::string context_content(
            (std::istreambuf_iterator<char>(context_stream)),
            std::istreambuf_iterator<char>());
        context_stream.close();
        response_profile->set_context(context_content);
      }

      // Read the binary
      std::ifstream binary_stream(profile.binary_path_);
      if (binary_stream.fail()) {
        LOG_WARN("Error opening file %s", profile.binary_path_.c_str());
      } else {
        std::string binary_content(
            (std::istreambuf_iterator<char>(binary_stream)),
            std::istreambuf_iterator<char>());
        response_profile->set_binary(binary_content);
        binary_stream.close();
      }
      // Create the response for the profile
      auto pipe_scope_vec = response_profile->mutable_pipe_scope();
      for (const auto &pipe : profile.pipe_scope_) {
        pipe_scope_vec->Add(pipe);
      }
    }
  }
  return Status();
}

// Right now constructing a BoundProgram object entails looking for
// an an already present BfRtInfo object. Later on, this needs to be
// changed into constructing one and storing the binary and bf_rt.json
BoundProgram::BoundProgram(const bf_rt_id_t &device_id,
                           const std::string &p4_name)
    : device_id_(device_id), p4_name_(p4_name) {
  auto bf_status =
      BfRtDevMgr::getInstance().bfRtInfoGet(device_id, p4_name, &bfrt_info_);
  if (bf_status != BF_SUCCESS) {
    throw bfRtInfoNotFoundException();
  }
}

// TODO implement a check in here which wil verify whether the config sent
// works or not
Status ConfigManager::verify(
    const DeviceForwardingConfig & /*device_forwarding_config*/) const {
  switch (fwd_config_state_) {
    default:
      break;
  }
  // verify whether this config works
  return Status();
}

Status ConfigManager::warmInitBegin(
    std::unique_ptr<DeviceForwardingConfig> device_forwarding_config,
    const bfrt_proto::SetForwardingPipelineConfigRequest &request) {
  switch (fwd_config_state_) {
    // Allowed transitions ----->
    // INITIAL ->  WARM_INIT_STARTED
    // WARM_INIT_FINISHED -> WARM_INIT_STARTED
    case ForwardingConfigState::INITIAL:
    case ForwardingConfigState::WARM_INIT_FINISHED: {
      LOG_DBG("%s:%d Starting warm_init", __func__, __LINE__);
      device_forwarding_config_ = std::move(device_forwarding_config);
      auto grpc_status = device_forwarding_config_->saveToDisk(request);
      grpc_check_and_return(grpc_status, "Failed to move files");
      const auto &req_mode = request.dev_init_mode();
      bf_dev_init_mode_t mode;
      switch (req_mode) {
        case bfrt_proto::
            SetForwardingPipelineConfigRequest_DevInitMode_FAST_RECONFIG:
          mode = BF_DEV_WARM_INIT_FAST_RECFG;
          break;
        case bfrt_proto::SetForwardingPipelineConfigRequest_DevInitMode_HITLESS:
          mode = BF_DEV_WARM_INIT_HITLESS;
          break;
        default:
          mode = BF_DEV_WARM_INIT_FAST_RECFG;
      }
      // Initiate warm_init with bf_pal
      // upgrade_agents need to be true here since we don't support
      // not upgrading platforms/diags lib while warm_init. Ports
      // won't come up if this is set to false right now
      auto bf_status = bf_pal_device_warm_init_begin(
          device_forwarding_config_->deviceIdGet(),
          mode,
          true);
      check_and_return(bf_status, "Failed to begin warm_init");

      // Add device with bf_pal
      bf_device_profile_t device_profile;
      grpc_status =
          device_forwarding_config_->getDeviceProfile(&device_profile);
      grpc_check_and_return(grpc_status, "Failed to get device_profile");

      bf_status = bf_pal_device_add(device_forwarding_config_->deviceIdGet(),
                                    &device_profile);
      check_and_return(bf_status, "Failed to add device during warm_init");
      fwd_config_state_ = ForwardingConfigState::WARM_INIT_STARTED;
      break;
    }
    case ForwardingConfigState::WARM_INIT_STARTED:
      check_and_return(BF_NOT_READY, "Warm_init in process.");
      break;
    default:
      check_and_return(BF_INVALID_ARG, "Invalid arg");
      break;
  }
  return Status();
}

Status ConfigManager::warmInitEnd() {
  switch (fwd_config_state_) {
    case ForwardingConfigState::INITIAL:
      check_and_return(BF_OBJECT_NOT_FOUND, "No config saved found");
      break;

    case ForwardingConfigState::WARM_INIT_STARTED: {
      // End warm_init with bf_pal. Since we are not replaying
      // anything ourselves as of now, we directly call warm_init
      // which will clear all forwarding state
      auto bf_status =
          bf_pal_device_warm_init_end(device_forwarding_config_->deviceIdGet());
      check_and_return(bf_status, "Failed to end warm_init");

      fwd_config_state_ = ForwardingConfigState::WARM_INIT_FINISHED;
      LOG_DBG("%s:%d Warm init finished !!", __func__, __LINE__);
      break;
    }
    case ForwardingConfigState::WARM_INIT_FINISHED:
      check_and_return(
          BF_NOT_READY,
          "Already warm_init_finished. Cannot perform WARM_INIT_END again");
      break;
    default:
      check_and_return(BF_INVALID_ARG, "Invalid arg");
  }
  return Status();
}

Status ConfigManager::get(
    const bf_dev_id_t &device_id,
    bfrt_proto::GetForwardingPipelineConfigResponse *response) const {
  switch (fwd_config_state_) {
    case ForwardingConfigState::INITIAL:
    case ForwardingConfigState::WARM_INIT_FINISHED: {
      // Creating an object itself since we don't want to return
      // the internally cached object which might have been created by another
      // client
      auto device_forwarding_config = std::unique_ptr<DeviceForwardingConfig>(
          new DeviceForwardingConfig(device_id));
      return device_forwarding_config->readFromDisk(response);
    }

    case ForwardingConfigState::WARM_INIT_STARTED: {
      // Since this client is in WARM_INIT_STARTED state, it can send its own
      // saved config
      if (device_forwarding_config_) {
        return device_forwarding_config_->readFromDisk(response);
      }
      check_and_return(BF_INVALID_ARG, "Unable to retrieve Forwarding config");
      break;
    }

    default:
      check_and_return(BF_INVALID_ARG, "Invalid arg");
  }
  return Status();
}

ConnectionData::ConnectionData(const uint32_t &client_id,
                               const Notifications &notifications,
                               std::shared_ptr<BfRtSession> session,
                               ServerContext *server_context,
                               StreamChannelReaderWriter *stream)
    : client_id_(client_id),
      notifications_(notifications),
      session_(session),
      server_context_(server_context) {
  is_independent_ = false;
  connection_ = StreamWrapper::make(stream);
}

// This constructor is used to create independent clients only
ConnectionData::ConnectionData(const uint32_t &client_id,
                               std::shared_ptr<BfRtSession> session)
    : ConnectionData(client_id,
                     Notifications(false, false, false),
                     session,
                     nullptr,
                     nullptr) {
  is_independent_ = true;
}

Status ConnectionData::boundProgramGet(
    const BoundProgram **bound_program) const {
  std::lock_guard<std::mutex> lock(bound_config_mutex_);
  if (bound_program_ == nullptr) {
    LOG_WARN("%s:%d Bound config not found", __func__, __LINE__);
    *bound_program = nullptr;
    return Status(StatusCode::NOT_FOUND, "Bound config not found");
  }
  *bound_program = bound_program_.get();
  return Status();
}

void ConnectionData::sendStreamMessage(
    bfrt_proto::StreamMessageResponse &response) const {
  if (connection_.get() == nullptr) {
    // Indicates that a bi-directional stream was not opened by the client
    // to receive any callbacks from the server
    return;
  }

  // Next we need to check if the ServerReaderWriter stream pointer managed by
  // grpc and cached in the connection_data object is actually still valid
  // and only send out the response when the stream is actually valid
  // This is important to check because we are caching a pointer to the stream
  // that is owned by the grpc infra. Thus when the client disconnects, the
  // thread which was held up in the StreamChannel RPC will break from the
  // while loop and return. This will cause grpc infra to invalidate it. Now,
  // it might so happen that another thread (learn cb) might be in the middle
  // of sending learn data to the client on this stream (which is no longer
  // valid).
  {
    std::lock_guard<std::mutex> stream_lock(getStreamChannelRWMutex());
    if (getStreamChannelRWValidFlag()) {
      connection_.get()->stream()->Write(response);
    }
  }
}

void ConnectionData::insertTableIdInList(const bf_rt_id_t &table_id) {
  operation_cb_sync_list.insert(table_id);
}

bool ConnectionData::removeIfPresentTableIdFromList(
    const bf_rt_id_t &table_id) {
  if (operation_cb_sync_list.find(table_id) == operation_cb_sync_list.end()) {
    return false;
  }
  operation_cb_sync_list.erase(table_id);
  return true;
}

void ConnectionData::setSubscribeForIndependentClient(
    const Notifications &notifications,
    ServerContext *context,
    StreamChannelReaderWriter *stream) {
  notifications_ = notifications;
  server_context_ = context;
  session_ = BfRtSession::sessionCreate();
  connection_ = StreamWrapper::make(stream);
  is_independent_ = false;
}

ServerData::ServerData(std::string server_name, std::string address)
    : server_name_(server_name), address_(address) {
  // The default receive message size for the server is 4MB. Change this to
  // INT_MAX so that it can accept messages of unlimited length (well almost)
  builder_.SetMaxReceiveMessageSize(INT_MAX);
  builder_.AddListeningPort(
      address_, grpc::InsecureServerCredentials(), &port_);
  builder_.RegisterService(&service_);
#ifdef PTF_ENABLED
  builder_.RegisterService(&ptfservice_); //Adding PTF Service on the server
#endif
  server_ = builder_.BuildAndStart();
  LOG_DBG("%s:%d  %s listening on port %d",
          __func__,
          __LINE__,
          server_name_.c_str(),
          port_);
}

BfRtServer &BfRtServer::getInstance(std::unique_ptr<ServerData> server_data) {
  // It's important to allocate Server object on the heap instead of the data
  // segment (static initialization) because when switchd application exits
  // (triggered by signal SIGINT or any other), the exit() function is called
  // which will call the destructors of all the statically allocated objects.
  // When the BfRtServer (and thereby grpc::Server) object is destroyed, there
  // is a check in the grpc backend which waits for any open client streams
  // to end before it can be successfully destroyed. Now, if we have an active
  // client stream, this check will prevent the switchd application to exit as
  // it will be held indefinitely waiting for the stream to end. Allocating
  // the object on the heap avoids this as in that case, exit() function won't
  // trigger BfRtServer's destructor to be called. Thus even if we have a
  // client stream open, the switchd application won't wait for it to be closed
  // and thus exit with the thread handling the client stream simply dying with
  // the switchd application. Refer to DRV-2477 for more details
  std::call_once(
      bfrt_server_once_flag,
      [&] { bfrt_server_obj = new BfRtServer(std::move(server_data)); });
  return *bfrt_server_obj;
}

BfRtServer::BfRtServer(std::unique_ptr<ServerData> server_data) {
  server_data_ = std::move(server_data);
}

Status BfRtServer::addConnection(const uint32_t &client_id,
                                 const bfrt_proto::Subscribe &subscribe,
                                 ServerContext *context,
                                 StreamChannelReaderWriter *stream) {
  std::lock_guard<std::mutex> lock(m);

  Notifications notifications(
      subscribe.notifications().enable_learn_notifications(),
      subscribe.notifications().enable_idletimeout_notifications(),
      subscribe.notifications().enable_port_status_change_notifications());

  auto connection_sp_it = client_id_to_connection_data_map.find(client_id);

  if (connection_sp_it != client_id_to_connection_data_map.end()) {
    // Checking if client is trying to change from INDEPENDENT -> SUBSCRIBED
    auto connection_sp = (*connection_sp_it).second;
    if (connection_sp->isIndependent()) {
      connection_sp->setSubscribeForIndependentClient(
          notifications, context, stream);
      return Status();
    } else {
      // Client IDs must be unique. This is an error
      check_and_return(BF_INVALID_ARG,
                       "Can't have two clients with the same client id");
    }
  }

  // Finally add the connection to the map
  client_id_to_connection_data_map.emplace(
      client_id,
      std::shared_ptr<ConnectionData>(
          new ConnectionData(client_id,
                             notifications,
                             BfRtSession::sessionCreate(),
                             context,
                             stream)));
  LOG_DBG("%s:%d Client %d Connected!", __func__, __LINE__, client_id);
  return Status();
}

// This function adds an independent connection
// it does nothing if client already exists in map
Status BfRtServer::addConnection(const uint32_t &client_id) {
  std::lock_guard<std::mutex> lock(m);
  if (client_id_to_connection_data_map.find(client_id) !=
      client_id_to_connection_data_map.end()) {
    // Client already exists. Just sending error code and not printing
    // error
    return to_grpc_status(BF_ALREADY_EXISTS, "Client already exists.");
  }

  // Add the connection to the map
  client_id_to_connection_data_map.emplace(
      client_id,
      std::shared_ptr<ConnectionData>(
          new ConnectionData(client_id, BfRtSession::sessionCreate())));
  LOG_DBG("%s:%d Client %d Connected!", __func__, __LINE__, client_id);
  return Status();
}

// This function needs to be called at the very beginning of a rpc
// call since the life of the shared_ptr (and hence the connectionData)
// needs to be for throughout the life of the rpc call
Status BfRtServer::getConnection(
    const uint32_t &client_id,
    std::shared_ptr<const ConnectionData> *connection_data) const {
  std::lock_guard<std::mutex> lock(m);
  if (client_id_to_connection_data_map.find(client_id) ==
      client_id_to_connection_data_map.end()) {
    check_and_return(BF_NOT_READY, "Unknown client. Client_id:%d", client_id);
  }
  *connection_data = client_id_to_connection_data_map.at(client_id);
  return Status();
}

Status BfRtServer::getConnection(
    const uint32_t &client_id,
    std::shared_ptr<ConnectionData> *connection_data) const {
  std::lock_guard<std::mutex> lock(m);
  if (client_id_to_connection_data_map.find(client_id) ==
      client_id_to_connection_data_map.end()) {
    check_and_return(BF_NOT_READY, "Unknown client. Client_id:%d", client_id);
  }
  *connection_data = client_id_to_connection_data_map.at(client_id);
  return Status();
}

Status BfRtServer::refreshConnections(const bf_dev_id_t &device_id) {
  // Assumption -> This function is being called after getting the
  // BfRtServer mutex
  // Get the device from the device mgr
  std::vector<std::reference_wrapper<const std::string>> p4_names;
  bf_status_t sts =
      BfRtDevMgr::getInstance().bfRtInfoP4NamesGet(device_id, p4_names);
  check_and_return(sts, "Failed to find p4 program for device %d", device_id);

  // For each client do the following
  // if the client is bound to some program and the bound device is the same as
  // the one getting refreshed
  //    if it needs to be kicked out,i.e. its bound program
  //    is not valid anymore, then
  //          Remove the bound object and cancel the connection to unblock
  //          the while loop
  //    Else it is valid,
  //          Be nice to it and update its BfRtInfo ptr since it must have
  //          gotten stale. Register learn_callback for them too.
  // else
  //  Leave it alone.
  for (const auto &connection_data_pair : client_id_to_connection_data_map) {
    auto connection_sp = connection_data_pair.second;
    const BoundProgram *bound_program;
    auto grpc_status = connection_sp->boundProgramGet(&bound_program);
    if (bound_program != nullptr && bound_program->deviceIdGet() == device_id) {
      // If the P4 couldn't be found, then kick it out
      // If P4 was found, then update its BfRtInfo by creating a new
      // BoundProgram Object
      if (std::find_if(p4_names.begin(),
                       p4_names.end(),
                       [&](const std::reference_wrapper<const std::string> &i) {
                         return bound_program->p4NameGet() == i.get();
                       }) == p4_names.end()) {
        LOG_WARN("%s:%d Kicking client out %d which was bound to %s",
                 __func__,
                 __LINE__,
                 connection_data_pair.first,
                 bound_program->p4NameGet().c_str());
        connection_sp->cancel();
        connection_sp->boundProgramReset();
      } else {
        std::string p4_name = bound_program->p4NameGet();
        try {
          connection_sp->boundProgramSet(std::unique_ptr<BoundProgram>(
              new BoundProgram(device_id, p4_name)));
        } catch (...) {
          check_and_return(BF_OBJECT_NOT_FOUND,
                           "Failed to find BfRtInfo for program %s",
                           p4_name.c_str());
        }
      }
    }
  }
  return Status();
}

Status BfRtServer::forwardingConfigBind(
    const uint32_t &client_id,
    const bfrt_proto::SetForwardingPipelineConfigRequest *request) {
  std::lock_guard<std::mutex> lock(m);
  if (client_id_to_connection_data_map.find(client_id) ==
      client_id_to_connection_data_map.end()) {
    check_and_return(BF_NOT_READY, "Unknown client. Client_id:%d", client_id);
  }
  auto connection_sp = client_id_to_connection_data_map.at(client_id);
  if (request->config().size() != 1) {
    check_and_return(BF_INVALID_ARG, "Exactly one config message required.");
  }
  for (const auto &kv : client_id_to_connection_data_map) {
    const BoundProgram *bound_program = nullptr;
    kv.second->boundProgramGet(&bound_program);
    if (bound_program == nullptr) {
      // This just means that the other client just hasn't set its
      // own bound_program yet. Leave them alone
      continue;
    }
    if (bound_program->p4NameGet() == request->config(0).p4_name()) {
      // Indicates that another client is already in-charge of this P4.
      // Hence return error for now.
      // TODO: In future need to handle this and maybe compare the client id
      // of the incoming request with the one already present and then if
      // it is greater promote it to actually handle this p4 and demote the
      // existing one to be only read-only. This is just one way of
      // handling. Can we think of any other approach (maybe consistent with
      // p4runtime)
      check_and_return(
          BF_ALREADY_EXISTS,
          "Client ID %d trying to bind but Client ID %d already owns this P4",
          client_id,
          kv.first);
    }
  }

  try {
    connection_sp->boundProgramSet(std::unique_ptr<BoundProgram>(
        new BoundProgram(request->device_id(), request->config(0).p4_name())));
  } catch (...) {
    check_and_return(BF_OBJECT_NOT_FOUND,
                     "Failed to find BfRtInfo for program %s",
                     request->config(0).p4_name().c_str());
  }
  return Status();
}

Status BfRtServer::forwardingConfigSet(
    const uint32_t &client_id,
    const bfrt_proto::SetForwardingPipelineConfigRequest *request) {
  std::lock_guard<std::mutex> lock(m);
  if (client_id_to_connection_data_map.find(client_id) ==
      client_id_to_connection_data_map.end()) {
    check_and_return(BF_NOT_READY, "Unknown client. Client_id:%d", client_id);
  }
  auto connection_sp = client_id_to_connection_data_map.at(client_id);

  auto fwd_action = request->action();
  std::unique_ptr<DeviceForwardingConfig> fwd_config;
  try {
    fwd_config = std::unique_ptr<DeviceForwardingConfig>(
        new DeviceForwardingConfig(*request));
  } catch (...) {
    check_and_return(BF_INVALID_ARG, "Invalid device config sent");
  }
  bool refresh = false;
  Status grpc_status = Status();

  switch (fwd_action) {
    case bfrt_proto::SetForwardingPipelineConfigRequest_Action_BIND: {
      // We are already handling this in forwardingConfigBind()
      break;
    }

    case bfrt_proto::SetForwardingPipelineConfigRequest_Action_VERIFY: {
      // Verify
      grpc_status = connection_sp->config_manager.verify(*fwd_config);
      grpc_check_and_return(grpc_status, "Verify config failed");
      break;
    }

    case bfrt_proto::
        SetForwardingPipelineConfigRequest_Action_VERIFY_AND_WARM_INIT_BEGIN: {
      // Verify
      grpc_status = connection_sp->config_manager.verify(*fwd_config);
      grpc_check_and_return(grpc_status, "Verify config failed");

      // WARM_INIT_BEGIN
      grpc_status = connection_sp->config_manager.warmInitBegin(
          std::move(fwd_config), *request);
      if (!grpc_status.ok()) {
        // set to refresh connections and break
        LOG_ERROR("%s:%d Failed to warm_init begin", __func__, __LINE__);
        refresh = true;
        break;
      }
      grpc_status = this->sendSetForwardingPipelineConfigResponse(
          bfrt_proto::SetForwardingPipelineConfigResponseType::
              WARM_INIT_STARTED);
      refresh = true;
      break;
    }
    case bfrt_proto::
        SetForwardingPipelineConfigRequest_Action_VERIFY_AND_WARM_INIT_BEGIN_AND_END: {
      // Verify
      grpc_status = connection_sp->config_manager.verify(*fwd_config);
      grpc_check_and_return(grpc_status, "Verify config failed");

      // WARM_INIT_BEGIN
      grpc_status = connection_sp->config_manager.warmInitBegin(
          std::move(fwd_config), *request);
      if (!grpc_status.ok()) {
        // set to refresh connections and break
        LOG_ERROR("%s:%d Failed to warm_init begin", __func__, __LINE__);
        refresh = true;
        break;
      }
      grpc_status = this->sendSetForwardingPipelineConfigResponse(
          bfrt_proto::SetForwardingPipelineConfigResponseType::
              WARM_INIT_STARTED);
      if (!grpc_status.ok()) {
        // set to refresh connections and break
        LOG_ERROR("%s:%d Failed to send setFwd response", __func__, __LINE__);
        refresh = true;
        break;
      }
      // WARM_INIT_END
      grpc_status = connection_sp->config_manager.warmInitEnd();
      grpc_check_and_return(grpc_status, "Commit config failed");
      grpc_status = this->sendSetForwardingPipelineConfigResponse(
          bfrt_proto::SetForwardingPipelineConfigResponseType::
              WARM_INIT_FINISHED);
      refresh = true;
      break;
    }
    case bfrt_proto::SetForwardingPipelineConfigRequest_Action_WARM_INIT_END: {
      if (request->config_size() > 0) {
        check_and_return(BF_INVALID_ARG,
                         "Commit msg cannot contain any config.");
      }
      // WARM_INIT_END
      grpc_status = connection_sp->config_manager.warmInitEnd();
      grpc_check_and_return(grpc_status, "Commit config failed");
      grpc_status = this->sendSetForwardingPipelineConfigResponse(
          bfrt_proto::SetForwardingPipelineConfigResponseType::
              WARM_INIT_FINISHED);
      grpc_check_and_return(grpc_status, "Failed to send set forward response");
      // No need to refresh clients since they would have been refreshed on
      // WARM_INIT_BEGIN
      break;
    }
    case bfrt_proto::
        SetForwardingPipelineConfigRequest_Action_RECONCILE_AND_WARM_INIT_END: {
      check_and_return(BF_NOT_SUPPORTED,
                       "Reconcile and warmInitEnd not supported");
      break;
    }
    default:
      break;
  }

  if (refresh) {
    // Refresh all connectionDatas and kick out invalid clients.
    grpc_status = this->refreshConnections(request->device_id());
    grpc_check_and_return(grpc_status, "Failed to refresh connections");
  }

  return grpc_status;
}

Status BfRtServer::sendSetForwardingPipelineConfigResponse(
    bfrt_proto::SetForwardingPipelineConfigResponseType response_status) {
  // send a msg to all the current clients with the status_code
  bfrt_proto::StreamMessageResponse response;
  auto type = response.mutable_set_forwarding_pipeline_config_response();
  type->set_set_forwarding_pipeline_config_response_type(response_status);
  for (const auto &client_pair : client_id_to_connection_data_map) {
    if (!client_pair.second->isIndependent()) {
      client_pair.second->sendStreamMessage(response);
    }
  }
  return Status();
}

void BfRtServer::cleanupConnection(const uint32_t &client_id) {
  std::lock_guard<std::mutex> lock(m);
  if (client_id_to_connection_data_map.find(client_id) ==
      client_id_to_connection_data_map.end()) {
    LOG_ERROR(
        "%s:%d Trying to delete connection for an unknown client with id %d",
        __func__,
        __LINE__,
        client_id);
    return;
  }
  LOG_DBG("Trying to clean up connection with client_ID %d", client_id)

  // Get connection data
  auto connection_data = client_id_to_connection_data_map.at(client_id);

  // If it is an Independent Client, all this is not needed
  if (!connection_data->isIndependent()) {
    const BoundProgram *bound_program = nullptr;
    auto grpc_status = connection_data->boundProgramGet(&bound_program);

    // Mark the reader writer stream channel as invalid
    connection_data->setStreamChannelRWValidFlag(false);
  }

  // Remove the connection data pertaining to this client
  LOG_DBG("%s:%d Deleting connection with client ID %d",
          __func__,
          __LINE__,
          client_id);
  client_id_to_connection_data_map.erase(client_id);
}

Status BfRtServer::clientListGet(std::vector<uint32_t> *client_id_list) const {
  std::lock_guard<std::mutex> lock(m);
  // Iterate over the map and return all the client ids
  for (const auto &item : client_id_to_connection_data_map) {
    client_id_list->push_back(item.first);
  }
  return Status();
}

}  // namespace server
}  // namespace bfrt

void bf_rt_grpc_server_run_with_addr(const char *server_address) {
  auto server_data = std::unique_ptr<bfrt::server::ServerData>(
      new bfrt::server::ServerData("BF-RT Server", server_address));
  printf("bfruntime gRPC server started on %s\n", server_address);
  bfrt::server::BfRtServer::getInstance(std::move(server_data));
}

void bf_rt_grpc_server_run(const char * /*program_name*/, bool local_only) {
  if (local_only) {
    bf_rt_grpc_server_run_with_addr("127.0.0.1:50052");
  } else {
    bf_rt_grpc_server_run_with_addr("0.0.0.0:50052");
  }
}

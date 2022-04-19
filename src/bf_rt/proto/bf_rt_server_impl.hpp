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
#ifndef _BF_RT_SERVER_HPP_
#define _BF_RT_SERVER_HPP_

#include <bf_rt/proto/bf_rt_server.h>
#include <bf_rt/bf_rt_common.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_pal/dev_intf.h>
#ifdef __cplusplus
}
#endif

#ifdef PTF_ENABLED
#include <ptf_server_impl.hpp>
#endif

#include <bfruntime.grpc.pb.h>
#include <google/rpc/status.grpc.pb.h>
#include <google/rpc/code.grpc.pb.h>
#include <grpc++/grpc++.h>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <chrono>

#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_session.hpp>
#include <bf_rt/bf_rt_table_attributes.hpp>
#include <bf_rt/bf_rt_table_operations.hpp>

namespace bfrt {
namespace server {

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;

using device_id_t = bf_dev_id_t;

using StreamChannelReaderWriter =
    ServerReaderWriter<bfrt_proto::StreamMessageResponse,
                       bfrt_proto::StreamMessageRequest>;

struct bfRtInfoNotFoundException : public std::exception {
  const char *what() const throw() { return "BfRtInfo Not Found"; }
};
struct fwdConfigError : public std::exception {
  const char *what() const throw() { return "Failed to set fwd config"; }
};
struct lockAcquireAgainError : public std::exception {
  const char *what() const throw() { return "Failed to acquire lock"; }
};

enum class ForwardingConfigState {
  INITIAL,
  WARM_INIT_STARTED,
  WARM_INIT_FINISHED
};

inline Status to_grpc_status(const bf_status_t &bf_status_from,
                             const std::string &msg_str_from) {
  switch (bf_status_from) {
    case BF_SUCCESS:
      return Status();
    case BF_NOT_READY:
    case BF_TABLE_LOCKED:
    case BF_DEVICE_LOCKED:
    case BF_IN_USE:
      return Status(StatusCode::UNAVAILABLE, msg_str_from);
    case BF_NO_SYS_RESOURCES:
    case BF_NO_SPACE:
    case BF_MAX_SESSIONS_EXCEEDED:
    case BF_STS_MAX:
    case BF_IO:
      return Status(StatusCode::RESOURCE_EXHAUSTED, msg_str_from);
    case BF_INVALID_ARG:
      return Status(StatusCode::INVALID_ARGUMENT, msg_str_from);
    case BF_ALREADY_EXISTS:
    case BF_ENTRY_REFERENCES_EXIST:
      return Status(StatusCode::ALREADY_EXISTS, msg_str_from);
    case BF_OBJECT_NOT_FOUND:
    case BF_SESSION_NOT_FOUND:
    case BF_TABLE_NOT_FOUND:
      return Status(StatusCode::NOT_FOUND, msg_str_from);
    case BF_NOT_SUPPORTED:
    case BF_NOT_IMPLEMENTED:
    case BF_TXN_NOT_SUPPORTED:
      return Status(StatusCode::UNIMPLEMENTED, msg_str_from);
    case BF_INTERNAL_ERROR:
      return Status(StatusCode::INTERNAL, msg_str_from);
    case BF_INIT_ERROR:
      return Status(StatusCode::ABORTED, msg_str_from);
    case BF_UNEXPECTED:
    case BF_HW_UPDATE_FAILED:
    case BF_HW_COMM_FAIL:
    case BF_NO_LEARN_CLIENTS:
    case BF_IDLE_UPDATE_IN_PROGRESS:
    case BF_EAGAIN:
      return Status(StatusCode::UNKNOWN, msg_str_from);
  }
  return Status(StatusCode::UNKNOWN, msg_str_from);
}

// Warning: This macro RETURNS grpc_error on non-success
// but just ignores and CONTINUES it is success
#define check_and_return(bf_status, ...)                 \
  do {                                                   \
    if (bf_status == BF_SUCCESS) {                       \
      break;                                             \
    }                                                    \
    std::string prnt_str;                                \
    char buf[1024];                                      \
    std::snprintf(buf, 1024, __VA_ARGS__);               \
    LOG_ERROR("BF_RT_SERVER:%s:%d %s %s",                \
              __func__,                                  \
              __LINE__,                                  \
              buf,                                       \
              bf_err_str(bf_status));                    \
    prnt_str = buf;                                      \
    prnt_str += " ";                                     \
    prnt_str += bf_err_str(bf_status);                   \
    Status status = to_grpc_status(bf_status, prnt_str); \
    if (!status.ok()) {                                  \
      return status;                                     \
    }                                                    \
  } while (0)

// Warning: This macro RETURNS grpc_error on non-success
// but just ignores and CONTINUES it is success. Doesn't change
// the grpc::Status error message
#define grpc_check_and_return(grpc_status, ...)                    \
  do {                                                             \
    std::string prnt_str;                                          \
    if (!grpc_status.ok()) {                                       \
      char buf[1024];                                              \
      std::snprintf(buf, 1024, __VA_ARGS__);                       \
      LOG_ERROR("BF_RT_SERVER:%s:%d %s", __func__, __LINE__, buf); \
      return grpc_status;                                          \
    }                                                              \
  } while (0)

// Forward declare
class ConnectionData;

// This class implements the interface of the Service exposed by the grpc
// server
class BfRuntimeServiceImpl : public bfrt_proto::BfRuntime::Service {
 private:
  Status GetForwardingPipelineConfig(
      ServerContext *context,
      const bfrt_proto::GetForwardingPipelineConfigRequest *request,
      bfrt_proto::GetForwardingPipelineConfigResponse *response) override;

  Status SetForwardingPipelineConfig(
      ServerContext *context,
      const bfrt_proto::SetForwardingPipelineConfigRequest *request,
      bfrt_proto::SetForwardingPipelineConfigResponse *response) override;

  Status Write(ServerContext *context,
               const bfrt_proto::WriteRequest *request,
               bfrt_proto::WriteResponse *response) override;

  Status Read(ServerContext *context,
              const bfrt_proto::ReadRequest *request,
              ServerWriter<bfrt_proto::ReadResponse> *writer) override;

  Status StreamChannel(ServerContext *context,
                       StreamChannelReaderWriter *stream) override;
};

// This is a helper class to manage the stream that a client open when
// connecting to the server. Please note that it is essential for the
// client to open a bi_directional stream so as to get notified about
// idle time, learn and other callbacks. In addition, the stream also serves
// as a hearbeat and is used by the server to detect when a client
// connection drops so that the server can cleanup the client state
class StreamWrapper {
 public:
  static std::unique_ptr<StreamWrapper> make(StreamChannelReaderWriter *stream);
  StreamChannelReaderWriter *stream() const { return stream_; }

 private:
  StreamWrapper(StreamChannelReaderWriter *stream) : stream_(stream) {}
  StreamChannelReaderWriter *stream_{nullptr};
};

/**
 * @brief This class creates and stores an entire device's config in form of
 * set_programs(prog_name, set_pipeline_profiles(profile_name,
 *set_pipe_scope()))
 * An object of this class can be constructed by 2 methods ->
 * 1. By passing in a protobuf of SetForwardingPipelineConfigRequest
 * 2. By passing in a device ID where we read from the current config on
 * the device and create this object.
 *
 * The object only contains metadata of DeviceConfig. The actual file contents
 * are not stored and are flushed to/ read from device directly via helpers
 * present
 */
class DeviceForwardingConfig {
 public:
  class ProgramConfig {
   public:
    class P4Pipeline {
     public:
      P4Pipeline(const std::string &profile_name,
                 const std::string &context_json_path,
                 const std::string &binary_path,
                 const std::set<bf_dev_pipe_t> pipe_scope)
          : profile_name_(profile_name),
            context_json_path_(context_json_path),
            binary_path_(binary_path),
            pipe_scope_(pipe_scope){};
      bool operator<(const P4Pipeline &anotherPipeline) const;
      const std::string profile_name_;
      const std::string context_json_path_;
      const std::string binary_path_;
      const std::set<bf_dev_pipe_t> pipe_scope_;
    };
    ProgramConfig(const std::string &prog_name,
                  const std::string &bfrt_info_path,
                  const std::set<P4Pipeline> &p4_pipelines)
        : prog_name_(prog_name),
          p4_bfrt_path_(bfrt_info_path),
          p4_pipelines_(p4_pipelines) {}
    bool operator<(const ProgramConfig &anotherProgram) const;
    const std::string prog_name_;
    const std::string p4_bfrt_path_;
    const std::set<P4Pipeline> p4_pipelines_;
  };

  DeviceForwardingConfig(
      const bfrt_proto::SetForwardingPipelineConfigRequest &request);
  DeviceForwardingConfig(const bf_dev_id_t &device_id);

  virtual ~DeviceForwardingConfig() = default;
  const bf_dev_id_t &deviceIdGet() const { return device_id_; }
  Status getDeviceProfile(bf_device_profile_t *device_profile);
  Status saveToDisk(
      const bfrt_proto::SetForwardingPipelineConfigRequest &request);
  Status readFromDisk(
      bfrt_proto::GetForwardingPipelineConfigResponse *response);

 private:
  const bf_dev_id_t device_id_;
  std::vector<std::string> fixed_info_path_vec_;
  std::set<ProgramConfig> programs_;
};

/**
 * @brief This class contains details about a bound program with
 * a client connection binds to. A client can be alive without binding
 * but it can only GetForwardingPipeline and SetForwardingPipeline right
 * now. It cannot Read/Write
 */
class BoundProgram {
 public:
  BoundProgram(const bf_rt_id_t &device_id, const std::string &p4_name);
  virtual ~BoundProgram() = default;

  const std::string &p4NameGet() const { return p4_name_; }
  const BfRtInfo *bfRtInfoGet() const { return bfrt_info_; }
  const bf_dev_id_t &deviceIdGet() const { return device_id_; }

 private:
  const bf_dev_id_t device_id_;
  const BfRtInfo *bfrt_info_;
  const std::string p4_name_;
};

/**
 * @brief Contains and manages a device fwd config. This only takes
 * care in case of a SetFwd master. This class helps separate out
 * Device config from what program a client is bound to. An instance
 * of it is present as a public member of ConnectionData so as to keep
 * things simple.
 */
class ConfigManager {
 public:
  ConfigManager() : fwd_config_state_(ForwardingConfigState::INITIAL){};
  Status verify(const DeviceForwardingConfig &device_forwarding_config) const;
  Status warmInitBegin(
      std::unique_ptr<DeviceForwardingConfig> device_forwarding_config,
      const bfrt_proto::SetForwardingPipelineConfigRequest &request);
  Status get(const bf_dev_id_t &device_id,
             bfrt_proto::GetForwardingPipelineConfigResponse *response) const;
  Status warmInitEnd();

 private:
  std::unique_ptr<DeviceForwardingConfig> device_forwarding_config_;
  ForwardingConfigState fwd_config_state_;
};

// Contains whether certain notifications are enabled
class Notifications {
 public:
  Notifications(bool enable_learn,
                bool enable_idletimeout,
                bool enable_port_status_change)
      : enable_learn_(enable_learn),
        enable_idletimeout_(enable_idletimeout),
        enable_port_status_change_(enable_port_status_change){};
  bool enable_learn_ = false;
  bool enable_idletimeout_ = false;
  bool enable_port_status_change_ = false;
};

// This is a helper class that stores all the data pertaining to a particular
// client connection on a device.
class ConnectionData {
 public:
  ConnectionData(const uint32_t &client_id,
                 const Notifications &notifications,
                 std::shared_ptr<BfRtSession> session,
                 ServerContext *context,
                 StreamChannelReaderWriter *stream);

  ConnectionData(const uint32_t &client_id,
                 std::shared_ptr<BfRtSession> session);

  ~ConnectionData() {
    // Complete operations. The session will be destroyed by itself
    // once the shared_ptr goes away
    session_->sessionCompleteOperations();
  }

  uint32_t clientIdGet() const { return client_id_; }
  const std::shared_ptr<BfRtSession> &sessionGet() const { return session_; }

  Status boundProgramGet(const BoundProgram **bound_program) const;
  void boundProgramSet(std::unique_ptr<BoundProgram> bound_program) {
    std::lock_guard<std::mutex> lock(bound_config_mutex_);
    bound_program_ = std::move(bound_program);
  }
  void boundProgramReset() {
    std::lock_guard<std::mutex> lock(bound_config_mutex_);
    bound_program_ = nullptr;
  }

  void sendStreamMessage(bfrt_proto::StreamMessageResponse &response) const;
  void insertTableIdInList(const bf_rt_id_t &table_id);
  bool removeIfPresentTableIdFromList(const bf_rt_id_t &table_id);
  std::condition_variable &getCV() { return operation_cv; }
  std::mutex &getMutex() { return operation_mtx; }
  uint8_t getTimeoutLimit() const { return timeout_limit_for_operation_cb; }
  void cancel() { server_context_->TryCancel(); }

  std::mutex &getStreamChannelRWMutex() const { return stream_channel_rw_mtx; }
  void setStreamChannelRWValidFlag(const bool &val) {
    std::lock_guard<std::mutex> stream_lock(getStreamChannelRWMutex());
    is_stream_channel_rw_valid = val;
  }
  bool getStreamChannelRWValidFlag() const {
    return is_stream_channel_rw_valid;
  }
  const Notifications &getNotifications() const { return notifications_; }
  const bool &isIndependent() const { return is_independent_; }
  void setSubscribeForIndependentClient(const Notifications &notifications,
                                        ServerContext *context,
                                        StreamChannelReaderWriter *stream);

  ConfigManager config_manager;

 private:
  uint32_t client_id_;
  Notifications notifications_;
  std::shared_ptr<BfRtSession> session_;
  ServerContext *server_context_;
  std::unique_ptr<BoundProgram> bound_program_;

  std::unique_ptr<StreamWrapper> connection_{nullptr};
  bool is_independent_ = false;

  // The cb registered with the sync operations will insert table ids (for
  // which the operation has been completed) into this list and the thread
  // which initiated the operation will be waiting for the table id (for
  // which it initiated the operation) to appear in this list
  std::unordered_set<bf_rt_id_t> operation_cb_sync_list;
  std::condition_variable operation_cv;
  std::mutex operation_mtx;
  // This is the max time that a thread will wait for the operation callback
  // to complete before returning to the client
  uint8_t timeout_limit_for_operation_cb{5}; /*seconds*/

  // The following mutex and flag is required to ensure the sanity of the
  // StreamChannelReaderWriter stream (which is managed by grpc and a pointer
  // to which is cached in connection_data object) before we write anything to
  // it OR read anything from it
  mutable std::mutex stream_channel_rw_mtx;
  // mutex to protect bound config
  mutable std::mutex bound_config_mutex_;
  bool is_stream_channel_rw_valid{true};
};

// This class contains all information regarding a GRPC connection
class ServerData {
 public:
  ServerData(std::string server_name, std::string address);

 private:
  std::string server_name_;
  std::string address_;
  int port_;
  BfRuntimeServiceImpl service_;
#ifdef PTF_ENABLED
  ptf::server::ptfServiceImpl ptfservice_;
#endif

  ServerBuilder builder_;
  std::unique_ptr<Server> server_;
};

/**
 * @brief Scope guard class to maintain a lock
 * for SetFwdPipe.Write lock is only taken when SetFwdPipe
 * is done. The read lock is taken by every Read/Write RPC
 * and the callbacks in order to avoid stepping over a SetFwdConfig
 * process
 *
 * Automatically released when goes out of scope.
 * The object creation still goes through even if the lock
 * acquire fails, but a boolean check on the object will return
 * false.
 */
class SetFwdConfigLockGuard {
 public:
  SetFwdConfigLockGuard(pthread_rwlock_t *set_fwd_rw_lock, const bool &is_write)
      : set_fwd_rw_lock_() {
    int success = 1;
    if (set_fwd_rw_lock != NULL) {
      if (is_write) {
        success = pthread_rwlock_wrlock(set_fwd_rw_lock);
      } else {
        success = pthread_rwlock_tryrdlock(set_fwd_rw_lock);
      }
      if (success == 0) {
        acquired_ = true;
        set_fwd_rw_lock_ = set_fwd_rw_lock;
      }
    }
  }
  ~SetFwdConfigLockGuard() {
    if (acquired_) {
      pthread_rwlock_unlock(set_fwd_rw_lock_);
    }
  }
  operator bool() const { return acquired_; }

 private:
  bool acquired_ = false;
  pthread_rwlock_t *set_fwd_rw_lock_;
  SetFwdConfigLockGuard() = delete;
};

class BfRtServer {
 public:
  static BfRtServer &getInstance(
      std::unique_ptr<ServerData> server_data = nullptr);
  BfRtServer(BfRtServer const &) = delete;
  void operator=(BfRtServer const &) = delete;
  // locks mutex
  Status getConnection(const uint32_t &client_id,
                       std::shared_ptr<const ConnectionData> *connection) const;
  // locks mutex
  Status getConnection(const uint32_t &client_id,
                       std::shared_ptr<ConnectionData> *connection) const;
  // locks mutex
  Status addConnection(const uint32_t &client_id,
                       const bfrt_proto::Subscribe &subscribe,
                       ServerContext *context,
                       StreamChannelReaderWriter *stream);
  Status addConnection(const uint32_t &client_id);
  // locks mutex
  Status forwardingConfigSet(
      const uint32_t &client_id,
      const bfrt_proto::SetForwardingPipelineConfigRequest *request);
  // locks mutex
  Status forwardingConfigBind(
      const uint32_t &client_id,
      const bfrt_proto::SetForwardingPipelineConfigRequest *request);
  // locks mutex
  void cleanupConnection(const uint32_t &client_id);
  // locks mutex
  Status clientListGet(std::vector<uint32_t> *client_id_list) const;
  Status sendSetForwardingPipelineConfigResponse(
      bfrt_proto::SetForwardingPipelineConfigResponseType response_status);
  Status refreshConnections(const bf_dev_id_t &device_id);
  pthread_rwlock_t *setFwdRwLockget() { return &set_fwd_rw_lock_; }

 private:
  explicit BfRtServer(std::unique_ptr<ServerData> server_data);
  static BfRtServer *bfrt_server_obj;
  std::unique_ptr<ServerData> server_data_{nullptr};
  // Client id to Connection data map and its mutex
  mutable std::mutex m{};
  std::map<uint32_t, std::shared_ptr<ConnectionData>>
      client_id_to_connection_data_map{};

  // RW lock for SetForwarding
  pthread_rwlock_t set_fwd_rw_lock_ = PTHREAD_RWLOCK_INITIALIZER;
};

// This class contains logic to convert multiple
// grpc::Status to one smushed grpc::Status.
//
// More details at
// https://s3-us-west-2.amazonaws.com/p4runtime/docs/v1.0.0-rc3/P4Runtime-Spec.html#sec-error-reporting-messages
class BfRtServerErrorReporter {
 public:
  BfRtServerErrorReporter(std::string default_msg)
      : default_msg_(default_msg){};

  using StatusMsg = ::google::rpc::Status;
  using Code = ::google::rpc::Code;

  // Stores all error messages in the reporter
  // $1 grpc::status (convert to) bfrt_proto::Error
  // $2 push $1 in a stack only if it was an error, else ignore
  void push_back(const Status &status) {
    if (!status.ok()) {
      bfrt_proto::Error error;
      error.set_canonical_code(status.error_code());
      error.set_message(status.error_message());
      error.set_space("");
      errors.emplace_back(index, error);
    }
    index++;
  }

  // Smushes all bfrt_proto::Error in 'any details' field of
  // ::google::rpc::Code. Then the ::google::rpc::Code message
  // is serialized into the "error_details" field of grpc::Status
  //
  // $1 Iterate over the stack if not empty and add to "details"
  //  in ::google::rpc::Code the current bfrt_proto::Error
  // $2 If for a certain missing index, there was no error present,
  //  then add to "details" a success bfrt_proto::Error msg
  // $3 Convert the ::google::rpc::Code to grpc::Status and return it
  Status get_status() const {
    StatusMsg status_msg;
    if (errors.empty()) {
      status_msg.set_code(Code::OK);
    } else {
      bfrt_proto::Error success;
      success.set_code(Code::OK);
      status_msg.set_code(Code::UNKNOWN);
      status_msg.set_message(default_msg_);
      size_t cur_index = 0;
      for (const auto &p : errors) {
        for (; cur_index++ < p.first;) {
          auto success_any = status_msg.add_details();
          success_any->PackFrom(success);
        }
        auto error_any = status_msg.add_details();
        error_any->PackFrom(p.second);
      }
    }
    Status ret_status;
    SetErrorDetails(status_msg, &ret_status);
    return ret_status;
  }

 private:
  // Copied from
  // https://github.com/grpc/grpc/blob/master/src/cpp/util/error_details.cc
  // Cannot use libgrpc++_error_details due to linkage issues
  static Status SetErrorDetails(const ::google::rpc::Status &from,
                                grpc::Status *to) {
    if (to == nullptr) {
      return Status(StatusCode::FAILED_PRECONDITION, "");
    }
    StatusCode code = StatusCode::UNKNOWN;
    if (from.code() >= StatusCode::OK && from.code() <= StatusCode::DATA_LOSS) {
      code = static_cast<StatusCode>(from.code());
    }
    *to = Status(code, from.message(), from.SerializeAsString());
    return Status::OK;
  }
  std::vector<std::pair<size_t, bfrt_proto::Error>> errors{};
  size_t index{0};
  std::string default_msg_{""};
};

}  // namespace server

}  // namespace bfrt

#endif  //_BF_RT_SERVER_HPP_

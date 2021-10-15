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

//#include <bf_rt_pre/bf_rt_mc_mgr_intf.hpp>
#include "bf_rt_pipe_mgr_intf.hpp"
#include "bf_rt_session_impl.hpp"
#include "bf_rt_utils.hpp"

#include "bf_rt_init_impl.hpp"

namespace bfrt {

std::shared_ptr<BfRtSession> BfRtSession::sessionCreate() {
  auto session = std::make_shared<BfRtSessionImpl>();
  auto status = session->sessionCreateInternal();
  if (status != BF_SUCCESS) {
    return nullptr;
  }
  return session;
}

bf_status_t BfRtSessionImpl::sessionCreateInternal() {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  pipe_sess_hdl_t sessHndl = 0;

  bool port_mgr_skip;
  BfRtDevMgrImpl::bfRtDeviceConfigGet(&port_mgr_skip);

  status = pipeMgr->pipeMgrClientInit(&sessHndl);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d Error in creating pipe mgr session, err %s",
              __func__,
              __LINE__,
              pipe_str_err((pipe_status_t)status));
    return status;
  }
  session_handle_ = sessHndl;

  // Mark session as valid
  valid_ = true;

  return BF_SUCCESS;
}

BfRtSessionImpl::BfRtSessionImpl()
    : session_handle_(), pre_session_handle_(), valid_(false) {}

BfRtSessionImpl::~BfRtSessionImpl() {
  // session delete here
  if (valid_) {
    bf_status_t status = sessionDestroy();
    (void)status;
  }
}

bf_status_t BfRtSessionImpl::sessionDestroy() {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();

  bool port_mgr_skip;
  BfRtDevMgrImpl::bfRtDeviceConfigGet(&port_mgr_skip);

  status = pipeMgr->pipeMgrClientCleanup(session_handle_);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in cleaning up the pipe mgr session "
        "with handle %d, err %s",
        __func__,
        __LINE__,
        session_handle_,
        pipe_str_err((pipe_status_t)status));
    return status;
  }

  // Mark session as invalid
  valid_ = false;

  return BF_SUCCESS;
}

bf_status_t BfRtSessionImpl::sessionCompleteOperations() const {
  if (valid_) {
    bf_status_t status = BF_SUCCESS;
    auto *pipeMgr = PipeMgrIntf::getInstance();

    bool port_mgr_skip;
    BfRtDevMgrImpl::bfRtDeviceConfigGet(&port_mgr_skip);

    status = pipeMgr->pipeMgrCompleteOperations(session_handle_);

    return status;
  }
  return BF_INVALID_ARG;
}

// Batching functions
bf_status_t BfRtSessionImpl::beginBatch() const {
  if (valid_) {
    bf_status_t status = BF_SUCCESS;
    auto *pipeMgr = PipeMgrIntf::getInstance();
    bool port_mgr_skip;
    BfRtDevMgrImpl::bfRtDeviceConfigGet(&port_mgr_skip);

    status = pipeMgr->pipeMgrBeginBatch(session_handle_);

    return status;
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtSessionImpl::flushBatch() const {
  if (valid_) {
    bf_status_t status = BF_SUCCESS;
    auto *pipeMgr = PipeMgrIntf::getInstance();
    bool port_mgr_skip;
    BfRtDevMgrImpl::bfRtDeviceConfigGet(&port_mgr_skip);

    status = pipeMgr->pipeMgrFlushBatch(session_handle_);


    return status;
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtSessionImpl::endBatch(bool hwSynchronous) const {
  if (valid_) {
    bf_status_t status = BF_SUCCESS;
    auto *pipeMgr = PipeMgrIntf::getInstance();

    bool port_mgr_skip;
    BfRtDevMgrImpl::bfRtDeviceConfigGet(&port_mgr_skip);

    status = pipeMgr->pipeMgrEndBatch(session_handle_, hwSynchronous);

    return status;
  }
  return BF_INVALID_ARG;
}

// Transaction functions
bf_status_t BfRtSessionImpl::beginTransaction(bool isAtomic) const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrBeginTxn(session_handle_, isAtomic);
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtSessionImpl::verifyTransaction() const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrVerifyTxn(session_handle_);
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtSessionImpl::commitTransaction(bool hwSynchronous) const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrCommitTxn(session_handle_, hwSynchronous);
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtSessionImpl::abortTransaction() const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrAbortTxn(session_handle_);
  }
  return BF_INVALID_ARG;
}

}  // bfrt

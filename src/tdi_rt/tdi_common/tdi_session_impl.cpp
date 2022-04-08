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

//#include <tdi_rt_pre/tdi_rt_mc_mgr_intf.hpp>
#include <tdi/common/tdi_utils.hpp>
#include "tdi_pipe_mgr_intf.hpp"
#include "tdi_session_impl.hpp"


namespace tdi {
namespace pna {
namespace rt {

tdi_status_t TdiSessionImpl::sessionCreateInternal() {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  pipe_sess_hdl_t sessHndl = 0;


  status = pipeMgr->pipeMgrClientInit(&sessHndl);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d Error in creating pipe mgr session, err %s",
              __func__,
              __LINE__,
              pipe_str_err((pipe_status_t)status));
    return status;
  }
  session_handle_ = sessHndl;

  // Mark session as valid
  valid_ = true;

  return TDI_SUCCESS;
}

TdiSessionImpl::TdiSessionImpl()
    : Session(std::vector<tdi_mgr_type_e>()),
      in_batch_(false),
      in_pipe_mgr_batch_(false),
      session_handle_(), 
      valid_(false) {}

TdiSessionImpl::~TdiSessionImpl() {
  // session delete here
  if (valid_)
    destroy();
}

tdi_status_t TdiSessionImpl::destroy() {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();

  status = pipeMgr->pipeMgrClientCleanup(session_handle_);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in cleaning up the pipe mgr session "
        "with handle %d, err %s",
        __func__,
        __LINE__,
        session_handle_,
        pipe_str_err((pipe_status_t)status));
    return status;
  }
  this->in_pipe_mgr_batch_ = false;
  this->in_batch_ = false;
  // Mark session as invalid
  valid_ = false;

  return TDI_SUCCESS;
}

tdi_status_t TdiSessionImpl::completeOperations() const {
  if (valid_) {
    tdi_status_t status = TDI_SUCCESS;
    auto *pipeMgr = PipeMgrIntf::getInstance();


    status = pipeMgr->pipeMgrCompleteOperations(session_handle_);

    return status;
  }
  return TDI_INVALID_ARG;
}

// Batching functions
tdi_status_t TdiSessionImpl::beginBatch() const {
  if (valid_ && !this->in_batch_) {
    this->in_batch_ = true;
    return TDI_SUCCESS;
  }
  LOG_ERROR(
      "%s:%d Session is invalid or already in batch.", __func__, __LINE__);
  return TDI_INVALID_ARG;
}

tdi_status_t TdiSessionImpl::flushBatch() const {
  if (valid_) {
    tdi_status_t status = TDI_SUCCESS;
    auto *pipeMgr = PipeMgrIntf::getInstance();

    if (this->in_pipe_mgr_batch_)
      status = pipeMgr->pipeMgrFlushBatch(session_handle_);


    return status;
  }
  return TDI_INVALID_ARG;
}

tdi_status_t TdiSessionImpl::endBatch(bool hwSynchronous) const {
  if (valid_) {
    tdi_status_t status = TDI_SUCCESS;
    auto *pipeMgr = PipeMgrIntf::getInstance();

    if (this->in_pipe_mgr_batch_) {
      status = pipeMgr->pipeMgrEndBatch(session_handle_, hwSynchronous);
      this->in_pipe_mgr_batch_ = false;
    }
    this->in_batch_ = false;
    return status;
  }
  return TDI_INVALID_ARG;
}

// Transaction functions
tdi_status_t TdiSessionImpl::beginTransaction(bool isAtomic) const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrBeginTxn(session_handle_, isAtomic);
  }
  return TDI_INVALID_ARG;
}

tdi_status_t TdiSessionImpl::verifyTransaction() const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrVerifyTxn(session_handle_);
  }
  return TDI_INVALID_ARG;
}

tdi_status_t TdiSessionImpl::commitTransaction(bool hwSynchronous) const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrCommitTxn(session_handle_, hwSynchronous);
  }
  return TDI_INVALID_ARG;
}

tdi_status_t TdiSessionImpl::abortTransaction() const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrAbortTxn(session_handle_);
  }
  return TDI_INVALID_ARG;
}

}
}
}  // tdi

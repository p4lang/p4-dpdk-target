// SPDX-FileCopyrightText: 2021 Intel Corporation
// Copyright (C) 2021 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0
#ifndef _TDI_SESSION_IMPL_HPP
#define _TDI_SESSION_IMPL_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <algorithm>

#include <tdi/common/tdi_session.hpp>

namespace tdi {
namespace pna {
namespace rt {

class TdiSessionImpl : public tdi::Session {
 public:
  TdiSessionImpl();

  ~TdiSessionImpl();

  tdi_status_t create();

  tdi_status_t destroy();

  tdi_status_t completeOperations() const;

  tdi_id_t handleGet(const tdi_mgr_type_e &mgr_type) const { return session_handle_; }

  const bool &isValid() const { return valid_; }

  // Batching functions
  tdi_status_t beginBatch() const;

  tdi_status_t flushBatch() const;

  tdi_status_t endBatch(bool hwSynchronous) const;

  // Transaction functions
  tdi_status_t beginTransaction(bool isAtomic) const;

  tdi_status_t verifyTransaction() const;

  tdi_status_t commitTransaction(bool hwSynchronous) const;

  tdi_status_t abortTransaction() const;

  // Hidden
  const bool &isInBatch() const { return in_batch_; }
  const bool &isInPipeBatch() const { return in_pipe_mgr_batch_; }
  void setPipeBatch(const bool batch) const { in_pipe_mgr_batch_ = batch; }

 private:
  mutable bool in_batch_;
  mutable bool in_pipe_mgr_batch_;
  tdi_id_t session_handle_;      // Pipe mgr session handle
  bool valid_;
};

}
}
}  // tdi

#endif  // _TDI_SESSION_IMPL_HPP

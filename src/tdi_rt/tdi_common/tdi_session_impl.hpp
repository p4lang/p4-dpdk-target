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

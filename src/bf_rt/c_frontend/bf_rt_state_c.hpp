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
#ifndef _BF_RT_STATE_C_HPP
#define _BF_RT_STATE_C_HPP

#include <mutex>
#include <unordered_map>

namespace bfrt {
namespace bfrt_c {

class BfRtCFrontEndSessionState {
 public:
  // To get the singleton instance. Threadsafe
  static BfRtCFrontEndSessionState &getInstance();

  // Get the shared_ptr from the raw pointer
  std::shared_ptr<BfRtSession> getSharedPtr(const BfRtSession *session_raw);

  // Insert shared_ptr in the state
  void insertShared(std::shared_ptr<BfRtSession> session);
  // Delete an entry from the raw ptr
  void removeShared(const BfRtSession *session_raw);
  BfRtCFrontEndSessionState(BfRtCFrontEndSessionState const &) = delete;
  void operator=(BfRtCFrontEndSessionState const &) = delete;

 private:
  BfRtCFrontEndSessionState() {}
  std::mutex state_lock;
  std::map<const BfRtSession *, std::shared_ptr<BfRtSession> > sessionStateMap;
};

}  // bfrt_c
}  // bfrt

#endif  // _BF_RT_STATE_C_HPP

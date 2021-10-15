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

#ifdef __cplusplus
}
#endif

#include <bf_rt/bf_rt_session.hpp>
#include "bf_rt_state_c.hpp"

namespace bfrt {
namespace bfrt_c {

BfRtCFrontEndSessionState &BfRtCFrontEndSessionState::getInstance() {
  static BfRtCFrontEndSessionState instance;
  return instance;
}

std::shared_ptr<BfRtSession> BfRtCFrontEndSessionState::getSharedPtr(
    const BfRtSession *session_raw) {
  if (session_raw == nullptr) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(state_lock);
  if (sessionStateMap.find(session_raw) != sessionStateMap.end()) {
    return sessionStateMap.at(session_raw);
  }
  return nullptr;
}

void BfRtCFrontEndSessionState::insertShared(
    std::shared_ptr<BfRtSession> session) {
  std::lock_guard<std::mutex> lock(state_lock);
  if (sessionStateMap.find(session.get()) != sessionStateMap.end()) {
    return;
  }
  sessionStateMap[session.get()] = session;
}

void BfRtCFrontEndSessionState::removeShared(const BfRtSession *session) {
  std::lock_guard<std::mutex> lock(state_lock);
  if (sessionStateMap.find(session) == sessionStateMap.end()) {
    return;
  }
  sessionStateMap.erase(sessionStateMap.find(session));
}

}  // bfrt_c
}  // bfrt

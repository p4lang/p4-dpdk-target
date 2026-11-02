// SPDX-FileCopyrightText: 2021 Intel Corporation
// Copyright (C) 2021 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0
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

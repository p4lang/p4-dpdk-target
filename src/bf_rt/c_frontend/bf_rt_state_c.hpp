// SPDX-FileCopyrightText: 2021 Intel Corporation
// Copyright (C) 2021 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0
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

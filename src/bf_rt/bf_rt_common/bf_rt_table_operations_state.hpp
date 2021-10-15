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
#ifndef _BF_RT_TBL_OPERATIONS_STATE_HPP
#define _BF_RT_TBL_OPERATIONS_STATE_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#ifdef __cplusplus
}
#endif

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <tuple>

#include <bf_rt/bf_rt_table_operations.h>
#include <bf_rt/bf_rt_table_operations.hpp>

namespace bfrt {

class BfRtTableObj;

template <typename T, typename U>
class BfRtStateTableOperations {
 public:
  BfRtStateTableOperations(bf_rt_id_t id) : table_id_(id){};

  void stateTableOperationsSet(T callback_cpp,
                               U callback_c,
                               const void *cookie);
  void stateTableOperationsReset();
  std::tuple<T, U, void *> stateTableOperationsGet();

 private:
  std::mutex state_lock;
  bf_rt_id_t table_id_;
  T callback_cpp_;
  U callback_c_;
  void *cookie_;
};

template <typename T, typename U>
void BfRtStateTableOperations<T, U>::stateTableOperationsSet(
    T callback_cpp, U callback_c, const void *cookie) {
  std::lock_guard<std::mutex> lock(state_lock);
  callback_cpp_ = callback_cpp;
  callback_c_ = callback_c;
  cookie_ = const_cast<void *>(cookie);
}

template <typename T, typename U>
void BfRtStateTableOperations<T, U>::stateTableOperationsReset() {
  std::lock_guard<std::mutex> lock(state_lock);
  callback_cpp_ = nullptr;
  callback_c_ = nullptr;
  cookie_ = nullptr;
}

template <typename T, typename U>
std::tuple<T, U, void *>
BfRtStateTableOperations<T, U>::stateTableOperationsGet() {
  std::lock_guard<std::mutex> lock(state_lock);
  return std::make_tuple(callback_cpp_, callback_c_, cookie_);
}

}  // bfrt

#endif  // _BF_RT_TBL_OPERATIONS_STATE_HPP

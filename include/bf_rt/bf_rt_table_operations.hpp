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
/** @file bf_rt_table_operations.hpp
 *
 *  @brief Contains BF-RT Table Operations APIs
 */
#ifndef _BF_RT_TABLE_OPERATIONS_HPP
#define _BF_RT_TABLE_OPERATIONS_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <functional>

#include <bf_rt/bf_rt_common.h>
#include <bf_rt/bf_rt_session.hpp>

namespace bfrt {

/**
 * @brief Table Operations Type
 */
enum class TableOperationsType {
  /** Update sw value of all counters with the value in hw.
     Applicable on Counters or MATs with direct counters */
  COUNTER_SYNC = 0,
  /** Update sw value of all registers with the value in hw.
     Applicable on Registers or MATs with direct registers */
  REGISTER_SYNC = 1,
  /** Update sw value of all hit state o entries with the actual
     hw status. Applicable MATs with idletimeout POLL mode*/
  HIT_STATUS_UPDATE = 2,
  INVALID = 3
};

/**
 * @brief Register Sync Callback
 * @param[in] dev_tgt Device target
 * @param[in] cookie User registered optional cookie
 */
typedef std::function<void(const bf_rt_target_t &dev_tgt, void *cookie)>
    BfRtRegisterSyncCb;

/**
 * @brief Counter Sync Callback
 * @param[in] dev_tgt Device target
 * @param[in] cookie User registered optional cookie
 */
typedef std::function<void(const bf_rt_target_t &dev_tgt, void *cookie)>
    BfRtCounterSyncCb;

/**
 * @brief Hit State Update Callback
 * @param[in] dev_tgt Device target
 * @param[in] cookie User registered optional cookie
 */
typedef std::function<void(const bf_rt_target_t &dev_tgt, void *cookie)>
    BfRtHitStateUpdateCb;

/**
 * @brief Class to construct Operations Object<br>
 * <B>Creation: </B> Can only be created using \ref
 * bfrt::BfRtTable::operationsAllocate()
 */
class BfRtTableOperations {
 public:
  virtual ~BfRtTableOperations() = default;

  /**
   * @brief Set Register sync callback.
   * Works on Match Action tables and register tables only
   *
   * @param[in] session Session Object
   * @param[in] dev_tgt Device target
   * @param[in] callback Register sync callback
   * @param[in] cookie User cookie
   *
   * @return Status of the API call
   */
  virtual bf_status_t registerSyncSet(const BfRtSession &session,
                                      const bf_rt_target_t &dev_tgt,
                                      const BfRtRegisterSyncCb &callback,
                                      const void *cookie) = 0;

  /**
   * @brief Set Counter sync callback.
   * Works on Match Action tables and counter tables only
   *
   * @param[in] session Session Object
   * @param[in] dev_tgt Device target
   * @param[in] callback Counter sync callback
   * @param[in] cookie User cookie
   *
   * @return Status of the API call
   */
  virtual bf_status_t counterSyncSet(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt,
                                     const BfRtCounterSyncCb &callback,
                                     const void *cookie) = 0;

  /**
   * @brief Set Hit State Update callback.
   * Works on Match Action tables only
   *
   * @param[in] session Session Object
   * @param[in] dev_tgt Device target
   * @param[in] callback Hit State update sync callback
   * @param[in] cookie User cookie
   *
   * @return Status of the API call
   */
  virtual bf_status_t hitStateUpdateSet(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const BfRtHitStateUpdateCb &callback,
                                        const void *cookie) = 0;
};
}  // bfrt

#endif  // _BF_RT_TABLE_OPERATIONS_HPP

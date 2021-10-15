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
#ifndef _BF_RT_PORT_TBL_ATTRIBUTES_STATE_HPP
#define _BF_RT_PORT_TBL_ATTRIBUTES_STATE_HPP

#include <mutex>
#include <unordered_map>

#include <bf_rt/bf_rt_table_attributes.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>

namespace bfrt {

class BfRtTableObj;

int bfRtPortStatusChgInternalCb(bf_dev_id_t dev_id,
                                int dev_port,
                                bool port_up,
                                void *cookie);

class BfRtStateTableAttributesPort {
 public:
  BfRtStateTableAttributesPort(bf_rt_id_t id) : table_id_(id){};

  void stateTableAttributesPortSet(bool enabled,
                                   BfRtPortStatusNotifCb callback_fn,
                                   bf_rt_port_status_chg_cb callback_c,
                                   const BfRtTableObj *table,
                                   void *cookie);

  void stateTableAttributesPortReset();
  std::tuple<bool,
             BfRtPortStatusNotifCb,
             bf_rt_port_status_chg_cb,
             const BfRtTableObj *,
             void *>
  stateTableAttributesPortGet();

 private:
  std::mutex state_lock;
  bf_rt_id_t table_id_;
  bool enabled_;
  BfRtPortStatusNotifCb callback_;
  bf_rt_port_status_chg_cb callback_c_;
  BfRtTableObj *table_;
  void *cookie_;
};

}  // bfrt
#endif  // _BF_RT_PORT_TBL_ATTRIBUTES_STATE_HPP

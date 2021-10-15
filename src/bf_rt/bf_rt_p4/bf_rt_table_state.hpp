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
#ifndef _BF_RT_TABLE_STATE_HPP
#define _BF_RT_TABLE_STATE_HPP

#include "pipe_mgr/pipe_mgr_intf.h"
#include <bf_rt/bf_rt_common.h>

#include <bf_rt_common/bf_rt_table_data_impl.hpp>

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace bfrt {
class BfRtStateActionProfile {
 public:
  using indirResMap = std::map<DataFieldType, bf_rt_id_t>;

  BfRtStateActionProfile(bf_rt_id_t id) : table_id(id){};

  bf_status_t stateActionProfileAdd(const bf_rt_id_t &mem_id,
                                    const bf_dev_pipe_t &pipe_id,
                                    const pipe_act_fn_hdl_t &act_fn_hdl,
                                    const pipe_adt_ent_hdl_t &act_entry_hdl,
                                    indirResMap res_map);

  bf_status_t stateActionProfileModify(const bf_rt_id_t &mem_id,
                                       const bf_dev_pipe_t &pipe_id,
                                       const bf_rt_id_t &act_id,
                                       const pipe_adt_ent_hdl_t &act_entry_hdl,
                                       indirResMap res_map);

  bf_status_t stateActionProfileRemove(const bf_rt_id_t &mem_id,
                                       const bf_dev_pipe_t &pipe_id);

  bf_status_t stateActionProfileGet(const bf_rt_id_t &mem_id,
                                    const bf_dev_pipe_t &pipe_id,
                                    pipe_act_fn_hdl_t *act_fn_hdl,
                                    pipe_adt_ent_hdl_t *act_entry_hdl,
                                    indirResMap *res_map);
  bf_status_t stateActionProfileListGet(
      std::vector<std::pair<bf_rt_id_t, bf_dev_pipe_t>> *mem_id_list);

  bf_status_t getActDetails(const bf_rt_id_t &mem_id,
                            const bf_dev_pipe_t &pipe_id,
                            pipe_adt_ent_hdl_t *adt_ent_hdl);

  bf_status_t getActDetails(const bf_rt_id_t &mem_id,
                            const bf_dev_pipe_t &pipe_id,
                            pipe_adt_ent_hdl_t *adt_ent_hdl,
                            pipe_act_fn_hdl_t *act_fn_hdl);

  bf_status_t getMbrIdFromHndl(pipe_adt_ent_hdl_t adt_ent_hdl,
                               bf_rt_id_t *mbr_id);

  bf_status_t getFirstMbr(bf_dev_pipe_t pipe_id,
                          bf_rt_id_t *first_mbr_id,
                          pipe_adt_ent_hdl_t *first_entry_hdl);

  bf_status_t getNextMbr(bf_rt_id_t mbr_id,
                         bf_dev_pipe_t pipe_id,
                         bf_rt_id_t *next_mbr_id,
                         bf_rt_id_t *next_entry_hdl);

 private:
  bf_rt_id_t table_id;
  std::mutex state_lock;
  //  mem_id  ----------> act_id       :: req by bf_rt
  //    |                   |
  //    V                   V
  //  act_entry_hdl       act_func_hdl :: req by pipe_mgr
  //
  // act_id to act_func_hdl is parsed from contextjson
  // and available from the action structure
  // This is a ordered map in order to serve get first
  std::map<std::pair<bf_rt_id_t, bf_dev_pipe_t>, pipe_adt_ent_hdl_t>
      memid_to_act_entry_map;
  std::map<std::pair<bf_rt_id_t, bf_dev_pipe_t>, bf_rt_id_t>
      memid_to_act_id_map;
  // (mem_id   ------> (type -> resource_id) )
  // one mem_id can have multiple types which will each have one res_id
  std::map<std::pair<bf_rt_id_t, bf_dev_pipe_t>,
           std::map<DataFieldType, bf_rt_id_t>> memid_to_indirect_res_map;
  // A mapping from action entry handle to member id, used during entry read
  std::unordered_map<pipe_adt_ent_hdl_t, std::pair<bf_rt_id_t, bf_dev_pipe_t>>
      act_entry_to_memid_map;
};

class BfRtStateSelector {
 public:
  BfRtStateSelector(bf_rt_id_t tbl_id, bf_rt_id_t act_prof_id)
      : table_id(tbl_id), action_profile_id(act_prof_id){};

  BfRtStateSelector(bf_rt_id_t tbl_id) : table_id(tbl_id){};

  bool grpIdExists(const bf_dev_pipe_t &pipe_id,
                   const bf_rt_id_t &grp_id,
                   pipe_sel_grp_hdl_t *grp_hdl);

  bf_status_t stateSelectorGetGrpSize(const bf_dev_pipe_t &pipe_id,
                                      const bf_rt_id_t &grp_id,
                                      uint32_t *max_size);

  bf_status_t stateSelectorAdd(const bf_dev_pipe_t &pipe_id,
                               const bf_rt_id_t &grp_id,
                               const pipe_sel_grp_hdl_t &grp_hdl,
                               const uint32_t &max_size);

  bf_status_t stateSelectorRemove(const bf_dev_pipe_t &pipe_id,
                                  const bf_rt_id_t &grp_id);

  bf_status_t getGroupIdFromHndl(pipe_sel_grp_hdl_t sel_grp_hdl,
                                 bf_rt_id_t *sel_grp_id);

  bf_status_t getFirstGrp(const bf_dev_pipe_t &pipe_id,
                          bf_rt_id_t *first_grp_id,
                          pipe_sel_grp_hdl_t *first_grp_hdl);

  bf_status_t getNextGrp(const bf_dev_pipe_t &pipe_id,
                         bf_rt_id_t grp_id,
                         bf_rt_id_t *next_grp_id,
                         pipe_sel_grp_hdl_t *next_grp_hdl) const;

  static const pipe_act_fn_hdl_t invalid_act_fn_hdl = 0xdeadbeef;
  static const bf_rt_id_t invalid_group_member = 0xdeadbeef;

 private:
  bf_rt_id_t table_id;
  bf_rt_id_t action_profile_id;
  mutable std::mutex state_lock;
  //  grp_id  ----------> grp_hdl
  // act_id to act_func_hdl is parsed from contextjson
  // and available from the action structure
  // This is a ordered map in order to serve get first
  std::map<std::pair<bf_rt_id_t, bf_dev_pipe_t>, pipe_sel_grp_hdl_t>
      grpid_to_grp_hdl_map;

  std::unordered_map<pipe_sel_grp_hdl_t, std::pair<bf_rt_id_t, bf_dev_pipe_t>>
      grp_hdl_to_grpid_map;

  std::map<std::pair<bf_rt_id_t, bf_dev_pipe_t>, uint32_t>
      grpid_to_max_size_map;
};

// This class stores handles for GetNext_n function calls in case
// of previously returned keys were deleted from device.
class BfRtStateNextRef {
 public:
  BfRtStateNextRef(bf_rt_id_t tbl_id) : table_id(tbl_id){};

  bf_status_t setRef(const bf_rt_id_t &session,
                     const bf_dev_pipe_t &pipe_id,
                     const pipe_mat_ent_hdl_t &mat_ent_hdl);
  bf_status_t getRef(const bf_rt_id_t &session,
                     const bf_dev_pipe_t &pipe_id,
                     pipe_mat_ent_hdl_t *mat_ent_hdl) const;

 private:
  bf_rt_id_t table_id;
  mutable std::mutex state_lock;

  // Store handle for GetNext_n function call per pipe.
  // Key is build using session number in upper 16 bits,
  // and pipe number for lower 16 bits.
  // (session_id << 16 | pipe_id)
  std::unordered_map<uint32_t, pipe_mat_ent_hdl_t> next_ref_;
};

}  // bfrt

#endif  // _BF_RT_TABLE_STATE_HPP

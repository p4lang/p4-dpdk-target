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

#include "bf_rt_table_state.hpp"
#include <bf_rt_common/bf_rt_utils.hpp>

#include <algorithm>

namespace bfrt {

// action profile state APIs

bf_status_t BfRtStateActionProfile::stateActionProfileAdd(
    const bf_rt_id_t &mem_id,
    const bf_dev_pipe_t &pipe_id,
    const pipe_act_fn_hdl_t &act_fn_hdl,
    const pipe_adt_ent_hdl_t &act_entry_hdl,
    indirResMap res_map) {
  std::lock_guard<std::mutex> lock(state_lock);
  if (memid_to_act_entry_map.find(std::make_pair(mem_id, pipe_id)) !=
          memid_to_act_entry_map.end() ||
      memid_to_act_id_map.find(std::make_pair(mem_id, pipe_id)) !=
          memid_to_act_id_map.end() ||
      memid_to_indirect_res_map.find(std::make_pair(mem_id, pipe_id)) !=
          memid_to_indirect_res_map.end()) {
    LOG_ERROR("%s:%d Mem_id %d already exists", __func__, __LINE__, mem_id);
    return BF_ALREADY_EXISTS;
  }

  memid_to_act_id_map[std::make_pair(mem_id, pipe_id)] = act_fn_hdl;
  memid_to_act_entry_map[std::make_pair(mem_id, pipe_id)] = act_entry_hdl;
  act_entry_to_memid_map[act_entry_hdl] = std::make_pair(mem_id, pipe_id);
  memid_to_indirect_res_map[std::make_pair(mem_id, pipe_id)] =
      std::move(res_map);
  return BF_SUCCESS;
}

bf_status_t BfRtStateActionProfile::stateActionProfileModify(
    const bf_rt_id_t &mem_id,
    const bf_dev_pipe_t &pipe_id,
    const bf_rt_id_t &act_id,
    const pipe_adt_ent_hdl_t &act_entry_hdl,
    indirResMap res_map) {
  std::lock_guard<std::mutex> lock(state_lock);
  if (memid_to_act_entry_map.find(std::make_pair(mem_id, pipe_id)) ==
          memid_to_act_entry_map.end() ||
      memid_to_act_id_map.find(std::make_pair(mem_id, pipe_id)) ==
          memid_to_act_id_map.end()) {
    LOG_ERROR("%s:%d Mem_id %d not found", __func__, __LINE__, mem_id);
    return BF_OBJECT_NOT_FOUND;
  }

  memid_to_act_id_map[std::make_pair(mem_id, pipe_id)] = act_id;
  memid_to_act_entry_map[std::make_pair(mem_id, pipe_id)] = act_entry_hdl;
  memid_to_indirect_res_map[std::make_pair(mem_id, pipe_id)] =
      std::move(res_map);
  return BF_SUCCESS;
}

bf_status_t BfRtStateActionProfile::stateActionProfileRemove(
    const bf_rt_id_t &mem_id, const bf_dev_pipe_t &pipe_id) {
  std::lock_guard<std::mutex> lock(state_lock);

  auto it_act_id = memid_to_act_id_map.find(std::make_pair(mem_id, pipe_id));
  auto it_act_ent =
      memid_to_act_entry_map.find(std::make_pair(mem_id, pipe_id));
  auto it_res_id =
      memid_to_indirect_res_map.find(std::make_pair(mem_id, pipe_id));

  // No need to check memid_to_indirect_res_map since it can be empty
  if (it_act_ent == memid_to_act_entry_map.end() ||
      it_act_id == memid_to_act_id_map.end()) {
    LOG_ERROR("%s:%d Mem_id %d not found", __func__, __LINE__, mem_id);
    return BF_OBJECT_NOT_FOUND;
  }

  pipe_act_fn_hdl_t act_fn_hdl = it_act_ent->second;
  auto it_act_mbr = act_entry_to_memid_map.find(act_fn_hdl);
  BF_RT_ASSERT(it_act_mbr != act_entry_to_memid_map.end());

  memid_to_act_id_map.erase(it_act_id);
  memid_to_act_entry_map.erase(it_act_ent);
  act_entry_to_memid_map.erase(act_fn_hdl);

  if (it_res_id != memid_to_indirect_res_map.end()) {
    memid_to_indirect_res_map.erase(it_res_id);
  }
  return BF_SUCCESS;
}

bf_status_t BfRtStateActionProfile::getMbrIdFromHndl(
    pipe_adt_ent_hdl_t adt_ent_hdl, bf_rt_id_t *mbr_id) {
  std::lock_guard<std::mutex> lock(state_lock);

  auto it_act_mbr_id = act_entry_to_memid_map.find(adt_ent_hdl);
  if (it_act_mbr_id == act_entry_to_memid_map.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  *mbr_id = it_act_mbr_id->second.first;
  return BF_SUCCESS;
}

bf_status_t BfRtStateActionProfile::stateActionProfileListGet(
    std::vector<std::pair<bf_rt_id_t, bf_dev_pipe_t>> *mem_id_list) {
  std::lock_guard<std::mutex> lock(state_lock);
  for (const auto &item : memid_to_act_entry_map) {
    mem_id_list->push_back(item.first);
  }
  return BF_SUCCESS;
}
bf_status_t BfRtStateActionProfile::stateActionProfileGet(
    const bf_rt_id_t &mem_id,
    const bf_dev_pipe_t &pipe_id,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_adt_ent_hdl_t *act_entry_hdl,
    indirResMap *res_map) {
  std::lock_guard<std::mutex> lock(state_lock);
  if (memid_to_act_entry_map.find(std::make_pair(mem_id, pipe_id)) ==
          memid_to_act_entry_map.end() ||
      memid_to_act_id_map.find(std::make_pair(mem_id, pipe_id)) ==
          memid_to_act_id_map.end() ||
      memid_to_indirect_res_map.find(std::make_pair(mem_id, pipe_id)) ==
          memid_to_indirect_res_map.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  *act_fn_hdl = memid_to_act_id_map[std::make_pair(mem_id, pipe_id)];
  *act_entry_hdl = memid_to_act_entry_map[std::make_pair(mem_id, pipe_id)];
  *res_map = memid_to_indirect_res_map[std::make_pair(mem_id, pipe_id)];
  return BF_SUCCESS;
}

bf_status_t BfRtStateActionProfile::getActDetails(
    const bf_rt_id_t &mem_id,
    const bf_dev_pipe_t &pipe_id,
    pipe_adt_ent_hdl_t *adt_ent_hdl) {
  std::lock_guard<std::mutex> lock(state_lock);
  if (memid_to_act_entry_map.find(std::make_pair(mem_id, pipe_id)) ==
          memid_to_act_entry_map.end() ||
      memid_to_act_id_map.find(std::make_pair(mem_id, pipe_id)) ==
          memid_to_act_id_map.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  *adt_ent_hdl = memid_to_act_entry_map[std::make_pair(mem_id, pipe_id)];
  return BF_SUCCESS;
}

// Just retrieving action details such as member handle and action function
// handle correposnding to a member id
bf_status_t BfRtStateActionProfile::getActDetails(
    const bf_rt_id_t &mem_id,
    const bf_dev_pipe_t &pipe_id,
    pipe_adt_ent_hdl_t *adt_ent_hdl,
    pipe_act_fn_hdl_t *act_fn_hdl) {
  std::lock_guard<std::mutex> lock(state_lock);
  if (memid_to_act_entry_map.find(std::make_pair(mem_id, pipe_id)) ==
          memid_to_act_entry_map.end() ||
      memid_to_act_id_map.find(std::make_pair(mem_id, pipe_id)) ==
          memid_to_act_id_map.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  *act_fn_hdl = memid_to_act_id_map[std::make_pair(mem_id, pipe_id)];
  *adt_ent_hdl = memid_to_act_entry_map[std::make_pair(mem_id, pipe_id)];
  return BF_SUCCESS;
}

bf_status_t BfRtStateActionProfile::getFirstMbr(
    bf_dev_pipe_t pipe_id,
    bf_rt_id_t *first_mbr_id,
    pipe_adt_ent_hdl_t *first_entry_hdl) {
  std::lock_guard<std::mutex> lock(state_lock);
  auto elem = memid_to_act_entry_map.begin();
  if (elem == memid_to_act_entry_map.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  // Return the first member found in the map for the specified pipe.
  for (; elem != memid_to_act_entry_map.end(); ++elem) {
    if (pipe_id == elem->first.second) {
      *first_mbr_id = elem->first.first;
      *first_entry_hdl = elem->second;
      return BF_SUCCESS;
    }
  }
  return BF_OBJECT_NOT_FOUND;
}

bf_status_t BfRtStateActionProfile::getNextMbr(
    bf_rt_id_t mbr_id,
    bf_dev_pipe_t pipe_id,
    bf_rt_id_t *next_mbr_id,
    pipe_adt_ent_hdl_t *next_entry_hdl) {
  std::lock_guard<std::mutex> lock(state_lock);
  auto elem =
      memid_to_act_entry_map.upper_bound(std::make_pair(mbr_id, pipe_id));
  if (elem == memid_to_act_entry_map.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  // Return the first member found in the map after the specified mbr-id+pipe-id
  // that is also on the specified pipe.
  for (; elem != memid_to_act_entry_map.end(); ++elem) {
    if (pipe_id == elem->first.second) {
      *next_mbr_id = elem->first.first;
      *next_entry_hdl = elem->second;
      return BF_SUCCESS;
    }
  }
  return BF_OBJECT_NOT_FOUND;
}

// selector state APIs
bf_status_t BfRtStateSelector::stateSelectorAdd(
    const bf_dev_pipe_t &pipe_id,
    const bf_rt_id_t &grp_id,
    const pipe_sel_grp_hdl_t &grp_hdl,
    const uint32_t &max_size) {
  std::lock_guard<std::mutex> lock(state_lock);
  if (grpid_to_grp_hdl_map.find(std::make_pair(grp_id, pipe_id)) !=
      grpid_to_grp_hdl_map.end()) {
    LOG_ERROR("%s:%d Grp_id %d already exists", __func__, __LINE__, grp_id);
    return BF_ALREADY_EXISTS;
  }

  grpid_to_grp_hdl_map[std::make_pair(grp_id, pipe_id)] = grp_hdl;
  grp_hdl_to_grpid_map[grp_hdl] = std::make_pair(grp_id, pipe_id);
  grpid_to_max_size_map[std::make_pair(grp_id, pipe_id)] = max_size;

  return BF_SUCCESS;
}

bf_status_t BfRtStateSelector::getGroupIdFromHndl(
    pipe_sel_grp_hdl_t sel_grp_hdl, bf_rt_id_t *sel_grp_id) {
  std::lock_guard<std::mutex> lock(state_lock);
  auto it_sel_grp_id = grp_hdl_to_grpid_map.find(sel_grp_hdl);
  if (it_sel_grp_id == grp_hdl_to_grpid_map.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  *sel_grp_id = it_sel_grp_id->second.first;
  return BF_SUCCESS;
}

bool BfRtStateSelector::grpIdExists(const bf_dev_pipe_t &pipe_id,
                                    const bf_rt_id_t &grp_id,
                                    pipe_sel_grp_hdl_t *grp_hdl) {
  std::lock_guard<std::mutex> lock(state_lock);
  auto res = grpid_to_grp_hdl_map.find(std::make_pair(grp_id, pipe_id));
  if (res != grpid_to_grp_hdl_map.end()) {
    *grp_hdl = res->second;
    return true;
  }
  return false;
}

bf_status_t BfRtStateSelector::stateSelectorGetGrpSize(
    const bf_dev_pipe_t &pipe_id,
    const bf_rt_id_t &grp_id,
    uint32_t *max_size) {
  std::lock_guard<std::mutex> lock(state_lock);
  auto res = grpid_to_max_size_map.find(std::make_pair(grp_id, pipe_id));
  if (res == grpid_to_max_size_map.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  *max_size = res->second;
  return BF_SUCCESS;
}

bf_status_t BfRtStateSelector::stateSelectorRemove(const bf_dev_pipe_t &pipe_id,
                                                   const bf_rt_id_t &grp_id) {
  std::lock_guard<std::mutex> lock(state_lock);

  auto it_grp_hdl = grpid_to_grp_hdl_map.find(std::make_pair(grp_id, pipe_id));

  if (it_grp_hdl == grpid_to_grp_hdl_map.end()) {
    LOG_ERROR("%s:%d GrpId %d not found", __func__, __LINE__, grp_id);
    return BF_OBJECT_NOT_FOUND;
  }

  pipe_sel_grp_hdl_t sel_grp_hdl = it_grp_hdl->second;
  auto it_grp_id = grp_hdl_to_grpid_map.find(sel_grp_hdl);
  BF_RT_ASSERT(it_grp_id != grp_hdl_to_grpid_map.end());

  grpid_to_grp_hdl_map.erase(it_grp_hdl);
  grp_hdl_to_grpid_map.erase(it_grp_id);
  grpid_to_max_size_map.erase(std::make_pair(grp_id, pipe_id));

  return BF_SUCCESS;
}

bf_status_t BfRtStateSelector::getFirstGrp(const bf_dev_pipe_t &pipe_id,
                                           bf_rt_id_t *first_grp_id,
                                           pipe_sel_grp_hdl_t *first_grp_hdl) {
  std::lock_guard<std::mutex> lock(state_lock);
  auto elem = grpid_to_grp_hdl_map.begin();
  if (elem == grpid_to_grp_hdl_map.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  // Return the first member found in the map for the specified pipe.
  for (; elem != grpid_to_grp_hdl_map.end(); ++elem) {
    if (pipe_id == elem->first.second) {
      *first_grp_id = elem->first.first;
      *first_grp_hdl = elem->second;
      return BF_SUCCESS;
    }
  }
  return BF_OBJECT_NOT_FOUND;
}

bf_status_t BfRtStateSelector::getNextGrp(
    const bf_dev_pipe_t &pipe_id,
    bf_rt_id_t grp_id,
    bf_rt_id_t *next_grp_id,
    pipe_sel_grp_hdl_t *next_grp_hdl) const {
  std::lock_guard<std::mutex> lock(state_lock);
  auto elem = grpid_to_grp_hdl_map.upper_bound(std::make_pair(grp_id, pipe_id));
  if (elem == grpid_to_grp_hdl_map.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  // Return the first group found in the map after the specified grp-id+pipe-id
  // that is also on the specified pipe.
  for (; elem != grpid_to_grp_hdl_map.end(); ++elem) {
    if (pipe_id == elem->first.second) {
      *next_grp_id = elem->first.first;
      *next_grp_hdl = elem->second;
      return BF_SUCCESS;
    }
  }
  return BF_OBJECT_NOT_FOUND;
}

// Reference to next object storage functions
bf_status_t BfRtStateNextRef::setRef(const bf_rt_id_t &session,
                                     const bf_dev_pipe_t &pipe_id,
                                     const pipe_mat_ent_hdl_t &mat_ent_hdl) {
  // Key operations assume max pipe number equal to BF_DEV_PIPE_ALL
  uint32_t key = session << 16;
  key |= pipe_id;
  std::lock_guard<std::mutex> lock(state_lock);
  this->next_ref_[key] = mat_ent_hdl;
  return BF_SUCCESS;
}

bf_status_t BfRtStateNextRef::getRef(const bf_rt_id_t &session,
                                     const bf_dev_pipe_t &pipe_id,
                                     pipe_mat_ent_hdl_t *mat_ent_hdl) const {
  // Key operations assume max pipe number equal to BF_DEV_PIPE_ALL
  uint32_t key = session << 16;
  key |= pipe_id;
  std::lock_guard<std::mutex> lock(state_lock);
  auto handle = this->next_ref_.find(key);
  if (handle == this->next_ref_.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  *mat_ent_hdl = handle->second;
  return BF_SUCCESS;
}

}  // bfrt

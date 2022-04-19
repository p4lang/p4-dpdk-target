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
#include <unordered_set>
#include <inttypes.h>

#include <bf_rt_common/bf_rt_init_impl.hpp>
#include <bf_rt_common/bf_rt_table_attributes_impl.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>
#include "bf_rt_table_attributes_state.hpp"

#include <bf_rt_common/bf_rt_pipe_mgr_intf.hpp>
#include "bf_rt_p4_table_data_impl.hpp"
#include "bf_rt_p4_table_impl.hpp"
#include "bf_rt_p4_table_key_impl.hpp"

namespace bfrt {
namespace {

// getActionSpec fetches both resources and action_spec from Pipe Mgr.
// If some resources are unsupported it will filter them out.
bf_status_t getActionSpec(const BfRtSession &session,
                          const dev_target_t &dev_tgt,
                          const uint64_t &flags,
                          const pipe_tbl_hdl_t &pipe_tbl_hdl,
                          const pipe_mat_ent_hdl_t &mat_ent_hdl,
                          uint32_t res_get_flags,
                          pipe_tbl_match_spec_t *pipe_match_spec,
                          pipe_action_spec_t *pipe_action_spec,
                          pipe_act_fn_hdl_t *act_fn_hdl,
                          pipe_res_get_data_t *res_data) {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  bool read_from_hw = false;
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    read_from_hw = true;
  }
  if (pipe_match_spec) {
    status = pipeMgr->pipeMgrGetEntry(session.sessHandleGet(),
                                      pipe_tbl_hdl,
                                      dev_tgt,
                                      mat_ent_hdl,
                                      pipe_match_spec,
                                      pipe_action_spec,
                                      act_fn_hdl,
                                      read_from_hw,
                                      res_get_flags,
                                      res_data);
  } else {
    // Idle resource is not supported on default entries.
    res_get_flags &= ~PIPE_RES_GET_FLAG_IDLE;
    status = pipeMgr->pipeMgrTableGetDefaultEntry(session.sessHandleGet(),
                                                  dev_tgt,
                                                  pipe_tbl_hdl,
                                                  pipe_action_spec,
                                                  act_fn_hdl,
                                                  read_from_hw,
                                                  res_get_flags,
                                                  res_data);
  }
  return status;
}

template <class T>
bf_status_t getStfulSpecFromPipeMgr(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    const pipe_tbl_hdl_t &pipe_tbl_hdl,
                                    const pipe_tbl_hdl_t &pipe_entry_hdl,
                                    const BfRtTable::BfRtTableGetFlag &flag,
                                    T *match_data) {
  int num_pipes = 0;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  bf_status_t status = pipeMgr->pipeStfulDirectQueryGetSizes(
      session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, &num_pipes);

  std::vector<pipe_stful_mem_spec_t> register_pipe_data(num_pipes);
  pipe_stful_mem_query_t stful_query;
  stful_query.data = register_pipe_data.data();
  stful_query.pipe_count = num_pipes;
  uint32_t pipe_api_flags = 0;
  if (flag == BfRtTable::BfRtTableGetFlag::GET_FROM_HW) {
    pipe_api_flags = PIPE_FLAG_SYNC_REQ;
  }
  status = pipeMgr->pipeStfulDirectEntQuery(session.sessHandleGet(),
                                            dev_tgt.dev_id,
                                            pipe_tbl_hdl,
                                            pipe_entry_hdl,
                                            &stful_query,
                                            pipe_api_flags);
  if (status != BF_SUCCESS) {
    return status;
  }
  match_data->getPipeActionSpecObj().setValueRegisterSpec(register_pipe_data);
  return BF_SUCCESS;
}

bf_status_t tableEntryModInternal(const BfRtTableObj &table,
                                  const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableData &data,
                                  const pipe_mat_ent_hdl_t &mat_ent_hdl) {
  bf_status_t status = BF_SUCCESS;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  const BfRtMatchActionTableData &match_data =
      static_cast<const BfRtMatchActionTableData &>(data);
  auto *pipeMgr = PipeMgrIntf::getInstance();
  bf_rt_id_t action_id = 0;
  status = match_data.actionIdGet(&action_id);
  BF_RT_ASSERT(status == BF_SUCCESS);

  std::vector<bf_rt_id_t> dataFields;
  if (match_data.allFieldsSet()) {
    // This function is used for both match action direct and match action
    // indirect tables. Match action direct tables have a non-zero action id and
    // match action indirect tables have a zero action id. Zero action id
    // implies action id is not applicable and appropriate APIs need to be used.
    if (action_id) {
      status = table.dataFieldIdListGet(action_id, &dataFields);
    } else {
      status = table.dataFieldIdListGet(&dataFields);
    }
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                status);
      return status;
    }
  } else {
    dataFields.assign(match_data.getActiveFields().begin(),
                      match_data.getActiveFields().end());
  }

  pipe_action_spec_t pipe_action_spec = {0};
  match_data.copy_pipe_action_spec(&pipe_action_spec);

  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();

  bool direct_resource_found = false;
  bool action_spec_found = false;
  bool ttl_found = false;
  bool direct_counter_found = false;

  // Pipe-mgr exposes different APIs to modify different parts of the data
  // 1. To modify any part of the action spec, pipe_mgr_mat_ent_set_action is
  // the API to use
  //    As part of this following direct resources can be modified
  //      a. LPF
  //      b. WRED
  //      c. METER
  //      d. REGISTER
  // 2. So, if any of the data fields that are to be modified is part of the
  // action spec
  //    the above mentioned direct resources get a free ride.
  // 3. If there are no action parameters to be modified, the resources need to
  // be modified using
  //     the set_resource API.
  // 4. For direct counter resource, pipe_mgr_mat_ent_direct_stat_set is the API
  // to be used.
  // 5. For modifying TTL, a separate API to set the ttl is used.

  for (const auto &dataFieldId : dataFields) {
    const BfRtTableDataField *tableDataField = nullptr;
    if (action_id) {
      status = table.getDataField(dataFieldId, action_id, &tableDataField);
    } else {
      status = table.getDataField(dataFieldId, &tableDataField);
    }
    BF_RT_ASSERT(status == BF_SUCCESS);
    auto fieldTypes = tableDataField->getTypes();
    fieldDestination field_destination =
        BfRtTableDataField::getDataFieldDestination(fieldTypes);
    switch (field_destination) {
      case fieldDestination::DIRECT_LPF:
      case fieldDestination::DIRECT_METER:
      case fieldDestination::DIRECT_WRED:
      case fieldDestination::DIRECT_REGISTER:
        direct_resource_found = true;
        break;

      case fieldDestination::ACTION_SPEC:
        action_spec_found = true;
        break;

      case fieldDestination::TTL:
      case fieldDestination::ENTRY_HIT_STATE:
        ttl_found = true;
        break;

      case fieldDestination::DIRECT_COUNTER:
        direct_counter_found = true;
        break;

      default:
        break;
    }
  }

  if (action_id) {
    // If the caller specified an action id then we need to program the entry to
    // use that action id.  Set action_spec_found to true so that we call
    // pipeMgrMatEntSetAction down below.
    // Note that if the action did not have any action parameters the for loop
    // over data fields would not have found any "action-spec" fields.
    action_spec_found = true;
  }

  if (action_spec_found) {
    BfRtTable::TableType table_type;
    table.tableTypeGet(&table_type);
    if (table_type == BfRtTable::TableType::MATCH_INDIRECT ||
        table_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR) {
      // If we are modifying the action spec for a match action indirect or
      // match action selector table, we need to verify the member ID or the
      // selector group id referenced here is legit.

      const BfRtMatchActionIndirectTableData &match_indir_data =
          static_cast<const BfRtMatchActionIndirectTableData &>(match_data);

      const BfRtMatchActionIndirectTable &mat_indir_table =
          static_cast<const BfRtMatchActionIndirectTable &>(table);
      BfRtStateActionProfile::indirResMap resource_map;

      pipe_adt_ent_hdl_t adt_ent_hdl = 0;
      pipe_sel_grp_hdl_t sel_grp_hdl = 0;
      status = mat_indir_table.getActionState(session,
                                              dev_tgt,
                                              &match_indir_data,
                                              &adt_ent_hdl,
                                              &sel_grp_hdl,
                                              &act_fn_hdl,
                                              &resource_map);

      if (status != BF_SUCCESS) {
        if (match_indir_data.isGroup()) {
          if (sel_grp_hdl == BfRtMatchActionIndirectTableData::invalid_group) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a group id "
                "%d which does not exist in the group table",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                match_indir_data.getGroupId());
            return BF_OBJECT_NOT_FOUND;
          } else if (adt_ent_hdl == BfRtMatchActionIndirectTableData::
                                        invalid_action_entry_hdl) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a group id "
                "%d which does not have any members in the group table "
                "associated with the table",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                match_indir_data.getGroupId());
            return BF_OBJECT_NOT_FOUND;
          }
        } else {
          if (adt_ent_hdl ==
              BfRtMatchActionIndirectTableData::invalid_action_entry_hdl) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a action "
                "member id %d which does not exist in the action profile table",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                match_indir_data.getActionMbrId());
            return BF_OBJECT_NOT_FOUND;
          }
        }
      }
      if (match_indir_data.isGroup()) {
        pipe_action_spec.sel_grp_hdl = sel_grp_hdl;
      } else {
        pipe_action_spec.adt_ent_hdl = adt_ent_hdl;
      }
    }
    status = pipeMgr->pipeMgrMatEntSetAction(session.sessHandleGet(),
                                             pipe_dev_tgt.device_id,
                                             table.tablePipeHandleGet(),
                                             mat_ent_hdl,
                                             act_fn_hdl,
                                             &pipe_action_spec,
                                             0 /* Pipe API flags */);

    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in modifying table data err %d",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                status);
      return status;
    }
  } else if (direct_resource_found) {
    status = pipeMgr->pipeMgrMatEntSetResource(session.sessHandleGet(),
                                               pipe_dev_tgt.device_id,
                                               table.tablePipeHandleGet(),
                                               mat_ent_hdl,
                                               pipe_action_spec.resources,
                                               pipe_action_spec.resource_count,
                                               0 /* Pipe API flags */);

    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR in modifying resources part of table data, err %d",
          __func__,
          __LINE__,
          table.table_name_get().c_str(),
          status);
      return status;
    }
  }

  if (direct_counter_found) {
    const pipe_stat_data_t *stat_data = match_data.getPipeActionSpecObj()
                                            .getCounterSpecObj()
                                            .getPipeCounterSpec();
    status = pipeMgr->pipeMgrMatEntDirectStatSet(
        session.sessHandleGet(),
        pipe_dev_tgt.device_id,
        table.tablePipeHandleGet(),
        mat_ent_hdl,
        const_cast<pipe_stat_data_t *>(stat_data));
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in modifying counter, err %d",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                status);
      return status;
    }
  }

  if (ttl_found) {
    if (table.idleTablePollMode()) {
      status =
          pipeMgr->pipeMgrIdleTimeSetHitState(session.sessHandleGet(),
                                              pipe_dev_tgt.device_id,
                                              table.tablePipeHandleGet(),
                                              mat_ent_hdl,
                                              match_data.get_entry_hit_state());
    } else {
      bool reset = true;
      if (BF_RT_FLAG_IS_SET(flags, BF_RT_SKIP_TTL_RESET)) {
        reset = false;
      }
      status = pipeMgr->pipeMgrMatEntSetIdleTtl(session.sessHandleGet(),
                                                pipe_dev_tgt.device_id,
                                                table.tablePipeHandleGet(),
                                                mat_ent_hdl,
                                                match_data.get_ttl(),
                                                0 /* Pipe API flags */,
                                                reset);
    }
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in modifying entry idle value, err %d",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                status);
      return status;
    }
  }

  return BF_SUCCESS;
}

template <typename T>
bf_status_t getTableUsage(const BfRtSession &session,
                          const bf_rt_target_t &dev_tgt,
                          const uint64_t &flags,
                          const T &table,
                          uint32_t *count) {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status = pipeMgr->pipeMgrGetEntryCount(
      session.sessHandleGet(),
      pipe_dev_tgt,
      table.tablePipeHandleGet(),
      BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW) ? true : false,
      count);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting to usage for table, err %d (%s)",
              __func__,
              __LINE__,
              table.table_name_get().c_str(),
              status,
              bf_err_str(status));
  }
  return status;
}

template <typename T>
bf_status_t getReservedEntries(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const T &table,
                               size_t *size) {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;

  bf_status_t status = pipeMgr->pipeMgrGetReservedEntryCount(
      session.sessHandleGet(), pipe_dev_tgt, table.tablePipeHandleGet(), size);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting reserved entries count for table, err %d "
        "(%s)",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        status,
        bf_err_str(status));
  }
  return status;
}

// Template function for getFirst for Indirect meters, LPF, WRED and register
// tables
template <typename Table, typename Key>
bf_status_t getFirst_for_resource_tbls(const Table &table,
                                       const BfRtSession &session,
                                       const bf_rt_target_t &dev_tgt,
                                       const uint64_t &flags,
                                       Key *key,
                                       BfRtTableData *data) {
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != table.table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  // First entry in a index based table is idx 0
  key->setIdxKey(0);

  return table.tableEntryGet(session, dev_tgt, flags, *key, data);
}

// Template function for getNext_n for Indirect meters, LPF, WRED and register
// tables
template <typename Table, typename Key>
bf_status_t getNext_n_for_resource_tbls(const Table &table,
                                        const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t &flags,
                                        const Key &key,
                                        const uint32_t &n,
                                        BfRtTable::keyDataPairs *key_data_pairs,
                                        uint32_t *num_returned) {
  bf_status_t status = BF_SUCCESS;
  size_t table_size = 0;
  status = table.tableSizeGet(session, dev_tgt, flags, &table_size);
  uint32_t start_key = key.getIdxKey();

  *num_returned = 0;
  uint32_t i = 0;
  uint32_t j = 0;
  for (i = start_key + 1, j = 0; i <= start_key + n; i++, j++) {
    if (i >= table_size) {
      break;
    }
    auto this_key = static_cast<Key *>((*key_data_pairs)[j].first);
    this_key->setIdxKey(i);
    auto this_data = (*key_data_pairs)[j].second;

    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != table.table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table.table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }

    bf_rt_id_t table_id_from_key;
    const BfRtTable *table_from_key;
    this_key->tableGet(&table_from_key);
    table_from_key->tableIdGet(&table_id_from_key);

    if (table_id_from_key != table.table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table key object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table.table_name_get().c_str(),
          table_id_from_key);
      return BF_INVALID_ARG;
    }

    status = table.tableEntryGet(session, dev_tgt, flags, *this_key, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting counter index %d, err %d",
                __func__,
                __LINE__,
                table.table_name_get().c_str(),
                i,
                status);
      // Make the data object null if error
      (*key_data_pairs)[j].second = nullptr;
    }

    (*num_returned)++;
  }
  return BF_SUCCESS;
}

// This function checks if the key idx (applicable for Action profile, selector,
// Indirect meter, Counter, LPF, WRED, Register tables) is within the bounds of
// the size of the table

bool verify_key_for_idx_tbls(const BfRtSession &session,
                             const bf_rt_target_t &dev_tgt,
                             const BfRtTableObj &table,
                             uint32_t idx) {
  size_t table_size;
  table.tableSizeGet(session, dev_tgt, 0, &table_size);
  if (idx < table_size) {
    return true;
  }
  LOG_ERROR("%s:%d %s : ERROR Idx %d for key exceeds the size of the table %zd",
            __func__,
            __LINE__,
            table.table_name_get().c_str(),
            idx,
            table_size);
  return false;
}

template <class Table, class Key>
bf_status_t key_reset(const Table &table, Key *match_key) {
  if (!table.validateTable_from_keyObj(*match_key)) {
    LOG_TRACE("%s:%d %s ERROR : Key object is not associated with the table",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return match_key->reset();
}

bf_status_t tableClearMatCommon(const BfRtSession &session,
                                const bf_rt_target_t &dev_tgt,
                                const bool &&reset_default_entry,
                                const BfRtTableObj *table) {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Clear the table
  bf_status_t status = pipeMgr->pipeMgrMatTblClear(session.sessHandleGet(),
                                                   pipe_dev_tgt,
                                                   table->tablePipeHandleGet(),
                                                   0 /* pipe api flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Failed to clear table %s, err %d",
              __func__,
              __LINE__,
              table->table_name_get().c_str(),
              status);
    return status;
  }
  if (reset_default_entry) {
    status = table->tableDefaultEntryReset(session, dev_tgt, 0);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in resetting default entry , err %d",
                __func__,
                __LINE__,
                table->table_name_get().c_str(),
                status);
    }
  }
  return status;
}

/* @brief
 * This function does the following
 * 1. Get the actual counts of the direct and the indirect resources needed by
 * this data object.
 * 2. Get the programmed counts of resources.
 * 3. Error out if there is a difference between actual indirect res
 * and the programmed indirect res count.
 * 4. Initialize the direct resources if there is a difference between the
 * actual and programmed direct res count.
 */
template <typename T>
bf_status_t resourceCheckAndInitialize(const BfRtTableObj &tbl,
                                       const T &tbl_data,
                                       const bool is_default) {
  // Get the pipe action spec from the data
  T &data = const_cast<T &>(tbl_data);
  PipeActionSpec &pipe_action_spec_obj = data.getPipeActionSpecObj();
  pipe_action_spec_t *pipe_action_spec =
      pipe_action_spec_obj.getPipeActionSpec();
  bf_rt_id_t action_id = 0;
  bool meter = false, reg = false, stat = false;
  auto status = data.actionIdGet(&action_id);
  if (BF_SUCCESS != status) {
    return status;
  }
  tbl.getActionResources(action_id, &meter, &reg, &stat);

  // We can get the indirect count from the data object directly by
  // counting the number of resource index field types set
  // However, for direct count, that is not possible since one
  // resource can have many different fields. We count direct
  // resources set by going through table_ref_map.
  // const auto &actual_indirect_count = tbl_data.indirectResCountGet();
  uint32_t actual_direct_count = 0;

  // Get a map of all the resources(direct and indirect) for this table
  auto &table_ref_map = tbl.getTableRefMap();
  if (table_ref_map.size() == 0) {
    // Nothing to be done. Just return
    return BF_SUCCESS;
  }
  // Iterate over the map to get all the direct resources attached to this
  // table
  std::unordered_set<pipe_tbl_hdl_t> direct_resources;
  for (const auto &ref : table_ref_map) {
    for (const auto &iter : ref.second) {
      if (iter.indirect_ref) {
        continue;
      }
      pipe_hdl_type_t hdl_type =
          static_cast<pipe_hdl_type_t>(PIPE_GET_HDL_TYPE(iter.tbl_hdl));
      switch (hdl_type) {
        case PIPE_HDL_TYPE_STAT_TBL:
          if (stat || !is_default) {
            direct_resources.insert(iter.tbl_hdl);
          }
          break;
        case PIPE_HDL_TYPE_METER_TBL:
          if (meter || !is_default) {
            direct_resources.insert(iter.tbl_hdl);
          }
          break;
        case PIPE_HDL_TYPE_STFUL_TBL:
          if (reg || !is_default) {
            direct_resources.insert(iter.tbl_hdl);
          }
          break;
        default:
          break;
      }
    }
  }
  actual_direct_count = direct_resources.size();

  // Get the number of indirect and direct resources associated with the
  // action_spec. It can be different from the actual counts depending
  // upon whether the user has set it in the action_spec or not
  const auto &programmed_direct_count =
      pipe_action_spec_obj.directResCountGet();
  // const auto &programmed_indirect_count =
  //    pipe_action_spec_obj.indirectResCountGet();

  // if (programmed_indirect_count != actual_indirect_count) {
  //  LOG_ERROR(
  //      "%s:%d %s ERROR Indirect resource should always be set for this table
  //      ",
  //      __func__,
  //      __LINE__,
  //      tbl.table_name_get().c_str());
  //  return BF_INVALID_ARG;
  //}
  if (programmed_direct_count == actual_direct_count) {
    // If the direct resource count is equal to the resource count in the
    // formed pipe action spec, we don't need to do anything. as it means that
    // the user has explicitly set the values of all the resources attached
    // to this table already
    return BF_SUCCESS;
  } else if (is_default && programmed_direct_count > actual_direct_count) {
    // For default entries program only resources that are applicable for
    // the specified action.
    for (int i = 0; i < pipe_action_spec->resource_count; i++) {
      if (direct_resources.find(pipe_action_spec->resources[i].tbl_hdl) ==
          direct_resources.end()) {
        // This means that resource from action_spec is not applicable for
        // this action. Remove it by placing last resource in it's place and
        // decrementing resource count. If resource is present on both don't do
        // anything.
        int last_idx = pipe_action_spec->resource_count - 1;
        pipe_action_spec->resources[i] = pipe_action_spec->resources[last_idx];
        pipe_action_spec->resources[last_idx].tag =
            PIPE_RES_ACTION_TAG_NO_CHANGE;
        pipe_action_spec->resources[last_idx].tbl_hdl = 0;
        pipe_action_spec->resource_count--;
        // Iterate over same entry again, since it was updated.
        i--;
      }
    }
  } else if (programmed_direct_count > actual_direct_count) {
    // This cannot happen. If this happens then it means that we somehow
    // ended up programming more direct resources in the pipe action spec
    // than what actually exist
    LOG_ERROR(
        "%s:%d %s ERROR Pipe action spec has more direct resources "
        "programmed (%d) than the actual direct resources (%d) attached to "
        "that table",
        __func__,
        __LINE__,
        tbl.table_name_get().c_str(),
        programmed_direct_count,
        actual_direct_count);
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  } else {
    // This means that the user has not intialized (via setValue) all the
    // direct resources that are attached to the table. Thus initialize
    // the remaining resources in the pipe action spec, so that the entry
    // gets programmed in all the respective resource managers
    // Remove the resources from the set that have already been
    // initialized
    for (int i = 0; i < pipe_action_spec->resource_count; i++) {
      if (direct_resources.find(pipe_action_spec->resources[i].tbl_hdl) ==
          direct_resources.end()) {
        // This means that we have an indirect resource in the action_spec
        // which we don't care about here. just log debug and continue
        LOG_DBG(
            "%s:%d %s Indirect resource with tbl hdl (%d) found "
            "programmed in the action spec",
            __func__,
            __LINE__,
            tbl.table_name_get().c_str(),
            pipe_action_spec->resources[i].tbl_hdl);
      } else {
        direct_resources.erase(pipe_action_spec->resources[i].tbl_hdl);
      }
    }

    // Now the set will have the remaining direct resoures which need to be
    // initialized in the action spec
    int i = pipe_action_spec->resource_count;
    for (const auto &iter : direct_resources) {
      // Since the pipe action spec already contains zeros for the resources
      // that have not been initialized, we just need to set the tbl_hdl
      // and the tag. No need to set again explicitly set the resource_data
      // to zero
      pipe_action_spec->resources[i].tbl_hdl = iter;
      pipe_action_spec->resources[i].tag = PIPE_RES_ACTION_TAG_ATTACHED;
      i++;
    }
    pipe_action_spec->resource_count = i;
  }
  return BF_SUCCESS;
}

bool checkDefaultOnly(const BfRtTable *table, const BfRtTableData &data) {
  bf_rt_id_t action_id = 0;
  auto bf_status = data.actionIdGet(&action_id);
  if (bf_status != BF_SUCCESS) {
    return false;
  }

  AnnotationSet annotations;
  bf_status = table->actionAnnotationsGet(action_id, &annotations);
  if (bf_status != BF_SUCCESS) {
    return false;
  }
  auto def_an = Annotation("@defaultonly", "");
  if (annotations.find(def_an) != annotations.end()) {
    return true;
  }
  return false;
}

bool checkTableOnly(const BfRtTable *table, const bf_rt_id_t &action_id) {
  if (!action_id) return false;
  AnnotationSet annotations;
  bf_status_t bf_status = table->actionAnnotationsGet(action_id, &annotations);
  if (bf_status != BF_SUCCESS) {
    return false;
  }
  auto def_an = Annotation("@tableonly", "");
  if (annotations.find(def_an) != annotations.end()) {
    return true;
  }
  return false;
}

// Function used to convert pipe_mgr format (which does not use "." and uses "_"
// instead) to naming used in bf-rt.json. Function will return input string if
// related table is not found.
const std::string getQualifiedTableName(const bf_dev_id_t &dev_id,
                                        const std::string &p4_name,
                                        const std::string &tbl_name) {
  const BfRtInfo *info;
  std::vector<const BfRtTable *> tables;
  PipelineProfInfoVec pipe_info;
  BfRtDevMgr::getInstance().bfRtInfoGet(dev_id, p4_name, &info);
  info->bfRtInfoPipelineInfoGet(&pipe_info);
  info->bfrtInfoGetTables(&tables);
  for (auto const table : tables) {
    // For comparison convert name to pipe_mgr format. No pipe name prefix and
    // "_" instead of ".".
    std::string name;
    table->tableNameGet(&name);
    std::string name_cmp = name;
    for (auto pinfo : pipe_info) {
      std::string prefix = pinfo.first;
      // Remove pipe name and trailing "." if present.
      if (name_cmp.rfind(prefix, 0) == 0)
        name_cmp.erase(0, prefix.length() + 1);
      std::replace(name_cmp.begin(), name_cmp.end(), '.', '_');
      if (!tbl_name.compare(name_cmp)) {
        return name;
      }
    }
  }
  // Return same name if not found
  return tbl_name;
}

std::string getPipeMgrTblName(const std::string &orig_name) {
  std::string tbl_name = orig_name;
  // Remove till first dot. Internal tables do not have dots in the name.
  tbl_name.erase(0, tbl_name.find(".") + 1);
  std::replace(tbl_name.begin(), tbl_name.end(), '.', '_');
  return tbl_name;
}

// Must be in line with bf_tbl_dbg_counter_type_t
const std::vector<std::string> cntTypeStr{"DISABLED",
                                          "TBL_MISS",
                                          "TBL_HIT",
                                          "GW_TBL_MISS",
                                          "GW_TBL_HIT",
                                          "GW_TBL_INHIBIT"};

}  // anonymous namespace

// BfRtMatchActionTable ******************
void BfRtMatchActionTable::setActionResources(const bf_dev_id_t &dev_id) {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  for (auto const &i : this->act_fn_hdl_to_id) {
    bool has_cntr, has_meter, has_lpf, has_wred, has_reg;
    has_cntr = has_meter = has_lpf = has_wred = has_reg = false;
    bf_status_t sts =
        pipeMgr->pipeMgrGetActionDirectResUsage(dev_id,
                                                this->pipe_tbl_hdl,
                                                i.first,
                                                &has_cntr,
                                                &has_meter,
                                                &has_lpf,
                                                &has_wred,
                                                &has_reg);
    if (sts) {
      LOG_ERROR("%s:%d %s Cannot fetch action resource information, %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                sts);
      continue;
    }
    this->act_uses_dir_cntr[i.second] = has_cntr;
    this->act_uses_dir_meter[i.second] = has_meter || has_lpf || has_wred;
    this->act_uses_dir_reg[i.second] = has_reg;
  }
}

bf_status_t BfRtMatchActionTable::tableEntryAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  const BfRtMatchActionTableData &match_data =
      static_cast<const BfRtMatchActionTableData &>(data);

  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Initialize the direct resources if they were not provided by the caller.
  auto status = resourceCheckAndInitialize<BfRtMatchActionTableData>(
      *this, match_data, false);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to initialize Direct resources",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  if (checkDefaultOnly(this, data)) {
    LOG_TRACE("%s:%d %s Error adding action because it is defaultOnly",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  match_key.populate_match_spec(&pipe_match_spec);
  pipe_action_spec = match_data.get_pipe_action_spec();
  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  uint32_t ttl = match_data.get_ttl();
  if (idle_table_state->isIdleTableinPollMode()) {
    /* In poll mode non-zero ttl means that entry should be marked as active */
    ttl = (match_data.get_entry_hit_state() == ENTRY_ACTIVE) ? 1 : 0;
  }
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return pipeMgr->pipeMgrMatEntAdd(session.sessHandleGet(),
                                   pipe_dev_tgt,
                                   pipe_tbl_hdl,
                                   &pipe_match_spec,
                                   act_fn_hdl,
                                   pipe_action_spec,
                                   ttl,
                                   0 /* Pipe API flags */,
                                   &pipe_entry_hdl);
}

bf_status_t BfRtMatchActionTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_mat_ent_hdl_t pipe_entry_hdl;
  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return tableEntryModInternal(
      *this, session, dev_tgt, flags, data, pipe_entry_hdl);
}

bf_status_t BfRtMatchActionTable::tableEntryDel(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t & /*flags*/,
                                                const BfRtTableKey &key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status =
      pipeMgr->pipeMgrMatEntDelByMatchSpec(session.sessHandleGet(),
                                           pipe_dev_tgt,
                                           pipe_tbl_hdl,
                                           &pipe_match_spec,
                                           0 /* Pipe api flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Entry delete failed for table %s, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtMatchActionTable::tableClear(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/) const {
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return tableClearMatCommon(session, dev_tgt, true, this);
}

bf_status_t BfRtMatchActionTable::tableDefaultEntrySet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionTableData &match_data =
      static_cast<const BfRtMatchActionTableData &>(data);
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  bf_rt_id_t action_id;
  status = match_data.actionIdGet(&action_id);
  if (status != BF_SUCCESS) {
    BF_RT_DBGCHK(status == BF_SUCCESS);
    return status;
  }
  if (checkTableOnly(this, action_id)) {
    LOG_TRACE("%s:%d %s Error adding action because it is tableOnly",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Check which direct resources were provided.
  bool direct_reg = false, direct_cntr = false, direct_mtr = false;
  if (action_id) {
    this->getActionResources(action_id, &direct_mtr, &direct_reg, &direct_cntr);
  } else {
    std::vector<bf_rt_id_t> dataFields;
    if (match_data.allFieldsSet()) {
      status = this->dataFieldIdListGet(&dataFields);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  status);
        return status;
      }
    } else {
      dataFields.assign(match_data.getActiveFields().begin(),
                        match_data.getActiveFields().end());
    }
    for (const auto &dataFieldId : dataFields) {
      const BfRtTableDataField *field = nullptr;
      status = this->getDataField(dataFieldId, &field);
      BF_RT_DBGCHK(status == BF_SUCCESS);
      fieldDestination dest =
          BfRtTableDataField::getDataFieldDestination(field->getTypes());
      switch (dest) {
        case fieldDestination::DIRECT_REGISTER:
          direct_reg = true;
          break;
        case fieldDestination::DIRECT_COUNTER:
          direct_cntr = true;
          break;
        case fieldDestination::DIRECT_METER:
        case fieldDestination::DIRECT_WRED:
        case fieldDestination::DIRECT_LPF:
          direct_mtr = true;
          break;
        default:
          break;
      }
    }
  }

  pipe_mat_ent_hdl_t entry_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  if (action_id) {
    // Initialize the direct resources if they were not provided by the
    // caller.
    status = resourceCheckAndInitialize<BfRtMatchActionTableData>(
        *this, match_data, true);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR Failed to initialize direct resources",
                __func__,
                __LINE__,
                this->table_name_get().c_str());
      return status;
    }

    pipe_action_spec = match_data.get_pipe_action_spec();
    pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();

    status = pipeMgr->pipeMgrMatDefaultEntrySet(session.sessHandleGet(),
                                                pipe_dev_tgt,
                                                pipe_tbl_hdl,
                                                act_fn_hdl,
                                                pipe_action_spec,
                                                0 /* Pipe API flags */,
                                                &entry_hdl);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error setting default entry, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->table_name_get().c_str(),
                status);
      return status;
    }
  } else {
    // Get the handle of the default entry.
    status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
        session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, &entry_hdl);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error getting entry handle, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->table_name_get().c_str(),
                status);
      return status;
    }
  }

  // Program direct counters if requested.
  if (direct_cntr) {
    const pipe_stat_data_t *stat_data = match_data.getPipeActionSpecObj()
                                            .getCounterSpecObj()
                                            .getPipeCounterSpec();
    status = pipeMgr->pipeMgrMatEntDirectStatSet(
        session.sessHandleGet(),
        pipe_dev_tgt.device_id,
        pipe_tbl_hdl,
        entry_hdl,
        const_cast<pipe_stat_data_t *>(stat_data));
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error setting direct stats, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->table_name_get().c_str(),
                status);
      return status;
    }
  }
  // Program direct meter/wred/lpf and registers only if we did not do a set
  // above.
  if (!action_id && (direct_reg || direct_mtr)) {
    pipe_action_spec = match_data.get_pipe_action_spec();
    status = pipeMgr->pipeMgrMatEntSetResource(
        session.sessHandleGet(),
        pipe_dev_tgt.device_id,
        pipe_tbl_hdl,
        entry_hdl,
        const_cast<pipe_res_spec_t *>(pipe_action_spec->resources),
        pipe_action_spec->resource_count,
        0 /* Pipe API flags */);
    if (BF_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error setting direct resources, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->table_name_get().c_str(),
                status);
      return status;
    }
  }
  return status;
}

bf_status_t BfRtMatchActionTable::tableDefaultEntryReset(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  return pipeMgr->pipeMgrMatTblDefaultEntryReset(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, 0);
}

bf_status_t BfRtMatchActionTable::tableEntryGet(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                const BfRtTableKey &key,
                                                BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return tableEntryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, &pipe_match_spec, data);
}

bf_status_t BfRtMatchActionTable::tableDefaultEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, &pipe_entry_hdl);
  if (BF_SUCCESS != status) {
    LOG_TRACE("%s:%d %s Dev %d pipe %x error %d getting default entry",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              status);
    return status;
  }
  return tableEntryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, nullptr, data);
}

bf_status_t BfRtMatchActionTable::tableEntryKeyGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_tbl_match_spec_t *pipe_match_spec;
  bf_dev_pipe_t entry_pipe;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  bf_status_t status = pipeMgr->pipeMgrEntHdlToMatchSpec(
      session.sessHandleGet(),
      pipe_dev_tgt,
      pipe_tbl_hdl,
      entry_handle,
      &entry_pipe,
      const_cast<const pipe_tbl_match_spec_t **>(&pipe_match_spec));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  entry_tgt->pipe_id = entry_pipe;
  match_key->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  return status;
}

bf_status_t BfRtMatchActionTable::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        entry_handle);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtMatchActionTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const bf_rt_handle_t &entry_handle,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key->populate_match_spec(&pipe_match_spec);

  auto status = tableEntryGet_internal(
      session, dev_tgt, flags, entry_handle, &pipe_match_spec, data);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

bf_status_t BfRtMatchActionTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  bf_status_t status;
  bool store_entries;

  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_tbl_match_spec_t pipe_match_spec;
  std::memset(&pipe_match_spec, 0, sizeof(pipe_tbl_match_spec_t));

  status = pipeMgr->pipeMgrStoreEntries(session.sessHandleGet(),
					pipe_tbl_hdl, pipe_dev_tgt,
					&store_entries);
  if (status) {
    LOG_ERROR("Checking if entries are stored in SDE failed.");
    return status;
  }
  if (store_entries) {
    // Get the first entry handle present in pipe-mgr
    uint32_t first_entry_handle;
    status = pipeMgr->pipeMgrGetFirstEntryHandle(
            session.sessHandleGet(), pipe_tbl_hdl, pipe_dev_tgt,
	    &first_entry_handle);

    if (status == BF_OBJECT_NOT_FOUND) {
        return status;
    } else if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
                            __func__,
                            __LINE__,
                            table_name_get().c_str(),
                            status);
    }

    match_key->populate_match_spec(&pipe_match_spec);

    status = tableEntryGet_internal(
            session, dev_tgt, flags, first_entry_handle, &pipe_match_spec, data);
    if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in getting first entry, err %d",
                            __func__,
                            __LINE__,
                            table_name_get().c_str(),
                            status);
    }
    match_key->populate_key_from_match_spec(pipe_match_spec);
    // Store ref point for GetNext_n to follow.
    auto device_state =
            BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
    auto nextRef = device_state->nextRefState.getObjState(this->table_id_get());
    nextRef->setRef(session.sessHandleGet(), dev_tgt.pipe_id, first_entry_handle);
  } else {
      BfRtMatchActionTableData *match_data =
        static_cast<BfRtMatchActionTableData *>(data);
      pipe_action_spec_t *pipe_action_spec = nullptr;
      bf_rt_id_t req_action_id = 0;
      pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;

      match_key->populate_match_spec(&pipe_match_spec);

      std::vector<bf_rt_id_t> dataFields;
      status = match_data->actionIdGet(&req_action_id);
      BF_RT_ASSERT(status == BF_SUCCESS);

      match_data->reset();
      pipe_action_spec = match_data->get_pipe_action_spec();
      status = pipeMgr->pipeMgrGetFirstEntry(
                  session.sessHandleGet(), pipe_tbl_hdl, pipe_dev_tgt,
                  &pipe_match_spec, pipe_action_spec, &pipe_act_fn_hdl);

      if (status == BF_OBJECT_NOT_FOUND) {
            return status;
      } else if (status != BF_SUCCESS) {
            LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
                                          __func__,
                                          __LINE__,
                                          table_name_get().c_str(),
                                          status);
      }
      match_key->populate_key_from_match_spec(pipe_match_spec);
      /* TODO: Build Action Data from the ActionSpec and re-enable this code. */
#if 0
      bf_rt_id_t action_id = this->getActIdFromActFnHdl(pipe_act_fn_hdl);
      if (req_action_id && req_action_id != action_id) {
         // Keeping this log as warning for iteration purposes.
         // Caller can decide to throw an error if required
         LOG_TRACE("%s:%d %s ERROR expecting action ID to be %d but recvd %d ",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              req_action_id,
              action_id);
         return BF_INVALID_ARG;
      }
#endif
      bf_rt_id_t action_id;
      this->actionIdGet("NoAction", &action_id);
      match_data->actionIdSet(action_id);
      match_data->setActiveFields(dataFields);
  }

  return status;
}

bf_status_t BfRtMatchActionTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  bf_status_t status;
  bool store_entries;

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  status = pipeMgr->pipeMgrStoreEntries(session.sessHandleGet(),
					pipe_tbl_hdl, pipe_dev_tgt,
					&store_entries);
  if (status) {
    LOG_ERROR("Checking if entries are stored in SDE failed.");
    return status;
  }
  if (store_entries) {
    status =
        pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                          pipe_dev_tgt,
                                          pipe_tbl_hdl,
                                          &pipe_match_spec,
                                          &pipe_entry_hdl);
    // If key is not found and this is subsequent call, API should continue
    // from previous call.
    if (status == BF_OBJECT_NOT_FOUND) {
      // Warn the user that currently used key no longer exist.
      LOG_WARN("%s:%d %s Provided key does not exist, trying previous handle",
               __func__,
               __LINE__,
               table_name_get().c_str());
      auto device_state =
          BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
      auto nextRef = device_state->nextRefState.getObjState(this->table_id_get());
      status = nextRef->getRef(
          session.sessHandleGet(), dev_tgt.pipe_id, &pipe_entry_hdl);
    }

    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }
    std::vector<uint32_t> next_entry_handles(n, 0);

    status = pipeMgr->pipeMgrGetNextEntryHandles(session.sessHandleGet(),
                                                 pipe_tbl_hdl,
                                                 pipe_dev_tgt,
                                                 pipe_entry_hdl,
                                                 n,
                                                 next_entry_handles.data());
    if (status == BF_OBJECT_NOT_FOUND) {
      if (num_returned) {
        *num_returned = 0;
      }
      return BF_SUCCESS;
    }
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR in getting next entry handles from pipe-mgr, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          status);
      return status;
    }
    pipe_tbl_match_spec_t match_spec;
    unsigned i = 0;
    for (i = 0; i < n; i++) {
      std::memset(&match_spec, 0, sizeof(pipe_tbl_match_spec_t));
      auto this_key =
          static_cast<BfRtMatchActionKey *>((*key_data_pairs)[i].first);
      auto this_data = (*key_data_pairs)[i].second;
      bf_rt_id_t table_id_from_data;
      const BfRtTable *table_from_data;
      this_data->getParent(&table_from_data);
      table_from_data->tableIdGet(&table_id_from_data);

      if (table_id_from_data != this->table_id_get()) {
        LOG_TRACE(
            "%s:%d %s ERROR : Table Data object with object id %d  does not "
            "match "
            "the table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            table_id_from_data);
        return BF_INVALID_ARG;
      }
      if (next_entry_handles[i] == (uint32_t)-1) {
        break;
      }
      this_key->populate_match_spec(&match_spec);
      status = tableEntryGet_internal(
          session, dev_tgt, flags, next_entry_handles[i], &match_spec, this_data);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  i + 1,
                  status);
        // Make the data object null if error
        (*key_data_pairs)[i].second = nullptr;
      }
      this_key->setPriority(match_spec.priority);
      this_key->setPartitionIndex(match_spec.partition_index);
    }
    auto device_state =
        BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
    auto nextRef = device_state->nextRefState.getObjState(this->table_id_get());
    // Even if tableEntryGet failed, since pipe_mgr can fetch next n handles
    // starting from non-existing handle, just store last one that was used.
    nextRef->setRef(
        session.sessHandleGet(), dev_tgt.pipe_id, next_entry_handles[i - 1]);

    if (num_returned) {
      *num_returned = i;
    }
  } else {
      uint32_t num;
      status = tableEntryGetNextNByKey(session, dev_tgt, flags,
				       &pipe_match_spec, n, key_data_pairs,
				       &num);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in getting entries from pipe-mgr, err %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  status);
      }
      if (num_returned) {
        *num_returned = num;
      }
  }
  return status;
}

bf_status_t BfRtMatchActionTable::tableSizeGet(const BfRtSession &session,
                                               const bf_rt_target_t &dev_tgt,
                                               const uint64_t & /*flags*/,
                                               size_t *count) const {
  bf_status_t status = BF_SUCCESS;
  size_t reserved = 0;
  status = getReservedEntries(session, dev_tgt, *(this), &reserved);
  *count = this->_table_size - reserved;
  return status;
}

bf_status_t BfRtMatchActionTable::tableUsageGet(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

bf_status_t BfRtMatchActionTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtMatchActionKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  return dataAllocate_internal(0, data_ret, fields);
}

bf_status_t BfRtMatchActionTable::dataAllocate(
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  // Create a empty vector to indicate all fields are needed
  std::vector<bf_rt_id_t> fields;
  return dataAllocate_internal(action_id, data_ret, fields);
}
bf_status_t BfRtMatchActionTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  return dataAllocate_internal(0, data_ret, fields);
}

bf_status_t BfRtMatchActionTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  return dataAllocate_internal(action_id, data_ret, fields);
}

bf_status_t dataReset_internal(const BfRtTableObj &table,
                               const bf_rt_id_t &action_id,
                               const std::vector<bf_rt_id_t> &fields,
                               BfRtTableData *data) {
  BfRtTableDataObj *data_obj = static_cast<BfRtTableDataObj *>(data);
  if (!table.validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return data_obj->reset(action_id, fields);
}

bf_status_t BfRtMatchActionTable::keyReset(BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  return key_reset<BfRtMatchActionTable, BfRtMatchActionKey>(*this, match_key);
}

bf_status_t BfRtMatchActionTable::dataReset(BfRtTableData *data) const {
  std::vector<bf_rt_id_t> fields;
  return dataReset_internal(*this, 0, fields, data);
}

bf_status_t BfRtMatchActionTable::dataReset(const bf_rt_id_t &action_id,
                                            BfRtTableData *data) const {
  std::vector<bf_rt_id_t> fields;
  return dataReset_internal(*this, action_id, fields, data);
}

bf_status_t BfRtMatchActionTable::dataReset(
    const std::vector<bf_rt_id_t> &fields, BfRtTableData *data) const {
  return dataReset_internal(*this, 0, fields, data);
}

bf_status_t BfRtMatchActionTable::dataReset(
    const std::vector<bf_rt_id_t> &fields,
    const bf_rt_id_t &action_id,
    BfRtTableData *data) const {
  return dataReset_internal(*this, action_id, fields, data);
}

bf_status_t BfRtMatchActionTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  if (type == TableAttributesType::IDLE_TABLE_RUNTIME) {
    LOG_TRACE(
        "%s:%d %s Idle Table Runtime Attribute requires a Mode, please use the "
        "appropriate API to include the idle table mode",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::attributeAllocate(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  if (type != TableAttributesType::IDLE_TABLE_RUNTIME) {
    LOG_TRACE("%s:%d %s Idle Table Mode cannot be set for Attribute %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type, idle_table_mode));
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(type));
      return BF_INVALID_ARG;
    case TableAttributesType::ENTRY_SCOPE:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      break;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtMatchActionTable::attributeReset(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      break;
    case TableAttributesType::ENTRY_SCOPE:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(type));
      return BF_INVALID_ARG;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type, idle_table_mode);
}

bf_status_t BfRtMatchActionTable::tableAttributesSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(0);
      bf_status_t sts = tbl_attr_impl->entryScopeParamsGet(
          &entry_scope,
          static_cast<BfRtTableEntryScopeArguments *>(&scope_args));
      if (sts != BF_SUCCESS) {
        return sts;
      }
      // Call the pipe mgr API to set entry scope
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      // Derive pipe mgr entry scope from BFRT entry scope and set it to
      // property value
      prop_val.value =
          entry_scope == TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
              ? PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
              : entry_scope == TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                    ? PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                    : PIPE_MGR_ENTRY_SCOPE_USER_DEFINED;
      std::bitset<32> bitval;
      scope_args.getValue(&bitval);
      args_val.value = bitval.to_ulong();
      auto *pipeMgr = PipeMgrIntf::getInstance();
      sts = pipeMgr->pipeMgrTblSetProperty(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           prop_type,
                                           prop_val,
                                           args_val);
      if (sts == BF_SUCCESS) {
        auto device_state =
            BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
        if (device_state == nullptr) {
          BF_RT_ASSERT(0);
          return BF_OBJECT_NOT_FOUND;
        }

        // Set ENTRY_SCOPE of the table
        auto attributes_state =
            device_state->attributesState.getObjState(table_id_get());
        attributes_state->setEntryScope(entry_scope);
      }
      return sts;
    }
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      bf_status_t sts = tbl_attr_impl->meterByteCountAdjGet(&byte_count);
      if (sts != BF_SUCCESS) {
        return sts;
      }
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      auto *pipeMgr = PipeMgrIntf::getInstance();
      // Just pass in any Meter related field type to get meter_hdl.
      pipe_tbl_hdl_t res_hdl =
          getResourceHdl(DataFieldType::METER_SPEC_CIR_PPS);
      return pipeMgr->pipeMgrMeterByteCountSet(
          session.sessHandleGet(), pipe_dev_tgt, res_hdl, byte_count);
    }
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::tableAttributesGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      auto *pipeMgr = PipeMgrIntf::getInstance();
      auto sts = pipeMgr->pipeMgrTblGetProperty(session.sessHandleGet(),
                                                dev_tgt.dev_id,
                                                pipe_tbl_hdl,
                                                prop_type,
                                                &prop_val,
                                                &args_val);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get entry scope from pipe_mgr",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return sts;
      }

      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(args_val.value);

      // Derive BFRT entry scope from pipe mgr entry scope
      entry_scope = prop_val.value == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
                        ? TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
                        : prop_val.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                              ? TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                              : TableEntryScope::ENTRY_SCOPE_USER_DEFINED;

      return tbl_attr_impl->entryScopeParamsSet(
          entry_scope, static_cast<BfRtTableEntryScopeArguments &>(scope_args));
    }
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      auto *pipeMgr = PipeMgrIntf::getInstance();
      // Just pass in any Meter related field type to get meter_hdl.
      pipe_tbl_hdl_t res_hdl =
          getResourceHdl(DataFieldType::METER_SPEC_CIR_PPS);
      auto sts = pipeMgr->pipeMgrMeterByteCountGet(
          session.sessHandleGet(), pipe_dev_tgt, res_hdl, &byte_count);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get meter bytecount adjust from pipe_mgr",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return sts;
      }
      return tbl_attr_impl->meterByteCountAdjSet(byte_count);
    }
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

// Unexposed functions

void BfRtMatchActionTable::setIsTernaryTable(const bf_dev_id_t &dev_id) {
  auto status = PipeMgrIntf::getInstance()->pipeMgrTblIsTern(
      dev_id, pipe_tbl_hdl, &this->is_ternary_table_);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to find whether table is ternary or not",
              __func__,
              __LINE__,
              table_name_get().c_str());
  }
}

bf_status_t BfRtMatchActionTable::dataAllocate_internal(
    bf_rt_id_t action_id,
    std::unique_ptr<BfRtTableData> *data_ret,
    const std::vector<bf_rt_id_t> &fields) const {
  if (action_id) {
    if (action_info_list.find(action_id) == action_info_list.end()) {
      LOG_TRACE("%s:%d Action_ID %d not found", __func__, __LINE__, action_id);
      return BF_OBJECT_NOT_FOUND;
    }
    *data_ret = std::unique_ptr<BfRtTableData>(
        new BfRtMatchActionTableData(this, action_id, fields));
  } else {
    *data_ret = std::unique_ptr<BfRtTableData>(
        new BfRtMatchActionTableData(this, fields));
  }

  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::tableEntryGetNextNByKey(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    pipe_tbl_match_spec_t *pipe_match_spec,
    uint32_t n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const{
  uint32_t i;
  bf_status_t status = BF_SUCCESS;
  auto match_specs = std::unique_ptr<pipe_tbl_match_spec_t[]>{ new pipe_tbl_match_spec_t[n] };
  auto action_specs = std::unique_ptr<struct pipe_action_spec*[]>{ new struct pipe_action_spec*[n] };
  auto act_fn_hdls = std::unique_ptr<uint32_t[]>{ new uint32_t[n]};
  bf_rt_id_t action_id;

  for (i = 0; i < n; i++) {
    auto this_key =
          static_cast<BfRtMatchActionKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    this_key->populate_match_spec(&match_specs.get()[i]);
    BfRtMatchActionTableData *match_data =
      static_cast<BfRtMatchActionTableData *>(this_data);

    match_data->reset();
    action_specs.get()[i] = match_data->get_pipe_action_spec();
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  status = pipeMgr->pipeMgrGetNextNByKey(session.sessHandleGet(), pipe_tbl_hdl,
				pipe_dev_tgt, pipe_match_spec, n,
				match_specs.get(), action_specs.get(),
				act_fn_hdls.get(), num_returned);
  if (status == BF_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  for (i = 0; i < *num_returned; i++) {
    auto this_data = (*key_data_pairs)[i].second;
    BfRtMatchActionTableData *match_data =
      static_cast<BfRtMatchActionTableData *>(this_data);

    std::vector<bf_rt_id_t> dataFields;
    /* TODO: Build Action Data from the ActionSpec and return correct action. */
    this->actionIdGet("NoAction", &action_id);
    match_data->actionIdSet(action_id);
    match_data->setActiveFields(dataFields);
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const pipe_mat_ent_hdl_t &pipe_entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_rt_id_t req_action_id = 0;
  bool stful_fetched = false;

  bf_rt_id_t ttl_field_id = 0;
  bf_rt_id_t hs_field_id = 0;

  uint32_t res_get_flags = 0;
  pipe_res_get_data_t res_data;
  res_data.stful.data = nullptr;
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
  pipe_action_spec_t *pipe_action_spec = nullptr;
  bool all_fields_set = false;

  BfRtMatchActionTableData *match_data =
      static_cast<BfRtMatchActionTableData *>(data);
  std::vector<bf_rt_id_t> dataFields;
  status = match_data->actionIdGet(&req_action_id);
  BF_RT_ASSERT(status == BF_SUCCESS);

  all_fields_set = match_data->allFieldsSet();
  if (all_fields_set) {
    res_get_flags = PIPE_RES_GET_FLAG_ALL;
    // do not assign dataFields in this case because we might not know
    // what the actual_action_id is yet
    // Based upon the actual action id, we will be filling in the data fields
    // later anyway
  } else {
    dataFields.assign(match_data->getActiveFields().begin(),
                      match_data->getActiveFields().end());
    for (const auto &dataFieldId : dataFields) {
      const BfRtTableDataField *tableDataField = nullptr;
      if (req_action_id) {
        status = getDataField(dataFieldId, req_action_id, &tableDataField);
      } else {
        status = getDataField(dataFieldId, &tableDataField);
      }
      BF_RT_ASSERT(status == BF_SUCCESS);
      auto fieldTypes = tableDataField->getTypes();
      fieldDestination field_destination =
          BfRtTableDataField::getDataFieldDestination(fieldTypes);
      switch (field_destination) {
        case fieldDestination::DIRECT_LPF:
        case fieldDestination::DIRECT_METER:
        case fieldDestination::DIRECT_WRED:
          res_get_flags |= PIPE_RES_GET_FLAG_METER;
          break;
        case fieldDestination::DIRECT_REGISTER:
          res_get_flags |= PIPE_RES_GET_FLAG_STFUL;
          break;
        case fieldDestination::ACTION_SPEC:
          res_get_flags |= PIPE_RES_GET_FLAG_ENTRY;
          break;
        case fieldDestination::DIRECT_COUNTER:
          res_get_flags |= PIPE_RES_GET_FLAG_CNTR;
          break;
        case fieldDestination::ENTRY_HIT_STATE:
        case fieldDestination::TTL:
          res_get_flags |= PIPE_RES_GET_FLAG_IDLE;
          break;
        default:
          break;
      }
    }
  }
  // All inputs from the data object have been processed. Now reset it
  // for out data purpose
  // We reset the data object with act_id 0 and all fields
  match_data->reset();
  pipe_action_spec = match_data->get_pipe_action_spec();

  status = getActionSpec(session,
                         pipe_dev_tgt,
                         flags,
                         pipe_tbl_hdl,
                         pipe_entry_hdl,
                         res_get_flags,
                         pipe_match_spec,
                         pipe_action_spec,
                         &pipe_act_fn_hdl,
                         &res_data);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        pipe_entry_hdl,
        status);
    return status;
  }

  bf_rt_id_t action_id = this->getActIdFromActFnHdl(pipe_act_fn_hdl);
  if (req_action_id && req_action_id != action_id) {
    // Keeping this log as warning for iteration purposes.
    // Caller can decide to throw an error if required
    LOG_TRACE("%s:%d %s ERROR expecting action ID to be %d but recvd %d ",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              req_action_id,
              action_id);
    return BF_INVALID_ARG;
  }

  match_data->actionIdSet(action_id);
  // Get the list of dataFields for action_id. The list of active fields needs
  // to be set
  if (all_fields_set) {
    status = dataFieldIdListGet(action_id, &dataFields);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                status);
      return status;
    }
    std::vector<bf_rt_id_t> empty;
    match_data->setActiveFields(empty);
  } else {
    // dataFields has already been populated
    // with the correct fields since the requested action and actual
    // action have also been verified. Only active fields need to be
    // corrected because all fields must have been set now
    match_data->setActiveFields(dataFields);
  }

  for (const auto &dataFieldId : dataFields) {
    const BfRtTableDataField *tableDataField = nullptr;
    if (action_id) {
      status = getDataField(dataFieldId, action_id, &tableDataField);
    } else {
      status = getDataField(dataFieldId, &tableDataField);
    }
    BF_RT_ASSERT(status == BF_SUCCESS);
    auto fieldTypes = tableDataField->getTypes();
    fieldDestination field_destination =
        BfRtTableDataField::getDataFieldDestination(fieldTypes);
    switch (field_destination) {
      case fieldDestination::DIRECT_LPF:
        if (res_data.has_lpf) {
          match_data->getPipeActionSpecObj().setValueLPFSpec(res_data.mtr.lpf);
        }
        break;
      case fieldDestination::DIRECT_WRED:
        if (res_data.has_red) {
          match_data->getPipeActionSpecObj().setValueWREDSpec(res_data.mtr.red);
        }
        break;
      case fieldDestination::DIRECT_METER:
        if (res_data.has_meter) {
          match_data->getPipeActionSpecObj().setValueMeterSpec(
              res_data.mtr.meter);
        }
        break;
      case fieldDestination::DIRECT_COUNTER:
        if (res_data.has_counter) {
          match_data->getPipeActionSpecObj().setValueCounterSpec(
              res_data.counter);
        }
        break;
      case fieldDestination::DIRECT_REGISTER:
        if (res_data.has_stful && !stful_fetched) {
          std::vector<pipe_stful_mem_spec_t> register_pipe_data(
              res_data.stful.data,
              res_data.stful.data + res_data.stful.pipe_count);
          match_data->getPipeActionSpecObj().setValueRegisterSpec(
              register_pipe_data);
          stful_fetched = true;
        }
        break;
      case fieldDestination::TTL:
        ttl_field_id = dataFieldId;
        if (res_data.has_ttl) {
          match_data->set_ttl_from_read(res_data.idle.ttl);
        }
        break;
      case fieldDestination::ENTRY_HIT_STATE:
        hs_field_id = dataFieldId;
        if (res_data.has_hit_state) {
          match_data->set_entry_hit_state(res_data.idle.hit_state);
        }
        break;
      case fieldDestination::ACTION_SPEC: {
        // We have already processed this. Ignore
        break;
      }
      default:
        LOG_TRACE("%s:%d %s Entry get for the data field %d not supported",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dataFieldId);
        return BF_NOT_SUPPORTED;
        break;
    }
  }
  // Must free stful related memory
  if (res_data.stful.data != nullptr) {
    bf_sys_free(res_data.stful.data);
  }
  // After going over all the data fields, check whether either one
  // of entry_ttl or hit_state was set, remove if not.
  if (!res_data.has_ttl) {
    match_data->removeActiveFields({ttl_field_id});
  }
  if (!res_data.has_hit_state) {
    match_data->removeActiveFields({hs_field_id});
  }
  return BF_SUCCESS;
}

// BfRtMatchActionIndirectTable **************
namespace {
const std::vector<DataFieldType> indirectResourceDataFields = {
    DataFieldType::COUNTER_INDEX,
    DataFieldType::METER_INDEX,
    DataFieldType::REGISTER_INDEX};
}

void BfRtMatchActionIndirectTable::setActionResources(
    const bf_dev_id_t &dev_id) {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  for (auto const &i : this->act_fn_hdl_to_id) {
    bool has_cntr, has_meter, has_lpf, has_wred, has_reg;
    has_cntr = has_meter = has_lpf = has_wred = has_reg = false;
    bf_status_t sts =
        pipeMgr->pipeMgrGetActionDirectResUsage(dev_id,
                                                this->pipe_tbl_hdl,
                                                i.first,
                                                &has_cntr,
                                                &has_meter,
                                                &has_lpf,
                                                &has_wred,
                                                &has_reg);
    if (sts) {
      LOG_ERROR("%s:%d %s Cannot fetch action resource information, %d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                sts);
      continue;
    }
    this->act_uses_dir_cntr[i.second] = has_cntr;
    this->act_uses_dir_meter[i.second] = has_meter || has_lpf || has_wred;
    this->act_uses_dir_reg[i.second] = has_reg;
  }
}

void BfRtMatchActionIndirectTable::populate_indirect_resources(
    const BfRtStateActionProfile::indirResMap &resource_map,
    pipe_action_spec_t *pipe_action_spec) const {
  // Here, we loop over all types of indirect resources that a table can
  // possibly have, and check if that resource is referenced and the reference
  // type is indirect. If the resource is referenced, it is searched for in the
  // resource_map passed here, which is the list of indirect resources
  // applicable to the action associated with the match entry. If the resource
  // is applicable to this action, the tag is ATTACHED else DETACHED

  for (const auto &indirectResource : indirectResourceDataFields) {
    pipe_tbl_hdl_t tbl_hdl = getIndirectResourceHdl(indirectResource);
    if (tbl_hdl) {
      auto elem = resource_map.find(indirectResource);
      pipe_res_spec_t *res_spec =
          &pipe_action_spec->resources[pipe_action_spec->resource_count];
      if (elem != resource_map.end()) {
        bf_rt_id_t resource_idx = elem->second;
        res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
        res_spec->tbl_idx = (pipe_res_idx_t)resource_idx;
      } else {
        res_spec->tag = PIPE_RES_ACTION_TAG_DETACHED;
      }
      res_spec->tbl_hdl = tbl_hdl;
      pipe_action_spec->resource_count++;
    }
  }
  return;
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryAdd(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  const BfRtMatchActionIndirectTableData &match_data =
      static_cast<const BfRtMatchActionIndirectTableData &>(data);
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Check if all indirect resource indices are supplied or not.
  // Entry Add mandates that all indirect indices be given
  // Initialize the direct resources if applicable
  status = resourceCheckAndInitialize<BfRtMatchActionIndirectTableData>(
      *this, match_data, false);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to initialize Direct resources",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  pipe_action_spec_t pipe_action_spec = {0};
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  pipe_act_fn_hdl_t act_fn_hdl = 0;

  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  match_key.populate_match_spec(&pipe_match_spec);

  uint32_t ttl = match_data.get_ttl();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  BfRtStateActionProfile::indirResMap resource_map;

  match_data.copy_pipe_action_spec(&pipe_action_spec);

  // Fill in state from action member id or selector group id in the action spec
  status = getActionState(session,
                          dev_tgt,
                          &match_data,
                          &adt_ent_hdl,
                          &sel_grp_hdl,
                          &act_fn_hdl,
                          &resource_map);

  if (status != BF_SUCCESS) {
    // This implies the action member ID or the selector group ID to which this
    // match entry wants to point to doesn't exist. In the selector group case,
    // the group might be empty, which cannot have a match entry pointing to
    // Its important we output the right error message. Hence check the
    // pipe_action_spec's action entry handle values to discern the exact error
    // 1. If the match entry wants to point to a group ID, if the grp handle in
    // the pipe_action_spec is of an INVALID value, then it implies group does
    // not exist
    // 2. If the group handle is valid and the action entry handle is of an
    // INVALID value, then it implies group exists, but the group is empty
    // 3. If the match entry wants to point to a action member ID, then an
    // invalid value of action entry handle indicates that the member ID does
    // not exist
    if (match_data.isGroup()) {
      if (sel_grp_hdl == BfRtMatchActionIndirectTableData::invalid_group) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a group id %d which "
            "does not exist in the group table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getGroupId());
        return BF_OBJECT_NOT_FOUND;
      } else if (adt_ent_hdl ==
                 BfRtMatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a group id %d which "
            "does not have any members in the group table associated with the "
            "table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getGroupId());
        return BF_OBJECT_NOT_FOUND;
      }
    } else {
      if (adt_ent_hdl ==
          BfRtMatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a action member id "
            "%d "
            "which "
            "does not exist in the action profile table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getActionMbrId());
        return BF_OBJECT_NOT_FOUND;
      }
    }
    return BF_UNEXPECTED;
  }

  if (match_data.isGroup()) {
    pipe_action_spec.sel_grp_hdl = sel_grp_hdl;
  } else {
    pipe_action_spec.adt_ent_hdl = adt_ent_hdl;
  }

  populate_indirect_resources(resource_map, &pipe_action_spec);

  // Ready to add the entry
  return pipeMgr->pipeMgrMatEntAdd(session.sessHandleGet(),
                                   pipe_dev_tgt,
                                   pipe_tbl_hdl,
                                   &pipe_match_spec,
                                   act_fn_hdl,
                                   &pipe_action_spec,
                                   ttl,
                                   0 /* Pipe API flags */,
                                   &pipe_entry_hdl);
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryMod(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_mat_ent_hdl_t pipe_entry_hdl;
  status = pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                             pipe_dev_tgt,
                                             pipe_tbl_hdl,
                                             &pipe_match_spec,
                                             &pipe_entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return tableEntryModInternal(
      *this, session, dev_tgt, flags, data, pipe_entry_hdl);
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryDel(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return pipeMgr->pipeMgrMatEntDelByMatchSpec(session.sessHandleGet(),
                                              pipe_dev_tgt,
                                              pipe_tbl_hdl,
                                              &pipe_match_spec,
                                              0 /* Pipe api flags */);
}

bf_status_t BfRtMatchActionIndirectTable::tableClear(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return tableClearMatCommon(session, dev_tgt, true, this);
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return tableEntryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, &pipe_match_spec, data);
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryKeyGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_tbl_match_spec_t *pipe_match_spec;
  bf_dev_pipe_t entry_pipe;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  bf_status_t status = pipeMgr->pipeMgrEntHdlToMatchSpec(
      session.sessHandleGet(),
      pipe_dev_tgt,
      pipe_tbl_hdl,
      entry_handle,
      &entry_pipe,
      const_cast<const pipe_tbl_match_spec_t **>(&pipe_match_spec));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  entry_tgt->pipe_id = entry_pipe;
  match_key->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  return status;
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        entry_handle);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const bf_rt_handle_t &entry_handle,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key->populate_match_spec(&pipe_match_spec);

  auto status = tableEntryGet_internal(
      session, dev_tgt, flags, entry_handle, &pipe_match_spec, data);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Get the first entry handle present in pipe-mgr
  uint32_t first_entry_handle;
  bf_status_t status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.sessHandleGet(), pipe_tbl_hdl, pipe_dev_tgt, &first_entry_handle);

  if (status == BF_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  pipe_tbl_match_spec_t pipe_match_spec;
  std::memset(&pipe_match_spec, 0, sizeof(pipe_tbl_match_spec_t));

  match_key->populate_match_spec(&pipe_match_spec);

  status = tableEntryGet_internal(
      session, dev_tgt, flags, first_entry_handle, &pipe_match_spec, data);

  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting first entry, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  // Store ref point for GetNext_n to follow.
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  auto nextRef = device_state->nextRefState.getObjState(this->table_id_get());
  nextRef->setRef(session.sessHandleGet(), dev_tgt.pipe_id, first_entry_handle);

  return status;
}

bf_status_t BfRtMatchActionIndirectTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl);
  // If key is not found and this is subsequent call, API should continue
  // from previous call.
  if (status == BF_OBJECT_NOT_FOUND) {
    // Warn the user that currently used key no longer exist.
    LOG_WARN("%s:%d %s Provided key does not exist, trying previous handle",
             __func__,
             __LINE__,
             table_name_get().c_str());
    auto device_state =
        BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
    auto nextRef = device_state->nextRefState.getObjState(this->table_id_get());
    status = nextRef->getRef(
        session.sessHandleGet(), dev_tgt.pipe_id, &pipe_entry_hdl);
  }

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  std::vector<uint32_t> next_entry_handles(n, 0);

  status = pipeMgr->pipeMgrGetNextEntryHandles(session.sessHandleGet(),
                                               pipe_tbl_hdl,
                                               pipe_dev_tgt,
                                               pipe_entry_hdl,
                                               n,
                                               next_entry_handles.data());
  if (status == BF_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return BF_SUCCESS;
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting next entry handles from pipe-mgr, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    return status;
  }
  pipe_tbl_match_spec_t match_spec;
  unsigned i = 0;
  for (i = 0; i < n; i++) {
    std::memset(&match_spec, 0, sizeof(pipe_tbl_match_spec_t));
    auto this_key =
        static_cast<BfRtMatchActionKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != this->table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }
    if (next_entry_handles[i] == (uint32_t)-1) {
      break;
    }
    this_key->populate_match_spec(&match_spec);
    status = tableEntryGet_internal(
        session, dev_tgt, flags, next_entry_handles[i], &match_spec, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    this_key->setPriority(match_spec.priority);
    this_key->setPartitionIndex(match_spec.partition_index);
  }
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  auto nextRef = device_state->nextRefState.getObjState(this->table_id_get());
  // Even if tableEntryGet failed, since pipe_mgr can fetch next n handles
  // starting from non-existing handle, just store last one that was used.
  nextRef->setRef(
      session.sessHandleGet(), dev_tgt.pipe_id, next_entry_handles[i - 1]);

  if (num_returned) {
    *num_returned = i;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::tableDefaultEntrySet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionIndirectTableData &match_data =
      static_cast<const BfRtMatchActionIndirectTableData &>(data);
  pipe_action_spec_t pipe_action_spec = {0};

  pipe_act_fn_hdl_t act_fn_hdl = 0;
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  pipe_sel_grp_hdl_t sel_grp_hdl = 0;

  if (this->has_const_default_action_) {
    LOG_TRACE(
        "%s:%d %s Cannot set Default action because the table has a const "
        "default action",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  BfRtStateActionProfile::indirResMap resource_map;
  // Fill in state from action member id or selector group id in the action spec
  status = getActionState(session,
                          dev_tgt,
                          &match_data,
                          &adt_ent_hdl,
                          &sel_grp_hdl,
                          &act_fn_hdl,
                          &resource_map);

  if (status != BF_SUCCESS) {
    // This implies the action member ID or the selector group ID to which this
    // match entry wants to point to doesn't exist. In the selector group case,
    // the group might be empty, which cannot have a match entry pointing to
    // Its important we output the right error message. Hence check the
    // pipe_action_spec's action entry handle values to discern the exact error
    // 1. If the match entry wants to point to a group ID, if the grp handle in
    // the pipe_action_spec is of an INVALID value, then it implies group does
    // not exist
    // 2. If the group handle is valid and the action entry handle is of an
    // INVALID value, then it implies group exists, but the group is empty
    // 3. If the match entry wants to point to a action member ID, then an
    // invalid value of action entry handle indicates that the member ID does
    // not exist
    if (match_data.isGroup()) {
      if (sel_grp_hdl == BfRtMatchActionIndirectTableData::invalid_group) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a group id %d "
            "which "
            "does not exist in the group table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getGroupId());
        return BF_OBJECT_NOT_FOUND;
      } else if (adt_ent_hdl ==
                 BfRtMatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a group id %d "
            "which "
            "does not exist in the group table associated with the table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getGroupId());
        return BF_OBJECT_NOT_FOUND;
      }
    } else {
      if (adt_ent_hdl ==
          BfRtMatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a action member "
            "id "
            "%d "
            "which "
            "does not exist in the action profile table",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            match_data.getActionMbrId());
        return BF_OBJECT_NOT_FOUND;
      }
    }
    return BF_UNEXPECTED;
  }

  match_data.copy_pipe_action_spec(&pipe_action_spec);

  if (match_data.isGroup()) {
    pipe_action_spec.sel_grp_hdl = sel_grp_hdl;
  } else {
    pipe_action_spec.adt_ent_hdl = adt_ent_hdl;
  }

  populate_indirect_resources(resource_map, &pipe_action_spec);

  return pipeMgr->pipeMgrMatDefaultEntrySet(session.sessHandleGet(),
                                            pipe_dev_tgt,
                                            pipe_tbl_hdl,
                                            act_fn_hdl,
                                            &pipe_action_spec,
                                            0 /* Pipe API flags */,
                                            &pipe_entry_hdl);
}

bf_status_t BfRtMatchActionIndirectTable::tableDefaultEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, &pipe_entry_hdl);
  if (BF_SUCCESS != status) {
    LOG_TRACE("%s:%d %s Dev %d pipe %x error %d getting default entry",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              status);
    return status;
  }
  return tableEntryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, nullptr, data);
}

bf_status_t BfRtMatchActionIndirectTable::tableDefaultEntryReset(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  if (this->has_const_default_action_) {
    // If default action is const, then this API is a no-op
    LOG_DBG(
        "%s:%d %s Calling reset on a table with const "
        "default action",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_SUCCESS;
  }
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  return pipeMgr->pipeMgrMatTblDefaultEntryReset(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, 0);
}

bf_status_t BfRtMatchActionIndirectTable::getActionState(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtMatchActionIndirectTableData *data,
    pipe_adt_ent_hdl_t *adt_entry_hdl,
    pipe_sel_grp_hdl_t *sel_grp_hdl,
    pipe_act_fn_hdl_t *act_fn_hdl,
    BfRtStateActionProfile::indirResMap *resource_map) const {
  bf_status_t status = BF_SUCCESS;
  BfRtActionTable *actTbl = static_cast<BfRtActionTable *>(actProfTbl);
  if (!data->isGroup()) {
    // Safe to do a static cast here since all table objects are constructed by
    // our factory and the right kind of sub-object is constructed by the
    // factory depending on the table type. Here the actProfTbl member of the
    // table object is a pointer to the action profile table associated with the
    // match-action table. It is guaranteed to be  of type BfRtActionTable

    status = actTbl->getMbrState(dev_tgt,
                                 data->getActionMbrId(),
                                 act_fn_hdl,
                                 adt_entry_hdl,
                                 resource_map);
    if (status != BF_SUCCESS) {
      *adt_entry_hdl =
          BfRtMatchActionIndirectTableData::invalid_action_entry_hdl;
      return status;
    }
  } else {
    BfRtSelectorTable *selTbl = static_cast<BfRtSelectorTable *>(selectorTbl);
    // Get device state and from that the selector group state of this table to
    // do
    // various checks.
    auto device_state =
        BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
    auto selector_state =
        device_state->selectorState.getObjState(getSelectorId());
    *act_fn_hdl = BfRtStateSelector::invalid_act_fn_hdl;
    // Get the selector group state from the group id
    bool found = selector_state->grpIdExists(
        dev_tgt.pipe_id, data->getGroupId(), sel_grp_hdl);
    if (!found) {
      *sel_grp_hdl = BfRtMatchActionIndirectTableData::invalid_group;
      return BF_OBJECT_NOT_FOUND;
    }

    status =
        selTbl->getOneMbr(session, dev_tgt, *sel_grp_hdl, adt_entry_hdl);
    if (status != BF_SUCCESS) {
      *adt_entry_hdl =
          BfRtMatchActionIndirectTableData::invalid_action_entry_hdl;
      return BF_OBJECT_NOT_FOUND;
    }

    bf_rt_id_t a_member_id = BfRtStateSelector::invalid_group_member;
    status =
        actTbl->getMbrIdFromHndl(dev_tgt.dev_id, *adt_entry_hdl, &a_member_id);
    if (status != BF_SUCCESS ||
        a_member_id == BfRtStateSelector::invalid_group_member) {
      *adt_entry_hdl =
          BfRtMatchActionIndirectTableData::invalid_action_entry_hdl;
      return BF_OBJECT_NOT_FOUND;
    }

    status = actTbl->getMbrState(
        dev_tgt, a_member_id, act_fn_hdl, adt_entry_hdl, resource_map);
  }
  return status;
}

void BfRtMatchActionIndirectTable::setIsTernaryTable(
    const bf_dev_id_t &dev_id) {
  auto status = PipeMgrIntf::getInstance()->pipeMgrTblIsTern(
      dev_id, pipe_tbl_hdl, &this->is_ternary_table_);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to find whether table is ternary or not",
              __func__,
              __LINE__,
              table_name_get().c_str());
  }
}

bf_status_t BfRtMatchActionIndirectTable::getActionMbrIdFromHndl(
    uint16_t device_id,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    bf_rt_id_t *mbr_id) const {
  BfRtActionTable *actTbl = static_cast<BfRtActionTable *>(actProfTbl);
  return actTbl->getMbrIdFromHndl(device_id, adt_ent_hdl, mbr_id);
}

bf_status_t BfRtMatchActionIndirectTable::getGroupIdFromHndl(
    uint16_t device_id,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    bf_rt_id_t *grp_id) const {
  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(device_id, prog_name);
  auto selector_state =
      device_state->selectorState.getObjState(getSelectorId());
  return selector_state->getGroupIdFromHndl(sel_grp_hdl, grp_id);
}

bf_status_t BfRtMatchActionIndirectTable::tableSizeGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    size_t *count) const {
  bf_status_t status = BF_SUCCESS;
  size_t reserved = 0;
  status = getReservedEntries(session, dev_tgt, *(this), &reserved);
  *count = this->_table_size - reserved;
  return status;
}

bf_status_t BfRtMatchActionIndirectTable::tableUsageGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

bf_status_t BfRtMatchActionIndirectTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtMatchActionKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::keyReset(BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  return key_reset<BfRtMatchActionIndirectTable, BfRtMatchActionKey>(*this,
                                                                     match_key);
}

bf_status_t BfRtMatchActionIndirectTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtMatchActionIndirectTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtMatchActionIndirectTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::dataReset(BfRtTableData *data) const {
  std::vector<bf_rt_id_t> fields;
  return dataReset_internal(*this, 0, fields, data);
}

bf_status_t BfRtMatchActionIndirectTable::dataReset(
    const std::vector<bf_rt_id_t> &fields, BfRtTableData *data) const {
  return dataReset_internal(*this, 0, fields, data);
}

bf_status_t BfRtMatchActionIndirectTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  if (type == TableAttributesType::IDLE_TABLE_RUNTIME) {
    LOG_TRACE(
        "%s:%d %s Idle Table Runtime Attribute requires a Mode, please use the "
        "appropriate API to include the idle table mode",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::attributeAllocate(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  if (type != TableAttributesType::IDLE_TABLE_RUNTIME) {
    LOG_TRACE("%s:%d %s Idle Table Mode cannot be set for Attribute %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type, idle_table_mode));
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(type));
      return BF_INVALID_ARG;
    case TableAttributesType::ENTRY_SCOPE:
      break;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtMatchActionIndirectTable::attributeReset(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      break;
    case TableAttributesType::ENTRY_SCOPE:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(type));
      return BF_INVALID_ARG;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type, idle_table_mode);
}

bf_status_t BfRtMatchActionIndirectTable::tableAttributesSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(0);
      bf_status_t sts = tbl_attr_impl->entryScopeParamsGet(
          &entry_scope,
          static_cast<BfRtTableEntryScopeArguments *>(&scope_args));
      if (sts != BF_SUCCESS) {
        return sts;
      }
      // Call the pipe mgr API to set entry scope
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      // Derive pipe mgr entry scope from BFRT entry scope and set it to
      // property value
      prop_val.value =
          entry_scope == TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
              ? PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
              : entry_scope == TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                    ? PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                    : PIPE_MGR_ENTRY_SCOPE_USER_DEFINED;
      std::bitset<32> bitval;
      scope_args.getValue(&bitval);
      args_val.value = bitval.to_ulong();
      auto *pipeMgr = PipeMgrIntf::getInstance();
      sts = pipeMgr->pipeMgrTblSetProperty(session.sessHandleGet(),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           prop_type,
                                           prop_val,
                                           args_val);
      if (sts == BF_SUCCESS) {
        auto device_state =
            BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
        if (device_state == nullptr) {
          BF_RT_ASSERT(0);
          return BF_OBJECT_NOT_FOUND;
        }

        // Set ENTRY_SCOPE of the table
        auto attributes_state =
            device_state->attributesState.getObjState(table_id_get());
        attributes_state->setEntryScope(entry_scope);
      }
      return sts;
    }
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMatchActionIndirectTable::tableAttributesGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      auto *pipeMgr = PipeMgrIntf::getInstance();
      auto sts = pipeMgr->pipeMgrTblGetProperty(session.sessHandleGet(),
                                                dev_tgt.dev_id,
                                                pipe_tbl_hdl,
                                                prop_type,
                                                &prop_val,
                                                &args_val);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get entry scope from pipe_mgr",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return sts;
      }

      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(args_val.value);

      // Derive BFRT entry scope from pipe mgr entry scope
      entry_scope = prop_val.value == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
                        ? TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
                        : prop_val.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                              ? TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                              : TableEntryScope::ENTRY_SCOPE_USER_DEFINED;

      return tbl_attr_impl->entryScopeParamsSet(
          entry_scope, static_cast<BfRtTableEntryScopeArguments &>(scope_args));
    }
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "get "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

// Unexposed functions
bf_status_t BfRtMatchActionIndirectTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const pipe_mat_ent_hdl_t &pipe_entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_action_spec_t *pipe_action_spec = nullptr;
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
  pipe_res_get_data_t res_data;

  bool stful_fetched = false;
  bf_rt_id_t ttl_field_id = 0;
  bf_rt_id_t hs_field_id = 0;
  uint32_t res_get_flags = 0;
  res_data.stful.data = nullptr;

  BfRtMatchActionIndirectTableData *match_data =
      static_cast<BfRtMatchActionIndirectTableData *>(data);
  std::vector<bf_rt_id_t> dataFields;
  bool all_fields_set = match_data->allFieldsSet();

  bf_rt_id_t req_action_id = 0;
  status = match_data->actionIdGet(&req_action_id);
  BF_RT_ASSERT(status == BF_SUCCESS);

  if (all_fields_set) {
    res_get_flags = PIPE_RES_GET_FLAG_ALL;
  } else {
    dataFields.assign(match_data->getActiveFields().begin(),
                      match_data->getActiveFields().end());
    for (const auto &dataFieldId : match_data->getActiveFields()) {
      const BfRtTableDataField *tableDataField = nullptr;
      status = getDataField(dataFieldId, &tableDataField);
      BF_RT_ASSERT(status == BF_SUCCESS);
      auto fieldTypes = tableDataField->getTypes();
      fieldDestination field_destination =
          BfRtTableDataField::getDataFieldDestination(fieldTypes);
      switch (field_destination) {
        case fieldDestination::DIRECT_LPF:
        case fieldDestination::DIRECT_METER:
        case fieldDestination::DIRECT_WRED:
          res_get_flags |= PIPE_RES_GET_FLAG_METER;
          break;
        case fieldDestination::DIRECT_REGISTER:
          res_get_flags |= PIPE_RES_GET_FLAG_STFUL;
          break;
        case fieldDestination::ACTION_SPEC:
          res_get_flags |= PIPE_RES_GET_FLAG_ENTRY;
          break;
        case fieldDestination::DIRECT_COUNTER:
          res_get_flags |= PIPE_RES_GET_FLAG_CNTR;
          break;
        case fieldDestination::ENTRY_HIT_STATE:
        case fieldDestination::TTL:
          res_get_flags |= PIPE_RES_GET_FLAG_IDLE;
          break;
        default:
          break;
      }
    }
  }
  // All inputs from the data object have been processed. Now reset it
  // for out data purpose
  // We reset the data object with act_id 0 and all fields
  match_data->reset();
  pipe_action_spec = match_data->get_pipe_action_spec();

  status = getActionSpec(session,
                         pipe_dev_tgt,
                         flags,
                         pipe_tbl_hdl,
                         pipe_entry_hdl,
                         res_get_flags,
                         pipe_match_spec,
                         pipe_action_spec,
                         &pipe_act_fn_hdl,
                         &res_data);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handle %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        pipe_entry_hdl,
        status);
    return status;
  }

  // There is no direct action in indirect flow, hence always fill in
  // only requested fields.
  if (all_fields_set) {
    status = dataFieldIdListGet(&dataFields);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                status);
      return status;
    }
  } else {
    // dataFields has already been populated
    // with the correct fields since the requested action and actual
    // action have also been verified. Only active fields need to be
    // corrected because all fields must have been set now
    match_data->setActiveFields(dataFields);
  }

  for (const auto &dataFieldId : dataFields) {
    const BfRtTableDataField *tableDataField = nullptr;
    status = getDataField(dataFieldId, &tableDataField);
    BF_RT_ASSERT(status == BF_SUCCESS);
    auto fieldTypes = tableDataField->getTypes();
    fieldDestination field_destination =
        BfRtTableDataField::getDataFieldDestination(fieldTypes);
    std::set<bf_rt_id_t> oneof_siblings;
    status = this->dataFieldOneofSiblingsGet(dataFieldId, &oneof_siblings);

    switch (field_destination) {
      case fieldDestination::DIRECT_METER:
        if (res_data.has_meter) {
          match_data->getPipeActionSpecObj().setValueMeterSpec(
              res_data.mtr.meter);
        }
        break;
      case fieldDestination::DIRECT_LPF:
        if (res_data.has_lpf) {
          match_data->getPipeActionSpecObj().setValueLPFSpec(res_data.mtr.lpf);
        }
        break;
      case fieldDestination::DIRECT_WRED:
        if (res_data.has_red) {
          match_data->getPipeActionSpecObj().setValueWREDSpec(res_data.mtr.red);
        }
        break;
      case fieldDestination::DIRECT_COUNTER:
        if (res_data.has_counter) {
          match_data->getPipeActionSpecObj().setValueCounterSpec(
              res_data.counter);
        }
        break;
      case fieldDestination::DIRECT_REGISTER:
        if (res_data.has_stful && !stful_fetched) {
          std::vector<pipe_stful_mem_spec_t> register_pipe_data(
              res_data.stful.data,
              res_data.stful.data + res_data.stful.pipe_count);
          match_data->getPipeActionSpecObj().setValueRegisterSpec(
              register_pipe_data);
          stful_fetched = true;
        }
        break;
      case fieldDestination::TTL:
        ttl_field_id = dataFieldId;
        if (res_data.has_ttl) {
          match_data->set_ttl_from_read(res_data.idle.ttl);
        }
        break;
      case fieldDestination::ENTRY_HIT_STATE:
        hs_field_id = dataFieldId;
        if (res_data.has_hit_state) {
          match_data->set_entry_hit_state(res_data.idle.hit_state);
        }
        break;
      case fieldDestination::ACTION_SPEC: {
        // If its the action member ID or the selector group ID, populate the
        // right member of the data object
        if (fieldTypes.find(DataFieldType::ACTION_MEMBER_ID) !=
            fieldTypes.end()) {
          if (IS_ACTION_SPEC_ACT_DATA_HDL(pipe_action_spec)) {
            bf_rt_id_t act_mbr_id;
            status = getActionMbrIdFromHndl(
                dev_tgt.dev_id, pipe_action_spec->adt_ent_hdl, &act_mbr_id);
            // Default entries will not have an action member handle if they
            // were installed automatically.  In this case return a member id
            // of zero.
            if (status == BF_OBJECT_NOT_FOUND &&
                !pipe_match_spec &&  // Default entries won't have a match spec
                pipe_action_spec->adt_ent_hdl == 0) {
              status = BF_SUCCESS;
              act_mbr_id = 0;
            }
            BF_RT_ASSERT(status == BF_SUCCESS);
            match_data->setActionMbrId(act_mbr_id);
            // Remove oneof sibling from active fields
            match_data->removeActiveFields(oneof_siblings);
          }
        } else if (fieldTypes.find(DataFieldType::SELECTOR_GROUP_ID) !=
                   fieldTypes.end()) {
          if (IS_ACTION_SPEC_SEL_GRP(pipe_action_spec)) {
            bf_rt_id_t sel_grp_id;
            status = getGroupIdFromHndl(
                dev_tgt.dev_id, pipe_action_spec->sel_grp_hdl, &sel_grp_id);
            BF_RT_ASSERT(status == BF_SUCCESS);
            match_data->setGroupId(sel_grp_id);
            // Remove oneof sibling from active fields
            match_data->removeActiveFields(oneof_siblings);
          }
        } else {
          BF_RT_ASSERT(0);
        }
        break;
      }
      default:
        LOG_TRACE("%s:%d %s Entry get for the data field %d not supported",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dataFieldId);
        return BF_NOT_SUPPORTED;
        break;
    }
  }
  // Must free stful related memory
  if (res_data.stful.data != nullptr) {
    bf_sys_free(res_data.stful.data);
  }
  // After going over all the data fields, check whether either one
  // of entry_ttl or hit_state was set, remove if not.
  if (!res_data.has_ttl) {
    match_data->removeActiveFields({ttl_field_id});
  }
  if (!res_data.has_hit_state) {
    match_data->removeActiveFields({hs_field_id});
  }
  return BF_SUCCESS;
}

// BfRtActionTable

bf_status_t BfRtActionTable::tableEntryAdd(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const BfRtTableKey &key,
                                           const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);
  const BfRtActionTableData &action_tbl_data =
      static_cast<const BfRtActionTableData &>(data);
  const pipe_action_spec_t *pipe_action_spec =
      action_tbl_data.get_pipe_action_spec();

  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();

  BfRtStateActionProfile::indirResMap existing_resource_map;
  pipe_act_fn_hdl_t existing_act_fn_hdl = 0;
  pipe_adt_ent_hdl_t adt_entry_hdl;
  // Check if the action member ID already exists
  std::lock_guard<std::mutex> lock(state_lock);
  status = getMbrState(dev_tgt,
                       mbr_id,
                       &existing_act_fn_hdl,
                       &adt_entry_hdl,
                       &existing_resource_map);

  if (status == BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Member Id %d already exists to add",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mbr_id)
    return BF_ALREADY_EXISTS;
  }

  pipe_act_fn_hdl_t act_fn_hdl = action_tbl_data.getActFnHdl();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  std::map<DataFieldType, bf_rt_id_t> resource_map =
      action_tbl_data.get_res_map();

  status = pipeMgr->pipeMgrAdtEntAdd(session.sessHandleGet(),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     act_fn_hdl,
                                     pipe_action_spec,
                                     &adt_entry_hdl,
                                     0 /* Pipe API flags */);

  if (status == BF_SUCCESS) {
    // 1. Populate the state to map the action member id to action entry
    // handle
    // returned by pipe-mgr
    // 2. Populate the state to map action member id to action fn handle
    status = addState(pipe_dev_tgt.device_id,
                      pipe_dev_tgt.dev_pipe_id,
                      mbr_id,
                      act_fn_hdl,
                      adt_entry_hdl,
                      resource_map);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d Error in adding action profile member state for tbl id %d, "
          "member id %d, err %d",
          __func__,
          __LINE__,
          table_id_get(),
          mbr_id,
          status);
      // assert
    }
  }
  return status;
}

bf_status_t BfRtActionTable::tableEntryMod(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const BfRtTableKey &key,
                                           const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);
  const BfRtActionTableData &action_tbl_data =
      static_cast<const BfRtActionTableData &>(data);
  const pipe_action_spec_t *pipe_action_spec =
      action_tbl_data.get_pipe_action_spec();

  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();

  // Two sets of member data. The existing ones and the new ones
  pipe_act_fn_hdl_t existing_act_fn_hdl = 0;
  pipe_act_fn_hdl_t new_act_fn_hdl = action_tbl_data.getActFnHdl();

  // Action entry handle doesn't change during a modify
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;

  // Get the new indirect resource map
  BfRtStateActionProfile::indirResMap new_resource_map =
      action_tbl_data.get_res_map();
  BfRtStateActionProfile::indirResMap existing_resource_map;

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Get the action member ID state
  std::lock_guard<std::mutex> lock(state_lock);
  status = getMbrState(dev_tgt,
                       mbr_id,
                       &existing_act_fn_hdl,
                       &adt_ent_hdl,
                       &existing_resource_map);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x to modify",
              __func__,
              __LINE__,
              mbr_id,
              table_id_get());
    return BF_OBJECT_NOT_FOUND;
  }

  status = pipeMgr->pipeMgrAdtEntSet(session.sessHandleGet(),
                                     pipe_dev_tgt.device_id,
                                     pipe_tbl_hdl,
                                     adt_ent_hdl,
                                     new_act_fn_hdl,
                                     pipe_action_spec,
                                     0 /* Pipe API flags */);

  if (status == BF_SUCCESS) {
    // Modify the state which maps the action member id to action fn handle
    status = modifyState(pipe_dev_tgt.device_id,
                         pipe_dev_tgt.dev_pipe_id,
                         mbr_id,
                         new_act_fn_hdl,
                         adt_ent_hdl,
                         new_resource_map);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in modifying action profile member state for tbl id %d "
          "member id %d, err %s",
          __func__,
          __LINE__,
          table_id_get(),
          mbr_id,
          pipe_str_err((pipe_status_t)status));
      // assert
      return BF_UNEXPECTED;
    }
  }
  return status;
}
bf_status_t BfRtActionTable::tableEntryDel(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const BfRtTableKey &key) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();
  pipe_act_fn_hdl_t act_fn_hdl = 0;

  BfRtStateActionProfile::indirResMap resource_map;

  // Get the action entry handle used by pipe-mgr from the member id
  std::lock_guard<std::mutex> lock(state_lock);
  status =
      getMbrState(dev_tgt, mbr_id, &act_fn_hdl, &adt_ent_hdl, &resource_map);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x to modify",
              __func__,
              __LINE__,
              mbr_id,
              table_id_get());
    return BF_OBJECT_NOT_FOUND;
  }

  status = pipeMgr->pipeMgrAdtEntDel(session.sessHandleGet(),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     adt_ent_hdl,
                                     0 /* Pipe api flags */);
  if (status == BF_SUCCESS) {
    // Delete the state
    status = deleteState(dev_tgt.dev_id, dev_tgt.pipe_id, mbr_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in modifying action profile member state for tbl id %d "
          "member id %d, err %s",
          __func__,
          __LINE__,
          table_id_get(),
          mbr_id,
          pipe_str_err((pipe_status_t)status));
      // assert
      return BF_UNEXPECTED;
    }
  }
  return status;
}

bf_status_t BfRtActionTable::tableClear(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t & /*flags*/) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  BfRtStateActionProfile::indirResMap resource_map{};

  // Get the list of all mem IDs
  std::vector<std::pair<bf_rt_id_t, bf_dev_pipe_t>> mbr_id_list;
  status = getMbrList(dev_tgt.dev_id, &mbr_id_list);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Failed to get mem_id list for tbl 0x%x",
              __func__,
              __LINE__,
              table_id_get());
    return BF_OBJECT_NOT_FOUND;
  }

  for (const auto &mbr_id_pair : mbr_id_list) {
    if (mbr_id_pair.second != dev_tgt.pipe_id &&
        BF_DEV_PIPE_ALL != dev_tgt.pipe_id)
      continue;
    bf_rt_target_t dt;
    dt.dev_id = dev_tgt.dev_id;
    dt.pipe_id = mbr_id_pair.second;

    std::lock_guard<std::mutex> lock(state_lock);
    status = getMbrState(
        dt, mbr_id_pair.first, &act_fn_hdl, &adt_ent_hdl, &resource_map);

    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x",
                __func__,
                __LINE__,
                mbr_id_pair.first,
                table_id_get());
      return BF_OBJECT_NOT_FOUND;
    }
    status = pipeMgr->pipeMgrAdtEntDel(session.sessHandleGet(),
                                       pipe_dev_tgt,
                                       pipe_tbl_hdl,
                                       adt_ent_hdl,
                                       0 /* Pipe api flags */);
    if (status == BF_SUCCESS) {
      // Delete the state
      status =
          deleteState(dev_tgt.dev_id, mbr_id_pair.second, mbr_id_pair.first);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error deleting action profile member state for tbl id 0x%x "
            "member id %d pipe id %x, err %s",
            __func__,
            __LINE__,
            table_id_get(),
            mbr_id_pair.first,
            mbr_id_pair.second,
            pipe_str_err((pipe_status_t)status));
        // assert
        return BF_UNEXPECTED;
      }
    }
  }
  return status;
}

bf_status_t BfRtActionTable::tableEntryGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           const BfRtTableKey &key,
                                           BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);
  BfRtActionTableData *action_tbl_data =
      static_cast<BfRtActionTableData *>(data);

  BfRtStateActionProfile::indirResMap resource_map;

  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_adt_ent_hdl_t adt_ent_hdl;
  status =
      getMbrState(dev_tgt, mbr_id, &act_fn_hdl, &adt_ent_hdl, &resource_map);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Action member Id %d does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              mbr_id);
    return BF_OBJECT_NOT_FOUND;
  }
  return tableEntryGet_internal(
      session, dev_tgt, flags, adt_ent_hdl, action_tbl_data);
}

bf_status_t BfRtActionTable::tableEntryKeyGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtActionTableKey *action_tbl_key = static_cast<BfRtActionTableKey *>(key);
  bf_rt_id_t mbr_id;
  bf_status_t status =
      this->getMbrIdFromHndl(dev_tgt.dev_id, entry_handle, &mbr_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  action_tbl_key->setMemberId(mbr_id);
  *entry_tgt = dev_tgt;
  return status;
}

bf_status_t BfRtActionTable::tableEntryHandleGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);
  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();
  BfRtStateActionProfile::indirResMap resource_map;
  pipe_act_fn_hdl_t act_fn_hdl;
  bf_status_t status =
      getMbrState(dev_tgt, mbr_id, &act_fn_hdl, entry_handle, &resource_map);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtActionTable::tableEntryGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           const bf_rt_handle_t &entry_handle,
                                           BfRtTableKey *key,
                                           BfRtTableData *data) const {
  bf_rt_target_t entry_tgt;
  bf_status_t status = this->tableEntryKeyGet(
      session, dev_tgt, flags, entry_handle, &entry_tgt, key);
  if (status != BF_SUCCESS) {
    return status;
  }
  return this->tableEntryGet(session, entry_tgt, flags, *key, data);
}

bf_status_t BfRtActionTable::tableEntryGetFirst(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                BfRtTableKey *key,
                                                BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  BfRtActionTableKey *action_tbl_key = static_cast<BfRtActionTableKey *>(key);
  BfRtActionTableData *action_tbl_data =
      static_cast<BfRtActionTableData *>(data);

  bf_rt_id_t first_mbr_id;
  pipe_adt_ent_hdl_t first_entry_hdl;

  status = getFirstMbr(dev_tgt, &first_mbr_id, &first_entry_hdl);
  if (status == BF_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  status = tableEntryGet_internal(
      session, dev_tgt, flags, first_entry_hdl, action_tbl_data);
  if (status != BF_SUCCESS) {
    return status;
  }
  action_tbl_key->setMemberId(first_mbr_id);
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::tableEntryGetNext_n(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t &flags,
                                                 const BfRtTableKey &key,
                                                 const uint32_t &n,
                                                 keyDataPairs *key_data_pairs,
                                                 uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtActionTableKey &action_tbl_key =
      static_cast<const BfRtActionTableKey &>(key);

  bf_rt_id_t mbr_id = action_tbl_key.getMemberId();
  bf_rt_id_t next_mbr_id;
  bf_rt_id_t next_entry_hdl;

  unsigned i = 0;

  for (i = 0; i < n; i++) {
    status = getNextMbr(dev_tgt, mbr_id, &next_mbr_id, &next_entry_hdl);
    if (status != BF_SUCCESS) {
      break;
    }
    auto this_key =
        static_cast<BfRtActionTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtActionTableData *>((*key_data_pairs)[i].second);
    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != this->table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }
    status = tableEntryGet_internal(
        session, dev_tgt, flags, next_entry_hdl, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR in getting %dth entry from pipe-mgr with entry "
          "handle %d, mbr id %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          i + 1,
          next_entry_hdl,
          next_mbr_id,
          status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    this_key->setMemberId(next_mbr_id);
    mbr_id = next_mbr_id;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::tableUsageGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

bf_status_t BfRtActionTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtActionTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::dataAllocate(
    const bf_rt_id_t &action_id,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  if (action_info_list.find(action_id) == action_info_list.end()) {
    LOG_TRACE("%s:%d Action_ID %d not found", __func__, __LINE__, action_id);
    return BF_OBJECT_NOT_FOUND;
  }
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtActionTableData(this, action_id));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  // This dataAllocate is mainly used for entry gets from the action table
  // wherein  the action id of the entry is not known and will be filled in by
  // the entry get
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtActionTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::addState(
    uint16_t device_id,
    bf_dev_pipe_t dev_pipe_id,
    bf_rt_id_t mbr_id,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_adt_ent_hdl_t adt_entry_hdl,
    BfRtStateActionProfile::indirResMap &resource_map) const {
  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(device_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto act_prof_state = device_state->actProfState.getObjState(table_id_get());
  bf_status_t status = act_prof_state->stateActionProfileAdd(
      mbr_id, dev_pipe_id, act_fn_hdl, adt_entry_hdl, resource_map);

  return status;
}
bf_status_t BfRtActionTable::getMbrList(
    uint16_t device_id,
    std::vector<std::pair<bf_rt_id_t, bf_dev_pipe_t>> *mbr_id_list) const {
  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(device_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto act_prof_state = device_state->actProfState.getObjState(table_id_get());
  if (act_prof_state == nullptr) {
    return BF_OBJECT_NOT_FOUND;
  }
  bf_status_t status = act_prof_state->stateActionProfileListGet(mbr_id_list);

  return status;
}

bf_status_t BfRtActionTable::getMbrState(
    const bf_rt_target_t &dev_tgt,
    bf_rt_id_t mbr_id,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_adt_ent_hdl_t *adt_ent_hdl,
    BfRtStateActionProfile::indirResMap *res_map) const {
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto act_prof_state = device_state->actProfState.getObjState(table_id_get());
  if (act_prof_state == nullptr) {
    return BF_OBJECT_NOT_FOUND;
  }
  bf_status_t status = act_prof_state->stateActionProfileGet(
      mbr_id, dev_tgt.pipe_id, act_fn_hdl, adt_ent_hdl, res_map);

  return status;
}

bf_status_t BfRtActionTable::getFirstMbr(
    const bf_rt_target_t &dev_tgt,
    bf_rt_id_t *first_mbr_id,
    pipe_adt_ent_hdl_t *first_entry_hdl) const {
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto act_prof_state = device_state->actProfState.getObjState(table_id_get());
  if (act_prof_state == nullptr) {
    return BF_OBJECT_NOT_FOUND;
  }
  return act_prof_state->getFirstMbr(
      dev_tgt.pipe_id, first_mbr_id, first_entry_hdl);
}

bf_status_t BfRtActionTable::getNextMbr(const bf_rt_target_t &dev_tgt,
                                        bf_rt_id_t mbr_id,
                                        bf_rt_id_t *next_mbr_id,
                                        bf_rt_id_t *next_entry_hdl) const {
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto act_prof_state = device_state->actProfState.getObjState(table_id_get());
  if (act_prof_state == nullptr) {
    return BF_OBJECT_NOT_FOUND;
  }
  return act_prof_state->getNextMbr(
      mbr_id, dev_tgt.pipe_id, next_mbr_id, next_entry_hdl);
}

bf_status_t BfRtActionTable::getMbrIdFromHndl(uint16_t device_id,
                                              pipe_adt_ent_hdl_t adt_ent_hdl,
                                              bf_rt_id_t *mbr_id) const {
  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(device_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto act_prof_state = device_state->actProfState.getObjState(table_id_get());

  return act_prof_state->getMbrIdFromHndl(adt_ent_hdl, mbr_id);
}

bf_status_t BfRtActionTable::modifyState(
    uint16_t device_id,
    const bf_dev_pipe_t pipe_id,
    const bf_rt_id_t mbr_id,
    const pipe_act_fn_hdl_t &act_fn_hdl,
    const pipe_adt_ent_hdl_t &adt_entry_hdl,
    BfRtStateActionProfile::indirResMap resource_map) const {
  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(device_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto act_prof_state = device_state->actProfState.getObjState(table_id_get());
  if (act_prof_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }

  bf_status_t status = act_prof_state->stateActionProfileModify(
      mbr_id, pipe_id, act_fn_hdl, adt_entry_hdl, resource_map);

  return status;
}

bf_status_t BfRtActionTable::deleteState(uint16_t device_id,
                                         const bf_dev_pipe_t &pipe_id,
                                         const bf_rt_id_t &mbr_id) const {
  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(device_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto act_prof_state = device_state->actProfState.getObjState(table_id_get());
  bf_status_t status =
      act_prof_state->stateActionProfileRemove(mbr_id, pipe_id);

  return status;
}

bf_status_t BfRtActionTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const pipe_adt_ent_hdl_t &entry_hdl,
    BfRtActionTableData *action_tbl_data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  pipe_action_spec_t *action_spec = action_tbl_data->mutable_pipe_action_spec();
  bool read_from_hw = false;
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    read_from_hw = true;
  }
  pipe_act_fn_hdl_t act_fn_hdl;
  dev_target_t pipe_dev_tgt;

  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  
  bf_status_t status =
      pipeMgr->pipeMgrGetActionDataEntry(session.sessHandleGet(), pipe_tbl_hdl,
                                         pipe_dev_tgt,
                                         entry_hdl,
                                         &action_spec->act_data,
                                         &act_fn_hdl,
                                         read_from_hw);
  // At this point, if a member wasn't found, there is a high chance
  // that the action data wasn't programmed in the hw itself because by
  // this time BF-RT sw state check has passed. So try it once again with
  // with read_from_hw = False
  if (status == BF_OBJECT_NOT_FOUND && read_from_hw) {
    status = pipeMgr->pipeMgrGetActionDataEntry(session.sessHandleGet(), pipe_tbl_hdl,
                                                pipe_dev_tgt,
                                                entry_hdl,
                                                &action_spec->act_data,
                                                &act_fn_hdl,
                                                false);
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting action data from pipe-mgr, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  bf_rt_id_t action_id = this->getActIdFromActFnHdl(act_fn_hdl);

  action_tbl_data->actionIdSet(action_id);
  std::vector<bf_rt_id_t> empty;
  action_tbl_data->setActiveFields(empty);
  return BF_SUCCESS;
}

bf_status_t BfRtActionTable::keyReset(BfRtTableKey *key) const {
  BfRtActionTableKey *action_key = static_cast<BfRtActionTableKey *>(key);
  return key_reset<BfRtActionTable, BfRtActionTableKey>(*this, action_key);
}

bf_status_t BfRtActionTable::dataReset(BfRtTableData *data) const {
  BfRtActionTableData *data_obj = static_cast<BfRtActionTableData *>(data);
  if (!this->validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return data_obj->reset(0);
}

bf_status_t BfRtActionTable::dataReset(const bf_rt_id_t &action_id,
                                       BfRtTableData *data) const {
  BfRtActionTableData *data_obj = static_cast<BfRtActionTableData *>(data);
  if (!this->validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return data_obj->reset(action_id);
}

// BfRtSelectorTable **************
bf_status_t BfRtSelectorTable::tableEntryAdd(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const BfRtTableKey &key,
                                             const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  bf_status_t temp_status;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtSelectorTableKey &sel_key =
      static_cast<const BfRtSelectorTableKey &>(key);
  const BfRtSelectorTableData &sel_data =
      static_cast<const BfRtSelectorTableData &>(data);
  // Make a call to pipe-mgr to first create the group
  pipe_sel_grp_hdl_t sel_grp_hdl;
  uint16_t max_grp_size = sel_data.get_max_grp_size();
  bf_rt_id_t sel_grp_id = sel_key.getGroupId();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  std::vector<bf_rt_id_t> members = sel_data.getMembers();
  std::vector<bool> member_status = sel_data.getMemberStatus();

  if (members.size() != member_status.size()) {
    LOG_TRACE("%s:%d MemberId size %zu and member status size %zu do not match",
              __func__,
              __LINE__,
              members.size(),
              member_status.size());
    return BF_INVALID_ARG;
  }

  if (members.size() > max_grp_size) {
    LOG_TRACE(
        "%s:%d %s Number of members provided %zd exceeds the maximum group "
        "size %d for group id %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        members.size(),
        max_grp_size,
        sel_grp_id);
    return BF_INVALID_ARG;
  }
  // Get device state and from that the selector group state of this table to do
  // various checks.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  auto selector_state = device_state->selectorState.getObjState(table_id_get());

  /* Get Act Prof state for action profile table associated with this selector
   * table */
  auto act_profile_state =
      device_state->actProfState.getObjState(getActProfId());

  if (selector_state == nullptr || act_profile_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_UNEXPECTED;
  }
  // First, check if the group ID doesn't already exist
  if (selector_state->grpIdExists(dev_tgt.pipe_id, sel_grp_id, &sel_grp_hdl)) {
    LOG_TRACE(
        "%s:%d %s : Error in adding a selector group id %d to pipe %x, group "
        "id already exists",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        sel_grp_id,
        dev_tgt.pipe_id);
    return BF_ALREADY_EXISTS;
  }

  // Before we add the group, we first check the validity of the members to be
  // added if any and build up a vector of action entry handles and action
  // function handles to be used to pass to the pipe-mgr API
  std::vector<pipe_adt_ent_hdl_t> action_entry_hdls(members.size(), 0);
  std::vector<char> pipe_member_status(members.size(), 0);

  for (unsigned i = 0; i < members.size(); ++i) {
    // For each member verify if the member ID specified exists. If so, get the
    // action function handle
    pipe_adt_ent_hdl_t adt_ent_hdl = 0;

    status = act_profile_state->getActDetails(
        members[i], dev_tgt.pipe_id, &adt_ent_hdl);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in adding member id %d which does not exist into "
          "group id %d on pipe %x",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          members[i],
          sel_grp_id,
          dev_tgt.pipe_id);
      return BF_INVALID_ARG;
    }

    action_entry_hdls[i] = adt_ent_hdl;
    pipe_member_status[i] = member_status[i];
  }

  // Now, attempt to add the group;
  status = pipeMgr->pipeMgrSelGrpAdd(session.sessHandleGet(),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     max_grp_size,
                                     &sel_grp_hdl,
                                     0 /* Pipe API flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in adding group id %d pipe %x, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              sel_grp_id,
              dev_tgt.pipe_id,
              status);
    return status;
  }

  status = selector_state->stateSelectorAdd(
      dev_tgt.pipe_id, sel_grp_id, sel_grp_hdl, max_grp_size);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in adding selector group mapping for grp id %d pipe %x "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        sel_grp_id,
        dev_tgt.pipe_id,
        status);
    goto cleanup_pipe;
  }

  // Set the membership of the group
  status = pipeMgr->pipeMgrSelGrpMbrsSet(session.sessHandleGet(),
                                         pipe_dev_tgt,
                                         pipe_tbl_hdl,
                                         sel_grp_hdl,
                                         members.size(),
                                         action_entry_hdls.data(),
                                         (bool *)(pipe_member_status.data()),
                                         0 /* Pipe API flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error in setting membership for group id %d pipe %x, err "
        "%d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        sel_grp_id,
        dev_tgt.pipe_id,
        status);
    goto cleanup;
  }

  return BF_SUCCESS;
cleanup:
  temp_status =
      selector_state->stateSelectorRemove(dev_tgt.pipe_id, sel_grp_id);
  if (temp_status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s ERROR in removing selector table state as part of cleanup, "
        "error %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        temp_status);
    BF_RT_DBGCHK(0);
    return temp_status;
  }
cleanup_pipe:
  pipeMgr->pipeMgrSelGrpDel(session.sessHandleGet(),
                            pipe_dev_tgt,
                            pipe_tbl_hdl,
                            sel_grp_hdl,
                            0 /* Pipe API flags */);

  return status;
}

bf_status_t BfRtSelectorTable::tableEntryMod(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const BfRtTableKey &key,
                                             const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtSelectorTableKey &sel_key =
      static_cast<const BfRtSelectorTableKey &>(key);
  const BfRtSelectorTableData &sel_data =
      static_cast<const BfRtSelectorTableData &>(data);

  std::vector<bf_rt_id_t> members = sel_data.getMembers();
  std::vector<bool> member_status = sel_data.getMemberStatus();
  std::vector<pipe_adt_ent_hdl_t> action_entry_hdls(members.size(), 0);
  std::vector<char> pipe_member_status(members.size(), 0);

  // Get the mapping from selector group id to selector group handle

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  bf_rt_id_t sel_grp_id = sel_key.getGroupId();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Get device state and from that the selector group state of this table to do
  // various checks and update state.
  {
    std::lock_guard<std::mutex> lock(state_lock);
    auto device_state =
        BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
    auto selector_state =
        device_state->selectorState.getObjState(table_id_get());

    /* Get Act Prof state for action profile table associated with this selector
     * table */
    auto act_profile_state =
        device_state->actProfState.getObjState(getActProfId());

    if (selector_state == nullptr || act_profile_state == nullptr) {
      BF_RT_ASSERT(0);
      return BF_UNEXPECTED;
    }

    pipe_adt_ent_hdl_t adt_ent_hdl;

    // First, check if the group ID exists
    if (!selector_state->grpIdExists(
            dev_tgt.pipe_id, sel_grp_id, &sel_grp_hdl)) {
      LOG_TRACE(
          "%s:%d %s : Error in modifying a selector group id %d pipe %x which "
          "does not exist ",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          sel_grp_id,
          dev_tgt.pipe_id);
      return BF_OBJECT_NOT_FOUND;
    }

    /*
    FIXME Ignore the following check for the time being. Ideally we would want
    to expose a dataAllocate API which takes a vector of fields and then see if
    the max group size field exists in the data object and then validate it
    only of does exist. This issue is being tracked in DRV-2374.
    */
    /*
    // The max group size of the group cannot be changed. Verify the
    // max_group_size field in the passed in selector data obj matches
    // the already configured max_group_size for the group
    const auto max_grp_size = sel_data.get_max_grp_size();
    uint32_t configured_max_grp_size;
    status = selector_state->stateSelectorGetGrpSize(dev_tgt.pipe_id,sel_grp_id,
                                                    &configured_max_grp_size);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in getting configured max group size for selector "
          "group with grp id %d err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          sel_grp_id,
          status);
      return status;
    }
    if (max_grp_size != configured_max_grp_size) {
      LOG_TRACE(
          "%s:%d %s : ERROR : Max group size of an existing group with grp id "
          "%d cannot be modified. Please delete the group and re-add",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          sel_grp_id);
      return BF_NOT_SUPPORTED;
    }
    */

    // Next, validate the member IDs
    for (unsigned i = 0; i < members.size(); ++i) {
      status = act_profile_state->getActDetails(
          members[i], dev_tgt.pipe_id, &adt_ent_hdl);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s Error in adding member id %d which not exist into group "
            "id "
            "%d pipe %x",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            members[i],
            sel_grp_id,
            dev_tgt.pipe_id);
        return BF_INVALID_ARG;
      }
      action_entry_hdls[i] = adt_ent_hdl;
      pipe_member_status[i] = member_status[i];
    }
  }

  status = pipeMgr->pipeMgrSelGrpMbrsSet(session.sessHandleGet(),
                                         pipe_dev_tgt,
                                         pipe_tbl_hdl,
                                         sel_grp_hdl,
                                         members.size(),
                                         action_entry_hdls.data(),
                                         (bool *)(pipe_member_status.data()),
                                         0 /* Pipe API flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in setting membership for group id %d pipe %x, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        sel_grp_id,
        dev_tgt.pipe_id,
        status);
    return status;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::tableEntryDel(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const BfRtTableKey &key) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtSelectorTableKey &sel_key =
      static_cast<const BfRtSelectorTableKey &>(key);
  bf_rt_id_t sel_grp_id = sel_key.getGroupId();

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Get device state and from that the selector group state of this table to do
  // various checks.
  std::lock_guard<std::mutex> lock(state_lock);
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  auto selector_state = device_state->selectorState.getObjState(table_id_get());

  if (selector_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_UNEXPECTED;
  }

  // First, check if the group ID exists
  if (!selector_state->grpIdExists(dev_tgt.pipe_id, sel_grp_id, &sel_grp_hdl)) {
    LOG_TRACE(
        "%s:%d %s : Error in deleting a selector group id %d pipe %x which "
        "does not exist",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        sel_grp_id,
        dev_tgt.pipe_id);
    return BF_OBJECT_NOT_FOUND;
  }

  status = pipeMgr->pipeMgrSelGrpDel(session.sessHandleGet(),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     sel_grp_hdl,
                                     0 /* Pipe API flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error deleting selector group %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              sel_grp_id,
              status);
    return status;
  }
  // Update state to remove mapping corresponding to this group id
  status = selector_state->stateSelectorRemove(dev_tgt.pipe_id, sel_grp_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in removing selector table state for group id %d pipe "
        "%x, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        sel_grp_id,
        dev_tgt.pipe_id,
        status);
    BF_RT_DBGCHK(0);
    return status;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::tableClear(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t & /*flags*/
                                          ) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  bf_rt_id_t sel_grp_id = 0;

  std::lock_guard<std::mutex> lock(state_lock);
  // Get device state and from that the selector group state of this table to do
  // various checks.
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  auto selector_state = device_state->selectorState.getObjState(table_id_get());

  if (selector_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_UNEXPECTED;
  }

  // Keep getting first and delete it
  while (true) {
    status = this->getFirstGrp(dev_tgt, &sel_grp_id, &sel_grp_hdl);
    if (status == BF_OBJECT_NOT_FOUND) {
      LOG_DBG("%s:%d No first member found on pipe %x. All deleted.",
              __func__,
              __LINE__,
              dev_tgt.pipe_id);
      break;
    }
    status = pipeMgr->pipeMgrSelGrpDel(session.sessHandleGet(),
                                       pipe_dev_tgt,
                                       pipe_tbl_hdl,
                                       sel_grp_hdl,
                                       0 /* Pipe API flags */);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Error deleting selector group %d pipe %x, err %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                sel_grp_id,
                dev_tgt.pipe_id,
                status);
      return status;
    }
    // Update state to remove mapping corresponding to this group id
    status = selector_state->stateSelectorRemove(dev_tgt.pipe_id, sel_grp_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in removing selector table state for group id %d "
          "pipe %x, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          sel_grp_id,
          dev_tgt.pipe_id,
          status);
      BF_RT_DBGCHK(0);
      return status;
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const bf_rt_id_t &grp_id,
    BfRtSelectorTableData *sel_tbl_data) const {
  bf_status_t status;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  pipe_sel_grp_hdl_t sel_grp_hdl;
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto sel_state = device_state->selectorState.getObjState(table_id_get());
  auto act_profile_state =
      device_state->actProfState.getObjState(getActProfId());
  if (sel_state == nullptr || act_profile_state == nullptr) {
    return BF_OBJECT_NOT_FOUND;
  }

  dev_target_t pipe_dev_tgt;
  
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  
  // Get the selector group hdl from the group id
  bool found = sel_state->grpIdExists(dev_tgt.pipe_id, grp_id, &sel_grp_hdl);
  if (!found) {
    LOG_TRACE("%s:%d %s ERROR Grp Id %d does not exist on pipe %x",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    return BF_OBJECT_NOT_FOUND;
  }

  // Get the max size configured for the group
  uint32_t max_grp_size = 0;
  status = sel_state->stateSelectorGetGrpSize(
      dev_tgt.pipe_id, grp_id, &max_grp_size);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get size for Grp Id %d on pipe %x",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    BF_RT_DBGCHK(0);
    return status;
  }

  // Query pipe mgr for member and status list
  uint32_t count = 0;
  status = pipeMgr->pipeMgrGetSelGrpMbrCount(session.sessHandleGet(),
                                             pipe_dev_tgt,
                                             pipe_tbl_hdl,
                                             sel_grp_hdl,
                                             &count);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get info for Grp Id %d pipe %x",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    return status;
  }

  std::vector<pipe_adt_ent_hdl_t> pipe_members(count, 0);
  std::vector<char> pipe_member_status(count, 0);
  uint32_t mbrs_populated = 0;
  status = pipeMgr->pipeMgrSelGrpMbrsGet(session.sessHandleGet(),
                                         pipe_dev_tgt,
                                         pipe_tbl_hdl,
                                         sel_grp_hdl,
                                         count,
                                         pipe_members.data(),
                                         (bool *)(pipe_member_status.data()),
                                         &mbrs_populated);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get membership for Grp Id %d pipe %x",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    return status;
  }

  std::vector<bf_rt_id_t> member_ids;
  std::vector<bool> member_id_status;
  for (unsigned i = 0; i < mbrs_populated; i++) {
    bf_rt_id_t member_id = 0;
    status = act_profile_state->getMbrIdFromHndl(pipe_members[i], &member_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Error in getting member id for member hdl %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pipe_members[i]);
      return BF_INVALID_ARG;
    }

    member_ids.push_back(member_id);
    member_id_status.push_back(pipe_member_status[i]);
  }
  sel_tbl_data->setMembers(member_ids);
  sel_tbl_data->setMemberStatus(member_id_status);
  sel_tbl_data->setMaxGrpSize(max_grp_size);
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::tableEntryGet(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             const BfRtTableKey &key,
                                             BfRtTableData *data) const {
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_WARN(
        "%s:%d %s : Table entry get from hardware not supported"
        " Defaulting to sw read",
        __func__,
        __LINE__,
        table_name_get().c_str());
  }

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  std::lock_guard<std::mutex> lock(state_lock);
  const BfRtSelectorTableKey &sel_tbl_key =
      static_cast<const BfRtSelectorTableKey &>(key);
  BfRtSelectorTableData *sel_tbl_data =
      static_cast<BfRtSelectorTableData *>(data);
  bf_rt_id_t grp_id = sel_tbl_key.getGroupId();

  return tableEntryGet_internal(session, dev_tgt, grp_id, sel_tbl_data);
}

bf_status_t BfRtSelectorTable::tableEntryKeyGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtSelectorTableKey *sel_tbl_key = static_cast<BfRtSelectorTableKey *>(key);
  bf_rt_id_t grp_id;
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto sel_state = device_state->selectorState.getObjState(table_id_get());
  if (sel_state == nullptr) {
    return BF_OBJECT_NOT_FOUND;
  }
  bf_status_t status = sel_state->getGroupIdFromHndl(entry_handle, &grp_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR getting Group Id for handle 0x%x.",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              entry_handle);
    return status;
  }
  sel_tbl_key->setGroupId(grp_id);
  *entry_tgt = dev_tgt;
  return status;
}

bf_status_t BfRtSelectorTable::tableEntryHandleGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtSelectorTableKey &sel_tbl_key =
      static_cast<const BfRtSelectorTableKey &>(key);
  bf_rt_id_t grp_id = sel_tbl_key.getGroupId();
  // Get the selector group hdl from the group id
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto sel_state = device_state->selectorState.getObjState(table_id_get());
  if (sel_state == nullptr) {
    return BF_OBJECT_NOT_FOUND;
  }
  bool found = sel_state->grpIdExists(dev_tgt.pipe_id, grp_id, entry_handle);
  if (!found) {
    LOG_TRACE("%s:%d %s ERROR Grp Id %d does not exist on pipe %x",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    return BF_OBJECT_NOT_FOUND;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::tableEntryGet(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             const bf_rt_handle_t &entry_handle,
                                             BfRtTableKey *key,
                                             BfRtTableData *data) const {
  bf_rt_target_t entry_tgt;
  bf_status_t status = this->tableEntryKeyGet(
      session, dev_tgt, flags, entry_handle, &entry_tgt, key);
  if (status != BF_SUCCESS) {
    return status;
  }
  return this->tableEntryGet(session, entry_tgt, flags, *key, data);
}

bf_status_t BfRtSelectorTable::tableEntryGetFirst(const BfRtSession &session,
                                                  const bf_rt_target_t &dev_tgt,
                                                  const uint64_t &flags,
                                                  BfRtTableKey *key,
                                                  BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t table_id_from_data;
  const BfRtTable *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_WARN(
        "%s:%d %s : Table entry get from hardware not supported."
        " Defaulting to sw read",
        __func__,
        __LINE__,
        table_name_get().c_str());
  }

  if (table_id_from_data != this->table_id_get()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        table_id_from_data);
    return BF_INVALID_ARG;
  }

  BfRtSelectorTableKey *sel_tbl_key = static_cast<BfRtSelectorTableKey *>(key);
  BfRtSelectorTableData *sel_tbl_data =
      static_cast<BfRtSelectorTableData *>(data);

  bf_rt_id_t first_grp_id;
  pipe_sel_grp_hdl_t first_grp_hdl;
  std::lock_guard<std::mutex> lock(state_lock);
  status = this->getFirstGrp(dev_tgt, &first_grp_id, &first_grp_hdl);
  if (status == BF_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  status = tableEntryGet_internal(session, dev_tgt, first_grp_id, sel_tbl_data);
  if (status != BF_SUCCESS) {
    return status;
  }
  sel_tbl_key->setGroupId(first_grp_id);
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtSelectorTableKey &sel_tbl_key =
      static_cast<const BfRtSelectorTableKey &>(key);

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_WARN(
        "%s:%d %s : Table entry get from hardware not supported"
        " Defaulting to sw read",
        __func__,
        __LINE__,
        table_name_get().c_str());
  }

  bf_rt_id_t grp_id = sel_tbl_key.getGroupId();
  bf_rt_id_t next_grp_id;
  pipe_sel_grp_hdl_t next_grp_hdl;

  unsigned i = 0;
  for (i = 0; i < n; i++) {
    std::lock_guard<std::mutex> lock(state_lock);
    status = this->getNextGrp(dev_tgt, grp_id, &next_grp_id, &next_grp_hdl);
    if (status != BF_SUCCESS) {
      break;
    }
    auto this_key =
        static_cast<BfRtSelectorTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtSelectorTableData *>((*key_data_pairs)[i].second);
    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != this->table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d does not match "
          "the table",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }
    status = tableEntryGet_internal(session, dev_tgt, next_grp_id, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR in getting %dth entry from pipe-mgr with group "
          "handle %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          i + 1,
          next_grp_hdl,
          status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    this_key->setGroupId(next_grp_id);
    grp_id = next_grp_id;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::getFirstGrp(
    const bf_rt_target_t &dev_tgt,
    bf_rt_id_t *first_grp_id,
    pipe_sel_grp_hdl_t *first_grp_hdl) const {
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto sel_state = device_state->selectorState.getObjState(table_id_get());
  if (sel_state == nullptr) {
    return BF_OBJECT_NOT_FOUND;
  }
  return sel_state->getFirstGrp(dev_tgt.pipe_id, first_grp_id, first_grp_hdl);
}

bf_status_t BfRtSelectorTable::getNextGrp(
    const bf_rt_target_t &dev_tgt,
    bf_rt_id_t grp_id,
    bf_rt_id_t *next_grp_id,
    pipe_sel_grp_hdl_t *next_grp_hdl) const {
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto sel_state = device_state->selectorState.getObjState(table_id_get());
  if (sel_state == nullptr) {
    return BF_OBJECT_NOT_FOUND;
  }
  return sel_state->getNextGrp(
      dev_tgt.pipe_id, grp_id, next_grp_id, next_grp_hdl);
}

bf_status_t BfRtSelectorTable::getOneMbr(const BfRtSession &session,
                                         const bf_rt_target_t &dev_tgt,
                                         const pipe_sel_grp_hdl_t sel_grp_hdl,
                                         pipe_adt_ent_hdl_t *member_hdl) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;

  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return pipeMgr->pipeMgrGetFirstGroupMember(session.sessHandleGet(),
                                             pipe_tbl_hdl,
                                             pipe_dev_tgt,
                                             sel_grp_hdl,
                                             member_hdl);
}

bf_status_t BfRtSelectorTable::getGrpIdFromHndl(
    const uint16_t &device_id,
    const pipe_sel_grp_hdl_t &sel_grp_hdl,
    bf_rt_id_t *sel_grp_id) const {
  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(device_id, prog_name);
  if (device_state == nullptr) {
    LOG_TRACE("%s:%d %s Unable to get device state for dev %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              device_id);
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto sel_state = device_state->selectorState.getObjState(table_id_get());
  if (sel_state == nullptr) {
    LOG_TRACE("%s:%d %s Unable to get selector state for dev %d table %d ",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              device_id,
              table_id_get());
    BF_RT_DBGCHK(0);
    return BF_OBJECT_NOT_FOUND;
  }
  return sel_state->getGroupIdFromHndl(sel_grp_hdl, sel_grp_id);
}

bf_status_t BfRtSelectorTable::getActMbrIdFromHndl(
    const uint16_t &device_id,
    const pipe_adt_ent_hdl_t &adt_ent_hdl,
    bf_rt_id_t *act_mbr_id) const {
  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(device_id, prog_name);
  if (device_state == nullptr) {
    LOG_TRACE("%s:%d %s Unable to get device state for dev %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              device_id);
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto act_profile_state =
      device_state->actProfState.getObjState(getActProfId());

  return act_profile_state->getMbrIdFromHndl(adt_ent_hdl, act_mbr_id);
}

bf_status_t BfRtSelectorTable::tableUsageGet(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

bf_status_t BfRtSelectorTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtSelectorTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::keyReset(BfRtTableKey *key) const {
  BfRtSelectorTableKey *sel_key = static_cast<BfRtSelectorTableKey *>(key);
  return key_reset<BfRtSelectorTable, BfRtSelectorTableKey>(*this, sel_key);
}

bf_status_t BfRtSelectorTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  const std::vector<bf_rt_id_t> fields{};
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtSelectorTableData(this, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error in allocating data",
              __func__,
              __LINE__,
              table_name_get().c_str())
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtSelectorTableData(this, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error in allocating data",
              __func__,
              __LINE__,
              table_name_get().c_str())
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::dataReset(BfRtTableData *data) const {
  BfRtSelectorTableData *sel_data = static_cast<BfRtSelectorTableData *>(data);
  if (!this->validateTable_from_dataObj(*sel_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return sel_data->reset();
}

bf_status_t BfRtSelectorTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  if (type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s ERROR Invalid Attribute type (%d)"
        "set "
        "attributes",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        static_cast<int>(type));
    return BF_INVALID_ARG;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  if (type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s ERROR Invalid Attribute type (%d)"
        "set "
        "attributes",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        static_cast<int>(type));
    return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtSelectorTable::processSelUpdateCbAttr(
    const BfRtTableAttributesImpl &tbl_attr_impl,
    const bf_rt_target_t &dev_tgt) const {
  // 1. From the table attribute object, get the selector update parameters.
  // 2. Make a table attribute state object to store the parameters that are
  // required for when the callback is invoked.
  // 3. Invoke pipe-mgr callback registration function to register BF-RT
  // internal callback.
  auto t = tbl_attr_impl.selectorUpdateCbInternalGet();

  auto enable = std::get<0>(t);
  auto session = std::get<1>(t);
  auto cpp_callback_fn = std::get<2>(t);
  auto c_callback_fn = std::get<3>(t);
  auto cookie = std::get<4>(t);

  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    LOG_TRACE("%s:%d %s Unable to get device state for dev %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.dev_id);
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }
  auto session_obj = session.lock();

  if (session_obj == nullptr) {
    LOG_TRACE("%s:%d %s ERROR Session object passed no longer exists",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Get the state
  auto attributes_state =
      device_state->attributesState.getObjState(table_id_get());

  BfRtStateSelUpdateCb sel_update_cb(
      enable, this, session, cpp_callback_fn, c_callback_fn, cookie);

  attributes_state->setSelUpdateCbObj(sel_update_cb);

  auto *pipeMgr = PipeMgrIntf::getInstance();
  bf_status_t status = pipeMgr->pipeMgrSelTblRegisterCb(
      session_obj->sessHandleGet(),
      dev_tgt.dev_id,
      pipe_tbl_hdl,
      selUpdatePipeMgrInternalCb,
      &(attributes_state->getSelUpdateCbObj()));

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in registering selector update callback with pipe-mgr, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    // Reset the selector update callback object, since pipeMgr registration did
    // not succeed
    attributes_state->resetSelUpdateCbObj();
    return status;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorTable::tableAttributesSet(
    const BfRtSession & /*session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();

  if (attr_type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
        "set "
        "attributes",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        static_cast<int>(attr_type));
    return BF_INVALID_ARG;
  }
  return this->processSelUpdateCbAttr(*tbl_attr_impl, dev_tgt);
}

bf_status_t BfRtSelectorTable::tableAttributesGet(
    const BfRtSession & /*session */,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // 1. From the table attribute state, retrieve all the params that were
  // registered by the user
  // 2. Set the params in the passed in tableAttributes obj

  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  if (attr_type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
        "set "
        "attributes",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        static_cast<int>(attr_type));
    return BF_INVALID_ARG;
  }

  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  if (device_state == nullptr) {
    LOG_TRACE("%s:%d %s Unable to get device state for dev %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.dev_id);
    BF_RT_ASSERT(0);
    return BF_OBJECT_NOT_FOUND;
  }

  // Get the state
  auto attributes_state =
      device_state->attributesState.getObjState(table_id_get());
  BfRtStateSelUpdateCb sel_update_cb;
  attributes_state->getSelUpdateCbObj(&sel_update_cb);

  // Set the state in the attribute object
  auto state_param = sel_update_cb.stateGet();
  tbl_attr_impl->selectorUpdateCbInternalSet(
      std::make_tuple(std::get<0>(state_param),
                      std::get<2>(state_param),
                      std::get<3>(state_param),
                      std::get<4>(state_param),
                      std::get<5>(state_param)));

  return BF_SUCCESS;
}
// COUNTER TABLE APIS

bf_status_t BfRtCounterTable::tableEntryAdd(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const BfRtTableKey &key,
                                            const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtCounterTableKey &cntr_key =
      static_cast<const BfRtCounterTableKey &>(key);
  const BfRtCounterTableData &cntr_data =
      static_cast<const BfRtCounterTableData &>(data);

  uint32_t counter_id = cntr_key.getCounterId();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, counter_id)) {
    return BF_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  const pipe_stat_data_t *stat_data =
      cntr_data.getCounterSpecObj().getPipeCounterSpec();
  status =
      pipeMgr->pipeMgrStatEntSet(session.sessHandleGet(),
                                 pipe_dev_tgt,
                                 pipe_tbl_hdl,
                                 counter_id,
                                 const_cast<pipe_stat_data_t *>(stat_data));

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in adding/modifying counter index %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              counter_id,
              status);
    return status;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::tableEntryMod(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags,
                                            const BfRtTableKey &key,
                                            const BfRtTableData &data) const {
  return tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtCounterTable::tableEntryGet(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags,
                                            const BfRtTableKey &key,
                                            BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtCounterTableKey &cntr_key =
      static_cast<const BfRtCounterTableKey &>(key);
  BfRtCounterTableData *cntr_data = static_cast<BfRtCounterTableData *>(data);

  uint32_t counter_id = cntr_key.getCounterId();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, counter_id)) {
    return BF_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    status = pipeMgr->pipeMgrStatEntDatabaseSync(
        session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, counter_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR in getting counter value from hardware for counter "
          "idx %d, err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          counter_id,
          status);
      return status;
    }
  }

  pipe_stat_data_t stat_data = {0};
  status = pipeMgr->pipeMgrStatEntQuery(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        counter_id,
                                        &stat_data);

  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in reading counter value for counter idx %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        counter_id,
        status);
    return status;
  }

  cntr_data->getCounterSpecObj().setCounterDataFromCounterSpec(stat_data);

  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::tableEntryGet(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags,
                                            const bf_rt_handle_t &entry_handle,
                                            BfRtTableKey *key,
                                            BfRtTableData *data) const {
  BfRtCounterTableKey *cntr_key = static_cast<BfRtCounterTableKey *>(key);
  cntr_key->setCounterId(entry_handle);
  return this->tableEntryGet(
      session, dev_tgt, flags, static_cast<const BfRtTableKey &>(*key), data);
}

bf_status_t BfRtCounterTable::tableEntryKeyGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtCounterTableKey *cntr_key = static_cast<BfRtCounterTableKey *>(key);
  cntr_key->setCounterId(entry_handle);
  *entry_tgt = dev_tgt;
  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::tableEntryHandleGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtCounterTableKey &cntr_key =
      static_cast<const BfRtCounterTableKey &>(key);
  *entry_handle = cntr_key.getCounterId();
  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::tableEntryGetFirst(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t &flags,
                                                 BfRtTableKey *key,
                                                 BfRtTableData *data) const {
  BfRtCounterTableKey *cntr_key = static_cast<BfRtCounterTableKey *>(key);
  return getFirst_for_resource_tbls<BfRtCounterTable, BfRtCounterTableKey>(
      *this, session, dev_tgt, flags, cntr_key, data);
}

bf_status_t BfRtCounterTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const BfRtCounterTableKey &cntr_key =
      static_cast<const BfRtCounterTableKey &>(key);
  return getNext_n_for_resource_tbls<BfRtCounterTable, BfRtCounterTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      cntr_key,
      n,
      key_data_pairs,
      num_returned);
}

bf_status_t BfRtCounterTable::tableClear(const BfRtSession &session,
                                         const bf_rt_target_t &dev_tgt,
                                         const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status = pipeMgr->pipeMgrStatTableReset(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, nullptr);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing counter table err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return status;
}

bf_status_t BfRtCounterTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtCounterTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::keyReset(BfRtTableKey *key) const {
  BfRtCounterTableKey *counter_key = static_cast<BfRtCounterTableKey *>(key);
  return key_reset<BfRtCounterTable, BfRtCounterTableKey>(*this, counter_key);
}

bf_status_t BfRtCounterTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtCounterTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtCounterTable::dataReset(BfRtTableData *data) const {
  BfRtCounterTableData *counter_data =
      static_cast<BfRtCounterTableData *>(data);
  if (!this->validateTable_from_dataObj(*counter_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return counter_data->reset();
}

// METER TABLE

bf_status_t BfRtMeterTable::tableEntryAdd(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t & /*flags*/,
                                          const BfRtTableKey &key,
                                          const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMeterTableKey &meter_key =
      static_cast<const BfRtMeterTableKey &>(key);
  const BfRtMeterTableData &meter_data =
      static_cast<const BfRtMeterTableData &>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  const pipe_meter_spec_t *meter_spec =
      meter_data.getMeterSpecObj().getPipeMeterSpec();
  pipe_meter_idx_t meter_idx = meter_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, meter_idx)) {
    return BF_INVALID_ARG;
  }

  return pipeMgr->pipeMgrMeterEntSet(session.sessHandleGet(),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     meter_idx,
                                     (pipe_meter_spec_t *)meter_spec,
                                     0 /* Pipe API flags */);
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::tableEntryMod(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t &flags,
                                          const BfRtTableKey &key,
                                          const BfRtTableData &data) const {
  return tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtMeterTable::tableEntryGet(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t &flags,
                                          const BfRtTableKey &key,
                                          BfRtTableData *data) const {
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    LOG_TRACE("%s:%d %s ERROR : Read from hardware not supported",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMeterTableKey &meter_key =
      static_cast<const BfRtMeterTableKey &>(key);
  BfRtMeterTableData *meter_data = static_cast<BfRtMeterTableData *>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_meter_spec_t meter_spec;
  std::memset(&meter_spec, 0, sizeof(meter_spec));

  pipe_meter_idx_t meter_idx = meter_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, meter_idx)) {
    return BF_INVALID_ARG;
  }

  status = pipeMgr->pipeMgrMeterReadEntryIdx(session.sessHandleGet(),
                                             pipe_dev_tgt,
                                             pipe_tbl_hdl,
                                             meter_idx,
                                             &meter_spec);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading meter entry idx %d, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              meter_idx,
              status);
    return status;
  }

  // Populate data elements right here
  meter_data->getMeterSpecObj().setMeterDataFromMeterSpec(meter_spec);
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::tableEntryGet(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t &flags,
                                          const bf_rt_handle_t &entry_handle,
                                          BfRtTableKey *key,
                                          BfRtTableData *data) const {
  BfRtMeterTableKey *mtr_key = static_cast<BfRtMeterTableKey *>(key);
  mtr_key->setIdxKey(entry_handle);
  return this->tableEntryGet(
      session, dev_tgt, flags, static_cast<const BfRtTableKey &>(*key), data);
}

bf_status_t BfRtMeterTable::tableEntryKeyGet(const BfRtSession & /*session*/,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const bf_rt_handle_t &entry_handle,
                                             bf_rt_target_t *entry_tgt,
                                             BfRtTableKey *key) const {
  BfRtMeterTableKey *mtr_key = static_cast<BfRtMeterTableKey *>(key);
  mtr_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::tableEntryHandleGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtMeterTableKey &mtr_key =
      static_cast<const BfRtMeterTableKey &>(key);
  *entry_handle = mtr_key.getIdxKey();
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::tableEntryGetFirst(const BfRtSession &session,
                                               const bf_rt_target_t &dev_tgt,
                                               const uint64_t &flags,
                                               BfRtTableKey *key,
                                               BfRtTableData *data) const {
  BfRtMeterTableKey *meter_key = static_cast<BfRtMeterTableKey *>(key);
  return getFirst_for_resource_tbls<BfRtMeterTable, BfRtMeterTableKey>(
      *this, session, dev_tgt, flags, meter_key, data);
}

bf_status_t BfRtMeterTable::tableEntryGetNext_n(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                const BfRtTableKey &key,
                                                const uint32_t &n,
                                                keyDataPairs *key_data_pairs,
                                                uint32_t *num_returned) const {
  const BfRtMeterTableKey &meter_key =
      static_cast<const BfRtMeterTableKey &>(key);
  return getNext_n_for_resource_tbls<BfRtMeterTable, BfRtMeterTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      meter_key,
      n,
      key_data_pairs,
      num_returned);
}

bf_status_t BfRtMeterTable::tableClear(const BfRtSession &session,
                                       const bf_rt_target_t &dev_tgt,
                                       const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status = pipeMgr->pipeMgrMeterReset(session.sessHandleGet(),
                                                  pipe_dev_tgt,
                                                  this->pipe_tbl_hdl,
                                                  0 /* Pipe API flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in CLearing Meter table, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return status;
}

bf_status_t BfRtMeterTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtMeterTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::keyReset(BfRtTableKey *key) const {
  BfRtMeterTableKey *meter_key = static_cast<BfRtMeterTableKey *>(key);
  return key_reset<BfRtMeterTable, BfRtMeterTableKey>(*this, meter_key);
}

bf_status_t BfRtMeterTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtMeterTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::dataReset(BfRtTableData *data) const {
  BfRtMeterTableData *meter_data = static_cast<BfRtMeterTableData *>(data);
  if (!this->validateTable_from_dataObj(*meter_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return meter_data->reset();
}

bf_status_t BfRtMeterTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Unable to reset attribute",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtMeterTable::tableAttributesSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      bf_status_t sts = tbl_attr_impl->meterByteCountAdjGet(&byte_count);
      if (sts != BF_SUCCESS) {
        return sts;
      }
      auto *pipeMgr = PipeMgrIntf::getInstance();
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      return pipeMgr->pipeMgrMeterByteCountSet(
          session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, byte_count);
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtMeterTable::tableAttributesGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      auto *pipeMgr = PipeMgrIntf::getInstance();
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      pipeMgr->pipeMgrMeterByteCountGet(
          session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, &byte_count);
      return tbl_attr_impl->meterByteCountAdjSet(byte_count);
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

// REGISTER TABLE
bf_status_t BfRtRegisterTable::tableEntryAdd(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const BfRtTableKey &key,
                                             const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtRegisterTableKey &register_key =
      static_cast<const BfRtRegisterTableKey &>(key);
  const BfRtRegisterTableData &register_data =
      static_cast<const BfRtRegisterTableData &>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_stful_mem_idx_t register_idx = register_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, register_idx)) {
    return BF_INVALID_ARG;
  }

  pipe_stful_mem_spec_t stful_spec;
  std::memset(&stful_spec, 0, sizeof(stful_spec));

  std::vector<bf_rt_id_t> dataFields;
  bf_status_t status = dataFieldIdListGet(&dataFields);
  BF_RT_ASSERT(status == BF_SUCCESS);
  const auto &register_spec_data = register_data.getRegisterSpecObj();
  const BfRtTableDataField *tableDataField;
  status = getDataField(dataFields[0], &tableDataField);
  BF_RT_ASSERT(status == BF_SUCCESS);
  register_spec_data.populateStfulSpecFromData(&stful_spec);

  return pipeMgr->pipeStfulEntSet(session.sessHandleGet(),
                                  pipe_dev_tgt,
                                  pipe_tbl_hdl,
                                  register_idx,
                                  &stful_spec,
                                  0 /* Pipe API flags */);
}

bf_status_t BfRtRegisterTable::tableEntryMod(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             const BfRtTableKey &key,
                                             const BfRtTableData &data) const {
  return tableEntryAdd(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtRegisterTable::tableEntryGet(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             const BfRtTableKey &key,
                                             BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtRegisterTableKey &register_key =
      static_cast<const BfRtRegisterTableKey &>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_stful_mem_spec_t stful_spec;
  std::memset(&stful_spec, 0, sizeof(stful_spec));

  // Query number of pipes to get possible number of results.
  int num_pipes = 0;
  status = pipeMgr->pipeStfulQueryGetSizes(
      session.sessHandleGet(), dev_tgt.dev_id, pipe_tbl_hdl, &num_pipes);

  // Use vectors to populate pipe mgr stful query data structure.
  // One vector to hold all possible pipe data.
  std::vector<pipe_stful_mem_spec_t> register_pipe_data(num_pipes);
  pipe_stful_mem_query_t stful_query;
  stful_query.data = register_pipe_data.data();
  stful_query.pipe_count = num_pipes;

  uint32_t pipe_api_flags = 0;
  if (BF_RT_FLAG_IS_SET(flags, BF_RT_FROM_HW)) {
    pipe_api_flags = PIPE_FLAG_SYNC_REQ;
  }
  pipe_stful_mem_idx_t register_idx = register_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, register_idx)) {
    return BF_INVALID_ARG;
  }

  status = pipeMgr->pipeStfulEntQuery(session.sessHandleGet(),
                                      pipe_dev_tgt,
                                      pipe_tbl_hdl,
                                      register_idx,
                                      &stful_query,
                                      pipe_api_flags);

  if (status == BF_SUCCESS) {
    std::vector<bf_rt_id_t> dataFields;
    status = dataFieldIdListGet(&dataFields);
    BF_RT_ASSERT(status == BF_SUCCESS);
    // Down cast to BfRtRegisterTableData
    BfRtRegisterTableData *register_data =
        static_cast<BfRtRegisterTableData *>(data);
    auto &register_spec_data = register_data->getRegisterSpecObj();
    const BfRtTableDataField *tableDataField;
    status = getDataField(dataFields[0], &tableDataField);
    BF_RT_ASSERT(status == BF_SUCCESS);
    // pipe_count is returned upon successful query,
    // hence use it instead of vector size.
    register_spec_data.populateDataFromStfulSpec(
        register_pipe_data, static_cast<uint32_t>(stful_query.pipe_count));
  }
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::tableEntryGet(const BfRtSession &session,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t &flags,
                                             const bf_rt_handle_t &entry_handle,
                                             BfRtTableKey *key,
                                             BfRtTableData *data) const {
  BfRtRegisterTableKey *reg_key = static_cast<BfRtRegisterTableKey *>(key);
  reg_key->setIdxKey(entry_handle);
  return this->tableEntryGet(
      session, dev_tgt, flags, static_cast<const BfRtTableKey &>(*key), data);
}

bf_status_t BfRtRegisterTable::tableEntryKeyGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtRegisterTableKey *reg_key = static_cast<BfRtRegisterTableKey *>(key);
  reg_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::tableEntryHandleGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtRegisterTableKey &reg_key =
      static_cast<const BfRtRegisterTableKey &>(key);
  *entry_handle = reg_key.getIdxKey();
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::tableEntryGetFirst(const BfRtSession &session,
                                                  const bf_rt_target_t &dev_tgt,
                                                  const uint64_t &flags,
                                                  BfRtTableKey *key,
                                                  BfRtTableData *data) const {
  BfRtRegisterTableKey *register_key = static_cast<BfRtRegisterTableKey *>(key);
  return getFirst_for_resource_tbls<BfRtRegisterTable, BfRtRegisterTableKey>(
      *this, session, dev_tgt, flags, register_key, data);
}

bf_status_t BfRtRegisterTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const BfRtRegisterTableKey &register_key =
      static_cast<const BfRtRegisterTableKey &>(key);
  return getNext_n_for_resource_tbls<BfRtRegisterTable, BfRtRegisterTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      register_key,
      n,
      key_data_pairs,
      num_returned);
}

bf_status_t BfRtRegisterTable::tableClear(const BfRtSession &session,
                                          const bf_rt_target_t &dev_tgt,
                                          const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status = pipeMgr->pipeStfulTableReset(
      session.sessHandleGet(), pipe_dev_tgt, pipe_tbl_hdl, nullptr);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing register table, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  return status;
}

bf_status_t BfRtRegisterTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtRegisterTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s ERROR : Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::keyReset(BfRtTableKey *key) const {
  BfRtRegisterTableKey *register_key = static_cast<BfRtRegisterTableKey *>(key);
  return key_reset<BfRtRegisterTable, BfRtRegisterTableKey>(*this,
                                                            register_key);
}

bf_status_t BfRtRegisterTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret = std::unique_ptr<BfRtTableData>(new BfRtRegisterTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s ERROR : Memory allocation failed",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::dataReset(BfRtTableData *data) const {
  BfRtRegisterTableData *register_data =
      static_cast<BfRtRegisterTableData *>(data);
  if (!this->validateTable_from_dataObj(*register_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return register_data->reset();
}

bf_status_t BfRtRegisterTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Unable to reset attribute",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtRegisterTable::tableAttributesSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(0);
      bf_status_t sts = tbl_attr_impl->entryScopeParamsGet(
          &entry_scope,
          static_cast<BfRtTableEntryScopeArguments *>(&scope_args));
      if (sts != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Unable to get the entry scope params",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return sts;
      }
      // Call the pipe mgr API to set entry scope
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      // Derive pipe mgr entry scope from BFRT entry scope and set it to
      // property value
      prop_val.value =
          entry_scope == TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
              ? PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
              : entry_scope == TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                    ? PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                    : PIPE_MGR_ENTRY_SCOPE_USER_DEFINED;
      std::bitset<32> bitval;
      scope_args.getValue(&bitval);
      args_val.value = bitval.to_ulong();
      auto *pipeMgr = PipeMgrIntf::getInstance();
      // We call the pipe mgr tbl property API on the compiler generated
      // table
      return pipeMgr->pipeMgrTblSetProperty(session.sessHandleGet(),
                                            dev_tgt.dev_id,
                                            ghost_pipe_tbl_hdl_,
                                            prop_type,
                                            prop_val,
                                            args_val);
    }
    case TableAttributesType::IDLE_TABLE_RUNTIME:
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::tableAttributesGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<BfRtTableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      auto *pipeMgr = PipeMgrIntf::getInstance();
      // We call the pipe mgr tbl property API on the compiler generated
      // table
      auto sts = pipeMgr->pipeMgrTblGetProperty(session.sessHandleGet(),
                                                dev_tgt.dev_id,
                                                ghost_pipe_tbl_hdl_,
                                                prop_type,
                                                &prop_val,
                                                &args_val);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s Failed to get entry scope from pipe_mgr for table %s",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            table_name_get().c_str());
        return sts;
      }

      TableEntryScope entry_scope;
      BfRtTableEntryScopeArgumentsImpl scope_args(args_val.value);

      // Derive BFRT entry scope from pipe mgr entry scope
      entry_scope = prop_val.value == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
                        ? TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
                        : prop_val.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                              ? TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                              : TableEntryScope::ENTRY_SCOPE_USER_DEFINED;

      return tbl_attr_impl->entryScopeParamsSet(
          entry_scope, static_cast<BfRtTableEntryScopeArguments &>(scope_args));
    }
    case TableAttributesType::IDLE_TABLE_RUNTIME:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          static_cast<int>(attr_type));
      BF_RT_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterTable::ghostTableHandleSet(
    const pipe_tbl_hdl_t &pipe_hdl) {
  ghost_pipe_tbl_hdl_ = pipe_hdl;
  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtMatchActionKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::keyReset(BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  return key_reset<BfRtPhase0Table, BfRtMatchActionKey>(*this, match_key);
}

bf_status_t BfRtPhase0Table::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  const auto item = action_info_list.begin();
  const auto action_id = item->first;
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPhase0TableData(this, action_id));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s : ERROR Unable to allocate data. Out of Memory",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::tableEntryAdd(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const BfRtTableKey &key,
                                           const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  const BfRtPhase0TableData &match_data =
      static_cast<const BfRtPhase0TableData &>(data);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  match_key.populate_match_spec(&pipe_match_spec);
  pipe_action_spec = match_data.get_pipe_action_spec();
  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return pipeMgr->pipeMgrMatEntAdd(session.sessHandleGet(),
                                   pipe_dev_tgt,
                                   pipe_tbl_hdl,
                                   &pipe_match_spec,
                                   act_fn_hdl,
                                   pipe_action_spec,
                                   0 /* ttl */,
                                   0 /* Pipe API flags */,
                                   &pipe_entry_hdl);
}

bf_status_t BfRtPhase0Table::tableEntryMod(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const BfRtTableKey &key,
                                           const BfRtTableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_mat_ent_hdl_t pipe_entry_hdl;
  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  const BfRtPhase0TableData &match_data =
      static_cast<const BfRtPhase0TableData &>(data);
  const pipe_action_spec_t *pipe_action_spec = nullptr;
  pipe_action_spec = match_data.get_pipe_action_spec();
  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();

  status = pipeMgr->pipeMgrMatEntSetAction(
      session.sessHandleGet(),
      pipe_dev_tgt.device_id,
      tablePipeHandleGet(),
      pipe_entry_hdl,
      act_fn_hdl,
      const_cast<pipe_action_spec_t *>(pipe_action_spec),
      0 /* Pipe API flags */);

  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in modifying table data err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::tableEntryDel(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const BfRtTableKey &key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status =
      pipeMgr->pipeMgrMatEntDelByMatchSpec(session.sessHandleGet(),
                                           pipe_dev_tgt,
                                           pipe_tbl_hdl,
                                           &pipe_match_spec,
                                           0 /* Pipe api flags */);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Entry delete failed for table %s, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtPhase0Table::tableClear(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const uint64_t & /*flags*/) const {
  return tableClearMatCommon(session, dev_tgt, false, this);
}

bf_status_t BfRtPhase0Table::tableEntryGet_internal(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const pipe_mat_ent_hdl_t &pipe_entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    BfRtTableData *data) const {
  BfRtPhase0TableData &match_data = static_cast<BfRtPhase0TableData &>(*data);
  pipe_action_spec_t *pipe_action_spec = match_data.get_pipe_action_spec();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;

  bf_status_t status = getActionSpec(session,
                                     pipe_dev_tgt,
                                     flags,
                                     pipe_tbl_hdl,
                                     pipe_entry_hdl,
                                     PIPE_RES_GET_FLAG_ENTRY,
                                     pipe_match_spec,
                                     pipe_action_spec,
                                     &pipe_act_fn_hdl,
                                     nullptr);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        pipe_entry_hdl,
        status);
    return status;
  }
  bf_rt_id_t action_id = this->getActIdFromActFnHdl(pipe_act_fn_hdl);
  match_data.actionIdSet(action_id);
  std::vector<bf_rt_id_t> empty;
  match_data.setActiveFields(empty);
  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::tableEntryGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           const BfRtTableKey &key,
                                           BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;

  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return this->tableEntryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, &pipe_match_spec, data);
}

bf_status_t BfRtPhase0Table::tableEntryKeyGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t &entry_handle,
    bf_rt_target_t *entry_tgt,
    BfRtTableKey *key) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_tbl_match_spec_t *pipe_match_spec;
  bf_dev_pipe_t entry_pipe;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  bf_status_t status = pipeMgr->pipeMgrEntHdlToMatchSpec(
      session.sessHandleGet(),
      pipe_dev_tgt,
      pipe_tbl_hdl,
      entry_handle,
      &entry_pipe,
      const_cast<const pipe_tbl_match_spec_t **>(&pipe_match_spec));
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  entry_tgt->pipe_id = entry_pipe;
  match_key->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  return status;
}

bf_status_t BfRtPhase0Table::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        entry_handle);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtPhase0Table::tableEntryGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           const bf_rt_handle_t &entry_handle,
                                           BfRtTableKey *key,
                                           BfRtTableData *data) const {
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key->populate_match_spec(&pipe_match_spec);

  auto status = tableEntryGet_internal(
      session, dev_tgt, flags, entry_handle, &pipe_match_spec, data);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

bf_status_t BfRtPhase0Table::tableEntryGetFirst(const BfRtSession &session,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t &flags,
                                                BfRtTableKey *key,
                                                BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  uint32_t pipe_entry_hdl = 0;
  bf_status_t status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.sessHandleGet(), pipe_tbl_hdl, pipe_dev_tgt, &pipe_entry_hdl);
  if (status == BF_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
  }

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  BfRtMatchActionKey *match_key = static_cast<BfRtMatchActionKey *>(key);
  match_key->populate_match_spec(&pipe_match_spec);

  BfRtPhase0TableData *match_data = static_cast<BfRtPhase0TableData *>(data);
  pipe_action_spec_t *pipe_action_spec = nullptr;
  pipe_action_spec = match_data->get_pipe_action_spec();
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
  status = getActionSpec(session,
                         pipe_dev_tgt,
                         flags,
                         pipe_tbl_hdl,
                         pipe_entry_hdl,
                         PIPE_RES_GET_FLAG_ENTRY,
                         &pipe_match_spec,
                         pipe_action_spec,
                         &pipe_act_fn_hdl,
                         nullptr);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
        "err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        pipe_entry_hdl,
        status);
    return status;
  }
  bf_rt_id_t action_id = this->getActIdFromActFnHdl(pipe_act_fn_hdl);
  match_data->actionIdSet(action_id);
  std::vector<bf_rt_id_t> empty;
  match_data->setActiveFields(empty);

  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::tableEntryGetNext_n(const BfRtSession &session,
                                                 const bf_rt_target_t &dev_tgt,
                                                 const uint64_t &flags,
                                                 const BfRtTableKey &key,
                                                 const uint32_t &n,
                                                 keyDataPairs *key_data_pairs,
                                                 uint32_t *num_returned) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const BfRtMatchActionKey &match_key =
      static_cast<const BfRtMatchActionKey &>(key);

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;

  bf_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.sessHandleGet(),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  std::vector<uint32_t> next_entry_handles(n, 0);
  status = pipeMgr->pipeMgrGetNextEntryHandles(session.sessHandleGet(),
                                               pipe_tbl_hdl,
                                               pipe_dev_tgt,
                                               pipe_entry_hdl,
                                               n,
                                               next_entry_handles.data());
  if (status == BF_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return BF_SUCCESS;
  }
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting next entry handles from pipe-mgr, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        status);
    return status;
  }

  uint32_t i = 0;
  std::vector<bf_rt_id_t> empty;
  for (i = 0; i < n; i++) {
    memset(&pipe_match_spec, 0, sizeof(pipe_match_spec));
    auto this_key =
        static_cast<BfRtMatchActionKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtPhase0TableData *>((*key_data_pairs)[i].second);
    bf_rt_id_t table_id_from_data;
    const BfRtTable *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);
    if (table_id_from_data != this->table_id_get()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          table_id_from_data);
      return BF_INVALID_ARG;
    }
    if (next_entry_handles[i] == (uint32_t)-1) {
      break;
    }

    this_key->populate_match_spec(&pipe_match_spec);
    pipe_action_spec_t *pipe_action_spec = nullptr;
    pipe_action_spec = this_data->get_pipe_action_spec();
    pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
    status = getActionSpec(session,
                           pipe_dev_tgt,
                           flags,
                           pipe_tbl_hdl,
                           next_entry_handles[i],
                           PIPE_RES_GET_FLAG_ENTRY,
                           &pipe_match_spec,
                           pipe_action_spec,
                           &pipe_act_fn_hdl,
                           nullptr);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
          "err %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          pipe_entry_hdl,
          status);
      return status;
    }
    bf_rt_id_t action_id = this->getActIdFromActFnHdl(pipe_act_fn_hdl);
    this_data->actionIdSet(action_id);
    this_data->setActiveFields(empty);
  }

  if (num_returned) {
    *num_returned = i;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPhase0Table::tableUsageGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

bf_status_t BfRtPhase0Table::dataReset(BfRtTableData *data) const {
  BfRtPhase0TableData *data_obj = static_cast<BfRtPhase0TableData *>(data);
  if (!this->validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  const auto item = action_info_list.begin();
  const auto action_id = item->first;
  return data_obj->reset(action_id);
}

// BfRtSelectorGetMemberTable****************
bf_status_t BfRtSelectorGetMemberTable::getRef(pipe_tbl_hdl_t *hdl,
                                               bf_rt_id_t *sel_tbl_id,
                                               bf_rt_id_t *act_tbl_id) const {
  auto ref_map = this->getTableRefMap();
  auto it = ref_map.find("other");
  if (it == ref_map.end()) {
    LOG_ERROR(
        "Cannot find sel ref. Missing required sel table reference "
        "in bf-rt.json");
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  if (it->second.size() != 1) {
    LOG_ERROR(
        "Invalid bf-rt.json configuration. SelGetMem should be part"
        " of exactly one Selector table.");
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  *hdl = it->second.back().tbl_hdl;
  *sel_tbl_id = it->second.back().id;

  // Get the Sel table
  const BfRtTable *sel_tbl;
  auto status = this->bfRtInfoGet()->bfrtTableFromIdGet(*sel_tbl_id, &sel_tbl);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  auto sel_tbl_obj = static_cast<const BfRtTableObj *>(sel_tbl);
  *act_tbl_id = sel_tbl_obj->getActProfId();
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorGetMemberTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  if (key_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  *key_ret =
      std::unique_ptr<BfRtTableKey>(new BfRtSelectorGetMemberTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorGetMemberTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  if (data_ret == nullptr) {
    return BF_INVALID_ARG;
  }
  std::vector<bf_rt_id_t> fields{};
  *data_ret = std::unique_ptr<BfRtTableData>(
      new BfRtSelectorGetMemberTableData(this, 0, fields));

  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtSelectorGetMemberTable::tableEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  const BfRtSelectorGetMemberTableKey &sel_key =
      static_cast<const BfRtSelectorGetMemberTableKey &>(key);
  BfRtSelectorGetMemberTableData *sel_data =
      static_cast<BfRtSelectorGetMemberTableData *>(data);

  uint64_t grp_id = 0;
  bf_rt_id_t field_id;
  status = this->keyFieldIdGet("$SELECTOR_GROUP_ID", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR fail to get field ID for grp ID",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  status = sel_key.getValue(field_id, &grp_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to grp_id",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  uint64_t hash = 0;
  status = this->keyFieldIdGet("hash_value", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for hash_value",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  status = sel_key.getValue(field_id, &hash);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get hash",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  pipe_tbl_hdl_t sel_tbl_hdl;
  bf_rt_id_t sel_tbl_id;
  bf_rt_id_t act_tbl_id;
  status = this->getRef(&sel_tbl_hdl, &sel_tbl_id, &act_tbl_id);
  if (status) {
    return status;
  }

  pipe_sel_grp_hdl_t grp_hdl;
  auto device_state =
      BfRtDevMgrImpl::bfRtDeviceStateGet(dev_tgt.dev_id, prog_name);
  auto selector_state = device_state->selectorState.getObjState(sel_tbl_id);
  auto act_profile_state = device_state->actProfState.getObjState(act_tbl_id);
  if (selector_state == nullptr || act_profile_state == nullptr) {
    LOG_ERROR("%s:%d %s ERROR State for selector/act profile absent",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_UNEXPECTED;
  }
  if (!selector_state->grpIdExists(dev_tgt.pipe_id, grp_id, &grp_hdl)) {
    LOG_ERROR("%s:%d %s : Error in finding a selector group id %" PRIu64
              " to pipe %x, group id already exists",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    return BF_OBJECT_NOT_FOUND;
  }
  uint32_t num_bytes = sizeof(uint64_t);
  std::vector<uint8_t> hash_arr(num_bytes, 0);
  BfRtEndiannessHandler::toNetworkOrder(num_bytes, hash, hash_arr.data());

  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  status = pipeMgr->pipeMgrSelGrpMbrGetFromHash(session.sessHandleGet(),
                                                dev_tgt.dev_id,
                                                sel_tbl_hdl,
                                                grp_hdl,
                                                hash_arr.data(),
                                                num_bytes,
                                                &adt_ent_hdl);
  if (status) {
    LOG_ERROR(
        "%s:%d %s : Error : Unable to convert (hash,grp_hdl) to adt_mbr_hdl"
        " for hash %" PRIu64 " grp hdl %d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        hash,
        grp_hdl);
    return status;
  }
  uint32_t act_mbr_id = 0;
  status = act_profile_state->getMbrIdFromHndl(adt_ent_hdl, &act_mbr_id);
  if (status) {
    LOG_ERROR("%s:%d %s : Error : Unable to get mbr ID for hdl %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              adt_ent_hdl);
    return status;
  }

  status = this->dataFieldIdGet("$ACTION_MEMBER_ID", &field_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for hash_value",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }
  status = sel_data->setValue(field_id, act_mbr_id);
  return status;
}

// Debug counter

// Caller must do null pointer check.
char **BfRtTblDbgCntTable::allocDataForTableNames() const {
  char **tbl_names = new char *[this->_table_size];
  if (tbl_names != NULL) {
    for (uint32_t i = 0; i < this->_table_size; i++) {
      tbl_names[i] = new char[PIPE_MGR_TBL_NAME_LEN];
      std::memset(tbl_names[i], 0, PIPE_MGR_TBL_NAME_LEN);
    }
  }
  return tbl_names;
}

void BfRtTblDbgCntTable::freeDataForTableNames(char **tbl_names) const {
  for (uint32_t i = 0; i < this->_table_size; i++) {
    delete[] tbl_names[i];
  }
  delete[] tbl_names;
}

bf_status_t BfRtTblDbgCntTable::tableEntryMod(const BfRtSession & /*session*/,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              const BfRtTableData &data) const {
  const BfRtTblDbgCntTableKey &dbg_key =
      static_cast<const BfRtTblDbgCntTableKey &>(key);
  const BfRtTblDbgCntTableData &dbg_data =
      static_cast<const BfRtTblDbgCntTableData &>(data);
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  bf_rt_id_t field_id;
  auto status = this->dataFieldIdGet("type", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  std::string type_str;
  status = dbg_data.getValue(field_id, &type_str);
  if (status) return status;
  auto it = std::find(cntTypeStr.begin(), cntTypeStr.end(), type_str);
  if (it == cntTypeStr.end()) {
    LOG_TRACE("%s:%d %s ERROR invalid value in debug counter table key, err %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    return BF_INVALID_ARG;
  }

  bf_tbl_dbg_counter_type_t type =
      static_cast<bf_tbl_dbg_counter_type_t>(it - cntTypeStr.begin());
  auto *pipeMgr = PipeMgrIntf::getInstance();
  // Convert name to pipe_mgr format
  std::string tbl_name = getPipeMgrTblName(dbg_key.getTblName());
  status =
      pipeMgr->bfDbgCounterSet(tgt, const_cast<char *>(tbl_name.c_str()), type);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting debug counter for table %s pipe %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        dbg_key.getTblName().c_str(),
        dev_tgt.pipe_id,
        status);
    return status;
  }
  return status;
}

bf_status_t BfRtTblDbgCntTable::tableEntryGet(const BfRtSession & /*session*/,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              BfRtTableData *data) const {
  const BfRtTblDbgCntTableKey &dbg_key =
      static_cast<const BfRtTblDbgCntTableKey &>(key);
  BfRtTblDbgCntTableData &dbg_data =
      static_cast<BfRtTblDbgCntTableData &>(*data);
  bf_tbl_dbg_counter_type_t type;
  uint32_t value;
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  // Convert name to pipe_mgr format
  std::string tbl_name = getPipeMgrTblName(dbg_key.getTblName());
  auto status = pipeMgr->bfDbgCounterGet(
      tgt, const_cast<char *>(tbl_name.c_str()), &type, &value);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting debug counter for table %s pipe %d, err %d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        dbg_key.getTblName().c_str(),
        dev_tgt.pipe_id,
        status);
    return status;
  }

  bf_rt_id_t field_id;
  status = this->dataFieldIdGet("type", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_data.setValue(field_id, cntTypeStr.at(type));
  if (status) return status;

  status = this->dataFieldIdGet("value", &field_id);
  if (status) {
    BF_RT_DBGCHK(0);
    return status;
  }
  status = dbg_data.setValue(field_id, static_cast<uint64_t>(value));
  if (status) return status;

  return status;
}

bf_status_t BfRtTblDbgCntTable::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  *num_returned = 0;
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  int num_tbls = 0;
  char **tbl_list = this->allocDataForTableNames();
  if (tbl_list == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  auto status = pipeMgr->bfDbgCounterTableListGet(tgt, tbl_list, &num_tbls);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error while getting debug counter tables %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    this->freeDataForTableNames(tbl_list);
    return status;
  }
  // Translate to vector for easier operations
  std::vector<std::string> tbl_names;
  for (int i = 0; i < num_tbls; i++) {
    tbl_names.push_back(tbl_list[i]);
  }
  this->freeDataForTableNames(tbl_list);

  // Sort and remove duplicates
  tbl_names.erase(std::unique(tbl_names.begin(), tbl_names.end()),
                  tbl_names.end());
  std::sort(tbl_names.begin(), tbl_names.end());
  const BfRtTblDbgCntTableKey &dbg_key =
      static_cast<const BfRtTblDbgCntTableKey &>(key);
  // Convert key to pipe_mgr format
  std::string tbl_name = getPipeMgrTblName(dbg_key.getTblName());
  auto it = std::find(tbl_names.begin(), tbl_names.end(), tbl_name);
  if (it == tbl_names.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  // Move to next element
  it++;

  for (uint32_t i = 0; i < n; i++) {
    auto this_key =
        static_cast<BfRtTblDbgCntTableKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    // If run out of entries, mark remaning data as empty.
    if (it == tbl_names.end()) {
      (*key_data_pairs)[i].second = nullptr;
      continue;
    }

    tbl_name = getQualifiedTableName(dev_tgt.dev_id, this->prog_name, *it);
    this_key->setTblName(tbl_name);
    status = this->tableEntryGet(session, dev_tgt, flags, *this_key, this_data);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    (*num_returned)++;
    it++;
  }

  return BF_SUCCESS;
}

bf_status_t BfRtTblDbgCntTable::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  BfRtTblDbgCntTableKey *dbg_key = static_cast<BfRtTblDbgCntTableKey *>(key);

  auto *pipeMgr = PipeMgrIntf::getInstance();
  int num_tbls = 0;
  char **tbl_list = this->allocDataForTableNames();
  if (tbl_list == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  auto status = pipeMgr->bfDbgCounterTableListGet(tgt, tbl_list, &num_tbls);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error while getting debug counter tables %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    this->freeDataForTableNames(tbl_list);
    return status;
  }
  // Translate to vector for easier operations
  std::vector<std::string> tbl_names;
  for (int i = 0; i < num_tbls; i++) {
    tbl_names.push_back(tbl_list[i]);
  }
  this->freeDataForTableNames(tbl_list);
  // Sort and remove duplicates
  tbl_names.erase(std::unique(tbl_names.begin(), tbl_names.end()),
                  tbl_names.end());
  std::sort(tbl_names.begin(), tbl_names.end());

  std::string new_name =
      getQualifiedTableName(dev_tgt.dev_id, this->prog_name, tbl_names[0]);
  // compare returns 0 on equal
  dbg_key->setTblName(new_name);
  return this->tableEntryGet(session, dev_tgt, flags, *key, data);
}

bf_status_t BfRtTblDbgCntTable::tableClear(const BfRtSession & /*session*/,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t & /*flags*/) const {
  dev_target_t tgt = {0};
  tgt.device_id = dev_tgt.dev_id;
  tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  int num_tbls = 0;
  char **tbl_list = this->allocDataForTableNames();
  if (tbl_list == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  auto status = pipeMgr->bfDbgCounterTableListGet(tgt, tbl_list, &num_tbls);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error while getting debug counter tables %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              status);
    this->freeDataForTableNames(tbl_list);
    return status;
  }

  for (int i = 0; i < num_tbls; ++i) {
    status = pipeMgr->bfDbgCounterClear(tgt, tbl_list[i]);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Error while clearing debug counter for table %s, %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                tbl_list[i],
                status);
      break;
    }
  }
  this->freeDataForTableNames(tbl_list);
  return status;
}

bf_status_t BfRtTblDbgCntTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  *count = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtTblDbgCntTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTblDbgCntTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTblDbgCntTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  return this->dataAllocate(fields, data_ret);
}

bf_status_t BfRtTblDbgCntTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtTblDbgCntTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

// Register param table
// Table reuses debug counter table data object.
bf_status_t BfRtRegisterParamTable::getRef(pipe_tbl_hdl_t *hdl) const {
  auto ref_map = this->getTableRefMap();
  auto it = ref_map.find("other");
  if (it == ref_map.end()) {
    LOG_ERROR(
        "Cannot set register param. Missing required register table reference "
        "in bf-rt.json.");
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  if (it->second.size() != 1) {
    LOG_ERROR(
        "Invalid bf-rt.json configuration. Register param should be part"
        " of exactly one register table.");
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  *hdl = it->second[0].tbl_hdl;
  return BF_SUCCESS;
}

bf_status_t BfRtRegisterParamTable::getParamHdl(bf_dev_id_t dev_id) const {
  pipe_reg_param_hdl_t reg_param_hdl;
  // Get param name from table name. Have to remove pipe name.
  std::string param_name = this->table_name_get();
  param_name = param_name.erase(0, param_name.find(".") + 1);

  auto *pipeMgr = PipeMgrIntf::getInstance();
  auto status =
      pipeMgr->pipeStfulParamGetHdl(dev_id, param_name.c_str(), &reg_param_hdl);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to get register param handle from pipe mgr",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    BF_RT_DBGCHK(0);
    return 0;
  }

  return reg_param_hdl;
}

bf_status_t BfRtRegisterParamTable::tableDefaultEntrySet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableData &data) const {
  const BfRtRegisterParamTableData &mdata =
      static_cast<const BfRtRegisterParamTableData &>(data);
  pipe_tbl_hdl_t tbl_hdl;
  auto status = this->getRef(&tbl_hdl);
  if (status) {
    return status;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeStfulParamSet(session.sessHandleGet(),
                                    pipe_dev_tgt,
                                    tbl_hdl,
                                    this->getParamHdl(dev_tgt.dev_id),
                                    mdata.value);
}

bf_status_t BfRtRegisterParamTable::tableDefaultEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableData *data) const {
  pipe_tbl_hdl_t tbl_hdl;
  auto status = this->getRef(&tbl_hdl);
  if (status) {
    return status;
  }
  BfRtRegisterParamTableData *mdata =
      static_cast<BfRtRegisterParamTableData *>(data);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeStfulParamGet(session.sessHandleGet(),
                                    pipe_dev_tgt,
                                    tbl_hdl,
                                    this->getParamHdl(dev_tgt.dev_id),
                                    &mdata->value);
}

bf_status_t BfRtRegisterParamTable::tableDefaultEntryReset(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  pipe_tbl_hdl_t tbl_hdl;
  auto status = this->getRef(&tbl_hdl);
  if (status) {
    return status;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeStfulParamReset(session.sessHandleGet(),
                                      pipe_dev_tgt,
                                      tbl_hdl,
                                      this->getParamHdl(dev_tgt.dev_id));
}

bf_status_t BfRtRegisterParamTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtRegisterParamTableData(this));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

}  // bfrt

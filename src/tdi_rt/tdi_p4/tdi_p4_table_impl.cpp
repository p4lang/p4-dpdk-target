/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) 2017-2018 Barefoot Networks, Inc.

 * All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of
 * Barefoot Networks, Inc. and its suppliers, if any. The intellectual and
 * technical concepts contained herein are proprietary to Barefoot Networks,
 * Inc.
 * and its suppliers and may be covered by U.S. and Foreign Patents, patents in
 * process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material is
 * strictly forbidden unless prior written permission is obtained from
 * Barefoot Networks, Inc.
 *
 * No warranty, explicit or implicit is provided, unless granted under a
 * written agreement with Barefoot Networks, Inc.
 *
 * $Id: $
 *
 ******************************************************************************/
#include <unordered_set>

// tdi include
#include <tdi/common/tdi_init.hpp>

// local rt includes
//#include <tdi_common/tdi_rt_target.hpp>
//#include <tdi_common/tdi_state.hpp>
//#include <tdi_common/tdi_rt_target.hpp>
#include <tdi_p4/tdi_p4_table_impl.hpp>
#include <tdi_p4/tdi_p4_table_key_impl.hpp>
#include <tdi_p4/tdi_p4_table_data_impl.hpp>
#include <tdi_common/tdi_rt_target.hpp>

namespace tdi {
namespace pna {
namespace rt {
namespace {

// getActionSpec fetches both resources and action_spec from Pipe Mgr.
// If some resources are unsupported it will filter them out.
tdi_status_t getActionSpec(const tdi::Session &session,
                           const dev_target_t &dev_tgt,
                           const tdi::Flags &flags,
                           const pipe_tbl_hdl_t &pipe_tbl_hdl,
                           const pipe_mat_ent_hdl_t &mat_ent_hdl,
                           uint32_t res_get_flags,
                           pipe_tbl_match_spec_t *pipe_match_spec,
                           pipe_action_spec_t *pipe_action_spec,
                           pipe_act_fn_hdl_t *act_fn_hdl,
                           pipe_res_get_data_t *res_data) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_status_t status = TDI_SUCCESS;
  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_RT_FLAGS_FROM_HW),
                 &read_from_hw);
  if (pipe_match_spec) {
    status = pipeMgr->pipeMgrGetEntry(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
        pipe_tbl_hdl,
        dev_tgt.device_id,
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
    status = pipeMgr->pipeMgrTableGetDefaultEntry(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
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

tdi_status_t entryModInternal(const tdi::Table &table,
                              const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const tdi::Flags &flags,
                              const tdi::TableData &data,
                              const pipe_mat_ent_hdl_t &mat_ent_hdl) {
  tdi_status_t status = TDI_SUCCESS;
  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto &match_data = static_cast<const MatchActionTableData &>(data);
//  auto &match_table = static_cast<const MatchActionDirect &>(table);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_id_t action_id = match_data.actionIdGet();

  std::vector<tdi_id_t> dataFields;
  if (match_data.allFieldsSetGet()) {
    dataFields = table.tableInfoGet()->dataFieldIdListGet(action_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  } else {
    dataFields.assign(match_data.activeFieldsGet().begin(),
                      match_data.activeFieldsGet().end());
  }

  pipe_action_spec_t pipe_action_spec = {0};
  match_data.copy_pipe_action_spec(&pipe_action_spec);

  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();

  bool direct_resource_found = false;
  bool action_spec_found = false;
  bool direct_counter_found = false;

  // Pipe-mgr exposes different APIs to modify different parts of the data
  // 1. To modify any part of the action spec, pipe_mgr_mat_ent_set_action is
  // the API to use
  //    As part of this following direct resources can be modified
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

  for (const auto &dataFieldId : dataFields) {
    auto tableDataField =
        table.tableInfoGet()->dataFieldGet(dataFieldId, action_id);
    if (!tableDataField) {
      return TDI_OBJECT_NOT_FOUND;
    }
    auto fieldTypes = static_cast<const RtDataFieldContextInfo *>(
                          tableDataField->dataFieldContextInfoGet())
                          ->typesGet();
    fieldDestination field_destination =
        RtDataFieldContextInfo::getDataFieldDestination(fieldTypes);
    switch (field_destination) {
      case fieldDestination::DIRECT_METER:
      case fieldDestination::DIRECT_REGISTER:
        direct_resource_found = true;
        break;

      case fieldDestination::ACTION_SPEC:
        action_spec_found = true;
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
  auto table_context_info =
      static_cast<const MatchActionDirectTableContextInfo *>(
          table.tableInfoGet()->tableContextInfoGet());

  if (action_spec_found) {
    auto table_type = static_cast<tdi_rt_table_type_e>(
        table.tableInfoGet()->tableTypeGet());
    if (table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT ||
        table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR) {
      // If we are modifying the action spec for a match action indirect or
      // match action selector table, we need to verify the member ID or the
      // selector group id referenced here is legit.

#if 0
      const MatchActionIndirectTableData &match_indir_data =
          static_cast<const MatchActionIndirectTableData &>(match_data);

      const MatchActionIndirectTable &mat_indir_table =
          static_cast<const MatchActionIndirectTable &>(table);
      pipe_mgr_adt_ent_data_t ap_ent_data;

      pipe_adt_ent_hdl_t adt_ent_hdl = 0;
      pipe_sel_grp_hdl_t sel_grp_hdl = 0;
      status = mat_indir_table.getActionState(session,
                                              dev_tgt,
                                              &match_indir_data,
                                              &adt_ent_hdl,
                                              &sel_grp_hdl,
                                              &act_fn_hdl,
                                              &ap_ent_data);

      if (status != TDI_SUCCESS) {
        if (match_indir_data.isGroup()) {
          if (sel_grp_hdl == MatchActionIndirectTableData::invalid_group) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a group id "
                "%d which does not exist in the group table",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                match_indir_data.getGroupId());
            return TDI_OBJECT_NOT_FOUND;
          } else if (adt_ent_hdl == MatchActionIndirectTableData::
                                        invalid_action_entry_hdl) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a group id "
                "%d which does not have any members in the group table "
                "associated with the table",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                match_indir_data.getGroupId());
            return TDI_OBJECT_NOT_FOUND;
          }
        } else {
          if (adt_ent_hdl ==
              MatchActionIndirectTableData::invalid_action_entry_hdl) {
            LOG_TRACE(
                "%s:%d %s: Cannot modify match entry referring to a action "
                "member id %d which does not exist in the action profile table",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                match_indir_data.getActionMbrId());
            return TDI_OBJECT_NOT_FOUND;
          }
        }
      }
      if (match_indir_data.isGroup()) {
        pipe_action_spec.sel_grp_hdl = sel_grp_hdl;
      } else {
        pipe_action_spec.adt_ent_hdl = adt_ent_hdl;
      }
#endif
    }
    status = pipeMgr->pipeMgrMatEntSetAction(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        mat_ent_hdl,
        act_fn_hdl,
        &pipe_action_spec,
        0 /* Pipe API flags */);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in modifying table data err %d",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  } else if (direct_resource_found) {
    status = pipeMgr->pipeMgrMatEntSetResource(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        mat_ent_hdl,
        pipe_action_spec.resources,
        pipe_action_spec.resource_count,
        0 /* Pipe API flags */);

    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR in modifying resources part of table data, err %d",
          __func__,
          __LINE__,
          table.tableInfoGet()->nameGet().c_str(),
          status);
      return status;
    }
  }

  if (direct_counter_found) {
    const pipe_stat_data_t *stat_data = match_data.getPipeActionSpecObj()
                                            .getCounterSpecObj()
                                            .getPipeCounterSpec();
    status = pipeMgr->pipeMgrMatEntDirectStatSet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        mat_ent_hdl,
        const_cast<pipe_stat_data_t *>(stat_data));
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in modifying counter, err %d",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  }

  return TDI_SUCCESS;
}

template <typename T>
tdi_status_t getTableUsage(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           const tdi::Flags &flags,
                           const T &table,
                           uint32_t *count) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      table.tableInfoGet()->tableContextInfoGet());
  bool read_from_hw = false;
  flags.getValue(static_cast<tdi_flags_e>(TDI_RT_FLAGS_FROM_HW),
                 &read_from_hw);

  tdi_status_t status = pipeMgr->pipeMgrGetEntryCount(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      read_from_hw,
      count);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting to usage for table, err %d (%s)",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              status,
              bf_err_str(status));
  }
  return status;
}

template <typename T>
tdi_status_t getReservedEntries(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const T &table,
                                size_t *size) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      table.tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrGetReservedEntryCount(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      size);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting reserved entries count for table, err %d "
        "(%s)",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        status,
        bf_err_str(status));
  }
  return status;
}

// Template function for getFirst for Indirect meters, LPF, WRED and register
// tables
template <typename Table, typename Key>
tdi_status_t getFirst_for_resource_tbls(const Table &table,
                                        const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags &flags,
                                        Key *key,
                                        tdi::TableData *data) {
  tdi_id_t table_id_from_data;
  const Table *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != table.tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  // First entry in a index based table is idx 0
  key->setIdxKey(0);

  return table.entryGet(session, dev_tgt, flags, *key, data);
}

// Template function for getNext_n for Indirect meters, LPF, WRED and register
// tables
template <typename Table, typename Key>
tdi_status_t getNext_n_for_resource_tbls(
    const Table &table,
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const Key &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) {
  tdi_status_t status = TDI_SUCCESS;
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

    tdi_id_t table_id_from_data;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != table.tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table.tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }

    tdi_id_t table_id_from_key;
    const Table *table_from_key;
    this_key->tableGet(&table_from_key);
    table_from_key->tableIdGet(&table_id_from_key);

    if (table_id_from_key != table.tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table key object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          table.tableInfoGet()->nameGet().c_str(),
          table_id_from_key);
      return TDI_INVALID_ARG;
    }

    status = table.entryGet(session, dev_tgt, flags, *this_key, this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting counter index %d, err %d",
                __func__,
                __LINE__,
                table.tableInfoGet()->nameGet().c_str(),
                i,
                status);
      // Make the data object null if error
      (*key_data_pairs)[j].second = nullptr;
    }

    (*num_returned)++;
  }
  return TDI_SUCCESS;
}

// This function checks if the key idx (applicable for Action profile, selector,
// Indirect meter, Counter, LPF, WRED, Register tables) is within the bounds of
// the size of the table

#if 0
bool verify_key_for_idx_tbls(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Table &table,
                             uint32_t idx) {
  size_t table_size;
  tdi::Flags flags(0);
  table.sizeGet(session, dev_tgt, flags, &table_size);
  if (idx < table_size) {
    return true;
  }
  LOG_ERROR("%s:%d %s : ERROR Idx %d for key exceeds the size of the table %zd",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            idx,
            table_size);
  return false;
}
#endif

template <class Table, class Key>
tdi_status_t key_reset(const Table & /*table*/, Key *match_key) {
// TODO(sayanb)
#if 0
  if (!table.validateTable_from_keyObj(*match_key)) {
    LOG_TRACE("%s:%d %s ERROR : Key object is not associated with the table",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
#endif
  return match_key->reset();
}

tdi_status_t tableClearMatCommon(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const bool &&reset_default_entry,
                                 const tdi::Table *table) {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      table->tableInfoGet()->tableContextInfoGet());

  // Clear the table
  tdi_status_t status = pipeMgr->pipeMgrMatTblClear(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      0 /* pipe api flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Failed to clear table %s, err %d",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  if (reset_default_entry) {
    tdi::Flags flags(0);
    status = table->defaultEntryReset(session, dev_tgt, flags);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in resetting default entry , err %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
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
tdi_status_t resourceCheckAndInitialize(const tdi::Table &tbl,
                                        const T &tbl_data,
                                        const bool is_default) {
  // Get the pipe action spec from the data
  T &data = const_cast<T &>(tbl_data);
  // auto &match_table = static_cast<const MatchActionDirect &>(tbl);
  auto table_context_info =
      static_cast<const MatchActionDirectTableContextInfo *>(
          tbl.tableInfoGet()->tableContextInfoGet());
  PipeActionSpec &pipe_action_spec_obj = data.getPipeActionSpecObj();
  pipe_action_spec_t *pipe_action_spec =
      pipe_action_spec_obj.getPipeActionSpec();
  bool meter = false, reg = false, stat = false;
  const auto &action_id = data.actionIdGet();
  table_context_info->actionResourcesGet(action_id, &meter, &reg, &stat);

  // We can get the indirect count from the data object directly by
  // counting the number of resource index field types set
  // However, for direct count, that is not possible since one
  // resource can have many different fields. We count direct
  // resources set by going through table_ref_map.
  // const auto &actual_indirect_count = tbl_data.indirectResCountGet();
  uint32_t actual_direct_count = 0;

  // Get a map of all the resources(direct and indirect) for this table
  auto &table_ref_map = table_context_info->tableRefMapGet();
  if (table_ref_map.size() == 0) {
    // Nothing to be done. Just return
    return TDI_SUCCESS;
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
  //      tbl.tableInfoGet()->nameGet().c_str());
  //  return TDI_INVALID_ARG;
  //}
  if (programmed_direct_count == actual_direct_count) {
    // If the direct resource count is equal to the resource count in the
    // formed pipe action spec, we don't need to do anything. as it means that
    // the user has explicitly set the values of all the resources attached
    // to this table already
    return TDI_SUCCESS;
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
        tbl.tableInfoGet()->nameGet().c_str(),
        programmed_direct_count,
        actual_direct_count);
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
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
            tbl.tableInfoGet()->nameGet().c_str(),
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
  return TDI_SUCCESS;
}

bool checkDefaultOnly(const Table *table, const tdi::TableData &data) {
  const auto &action_id = data.actionIdGet();

  auto &annotations =
      table->tableInfoGet()->actionGet(action_id)->annotationsGet();

  auto def_an = Annotation("@defaultonly", "");
  if (annotations.find(def_an) != annotations.end()) {
    return true;
  }
  return false;
}

bool checkTableOnly(const Table *table, const tdi_id_t &action_id) {
  if (!action_id) return false;

  auto &annotations =
      table->tableInfoGet()->actionGet(action_id)->annotationsGet();
  auto def_an = Annotation("@tableonly", "");
  if (annotations.find(def_an) != annotations.end()) {
    return true;
  }
  return false;
}

#if 0
{

// used by dynamic key mask attribute, from key field mask to match spec
tdi_status_t fieldMasksToMatchSpec(
    const std::unordered_map<tdi_id_t, std::vector<uint8_t>> &field_mask,
    pipe_tbl_match_spec_t *mat_spec,
    const tdi::Table *table) {
  for (const auto &field : field_mask) {
    const tdi::KeyFieldInfo *key_field;
    auto status = table->getKeyField(field.first, &key_field);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR Fail to get key field, field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field.first);
      return status;
    }
    size_t sz;
    status = table->keyFieldSizeGet(field.first, &sz);
    if ((status != TDI_SUCCESS) || (((sz + 7) / 8) != field.second.size())) {
      LOG_TRACE("%s:%d %s ERROR Invalid key field mask size, field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field.first);
      return status;
    }
    std::memcpy(mat_spec->match_mask_bits + key_field->getOffset(),
                field.second.data(),
                field.second.size());
  }
  mat_spec->num_match_bytes = table->getKeySize().bytes;
  mat_spec->num_valid_match_bits = table->getKeySize().bits;
  return TDI_SUCCESS;
}
// used by dynamic key mask attribute, from match spec to key field mask
tdi_status_t matchSpecToFieldMasks(
    const pipe_tbl_match_spec_t &mat_spec,
    std::unordered_map<tdi_id_t, std::vector<uint8_t>> *field_mask,
    const tdi::Table *table) {
  if (mat_spec.match_mask_bits == nullptr) return TDI_INVALID_ARG;
  std::vector<tdi_id_t> id_vec;
  table->keyFieldIdListGet(&id_vec);
  int cnt = 0;
  for (auto field_id : id_vec) {
    const tdi::KeyFieldInfo *key_field;
    auto status = table->getKeyField(field_id, &key_field);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR Fail to get key field, field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id);
      return status;
    }
    cnt = key_field->getOffset();
    size_t sz;
    status = table->keyFieldSizeGet(field_id, &sz);
    sz = (sz + 7) / 8;
    std::vector<uint8_t> mask(sz);
    for (uint32_t i = 0; i < sz; i++) {
      mask[i] = (mat_spec.match_mask_bits[cnt + i]);
    }
    if (mask.empty()) {
      LOG_TRACE("%s:%d %s Invalid mask for field id %d",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id);
      return TDI_INVALID_ARG;
    }
    if (field_mask->find(field_id) != field_mask->end()) {
      LOG_WARN(
          "%s:%d %s Field id %d has been configured duplicatedly, use the "
          "latest configuration",
          __func__,
          __LINE__,
          table->tableInfoGet()->nameGet().c_str(),
          field_id);
      field_mask->at(field_id) = mask;
    } else {
      field_mask->emplace(std::make_pair(field_id, mask));
    }
  }
  return TDI_SUCCESS;
}

// Function used to convert pipe_mgr format (which does not use "." and uses "_"
// instead) to naming used in tdi.json. Function will return input string if
// related table is not found.
const std::string getQualifiedTableName(const tdi_dev_id_t &dev_id,
                                        const std::string &p4_name,
                                        const std::string &tbl_name) {
  const Info *info;
  std::vector<const Table *> tables;
  PipelineProfInfoVec pipe_info;
  DevMgr::getInstance().tdiInfoGet(dev_id, p4_name, &info);
  info->tdiInfoPipelineInfoGet(&pipe_info);
  info->tdiInfoGetTables(&tables);
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
}

#endif

}  // anonymous namespace

// MatchActionTable ******************

tdi_status_t MatchActionDirect::entryAdd(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags & /*flags*/,
                                         const tdi::TableKey &key,
                                         const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  const MatchActionTableData &match_data =
      static_cast<const MatchActionTableData &>(data);

  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // Initialize the direct resources if they were not provided by the caller.
  auto status = resourceCheckAndInitialize<MatchActionTableData>(
      *this, match_data, false);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to initialize Direct resources",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  pipe_tbl_match_spec_t pipe_match_spec = {0};
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  if (checkDefaultOnly(this, data)) {
    LOG_TRACE("%s:%d %s Error adding action because it is defaultOnly",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  match_key.populate_match_spec(&pipe_match_spec);
  pipe_action_spec = match_data.get_pipe_action_spec();
  pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();
  pipe_mat_ent_hdl_t pipe_entry_hdl = 0;
  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  return pipeMgr->pipeMgrMatEntAdd(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      act_fn_hdl,
      pipe_action_spec,
      0 /* ttl */,
      0 /* Pipe API flags */,
      &pipe_entry_hdl);
}

tdi_status_t MatchActionDirect::entryMod(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         const tdi::TableKey &key,
                                         const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  pipe_mat_ent_hdl_t pipe_entry_hdl;
  tdi_status_t status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      &pipe_entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  return entryModInternal(*this, session, dev_tgt, flags, data, pipe_entry_hdl);
}

tdi_status_t MatchActionDirect::entryDel(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags & /*flags*/,
                                         const tdi::TableKey &key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrMatEntDelByMatchSpec(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      0 /* Pipe api flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Entry delete failed for table %s, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t MatchActionDirect::clear(const tdi::Session &session,
                                      const tdi::Target &dev_tgt,
                                      const tdi::Flags & /*flags*/) const {
  if (this->tableInfoGet()->isConst()) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return tableClearMatCommon(session, dev_tgt, true, this);
}

tdi_status_t MatchActionDirect::defaultEntrySet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionTableData &match_data =
      static_cast<const MatchActionTableData &>(data);
  const pipe_action_spec_t *pipe_action_spec = nullptr;

  const auto &action_id = match_data.actionIdGet();
  if (checkTableOnly(this, action_id)) {
    LOG_TRACE("%s:%d %s Error adding action because it is tableOnly",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Check which direct resources were provided.
  bool direct_reg = false, direct_cntr = false, direct_mtr = false;
  if (action_id) {
    auto table_context_info =
        static_cast<const MatchActionDirectTableContextInfo *>(
            this->tableInfoGet()->tableContextInfoGet());
    table_context_info->actionResourcesGet(
        action_id, &direct_mtr, &direct_reg, &direct_cntr);
  } else {
    std::vector<tdi_id_t> dataFields;
    if (match_data.allFieldsSetGet()) {
      dataFields = this->tableInfoGet()->dataFieldIdListGet();
      if (status != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                  __func__,
                  __LINE__,
                  this->tableInfoGet()->nameGet().c_str(),
                  status);
        return status;
      }
    } else {
      dataFields.assign(match_data.activeFieldsGet().begin(),
                        match_data.activeFieldsGet().end());
    }
    for (const auto &dataFieldId : dataFields) {
      const tdi::DataFieldInfo *tableDataField =
          this->tableInfoGet()->dataFieldGet(dataFieldId);
      if (!tableDataField) {
        return TDI_OBJECT_NOT_FOUND;
      }
      auto fieldTypes = static_cast<const RtDataFieldContextInfo *>(
                            tableDataField->dataFieldContextInfoGet())
                            ->typesGet();
      auto dest =
          RtDataFieldContextInfo::getDataFieldDestination(fieldTypes);
      switch (dest) {
        case fieldDestination::DIRECT_REGISTER:
          direct_reg = true;
          break;
        case fieldDestination::DIRECT_COUNTER:
          direct_cntr = true;
          break;
        case fieldDestination::DIRECT_METER:
          direct_mtr = true;
          break;
        default:
          break;
      }
    }
  }

  pipe_mat_ent_hdl_t entry_hdl = 0;
  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  if (action_id) {
    // Initialize the direct resources if they were not provided by the
    // caller.
    status = resourceCheckAndInitialize<MatchActionTableData>(
        *this, match_data, true);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR Failed to initialize direct resources",
                __func__,
                __LINE__,
                this->tableInfoGet()->nameGet().c_str());
      return status;
    }

    pipe_action_spec = match_data.get_pipe_action_spec();
    pipe_act_fn_hdl_t act_fn_hdl = match_data.getActFnHdl();

    status = pipeMgr->pipeMgrMatDefaultEntrySet(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        table_context_info->tableHdlGet(),
        act_fn_hdl,
        pipe_action_spec,
        0 /* Pipe API flags */,
        &entry_hdl);
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error setting default entry, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  } else {
    // Get the handle of the default entry.
    status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt,
        table_context_info->tableHdlGet(),
        &entry_hdl);
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error getting entry handle, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->tableInfoGet()->nameGet().c_str(),
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
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        entry_hdl,
        const_cast<pipe_stat_data_t *>(stat_data));
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error setting direct stats, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  }
  // Program direct meter/wred/lpf and registers only if we did not do a set
  // above.
  if (!action_id && (direct_reg || direct_mtr)) {
    pipe_action_spec = match_data.get_pipe_action_spec();
    status = pipeMgr->pipeMgrMatEntSetResource(
        session.handleGet(
            static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
        pipe_dev_tgt.device_id,
        table_context_info->tableHdlGet(),
        entry_hdl,
        const_cast<pipe_res_spec_t *>(pipe_action_spec->resources),
        pipe_action_spec->resource_count,
        0 /* Pipe API flags */);
    if (TDI_SUCCESS != status) {
      LOG_TRACE("%s:%d Dev %d pipe %x %s error setting direct resources, %d",
                __func__,
                __LINE__,
                pipe_dev_tgt.device_id,
                pipe_dev_tgt.dev_pipe_id,
                this->tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
  }
  return status;
}

tdi_status_t MatchActionDirect::defaultEntryReset(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  return pipeMgr->pipeMgrMatTblDefaultEntryReset(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      0);
}

tdi_status_t MatchActionDirect::entryGet(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         const tdi::TableKey &key,
                                         tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  tdi_status_t status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      &pipe_entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  return entryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, &pipe_match_spec, data);
}

tdi_status_t MatchActionDirect::defaultEntryGet(const tdi::Session &session,
                                                const tdi::Target &dev_tgt,
                                                const tdi::Flags &flags,
                                                tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_entry_hdl);
  if (TDI_SUCCESS != status) {
    LOG_TRACE("%s:%d %s Dev %d pipe %x error %d getting default entry",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id,
              pipe_dev_tgt.dev_pipe_id,
              status);
    return status;
  }
  return entryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, nullptr, data);
}

tdi_status_t MatchActionDirect::entryKeyGet(const tdi::Session &session,
                                            const tdi::Target &dev_tgt,
                                            const tdi::Flags & /*flags*/,
                                            const tdi_handle_t &entry_handle,
                                            tdi::Target *entry_tgt,
                                            tdi::TableKey *key) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());
  pipe_tbl_match_spec_t *pipe_match_spec;
  bf_dev_pipe_t entry_pipe;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_status_t status = pipeMgr->pipeMgrEntHdlToMatchSpec(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      entry_handle,
      &entry_pipe,
      const_cast<const pipe_tbl_match_spec_t **>(&pipe_match_spec));
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  entry_tgt->setValue(static_cast<tdi_target_e>(PNA_TARGET_PIPE_ID),
                      entry_pipe);
  match_key->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  return status;
}

tdi_status_t MatchActionDirect::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  tdi_status_t status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      entry_handle);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t MatchActionDirect::entryGet(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         const tdi_handle_t &entry_handle,
                                         tdi::TableKey *key,
                                         tdi::TableData *data) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key->populate_match_spec(&pipe_match_spec);

  auto status = entryGet_internal(
      session, dev_tgt, flags, entry_handle, &pipe_match_spec, data);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

tdi_status_t MatchActionDirect::entryGetFirst(const tdi::Session &session,
                                              const tdi::Target &dev_tgt,
                                              const tdi::Flags &flags,
                                              tdi::TableKey *key,
                                              tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);

  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      this->tableInfoGet()->tableContextInfoGet());

  // Get the first entry handle present in pipe-mgr
  int first_entry_handle;
  tdi_status_t status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      table_context_info->tableHdlGet(),
      pipe_dev_tgt,
      reinterpret_cast<uint32_t *>(&first_entry_handle));

  if (status == TDI_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }

  pipe_tbl_match_spec_t pipe_match_spec;
  std::memset(&pipe_match_spec, 0, sizeof(pipe_tbl_match_spec_t));

  match_key->populate_match_spec(&pipe_match_spec);

  status = entryGet_internal(
      session, dev_tgt, flags, first_entry_handle, &pipe_match_spec, data);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting first entry, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  // Store ref point for GetNext_n to follow.
  const tdi::Device *device;
  status = DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id,
              status);
    return TDI_OBJECT_NOT_FOUND;
  }

  return status;
}

tdi_status_t MatchActionDirect::entryGetNextN(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  const MatchActionKey &match_key = static_cast<const MatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const RtTableContextInfo *>(
      tableInfoGet()->tableContextInfoGet());

  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  tdi_status_t status = pipeMgr->pipeMgrMatchSpecToEntHdl(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      table_context_info->tableHdlGet(),
      &pipe_match_spec,
      &pipe_entry_hdl);
  // If key is not found and this is subsequent call, API should continue
  // from previous call.
  if (status == TDI_OBJECT_NOT_FOUND) {
    // Warn the user that currently used key no longer exist.
    LOG_WARN("%s:%d %s Provided key does not exist, trying previous handle",
             __func__,
             __LINE__,
             tableInfoGet()->nameGet().c_str());

    const tdi::Device *device;
    status = DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                pipe_dev_tgt.device_id,
                status);
      return TDI_OBJECT_NOT_FOUND;
    }

  }

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  std::vector<int> next_entry_handles(n, 0);

  status = pipeMgr->pipeMgrGetNextEntryHandles(
      session.handleGet(
          static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      table_context_info->tableHdlGet(),
      pipe_dev_tgt,
      pipe_entry_hdl,
      n,
      reinterpret_cast<uint32_t *>(next_entry_handles.data()));
  if (status == TDI_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return TDI_SUCCESS;
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting next entry handles from pipe-mgr, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        status);
    return status;
  }
  pipe_tbl_match_spec_t match_spec;
  unsigned i = 0;
  for (i = 0; i < n; i++) {
    std::memset(&match_spec, 0, sizeof(pipe_tbl_match_spec_t));
    auto this_key = static_cast<MatchActionKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }
    if (next_entry_handles[i] == -1) {
      break;
    }
    this_key->populate_match_spec(&match_spec);
    status = entryGet_internal(
        session, dev_tgt, flags, next_entry_handles[i], &match_spec, this_data);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    this_key->setPriority(match_spec.priority);
    this_key->setPartitionIndex(match_spec.partition_index);
  }
  const tdi::Device *device;
  status = DevMgr::getInstance().deviceGet(pipe_dev_tgt.device_id, &device);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting Device for ID %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              pipe_dev_tgt.device_id,
              status);
    return TDI_OBJECT_NOT_FOUND;
  }

  if (num_returned) {
    *num_returned = i;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionDirect::sizeGet(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const tdi::Flags & /*flags*/,
                                        size_t *count) const {
  tdi_status_t status = TDI_SUCCESS;
  size_t reserved = 0;
  status = getReservedEntries(session, dev_tgt, *(this), &reserved);
  *count = this->tableInfoGet()->sizeGet() - reserved;
  return status;
}

tdi_status_t MatchActionDirect::usageGet(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const tdi::Flags &flags,
                                         uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

tdi_status_t MatchActionDirect::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new MatchActionKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionDirect::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  return dataAllocate_internal(0, data_ret, fields);
}

tdi_status_t MatchActionDirect::dataAllocate(
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  // Create a empty vector to indicate all fields are needed
  std::vector<tdi_id_t> fields;
  return dataAllocate_internal(action_id, data_ret, fields);
}
tdi_status_t MatchActionDirect::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  return dataAllocate_internal(0, data_ret, fields);
}

tdi_status_t MatchActionDirect::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  return dataAllocate_internal(action_id, data_ret, fields);
}

tdi_status_t dataReset_internal(const tdi::Table & /*table*/,
                                const tdi_id_t & /*action_id*/,
                                const std::vector<tdi_id_t> & /*fields*/,
                                tdi::TableData * /*data*/) {
#if 0
  tdi::TableData *data_obj = static_cast<tdi::TableData *>(data);
  if (!table.validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return data_obj->reset(action_id, fields);
#endif
  // TODO(sayanb)
  return TDI_SUCCESS;
}

tdi_status_t MatchActionDirect::keyReset(TableKey *key) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  return key_reset<MatchActionDirect, MatchActionKey>(*this, match_key);
}

tdi_status_t MatchActionDirect::dataReset(tdi::TableData *data) const {
  std::vector<tdi_id_t> fields;
  return dataReset_internal(*this, 0, fields, data);
}

tdi_status_t MatchActionDirect::dataReset(const tdi_id_t &action_id,
                                          tdi::TableData *data) const {
  std::vector<tdi_id_t> fields;
  return dataReset_internal(*this, action_id, fields, data);
}

tdi_status_t MatchActionDirect::dataReset(const std::vector<tdi_id_t> &fields,
                                          tdi::TableData *data) const {
  return dataReset_internal(*this, 0, fields, data);
}

tdi_status_t MatchActionDirect::dataReset(const std::vector<tdi_id_t> &fields,
                                          const tdi_id_t &action_id,
                                          tdi::TableData *data) const {
  return dataReset_internal(*this, action_id, fields, data);
}

tdi_status_t MatchActionDirect::attributeAllocate(
    const tdi_attributes_type_e &type,
    std::unique_ptr<TableAttributes> * /*attr*/) const {
  auto &attribute_type_set = tableInfoGet()->attributesSupported();
  if (attribute_type_set.find(type) == attribute_type_set.end()) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  // TODO(sayanb)
#if 0
  *attr = std::unique_ptr<TableAttributes>(
      new TableAttributesImpl(this, type));
#endif
  return TDI_SUCCESS;
}

#if 0
tdi_status_t MatchActionDirect::attributeAllocate(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<TableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != TDI_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  if (type != TableAttributesType::IDLE_TABLE_RUNTIME) {
    LOG_TRACE("%s:%d %s Idle Table Mode cannot be set for Attribute %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_INVALID_ARG;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new TableAttributesImpl(this, type, idle_table_mode));
  return TDI_SUCCESS;
}

tdi_status_t MatchActionDirect::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<TableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(type));
      return TDI_INVALID_ARG;
    case TableAttributesType::ENTRY_SCOPE:
    case TableAttributesType::DYNAMIC_KEY_MASK:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      break;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                static_cast<int>(type));
      return TDI_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

tdi_status_t MatchActionDirect::attributeReset(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<TableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      break;
    case TableAttributesType::ENTRY_SCOPE:
    case TableAttributesType::DYNAMIC_KEY_MASK:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(type));
      return TDI_INVALID_ARG;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                static_cast<int>(type));
      return TDI_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type, idle_table_mode);
}

tdi_status_t MatchActionDirect::tableAttributesSet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const TableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const TableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != TDI_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(attr_type));
    return TDI_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      TableEntryScope entry_scope;
      TableEntryScopeArgumentsImpl scope_args(0);
      tdi_status_t sts = tbl_attr_impl->entryScopeParamsGet(
          &entry_scope,
          static_cast<TableEntryScopeArguments *>(&scope_args));
      if (sts != TDI_SUCCESS) {
        return sts;
      }
      // Call the pipe mgr API to set entry scope
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      // Derive pipe mgr entry scope from TDI entry scope and set it to
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
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      sts = pipeMgr->pipeMgrTblSetProperty(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           prop_type,
                                           prop_val,
                                           args_val);
      return sts;
    }
    case TableAttributesType::DYNAMIC_KEY_MASK: {
      std::unordered_map<tdi_id_t, std::vector<uint8_t>> field_mask;
      tdi_status_t sts = tbl_attr_impl->dynKeyMaskGet(&field_mask);
      pipe_tbl_match_spec_t match_spec;
      const int sz = this->getKeySize().bytes;
      std::vector<uint8_t> match_mask_bits(sz, 0);
      match_spec.match_mask_bits = match_mask_bits.data();
      // translate from map to match_spec
      sts = fieldMasksToMatchSpec(field_mask, &match_spec, this);
      if (sts != TDI_SUCCESS) return sts;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      sts = pipeMgr->pipeMgrMatchKeyMaskSpecSet(
          session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, pipe_tbl_hdl, &match_spec);
      return sts;
    }
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      tdi_status_t sts = tbl_attr_impl->meterByteCountAdjGet(&byte_count);
      if (sts != TDI_SUCCESS) {
        return sts;
      }
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // Just pass in any Meter related field type to get meter_hdl.
      pipe_tbl_hdl_t res_hdl =
          getResourceHdl(DataFieldType::METER_SPEC_CIR_PPS);
      return pipeMgr->pipeMgrMeterByteCountSet(
          session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, res_hdl, byte_count);
    }
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      return setIdleTable(const_cast<MatchActionTable &>(*this),
                          session,
                          dev_tgt,
                          *tbl_attr_impl);
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionDirect::tableAttributesGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    TableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<TableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != TDI_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(attr_type));
    return TDI_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      auto sts = pipeMgr->pipeMgrTblGetProperty(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                                dev_tgt.dev_id,
                                                pipe_tbl_hdl,
                                                prop_type,
                                                &prop_val,
                                                &args_val);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get entry scope from pipe_mgr",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return sts;
      }

      TableEntryScope entry_scope;
      TableEntryScopeArgumentsImpl scope_args(args_val.value);

      // Derive TDI entry scope from pipe mgr entry scope
      entry_scope = prop_val.value == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
                        ? TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
                        : prop_val.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                              ? TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                              : TableEntryScope::ENTRY_SCOPE_USER_DEFINED;

      return tbl_attr_impl->entryScopeParamsSet(
          entry_scope, static_cast<TableEntryScopeArguments &>(scope_args));
    }
    case TableAttributesType::DYNAMIC_KEY_MASK: {
      pipe_tbl_match_spec_t mat_spec;
      const int sz = this->getKeySize().bytes;
      std::vector<uint8_t> match_mask_bits(sz, 0);
      mat_spec.match_mask_bits = match_mask_bits.data();
      mat_spec.num_match_bytes = sz;
      mat_spec.num_valid_match_bits = this->getKeySize().bits;

      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      pipeMgr->pipeMgrMatchKeyMaskSpecGet(
          session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, pipe_tbl_hdl, &mat_spec);
      std::unordered_map<tdi_id_t, std::vector<uint8_t>> field_mask;
      matchSpecToFieldMasks(mat_spec, &field_mask, this);
      return tbl_attr_impl->dynKeyMaskSet(field_mask);
    }
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      return getIdleTable(*this, session, dev_tgt, tbl_attr_impl);
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // Just pass in any Meter related field type to get meter_hdl.
      pipe_tbl_hdl_t res_hdl =
          getResourceHdl(DataFieldType::METER_SPEC_CIR_PPS);
      auto sts = pipeMgr->pipeMgrMeterByteCountGet(
          session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, res_hdl, &byte_count);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get meter bytecount adjust from pipe_mgr",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
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
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionDirect::registerMatUpdateCb(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const pipe_mat_update_cb &cb,
    const void *cookie) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeRegisterMatUpdateCb(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                          dev_tgt.dev_id,
                                          this->tablePipeHandleGet(),
                                          cb,
                                          const_cast<void *>(cookie));
}
#endif

tdi_status_t MatchActionDirect::dataAllocate_internal(
    tdi_id_t action_id,
    std::unique_ptr<tdi::TableData> *data_ret,
    const std::vector<tdi_id_t> &fields) const {
  if (action_id && tableInfoGet()->actionGet(action_id) == nullptr) {
    LOG_TRACE("%s:%d Action_ID %d not found", __func__, __LINE__, action_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *data_ret = std::unique_ptr<tdi::TableData>(
      new MatchActionTableData(this, action_id, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionDirect::entryGet_internal(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const pipe_mat_ent_hdl_t &pipe_entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  dev_target_t pipe_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&pipe_dev_tgt, nullptr);

  auto table_context_info = static_cast<const MatchActionTableContextInfo *>(
      tableInfoGet()->tableContextInfoGet());

  tdi_id_t req_action_id = 0;

  uint32_t res_get_flags = 0;
  pipe_res_get_data_t res_data;
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
  pipe_action_spec_t *pipe_action_spec = nullptr;
  bool all_fields_set = false;

  MatchActionTableData *match_data = static_cast<MatchActionTableData *>(data);
  std::vector<tdi_id_t> dataFields;
  // const auto &req_action_ids = match_data->actionIdGet();

  all_fields_set = match_data->allFieldsSetGet();
  if (all_fields_set) {
    res_get_flags = PIPE_RES_GET_FLAG_ALL;
    // do not assign dataFields in this case because we might not know
    // what the actual_action_id is yet
    // Based upon the actual action id, we will be filling in the data fields
    // later anyway
  } else {
    dataFields.assign(match_data->activeFieldsGet().begin(),
                      match_data->activeFieldsGet().end());
    for (const auto &dataFieldId : dataFields) {
      const tdi::DataFieldInfo *tableDataField =
          this->tableInfoGet()->dataFieldGet(dataFieldId);
      if (!tableDataField) {
        return TDI_OBJECT_NOT_FOUND;
      }
      auto fieldTypes = static_cast<const RtDataFieldContextInfo *>(
                            tableDataField->dataFieldContextInfoGet())
                            ->typesGet();
      auto dest =
          RtDataFieldContextInfo::getDataFieldDestination(fieldTypes);
      switch (dest) {
        case fieldDestination::DIRECT_METER:
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
                         table_context_info->tableHdlGet(),
                         pipe_entry_hdl,
                         res_get_flags,
                         pipe_match_spec,
                         pipe_action_spec,
                         &pipe_act_fn_hdl,
                         &res_data);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handl %d, "
        "err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        pipe_entry_hdl,
        status);
    return status;
  }

  auto &action_id = table_context_info->actFnHdlToIdGet().at(pipe_act_fn_hdl);
  if (req_action_id && req_action_id != action_id) {
    // Keeping this log as warning for iteration purposes.
    // Caller can decide to throw an error if required
    LOG_TRACE("%s:%d %s ERROR expecting action ID to be %d but recvd %d ",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              req_action_id,
              action_id);
    return TDI_INVALID_ARG;
  }

  // TODO(sayanb)
  // match_data->actionIdSet(action_id);
  // Get the list of dataFields for action_id. The list of active fields needs
  // to be set
  if (all_fields_set) {
    dataFields = tableInfoGet()->dataFieldIdListGet(action_id);
    if (dataFields.empty()) {
      LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                status);
      return status;
    }
// TODO(sayanb)
#if 0
    std::vector<tdi_id_t> empty;
    match_data->setActiveFields(empty);
#endif
  } else {
// dataFields has already been populated
// with the correct fields since the requested action and actual
// action have also been verified. Only active fields need to be
// corrected because all fields must have been set now
// TODO(sayanb)
#if 0
    match_data->setActiveFields(dataFields);
#endif
  }

  for (const auto &dataFieldId : dataFields) {
    const tdi::DataFieldInfo *tableDataField =
        this->tableInfoGet()->dataFieldGet(dataFieldId, action_id);
    if (!tableDataField) {
      return TDI_OBJECT_NOT_FOUND;
    }
    auto fieldTypes = static_cast<const RtDataFieldContextInfo *>(
                          tableDataField->dataFieldContextInfoGet())
                          ->typesGet();
    auto dest = RtDataFieldContextInfo::getDataFieldDestination(fieldTypes);
    switch (dest) {
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
      case fieldDestination::ACTION_SPEC: {
        // We have already processed this. Ignore
        break;
      }
      default:
        LOG_TRACE("%s:%d %s Entry get for the data field %d not supported",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dataFieldId);
        return TDI_NOT_SUPPORTED;
        break;
    }
  }
  return TDI_SUCCESS;
}

#if 0

// MatchActionIndirectTable **************
namespace {
const std::vector<DataFieldType> indirectResourceDataFields = {
    DataFieldType::COUNTER_INDEX,
    DataFieldType::METER_INDEX,
    DataFieldType::REGISTER_INDEX};
}


void MatchActionIndirectTable::populate_indirect_resources(
    const pipe_mgr_adt_ent_data_t &ent_data,
    pipe_action_spec_t *pipe_action_spec) const {
  /* Append indirect action resources after already set resources in
   * pipe_action_spec. */
  for (int j = 0; j < ent_data.num_resources; j++) {
    int free_idx = pipe_action_spec->resource_count;
    pipe_res_spec_t *res_spec = &pipe_action_spec->resources[free_idx];
    res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
    res_spec->tbl_idx = (pipe_res_idx_t)ent_data.adt_data_resources[j].tbl_idx;
    res_spec->tbl_hdl = ent_data.adt_data_resources[j].tbl_hdl;
    pipe_action_spec->resource_count++;
  }
  return;
}

tdi_status_t MatchActionIndirectTable::entryAdd(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi::TableKey &key,
    const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);
  const MatchActionIndirectTableData &match_data =
      static_cast<const MatchActionIndirectTableData &>(data);
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  // Check if all indirect resource indices are supplied or not.
  // Entry Add mandates that all indirect indices be given
  // Initialize the direct resources if applicable
  status = resourceCheckAndInitialize<MatchActionIndirectTableData>(
      *this, match_data, false);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to initialize Direct resources",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
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

  match_data.copy_pipe_action_spec(&pipe_action_spec);

  // Fill in state from action member id or selector group id in the action spec
  pipe_mgr_adt_ent_data_t ap_ent_data;
  status = getActionState(session,
                          dev_tgt,
                          &match_data,
                          &adt_ent_hdl,
                          &sel_grp_hdl,
                          &act_fn_hdl,
                          &ap_ent_data);

  if (status != TDI_SUCCESS) {
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
      if (sel_grp_hdl == MatchActionIndirectTableData::invalid_group) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a group id %d which "
            "does not exist in the group table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getGroupId());
        return TDI_OBJECT_NOT_FOUND;
      } else if (adt_ent_hdl ==
                 MatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a group id %d which "
            "does not have any members in the group table associated with the "
            "table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getGroupId());
        return TDI_OBJECT_NOT_FOUND;
      }
    } else {
      if (adt_ent_hdl ==
          MatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add match entry referring to a action member id "
            "%d "
            "which "
            "does not exist in the action profile table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getActionMbrId());
        return TDI_OBJECT_NOT_FOUND;
      }
    }
    return TDI_UNEXPECTED;
  }

  if (match_data.isGroup()) {
    pipe_action_spec.sel_grp_hdl = sel_grp_hdl;
  } else {
    pipe_action_spec.adt_ent_hdl = adt_ent_hdl;
  }

  populate_indirect_resources(ap_ent_data, &pipe_action_spec);

  // Ready to add the entry
  return pipeMgr->pipeMgrMatEntAdd(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                   pipe_dev_tgt,
                                   pipe_tbl_hdl,
                                   &pipe_match_spec,
                                   act_fn_hdl,
                                   &pipe_action_spec,
                                   ttl,
                                   0 /* Pipe API flags */,
                                   &pipe_entry_hdl);
}

tdi_status_t MatchActionIndirectTable::entryMod(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_mat_ent_hdl_t pipe_entry_hdl;
  status = pipeMgr->pipeMgrMatchSpecToEntHdl(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                             pipe_dev_tgt,
                                             pipe_tbl_hdl,
                                             &pipe_match_spec,
                                             &pipe_entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  return entryModInternal(
      *this, session, dev_tgt, flags, data, pipe_entry_hdl);
}

tdi_status_t MatchActionIndirectTable::entryDel(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi::TableKey &key) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  pipe_tbl_match_spec_t pipe_match_spec = {0};

  match_key.populate_match_spec(&pipe_match_spec);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return pipeMgr->pipeMgrMatEntDelByMatchSpec(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                              pipe_dev_tgt,
                                              pipe_tbl_hdl,
                                              &pipe_match_spec,
                                              0 /* Pipe api flags */);
}

tdi_status_t MatchActionIndirectTable::tableClear(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/) const {
  if (this->is_const_table_) {
    LOG_TRACE(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return tableClearMatCommon(session, dev_tgt, true, this);
}

tdi_status_t MatchActionIndirectTable::entryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const Table *table_from_data;
  data->getParent(&table_from_data);
  auto table_id_from_data = table_from_data->tableInfoGet()->idGet();

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  tdi_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }

  return entryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, &pipe_match_spec, data);
}

tdi_status_t MatchActionIndirectTable::entryKeyGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi_handle_t &entry_handle,
    tdi::Target *entry_tgt,
    tdi::TableKey *key) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_tbl_match_spec_t *pipe_match_spec;
  bf_dev_pipe_t entry_pipe;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_status_t status = pipeMgr->pipeMgrEntHdlToMatchSpec(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      pipe_tbl_hdl,
      entry_handle,
      &entry_pipe,
      const_cast<const pipe_tbl_match_spec_t **>(&pipe_match_spec));
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  *entry_tgt = dev_tgt;
  entry_tgt->pipe_id = entry_pipe;
  match_key->set_key_from_match_spec_by_deepcopy(pipe_match_spec);
  return status;
}

tdi_status_t MatchActionIndirectTable::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        entry_handle);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t MatchActionIndirectTable::entryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi_handle_t &entry_handle,
    tdi::TableKey *key,
    tdi::TableData *data) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key->populate_match_spec(&pipe_match_spec);

  auto status = entryGet_internal(
      session, dev_tgt, flags, entry_handle, &pipe_match_spec, data);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

tdi_status_t MatchActionIndirectTable::entryGetFirst(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    tdi::TableKey *key,
    tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  tdi_id_t table_id_from_data;
  const Table *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Get the first entry handle present in pipe-mgr
  int first_entry_handle;
  tdi_status_t status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_tbl_hdl, pipe_dev_tgt, &first_entry_handle);

  if (status == TDI_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  pipe_tbl_match_spec_t pipe_match_spec;
  std::memset(&pipe_match_spec, 0, sizeof(pipe_tbl_match_spec_t));

  match_key->populate_match_spec(&pipe_match_spec);

  status = entryGet_internal(
      session, dev_tgt, flags, first_entry_handle, &pipe_match_spec, data);

  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting first entry, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  match_key->populate_key_from_match_spec(pipe_match_spec);
  return status;
}

tdi_status_t MatchActionIndirectTable::entryGetNext_n(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  const MatchActionKey &match_key =
      static_cast<const MatchActionKey &>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // First, get pipe-mgr entry handle associated with this key, since any get
  // API exposed by pipe-mgr needs entry handle
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  pipe_tbl_match_spec_t pipe_match_spec = {0};
  match_key.populate_match_spec(&pipe_match_spec);
  tdi_status_t status =
      pipeMgr->pipeMgrMatchSpecToEntHdl(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        &pipe_match_spec,
                                        &pipe_entry_hdl);
  // If key is not found and this is subsequent call, API should continue
  // from previous call.
  if (status == TDI_OBJECT_NOT_FOUND) {
    // Warn the user that currently used key no longer exist.
    LOG_WARN("%s:%d %s Provided key does not exist, trying previous handle",
             __func__,
             __LINE__,
             tableInfoGet()->nameGet().c_str());
  }

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : Entry does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  std::vector<int> next_entry_handles(n, 0);

  status = pipeMgr->pipeMgrGetNextEntryHandles(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                               pipe_tbl_hdl,
                                               pipe_dev_tgt,
                                               pipe_entry_hdl,
                                               n,
                                               next_entry_handles.data());
  if (status == TDI_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return TDI_SUCCESS;
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in getting next entry handles from pipe-mgr, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        status);
    return status;
  }
  pipe_tbl_match_spec_t match_spec;
  unsigned i = 0;
  for (i = 0; i < n; i++) {
    std::memset(&match_spec, 0, sizeof(pipe_tbl_match_spec_t));
    auto this_key =
        static_cast<MatchActionKey *>((*key_data_pairs)[i].first);
    auto this_data = (*key_data_pairs)[i].second;
    tdi_id_t table_id_from_data;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }
    if (next_entry_handles[i] == -1) {
      break;
    }
    this_key->populate_match_spec(&match_spec);
    status = entryGet_internal(
        session, dev_tgt, flags, next_entry_handles[i], &match_spec, this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR("%s:%d %s ERROR in getting %dth entry from pipe-mgr, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                i + 1,
                status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    this_key->setPriority(match_spec.priority);
    this_key->setPartitionIndex(match_spec.partition_index);
  }
  if (num_returned) {
    *num_returned = i;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirectTable::tableDefaultEntrySet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MatchActionIndirectTableData &match_data =
      static_cast<const MatchActionIndirectTableData &>(data);
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
        tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  pipe_mgr_adt_ent_data_t ap_ent_data;
  // Fill in state from action member id or selector group id in the action spec
  status = getActionState(session,
                          dev_tgt,
                          &match_data,
                          &adt_ent_hdl,
                          &sel_grp_hdl,
                          &act_fn_hdl,
                          &ap_ent_data);

  if (status != TDI_SUCCESS) {
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
      if (sel_grp_hdl == MatchActionIndirectTableData::invalid_group) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a group id %d "
            "which "
            "does not exist in the group table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getGroupId());
        return TDI_OBJECT_NOT_FOUND;
      } else if (adt_ent_hdl ==
                 MatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a group id %d "
            "which "
            "does not exist in the group table associated with the table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getGroupId());
        return TDI_OBJECT_NOT_FOUND;
      }
    } else {
      if (adt_ent_hdl ==
          MatchActionIndirectTableData::invalid_action_entry_hdl) {
        LOG_TRACE(
            "%s:%d %s: Cannot add default entry referring to a action member "
            "id "
            "%d "
            "which "
            "does not exist in the action profile table",
            __func__,
            __LINE__,
            tableInfoGet()->nameGet().c_str(),
            match_data.getActionMbrId());
        return TDI_OBJECT_NOT_FOUND;
      }
    }
    return TDI_UNEXPECTED;
  }

  match_data.copy_pipe_action_spec(&pipe_action_spec);

  if (match_data.isGroup()) {
    pipe_action_spec.sel_grp_hdl = sel_grp_hdl;
  } else {
    pipe_action_spec.adt_ent_hdl = adt_ent_hdl;
  }

  populate_indirect_resources(ap_ent_data, &pipe_action_spec);

  return pipeMgr->pipeMgrMatDefaultEntrySet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                            pipe_dev_tgt,
                                            pipe_tbl_hdl,
                                            act_fn_hdl,
                                            &pipe_action_spec,
                                            0 /* Pipe API flags */,
                                            &pipe_entry_hdl);
}

tdi_status_t MatchActionIndirectTable::tableDefaultEntryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_mat_ent_hdl_t pipe_entry_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  status = pipeMgr->pipeMgrTableGetDefaultEntryHandle(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, pipe_tbl_hdl, &pipe_entry_hdl);
  if (TDI_SUCCESS != status) {
    LOG_TRACE("%s:%d %s Dev %d pipe %x error %d getting default entry",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id,
              status);
    return status;
  }
  return entryGet_internal(
      session, dev_tgt, flags, pipe_entry_hdl, nullptr, data);
}

tdi_status_t MatchActionIndirectTable::tableDefaultEntryReset(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  if (this->has_const_default_action_) {
    // If default action is const, then this API is a no-op
    LOG_DBG(
        "%s:%d %s Calling reset on a table with const "
        "default action",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return TDI_SUCCESS;
  }
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  return pipeMgr->pipeMgrMatTblDefaultEntryReset(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, pipe_tbl_hdl, 0);
}

tdi_status_t MatchActionIndirectTable::getActionState(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const MatchActionIndirectTableData *data,
    pipe_adt_ent_hdl_t *adt_entry_hdl,
    pipe_sel_grp_hdl_t *sel_grp_hdl,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_mgr_adt_ent_data_t *ap_ent_data) const {
  tdi_status_t status = TDI_SUCCESS;
  ActionTable *actTbl = static_cast<ActionTable *>(actProfTbl);
  if (!data->isGroup()) {
    // Safe to do a static cast here since all table objects are constructed by
    // our factory and the right kind of sub-object is constructed by the
    // factory depending on the table type. Here the actProfTbl member of the
    // table object is a pointer to the action profile table associated with the
    // match-action table. It is guaranteed to be  of type ActionTable

    status = actTbl->getMbrState(session,
                                 dev_tgt,
                                 data->getActionMbrId(),
                                 act_fn_hdl,
                                 adt_entry_hdl,
                                 ap_ent_data);
    if (status != TDI_SUCCESS) {
      *adt_entry_hdl =
          MatchActionIndirectTableData::invalid_action_entry_hdl;
      return status;
    }
  } else {
    SelectorTable *selTbl = static_cast<SelectorTable *>(selectorTbl);
    status =
        selTbl->getGrpHdl(session, dev_tgt, data->getGroupId(), sel_grp_hdl);
    if (status != TDI_SUCCESS) {
      *sel_grp_hdl = MatchActionIndirectTableData::invalid_group;
      return TDI_OBJECT_NOT_FOUND;
    }
    status =
        selTbl->getOneMbr(session, dev_tgt.dev_id, *sel_grp_hdl, adt_entry_hdl);
    if (status != TDI_SUCCESS) {
      *adt_entry_hdl =
          MatchActionIndirectTableData::invalid_action_entry_hdl;
      return TDI_OBJECT_NOT_FOUND;
    }

    tdi_id_t a_member_id = 0;
    status =
        getActionMbrIdFromHndl(session, dev_tgt, *adt_entry_hdl, &a_member_id);
    if (status != TDI_SUCCESS) {
      return TDI_OBJECT_NOT_FOUND;
    }

    status = actTbl->getMbrState(
        session, dev_tgt, a_member_id, act_fn_hdl, adt_entry_hdl, ap_ent_data);
  }
  return status;
}


tdi_status_t MatchActionIndirectTable::getActionMbrIdFromHndl(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const pipe_adt_ent_hdl_t adt_ent_hdl,
    tdi_id_t *mbr_id) const {
  ActionTable *actTbl = static_cast<ActionTable *>(actProfTbl);
  return actTbl->getMbrIdFromHndl(session, dev_tgt, adt_ent_hdl, mbr_id);
}

tdi_status_t MatchActionIndirectTable::getGroupIdFromHndl(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const pipe_sel_grp_hdl_t sel_grp_hdl,
    tdi_id_t *grp_id) const {
  SelectorTable *selTbl = static_cast<SelectorTable *>(selectorTbl);
  return selTbl->getGrpIdFromHndl(session, dev_tgt, sel_grp_hdl, grp_id);
}

tdi_status_t MatchActionIndirectTable::tableSizeGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    size_t *count) const {
  tdi_status_t status = TDI_SUCCESS;
  size_t reserved = 0;
  status = getReservedEntries(session, dev_tgt, *(this), &reserved);
  *count = this->_table_size - reserved;
  return status;
}

tdi_status_t MatchActionIndirectTable::tableUsageGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

tdi_status_t MatchActionIndirectTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new MatchActionKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirectTable::keyReset(TableKey *key) const {
  MatchActionKey *match_key = static_cast<MatchActionKey *>(key);
  return key_reset<MatchActionIndirectTable, MatchActionKey>(*this,
                                                                     match_key);
}

tdi_status_t MatchActionIndirectTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  *data_ret = std::unique_ptr<tdi::TableData>(
      new MatchActionIndirectTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirectTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(
      new MatchActionIndirectTableData(this, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirectTable::dataReset(tdi::TableData *data) const {
  std::vector<tdi_id_t> fields;
  return dataReset_internal(*this, 0, fields, data);
}

tdi_status_t MatchActionIndirectTable::dataReset(
    const std::vector<tdi_id_t> &fields, tdi::TableData *data) const {
  return dataReset_internal(*this, 0, fields, data);
}

tdi_status_t MatchActionIndirectTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != TDI_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  if (type == TableAttributesType::IDLE_TABLE_RUNTIME) {
    LOG_TRACE(
        "%s:%d %s Idle Table Runtime Attribute requires a Mode, please use the "
        "appropriate API to include the idle table mode",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new TableAttributesImpl(this, type));
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirectTable::attributeAllocate(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<TableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != TDI_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  if (type != TableAttributesType::IDLE_TABLE_RUNTIME) {
    LOG_TRACE("%s:%d %s Idle Table Mode cannot be set for Attribute %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_INVALID_ARG;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new TableAttributesImpl(this, type, idle_table_mode));
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirectTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<TableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(type));
      return TDI_INVALID_ARG;
    case TableAttributesType::ENTRY_SCOPE:
    case TableAttributesType::DYNAMIC_KEY_MASK:
      break;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                static_cast<int>(type));
      return TDI_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

tdi_status_t MatchActionIndirectTable::attributeReset(
    const TableAttributesType &type,
    const TableAttributesIdleTableMode &idle_table_mode,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<TableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      break;
    case TableAttributesType::ENTRY_SCOPE:
    case TableAttributesType::DYNAMIC_KEY_MASK:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
      LOG_TRACE(
          "%s:%d %s This API cannot be used to Reset Attribute Obj to type %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(type));
      return TDI_INVALID_ARG;
    default:
      LOG_TRACE("%s:%d %s Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                static_cast<int>(type));
      return TDI_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type, idle_table_mode);
}

tdi_status_t MatchActionIndirectTable::tableAttributesSet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const TableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const TableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != TDI_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(attr_type));
    return TDI_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      TableEntryScope entry_scope;
      TableEntryScopeArgumentsImpl scope_args(0);
      tdi_status_t sts = tbl_attr_impl->entryScopeParamsGet(
          &entry_scope,
          static_cast<TableEntryScopeArguments *>(&scope_args));
      if (sts != TDI_SUCCESS) {
        return sts;
      }
      // Call the pipe mgr API to set entry scope
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      // Derive pipe mgr entry scope from TDI entry scope and set it to
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
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      sts = pipeMgr->pipeMgrTblSetProperty(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                           dev_tgt.dev_id,
                                           pipe_tbl_hdl,
                                           prop_type,
                                           prop_val,
                                           args_val);
      return sts;
    }
    case TableAttributesType::DYNAMIC_KEY_MASK: {
      std::unordered_map<tdi_id_t, std::vector<uint8_t>> field_mask;
      tdi_status_t sts = tbl_attr_impl->dynKeyMaskGet(&field_mask);
      pipe_tbl_match_spec_t match_spec;
      const int sz = this->getKeySize().bytes;
      std::vector<uint8_t> match_mask_bits(sz, 0);
      match_spec.match_mask_bits = match_mask_bits.data();
      // translate from map to match_spec
      sts = fieldMasksToMatchSpec(field_mask, &match_spec, this);
      if (sts != TDI_SUCCESS) return sts;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      sts = pipeMgr->pipeMgrMatchKeyMaskSpecSet(
          session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, pipe_tbl_hdl, &match_spec);
      return sts;
    }
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      return setIdleTable(const_cast<MatchActionIndirectTable &>(*this),
                          session,
                          dev_tgt,
                          *tbl_attr_impl);
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirectTable::tableAttributesGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    TableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<TableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != TDI_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(attr_type));
    return TDI_NOT_SUPPORTED;
  }
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      auto sts = pipeMgr->pipeMgrTblGetProperty(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                                dev_tgt.dev_id,
                                                pipe_tbl_hdl,
                                                prop_type,
                                                &prop_val,
                                                &args_val);
      if (sts != PIPE_SUCCESS) {
        LOG_TRACE("%s:%d %s Failed to get entry scope from pipe_mgr",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return sts;
      }

      TableEntryScope entry_scope;
      TableEntryScopeArgumentsImpl scope_args(args_val.value);

      // Derive TDI entry scope from pipe mgr entry scope
      entry_scope = prop_val.value == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
                        ? TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
                        : prop_val.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                              ? TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                              : TableEntryScope::ENTRY_SCOPE_USER_DEFINED;

      return tbl_attr_impl->entryScopeParamsSet(
          entry_scope, static_cast<TableEntryScopeArguments &>(scope_args));
    }
    case TableAttributesType::DYNAMIC_KEY_MASK: {
      pipe_tbl_match_spec_t mat_spec;
      const int sz = this->getKeySize().bytes;
      std::vector<uint8_t> match_mask_bits(sz, 0);
      mat_spec.match_mask_bits = match_mask_bits.data();
      mat_spec.num_match_bytes = sz;
      mat_spec.num_valid_match_bits = this->getKeySize().bits;

      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      pipeMgr->pipeMgrMatchKeyMaskSpecGet(
          session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, pipe_tbl_hdl, &mat_spec);
      std::unordered_map<tdi_id_t, std::vector<uint8_t>> field_mask;
      matchSpecToFieldMasks(mat_spec, &field_mask, this);
      return tbl_attr_impl->dynKeyMaskSet(field_mask);
    }
    case TableAttributesType::IDLE_TABLE_RUNTIME:
      return getIdleTable(*this, session, dev_tgt, tbl_attr_impl);
    case TableAttributesType::DYNAMIC_HASH_ALG_SEED:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "get "
          "attributes",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

// Unexposed functions
tdi_status_t MatchActionIndirectTable::entryGet_internal(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const pipe_mat_ent_hdl_t &pipe_entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_action_spec_t *pipe_action_spec = nullptr;
  pipe_act_fn_hdl_t pipe_act_fn_hdl = 0;
  pipe_res_get_data_t res_data;

  bool stful_fetched = false;
  tdi_id_t ttl_field_id = 0;
  tdi_id_t hs_field_id = 0;
  uint32_t res_get_flags = 0;
  res_data.stful.data = nullptr;

  MatchActionIndirectTableData *match_data =
      static_cast<MatchActionIndirectTableData *>(data);
  std::vector<tdi_id_t> dataFields;
  bool all_fields_set = match_data->allFieldsSetGet();

  tdi_id_t req_action_id = match_data->actionIdGet(&req_action_id);

  if (all_fields_set) {
    res_get_flags = PIPE_RES_GET_FLAG_ALL;
  } else {
    dataFields.assign(match_data->activeFieldsGet().begin(),
                      match_data->activeFieldsGet().end());
    for (const auto &dataFieldId : match_data->activeFieldsGet()) {
      const tdi::DataFieldInfo *tableDataField = nullptr;
      status = dataFieldGet(dataFieldId, &tableDataField);
      TDI_ASSERT(status == TDI_SUCCESS);
      auto fieldTypes = tableDataField->getTypes();
      fieldDestination field_destination =
          tdi::DataFieldInfo::getDataFieldDestination(fieldTypes);
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
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR getting action spec for pipe entry handle %d, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        pipe_entry_hdl,
        status);
    // Must free stful related memory
    if (res_data.stful.data != nullptr) {
      bf_sys_free(res_data.stful.data);
    }
    return status;
  }

  // There is no direct action in indirect flow, hence always fill in
  // only requested fields.
  if (all_fields_set) {
    status = dataFieldIdListGet(&dataFields);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s ERROR in getting data Fields, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                status);
      if (res_data.stful.data != nullptr) {
        bf_sys_free(res_data.stful.data);
      }
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
    const tdi::DataFieldInfo *tableDataField = nullptr;
    status = dataFieldGet(dataFieldId, &tableDataField);
    TDI_ASSERT(status == TDI_SUCCESS);
    auto fieldTypes = tableDataField->getTypes();
    fieldDestination field_destination =
        tdi::DataFieldInfo::getDataFieldDestination(fieldTypes);
    std::set<tdi_id_t> oneof_siblings;
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
            tdi_id_t act_mbr_id;
            status = this->getActionMbrIdFromHndl(
                session, dev_tgt, pipe_action_spec->adt_ent_hdl, &act_mbr_id);
            // Default entries will not have an action member handle if they
            // were installed automatically.  In this case return a member id
            // of zero.
            if (status == TDI_OBJECT_NOT_FOUND &&
                !pipe_match_spec &&  // Default entries won't have a match spec
                pipe_action_spec->adt_ent_hdl == 0) {
              status = TDI_SUCCESS;
              act_mbr_id = 0;
            }
            TDI_ASSERT(status == TDI_SUCCESS);
            match_data->setActionMbrId(act_mbr_id);
            // Remove oneof sibling from active fields
            match_data->removeActiveFields(oneof_siblings);
          }
        } else if (fieldTypes.find(DataFieldType::SELECTOR_GROUP_ID) !=
                   fieldTypes.end()) {
          if (IS_ACTION_SPEC_SEL_GRP(pipe_action_spec)) {
            tdi_id_t sel_grp_id;
            status = getGroupIdFromHndl(
                session, dev_tgt, pipe_action_spec->sel_grp_hdl, &sel_grp_id);
            TDI_ASSERT(status == TDI_SUCCESS);
            match_data->setGroupId(sel_grp_id);
            // Remove oneof sibling from active fields
            match_data->removeActiveFields(oneof_siblings);
          }
        } else {
          TDI_ASSERT(0);
        }
        break;
      }
      default:
        LOG_TRACE("%s:%d %s Entry get for the data field %d not supported",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str(),
                  dataFieldId);
        if (res_data.stful.data != nullptr) {
          bf_sys_free(res_data.stful.data);
        }
        return TDI_NOT_SUPPORTED;
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
  return TDI_SUCCESS;
}

tdi_status_t MatchActionIndirectTable::registerMatUpdateCb(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const pipe_mat_update_cb &cb,
    const void *cookie) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeRegisterMatUpdateCb(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                          dev_tgt.dev_id,
                                          this->tablePipeHandleGet(),
                                          cb,
                                          const_cast<void *>(cookie));
}

// ActionTable

tdi_status_t ActionTable::entryAdd(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const tdi::TableKey &key,
                                           const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const ActionTableKey &action_tbl_key =
      static_cast<const ActionTableKey &>(key);
  const ActionTableData &action_tbl_data =
      static_cast<const ActionTableData &>(data);
  const pipe_action_spec_t *pipe_action_spec =
      action_tbl_data.get_pipe_action_spec();

  tdi_id_t mbr_id = action_tbl_key.getMemberId();

  pipe_adt_ent_hdl_t adt_entry_hdl;
  pipe_act_fn_hdl_t act_fn_hdl = action_tbl_data.getActFnHdl();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  uint32_t pipe_flags = PIPE_FLAG_CACHE_ENT_ID;
  status = pipeMgr->pipeMgrAdtEntAdd(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     act_fn_hdl,
                                     mbr_id,
                                     pipe_action_spec,
                                     &adt_entry_hdl,
                                     pipe_flags);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Error adding new ADT %d entry with mbr_id %d.",
              __func__,
              __LINE__,
              tableInfoGet()->idGet(),
              mbr_id);
  }
  return status;
}

tdi_status_t ActionTable::entryMod(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const tdi::TableKey &key,
                                           const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const ActionTableKey &action_tbl_key =
      static_cast<const ActionTableKey &>(key);
  const ActionTableData &action_tbl_data =
      static_cast<const ActionTableData &>(data);
  const pipe_action_spec_t *pipe_action_spec =
      action_tbl_data.get_pipe_action_spec();

  tdi_id_t mbr_id = action_tbl_key.getMemberId();

  pipe_act_fn_hdl_t act_fn_hdl = action_tbl_data.getActFnHdl();

  // Action entry handle doesn't change during a modify
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  // Get the action entry handle used by pipe-mgr from the member id
  status = pipeMgr->pipeMgrAdtEntHdlGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        mbr_id,
                                        &adt_ent_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x to modify",
              __func__,
              __LINE__,
              mbr_id,
              tableInfoGet()->idGet());
    return TDI_OBJECT_NOT_FOUND;
  }

  status = pipeMgr->pipeMgrAdtEntSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                     pipe_dev_tgt.device_id,
                                     pipe_tbl_hdl,
                                     adt_ent_hdl,
                                     act_fn_hdl,
                                     pipe_action_spec,
                                     0 /* Pipe API flags */);

  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d Error in modifying action profile member for tbl id %d "
        "member id %d, err %s",
        __func__,
        __LINE__,
        tableInfoGet()->idGet(),
        mbr_id,
        pipe_str_err((pipe_status_t)status));
  }
  return status;
}

tdi_status_t ActionTable::entryDel(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const uint64_t & /*flags*/,
                                           const tdi::TableKey &key) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const ActionTableKey &action_tbl_key =
      static_cast<const ActionTableKey &>(key);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  tdi_id_t mbr_id = action_tbl_key.getMemberId();

  // Get the action entry handle used by pipe-mgr from the member id
  status = pipeMgr->pipeMgrAdtEntHdlGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        mbr_id,
                                        &adt_ent_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x to modify",
              __func__,
              __LINE__,
              mbr_id,
              tableInfoGet()->idGet());
    return TDI_OBJECT_NOT_FOUND;
  }

  status = pipeMgr->pipeMgrAdtEntDel(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                     pipe_dev_tgt.device_id,
                                     pipe_tbl_hdl,
                                     adt_ent_hdl,
                                     0 /* Pipe api flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d Error in deletion of action profile member %d for tbl id %d "
        ", err %s",
        __func__,
        __LINE__,
        mbr_id,
        tableInfoGet()->idGet(),
        pipe_str_err((pipe_status_t)status));
  }
  return status;
}

tdi_status_t ActionTable::tableClear(const tdi::Session &session,
                                        const tdi::Target &dev_tgt,
                                        const uint64_t & /*flags*/) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_adt_ent_hdl_t adt_ent_hdl = 0;

  while (TDI_SUCCESS ==
         pipeMgr->pipeMgrGetFirstEntryHandle(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                             pipe_tbl_hdl,
                                             pipe_dev_tgt,
                                             (int *)&adt_ent_hdl)) {
    status = pipeMgr->pipeMgrAdtEntDel(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                       pipe_dev_tgt.device_id,
                                       pipe_tbl_hdl,
                                       adt_ent_hdl,
                                       0 /* Pipe api flags */);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d Error deleting action profile member for tbl id 0x%x "
          "handle %d, err %s",
          __func__,
          __LINE__,
          tableInfoGet()->idGet(),
          adt_ent_hdl,
          pipe_str_err((pipe_status_t)status));
      return status;
    }
  }
  return status;
}

tdi_status_t ActionTable::entryGet(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags &flags,
                                           const tdi::TableKey &key,
                                           tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  tdi_id_t table_id_from_data;
  const Table *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  const ActionTableKey &action_tbl_key =
      static_cast<const ActionTableKey &>(key);
  ActionTableData *action_tbl_data =
      static_cast<ActionTableData *>(data);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  tdi_id_t mbr_id = action_tbl_key.getMemberId();
  pipe_adt_ent_hdl_t adt_ent_hdl;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  status = pipeMgr->pipeMgrAdtEntHdlGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        mbr_id,
                                        &adt_ent_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Action member Id %d does not exist",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              mbr_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  return entryGet_internal(
      session, dev_tgt, flags, adt_ent_hdl, action_tbl_data);
}

tdi_status_t ActionTable::entryKeyGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi_handle_t &entry_handle,
    tdi::Target *entry_tgt,
    tdi::TableKey *key) const {
  ActionTableKey *action_tbl_key = static_cast<ActionTableKey *>(key);
  tdi_id_t mbr_id;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  auto status = pipeMgr->pipeMgrAdtEntMbrIdGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                               pipe_dev_tgt,
                                               pipe_tbl_hdl,
                                               entry_handle,
                                               &mbr_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting entry key, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  action_tbl_key->setMemberId(mbr_id);
  *entry_tgt = dev_tgt;
  return status;
}

tdi_status_t ActionTable::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const ActionTableKey &action_tbl_key =
      static_cast<const ActionTableKey &>(key);
  tdi_id_t mbr_id = action_tbl_key.getMemberId();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  auto status = pipeMgr->pipeMgrAdtEntHdlGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                             pipe_dev_tgt,
                                             pipe_tbl_hdl,
                                             mbr_id,
                                             entry_handle);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t ActionTable::entryGet(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags &flags,
                                           const tdi_handle_t &entry_handle,
                                           tdi::TableKey *key,
                                           tdi::TableData *data) const {
  tdi::Target entry_tgt;
  tdi_status_t status = this->entryKeyGet(
      session, dev_tgt, flags, entry_handle, &entry_tgt, key);
  if (status != TDI_SUCCESS) {
    return status;
  }
  return this->entryGet(session, entry_tgt, flags, *key, data);
}

tdi_status_t ActionTable::entryGetFirst(const tdi::Session &session,
                                                const tdi::Target &dev_tgt,
                                                const tdi::Flags &flags,
                                                tdi::TableKey *key,
                                                tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  tdi_id_t table_id_from_data;
  const Table *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  ActionTableKey *action_tbl_key = static_cast<ActionTableKey *>(key);
  ActionTableData *action_tbl_data =
      static_cast<ActionTableData *>(data);

  tdi_id_t first_mbr_id;
  int first_entry_hdl;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_tbl_hdl, pipe_dev_tgt, &first_entry_hdl);

  if (status == TDI_OBJECT_NOT_FOUND) {
    return status;
  } else if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first, status %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  status = pipeMgr->pipeMgrAdtEntMbrIdGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                          pipe_dev_tgt,
                                          pipe_tbl_hdl,
                                          first_entry_hdl,
                                          &first_mbr_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first entry member id, status %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  status = entryGet_internal(
      session, dev_tgt, flags, first_entry_hdl, action_tbl_data);
  if (status != TDI_SUCCESS) {
    return status;
  }
  action_tbl_key->setMemberId(first_mbr_id);
  return TDI_SUCCESS;
}

tdi_status_t ActionTable::entryGetNext_n(const tdi::Session &session,
                                                 const tdi::Target &dev_tgt,
                                                 const tdi::Flags &flags,
                                                 const tdi::TableKey &key,
                                                 const uint32_t &n,
                                                 tdi::Table::keyDataPairs *key_data_pairs,
                                                 uint32_t *num_returned) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const ActionTableKey &action_tbl_key =
      static_cast<const ActionTableKey &>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_id_t mbr_id = action_tbl_key.getMemberId();
  tdi_id_t pipe_entry_hdl;
  status = pipeMgr->pipeMgrAdtEntHdlGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        mbr_id,
                                        &pipe_entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Member Id %d does not exist for tbl 0x%x",
              __func__,
              __LINE__,
              mbr_id,
              tableInfoGet()->idGet());
    return TDI_OBJECT_NOT_FOUND;
  }

  std::vector<int> next_entry_handles(n, 0);
  status = pipeMgr->pipeMgrGetNextEntryHandles(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                               pipe_tbl_hdl,
                                               pipe_dev_tgt,
                                               pipe_entry_hdl,
                                               n,
                                               next_entry_handles.data());
  if (status == TDI_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return TDI_SUCCESS;
  }

  unsigned i = 0;
  for (i = 0; i < n; i++) {
    tdi_id_t next_mbr_id;
    // Get the action entry handle used by pipe-mgr from the member id
    status = pipeMgr->pipeMgrAdtEntMbrIdGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                            pipe_dev_tgt,
                                            pipe_tbl_hdl,
                                            next_entry_handles[i],
                                            &next_mbr_id);
    if (status) break;
    auto this_key =
        static_cast<ActionTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<ActionTableData *>((*key_data_pairs)[i].second);
    tdi_id_t table_id_from_data;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d  does not "
          "match "
          "the table",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }
    status = entryGet_internal(
        session, dev_tgt, flags, next_entry_handles[i], this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR in getting %dth entry from pipe-mgr with entry "
          "handle %d, mbr id %d, err %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          i + 1,
          next_entry_handles[i],
          next_mbr_id,
          status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    this_key->setMemberId(next_mbr_id);
  }
  if (num_returned) {
    *num_returned = i;
  }
  return TDI_SUCCESS;
}

tdi_status_t ActionTable::tableUsageGet(const tdi::Session &session,
                                           const tdi::Target &dev_tgt,
                                           const tdi::Flags &flags,
                                           uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

tdi_status_t ActionTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new ActionTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t ActionTable::dataAllocate(
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  if (action_info_list.find(action_id) == action_info_list.end()) {
    LOG_TRACE("%s:%d Action_ID %d not found", __func__, __LINE__, action_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *data_ret =
      std::unique_ptr<tdi::TableData>(new ActionTableData(this, action_id));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t ActionTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  // This dataAllocate is mainly used for entry gets from the action table
  // wherein  the action id of the entry is not known and will be filled in by
  // the entry get
  *data_ret = std::unique_ptr<tdi::TableData>(new ActionTableData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t ActionTable::getMbrState(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    tdi_id_t mbr_id,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_adt_ent_hdl_t *adt_ent_hdl,
    pipe_mgr_adt_ent_data_t *ap_ent_data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto status = pipeMgr->pipeMgrAdtEntDataGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                              pipe_dev_tgt,
                                              pipe_tbl_hdl,
                                              mbr_id,
                                              adt_ent_hdl,
                                              ap_ent_data);
  *act_fn_hdl = ap_ent_data->act_fn_hdl;

  return status;
}

tdi_status_t ActionTable::getMbrIdFromHndl(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const pipe_adt_ent_hdl_t adt_ent_hdl,
    tdi_id_t *mbr_id) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return pipeMgr->pipeMgrAdtEntMbrIdGet(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, pipe_tbl_hdl, adt_ent_hdl, mbr_id);
}

tdi_status_t ActionTable::getHdlFromMbrId(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi_id_t mbr_id,
    pipe_adt_ent_hdl_t *adt_ent_hdl) const {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  return pipeMgr->pipeMgrAdtEntHdlGet(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, pipe_tbl_hdl, mbr_id, adt_ent_hdl);
}

tdi_status_t ActionTable::registerAdtUpdateCb(const tdi::Session &session,
                                                 const tdi::Target &dev_tgt,
                                                 const uint64_t & /*flags*/,
                                                 const pipe_adt_update_cb &cb,
                                                 const void *cookie) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeRegisterAdtUpdateCb(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                          dev_tgt.dev_id,
                                          this->tablePipeHandleGet(),
                                          cb,
                                          const_cast<void *>(cookie));
}

tdi_status_t ActionTable::entryGet_internal(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const pipe_adt_ent_hdl_t &entry_hdl,
    ActionTableData *action_tbl_data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  pipe_action_spec_t *action_spec = action_tbl_data->mutable_pipe_action_spec();
  bool read_from_hw = false;
  if (TDI_FLAG_IS_SET(flags, TDI_FROM_HW)) {
    read_from_hw = true;
  }
  pipe_act_fn_hdl_t act_fn_hdl;
  tdi_status_t status =
      pipeMgr->pipeMgrGetActionDataEntry(pipe_tbl_hdl,
                                         dev_tgt.dev_id,
                                         entry_hdl,
                                         &action_spec->act_data,
                                         &act_fn_hdl,
                                         read_from_hw);
  // At this point, if a member wasn't found, there is a high chance
  // that the action data wasn't programmed in the hw itself because by
  // this time TDI sw state check has passed. So try it once again with
  // with read_from_hw = False
  if (status == TDI_OBJECT_NOT_FOUND && read_from_hw) {
    status = pipeMgr->pipeMgrGetActionDataEntry(pipe_tbl_hdl,
                                                dev_tgt.dev_id,
                                                entry_hdl,
                                                &action_spec->act_data,
                                                &act_fn_hdl,
                                                false);
  }
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR in getting action data from pipe-mgr, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  tdi_id_t action_id = this->getActIdFromActFnHdl(act_fn_hdl);

  action_tbl_data->actionIdSet(action_id);
  std::vector<tdi_id_t> empty;
  action_tbl_data->setActiveFields(empty);
  return TDI_SUCCESS;
}

tdi_status_t ActionTable::keyReset(TableKey *key) const {
  ActionTableKey *action_key = static_cast<ActionTableKey *>(key);
  return key_reset<ActionTable, ActionTableKey>(*this, action_key);
}

tdi_status_t ActionTable::dataReset(tdi::TableData *data) const {
  ActionTableData *data_obj = static_cast<ActionTableData *>(data);
  if (!this->validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return data_obj->reset(0);
}

tdi_status_t ActionTable::dataReset(const tdi_id_t &action_id,
                                       tdi::TableData *data) const {
  ActionTableData *data_obj = static_cast<ActionTableData *>(data);
  if (!this->validateTable_from_dataObj(*data_obj)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return data_obj->reset(action_id);
}

// SelectorTable **************
tdi_status_t SelectorTable::getActMbrIdFromHndl(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const pipe_adt_ent_hdl_t &adt_ent_hdl,
    tdi_id_t *act_mbr_id) const {
  ActionTable *actTbl = static_cast<ActionTable *>(actProfTbl);
  return actTbl->getMbrIdFromHndl(session, dev_tgt, adt_ent_hdl, act_mbr_id);
}

tdi_status_t SelectorTable::entryAdd(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const tdi::TableKey &key,
                                             const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SelectorTableKey &sel_key =
      static_cast<const SelectorTableKey &>(key);
  const SelectorTableData &sel_data =
      static_cast<const SelectorTableData &>(data);
  // Make a call to pipe-mgr to first create the group
  pipe_sel_grp_hdl_t sel_grp_hdl;
  uint32_t max_grp_size = sel_data.get_max_grp_size();
  tdi_id_t sel_grp_id = sel_key.getGroupId();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  std::vector<tdi_id_t> members = sel_data.getMembers();
  std::vector<bool> member_status = sel_data.getMemberStatus();

  if (members.size() != member_status.size()) {
    LOG_TRACE("%s:%d MemberId size %zu and member status size %zu do not match",
              __func__,
              __LINE__,
              members.size(),
              member_status.size());
    return TDI_INVALID_ARG;
  }

  if (members.size() > max_grp_size) {
    LOG_TRACE(
        "%s:%d %s Number of members provided %zd exceeds the maximum group "
        "size %d for group id %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        members.size(),
        max_grp_size,
        sel_grp_id);
    return TDI_INVALID_ARG;
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

    ActionTable *actTbl = static_cast<ActionTable *>(actProfTbl);
    status =
        actTbl->getHdlFromMbrId(session, dev_tgt, members[i], &adt_ent_hdl);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in adding member id %d which does not exist into "
          "group id %d on pipe %x",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          members[i],
          sel_grp_id,
          dev_tgt.pipe_id);
      return TDI_INVALID_ARG;
    }

    action_entry_hdls[i] = adt_ent_hdl;
    pipe_member_status[i] = member_status[i];
  }

  // Now, attempt to add the group;
  uint32_t pipe_flags = PIPE_FLAG_CACHE_ENT_ID;
  status = pipeMgr->pipeMgrSelGrpAdd(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     sel_grp_id,
                                     max_grp_size,
                                     &sel_grp_hdl,
                                     pipe_flags);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in adding group id %d pipe %x, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              sel_grp_id,
              dev_tgt.pipe_id,
              status);
    return status;
  }

  // Set the membership of the group
  status = pipeMgr->pipeMgrSelGrpMbrsSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                         pipe_dev_tgt.device_id,
                                         pipe_tbl_hdl,
                                         sel_grp_hdl,
                                         members.size(),
                                         action_entry_hdls.data(),
                                         (bool *)(pipe_member_status.data()),
                                         0 /* Pipe API flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s : Error in setting membership for group id %d pipe %x, err "
        "%d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        sel_grp_id,
        dev_tgt.pipe_id,
        status);
    pipeMgr->pipeMgrSelGrpDel(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                              pipe_dev_tgt.device_id,
                              pipe_tbl_hdl,
                              sel_grp_hdl,
                              0 /* Pipe API flags */);
  }

  return status;
}

tdi_status_t SelectorTable::entryMod(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const tdi::TableKey &key,
                                             const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SelectorTableKey &sel_key =
      static_cast<const SelectorTableKey &>(key);
  const SelectorTableData &sel_data =
      static_cast<const SelectorTableData &>(data);

  std::vector<tdi_id_t> members = sel_data.getMembers();
  std::vector<bool> member_status = sel_data.getMemberStatus();
  std::vector<pipe_adt_ent_hdl_t> action_entry_hdls(members.size(), 0);
  std::vector<char> pipe_member_status(members.size(), 0);

  // Get the mapping from selector group id to selector group handle

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  tdi_id_t sel_grp_id = sel_key.getGroupId();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  status = pipeMgr->pipeMgrSelGrpHdlGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        sel_grp_id,
                                        &sel_grp_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  uint32_t curr_size;
  status = pipeMgr->pipeMgrSelGrpMaxSizeGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                            dev_tgt.dev_id,
                                            pipe_tbl_hdl,
                                            sel_grp_hdl,
                                            &curr_size);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting max grp size, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  // Next, validate the member IDs
  ActionTable *actTbl = static_cast<ActionTable *>(actProfTbl);
  for (unsigned i = 0; i < members.size(); ++i) {
    pipe_adt_ent_hdl_t adt_ent_hdl;
    status =
        actTbl->getHdlFromMbrId(session, dev_tgt, members[i], &adt_ent_hdl);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in adding member id %d which not exist into group "
          "id %d pipe %x",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          members[i],
          sel_grp_id,
          dev_tgt.pipe_id);
      return TDI_INVALID_ARG;
    }
    action_entry_hdls[i] = adt_ent_hdl;
    pipe_member_status[i] = member_status[i];
  }

  bool membrs_set = false;
  // If new members will fit current size, set members first to support
  // downsizing of the group.
  if (curr_size >= members.size()) {
    status = pipeMgr->pipeMgrSelGrpMbrsSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                           pipe_dev_tgt.device_id,
                                           pipe_tbl_hdl,
                                           sel_grp_hdl,
                                           members.size(),
                                           action_entry_hdls.data(),
                                           (bool *)(pipe_member_status.data()),
                                           0 /* Pipe API flags */);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in setting membership for group id %d pipe %x, err "
          "%d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          sel_grp_id,
          dev_tgt.pipe_id,
          status);
      return status;
    }
    membrs_set = true;
  }
  const auto max_grp_size = sel_data.get_max_grp_size();
  // Size of 0 is ignored, means no change in size.
  if (max_grp_size != 0 && curr_size != max_grp_size) {
    status = pipeMgr->pipeMgrSelGrpSizeSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                           pipe_dev_tgt,
                                           pipe_tbl_hdl,
                                           sel_grp_hdl,
                                           max_grp_size);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in setting group size for id %d pipe %x, err %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          sel_grp_id,
          dev_tgt.pipe_id,
          status);
      return status;
    }
  }
  if (membrs_set == false) {
    status = pipeMgr->pipeMgrSelGrpMbrsSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                           pipe_dev_tgt.device_id,
                                           pipe_tbl_hdl,
                                           sel_grp_hdl,
                                           members.size(),
                                           action_entry_hdls.data(),
                                           (bool *)(pipe_member_status.data()),
                                           0 /* Pipe API flags */);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s Error in setting membership for group id %d pipe %x, err "
          "%d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          sel_grp_id,
          dev_tgt.pipe_id,
          status);
      return status;
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t SelectorTable::entryDel(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const tdi::TableKey &key) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const SelectorTableKey &sel_key =
      static_cast<const SelectorTableKey &>(key);
  tdi_id_t sel_grp_id = sel_key.getGroupId();

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  status = pipeMgr->pipeMgrSelGrpHdlGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        sel_grp_id,
                                        &sel_grp_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  status = pipeMgr->pipeMgrSelGrpDel(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                     pipe_dev_tgt.device_id,
                                     pipe_tbl_hdl,
                                     sel_grp_hdl,
                                     0 /* Pipe API flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error deleting selector group %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              sel_grp_id,
              status);
    return status;
  }
  return TDI_SUCCESS;
}

tdi_status_t SelectorTable::tableClear(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const uint64_t & /*flags*/
                                          ) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  pipe_sel_grp_hdl_t sel_grp_hdl = 0;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  while (TDI_SUCCESS ==
         pipeMgr->pipeMgrGetFirstEntryHandle(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                             pipe_tbl_hdl,
                                             pipe_dev_tgt,
                                             (int *)&sel_grp_hdl)) {
    status = pipeMgr->pipeMgrSelGrpDel(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                       pipe_dev_tgt.device_id,
                                       pipe_tbl_hdl,
                                       sel_grp_hdl,
                                       0 /* Pipe API flags */);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s Error deleting selector group %d pipe %x, err %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                sel_grp_hdl,
                dev_tgt.pipe_id,
                status);
      return status;
    }
  }
  return status;
}

tdi_status_t SelectorTable::entryGet_internal(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi_id_t &grp_id,
    SelectorTableData *sel_tbl_data) const {
  tdi_status_t status;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_sel_grp_hdl_t sel_grp_hdl;
  status = pipeMgr->pipeMgrSelGrpHdlGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        grp_id,
                                        &sel_grp_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }

  // Get the max size configured for the group
  uint32_t max_grp_size = 0;
  status = pipeMgr->pipeMgrSelGrpMaxSizeGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                            dev_tgt.dev_id,
                                            pipe_tbl_hdl,
                                            sel_grp_hdl,
                                            &max_grp_size);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get size for Grp Id %d on pipe %x",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    return status;
  }

  // Query pipe mgr for member and status list
  uint32_t count = 0;
  status = pipeMgr->pipeMgrGetSelGrpMbrCount(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                             dev_tgt.dev_id,
                                             pipe_tbl_hdl,
                                             sel_grp_hdl,
                                             &count);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get info for Grp Id %d pipe %x",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    return status;
  }

  std::vector<pipe_adt_ent_hdl_t> pipe_members(count, 0);
  std::vector<char> pipe_member_status(count, 0);
  uint32_t mbrs_populated = 0;
  status = pipeMgr->pipeMgrSelGrpMbrsGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                         dev_tgt.dev_id,
                                         pipe_tbl_hdl,
                                         sel_grp_hdl,
                                         count,
                                         pipe_members.data(),
                                         (bool *)(pipe_member_status.data()),
                                         &mbrs_populated);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR Failed to get membership for Grp Id %d pipe %x",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              grp_id,
              dev_tgt.pipe_id);
    return status;
  }

  std::vector<tdi_id_t> member_ids;
  std::vector<bool> member_id_status;
  for (unsigned i = 0; i < mbrs_populated; i++) {
    tdi_id_t member_id = 0;
    status = getActMbrIdFromHndl(session, dev_tgt, pipe_members[i], &member_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE("%s:%d %s Error in getting member id for member hdl %d",
                __func__,
                __LINE__,
                tableInfoGet()->nameGet().c_str(),
                pipe_members[i]);
      return TDI_INVALID_ARG;
    }

    member_ids.push_back(member_id);
    member_id_status.push_back(pipe_member_status[i]);
  }
  sel_tbl_data->setMembers(member_ids);
  sel_tbl_data->setMemberStatus(member_id_status);
  sel_tbl_data->setMaxGrpSize(max_grp_size);
  return TDI_SUCCESS;
}

tdi_status_t SelectorTable::entryGet(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             const tdi::TableKey &key,
                                             tdi::TableData *data) const {
  tdi_id_t table_id_from_data;
  const Table *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (TDI_FLAG_IS_SET(flags, TDI_FROM_HW)) {
    LOG_WARN(
        "%s:%d %s : Table entry get from hardware not supported"
        " Defaulting to sw read",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
  }

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  const SelectorTableKey &sel_tbl_key =
      static_cast<const SelectorTableKey &>(key);
  SelectorTableData *sel_tbl_data =
      static_cast<SelectorTableData *>(data);
  tdi_id_t grp_id = sel_tbl_key.getGroupId();

  return entryGet_internal(session, dev_tgt, grp_id, sel_tbl_data);
}

tdi_status_t SelectorTable::entryKeyGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi_handle_t &entry_handle,
    tdi::Target *entry_tgt,
    tdi::TableKey *key) const {
  SelectorTableKey *sel_tbl_key = static_cast<SelectorTableKey *>(key);
  tdi_id_t grp_id;
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  auto status = pipeMgr->pipeMgrSelGrpIdGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                            pipe_dev_tgt,
                                            pipe_tbl_hdl,
                                            entry_handle,
                                            &grp_id);
  if (status != TDI_SUCCESS) return status;
  sel_tbl_key->setGroupId(grp_id);
  *entry_tgt = dev_tgt;
  return status;
}

tdi_status_t SelectorTable::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const SelectorTableKey &sel_tbl_key =
      static_cast<const SelectorTableKey &>(key);
  tdi_id_t sel_grp_id = sel_tbl_key.getGroupId();

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeMgrSelGrpHdlGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                      pipe_dev_tgt,
                                      pipe_tbl_hdl,
                                      sel_grp_id,
                                      entry_handle);
}

tdi_status_t SelectorTable::entryGet(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             const tdi_handle_t &entry_handle,
                                             tdi::TableKey *key,
                                             tdi::TableData *data) const {
  tdi::Target entry_tgt;
  tdi_status_t status = this->entryKeyGet(
      session, dev_tgt, flags, entry_handle, &entry_tgt, key);
  if (status != TDI_SUCCESS) {
    return status;
  }
  return this->entryGet(session, entry_tgt, flags, *key, data);
}

tdi_status_t SelectorTable::entryGetFirst(const tdi::Session &session,
                                                  const tdi::Target &dev_tgt,
                                                  const tdi::Flags &flags,
                                                  tdi::TableKey *key,
                                                  tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  tdi_id_t table_id_from_data;
  const Table *table_from_data;
  data->getParent(&table_from_data);
  table_from_data->tableIdGet(&table_id_from_data);

  if (TDI_FLAG_IS_SET(flags, TDI_FROM_HW)) {
    LOG_WARN(
        "%s:%d %s : Table entry get from hardware not supported."
        " Defaulting to sw read",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
  }

  if (table_id_from_data != this->tableInfoGet()->idGet()) {
    LOG_TRACE(
        "%s:%d %s ERROR : Table Data object with object id %d  does not match "
        "the table",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        table_id_from_data);
    return TDI_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_sel_grp_hdl_t sel_grp_hdl;
  status = pipeMgr->pipeMgrGetFirstEntryHandle(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_tbl_hdl, pipe_dev_tgt, (int *)&sel_grp_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : cannot get first handle, status %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  return this->entryGet(session, dev_tgt, flags, sel_grp_hdl, key, data);
}

tdi_status_t SelectorTable::entryGetNext_n(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  const SelectorTableKey &sel_tbl_key =
      static_cast<const SelectorTableKey &>(key);

  if (TDI_FLAG_IS_SET(flags, TDI_FROM_HW)) {
    LOG_WARN(
        "%s:%d %s : Table entry get from hardware not supported"
        " Defaulting to sw read",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str());
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_id_t sel_grp_id = sel_tbl_key.getGroupId();
  tdi_id_t pipe_entry_hdl;
  status = pipeMgr->pipeMgrSelGrpHdlGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        sel_grp_id,
                                        &pipe_entry_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d Grp Id %d does not exist for tbl 0x%x",
              __func__,
              __LINE__,
              sel_grp_id,
              tableInfoGet()->idGet());
    return TDI_OBJECT_NOT_FOUND;
  }

  std::vector<int> next_entry_handles(n, 0);
  status = pipeMgr->pipeMgrGetNextEntryHandles(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                               pipe_tbl_hdl,
                                               pipe_dev_tgt,
                                               pipe_entry_hdl,
                                               n,
                                               next_entry_handles.data());
  if (status == TDI_OBJECT_NOT_FOUND) {
    if (num_returned) {
      *num_returned = 0;
    }
    return TDI_SUCCESS;
  }

  unsigned i = 0;
  for (i = 0; i < n; i++) {
    if (next_entry_handles[i] == -1) {
      break;
    }
    auto this_key = static_cast<TableKey *>((*key_data_pairs)[i].first);
    auto this_data = static_cast<tdi::TableData *>((*key_data_pairs)[i].second);
    tdi_id_t table_id_from_data;
    const Table *table_from_data;
    this_data->getParent(&table_from_data);
    table_from_data->tableIdGet(&table_id_from_data);

    if (table_id_from_data != this->tableInfoGet()->idGet()) {
      LOG_TRACE(
          "%s:%d %s ERROR : Table Data object with object id %d does not match "
          "the table",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          table_id_from_data);
      return TDI_INVALID_ARG;
    }
    status = entryGet(
        session, dev_tgt, flags, next_entry_handles[i], this_key, this_data);
    if (status != TDI_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR in getting %dth entry from pipe-mgr with group "
          "handle %d, err %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          i + 1,
          next_entry_handles[i],
          status);
      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
  }
  if (num_returned) {
    *num_returned = i;
  }
  return TDI_SUCCESS;
}

tdi_status_t SelectorTable::getOneMbr(const tdi::Session &session,
                                         const uint16_t device_id,
                                         const pipe_sel_grp_hdl_t sel_grp_hdl,
                                         pipe_adt_ent_hdl_t *member_hdl) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeMgrGetFirstGroupMember(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                             pipe_tbl_hdl,
                                             device_id,
                                             sel_grp_hdl,
                                             member_hdl);
}

tdi_status_t SelectorTable::getGrpIdFromHndl(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const pipe_sel_grp_hdl_t &sel_grp_hdl,
    tdi_id_t *sel_grp_id) const {
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeMgrSelGrpIdGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     sel_grp_hdl,
                                     sel_grp_id);
}

tdi_status_t SelectorTable::getGrpHdl(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi_id_t sel_grp_id,
    pipe_sel_grp_hdl_t *sel_grp_hdl) const {
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance();
  return pipeMgr->pipeMgrSelGrpHdlGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                      pipe_dev_tgt,
                                      pipe_tbl_hdl,
                                      sel_grp_id,
                                      sel_grp_hdl);
}

tdi_status_t SelectorTable::tableUsageGet(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             uint32_t *count) const {
  return getTableUsage(session, dev_tgt, flags, *(this), count);
}

tdi_status_t SelectorTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new SelectorTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SelectorTable::keyReset(TableKey *key) const {
  SelectorTableKey *sel_key = static_cast<SelectorTableKey *>(key);
  return key_reset<SelectorTable, SelectorTableKey>(*this, sel_key);
}

tdi_status_t SelectorTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  const std::vector<tdi_id_t> fields{};
  *data_ret =
      std::unique_ptr<tdi::TableData>(new SelectorTableData(this, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error in allocating data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str())
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SelectorTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new SelectorTableData(this, fields));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error in allocating data",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str())
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SelectorTable::dataReset(tdi::TableData *data) const {
  SelectorTableData *sel_data = static_cast<SelectorTableData *>(data);
  if (!this->validateTable_from_dataObj(*sel_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return sel_data->reset();
}

tdi_status_t SelectorTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  if (type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s ERROR Invalid Attribute type (%d)"
        "set "
        "attributes",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        static_cast<int>(type));
    return TDI_INVALID_ARG;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new TableAttributesImpl(this, type));
  return TDI_SUCCESS;
}

tdi_status_t SelectorTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<TableAttributesImpl &>(*(attr->get()));
  if (type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s ERROR Invalid Attribute type (%d)"
        "set "
        "attributes",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        static_cast<int>(type));
    return TDI_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

tdi_status_t SelectorTable::processSelUpdateCbAttr(
    const TableAttributesImpl &tbl_attr_impl,
    const tdi::Target &dev_tgt) const {
  // 1. From the table attribute object, get the selector update parameters.
  // 2. Make a table attribute state object to store the parameters that are
  // required for when the callback is invoked.
  // 3. Invoke pipe-mgr callback registration function to register TDI
  // internal callback.
  auto t = tbl_attr_impl.selectorUpdateCbInternalGet();

  auto enable = std::get<0>(t);
  auto session = std::get<1>(t);
  auto cpp_callback_fn = std::get<2>(t);
  auto c_callback_fn = std::get<3>(t);
  auto cookie = std::get<4>(t);

  auto session_obj = session.lock();

  if (session_obj == nullptr) {
    LOG_TRACE("%s:%d %s ERROR Session object passed no longer exists",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  auto *pipeMgr = PipeMgrIntf::getInstance(*session_obj);
  tdi_status_t status = pipeMgr->pipeMgrSelTblRegisterCb(
      session_obj->handleGet(),
      dev_tgt.dev_id,
      pipe_tbl_hdl,
      selUpdatePipeMgrInternalCb,
      &(attributes_state->getSelUpdateCbObj()));

  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in registering selector update callback with pipe-mgr, "
        "err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        status);
    // Reset the selector update callback object, since pipeMgr registration did
    // not succeed
    attributes_state->resetSelUpdateCbObj();
    return status;
  }
  return TDI_SUCCESS;
}

tdi_status_t SelectorTable::tableAttributesSet(
    const tdi::Session & /*session*/,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const TableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const TableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();

  if (attr_type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
        "set "
        "attributes",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        static_cast<int>(attr_type));
    return TDI_INVALID_ARG;
  }
  return this->processSelUpdateCbAttr(*tbl_attr_impl, dev_tgt);
}

tdi_status_t SelectorTable::tableAttributesGet(
    const tdi::Session & /*session */,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    TableAttributes *tableAttributes) const {
  // 1. From the table attribute state, retrieve all the params that were
  // registered by the user
  // 2. Set the params in the passed in tableAttributes obj

  auto tbl_attr_impl = static_cast<TableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  if (attr_type != TableAttributesType::SELECTOR_UPDATE_CALLBACK) {
    LOG_TRACE(
        "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
        "set "
        "attributes",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        static_cast<int>(attr_type));
    return TDI_INVALID_ARG;
  }

  return TDI_SUCCESS;
}
// COUNTER TABLE APIS

tdi_status_t CounterTable::entryAdd(const tdi::Session &session,
                                            const tdi::Target &dev_tgt,
                                            const uint64_t & /*flags*/,
                                            const tdi::TableKey &key,
                                            const tdi::TableData &data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const CounterTableKey &cntr_key =
      static_cast<const CounterTableKey &>(key);
  const CounterTableData &cntr_data =
      static_cast<const CounterTableData &>(data);

  uint32_t counter_id = cntr_key.getCounterId();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, counter_id)) {
    return TDI_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  const pipe_stat_data_t *stat_data =
      cntr_data.getCounterSpecObj().getPipeCounterSpec();
  status =
      pipeMgr->pipeMgrStatEntSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                 pipe_dev_tgt,
                                 pipe_tbl_hdl,
                                 counter_id,
                                 const_cast<pipe_stat_data_t *>(stat_data));

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in adding/modifying counter index %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              counter_id,
              status);
    return status;
  }
  return TDI_SUCCESS;
}

tdi_status_t CounterTable::entryMod(const tdi::Session &session,
                                            const tdi::Target &dev_tgt,
                                            const tdi::Flags &flags,
                                            const tdi::TableKey &key,
                                            const tdi::TableData &data) const {
  return entryAdd(session, dev_tgt, flags, key, data);
}

tdi_status_t CounterTable::entryGet(const tdi::Session &session,
                                            const tdi::Target &dev_tgt,
                                            const tdi::Flags &flags,
                                            const tdi::TableKey &key,
                                            tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const CounterTableKey &cntr_key =
      static_cast<const CounterTableKey &>(key);
  CounterTableData *cntr_data = static_cast<CounterTableData *>(data);

  uint32_t counter_id = cntr_key.getCounterId();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, counter_id)) {
    return TDI_INVALID_ARG;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  if (TDI_FLAG_IS_SET(flags, TDI_FROM_HW)) {
    status = pipeMgr->pipeMgrStatEntDatabaseSync(
        session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, pipe_tbl_hdl, counter_id);
    if (status != TDI_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s ERROR in getting counter value from hardware for counter "
          "idx %d, err %d",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          counter_id,
          status);
      return status;
    }
  }

  pipe_stat_data_t stat_data = {0};
  status = pipeMgr->pipeMgrStatEntQuery(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                        pipe_dev_tgt,
                                        pipe_tbl_hdl,
                                        counter_id,
                                        &stat_data);

  if (status != TDI_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s ERROR in reading counter value for counter idx %d, err %d",
        __func__,
        __LINE__,
        tableInfoGet()->nameGet().c_str(),
        counter_id,
        status);
    return status;
  }

  cntr_data->getCounterSpecObj().setCounterDataFromCounterSpec(stat_data);

  return TDI_SUCCESS;
}

tdi_status_t CounterTable::entryGet(const tdi::Session &session,
                                            const tdi::Target &dev_tgt,
                                            const tdi::Flags &flags,
                                            const tdi_handle_t &entry_handle,
                                            tdi::TableKey *key,
                                            tdi::TableData *data) const {
  CounterTableKey *cntr_key = static_cast<CounterTableKey *>(key);
  cntr_key->setCounterId(entry_handle);
  return this->entryGet(
      session, dev_tgt, flags, static_cast<const tdi::TableKey &>(*key), data);
}

tdi_status_t CounterTable::entryKeyGet(
    const tdi::Session & /*session*/,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi_handle_t &entry_handle,
    tdi::Target *entry_tgt,
    tdi::TableKey *key) const {
  CounterTableKey *cntr_key = static_cast<CounterTableKey *>(key);
  cntr_key->setCounterId(entry_handle);
  *entry_tgt = dev_tgt;
  return TDI_SUCCESS;
}

tdi_status_t CounterTable::entryHandleGet(
    const tdi::Session & /*session*/,
    const tdi::Target & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const CounterTableKey &cntr_key =
      static_cast<const CounterTableKey &>(key);
  *entry_handle = cntr_key.getCounterId();
  return TDI_SUCCESS;
}

tdi_status_t CounterTable::entryGetFirst(const tdi::Session &session,
                                                 const tdi::Target &dev_tgt,
                                                 const tdi::Flags &flags,
                                                 tdi::TableKey *key,
                                                 tdi::TableData *data) const {
  CounterTableKey *cntr_key = static_cast<CounterTableKey *>(key);
  return getFirst_for_resource_tbls<CounterTable, CounterTableKey>(
      *this, session, dev_tgt, flags, cntr_key, data);
}

tdi_status_t CounterTable::entryGetNext_n(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const CounterTableKey &cntr_key =
      static_cast<const CounterTableKey &>(key);
  return getNext_n_for_resource_tbls<CounterTable, CounterTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      cntr_key,
      n,
      key_data_pairs,
      num_returned);
}

tdi_status_t CounterTable::tableClear(const tdi::Session &session,
                                         const tdi::Target &dev_tgt,
                                         const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_status_t status = pipeMgr->pipeMgrStatTableReset(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, pipe_tbl_hdl, nullptr);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing counter table err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  return status;
}

tdi_status_t CounterTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new CounterTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t CounterTable::keyReset(TableKey *key) const {
  CounterTableKey *counter_key = static_cast<CounterTableKey *>(key);
  return key_reset<CounterTable, CounterTableKey>(*this, counter_key);
}

tdi_status_t CounterTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(new CounterTableData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t CounterTable::dataReset(tdi::TableData *data) const {
  CounterTableData *counter_data =
      static_cast<CounterTableData *>(data);
  if (!this->validateTable_from_dataObj(*counter_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return counter_data->reset();
}

// METER TABLE

tdi_status_t MeterTable::entryAdd(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const uint64_t & /*flags*/,
                                          const tdi::TableKey &key,
                                          const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MeterTableKey &meter_key =
      static_cast<const MeterTableKey &>(key);
  const MeterTableData &meter_data =
      static_cast<const MeterTableData &>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  const pipe_meter_spec_t *meter_spec =
      meter_data.getMeterSpecObj().getPipeMeterSpec();
  pipe_meter_idx_t meter_idx = meter_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, meter_idx)) {
    return TDI_INVALID_ARG;
  }

  return pipeMgr->pipeMgrMeterEntSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                     pipe_dev_tgt,
                                     pipe_tbl_hdl,
                                     meter_idx,
                                     (pipe_meter_spec_t *)meter_spec,
                                     0 /* Pipe API flags */);
  return TDI_SUCCESS;
}

tdi_status_t MeterTable::entryMod(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags &flags,
                                          const tdi::TableKey &key,
                                          const tdi::TableData &data) const {
  return entryAdd(session, dev_tgt, flags, key, data);
}

tdi_status_t MeterTable::entryGet(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags &flags,
                                          const tdi::TableKey &key,
                                          tdi::TableData *data) const {
  bool from_hw = false;

  if (TDI_FLAG_IS_SET(flags, TDI_FROM_HW)) {
    from_hw = true;
  }

  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const MeterTableKey &meter_key =
      static_cast<const MeterTableKey &>(key);
  MeterTableData *meter_data = static_cast<MeterTableData *>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_meter_spec_t meter_spec;
  std::memset(&meter_spec, 0, sizeof(meter_spec));

  pipe_meter_idx_t meter_idx = meter_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, meter_idx)) {
    return TDI_INVALID_ARG;
  }

  status = pipeMgr->pipeMgrMeterReadEntryIdx(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                             pipe_dev_tgt,
                                             pipe_tbl_hdl,
                                             meter_idx,
                                             &meter_spec,
                                             from_hw);

  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR reading meter entry idx %d, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              meter_idx,
              status);
    return status;
  }

  // Populate data elements right here
  meter_data->getMeterSpecObj().setMeterDataFromMeterSpec(meter_spec);
  return TDI_SUCCESS;
}

tdi_status_t MeterTable::entryGet(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const tdi::Flags &flags,
                                          const tdi_handle_t &entry_handle,
                                          tdi::TableKey *key,
                                          tdi::TableData *data) const {
  MeterTableKey *mtr_key = static_cast<MeterTableKey *>(key);
  mtr_key->setIdxKey(entry_handle);
  return this->entryGet(
      session, dev_tgt, flags, static_cast<const tdi::TableKey &>(*key), data);
}

tdi_status_t MeterTable::entryKeyGet(const tdi::Session & /*session*/,
                                             const tdi::Target &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const tdi_handle_t &entry_handle,
                                             tdi::Target *entry_tgt,
                                             tdi::TableKey *key) const {
  MeterTableKey *mtr_key = static_cast<MeterTableKey *>(key);
  mtr_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return TDI_SUCCESS;
}

tdi_status_t MeterTable::entryHandleGet(
    const tdi::Session & /*session*/,
    const tdi::Target & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const MeterTableKey &mtr_key =
      static_cast<const MeterTableKey &>(key);
  *entry_handle = mtr_key.getIdxKey();
  return TDI_SUCCESS;
}

tdi_status_t MeterTable::entryGetFirst(const tdi::Session &session,
                                               const tdi::Target &dev_tgt,
                                               const tdi::Flags &flags,
                                               tdi::TableKey *key,
                                               tdi::TableData *data) const {
  MeterTableKey *meter_key = static_cast<MeterTableKey *>(key);
  return getFirst_for_resource_tbls<MeterTable, MeterTableKey>(
      *this, session, dev_tgt, flags, meter_key, data);
}

tdi_status_t MeterTable::entryGetNext_n(const tdi::Session &session,
                                                const tdi::Target &dev_tgt,
                                                const tdi::Flags &flags,
                                                const tdi::TableKey &key,
                                                const uint32_t &n,
                                                tdi::Table::keyDataPairs *key_data_pairs,
                                                uint32_t *num_returned) const {
  const MeterTableKey &meter_key =
      static_cast<const MeterTableKey &>(key);
  return getNext_n_for_resource_tbls<MeterTable, MeterTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      meter_key,
      n,
      key_data_pairs,
      num_returned);
}

tdi_status_t MeterTable::tableClear(const tdi::Session &session,
                                       const tdi::Target &dev_tgt,
                                       const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_status_t status = pipeMgr->pipeMgrMeterReset(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                                  pipe_dev_tgt,
                                                  this->pipe_tbl_hdl,
                                                  0 /* Pipe API flags */);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in CLearing Meter table, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  return status;
}

tdi_status_t MeterTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new MeterTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MeterTable::keyReset(TableKey *key) const {
  MeterTableKey *meter_key = static_cast<MeterTableKey *>(key);
  return key_reset<MeterTable, MeterTableKey>(*this, meter_key);
}

tdi_status_t MeterTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(new MeterTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t MeterTable::dataReset(tdi::TableData *data) const {
  MeterTableData *meter_data = static_cast<MeterTableData *>(data);
  if (!this->validateTable_from_dataObj(*meter_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return meter_data->reset();
}

tdi_status_t MeterTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != TDI_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new TableAttributesImpl(this, type));
  return TDI_SUCCESS;
}

tdi_status_t MeterTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<TableAttributesImpl &>(*(attr->get()));
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != TDI_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Unable to reset attribute",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

tdi_status_t MeterTable::tableAttributesSet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const TableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const TableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      tdi_status_t sts = tbl_attr_impl->meterByteCountAdjGet(&byte_count);
      if (sts != TDI_SUCCESS) {
        return sts;
      }
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      return pipeMgr->pipeMgrMeterByteCountSet(
          session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, pipe_tbl_hdl, byte_count);
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t MeterTable::tableAttributesGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    TableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<TableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::METER_BYTE_COUNT_ADJ: {
      int byte_count;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      dev_target_t pipe_dev_tgt;
      pipe_dev_tgt.device_id = dev_tgt.dev_id;
      pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
      pipeMgr->pipeMgrMeterByteCountGet(
          session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, pipe_tbl_hdl, &byte_count);
      return tbl_attr_impl->meterByteCountAdjSet(byte_count);
    }
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

// REGISTER TABLE
tdi_status_t RegisterTable::entryAdd(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const uint64_t & /*flags*/,
                                             const tdi::TableKey &key,
                                             const tdi::TableData &data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const RegisterTableKey &register_key =
      static_cast<const RegisterTableKey &>(key);
  const RegisterTableData &register_data =
      static_cast<const RegisterTableData &>(data);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;
  pipe_stful_mem_idx_t register_idx = register_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, register_idx)) {
    return TDI_INVALID_ARG;
  }

  pipe_stful_mem_spec_t stful_spec;
  std::memset(&stful_spec, 0, sizeof(stful_spec));

  std::vector<tdi_id_t> dataFields;
  tdi_status_t status = dataFieldIdListGet(&dataFields);
  TDI_ASSERT(status == TDI_SUCCESS);
  const auto &register_spec_data = register_data.getRegisterSpecObj();
  const tdi::DataFieldInfo *tableDataField;
  status = dataFieldGet(dataFields[0], &tableDataField);
  TDI_ASSERT(status == TDI_SUCCESS);
  register_spec_data.populateStfulSpecFromData(&stful_spec);

  return pipeMgr->pipeStfulEntSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                  pipe_dev_tgt,
                                  pipe_tbl_hdl,
                                  register_idx,
                                  &stful_spec,
                                  0 /* Pipe API flags */);
}

tdi_status_t RegisterTable::entryMod(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             const tdi::TableKey &key,
                                             const tdi::TableData &data) const {
  return entryAdd(session, dev_tgt, flags, key, data);
}

tdi_status_t RegisterTable::entryGet(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             const tdi::TableKey &key,
                                             tdi::TableData *data) const {
  tdi_status_t status = TDI_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  const RegisterTableKey &register_key =
      static_cast<const RegisterTableKey &>(key);
  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  pipe_stful_mem_spec_t stful_spec;
  std::memset(&stful_spec, 0, sizeof(stful_spec));

  // Query number of pipes to get possible number of results.
  int num_pipes = 0;
  status = pipeMgr->pipeStfulQueryGetSizes(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), dev_tgt.dev_id, pipe_tbl_hdl, &num_pipes);

  // Use vectors to populate pipe mgr stful query data structure.
  // One vector to hold all possible pipe data.
  std::vector<pipe_stful_mem_spec_t> register_pipe_data(num_pipes);
  pipe_stful_mem_query_t stful_query;
  stful_query.data = register_pipe_data.data();
  stful_query.pipe_count = num_pipes;

  uint32_t pipe_api_flags = 0;
  if (TDI_FLAG_IS_SET(flags, TDI_FROM_HW)) {
    pipe_api_flags = PIPE_FLAG_SYNC_REQ;
  }
  pipe_stful_mem_idx_t register_idx = register_key.getIdxKey();

  if (!verify_key_for_idx_tbls(session, dev_tgt, *this, register_idx)) {
    return TDI_INVALID_ARG;
  }

  status = pipeMgr->pipeStfulEntQuery(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                      pipe_dev_tgt,
                                      pipe_tbl_hdl,
                                      register_idx,
                                      &stful_query,
                                      pipe_api_flags);

  if (status == TDI_SUCCESS) {
    std::vector<tdi_id_t> dataFields;
    status = dataFieldIdListGet(&dataFields);
    TDI_ASSERT(status == TDI_SUCCESS);
    // Down cast to RegisterTableData
    RegisterTableData *register_data =
        static_cast<RegisterTableData *>(data);
    auto &register_spec_data = register_data->getRegisterSpecObj();
    const tdi::DataFieldInfo *tableDataField;
    status = dataFieldGet(dataFields[0], &tableDataField);
    TDI_ASSERT(status == TDI_SUCCESS);
    // pipe_count is returned upon successful query,
    // hence use it instead of vector size.
    register_spec_data.populateDataFromStfulSpec(
        register_pipe_data, static_cast<uint32_t>(stful_query.pipe_count));
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::entryGet(const tdi::Session &session,
                                             const tdi::Target &dev_tgt,
                                             const tdi::Flags &flags,
                                             const tdi_handle_t &entry_handle,
                                             tdi::TableKey *key,
                                             tdi::TableData *data) const {
  RegisterTableKey *reg_key = static_cast<RegisterTableKey *>(key);
  reg_key->setIdxKey(entry_handle);
  return this->entryGet(
      session, dev_tgt, flags, static_cast<const tdi::TableKey &>(*key), data);
}

tdi_status_t RegisterTable::entryKeyGet(
    const tdi::Session & /*session*/,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi_handle_t &entry_handle,
    tdi::Target *entry_tgt,
    tdi::TableKey *key) const {
  RegisterTableKey *reg_key = static_cast<RegisterTableKey *>(key);
  reg_key->setIdxKey(entry_handle);
  *entry_tgt = dev_tgt;
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::entryHandleGet(
    const tdi::Session & /*session*/,
    const tdi::Target & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  const RegisterTableKey &reg_key =
      static_cast<const RegisterTableKey &>(key);
  *entry_handle = reg_key.getIdxKey();
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::entryGetFirst(const tdi::Session &session,
                                                  const tdi::Target &dev_tgt,
                                                  const tdi::Flags &flags,
                                                  tdi::TableKey *key,
                                                  tdi::TableData *data) const {
  RegisterTableKey *register_key = static_cast<RegisterTableKey *>(key);
  return getFirst_for_resource_tbls<RegisterTable, RegisterTableKey>(
      *this, session, dev_tgt, flags, register_key, data);
}

tdi_status_t RegisterTable::entryGetNext_n(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const RegisterTableKey &register_key =
      static_cast<const RegisterTableKey &>(key);
  return getNext_n_for_resource_tbls<RegisterTable, RegisterTableKey>(
      *this,
      session,
      dev_tgt,
      flags,
      register_key,
      n,
      key_data_pairs,
      num_returned);
}

tdi_status_t RegisterTable::tableClear(const tdi::Session &session,
                                          const tdi::Target &dev_tgt,
                                          const uint64_t & /*flags*/) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  tdi_status_t status = pipeMgr->pipeStfulTableReset(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)), pipe_dev_tgt, pipe_tbl_hdl, nullptr);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in Clearing register table, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }
  return status;
}

tdi_status_t RegisterTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new RegisterTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s ERROR : Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::keyReset(TableKey *key) const {
  RegisterTableKey *register_key = static_cast<RegisterTableKey *>(key);
  return key_reset<RegisterTable, RegisterTableKey>(*this,
                                                            register_key);
}

tdi_status_t RegisterTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret = std::unique_ptr<tdi::TableData>(new RegisterTableData(this));
  if (*data_ret == nullptr) {
    LOG_TRACE("%s:%d %s ERROR : Memory allocation failed",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::dataReset(tdi::TableData *data) const {
  RegisterTableData *register_data =
      static_cast<RegisterTableData *>(data);
  if (!this->validateTable_from_dataObj(*register_data)) {
    LOG_TRACE("%s:%d %s ERROR : Data object is not associated with the table",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  return register_data->reset();
}

tdi_status_t RegisterTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != TDI_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Attribute %d is not supported",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              static_cast<int>(type));
    return TDI_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<TableAttributes>(
      new TableAttributesImpl(this, type));
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<TableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<TableAttributesImpl &>(*(attr->get()));
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != TDI_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_TRACE("%s:%d %s Unable to reset attribute",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return status;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

tdi_status_t RegisterTable::tableAttributesSet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const TableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const TableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      TableEntryScope entry_scope;
      TableEntryScopeArgumentsImpl scope_args(0);
      tdi_status_t sts = tbl_attr_impl->entryScopeParamsGet(
          &entry_scope,
          static_cast<TableEntryScopeArguments *>(&scope_args));
      if (sts != TDI_SUCCESS) {
        LOG_TRACE("%s:%d %s Unable to get the entry scope params",
                  __func__,
                  __LINE__,
                  tableInfoGet()->nameGet().c_str());
        return sts;
      }
      // Call the pipe mgr API to set entry scope
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      // Derive pipe mgr entry scope from TDI entry scope and set it to
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
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // We call the pipe mgr tbl property API on the compiler generated
      // table
      return pipeMgr->pipeMgrTblSetProperty(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                            dev_tgt.dev_id,
                                            ghost_pipe_tbl_hdl_,
                                            prop_type,
                                            prop_val,
                                            args_val);
    }
    case TableAttributesType::DYNAMIC_KEY_MASK:
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
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::tableAttributesGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    TableAttributes *tableAttributes) const {
  // Check for out param memory
  if (!tableAttributes) {
    LOG_TRACE("%s:%d %s Please pass in the tableAttributes",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  auto tbl_attr_impl = static_cast<TableAttributesImpl *>(tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  switch (attr_type) {
    case TableAttributesType::ENTRY_SCOPE: {
      pipe_mgr_tbl_prop_type_t prop_type = PIPE_MGR_TABLE_ENTRY_SCOPE;
      pipe_mgr_tbl_prop_value_t prop_val;
      pipe_mgr_tbl_prop_args_t args_val;
      auto *pipeMgr = PipeMgrIntf::getInstance(session);
      // We call the pipe mgr tbl property API on the compiler generated
      // table
      auto sts = pipeMgr->pipeMgrTblGetProperty(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
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
            tableInfoGet()->nameGet().c_str(),
            tableInfoGet()->nameGet().c_str());
        return sts;
      }

      TableEntryScope entry_scope;
      TableEntryScopeArgumentsImpl scope_args(args_val.value);

      // Derive TDI entry scope from pipe mgr entry scope
      entry_scope = prop_val.value == PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES
                        ? TableEntryScope::ENTRY_SCOPE_ALL_PIPELINES
                        : prop_val.value == PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE
                              ? TableEntryScope::ENTRY_SCOPE_SINGLE_PIPELINE
                              : TableEntryScope::ENTRY_SCOPE_USER_DEFINED;

      return tbl_attr_impl->entryScopeParamsSet(
          entry_scope, static_cast<TableEntryScopeArguments &>(scope_args));
    }
    case TableAttributesType::DYNAMIC_KEY_MASK:
    case TableAttributesType::IDLE_TABLE_RUNTIME:
    case TableAttributesType::METER_BYTE_COUNT_ADJ:
    default:
      LOG_TRACE(
          "%s:%d %s Invalid Attribute type (%d) encountered while trying to "
          "set "
          "attributes",
          __func__,
          __LINE__,
          tableInfoGet()->nameGet().c_str(),
          static_cast<int>(attr_type));
      TDI_DBGCHK(0);
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterTable::ghostTableHandleSet(
    const pipe_tbl_hdl_t &pipe_hdl) {
  ghost_pipe_tbl_hdl_ = pipe_hdl;
  return TDI_SUCCESS;
}

// SelectorGetMemberTable****************
tdi_status_t SelectorGetMemberTable::getRef(
    pipe_sel_tbl_hdl_t *sel_tbl_hdl,
    const SelectorTable **sel_tbl,
    const ActionTable **act_tbl) const {
  auto ref_map = this->getTableRefMap();
  auto it = ref_map.find("other");
  if (it == ref_map.end()) {
    LOG_ERROR(
        "Cannot find sel ref. Missing required sel table reference "
        "in tdi.json");
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
  }
  if (it->second.size() != 1) {
    LOG_ERROR(
        "Invalid tdi.json configuration. SelGetMem should be part"
        " of exactly one Selector table.");
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
  }
  *sel_tbl_hdl = it->second.back().tbl_hdl;
  auto sel_tbl_id = it->second.back().id;

  // Get the Sel table
  const Table *tbl;
  auto status = this->tdiInfoGet()->tdiTableFromIdGet(sel_tbl_id, &tbl);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }
  auto tbl_obj = static_cast<const tdi::Table *>(tbl);
  *sel_tbl = static_cast<const SelectorTable *>(tbl_obj);

  // Get action profile table
  status =
      this->tdiInfoGet()->tdiTableFromIdGet(tbl_obj->getActProfId(), &tbl);
  if (status) {
    TDI_DBGCHK(0);
    return status;
  }
  tbl_obj = static_cast<const tdi::Table *>(tbl);
  *act_tbl = static_cast<const ActionTable *>(tbl_obj);
  return TDI_SUCCESS;
}

tdi_status_t SelectorGetMemberTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  if (key_ret == nullptr) {
    return TDI_INVALID_ARG;
  }
  *key_ret =
      std::unique_ptr<TableKey>(new SelectorGetMemberTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SelectorGetMemberTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  if (data_ret == nullptr) {
    return TDI_INVALID_ARG;
  }
  std::vector<tdi_id_t> fields{};
  *data_ret = std::unique_ptr<tdi::TableData>(
      new SelectorGetMemberTableData(this, 0, fields));

  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t SelectorGetMemberTable::entryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi::TableKey &key,
    tdi::TableData *data) const {
  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  tdi_status_t status = TDI_SUCCESS;
  if (data == nullptr) {
    return TDI_INVALID_ARG;
  }

  const SelectorGetMemberTableKey &sel_key =
      static_cast<const SelectorGetMemberTableKey &>(key);
  SelectorGetMemberTableData *sel_data =
      static_cast<SelectorGetMemberTableData *>(data);

  uint64_t grp_id = 0;
  tdi_id_t field_id;
  status = this->keyFieldIdGet("$SELECTOR_GROUP_ID", &field_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR fail to get field ID for grp ID",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = sel_key.getValue(field_id, &grp_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to grp_id",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  uint64_t hash = 0;
  status = this->keyFieldIdGet("hash_value", &field_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for hash_value",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = sel_key.getValue(field_id, &hash);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get hash",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }

  const SelectorTable *sel_tbl;
  const ActionTable *act_tbl;
  pipe_sel_tbl_hdl_t sel_tbl_hdl;
  status = this->getRef(&sel_tbl_hdl, &sel_tbl, &act_tbl);
  if (status) {
    return status;
  }

  pipe_sel_grp_hdl_t grp_hdl;
  status = sel_tbl->getGrpHdl(session, dev_tgt, grp_id, &grp_hdl);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s ERROR : in getting entry handle, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
    return status;
  }

  uint32_t num_bytes = sizeof(uint64_t);
  std::vector<uint8_t> hash_arr(num_bytes, 0);
  EndiannessHandler::toNetworkOrder(num_bytes, hash, hash_arr.data());

  pipe_adt_ent_hdl_t adt_ent_hdl = 0;
  status = pipeMgr->pipeMgrSelGrpMbrGetFromHash(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
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
        this->tableInfoGet()->nameGet().c_str(),
        hash,
        grp_hdl);
    return status;
  }
  uint32_t act_mbr_id = 0;
  status =
      act_tbl->getMbrIdFromHndl(session, dev_tgt, adt_ent_hdl, &act_mbr_id);
  if (status) {
    LOG_ERROR("%s:%d %s : Error : Unable to get mbr ID for hdl %d",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str(),
              adt_ent_hdl);
    return status;
  }

  status = this->dataFieldIdGet("$ACTION_MEMBER_ID", &field_id);
  if (status != TDI_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get the field id for hash_value",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    return status;
  }
  status = sel_data->setValue(field_id, act_mbr_id);
  return status;
}

// Register param table
// Table reuses debug counter table data object.
tdi_status_t RegisterParamTable::getRef(pipe_tbl_hdl_t *hdl) const {
  auto ref_map = this->getTableRefMap();
  auto it = ref_map.find("other");
  if (it == ref_map.end()) {
    LOG_ERROR(
        "Cannot set register param. Missing required register table reference "
        "in tdi.json.");
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
  }
  if (it->second.size() != 1) {
    LOG_ERROR(
        "Invalid tdi.json configuration. Register param should be part"
        " of exactly one register table.");
    TDI_DBGCHK(0);
    return TDI_UNEXPECTED;
  }
  *hdl = it->second[0].tbl_hdl;
  return TDI_SUCCESS;
}

tdi_status_t RegisterParamTable::getParamHdl(const tdi::Session &session,
                                                tdi_dev_id_t dev_id) const {
  pipe_reg_param_hdl_t reg_param_hdl;
  // Get param name from table name. Have to remove pipe name.
  std::string param_name = this->tableInfoGet()->nameGet();
  param_name = param_name.erase(0, param_name.find(".") + 1);

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  auto status =
      pipeMgr->pipeStfulParamGetHdl(dev_id, param_name.c_str(), &reg_param_hdl);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to get register param handle from pipe mgr",
              __func__,
              __LINE__,
              this->tableInfoGet()->nameGet().c_str());
    TDI_DBGCHK(0);
    return 0;
  }

  return reg_param_hdl;
}

tdi_status_t RegisterParamTable::tableDefaultEntrySet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    const tdi::TableData &data) const {
  const RegisterParamTableData &mdata =
      static_cast<const RegisterParamTableData &>(data);
  pipe_tbl_hdl_t tbl_hdl;
  auto status = this->getRef(&tbl_hdl);
  if (status) {
    return status;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeStfulParamSet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                    pipe_dev_tgt,
                                    tbl_hdl,
                                    this->getParamHdl(session, dev_tgt.dev_id),
                                    mdata.value);
}

tdi_status_t RegisterParamTable::tableDefaultEntryGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/,
    tdi::TableData *data) const {
  pipe_tbl_hdl_t tbl_hdl;
  auto status = this->getRef(&tbl_hdl);
  if (status) {
    return status;
  }
  RegisterParamTableData *mdata =
      static_cast<RegisterParamTableData *>(data);

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeStfulParamGet(session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
                                    pipe_dev_tgt,
                                    tbl_hdl,
                                    this->getParamHdl(session, dev_tgt.dev_id),
                                    &mdata->value);
}

tdi_status_t RegisterParamTable::tableDefaultEntryReset(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const uint64_t & /*flags*/) const {
  pipe_tbl_hdl_t tbl_hdl;
  auto status = this->getRef(&tbl_hdl);
  if (status) {
    return status;
  }

  dev_target_t pipe_dev_tgt;
  pipe_dev_tgt.device_id = dev_tgt.dev_id;
  pipe_dev_tgt.dev_pipe_id = dev_tgt.pipe_id;

  auto *pipeMgr = PipeMgrIntf::getInstance(session);
  return pipeMgr->pipeStfulParamReset(
      session.handleGet(static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
      pipe_dev_tgt,
      tbl_hdl,
      this->getParamHdl(session, dev_tgt.dev_id));
}

tdi_status_t RegisterParamTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<tdi::TableData>(new RegisterParamTableData(this));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}
#endif

}  // namespace rt
}  // namespace pna
}  // namespace tdi

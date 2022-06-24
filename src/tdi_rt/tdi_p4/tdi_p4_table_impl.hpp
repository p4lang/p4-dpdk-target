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
#ifndef _TDI_P4_TABLE_OBJ_HPP
#define _TDI_P4_TABLE_OBJ_HPP

// tdi includes
#include <tdi/common/tdi_table.hpp>

//#include "tdi_table_state.hpp"
//#include "tdi_p4_table_key_impl.hpp"
#include <tdi_common/tdi_pipe_mgr_intf.hpp>
#include "tdi_p4_table_data_impl.hpp"

namespace tdi {
namespace pna {
namespace rt {

/*
 * MatchActionDirect
 * MatchActionIndirectTable
 * ActionTable
 * Selector
 * CounterIndirect
 * MeterTable
 * RegisterTable
 * RegisterParamTable
 */

class MatchActionDirect : public tdi::Table {
 public:
  MatchActionDirect(const tdi::TdiInfo *tdi_info,
                    const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_ADD, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DELETE, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
#if 0
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_SET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_RESET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
#endif
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_USAGE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_SIZE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  }

  virtual tdi_status_t entryAdd(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      const tdi::TableData &data) const override final;
  virtual tdi_status_t entryMod(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryDel(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key) const override;

  virtual tdi_status_t clear(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags) const override;

  virtual tdi_status_t defaultEntrySet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableData &data) const override;

#if 0
  virtual tdi_status_t defaultEntryMod(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableData &data) const override;
#endif

  virtual tdi_status_t defaultEntryReset(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags) const override;

  virtual tdi_status_t defaultEntryGet(const tdi::Session &session,
                                       const tdi::Target &dev_tgt,
                                       const tdi::Flags &flags,
                                       tdi::TableData *data) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                tdi::TableData *data) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi_handle_t &entry_handle,
                                tdi::TableKey *key,
                                tdi::TableData *data) const override;

  virtual tdi_status_t entryKeyGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   const tdi_handle_t &entry_handle,
                                   tdi::Target *entry_tgt,
                                   tdi::TableKey *key) const override;

  virtual tdi_status_t entryHandleGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      tdi_handle_t *entry_handle) const override;

  virtual tdi_status_t entryGetFirst(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     tdi::TableKey *key,
                                     tdi::TableData *data) const override;

  virtual tdi_status_t entryGetNextN(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     const uint32_t &n,
                                     keyDataPairs *key_data_pairs,
                                     uint32_t *num_returned) const override;

  virtual tdi_status_t usageGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                uint32_t *count) const override;

  virtual tdi_status_t sizeGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               size_t *size) const override;
  virtual tdi_status_t keyAllocate(
      std::unique_ptr<tdi::TableKey> *key_ret) const override;
  virtual tdi_status_t keyReset(tdi::TableKey *key) const override;

  virtual tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  virtual tdi_status_t dataAllocate(
      const tdi_id_t &action_id,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  virtual tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  virtual tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      const tdi_id_t &action_id,
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  virtual tdi_status_t dataReset(tdi::TableData *data) const override;
  virtual tdi_status_t dataReset(const tdi_id_t &action_id,
                                 tdi::TableData *data) const override;
  virtual tdi_status_t dataReset(const std::vector<tdi_id_t> &fields,
                                 tdi::TableData *data) const override;
  virtual tdi_status_t dataReset(const std::vector<tdi_id_t> &fields,
                                 const tdi_id_t &action_id,
                                 tdi::TableData *data) const override;

  virtual tdi_status_t attributeAllocate(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;
#if 0
  virtual tdi_status_t attributeReset(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;
  virtual tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableAttributes &tableAttributes) const override;
  virtual tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      tdi::TableAttributes *tableAttributes) const override;

  virtual tdi_status_t operationsAllocate(
      const tdi_operations_type_e &type,
      std::unique_ptr<tdi::TableOperations> *table_ops) const override;
  virtual tdi_status_t tableOperationsExecute(
      const tdi::TableOperations &tableOperations) const override;
#endif

  bool actionIdApplicable() const override { return false; };

 private:
  tdi_status_t dataAllocate_internal(tdi_id_t action_id,
                                     std::unique_ptr<tdi::TableData> *data_ret,
                                     const std::vector<tdi_id_t> &fields) const;

  tdi_status_t entryGet_internal(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 const pipe_mat_ent_hdl_t &pipe_entry_hdl,
                                 pipe_tbl_match_spec_t *pipe_match_spec,
                                 tdi::TableData *data) const;

  // Store information about direct resources applicable per action
  std::map<tdi_id_t, bool> act_uses_dir_meter;
  std::map<tdi_id_t, bool> act_uses_dir_cntr;
  std::map<tdi_id_t, bool> act_uses_dir_reg;
};

class MatchActionIndirect : public tdi::Table {
 public:
  MatchActionIndirect(const tdi::TdiInfo *tdi_info,
                      const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_ADD, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DELETE, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_SET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_RESET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_USAGE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_SIZE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  }
  tdi_status_t getActionState(const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const MatchActionIndirectTableData *data,
                              pipe_adt_ent_hdl_t *adt_entry_hdl,
                              pipe_sel_grp_hdl_t *sel_grp_hdl,
                              pipe_act_fn_hdl_t *act_fn_hdl,
                              pipe_mgr_adt_ent_data_t *ap_ent_data) const;
  tdi_status_t getActionMbrIdFromHndl(const tdi::Session &session,
                                      const tdi::Target &dev_tgt,
                                      const pipe_adt_ent_hdl_t adt_ent_hdl,
                                      tdi_id_t *mbr_id) const;

  tdi_status_t getGroupIdFromHndl(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const pipe_sel_grp_hdl_t sel_grp_hdl,
                                  tdi_id_t *grp_id) const;

  virtual tdi_status_t entryAdd(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      const tdi::TableData &data) const override final;
  virtual tdi_status_t entryMod(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryDel(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key) const override;

  virtual tdi_status_t clear(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags) const override;

  virtual tdi_status_t defaultEntrySet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableData &data) const override;

#if 0
  virtual tdi_status_t defaultEntryMod(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableData &data) const override;
#endif

  virtual tdi_status_t defaultEntryReset(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags) const override;

  virtual tdi_status_t defaultEntryGet(const tdi::Session &session,
                                       const tdi::Target &dev_tgt,
                                       const tdi::Flags &flags,
                                       tdi::TableData *data) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                tdi::TableData *data) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi_handle_t &entry_handle,
                                tdi::TableKey *key,
                                tdi::TableData *data) const override;

  virtual tdi_status_t entryKeyGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   const tdi_handle_t &entry_handle,
                                   tdi::Target *entry_tgt,
                                   tdi::TableKey *key) const override;

  virtual tdi_status_t entryHandleGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      tdi_handle_t *entry_handle) const override;

  virtual tdi_status_t entryGetFirst(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     tdi::TableKey *key,
                                     tdi::TableData *data) const override;

  virtual tdi_status_t entryGetNextN(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     const uint32_t &n,
                                     keyDataPairs *key_data_pairs,
                                     uint32_t *num_returned) const override;

  virtual tdi_status_t usageGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                uint32_t *count) const override;

  virtual tdi_status_t sizeGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               size_t *size) const override;
  virtual tdi_status_t keyAllocate(
      std::unique_ptr<tdi::TableKey> *key_ret) const override;
  virtual tdi_status_t keyReset(tdi::TableKey *key) const override;

  virtual tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  virtual tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  virtual tdi_status_t dataReset(tdi::TableData *data) const override;
  virtual tdi_status_t dataReset(const std::vector<tdi_id_t> &fields,
                                 tdi::TableData *data) const override;

#if 0
  virtual tdi_status_t attributeAllocate(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;
  virtual tdi_status_t attributeReset(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;
  virtual tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableAttributes &tableAttributes) const override;
  virtual tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      tdi::TableAttributes *tableAttributes) const override;

  virtual tdi_status_t operationsAllocate(
      const tdi_operations_type_e &type,
      std::unique_ptr<tdi::TableOperations> *table_ops) const override;
  virtual tdi_status_t tableOperationsExecute(
      const tdi::TableOperations &tableOperations) const override;
#endif

  bool actionIdApplicable() const override { return false; };

 private:
  tdi_status_t dataAllocate_internal(tdi_id_t action_id,
                                     std::unique_ptr<tdi::TableData> *data_ret,
                                     const std::vector<tdi_id_t> &fields) const;

  tdi_status_t entryGet_internal(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 const pipe_mat_ent_hdl_t &pipe_entry_hdl,
                                 pipe_tbl_match_spec_t *pipe_match_spec,
                                 tdi::TableData *data) const;

  // Store information about direct resources applicable per action
  std::map<tdi_id_t, bool> act_uses_dir_meter;
  std::map<tdi_id_t, bool> act_uses_dir_cntr;
  std::map<tdi_id_t, bool> act_uses_dir_reg;

  // indirect table specific
  void populate_indirect_resources(const pipe_mgr_adt_ent_data_t &ent_data,
                                   pipe_action_spec_t *pipe_action_spec) const;
};

class ActionProfile : public tdi::Table {
 public:
  ActionProfile(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_ADD, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DELETE, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_USAGE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };

  virtual tdi_status_t entryAdd(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryMod(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryDel(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key) const override;

  virtual tdi_status_t clear(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                tdi::TableData *data) const override;
  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi_handle_t &entry_handle,
                                tdi::TableKey *key,
                                tdi::TableData *data) const override;
  virtual tdi_status_t entryKeyGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   const tdi_handle_t &entry_handle,
                                   tdi::Target *entry_tgt,
                                   tdi::TableKey *key) const override;

  virtual tdi_status_t entryHandleGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      tdi_handle_t *entry_handle) const override;

  virtual tdi_status_t entryGetFirst(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     tdi::TableKey *key,
                                     tdi::TableData *data) const override;

  virtual tdi_status_t entryGetNextN(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     const uint32_t &n,
                                     keyDataPairs *key_data_pairs,
                                     uint32_t *num_returned) const override;

  virtual tdi_status_t usageGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                uint32_t *count) const override;

  virtual tdi_status_t keyAllocate(
      std::unique_ptr<tdi::TableKey> *key_ret) const override;
  virtual tdi_status_t keyReset(tdi::TableKey *key) const override;

  virtual tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  virtual tdi_status_t dataAllocate(
      const tdi_id_t &action_id,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  tdi_status_t getMbrIdFromHndl(const tdi::Session &session,
                                const tdi::Target &dev_tgt,

                                const pipe_adt_ent_hdl_t adt_ent_hdl,
                                tdi_id_t *mbr_id) const;

  tdi_status_t getHdlFromMbrId(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi_id_t mbr_id,
                               pipe_adt_ent_hdl_t *adt_ent_hdl) const;

  tdi_status_t getMbrState(const tdi::Session &session,
                           const tdi::Target &dev_tgt,
                           tdi_id_t mbr_id,
                           pipe_act_fn_hdl_t *act_fn_hdl,
                           pipe_adt_ent_hdl_t *adt_ent_hdl,
                           pipe_mgr_adt_ent_data_t *ap_ent_data) const;
  tdi_status_t getFirstMbr(const tdi::Target &dev_tgt,
                           tdi_id_t *first_mbr_id,
                           pipe_adt_ent_hdl_t *first_entry_hdl) const;

  tdi_status_t getNextMbr(const tdi::Target &dev_tgt,
                          tdi_id_t mbr_id,
                          tdi_id_t *next_mbr_id,
                          tdi_id_t *next_entry_hdl) const;
  tdi_status_t getMbrList(
      uint16_t device_id,
      std::vector<std::pair<tdi_id_t, bf_dev_pipe_t>> *mbr_id_list) const;

 private:
  tdi_status_t entryGet_internal(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 const pipe_adt_ent_hdl_t &entry_hdl,
                                 ActionProfileData *action_tbl_data) const;
};

class Selector : public tdi::Table {
 public:
  Selector(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_ADD, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DELETE, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_USAGE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };

  virtual tdi_status_t entryAdd(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryMod(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                const tdi::TableData &data) const override;
  virtual tdi_status_t entryDel(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key) const override;

  virtual tdi_status_t clear(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags) const override;

  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi::TableKey &key,
                                tdi::TableData *data) const override;
  virtual tdi_status_t entryGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                const tdi_handle_t &entry_handle,
                                tdi::TableKey *key,
                                tdi::TableData *data) const override;
  virtual tdi_status_t entryKeyGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   const tdi_handle_t &entry_handle,
                                   tdi::Target *entry_tgt,
                                   tdi::TableKey *key) const override;

  virtual tdi_status_t entryHandleGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableKey &key,
      tdi_handle_t *entry_handle) const override;

  virtual tdi_status_t entryGetFirst(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     tdi::TableKey *key,
                                     tdi::TableData *data) const override;

  virtual tdi_status_t entryGetNextN(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags,
                                     const tdi::TableKey &key,
                                     const uint32_t &n,
                                     keyDataPairs *key_data_pairs,
                                     uint32_t *num_returned) const override;

  virtual tdi_status_t getOneMbr(const tdi::Session &session,
                                 const dev_target_t dev_tgt,
                                 const pipe_sel_grp_hdl_t sel_grp_hdl,
                                 pipe_adt_ent_hdl_t *member_hdl) const;

  virtual tdi_status_t usageGet(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const tdi::Flags &flags,
                                uint32_t *count) const override;

  virtual tdi_status_t keyAllocate(
      std::unique_ptr<tdi::TableKey> *key_ret) const override;
  virtual tdi_status_t keyReset(tdi::TableKey *key) const override;

  virtual tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  virtual tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  virtual tdi_status_t dataReset(tdi::TableData *data) const override;

#if 0  // TODO: Attributes
  virtual tdi_status_t attributeAllocate(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;

  virtual tdi_status_t attributeReset(
      const tdi_attributes_type_e &type,
      std::unique_ptr<tdi::TableAttributes> *attr) const override;

  virtual tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const tdi::TableAttributes &tableAttributes) const override;

  virtual tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      tdi::TableAttributes *tableAttributes) const override;

#endif

  tdi_status_t getGrpIdFromHndl(const tdi::Session &session,
                                const tdi::Target &dev_tgt,
                                const pipe_sel_grp_hdl_t &sel_grp_hdl,
                                tdi_id_t *sel_grp_id) const;

  tdi_status_t getGrpHdl(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi_id_t sel_grp_id,
                         pipe_sel_grp_hdl_t *sel_grp_hdl) const;

  tdi_status_t getActMbrIdFromHndl(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const pipe_adt_ent_hdl_t &adt_ent_hd,
                                   tdi_id_t *act_mbr_id) const;

 private:
  tdi_status_t entryGet_internal(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi_id_t &grp_id,
                                 SelectorTableData *sel_tbl_data) const;

#if 0  // TODO: tableAttributesImpl not defined
  tdi_status_t processSelUpdateCbAttr(
      const BfRtTableAttributesImpl &tbl_attr_impl,
      const bf_rt_target_t &dev_tgt) const;
#endif
  tdi_status_t getFirstGrp(const tdi::Target &dev_tgt,
                           tdi_id_t *first_grp_id,
                           pipe_sel_grp_hdl_t *first_grp_hdl) const;

  tdi_status_t getNextGrp(const tdi::Target &dev_tgt,
                          tdi_id_t grp_id,
                          tdi_id_t *next_grp_id,
                          pipe_sel_grp_hdl_t *next_grp_hdl) const;
};

class CounterIndirect : public tdi::Table {
 public:
  CounterIndirect(const tdi::TdiInfo *tdi_info,
                  const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_GET, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_FIRST,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_NEXT_N,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_GET_BY_HANDLE,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_HANDLE_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_KEY_GET, {"dev_id", "pipe_id", "pipe_all"}},
            }),
            table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  }

  tdi_status_t entryGet(const tdi::Session &session, const tdi::Target &dev_tgt,
                        const tdi::Flags &flags, const tdi::TableKey &key,
                        tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                           const tdi::Target &dev_tgt, const tdi::Flags &flags,
                           const tdi_handle_t &entry_handle,
                           tdi::Target *entry_tgt,
                           tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const tdi::Flags &flags, const tdi::TableKey &key,
                              tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session, const tdi::Target &dev_tgt,
                        const tdi::Flags &flags,
                        const tdi_handle_t &entry_handle, tdi::TableKey *key,
                        tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags, tdi::TableKey *key,
                             tdi::TableData *data) const override;

  tdi_status_t entryGetNextN(const tdi::Session &session,
                             const tdi::Target &dev_tgt,
                             const tdi::Flags &flags, const tdi::TableKey &key,
                             const uint32_t &n, keyDataPairs *key_data_pairs,
                             uint32_t *num_returned) const override;

  tdi_status_t clear(const tdi::Session &session, const tdi::Target &dev_tgt,
                     const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;
};

class MeterIndirect : public tdi::Table {
 public:
  MeterIndirect(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

#if 0

class ActionTable : public tdi::Table {
 public:
  ActionTable(const std::string &program_name,
                  const tdi_id_t &id,
                  const std::string &name,
                  const size_t &size,
                  const pipe_adt_tbl_hdl_t &pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::ACTION_PROFILE,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::DELETE,
                         TableApi::CLEAR,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::USAGE_GET,
                         TableApi::GET_BY_HANDLE,
                         TableApi::KEY_GET,
                         TableApi::HANDLE_GET,
                     },
                     pipe_hdl){};

  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryDel(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableUsageGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            uint32_t *count) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;

  tdi_status_t keyReset(TableKey *) const override;

  tdi_status_t dataAllocate(
      const tdi_id_t &action_id,
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;

  tdi_status_t dataReset(const tdi_id_t &action_id,
                        tdi::TableData *data) const override;

  // This API is more than enough to enable action APIs
  // on this table
  bool actionIdApplicable() const override { return true; };

  tdi_status_t registerAdtUpdateCb(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const pipe_adt_update_cb &cb,
                                  const void *cookie) const override;

  tdi_status_t getMbrIdFromHndl(const tdi::Session &session,
                               const tdi::Target &dev_tgt,

                               const pipe_adt_ent_hdl_t adt_ent_hdl,
                               tdi_id_t *mbr_id) const;

  tdi_status_t getHdlFromMbrId(const tdi::Session &session,
                              const tdi::Target &dev_tgt,
                              const tdi_id_t mbr_id,
                              pipe_adt_ent_hdl_t *adt_ent_hdl) const;

  tdi_status_t getMbrState(const tdi::Session &session,
                          const tdi::Target &dev_tgt,
                          tdi_id_t mbr_id,
                          pipe_act_fn_hdl_t *act_fn_hdl,
                          pipe_adt_ent_hdl_t *adt_ent_hdl,
                          pipe_mgr_adt_ent_data_t *ap_ent_data) const;
  tdi_status_t getFirstMbr(const tdi::Target &dev_tgt,
                          tdi_id_t *first_mbr_id,
                          pipe_adt_ent_hdl_t *first_entry_hdl) const;

  tdi_status_t getNextMbr(const tdi::Target &dev_tgt,
                         tdi_id_t mbr_id,
                         tdi_id_t *next_mbr_id,
                         tdi_id_t *next_entry_hdl) const;
  tdi_status_t getMbrList(
      uint16_t device_id,
      std::vector<std::pair<tdi_id_t, bf_dev_pipe_t>> *mbr_id_list) const;

 private:
  tdi_status_t entryGet_internal(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const pipe_adt_ent_hdl_t &entry_hdl,
      ActionTableData *action_tbl_data) const;
};

class SelectorTable : public tdi::Table {
 public:
  SelectorTable(const std::string &program_name,
                    const tdi_id_t &id,
                    const std::string &name,
                    const size_t &size,
                    const pipe_sel_tbl_hdl_t &pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::SELECTOR,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::DELETE,
                         TableApi::CLEAR,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::USAGE_GET,
                         TableApi::GET_BY_HANDLE,
                         TableApi::KEY_GET,
                         TableApi::HANDLE_GET,
                     },
                     pipe_hdl){};

  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryDel(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableUsageGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            uint32_t *count) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;
  tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;

  tdi_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;

  tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const TableAttributes &tableAttributes) const override;

  tdi_status_t tableAttributesGet(
      const tdi::Session & /*session */,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      TableAttributes *tableAttributes) const override;

  tdi_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;

  tdi_status_t getGrpIdFromHndl(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const pipe_sel_grp_hdl_t &sel_grp_hdl,
                               tdi_id_t *sel_grp_id) const;

  tdi_status_t getGrpHdl(const tdi::Session &session,
                        const tdi::Target &dev_tgt,
                        const tdi_id_t sel_grp_id,
                        pipe_sel_grp_hdl_t *sel_grp_hdl) const;

  tdi_status_t getActMbrIdFromHndl(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const pipe_adt_ent_hdl_t &adt_ent_hdl,
                                  tdi_id_t *act_mbr_id) const;

 private:
  tdi_status_t entryGet_internal(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi_id_t &grp_id,
                                     SelectorTableData *sel_tbl_data) const;

  tdi_status_t processSelUpdateCbAttr(
      const TableAttributesImpl &tbl_attr_impl,
      const tdi::Target &dev_tgt) const;

  tdi_status_t getFirstGrp(const tdi::Target &dev_tgt,
                          tdi_id_t *first_grp_id,
                          pipe_sel_grp_hdl_t *first_grp_hdl) const;

  tdi_status_t getNextGrp(const tdi::Target &dev_tgt,
                         tdi_id_t grp_id,
                         tdi_id_t *next_grp_id,
                         pipe_sel_grp_hdl_t *next_grp_hdl) const;
};

class MeterTable : public tdi::Table {
 public:
  MeterTable(const std::string &program_name,
                 const tdi_id_t &id,
                 const std::string &name,
                 const size_t &size,
                 const pipe_meter_tbl_hdl_t &pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::METER,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::CLEAR,
                         TableApi::GET_BY_HANDLE,
                         TableApi::HANDLE_GET,
                         TableApi::KEY_GET,
                     },
                     pipe_hdl){};

  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;

  // Table attributes APIs
  tdi_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;

  tdi_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;

  tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const TableAttributes &tableAttributes) const override;

  tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      TableAttributes *tableAttributes) const override;
};

class RegisterTable : public tdi::Table {
 public:
  RegisterTable(const std::string &program_name,
                    const tdi_id_t &id,
                    const std::string &name,
                    const size_t &size,
                    const pipe_tbl_hdl_t &pipe_hdl)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::REGISTER,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::CLEAR,
                         TableApi::GET_BY_HANDLE,
                         TableApi::HANDLE_GET,
                         TableApi::KEY_GET,
                     },
                     pipe_hdl),
        ghost_pipe_tbl_hdl_(){};
  tdi_status_t entryAdd(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryMod(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            const tdi::TableData &data) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t entryKeyGet(const tdi::Session &session,
                               const tdi::Target &dev_tgt,
                               const tdi::Flags &flags,
                               const tdi_handle_t &entry_handle,
                               tdi::Target *entry_tgt,
                               tdi::TableKey *key) const override;

  tdi_status_t entryHandleGet(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  tdi_handle_t *entry_handle) const override;

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi_handle_t &entry_handle,
                            tdi::TableKey *key,
                            tdi::TableData *data) const override;

  tdi_status_t entryGetFirst(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 tdi::TableKey *key,
                                 tdi::TableData *data) const override;

  tdi_status_t entryGetNext_n(const tdi::Session &session,
                                  const tdi::Target &dev_tgt,
                                  const tdi::Flags &flags,
                                  const tdi::TableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override;

  tdi_status_t tableClear(const tdi::Session &session,
                         const tdi::Target &dev_tgt,
                         const tdi::Flags &flags) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;
  tdi_status_t keyReset(TableKey *key) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

  tdi_status_t dataReset(tdi::TableData *data) const override;

  // Attribute APIs
  tdi_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;
  tdi_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;
  tdi_status_t tableAttributesSet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      const TableAttributes &tableAttributes) const override;
  tdi_status_t tableAttributesGet(
      const tdi::Session &session,
      const tdi::Target &dev_tgt,
      const tdi::Flags &flags,
      TableAttributes *tableAttributes) const override;

  tdi_status_t ghostTableHandleSet(const pipe_tbl_hdl_t &pipe_hdl) override;

 private:
  // Ghost table handle
  pipe_tbl_hdl_t ghost_pipe_tbl_hdl_;
};

class RegisterParamTable : public tdi::Table {
 public:
  RegisterParamTable(const std::string &program_name,
                         tdi_id_t id,
                         std::string name,
                         const size_t &size)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::REG_PARAM,
                     std::set<TableApi>{TableApi::DEFAULT_ENTRY_SET,
                                        TableApi::DEFAULT_ENTRY_RESET,
                                        TableApi::DEFAULT_ENTRY_GET}){};

  tdi_status_t tableDefaultEntrySet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   const tdi::TableData &data) const override;

  tdi_status_t tableDefaultEntryReset(const tdi::Session &session,
                                     const tdi::Target &dev_tgt,
                                     const tdi::Flags &flags) const override;

  tdi_status_t tableDefaultEntryGet(const tdi::Session &session,
                                   const tdi::Target &dev_tgt,
                                   const tdi::Flags &flags,
                                   tdi::TableData *data) const override;
  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override final;

 private:
  tdi_status_t getRef(pipe_tbl_hdl_t *hdl) const;
  tdi_status_t getParamHdl(const tdi::Session &session, tdi_dev_id_t dev_id) const;
};

class SelectorGetMemberTable : public tdi::Table {
 public:
  SelectorGetMemberTable(const std::string &program_name,
                             tdi_id_t id,
                             std::string name,
                             const size_t &size)
      : tdi::Table(program_name,
                     id,
                     name,
                     size,
                     TableType::SELECTOR_GET_MEMBER,
                     std::set<TableApi>{
                         TableApi::GET,
                     }){};

  tdi_status_t entryGet(const tdi::Session &session,
                            const tdi::Target &dev_tgt,
                            const tdi::Flags &flags,
                            const tdi::TableKey &key,
                            tdi::TableData *data) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<tdi::TableData> *data_ret) const override;

 private:
  tdi_status_t getRef(pipe_sel_tbl_hdl_t *sel_tbl_id,
                     const SelectorTable **sel_tbl,
                     const ActionTable **act_tbl) const;
};
#endif

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  // _TDI_P4_TABLE_OBJ_HPP

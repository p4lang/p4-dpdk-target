/*
 * Copyright(c) 2022 Intel Corporation.
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
#ifndef _TDI_FIXED_TABLE_OBJ_HPP
#define _TDI_FIXED_TABLE_OBJ_HPP

// tdi includes
#include <tdi/common/tdi_table.hpp>

#include <tdi_common/tdi_pipe_mgr_intf.hpp>
#include <tdi_common/tdi_fixed_mgr_intf.hpp>
#include "tdi_fixed_table_data_impl.hpp"
#include <port_mgr/port_mgr_port.h>

namespace tdi {
namespace pna {
namespace rt {
/*
 * FixedFunctionConfigTable
 */
class FixedFunctionConfigTable : public tdi::Table {
 public:
  FixedFunctionConfigTable(const tdi::TdiInfo *tdi_info,
                    const tdi::TableInfo *table_info)
      : tdi::Table(
            tdi_info,
            tdi::SupportedApis({
                {TDI_TABLE_API_TYPE_ADD, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_MODIFY, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DELETE, {"dev_id", "pipe_id", "pipe_all"}},
                {TDI_TABLE_API_TYPE_DEFAULT_ENTRY_GET,
                 {"dev_id", "pipe_id", "pipe_all"}},
                /*{TDI_TABLE_API_TYPE_CLEAR, {"dev_id", "pipe_id", "pipe_all"}},
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
                 {"dev_id", "pipe_id", "pipe_all"}},*/
            }),
            table_info) {
    LOG_DBG("Creating fixed function table for %s", table_info->nameGet().c_str());
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

 private:
  tdi_status_t dataAllocate_internal(tdi_id_t action_id,
                                     std::unique_ptr<tdi::TableData> *data_ret,
                                     const std::vector<tdi_id_t> &fields) const;

  tdi_status_t entryGet_internal(const tdi::Session &session,
                                 const tdi::Target &dev_tgt,
                                 const tdi::Flags &flags,
                                 fixed_function_key_spec_t *pipe_match_spec,
                                 tdi::TableData *data) const;
};

}  // namespace rt
}  // namespace pna
}  // namespace tdi
#endif  // _TDI_FIXED_TABLE_OBJ_HPP

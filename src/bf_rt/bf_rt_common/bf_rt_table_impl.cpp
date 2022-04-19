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
#include <bf_rt_common/bf_rt_table_impl.hpp>
//#include <bf_rt_dev/bf_rt_dev_table_impl.hpp>
//#include <bf_rt_mirror/bf_rt_mirror_table_impl.hpp>
#include <bf_rt_p4/bf_rt_p4_table_impl.hpp>
//#include <bf_rt_pktgen/bf_rt_pktgen_table_impl.hpp>
#include "bf_rt_port_table_impl.hpp"
//#include <bf_rt_pre/bf_rt_pre_table_impl.hpp>
//#include <bf_rt_tm/bf_rt_tm_table_impl.hpp>
//#include <bf_rt_tm/bf_rt_tm_table_impl_counters.hpp>
//#include <bf_rt_tm/bf_rt_tm_table_impl_mirror.hpp>
//#include <bf_rt_tm/bf_rt_tm_table_impl_pool.hpp>
//#include <bf_rt_tm/bf_rt_tm_table_impl_port.hpp>
//#include <bf_rt_tm/bf_rt_tm_table_impl_portgroup.hpp>
//#include <bf_rt_tm/bf_rt_tm_table_impl_ppg.hpp>
//#include <bf_rt_tm/bf_rt_tm_table_impl_queue.hpp>
//#include <bf_rt_tm/bf_rt_tm_table_impl_cfg.hpp>
//#include <bf_rt_tm/bf_rt_tm_table_impl_pipe.hpp>
#include "bf_rt_table_operations_impl.hpp"

namespace bfrt {
std::string bfRtNullStr = "";
// Base BfRtTableObj ************
std::unique_ptr<BfRtTableObj> BfRtTableObj::make_table(
    const std::string &prog_name,
    const TableType &table_type,
    const bf_rt_id_t &id,
    const std::string &name,
    const size_t &size,
    const pipe_tbl_hdl_t &pipe_hdl) {
  switch (table_type) {
    case TableType::MATCH_DIRECT:
      return std::unique_ptr<BfRtTableObj>(
          new BfRtMatchActionTable(prog_name, id, name, size, pipe_hdl));
    case TableType::MATCH_INDIRECT:
    case TableType::MATCH_INDIRECT_SELECTOR:
      return std::unique_ptr<BfRtTableObj>(new BfRtMatchActionIndirectTable(
          prog_name, id, name, size, table_type, pipe_hdl));
    case TableType::ACTION_PROFILE:
      return std::unique_ptr<BfRtTableObj>(
          new BfRtActionTable(prog_name, id, name, size, pipe_hdl));
    case TableType::SELECTOR:
      return std::unique_ptr<BfRtTableObj>(
          new BfRtSelectorTable(prog_name, id, name, size, pipe_hdl));
    case TableType::COUNTER:
      return std::unique_ptr<BfRtTableObj>(
          new BfRtCounterTable(prog_name, id, name, size, pipe_hdl));
    case TableType::METER:
      return std::unique_ptr<BfRtTableObj>(
          new BfRtMeterTable(prog_name, id, name, size, pipe_hdl));
    default:
      break;
  }
  return nullptr;
}

std::unique_ptr<BfRtTableObj> BfRtTableObj::make_table(
    const std::string &prog_name,
    const TableType &table_type,
    const bf_rt_id_t &id,
    const std::string &name,
    const size_t &size) {
  switch (table_type) {
    case TableType::PORT_CFG:
      return std::unique_ptr<BfRtTableObj>(
          new BfRtPortCfgTable(prog_name, id, name, size));
    case TableType::PORT_STAT:
      return std::unique_ptr<BfRtTableObj>(
          new BfRtPortStatTable(prog_name, id, name, size));
    case TableType::REG_PARAM:
      return std::unique_ptr<BfRtTableObj>(
          new BfRtRegisterParamTable(prog_name, id, name, size));
    case TableType::SELECTOR_GET_MEMBER:
      return std::unique_ptr<BfRtTableObj>(
          new BfRtSelectorGetMemberTable(prog_name, id, name, size));
    default:
      break;
  }
  return nullptr;
}

bool Annotation::operator<(const Annotation &other) const {
  return (this->full_name_ < other.full_name_);
}
bool Annotation::operator==(const Annotation &other) const {
  return (this->full_name_ == other.full_name_);
}
bool Annotation::operator==(const std::string &other_str) const {
  return (this->full_name_ == other_str);
}
bf_status_t Annotation::fullNameGet(std::string *full_name) const {
  *full_name = full_name_;
  return BF_SUCCESS;
}

// Beginning of old flags wrappers section.
bf_status_t BfRtTableObj::tableEntryAdd(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const BfRtTableKey &key,
                                        const BfRtTableData &data) const {
  return this->tableEntryAdd(session, dev_tgt, 0, key, data);
}

bf_status_t BfRtTableObj::tableEntryMod(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const BfRtTableKey &key,
                                        const BfRtTableData &data) const {
  return this->tableEntryMod(session, dev_tgt, 0, key, data);
}

bf_status_t BfRtTableObj::tableEntryModInc(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtTableKey &key,
    const BfRtTableData &data,
    const BfRtTable::BfRtTableModIncFlag &flag) const {
  uint64_t flags = 0;
  if (flag == BfRtTable::BfRtTableModIncFlag::MOD_INC_DELETE) {
    BF_RT_FLAG_SET(flags, BF_RT_INC_DEL);
  }
  return this->tableEntryModInc(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtTableObj::tableEntryDel(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const BfRtTableKey &key) const {
  return this->tableEntryDel(session, dev_tgt, 0, key);
}

bf_status_t BfRtTableObj::tableClear(const BfRtSession &session,
                                     const bf_rt_target_t &dev_tgt) const {
  return this->tableClear(session, dev_tgt, 0);
}

bf_status_t BfRtTableObj::tableDefaultEntrySet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtTableData &data) const {
  return this->tableDefaultEntrySet(session, dev_tgt, 0, data);
}

bf_status_t BfRtTableObj::tableDefaultEntryReset(
    const BfRtSession &session, const bf_rt_target_t &dev_tgt) const {
  return this->tableDefaultEntryReset(session, dev_tgt, 0);
}

bf_status_t BfRtTableObj::tableDefaultEntryGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtTable::BfRtTableGetFlag &flag,
    BfRtTableData *data) const {
  uint64_t flags = 0;
  if (flag == BfRtTable::BfRtTableGetFlag::GET_FROM_HW) {
    BF_RT_FLAG_SET(flags, BF_RT_FROM_HW);
  }
  return this->tableDefaultEntryGet(session, dev_tgt, flags, data);
}

bf_status_t BfRtTableObj::tableEntryGet(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const BfRtTableKey &key,
                                        const BfRtTable::BfRtTableGetFlag &flag,
                                        BfRtTableData *data) const {
  uint64_t flags = 0;
  if (flag == BfRtTable::BfRtTableGetFlag::GET_FROM_HW) {
    BF_RT_FLAG_SET(flags, BF_RT_FROM_HW);
  }
  return this->tableEntryGet(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtTableObj::tableEntryGetFirst(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtTable::BfRtTableGetFlag &flag,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  uint64_t flags = 0;
  if (flag == BfRtTable::BfRtTableGetFlag::GET_FROM_HW) {
    BF_RT_FLAG_SET(flags, BF_RT_FROM_HW);
  }
  return this->tableEntryGetFirst(session, dev_tgt, flags, key, data);
}

bf_status_t BfRtTableObj::tableEntryGet(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const BfRtTable::BfRtTableGetFlag &flag,
                                        const bf_rt_handle_t &entry_handle,
                                        BfRtTableKey *key,
                                        BfRtTableData *data) const {
  uint64_t flags = 0;
  if (flag == BfRtTable::BfRtTableGetFlag::GET_FROM_HW) {
    BF_RT_FLAG_SET(flags, BF_RT_FROM_HW);
  }
  return this->tableEntryGet(session, dev_tgt, flags, entry_handle, key, data);
}

bf_status_t BfRtTableObj::tableEntryKeyGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const bf_rt_handle_t &entry_handle,
                                           bf_rt_target_t *entry_tgt,
                                           BfRtTableKey *key) const {
  return this->tableEntryKeyGet(
      session, dev_tgt, 0, entry_handle, entry_tgt, key);
}

bf_status_t BfRtTableObj::tableEntryHandleGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtTableKey &key,
    bf_rt_handle_t *entry_handle) const {
  return this->tableEntryHandleGet(session, dev_tgt, 0, key, entry_handle);
}

bf_status_t BfRtTableObj::tableEntryGetNext_n(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtTableKey &key,
    const uint32_t &n,
    const BfRtTable::BfRtTableGetFlag &flag,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  uint64_t flags = 0;
  if (flag == BfRtTable::BfRtTableGetFlag::GET_FROM_HW) {
    BF_RT_FLAG_SET(flags, BF_RT_FROM_HW);
  }
  return this->tableEntryGetNext_n(
      session, dev_tgt, flags, key, n, key_data_pairs, num_returned);
}

bf_status_t BfRtTableObj::tableSizeGet(const BfRtSession &session,
                                       const bf_rt_target_t &dev_tgt,
                                       size_t *size) const {
  return this->tableSizeGet(session, dev_tgt, 0, size);
}

bf_status_t BfRtTableObj::tableUsageGet(const BfRtSession &session,
                                        const bf_rt_target_t &dev_tgt,
                                        const BfRtTable::BfRtTableGetFlag &flag,
                                        uint32_t *count) const {
  uint64_t flags = 0;
  if (flag == BfRtTable::BfRtTableGetFlag::GET_FROM_HW) {
    BF_RT_FLAG_SET(flags, BF_RT_FROM_HW);
  }
  return this->tableUsageGet(session, dev_tgt, flags, count);
}
// End of old flags wrappers section.

bf_status_t BfRtTableObj::tableNameGet(std::string *name) const {
  if (name == nullptr) {
    LOG_ERROR("%s:%d %s ERROR Outparam passed null",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  *name = object_name;
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::tableIdGet(bf_rt_id_t *id) const {
  if (id == nullptr) {
    LOG_ERROR("%s:%d %s ERROR Outparam passed null",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  *id = object_id;
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::tableTypeGet(BfRtTable::TableType *table_type) const {
  if (nullptr == table_type) {
    LOG_ERROR("%s:%d Outparam passed is nullptr", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  *table_type = object_type;
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::tableEntryAdd(const BfRtSession & /*session*/,
                                        const bf_rt_target_t & /*dev_tgt*/,
                                        const uint64_t & /*flags*/,
                                        const BfRtTableKey & /*key*/,
                                        const BfRtTableData & /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table Entry add not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableEntryMod(const BfRtSession & /*session*/,
                                        const bf_rt_target_t & /*dev_tgt*/,
                                        const uint64_t & /*flags*/,
                                        const BfRtTableKey & /*key*/,
                                        const BfRtTableData & /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table entry mod not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableEntryModInc(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableKey & /*key*/,
    const BfRtTableData & /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table entry modify incremental not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableEntryDel(const BfRtSession & /*session*/,
                                        const bf_rt_target_t & /*dev_tgt*/,
                                        const uint64_t & /*flags*/,
                                        const BfRtTableKey & /*key*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table entry Delete not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableClear(const BfRtSession & /*session*/,
                                     const bf_rt_target_t & /*dev_tgt*/,
                                     const uint64_t & /*flags*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table Clear not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableDefaultEntrySet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableData & /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table default entry set not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableDefaultEntryReset(
    const BfRtSession & /* session */,
    const bf_rt_target_t & /* dev_tgt */,
    const uint64_t & /*flags*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table default entry reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableDefaultEntryGet(
    const BfRtSession & /* session */,
    const bf_rt_target_t & /* dev_tgt */,
    const uint64_t & /*flags*/,
    BfRtTableData * /* data */) const {
  LOG_ERROR("%s:%d %s ERROR : Table default entry get not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableEntryGet(const BfRtSession & /*session */,
                                        const bf_rt_target_t & /* dev_tgt */,
                                        const uint64_t & /*flags*/,
                                        const BfRtTableKey & /* &key */,
                                        BfRtTableData * /* data */) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableEntryGetFirst(const BfRtSession & /*session*/,
                                             const bf_rt_target_t & /*dev_tgt*/,
                                             const uint64_t & /*flags*/,
                                             BfRtTableKey * /*key*/,
                                             BfRtTableData * /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get first not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableEntryGet(const BfRtSession & /*session*/,
                                        const bf_rt_target_t & /*dev_tgt*/,
                                        const uint64_t & /*flags*/,
                                        const bf_rt_handle_t & /*entry_handle*/,
                                        BfRtTableKey * /*key*/,
                                        BfRtTableData * /*data*/) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get by handle not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableEntryKeyGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const bf_rt_handle_t & /*entry_handle*/,
    bf_rt_target_t * /*entry_tgt*/,
    BfRtTableKey * /*key*/) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get key not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableEntryHandleGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableKey & /*key*/,
    bf_rt_handle_t * /*entry_handle*/) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get handle not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableKey & /*key*/,
    const uint32_t & /*n*/,
    keyDataPairs * /*key_data_pairs*/,
    uint32_t * /*num_returned*/) const {
  LOG_ERROR("%s:%d %s ERROR Table entry get next_n not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableUsageGet(const BfRtSession & /*session*/,
                                        const bf_rt_target_t & /*dev_tgt*/,
                                        const uint64_t & /*flags*/,
                                        uint32_t * /*count*/) const {
  LOG_ERROR(
      "%s:%d %s Not supported", __func__, __LINE__, table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableSizeGet(const BfRtSession & /*session*/,
                                       const bf_rt_target_t & /*dev_tgt*/,
                                       const uint64_t & /*flags*/,
                                       size_t *size) const {
  if (nullptr == size) {
    LOG_ERROR("%s:%d Outparam passed is nullptr", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  *size = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::tableHasConstDefaultAction(
    bool *has_const_default_action) const {
  *has_const_default_action = this->has_const_default_action_;
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::tableIsConst(bool *is_const) const {
  *is_const = this->is_const_table_;
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::tableAnnotationsGet(
    AnnotationSet *annotations) const {
  for (const auto &annotation : this->annotations_) {
    (*annotations).insert(std::cref(annotation));
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::tableApiSupportedGet(TableApiSet *tableApis) const {
  for (const auto &api : this->table_apis_) {
    tableApis->insert(std::cref(api));
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::keyAllocate(
    std::unique_ptr<BfRtTableKey> * /*key_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table Key allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::keyReset(BfRtTableKey * /* key */) const {
  LOG_ERROR("%s:%d %s ERROR : Table Key reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataAllocate(
    std::unique_ptr<BfRtTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataAllocate(
    const bf_rt_id_t & /*action_id*/,
    std::unique_ptr<BfRtTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataAllocate(
    const std::vector<bf_rt_id_t> & /*fields*/,
    const bf_rt_id_t & /* action_id */,
    std::unique_ptr<BfRtTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataAllocate(
    const std::vector<bf_rt_id_t> & /* fields */,
    std::unique_ptr<BfRtTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataAllocateContainer(
    const bf_rt_id_t & /*container_id*/,
    std::unique_ptr<BfRtTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate container not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataAllocateContainer(
    const bf_rt_id_t & /*container_id*/,
    const bf_rt_id_t & /*action_id*/,
    std::unique_ptr<BfRtTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate container not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataAllocateContainer(
    const bf_rt_id_t & /*container_id*/,
    const std::vector<bf_rt_id_t> & /*fields*/,
    std::unique_ptr<BfRtTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate container not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataAllocateContainer(
    const bf_rt_id_t & /*container_id*/,
    const std::vector<bf_rt_id_t> & /*fields*/,
    const bf_rt_id_t & /*action_id*/,
    std::unique_ptr<BfRtTableData> * /*data_ret*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table data allocate container not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataReset(BfRtTableData * /*data */) const {
  LOG_ERROR("%s:%d %s ERROR : Table data reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataReset(const bf_rt_id_t & /* action_id */,
                                    BfRtTableData * /* data */) const {
  LOG_ERROR("%s:%d %s ERROR : Table data reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataReset(
    const std::vector<bf_rt_id_t> & /* fields */,
    BfRtTableData * /* data */) const {
  LOG_ERROR("%s:%d %s ERROR : Table data reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataReset(
    const std::vector<bf_rt_id_t> & /* fields */,
    const bf_rt_id_t & /* action_id */,
    BfRtTableData * /* data */) const {
  LOG_ERROR("%s:%d %s ERROR : Table data reset not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::attributeAllocate(
    const TableAttributesType & /*type*/,
    std::unique_ptr<BfRtTableAttributes> * /*attr*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table attribute allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::attributeAllocate(
    const TableAttributesType & /*type*/,
    const TableAttributesIdleTableMode & /*idle_table_mode*/,
    std::unique_ptr<BfRtTableAttributes> * /*attr*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table attribute allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::attributeReset(
    const TableAttributesType & /*type*/,
    std::unique_ptr<BfRtTableAttributes> * /*attr*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table attribute allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::attributeReset(
    const TableAttributesType & /*type*/,
    const TableAttributesIdleTableMode & /*idle_table_mode*/,
    std::unique_ptr<BfRtTableAttributes> * /*attr*/) const {
  LOG_ERROR("%s:%d %s ERROR : Table attribute allocate not supported",
            __func__,
            __LINE__,
            table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::operationsAllocate(
    const TableOperationsType &op_type,
    std::unique_ptr<BfRtTableOperations> *table_ops) const {
  auto op_found = operations_type_set.find(op_type);
  if (op_found == operations_type_set.end()) {
    *table_ops = nullptr;
    LOG_ERROR("%s:%d %s Operation not supported for this table",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }

  *table_ops = std::unique_ptr<BfRtTableOperations>(
      new BfRtTableOperationsImpl(this, op_type));
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::keyFieldIdListGet(
    std::vector<bf_rt_id_t> *id_vec) const {
  if (id_vec == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  for (const auto &kv : key_fields) {
    id_vec->push_back(kv.first);
  }
  std::sort(id_vec->begin(), id_vec->end());
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::keyFieldTypeGet(const bf_rt_id_t &field_id,
                                          KeyFieldType *field_type) const {
  if (field_type == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return BF_OBJECT_NOT_FOUND;
  }
  *field_type = key_fields.at(field_id)->getType();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::keyFieldDataTypeGet(const bf_rt_id_t &field_id,
                                              DataType *data_type) const {
  if (data_type == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return BF_OBJECT_NOT_FOUND;
  }
  *data_type = key_fields.at(field_id)->getDataType();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::keyFieldIdGet(const std::string &name,
                                        bf_rt_id_t *field_id) const {
  if (field_id == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  auto found = std::find_if(
      key_fields.begin(),
      key_fields.end(),
      [&name](const std::pair<const bf_rt_id_t,
                              std::unique_ptr<BfRtTableKeyField>> &map_item) {
        return (name == map_item.second->getName());
      });
  if (found != key_fields.end()) {
    *field_id = (*found).second->getId();
    return BF_SUCCESS;
  }

  LOG_ERROR("%s:%d %s Field \"%s\" not found in key field list",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            name.c_str());
  return BF_OBJECT_NOT_FOUND;
}

bf_status_t BfRtTableObj::keyFieldSizeGet(const bf_rt_id_t &field_id,
                                          size_t *size) const {
  if (size == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return BF_OBJECT_NOT_FOUND;
  }
  *size = key_fields.at(field_id)->getSize();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::keyFieldIsPtrGet(const bf_rt_id_t &field_id,
                                           bool *is_ptr) const {
  if (is_ptr == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return BF_OBJECT_NOT_FOUND;
  }
  *is_ptr = key_fields.at(field_id)->isPtr();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::keyFieldNameGet(const bf_rt_id_t &field_id,
                                          std::string *name) const {
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return BF_OBJECT_NOT_FOUND;
  }
  *name = key_fields.at(field_id)->getName();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::keyFieldAllowedChoicesGet(
    const bf_rt_id_t &field_id,
    std::vector<std::reference_wrapper<const std::string>> *choices) const {
  DataType data_type;
  bf_status_t sts = this->keyFieldDataTypeGet(field_id, &data_type);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  if (data_type != DataType::STRING) {
    LOG_ERROR("%s:%d %s This API is valid only for fields of type STRING",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  choices->clear();
  for (const auto &iter : this->key_fields.at(field_id)->getEnumChoices()) {
    choices->push_back(std::cref(iter));
  }
  return BF_SUCCESS;
}

uint32_t BfRtTableObj::dataFieldListSize() const {
  return this->dataFieldListSize(0);
}

uint32_t BfRtTableObj::dataFieldListSize(const bf_rt_id_t &action_id) const {
  if (action_info_list.find(action_id) != action_info_list.end()) {
    return action_info_list.at(action_id)->data_fields.size() +
           common_data_fields.size();
  }
  return common_data_fields.size();
}

bf_status_t BfRtTableObj::dataFieldIdListGet(
    const bf_rt_id_t &action_id, std::vector<bf_rt_id_t> *id_vec) const {
  if (id_vec == nullptr) {
    LOG_ERROR("%s:%d %s Please pass mem allocated out param",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  if (action_id) {
    if (action_info_list.find(action_id) == action_info_list.end()) {
      LOG_ERROR("%s:%d %s Action Id %d Not Found",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                action_id);
      return BF_OBJECT_NOT_FOUND;
    }
    for (const auto &kv : action_info_list.at(action_id)->data_fields) {
      id_vec->push_back(kv.first);
    }
  }
  // Also include common data fields
  for (const auto &kv : common_data_fields) {
    id_vec->push_back(kv.first);
  }
  std::sort(id_vec->begin(), id_vec->end());
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::dataFieldIdListGet(
    std::vector<bf_rt_id_t> *id_vec) const {
  return this->dataFieldIdListGet(0, id_vec);
}

bf_status_t BfRtTableObj::containerDataFieldIdListGet(
    const bf_rt_id_t &field_id, std::vector<bf_rt_id_t> *id_vec) const {
  if (id_vec == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, 0, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d not found", __func__, __LINE__, field_id);
    return status;
  }
  return field->containerFieldIdListGet(id_vec);
}

bf_status_t BfRtTableObj::dataFieldIdGet(const std::string &name,
                                         bf_rt_id_t *field_id) const {
  return this->dataFieldIdGet(name, 0, field_id);
}

bf_status_t BfRtTableObj::dataFieldIdGet(const std::string &name,
                                         const bf_rt_id_t &action_id,
                                         bf_rt_id_t *field_id) const {
  if (field_id == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  std::vector<bf_rt_id_t> empty;
  auto status = getDataField(name, action_id, empty, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field name %s Action ID %d not found",
              __func__,
              __LINE__,
              name.c_str(),
              action_id);
    return status;
  }
  *field_id = field->getId();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::dataFieldSizeGet(const bf_rt_id_t &field_id,
                                           size_t *size) const {
  return this->dataFieldSizeGet(field_id, 0, size);
}

bf_status_t BfRtTableObj::dataFieldSizeGet(const bf_rt_id_t &field_id,
                                           const bf_rt_id_t &action_id,
                                           size_t *size) const {
  if (size == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  if (field->isContainerValid()) {
    *size = field->containerSizeGet();
  } else {
    *size = field->getSize();
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::dataFieldIsPtrGet(const bf_rt_id_t &field_id,
                                            bool *is_ptr) const {
  return this->dataFieldIsPtrGet(field_id, 0, is_ptr);
}

bf_status_t BfRtTableObj::dataFieldIsPtrGet(const bf_rt_id_t &field_id,
                                            const bf_rt_id_t &action_id,
                                            bool *is_ptr) const {
  if (is_ptr == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *is_ptr = field->isPtr();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::dataFieldMandatoryGet(const bf_rt_id_t &field_id,
                                                bool *is_mandatory) const {
  return this->dataFieldMandatoryGet(field_id, 0, is_mandatory);
}

bf_status_t BfRtTableObj::dataFieldMandatoryGet(const bf_rt_id_t &field_id,
                                                const bf_rt_id_t &action_id,
                                                bool *is_mandatory) const {
  if (is_mandatory == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *is_mandatory = field->isMandatory();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::dataFieldReadOnlyGet(const bf_rt_id_t &field_id,
                                               bool *is_read_only) const {
  return this->dataFieldReadOnlyGet(field_id, 0, is_read_only);
}

bf_status_t BfRtTableObj::dataFieldReadOnlyGet(const bf_rt_id_t &field_id,
                                               const bf_rt_id_t &action_id,
                                               bool *is_read_only) const {
  if (is_read_only == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *is_read_only = field->isReadOnly();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::dataFieldOneofSiblingsGet(
    const bf_rt_id_t &field_id, std::set<bf_rt_id_t> *oneof_siblings) const {
  return this->dataFieldOneofSiblingsGet(field_id, 0, oneof_siblings);
}

bf_status_t BfRtTableObj::dataFieldOneofSiblingsGet(
    const bf_rt_id_t &field_id,
    const bf_rt_id_t &action_id,
    std::set<bf_rt_id_t> *oneof_siblings) const {
  if (oneof_siblings == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *oneof_siblings = field->oneofSiblings();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::dataFieldNameGet(const bf_rt_id_t &field_id,
                                           std::string *name) const {
  return this->dataFieldNameGet(field_id, 0, name);
}

bf_status_t BfRtTableObj::dataFieldNameGet(const bf_rt_id_t &field_id,
                                           const bf_rt_id_t &action_id,
                                           std::string *name) const {
  if (name == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *name = field->getName();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::dataFieldDataTypeGet(const bf_rt_id_t &field_id,
                                               DataType *type) const {
  return this->dataFieldDataTypeGet(field_id, 0, type);
}

bf_status_t BfRtTableObj::dataFieldDataTypeGet(const bf_rt_id_t &field_id,
                                               const bf_rt_id_t &action_id,
                                               DataType *type) const {
  if (type == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *type = field->getDataType();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::dataFieldAllowedChoicesGet(
    const bf_rt_id_t &field_id,
    std::vector<std::reference_wrapper<const std::string>> *choices) const {
  return dataFieldAllowedChoicesGet(field_id, 0, choices);
}

bf_status_t BfRtTableObj::dataFieldAllowedChoicesGet(
    const bf_rt_id_t &field_id,
    const bf_rt_id_t &action_id,
    std::vector<std::reference_wrapper<const std::string>> *choices) const {
  if (choices == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  if (field->getDataType() != DataType::STRING &&
      field->getDataType() != DataType::STRING_ARR) {
    LOG_ERROR("%s:%d %s API valid only for fields of type STRING field ID:%d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  choices->clear();
  for (const auto &iter : field->getEnumChoices()) {
    choices->push_back(std::cref(iter));
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::dataFieldAnnotationsGet(
    const bf_rt_id_t &field_id, AnnotationSet *annotations) const {
  return this->dataFieldAnnotationsGet(field_id, 0, annotations);
}

bf_status_t BfRtTableObj::dataFieldAnnotationsGet(
    const bf_rt_id_t &field_id,
    const bf_rt_id_t &action_id,
    AnnotationSet *annotations) const {
  if (annotations == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  for (const auto &annotation : field->getAnnotations()) {
    (*annotations).insert(std::cref(annotation));
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::defaultDataValueGet(const bf_rt_id_t &field_id,
                                              const bf_rt_id_t action_id,
                                              uint64_t *default_value) const {
  if (default_value == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *default_value = field->defaultValueGet();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::defaultDataValueGet(const bf_rt_id_t &field_id,
                                              const bf_rt_id_t action_id,
                                              float *default_value) const {
  if (default_value == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = BF_SUCCESS;
  status = getDataField(field_id, action_id, &tableDataField);

  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Field id %d not found for action id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id,
              action_id);
    return status;
  }

  *default_value = tableDataField->defaultFlValueGet();

  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::defaultDataValueGet(
    const bf_rt_id_t &field_id,
    const bf_rt_id_t action_id,
    std::string *default_value) const {
  if (default_value == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *tableDataField = nullptr;
  bf_status_t status = BF_SUCCESS;
  status = getDataField(field_id, action_id, &tableDataField);

  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Field id %d not found for action id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id,
              action_id);
    return status;
  }

  *default_value = tableDataField->defaultStrValueGet();

  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::actionIdGet(const std::string &name,
                                      bf_rt_id_t *action_id) const {
  if (action_id == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  if (!this->actionIdApplicable()) {
    LOG_TRACE("%s:%d Not supported", __func__, __LINE__);
    return BF_NOT_SUPPORTED;
  }
  auto action_found = std::find_if(
      action_info_list.begin(),
      action_info_list.end(),
      [&name](const std::pair<const bf_rt_id_t,
                              std::unique_ptr<bf_rt_info_action_info_t>> &
                  action_map_pair) {
        return action_map_pair.second->name == name;
      });
  if (action_found != action_info_list.end()) {
    *action_id = (*action_found).second->action_id;
    return BF_SUCCESS;
  }
  LOG_TRACE("%s:%d Action_ID for action %s not found",
            __func__,
            __LINE__,
            name.c_str());
  return BF_OBJECT_NOT_FOUND;
}

bool BfRtTableObj::actionIdApplicable() const { return false; }

bf_status_t BfRtTableObj::actionNameGet(const bf_rt_id_t &action_id,
                                        std::string *name) const {
  if (!this->actionIdApplicable()) {
    LOG_TRACE("%s:%d Not supported", __func__, __LINE__);
    return BF_NOT_SUPPORTED;
  }
  if (action_info_list.find(action_id) == action_info_list.end()) {
    LOG_TRACE("%s:%d Action Id %d Not Found", __func__, __LINE__, action_id);
    return BF_OBJECT_NOT_FOUND;
  }
  *name = action_info_list.at(action_id)->name;
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::actionIdListGet(
    std::vector<bf_rt_id_t> *id_vec) const {
  if (!this->actionIdApplicable()) {
    LOG_TRACE("%s:%d Not supported", __func__, __LINE__);
    return BF_NOT_SUPPORTED;
  }
  if (id_vec == nullptr) {
    LOG_TRACE("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  if (action_info_list.empty()) {
    LOG_TRACE("%s:%d Table has no action IDs", __func__, __LINE__);
    return BF_OBJECT_NOT_FOUND;
  }
  for (const auto &kv : action_info_list) {
    id_vec->push_back(kv.first);
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::actionAnnotationsGet(
    const bf_rt_id_t &action_id, AnnotationSet *annotations) const {
  // For the table to support actions, it needs to only override
  // actionIdApplicable() and all these action functions will remain
  // common
  if (!this->actionIdApplicable()) {
    LOG_TRACE("%s:%d Not supported", __func__, __LINE__);
    return BF_NOT_SUPPORTED;
  }
  if (action_info_list.find(action_id) == action_info_list.end()) {
    LOG_TRACE("%s:%d Action Id %d Not Found", __func__, __LINE__, action_id);
    return BF_OBJECT_NOT_FOUND;
  }
  for (const auto &annotation :
       this->action_info_list.at(action_id)->annotations) {
    (*annotations).insert(std::cref(annotation));
  }
  return BF_SUCCESS;
}

// Beginning of old flags wrapper section.
bf_status_t BfRtTableObj::tableAttributesSet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const BfRtTableAttributes &tableAttributes) const {
  return this->tableAttributesSet(session, dev_tgt, 0, tableAttributes);
}

bf_status_t BfRtTableObj::tableAttributesGet(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    BfRtTableAttributes *tableAttributes) const {
  return this->tableAttributesGet(session, dev_tgt, 0, tableAttributes);
}
// End of old flags wrapper section.

bf_status_t BfRtTableObj::tableAttributesSet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    const BfRtTableAttributes & /*tableAttributes*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::tableAttributesGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    BfRtTableAttributes * /*tableAttributes*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}
bf_status_t BfRtTableObj::tableAttributesSupported(
    std::set<TableAttributesType> *type_set) const {
  *type_set = attributes_type_set;
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::tableOperationsSupported(
    std::set<TableOperationsType> *op_type_set) const {
  *op_type_set = operations_type_set;
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::tableOperationsExecute(
    const BfRtTableOperations &table_ops) const {
  auto table_ops_impl =
      static_cast<const BfRtTableOperationsImpl *>(&table_ops);
  if (table_ops_impl->tableGet()->table_id_get() != table_id_get()) {
    LOG_ERROR("%s:%d %s Table mismatch. Sent in %s expected %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              table_ops_impl->tableGet()->table_name_get().c_str(),
              table_name_get().c_str());
  }
  switch (table_ops_impl->getAllowedOp()) {
    case TableOperationsType::REGISTER_SYNC:
      return table_ops_impl->registerSyncExecute();
    case TableOperationsType::COUNTER_SYNC:
      return table_ops_impl->counterSyncExecute();
    case TableOperationsType::HIT_STATUS_UPDATE:
      return table_ops_impl->hitStateUpdateExecute();
    default:
      LOG_ERROR("%s:%d %s Table operation is not allowed",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return BF_INVALID_ARG;
  }
  return BF_NOT_SUPPORTED;
}
bf_status_t BfRtTableObj::ghostTableHandleSet(const pipe_tbl_hdl_t & /*hdl*/) {
  LOG_ERROR("%s:%d API Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableObj::dataFieldTypeGet(
    const bf_rt_id_t &field_id, std::set<DataFieldType> *ret_type) const {
  return this->dataFieldTypeGet(field_id, 0, ret_type);
}

bf_status_t BfRtTableObj::dataFieldTypeGet(
    const bf_rt_id_t &field_id,
    const bf_rt_id_t &action_id,
    std::set<DataFieldType> *ret_type) const {
  if (ret_type == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return status;
  }
  *ret_type = field->getTypes();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::getResourceInternal(
    const DataFieldType &field_type, bf_rt_table_ref_info_t *tbl_ref) const {
  // right now we return the first ref info back only since
  // we do not have provision for multiple refs
  switch (field_type) {
    case (DataFieldType::COUNTER_INDEX):
    case (DataFieldType::COUNTER_SPEC_BYTES):
    case (DataFieldType::COUNTER_SPEC_PACKETS): {
      auto vec = tableGetRefNameVec("statistics_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return BF_SUCCESS;
      }
      break;
    }

    case (DataFieldType::REGISTER_SPEC):
    case (DataFieldType::REGISTER_SPEC_HI):
    case (DataFieldType::REGISTER_SPEC_LO):
    case (DataFieldType::REGISTER_INDEX): {
      auto vec = tableGetRefNameVec("stateful_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return BF_SUCCESS;
      }
      break;
    }

    case (DataFieldType::LPF_INDEX):
    case (DataFieldType::LPF_SPEC_TYPE):
    case (DataFieldType::LPF_SPEC_GAIN_TIME_CONSTANT):
    case (DataFieldType::LPF_SPEC_DECAY_TIME_CONSTANT):
    case (DataFieldType::LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR):
    case (DataFieldType::WRED_INDEX):
    case (DataFieldType::WRED_SPEC_TIME_CONSTANT):
    case (DataFieldType::WRED_SPEC_MIN_THRESHOLD):
    case (DataFieldType::WRED_SPEC_MAX_THRESHOLD):
    case (DataFieldType::WRED_SPEC_MAX_PROBABILITY):
    case (DataFieldType::METER_INDEX):
    case (DataFieldType::METER_SPEC_CIR_PPS):
    case (DataFieldType::METER_SPEC_PIR_PPS):
    case (DataFieldType::METER_SPEC_CBS_PKTS):
    case (DataFieldType::METER_SPEC_PBS_PKTS):
    case (DataFieldType::METER_SPEC_CIR_KBPS):
    case (DataFieldType::METER_SPEC_PIR_KBPS):
    case (DataFieldType::METER_SPEC_CBS_KBITS):
    case (DataFieldType::METER_SPEC_PBS_KBITS): {
      auto vec = tableGetRefNameVec("meter_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return BF_SUCCESS;
      }
      break;
    }
    default:
      LOG_ERROR("%s:%d %s ERROR: Wrong dataFieldType provided",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return BF_OBJECT_NOT_FOUND;
  }
  return BF_OBJECT_NOT_FOUND;
}

pipe_tbl_hdl_t BfRtTableObj::getResourceHdl(
    const DataFieldType &field_type) const {
  bf_rt_table_ref_info_t tbl_ref;
  bf_status_t status = getResourceInternal(field_type, &tbl_ref);
  if (status != BF_SUCCESS) {
    return 0;
  }
  return tbl_ref.tbl_hdl;
}

pipe_tbl_hdl_t BfRtTableObj::getIndirectResourceHdl(
    const DataFieldType &field_type) const {
  bf_rt_table_ref_info_t tbl_ref;
  bf_status_t status = getResourceInternal(field_type, &tbl_ref);
  if (status != BF_SUCCESS) {
    return 0;
  }
  if (tbl_ref.indirect_ref) {
    return tbl_ref.tbl_hdl;
  }
  return 0;
}

bf_status_t BfRtTableObj::getKeyField(const bf_rt_id_t &field_id,
                                      const BfRtTableKeyField **field) const {
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d Field ID %d not found", __func__, __LINE__, field_id);
    return BF_OBJECT_NOT_FOUND;
  }
  *field = key_fields.at(field_id).get();
  return BF_SUCCESS;
}

bf_status_t BfRtTableObj::getDataField(const bf_rt_id_t &field_id,
                                       const BfRtTableDataField **field) const {
  return this->getDataField(field_id, 0, field);
}

bf_status_t BfRtTableObj::getDataField(const bf_rt_id_t &field_id,
                                       const bf_rt_id_t &action_id,
                                       const BfRtTableDataField **field) const {
  std::vector<bf_rt_id_t> empty;
  return this->getDataField(field_id, action_id, empty, field);
}

bf_status_t BfRtTableObj::getDataField(
    const bf_rt_id_t &field_id,
    const bf_rt_id_t &action_id,
    const std::vector<bf_rt_id_t> &container_id_vec,
    const BfRtTableDataField **field) const {
  *field = nullptr;
  if (action_info_list.find(action_id) != action_info_list.end()) {
    *field =
        this->getDataFieldHelper(field_id,
                                 container_id_vec,
                                 0,
                                 action_info_list.at(action_id)->data_fields);
    if (*field) return BF_SUCCESS;
  }
  // We need to search the common data too
  *field = this->getDataFieldHelper(
      field_id, container_id_vec, 0, common_data_fields);
  if (*field) return BF_SUCCESS;
  // Logging only warning so as to not overwhelm logs. Users supposed to check
  // error code if something wrong
  LOG_WARN("%s:%d Data field ID %d actionID %d not found",
           __func__,
           __LINE__,
           field_id,
           action_id);
  return BF_OBJECT_NOT_FOUND;
}

const BfRtTableDataField *BfRtTableObj::getDataFieldHelper(
    const bf_rt_id_t &field_id,
    const std::vector<bf_rt_id_t> &container_id_vec,
    const uint32_t depth,
    const std::map<bf_rt_id_t, std::unique_ptr<BfRtTableDataField>> &field_map)
    const {
  if (field_map.find(field_id) != field_map.end()) {
    return field_map.at(field_id).get();
  }
  // iterate all fields and recursively search for the data field
  // if this field is a container field and it exists in the contaienr
  // set provided by caller.
  // If container ID vector is empty, then just try and find first
  // instance
  for (const auto &p : field_map) {
    if (((container_id_vec.size() >= depth + 1 &&
          container_id_vec[depth] == p.first) ||
         container_id_vec.empty()) &&
        p.second.get()->isContainerValid()) {
      auto field = this->getDataFieldHelper(field_id,
                                            container_id_vec,
                                            depth + 1,
                                            p.second.get()->containerMapGet());
      if (field) return field;
    }
  }
  return nullptr;
}

bf_status_t BfRtTableObj::getDataField(
    const std::string &field_name,
    const bf_rt_id_t &action_id,
    const std::vector<bf_rt_id_t> &container_id_vec,
    const BfRtTableDataField **field) const {
  *field = nullptr;
  if (action_info_list.find(action_id) != action_info_list.end()) {
    *field = this->getDataFieldHelper(
        field_name,
        container_id_vec,
        0,
        action_info_list.at(action_id)->data_fields_names);
    if (*field) return BF_SUCCESS;
  }
  // We need to search the common data too
  *field = this->getDataFieldHelper(
      field_name, container_id_vec, 0, common_data_fields_names);
  if (*field) return BF_SUCCESS;
  // Logging only warning so as to not overwhelm logs. Users supposed to check
  // error code if something wrong
  // TODO change this to WARN
  LOG_TRACE("%s:%d Data field name %s actionID %d not found",
            __func__,
            __LINE__,
            field_name.c_str(),
            action_id);
  return BF_OBJECT_NOT_FOUND;
}

const BfRtTableDataField *BfRtTableObj::getDataFieldHelper(
    const std::string &field_name,
    const std::vector<bf_rt_id_t> &container_id_vec,
    const uint32_t depth,
    const std::map<std::string, BfRtTableDataField *> &field_map) const {
  if (field_map.find(field_name) != field_map.end()) {
    return field_map.at(field_name);
  }
  // iterate all fields and recursively search for the data field
  // if this field is a container field and it exists in the contaienr
  // vector provided by caller.
  // If container ID vector is empty, then just try and find first
  // instance
  for (const auto &p : field_map) {
    if (((container_id_vec.size() >= depth + 1 &&
          container_id_vec[depth] == p.second->getId()) ||
         container_id_vec.empty()) &&
        p.second->isContainerValid()) {
      auto field = this->getDataFieldHelper(field_name,
                                            container_id_vec,
                                            depth + 1,
                                            p.second->containerNameMapGet());
      if (field) return field;
    }
  }
  return nullptr;
}

pipe_act_fn_hdl_t BfRtTableObj::getActFnHdl(const bf_rt_id_t &action_id) const {
  auto elem = action_info_list.find(action_id);
  if (elem == action_info_list.end()) {
    return 0;
  }
  return elem->second->act_fn_hdl;
}

bf_rt_id_t BfRtTableObj::getActIdFromActFnHdl(
    const pipe_act_fn_hdl_t &act_fn_hdl) const {
  auto elem = act_fn_hdl_to_id.find(act_fn_hdl);
  if (elem == act_fn_hdl_to_id.end()) {
    LOG_ERROR("%s:%d %s Action hdl %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              act_fn_hdl);
    BF_RT_ASSERT(0);
  }
  return elem->second;
}

void BfRtTableObj::addDataFieldType(const bf_rt_id_t &field_id,
                                    const DataFieldType &type) {
  // This method adds a dataField Type to the given field id
  auto dataField = common_data_fields.find(field_id);
  if (dataField == common_data_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    BF_RT_ASSERT(0);
  }
  BfRtTableDataField *field = dataField->second.get();
  field->addDataFieldType(type);
  return;
}

const std::string &BfRtTableObj::table_name_get() const { return object_name; }

bf_rt_id_t BfRtTableObj::table_id_get() const {
  bf_rt_id_t id;
  auto bf_status = tableIdGet(&id);
  if (bf_status == BF_SUCCESS) {
    return id;
  }
  return 0;
}
const std::string &BfRtTableObj::key_field_name_get(
    const bf_rt_id_t &field_id) const {
  if (key_fields.find(field_id) == key_fields.end()) {
    LOG_ERROR("%s:%d %s Field ID %d not found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              field_id);
    return bfRtNullStr;
  }
  return key_fields.at(field_id)->getName();
}

const std::string &BfRtTableObj::data_field_name_get(
    const bf_rt_id_t &field_id) const {
  return this->data_field_name_get(field_id, 0);
}

const std::string &BfRtTableObj::data_field_name_get(
    const bf_rt_id_t &field_id, const bf_rt_id_t &action_id) const {
  const BfRtTableDataField *field;
  auto status = getDataField(field_id, action_id, &field);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d Field ID %d Action ID %d not found",
              __func__,
              __LINE__,
              field_id,
              action_id);
    return bfRtNullStr;
  }
  return field->getName();
}

const std::string &BfRtTableObj::action_name_get(
    const bf_rt_id_t &action_id) const {
  if (action_info_list.find(action_id) == action_info_list.end()) {
    LOG_ERROR("%s:%d %s Action Id %d Not Found",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              action_id);
    return bfRtNullStr;
  }
  return action_info_list.at(action_id)->name;
}

size_t BfRtTableObj::getMaxdataSz() const {
  size_t max_sz = 0;
  for (const auto &each_action : action_info_list) {
    size_t this_sz = each_action.second->dataSz;
    if (this_sz > max_sz) {
      max_sz = this_sz;
    }
  }
  return max_sz;
}

size_t BfRtTableObj::getMaxdataSzbits() const {
  size_t max_sz = 0;
  for (const auto &each_action : action_info_list) {
    size_t this_sz = each_action.second->dataSzbits;
    if (this_sz > max_sz) {
      max_sz = this_sz;
    }
  }
  return max_sz;
}

size_t BfRtTableObj::getdataSz(bf_rt_id_t act_id) const {
  auto elem = action_info_list.find(act_id);
  if (elem == action_info_list.end()) {
    BF_RT_ASSERT(0);
    return 0;
  }
  return elem->second->dataSz;
}

size_t BfRtTableObj::getdataSzbits(bf_rt_id_t act_id) const {
  auto elem = action_info_list.find(act_id);
  if (elem == action_info_list.end()) {
    BF_RT_ASSERT(0);
    return 0;
  }
  return elem->second->dataSzbits;
}

bool BfRtTableObj::validateTable_from_keyObj(const BfRtTableKeyObj &key) const {
  bf_rt_id_t table_id;
  bf_rt_id_t table_id_of_key_obj;

  const BfRtTable *table_from_key = nullptr;
  bf_status_t bf_status = this->tableIdGet(&table_id);
  if (bf_status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table id, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    BF_RT_DBGCHK(0);
    return false;
  }

  bf_status = key.tableGet(&table_from_key);
  if (bf_status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table object from key object, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    BF_RT_DBGCHK(0);
    return false;
  }

  bf_status = table_from_key->tableIdGet(&table_id_of_key_obj);
  if (bf_status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table id from key object, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    BF_RT_DBGCHK(0);
    return false;
  }

  if (table_id == table_id_of_key_obj) {
    return true;
  }
  return false;
}

bool BfRtTableObj::validateTable_from_dataObj(
    const BfRtTableDataObj &data) const {
  bf_rt_id_t table_id;
  bf_rt_id_t table_id_of_data_obj;

  bf_status_t bf_status = this->tableIdGet(&table_id);
  if (bf_status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table id, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    BF_RT_DBGCHK(0);
    return false;
  }

  const BfRtTable *table_from_data = nullptr;
  bf_status = data.getParent(&table_from_data);
  if (bf_status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table object from data object, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    BF_RT_DBGCHK(0);
    return false;
  }

  bf_status = table_from_data->tableIdGet(&table_id_of_data_obj);
  if (bf_status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR in getting table id from data object, err %d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              bf_status);
    BF_RT_DBGCHK(0);
    return false;
  }

  if (table_id == table_id_of_data_obj) {
    return true;
  }
  return false;
}

void BfRtTableObj::getActionResources(const bf_rt_id_t action_id,
                                      bool *meter,
                                      bool *reg,
                                      bool *cntr) const {
  *cntr = *meter = *reg = false;
  // Will default to false if action_id does not exist.
  if (this->act_uses_dir_cntr.find(action_id) !=
      this->act_uses_dir_cntr.end()) {
    *cntr = this->act_uses_dir_cntr.at(action_id);
  }
  if (this->act_uses_dir_reg.find(action_id) != this->act_uses_dir_reg.end()) {
    *reg = this->act_uses_dir_reg.at(action_id);
  }
  if (this->act_uses_dir_meter.find(action_id) !=
      this->act_uses_dir_meter.end()) {
    *meter = this->act_uses_dir_meter.at(action_id);
  }
}

bf_status_t BfRtTableObj::getKeyStringChoices(
    const std::string &key_str, std::vector<std::string> &choices) const {
  bf_rt_id_t key_id;
  auto status = this->keyFieldIdGet(key_str, &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_str.c_str());
    return status;
  }

  std::vector<std::reference_wrapper<const std::string>> key_str_choices;
  status = this->keyFieldAllowedChoicesGet(key_id, &key_str_choices);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get string choices for key field Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_id);
    return status;
  }

  for (auto key_str_choice : key_str_choices) {
    choices.push_back(key_str_choice.get());
  }

  return status;
}

bf_status_t BfRtTableObj::getDataStringChoices(
    const std::string &data_str, std::vector<std::string> &choices) const {
  bf_rt_id_t data_id;
  auto status = this->dataFieldIdGet(data_str, &data_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              data_str.c_str());
    return status;
  }

  std::vector<std::reference_wrapper<const std::string>> data_str_choices;
  status = this->dataFieldAllowedChoicesGet(data_id, &data_str_choices);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get string choices for data field Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              data_id);
    return status;
  }

  for (auto data_str_choice : data_str_choices) {
    choices.push_back(data_str_choice.get());
  }

  return status;
}
}  // bfrt

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
#ifndef _BF_RT_PORT_TABLE_IMPL_HPP
#define _BF_RT_PORT_TABLE_IMPL_HPP

#include <bf_rt_common/bf_rt_table_impl.hpp>
#include <bf_rt_port/dpdk/bf_rt_port_table_data_impl.hpp>
#include <bf_rt_port/bf_rt_port_table_key_impl.hpp>

namespace bfrt {

class BfRtInfo;
/*
 * BfRtPortCfgTable
 * BfRtPortStatTable
 */
class BfRtPortCfgTable : public BfRtTableObj {
 public:
  BfRtPortCfgTable(const std::string &program_name,
                   bf_rt_id_t id,
                   std::string name,
                   const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PORT_CFG,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::DELETE,
                     }) {
    mapInit();
  };
  bf_status_t tableEntryAdd(const BfRtSession & /*session*/,
                            const bf_rt_target_t &dev_tgt,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override;
  bf_status_t tableEntryDel(const BfRtSession & /*session*/,
                            const bf_rt_target_t &dev_tgt,
                            const BfRtTableKey &key) const override;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;

  bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                        BfRtTableData *data) const override final;

  // Attribute APIs
  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
  bf_status_t tableAttributesSet(
      const BfRtSession & /*session*/,
      const bf_rt_target_t &dev_tgt,
      const BfRtTableAttributes &tableAttributes) const override;

 private:
  void mapInit();
  std::map<std::string, dpdk_port_type_t> portTypeMap;
  std::map<std::string, bf_pm_port_dir_e> portDirMap;
};

class BfRtPortStatTable : public BfRtTableObj {
 public:
  BfRtPortStatTable(const std::string &program_name,
                    bf_rt_id_t id,
                    std::string name,
                    const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::PORT_STAT,
                     std::set<TableApi>{TableApi::GET,}){};

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const BfRtTableKey &key,
                            const BfRtTable::BfRtTableGetFlag &flag,
                            BfRtTableData *data) const override;
  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  bf_status_t dataReset(BfRtTableData *data) const override final;

  bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                        BfRtTableData *data) const override final;

  // Attribute APIs
  bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;
 private:
  bf_status_t tableEntryGet_internal(const bf_rt_target_t &dev_tgt,
                                     const uint32_t &dev_port,
                                     const BfRtTable::BfRtTableGetFlag &flag,
                                     BfRtPortStatTableData *data) const;
};

}  // bfrt

#endif  // _BF_RT_PORT_TABLE_IMPL_HPP

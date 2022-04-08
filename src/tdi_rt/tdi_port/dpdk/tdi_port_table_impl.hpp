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
#ifndef _TDI_PORT_TABLE_IMPL_HPP
#define _TDI_PORT_TABLE_IMPL_HPP

//#include <tdi_rt/tdi_port/tdi_table_impl.hpp>
//#include <tdi_port/dpdk/tdi_port_table_data_impl.hpp>
#include <tdi_rt/tdi_port/dpdk/tdi_port_table_data_impl.hpp>
#include <tdi_rt/tdi_port/tdi_port_table_key_impl.hpp>
//#include <tdi_rt/tdi_dpdk_defs.h>

namespace tdi {

class TdiInfo;
/*
 * PortCfgTable
 * PortStatTable
 */

class PortCfgTable : public Table {
 public:
  PortCfgTable(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info) :
               tdi::Table(tdi_info, table_info) {
    mapInit();
    LOG_DBG("Creating PortCfgTable table for %s", table_info->nameGet().c_str());
  };

  tdi_status_t entryAdd(const Session & /*session*/,
                        const Target &dev_tgt,
			const Flags &flags,
                        const TableKey &key,
                        const TableData &data) const override;

  tdi_status_t entryDel(const Session & /*session*/,
                        const Target &dev_tgt,
			const Flags &flags,
                        const TableKey &key) const override;

  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<TableData> *data_ret) const override;

  tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<TableData> *data_ret) const /*override*/;

  tdi_status_t keyReset(TableKey *key) const override final;

  //tdi_status_t dataReset(TableData *data) const override final;
  tdi_status_t dataReset(TableData *data) const;
  //tdi_status_t dataReset(const std::vector<tdi_id_t> &fields,
  //                      TableData *data) const override final;
  tdi_status_t dataReset(const std::vector<tdi_id_t> &fields,
                         TableData *data) const;
  // Attribute APIs
#if 0
  // wait for attribute core class defined.
  tdi_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;
  tdi_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;
  tdi_status_t tableAttributesSet(
      const Session & /*session*/,
      const Target &dev_tgt,
      const TableAttributes &tableAttributes) const override;
#endif

 private:
  void mapInit();
  std::map<std::string, dpdk_port_type_t> portTypeMap;
  std::map<std::string, bf_pm_port_dir_e> portDirMap;
  
  tdi_status_t entryAdd(const Session & /*session*/,
                        const Target & /*dev_tgt*/,
                        const TableKey &key,
                        const TableData &data) const;
};

class PortStatTable : public Table {
 public:
  PortStatTable(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info) :
                tdi::Table(tdi_info, table_info) {
    LOG_DBG("Creating PortStatTable table for %s", table_info->nameGet().c_str());
  };

  tdi_status_t entryGet(const Session &session,
                        const Target &dev_tgt,
			const Flags &flag,
                        const TableKey &key,
                        //const Table::TableGetFlag &flag,
                        TableData *data) const override;
  tdi_status_t keyAllocate(
      std::unique_ptr<TableKey> *key_ret) const override;

  tdi_status_t dataAllocate(
      std::unique_ptr<TableData> *data_ret) const override;

  tdi_status_t dataAllocate(
      const std::vector<tdi_id_t> &fields,
      std::unique_ptr<TableData> *data_ret) const override;

  tdi_status_t dataReset(TableData *data) const override final;
  tdi_status_t dataReset(const std::vector<tdi_id_t> &fields,
                         TableData *data) const override final;
#if 0
  // Attribute APIs
  tdi_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<TableAttributes> *attr) const override;
#endif

 private:
  tdi_status_t entryGet_internal(const Target &dev_tgt,
                                 const uint32_t &dev_port,
                                 PortStatTableData *data) const;
};
}  // tdi

#endif  // _TDI_PORT_TABLE_IMPL_HPP

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

#ifndef _TDI_PORT_TABLE_DATA_IMPL_HPP
#define _TDI_PORT_TABLE_DATA_IMPL_HPP

#include <unordered_set>
#include <tdi/common/tdi_utils.hpp>
#include <tdi/common/tdi_defs.h>
#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_table_data.hpp>

#include "bf_rt_port/bf_rt_port_mgr_intf.hpp"
//#include "tdi_common/tdi_table_data_impl.hpp"
//#include "tdi_common/tdi_table_impl.hpp"

namespace tdi {

class PortCfgTableData : public TableData {
 public:
  // TableDataObj
  PortCfgTableData(const Table *tbl_obj,
                       const std::vector<tdi_id_t> &fields)
      : TableData(tbl_obj) {
    set_active_fields(fields);
  }
  ~PortCfgTableData() = default;
  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value) override final;

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override;

  tdi_status_t setValue(const tdi_id_t &field_id, const bool &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id, const std::string &str) override;

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id, bool *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id, std::string *str) const override;

  //tdi_status_t reset(const std::vector<tdi_id_t> &fields) override final;
  //tdi_status_t reset() override final;
  tdi_status_t reset(const std::vector<tdi_id_t> &fields);
  tdi_status_t reset();

  // unexposed API
  const std::unordered_set<tdi_id_t> &getActiveDataFields() const {
    return activeFields;
  }
  const std::unordered_map<tdi_id_t, bool> &getBoolFieldDataMap() const {
    return boolFieldData;
  }
  const std::unordered_map<tdi_id_t, uint32_t> &getU32FieldDataMap() const {
    return u32FieldData;
  }
  const std::unordered_map<tdi_id_t, std::string> &getStrFieldDataMap()
      const {
    return strFieldData;
  }

 private:
  tdi_status_t setU32ValueInternal(const tdi_id_t &field_id,
                                   const uint64_t &value,
                                   const uint8_t *value_ptr,
                                   const size_t &s);
  tdi_status_t getU32ValueInternal(const tdi_id_t &field_id,
                                   uint64_t *value,
                                   uint8_t *value_ptr,
                                   const size_t &s) const;
  tdi_status_t set_active_fields(const std::vector<tdi_id_t> &fields);
  bool checkFieldActive(const tdi_id_t &field_id,
                        const tdi_field_data_type_e dataType) const;
  std::set<tdi_id_t> fieldPresent;
  std::unordered_map<tdi_id_t, bool> boolFieldData;
  std::unordered_map<tdi_id_t, uint32_t> u32FieldData;
  std::unordered_map<tdi_id_t, std::string> strFieldData;
  std::unordered_set<tdi_id_t> activeFields;
};

class PortStatTableData : public TableData {
 public:
#if 0
  PortStatTableData(const Table *tbl_obj, tdi_id_t action_id)
      : table_(tbl_obj), action_id_(action_id) {
    //this->actionIdSet(action_id);
    std::set<tdi_id_t> empty;
    this->setActiveFields(empty);
    set_active_fields(empty);
  };
  PortStatTableData(const Table *tbl_obj)
      : PortStatTableData(tbl_obj, 0){};
  PortStatTableData(const Table *tbl_obj,
                        const std::vector<tdi_id_t> &fields)
      : PortStatTableData(tbl_obj) {
    std::memset(u64FieldDataArray, 0, BF_PORT_NUM_COUNTERS * sizeof(uint64_t));
    set_active_fields(fields);
  }
#endif
  PortStatTableData(const Table *tbl_obj,
                    const std::vector<tdi_id_t> &fields)
      : TableData(tbl_obj) {
    std::memset(u64FieldDataArray, 0, BF_PORT_NUM_COUNTERS * sizeof(uint64_t));
    set_active_fields(fields);
  }

  ~PortStatTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);
  // need to override virtual base class
  tdi_status_t setValue(const tdi_id_t &field_id,
                               const std::string &str) { return TDI_NOT_SUPPORTED; };

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  // need to override virtual base class
  tdi_status_t getValue(const tdi_id_t &field_id,
                               bool *value) const { return TDI_NOT_SUPPORTED; };
  tdi_status_t getValue(const tdi_id_t &field_id,
                               std::string *str) const { return TDI_NOT_SUPPORTED; };

  //tdi_status_t reset(const std::set<tdi_id_t> &fields) override final;
  //tdi_status_t reset() override final;
  tdi_status_t reset(const std::vector<tdi_id_t> &fields);
  tdi_status_t reset();

  // unexposed API
  void setAllValues(const uint64_t *stats);
  const std::vector<tdi_id_t> &getActiveDataFields() const {
    return activeFields;
  }
  const uint64_t *getU64FieldData() const { return u64FieldDataArray; }
  const uint32_t &getAllStatsBoundry() const { return AllStatsBoundry; }

 private:
  tdi_status_t set_active_fields(const std::vector<tdi_id_t> &fields);

  tdi_status_t setU64ValueInternal(const tdi_id_t &field_id,
                                  const uint64_t &value,
                                  const uint8_t *value_ptr,
                                  const size_t &s);
  tdi_status_t getU64ValueInternal(const tdi_id_t &field_id,
                                  uint64_t *value,
                                  uint8_t *value_ptr,
                                  const size_t &s) const;
  bool all_fields_set;
  std::set<tdi_id_t> fieldPresent;
  uint64_t u64FieldDataArray[BF_PORT_NUM_COUNTERS];
  std::vector<tdi_id_t> activeFields;
  const uint32_t AllStatsBoundry = 20;
};

}  // tdi
#endif  // _TDI_PORT_TABLE_DATA_IMPL_HPP

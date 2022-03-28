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

#include <arpa/inet.h>
#include <inttypes.h>

#include "tdi/common/tdi_defs.h"
#include "tdi_port_table_data_impl.hpp"
#include <tdi_rt/tdi_port/tdi_table_data_utils.hpp>
#include <tdi/common/tdi_utils.hpp>

namespace tdi {
namespace {
template <class T>
tdi_status_t getDevPortValue(const T &data,
                            const Table &table,
                            const tdi_id_t &field_id,
                            uint64_t *value,
                            uint8_t *value_ptr,
                            const size_t &size) {
  tdi_status_t status = BF_SUCCESS;
  DataFieldInfo *tableDataField = nullptr;

  auto tableInfo = table.tableInfoGet();
  tableDataField = tableInfo->dataFieldGet(field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return BF_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  auto sts = utils::TableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, value, value_ptr, size);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  uint32_t dev_port;
  data.getDevPort(&dev_port);
  uint64_t val = static_cast<uint64_t>(dev_port);
  if (value_ptr) {
    utils::TableDataUtils::toNetworkOrderData(
        *tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return BF_SUCCESS;
}

template <class T>
tdi_status_t setDevPortValue(T &data,
                            const Table &table,
                            const tdi_id_t &field_id,
                            const uint64_t &value,
                            const uint8_t *value_ptr,
                            const size_t &size) {
  tdi_status_t status = BF_SUCCESS;
  DataFieldInfo *tableDataField = nullptr;
  auto tableInfo = table.tableInfoGet();
  tableDataField = tableInfo->dataFieldGet(field_id);

  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return BF_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  auto sts = utils::TableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, &value, value_ptr, size);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  sts = utils::TableDataUtils::boundsCheck(
      *tableDataField, value, value_ptr, size);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              tableDataField->idGet());
    return sts;
  }
  uint64_t local_val;
  if (value_ptr) {
    utils::TableDataUtils::toHostOrderData(
        *tableDataField, value_ptr, &local_val);
  } else {
    local_val = value;
  }
  const uint32_t dev_port = static_cast<uint32_t>(local_val);
  data.setDevPort(dev_port);
  return BF_SUCCESS;
}
}  // anonymous namespace

// Port Cfg Table Data
tdi_status_t PortCfgTableData::reset(const std::vector<tdi_id_t> &fields) {
  fieldPresent.clear();
  boolFieldData.clear();
  u32FieldData.clear();
  strFieldData.clear();
  active_fields_.clear();
  return this->set_active_fields(fields);
}

tdi_status_t PortCfgTableData::reset() {
  std::vector<tdi_id_t> emptyfield;
  return this->reset(emptyfield);
}

tdi_status_t PortCfgTableData::set_active_fields(
    const std::vector<tdi_id_t> &fields) {
  if (fields.empty()) {
    this->all_fields_set_ = true;
  } else {
    for (const auto &field : fields) {
      if (active_fields_.find(field) != active_fields_.end()) {
        LOG_ERROR(
            "%s:%d ERROR Field id %d specified multiple times for data "
            "allocate",
            __func__,
            __LINE__,
            field);
        return BF_INVALID_ARG;
      }
      active_fields_.insert(field);
    }
    all_fields_set_ = false;
  }
  return BF_SUCCESS;
}

bool PortCfgTableData::checkFieldActive(const tdi_id_t &field_id,
                                        const tdi_field_data_type_e dataType) const {
  tdi_status_t status = BF_SUCCESS;
  auto tableInfo = this->table_->tableInfoGet();
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return false;
  }
  if (all_fields_set_) return true;
  auto elem1 = active_fields_.find(field_id);
  if (elem1 == active_fields_.end()) {
    LOG_ERROR("ERROR: %s:%d Inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return false;
  }
  if (dataType == TDI_FIELD_DATA_TYPE_BOOL) {
    // bool
    auto elem2 = boolFieldData.find(field_id);
    if (elem2 == boolFieldData.end()) return true;
  } else if (dataType == TDI_FIELD_DATA_TYPE_UINT64) {
    // uint32_t
    auto elem2 = u32FieldData.find(field_id);
    if (elem2 == u32FieldData.end()) return true;
  } else if (dataType == TDI_FIELD_DATA_TYPE_STRING) {
    // string
    auto elem2 = strFieldData.find(field_id);
    if (elem2 == strFieldData.end()) return true;
  }
  LOG_ERROR("ERROR: %s:%d Field id %d has already been assigned for table %s",
            __func__,
            __LINE__,
            field_id,
            tableInfo->nameGet().c_str());
  return false;
}

tdi_status_t PortCfgTableData::setU32ValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t &s) {
  tdi_status_t status = BF_SUCCESS;
  auto tableInfo = this->table_->tableInfoGet(); 
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return BF_INVALID_ARG;
  }
  // Do some bounds checking using the utility functions
  auto sts = utils::TableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, &value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }

  sts = utils::TableDataUtils::boundsCheck(
      *tableDataField, value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  if (!checkFieldActive(field_id, TDI_FIELD_DATA_TYPE_UINT64)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              (this->table_->tableInfoGet())->nameGet().c_str());
    return BF_INVALID_ARG;
  }
  uint64_t val;
  if (value_ptr) {
    utils::TableDataUtils::toHostOrderData(
        *tableDataField, value_ptr, &val);
  } else {
    val = value;
  }
  u32FieldData[field_id] = val;
  return BF_SUCCESS;
}

tdi_status_t PortCfgTableData::setValue(const tdi_id_t &field_id,
                                        const uint64_t &value) {
  return setU32ValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t PortCfgTableData::setValue(const tdi_id_t &field_id,
                                        const uint8_t *value,
                                        const size_t &size) {
  return setU32ValueInternal(field_id, 0, value, size);
}

tdi_status_t PortCfgTableData::setValue(const tdi_id_t &field_id,
                                        const bool &value) {
  tdi_status_t status = BF_SUCCESS;
  const DataFieldInfo *tableDataField = nullptr;
  tableDataField = (this->table_->tableInfoGet())->dataFieldGet(field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              (this->table_->tableInfoGet())->nameGet().c_str());
    return BF_INVALID_ARG;
  }
  // Next, check if this setter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        (this->table_->tableInfoGet())->nameGet().c_str());
    return BF_NOT_SUPPORTED;
  }
  if (!checkFieldActive(field_id, TDI_FIELD_DATA_TYPE_BOOL)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              (this->table_->tableInfoGet())->nameGet().c_str());
    return BF_INVALID_ARG;
  }
  boolFieldData[field_id] = value;
  return BF_SUCCESS;
}

tdi_status_t PortCfgTableData::setValue(const tdi_id_t &field_id,
                                        const std::string &str) {
  tdi_status_t status = BF_SUCCESS;
  //DataFieldInfo *tableDataField = nullptr;
  auto tableInfo = this->table_->tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              (this->table_->tableInfoGet())->nameGet().c_str());
              //this->table_->nameGet().c_str());
    return BF_INVALID_ARG;
  }
  // Next, check if this setter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string or enum",
        __func__,
        __LINE__,
        field_id,
        (this->table_->tableInfoGet())->nameGet().c_str());
    return BF_NOT_SUPPORTED;
  }
  if (!checkFieldActive(field_id, TDI_FIELD_DATA_TYPE_STRING)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              (this->table_->tableInfoGet())->nameGet().c_str());
    return BF_INVALID_ARG;
  }
  strFieldData[field_id] = str;
  return BF_SUCCESS;
}

tdi_status_t PortCfgTableData::getU32ValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value,
    uint8_t *value_ptr,
    const size_t &s) const {
  tdi_status_t status = BF_SUCCESS;
  //DataFieldInfo *tableDataField = nullptr;
  auto tableInfo = this->table_->tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              tableInfo->nameGet().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  status = utils::TableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, value, value_ptr, s);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  auto elem = u32FieldData.find(field_id);
  if (elem == u32FieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              (this->table_->tableInfoGet())->nameGet().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  uint64_t val = elem->second;
  if (value_ptr) {
    utils::TableDataUtils::toNetworkOrderData(
        *tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return BF_SUCCESS;
}

tdi_status_t PortCfgTableData::getValue(const tdi_id_t &field_id,
                                           uint64_t *value) const {
  return getU32ValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t PortCfgTableData::getValue(const tdi_id_t &field_id,
                                           const size_t &size,
                                           uint8_t *value) const {
  return getU32ValueInternal(field_id, nullptr, value, size);
}

tdi_status_t PortCfgTableData::getValue(const tdi_id_t &field_id,
                                           bool *value) const {
  tdi_status_t status = BF_SUCCESS;
  const DataFieldInfo *tableDataField = nullptr;
  auto tableInfo = this->table_->tableInfoGet();
  tableDataField = tableInfo->dataFieldGet(field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              (this->table_->tableInfoGet())->nameGet().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  // Next, check if this getter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        (this->table_->tableInfoGet())->nameGet().c_str());
    return BF_NOT_SUPPORTED;
  }
  auto elem = boolFieldData.find(field_id);
  if (elem == boolFieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              (this->table_->tableInfoGet())->nameGet().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  *value = elem->second;
  return BF_SUCCESS;
}

tdi_status_t PortCfgTableData::getValue(const tdi_id_t &field_id,
                                           std::string *str) const {
  tdi_status_t status = BF_SUCCESS;
  const DataFieldInfo *tableDataField = nullptr;
  auto tableInfo = this->table_->tableInfoGet();
  tableDataField = tableInfo->dataFieldGet(field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              (this->table_->tableInfoGet())->nameGet().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  // Next, check if this getter can be used
  if (tableDataField->dataTypeGet() != TDI_FIELD_DATA_TYPE_STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string nor enum",
        __func__,
        __LINE__,
        field_id,
        (this->table_->tableInfoGet())->nameGet().c_str());
    return BF_NOT_SUPPORTED;
  }
  auto elem = strFieldData.find(field_id);
  if (elem == strFieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              (this->table_->tableInfoGet())->nameGet().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  *str = elem->second;
  return BF_SUCCESS;
}

// Port Stat Table Data
tdi_status_t PortStatTableData::reset(
    const std::vector<tdi_id_t> &fields) {
  fieldPresent.clear();
  std::memset(u64FieldDataArray, 0, sizeof(u64FieldDataArray));
  active_fields_.clear();
  return this->set_active_fields(fields);
}

tdi_status_t PortStatTableData::reset() {
  std::vector<tdi_id_t> emptyfield;
  return this->reset(emptyfield);
}

tdi_status_t PortStatTableData::set_active_fields(
    const std::vector<tdi_id_t> &fields) {
  if (fields.empty()) {
    all_fields_set_ = true;
  } else {
    for (const auto &field : fields) {
      auto elem = fieldPresent.find(field);
      if (elem == fieldPresent.end()) {
        fieldPresent.insert(field);
      } else {
        LOG_ERROR(
            "%s:%d ERROR Field id %d specified multiple times for data "
            "allocate",
            __func__,
            __LINE__,
            field);
        return BF_INVALID_ARG;
      }
    }
    active_fields_ = fields;
    all_fields_set_ = false;
  }
  return BF_SUCCESS;
}

void PortStatTableData::setAllValues(const uint64_t *stats) {
  if (stats == NULL) return;
  std::memcpy(
      u64FieldDataArray, stats, BF_PORT_NUM_COUNTERS * sizeof(uint64_t));
}

tdi_status_t PortStatTableData::setU64ValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t &s) {
  //tdi_status_t status = BF_SUCCESS;
  //DataFieldInfo *tableDataField = nullptr;
  auto tableInfo = this->table_->tableInfoGet();
  auto tableDataField = tableInfo->dataFieldGet(field_id);
  /*
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              tableInfo->nameGet().c_str());
    return BF_INVALID_ARG;
  }
  */
  // Do some bounds checking using the utility functions
  auto sts = utils::TableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, &value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        tableInfo->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }

  sts = utils::TableDataUtils::boundsCheck(
      *tableDataField, value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        (this->table_->tableInfoGet())->nameGet().c_str(),
        tableDataField->idGet());
    return sts;
  }
  if ((all_fields_set_ == false) &&
      (fieldPresent.find(field_id) == fieldPresent.end())) {
    LOG_ERROR("ERROR: %s:%d Inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              (this->table_->tableInfoGet())->nameGet().c_str());
    return BF_INVALID_ARG;
  }
  uint64_t val;
  if (value_ptr) {
    utils::TableDataUtils::toHostOrderData(
        *tableDataField, value_ptr, &val);
  } else {
    val = value;
  }
  u64FieldDataArray[field_id - 1] = val;
  return BF_SUCCESS;
}

tdi_status_t PortStatTableData::getU64ValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value,
    uint8_t *value_ptr,
    const size_t &s) const {
  tdi_status_t status = BF_SUCCESS;
  const DataFieldInfo *tableDataField = nullptr;
  auto tableInfo = this->table_->tableInfoGet();
  tableDataField = tableInfo->dataFieldGet(field_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              (this->table_->tableInfoGet())->nameGet().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  status = utils::TableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, value, value_ptr, s);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        (this->table_->tableInfoGet())->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
  if ((all_fields_set_ == false) &&
      (fieldPresent.find(field_id) == fieldPresent.end())) {
    LOG_ERROR("ERROR: %s:%d Inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              (this->table_->tableInfoGet())->nameGet().c_str());
    return BF_INVALID_ARG;
  }
  uint64_t val = u64FieldDataArray[field_id - 1];
  if (value_ptr) {
    utils::TableDataUtils::toNetworkOrderData(
        *tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return BF_SUCCESS;
}

tdi_status_t PortStatTableData::setValue(const tdi_id_t &field_id,
                                            const uint64_t &value) {
  return setU64ValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t PortStatTableData::setValue(const tdi_id_t &field_id,
                                            const uint8_t *value,
                                            const size_t &size) {
  return setU64ValueInternal(field_id, 0, value, size);
}

tdi_status_t PortStatTableData::getValue(const tdi_id_t &field_id,
                                            uint64_t *value) const {
  return getU64ValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t PortStatTableData::getValue(const tdi_id_t &field_id,
                                            const size_t &size,
                                            uint8_t *value) const {
  return getU64ValueInternal(field_id, nullptr, value, size);
}

}  // tdi

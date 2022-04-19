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

#include "bf_rt_port_table_data_impl.hpp"
#include <bf_rt_common/bf_rt_table_data_utils.hpp>
#include <bf_rt_common/bf_rt_utils.hpp>

namespace bfrt {
namespace {
template <class T>
bf_status_t getDevPortValue(const T &data,
                            const BfRtTableObj &table,
                            const bf_rt_id_t &field_id,
                            uint64_t *value,
                            uint8_t *value_ptr,
                            const size_t &size) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = table.getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  auto sts = utils::BfRtTableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, value, value_ptr, size);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }
  uint32_t dev_port;
  data.getDevPort(&dev_port);
  uint64_t val = static_cast<uint64_t>(dev_port);
  if (value_ptr) {
    utils::BfRtTableDataUtils::toNetworkOrderData(
        *tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return BF_SUCCESS;
}

template <class T>
bf_status_t setDevPortValue(T &data,
                            const BfRtTableObj &table,
                            const bf_rt_id_t &field_id,
                            const uint64_t &value,
                            const uint8_t *value_ptr,
                            const size_t &size) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;

  status = table.getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              table.table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  auto sts = utils::BfRtTableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, &value, value_ptr, size);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }
  sts = utils::BfRtTableDataUtils::boundsCheck(
      *tableDataField, value, value_ptr, size);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              table.table_name_get().c_str(),
              tableDataField->getId());
    return sts;
  }
  uint64_t local_val;
  if (value_ptr) {
    utils::BfRtTableDataUtils::toHostOrderData(
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
bf_status_t BfRtPortCfgTableData::reset(const std::vector<bf_rt_id_t> &fields) {
  fieldPresent.clear();
  boolFieldData.clear();
  u32FieldData.clear();
  strFieldData.clear();
  activeFields.clear();
  return this->set_active_fields(fields);
}

bf_status_t BfRtPortCfgTableData::reset() {
  std::vector<bf_rt_id_t> emptyfield;
  return this->reset(emptyfield);
}

bf_status_t BfRtPortCfgTableData::set_active_fields(
    const std::vector<bf_rt_id_t> &fields) {
  if (fields.empty()) {
    all_fields_set = true;
  } else {
    for (const auto &field : fields) {
      if (activeFields.find(field) != activeFields.end()) {
        LOG_ERROR(
            "%s:%d ERROR Field id %d specified multiple times for data "
            "allocate",
            __func__,
            __LINE__,
            field);
        return BF_INVALID_ARG;
      }
      activeFields.insert(field);
    }
    all_fields_set = false;
  }
  return BF_SUCCESS;
}

bool BfRtPortCfgTableData::checkFieldActive(const bf_rt_id_t &field_id,
                                            const DataType &dataType) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return false;
  }
  if (all_fields_set) return true;
  auto elem1 = activeFields.find(field_id);
  if (elem1 == activeFields.end()) {
    LOG_ERROR("ERROR: %s:%d Inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return false;
  }
  if (dataType == DataType::BOOL) {
    // bool
    auto elem2 = boolFieldData.find(field_id);
    if (elem2 == boolFieldData.end()) return true;
  } else if (dataType == DataType::UINT64) {
    // uint32_t
    auto elem2 = u32FieldData.find(field_id);
    if (elem2 == u32FieldData.end()) return true;
  } else if (dataType == DataType::STRING) {
    // string
    auto elem2 = strFieldData.find(field_id);
    if (elem2 == strFieldData.end()) return true;
  }
  LOG_ERROR("ERROR: %s:%d Field id %d has already been assigned for table %s",
            __func__,
            __LINE__,
            field_id,
            this->table_->table_name_get().c_str());
  return false;
}

bf_status_t BfRtPortCfgTableData::setU32ValueInternal(
    const bf_rt_id_t &field_id,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t &s) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Do some bounds checking using the utility functions
  auto sts = utils::BfRtTableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, &value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }

  sts = utils::BfRtTableDataUtils::boundsCheck(
      *tableDataField, value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }
  if (!checkFieldActive(field_id, DataType::UINT64)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  uint64_t val;
  if (value_ptr) {
    utils::BfRtTableDataUtils::toHostOrderData(
        *tableDataField, value_ptr, &val);
  } else {
    val = value;
  }
  u32FieldData[field_id] = val;
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTableData::setValue(const bf_rt_id_t &field_id,
                                           const uint64_t &value) {
  return setU32ValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtPortCfgTableData::setValue(const bf_rt_id_t &field_id,
                                           const uint8_t *value,
                                           const size_t &size) {
  return setU32ValueInternal(field_id, 0, value, size);
}

bf_status_t BfRtPortCfgTableData::setValue(const bf_rt_id_t &field_id,
                                           const bool &value) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Next, check if this setter can be used
  if (tableDataField->getDataType() != DataType::BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  if (!checkFieldActive(field_id, DataType::BOOL)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  boolFieldData[field_id] = value;
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTableData::setValue(const bf_rt_id_t &field_id,
                                           const std::string &str) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Next, check if this setter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string or enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  if (!checkFieldActive(field_id, DataType::STRING)) {
    LOG_ERROR("ERROR: %s:%d Set inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  strFieldData[field_id] = str;
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTableData::getU32ValueInternal(
    const bf_rt_id_t &field_id,
    uint64_t *value,
    uint8_t *value_ptr,
    const size_t &s) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  status = utils::BfRtTableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, value, value_ptr, s);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return status;
  }
  auto elem = u32FieldData.find(field_id);
  if (elem == u32FieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  uint64_t val = elem->second;
  if (value_ptr) {
    utils::BfRtTableDataUtils::toNetworkOrderData(
        *tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTableData::getValue(const bf_rt_id_t &field_id,
                                           uint64_t *value) const {
  return getU32ValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtPortCfgTableData::getValue(const bf_rt_id_t &field_id,
                                           const size_t &size,
                                           uint8_t *value) const {
  return getU32ValueInternal(field_id, nullptr, value, size);
}

bf_status_t BfRtPortCfgTableData::getValue(const bf_rt_id_t &field_id,
                                           bool *value) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  // Next, check if this getter can be used
  if (tableDataField->getDataType() != DataType::BOOL) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  auto elem = boolFieldData.find(field_id);
  if (elem == boolFieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  *value = elem->second;
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTableData::getValue(const bf_rt_id_t &field_id,
                                           std::string *str) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  // Next, check if this getter can be used
  if (tableDataField->getDataType() != DataType::STRING) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a string nor enum",
        __func__,
        __LINE__,
        field_id,
        this->table_->table_name_get().c_str());
    return BF_NOT_SUPPORTED;
  }
  auto elem = strFieldData.find(field_id);
  if (elem == strFieldData.end()) {
    LOG_ERROR("ERROR: %s:%d %s : field id %d has no valid data",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  *str = elem->second;
  return BF_SUCCESS;
}

// Port Stat Table Data
bf_status_t BfRtPortStatTableData::reset(
    const std::vector<bf_rt_id_t> &fields) {
  fieldPresent.clear();
  std::memset(u64FieldDataArray, 0, sizeof(u64FieldDataArray));
  activeFields.clear();
  return this->set_active_fields(fields);
}

bf_status_t BfRtPortStatTableData::reset() {
  std::vector<bf_rt_id_t> emptyfield;
  return this->reset(emptyfield);
}

bf_status_t BfRtPortStatTableData::set_active_fields(
    const std::vector<bf_rt_id_t> &fields) {
  if (fields.empty()) {
    all_fields_set = true;
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
    activeFields = fields;
    all_fields_set = false;
  }
  return BF_SUCCESS;
}

void BfRtPortStatTableData::setAllValues(const uint64_t *stats) {
  if (stats == NULL) return;
  std::memcpy(
      u64FieldDataArray, stats, BF_PORT_NUM_COUNTERS * sizeof(uint64_t));
}

bf_status_t BfRtPortStatTableData::setU64ValueInternal(
    const bf_rt_id_t &field_id,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t &s) {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  // Do some bounds checking using the utility functions
  auto sts = utils::BfRtTableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, &value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }

  sts = utils::BfRtTableDataUtils::boundsCheck(
      *tableDataField, value, value_ptr, s);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return sts;
  }
  if ((all_fields_set == false) &&
      (fieldPresent.find(field_id) == fieldPresent.end())) {
    LOG_ERROR("ERROR: %s:%d Inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  uint64_t val;
  if (value_ptr) {
    utils::BfRtTableDataUtils::toHostOrderData(
        *tableDataField, value_ptr, &val);
  } else {
    val = value;
  }
  u64FieldDataArray[field_id - 1] = val;
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTableData::getU64ValueInternal(
    const bf_rt_id_t &field_id,
    uint64_t *value,
    uint8_t *value_ptr,
    const size_t &s) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTableDataField *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR %s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              field_id);
    return BF_INVALID_ARG;
  }
  status = utils::BfRtTableDataUtils::fieldTypeCompatibilityCheck(
      *tableDataField, value, value_ptr, s);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->table_name_get().c_str(),
        tableDataField->getId());
    return status;
  }
  if ((all_fields_set == false) &&
      (fieldPresent.find(field_id) == fieldPresent.end())) {
    LOG_ERROR("ERROR: %s:%d Inactive field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  uint64_t val = u64FieldDataArray[field_id - 1];
  if (value_ptr) {
    utils::BfRtTableDataUtils::toNetworkOrderData(
        *tableDataField, val, value_ptr);
  } else {
    *value = val;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTableData::setValue(const bf_rt_id_t &field_id,
                                            const uint64_t &value) {
  return setU64ValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtPortStatTableData::setValue(const bf_rt_id_t &field_id,
                                            const uint8_t *value,
                                            const size_t &size) {
  return setU64ValueInternal(field_id, 0, value, size);
}

bf_status_t BfRtPortStatTableData::getValue(const bf_rt_id_t &field_id,
                                            uint64_t *value) const {
  return getU64ValueInternal(field_id, value, nullptr, 0);
}

bf_status_t BfRtPortStatTableData::getValue(const bf_rt_id_t &field_id,
                                            const size_t &size,
                                            uint8_t *value) const {
  return getU64ValueInternal(field_id, nullptr, value, size);
}

}  // bfrt

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

#include <bitset>
#include <arpa/inet.h>
#include <inttypes.h>

#include <bf_rt_common/bf_rt_utils.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>
#include "bf_rt_port_table_key_impl.hpp"

namespace bfrt {

namespace {
inline bf_status_t getKeyFieldSafe(const bf_rt_id_t &field_id,
                                   const BfRtTableKeyField **key_field,
                                   const KeyFieldType &key_field_type_req,
                                   const BfRtTableObj *table) {
  auto status = table->getKeyField(field_id, key_field);
  if (status != BF_SUCCESS) {
    return status;
  }

  // If the key_field type is not what is required here, then just return
  // error
  if ((*key_field)->getType() != key_field_type_req) {
    LOG_ERROR(
        "%s:%d %s Wrong API called for this field type,"
        " field_id %d expected type: %d received type: %d",
        __func__,
        __LINE__,
        table->table_name_get().c_str(),
        field_id,
        static_cast<int>((*key_field)->getType()),
        static_cast<int>(key_field_type_req));
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

template <class T>
bf_status_t getKeyIdxValue(const T &key,
                           const BfRtTableObj &table,
                           const bf_rt_id_t &field_id,
                           uint64_t *value) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, &table);
  if (status != BF_SUCCESS) {
    return status;
  }
  *value = key.getId();
  return BF_SUCCESS;
}

template <class T>
bf_status_t getKeyIdxValue(const T &key,
                           const BfRtTableObj &table,
                           const bf_rt_id_t &field_id,
                           uint8_t *value,
                           const size_t &size) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, &table);
  if (status != BF_SUCCESS) {
    return status;
  }
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        size,
        sizeof(uint32_t),
        field_id);
    return BF_INVALID_ARG;
  }
  uint32_t local_val = htobe32(key.getId());
  std::memcpy(value, &local_val, sizeof(uint32_t));
  return BF_SUCCESS;
}

}  // anonymous namespace

bf_status_t BfRtPortCfgTableKey::reset() {
  dev_port_ = 0;
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTableKey::setValue(const bf_rt_id_t &field_id,
                                          const uint64_t &value) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }
  dev_port_ = value;
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTableKey::setValue(const bf_rt_id_t &field_id,
                                          const uint8_t *value,
                                          const size_t &size) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint32_t),
        field_id);
    return BF_INVALID_ARG;
  }
  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  val = be32toh(val);
  dev_port_ = val;
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTableKey::getValue(const bf_rt_id_t &field_id,
                                          uint64_t *value) const {
  return getKeyIdxValue<BfRtPortCfgTableKey>(*this, *table_, field_id, value);
}

bf_status_t BfRtPortCfgTableKey::getValue(const bf_rt_id_t &field_id,
                                          const size_t &size,
                                          uint8_t *value) const {
  return getKeyIdxValue<BfRtPortCfgTableKey>(
      *this, *table_, field_id, value, size);
}

bf_status_t BfRtPortStatTableKey::reset() {
  dev_port_ = 0;
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTableKey::setValue(const bf_rt_id_t &field_id,
                                           const uint64_t &value) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }
  dev_port_ = value;
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTableKey::setValue(const bf_rt_id_t &field_id,
                                           const uint8_t *value,
                                           const size_t &size) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint32_t),
        field_id);
    return BF_INVALID_ARG;
  }
  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  val = be32toh(val);
  dev_port_ = val;
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTableKey::getValue(const bf_rt_id_t &field_id,
                                           uint64_t *value) const {
  return getKeyIdxValue<BfRtPortStatTableKey>(*this, *table_, field_id, value);
}

bf_status_t BfRtPortStatTableKey::getValue(const bf_rt_id_t &field_id,
                                           const size_t &size,
                                           uint8_t *value) const {
  return getKeyIdxValue<BfRtPortStatTableKey>(
      *this, *table_, field_id, value, size);
}

bf_status_t BfRtPortHdlInfoTableKey::reset() {
  conn_id_ = 0;
  chnl_id_ = 0;
  return BF_SUCCESS;
}

bf_status_t BfRtPortHdlInfoTableKey::setValue(const bf_rt_id_t &field_id,
                                              const uint64_t &value) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (key_field->getName() == "$CONN_ID") {
    conn_id_ = value;
  } else if (key_field->getName() == "$CHNL_ID") {
    chnl_id_ = value;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortHdlInfoTableKey::setValue(const bf_rt_id_t &field_id,
                                              const uint8_t *value,
                                              const size_t &size) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint32_t),
        field_id);
    return BF_INVALID_ARG;
  }
  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  val = be32toh(val);
  if (key_field->getName() == "$CONN_ID") {
    conn_id_ = val;
  } else if (key_field->getName() == "$CHNL_ID") {
    chnl_id_ = val;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortHdlInfoTableKey::getValue(const bf_rt_id_t &field_id,
                                              uint64_t *value) const {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }
  if (key_field->getName() == "$CONN_ID") {
    *value = conn_id_;
  } else if (key_field->getName() == "$CHNL_ID") {
    *value = chnl_id_;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortHdlInfoTableKey::getValue(const bf_rt_id_t &field_id,
                                              const size_t &size,
                                              uint8_t *value) const {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint32_t),
        field_id);
    return BF_INVALID_ARG;
  }
  uint32_t local_val = 0;
  if (key_field->getName() == "$CONN_ID") {
    local_val = conn_id_;
  } else if (key_field->getName() == "$CHNL_ID") {
    local_val = chnl_id_;
  }
  local_val = htobe32(local_val);
  std::memcpy(value, &local_val, sizeof(uint32_t));
  return BF_SUCCESS;
}

bf_status_t BfRtPortHdlInfoTableKey::getPortHdl(uint32_t *conn_id,
                                                uint32_t *chnl_id) const {
  if ((conn_id != NULL) && (chnl_id != NULL)) {
    *conn_id = conn_id_;
    *chnl_id = chnl_id_;
    return BF_SUCCESS;
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtPortFpIdxInfoTableKey::reset() {
  fp_idx_ = 0;
  return BF_SUCCESS;
}

bf_status_t BfRtPortFpIdxInfoTableKey::setValue(const bf_rt_id_t &field_id,
                                                const uint64_t &value) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }
  fp_idx_ = value;
  return BF_SUCCESS;
}

bf_status_t BfRtPortFpIdxInfoTableKey::setValue(const bf_rt_id_t &field_id,
                                                const uint8_t *value,
                                                const size_t &size) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->table_name_get().c_str(),
        size,
        sizeof(uint32_t),
        field_id);
    return BF_INVALID_ARG;
  }
  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  val = be32toh(val);
  fp_idx_ = val;
  return BF_SUCCESS;
}

bf_status_t BfRtPortFpIdxInfoTableKey::getValue(const bf_rt_id_t &field_id,
                                                uint64_t *value) const {
  return getKeyIdxValue<BfRtPortFpIdxInfoTableKey>(
      *this, *table_, field_id, value);
}

bf_status_t BfRtPortFpIdxInfoTableKey::getValue(const bf_rt_id_t &field_id,
                                                const size_t &size,
                                                uint8_t *value) const {
  return getKeyIdxValue<BfRtPortFpIdxInfoTableKey>(
      *this, *table_, field_id, value, size);
}

bf_status_t BfRtPortStrInfoTableKey::setValue(const bf_rt_id_t &field_id,
                                              const std::string &value) {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }
  port_str_ = value;
  return BF_SUCCESS;
}

bf_status_t BfRtPortStrInfoTableKey::getValue(const bf_rt_id_t &field_id,
                                              std::string *value) const {
  // Get the key_field from the table
  const BfRtTableKeyField *key_field;
  auto status =
      getKeyFieldSafe(field_id, &key_field, KeyFieldType::EXACT, table_);
  if (status != BF_SUCCESS) {
    return status;
  }
  *value = port_str_;
  return BF_SUCCESS;
}

}  // bfrt

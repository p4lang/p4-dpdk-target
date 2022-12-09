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

#include <bitset>
#include <arpa/inet.h>
#include <inttypes.h>

#include <tdi/common/tdi_utils.hpp>
#include <tdi_common/tdi_table_field_utils.hpp>

#include "tdi_p4_table_key_impl.hpp"

namespace tdi {
namespace pna {
namespace rt {
// template setValue helper for Action Profile, Selector, Counter and Meter
template <typename Key>
tdi_status_t setValueHelper(Key *key,
                            const tdi_id_t &field_id,
                            const tdi::KeyFieldValue &field_value) {
  const tdi::Table *table;
  auto status = key->tableGet(&table);
  // Get the key_field from the table
  const KeyFieldInfo *key_field;
  status = utils::TableFieldUtils::keyFieldSafeGet(
      field_id, &key_field, &field_value, table);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (field_value.matchTypeGet() !=
      static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_EXACT)) {
    return TDI_INVALID_ARG;
  }

  if (field_value.is_pointer()) {
    auto e_fv = static_cast<const tdi::KeyFieldValueExact<const uint8_t *> *>(
        &field_value);
    return key->setValue(key_field, e_fv->value_, e_fv->size_);
  } else {
    auto e_fv = static_cast<const tdi::KeyFieldValueExact<const uint64_t> *>(
        &field_value);
    return key->setValue(key_field, e_fv->value_);
  }

  return TDI_UNEXPECTED;
}
	
// template getValue helper for Action Profile, Selector, Counter and Meter
template <typename Key>
tdi_status_t getValueHelper(const Key *key,
                            const tdi_id_t &field_id,
                            tdi::KeyFieldValue *field_value) {
  const tdi::Table *table;
  auto status = key->tableGet(&table);

  if (!field_value) {
    LOG_ERROR("%s:%d %s input param passed null for key field_id %d, ",
              __func__,
              __LINE__,
              table->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  // Get the key_field from the table
  const KeyFieldInfo *key_field;
  status = utils::TableFieldUtils::keyFieldSafeGet(
      field_id, &key_field, field_value, table);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (field_value->matchTypeGet() !=
      static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_EXACT)) {
    return TDI_INVALID_ARG;
  }

  if (field_value->is_pointer()) {
    auto e_fv = static_cast<tdi::KeyFieldValueExact<uint8_t *> *>(field_value);
    return key->getValue(key_field, e_fv->size_, e_fv->value_);
  } else {
    auto e_fv = static_cast<tdi::KeyFieldValueExact<uint64_t> *>(field_value);
    return key->getValue(key_field, &e_fv->value_);
  }

  return TDI_UNEXPECTED;
}

template <class T>
tdi_status_t getKeyIdxValue(const T &key,
                           uint64_t *value) {
  *value = key.getIdxKey();
  return TDI_SUCCESS;
}
	

// MatchActionKey *****************
tdi_status_t MatchActionKey::setValue(const tdi_id_t &field_id,
                                      const tdi::KeyFieldValue &field_value) {
  const KeyFieldInfo *key_field;
  auto status = utils::TableFieldUtils::keyFieldSafeGet(
      field_id, &key_field, &field_value, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (field_value.matchTypeGet() ==
      static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_EXACT)) {
    if (field_value.is_pointer()) {
      auto e_fv = static_cast<const tdi::KeyFieldValueExact<const uint8_t *> *>(
          &field_value);
      return this->setValue(key_field, e_fv->value_, e_fv->size_);
    } else {
      auto e_fv = static_cast<const tdi::KeyFieldValueExact<const uint64_t> *>(
          &field_value);
      return this->setValue(key_field, e_fv->value_);
    }
  } else if (field_value.matchTypeGet() ==
             static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_TERNARY)) {
    if (field_value.is_pointer()) {
      auto e_fv =
          static_cast<const tdi::KeyFieldValueTernary<const uint8_t *> *>(
              &field_value);
      return this->setValueandMask(
          key_field, e_fv->value_, e_fv->mask_, e_fv->size_);
    } else {
      auto e_fv =
          static_cast<const tdi::KeyFieldValueTernary<const uint64_t> *>(
              &field_value);
      return this->setValueandMask(key_field, e_fv->value_, e_fv->mask_);
    }
  } else if (field_value.matchTypeGet() ==
             static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_RANGE)) {
    if (field_value.is_pointer()) {
      auto e_fv = static_cast<const tdi::KeyFieldValueRange<const uint8_t *> *>(
          &field_value);
      return this->setValueRange(
          key_field, e_fv->low_, e_fv->high_, e_fv->size_);
    } else {
      auto e_fv = static_cast<const tdi::KeyFieldValueRange<const uint64_t> *>(
          &field_value);
      return this->setValueRange(key_field, e_fv->low_, e_fv->high_);
    }
  } else if (field_value.matchTypeGet() ==
             static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_LPM)) {
    if (field_value.is_pointer()) {
      auto e_fv = static_cast<const tdi::KeyFieldValueLPM<const uint8_t *> *>(
          &field_value);
      return this->setValueLpm(
          key_field, e_fv->value_, e_fv->prefix_len_, e_fv->size_);
    } else {
      auto e_fv = static_cast<const tdi::KeyFieldValueLPM<const uint64_t> *>(
          &field_value);
      return this->setValueLpm(key_field, e_fv->value_, e_fv->prefix_len_);
    }
  }
  return TDI_UNEXPECTED;
}

tdi_status_t MatchActionKey::getValue(const tdi_id_t &field_id,
                                      tdi::KeyFieldValue *field_value) const {
  if (!field_value) {
    LOG_ERROR("%s:%d %s input param passed null for key field_id %d, ",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  const KeyFieldInfo *key_field;
  auto status = utils::TableFieldUtils::keyFieldSafeGet(
      field_id, &key_field, field_value, table_);
  if (status != TDI_SUCCESS) {
    return status;
  }

  if (field_value->matchTypeGet() ==
      static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_EXACT)) {
    if (field_value->is_pointer()) {
      auto e_fv =
          static_cast<tdi::KeyFieldValueExact<uint8_t *> *>(field_value);
      return this->getValue(key_field, e_fv->size_, e_fv->value_);
    } else {
      auto e_fv = static_cast<tdi::KeyFieldValueExact<uint64_t> *>(field_value);
      return this->getValue(key_field, &e_fv->value_);
    }
  } else if (field_value->matchTypeGet() ==
             static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_TERNARY)) {
    if (field_value->is_pointer()) {
      auto e_fv =
          static_cast<tdi::KeyFieldValueTernary<uint8_t *> *>(field_value);
      return this->getValueandMask(
          key_field, e_fv->size_, e_fv->value_, e_fv->mask_);
    } else {
      auto e_fv =
          static_cast<tdi::KeyFieldValueTernary<uint64_t> *>(field_value);
      return this->getValueandMask(key_field, &e_fv->value_, &e_fv->mask_);
    }
  } else if (field_value->matchTypeGet() ==
             static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_RANGE)) {
    if (field_value->is_pointer()) {
      auto e_fv =
          static_cast<tdi::KeyFieldValueRange<uint8_t *> *>(field_value);
      return this->getValueRange(
          key_field, e_fv->size_, e_fv->low_, e_fv->high_);
    } else {
      auto e_fv = static_cast<tdi::KeyFieldValueRange<uint64_t> *>(field_value);
      return this->getValueRange(key_field, &e_fv->low_, &e_fv->high_);
    }
  } else if (field_value->matchTypeGet() ==
             static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_LPM)) {
    if (field_value->is_pointer()) {
      auto e_fv = static_cast<tdi::KeyFieldValueLPM<uint8_t *> *>(field_value);
      return this->getValueLpm(
          key_field, e_fv->size_, e_fv->value_, &e_fv->prefix_len_);
    } else {
      auto e_fv = static_cast<tdi::KeyFieldValueLPM<uint64_t> *>(field_value);
      return this->getValueLpm(key_field, &e_fv->value_, &e_fv->prefix_len_);
    }
  }
  return TDI_UNEXPECTED;
}

tdi_status_t MatchActionKey::setValue(const tdi::KeyFieldInfo *key_field,
                                      const uint64_t &value) {
  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field->keyFieldContextInfoGet());

  if (key_context->isMatchPriority()) {
    priority = value;
  } else {
    auto status = utils::TableFieldUtils::boundsCheck(
        *table_, *key_field, value, nullptr, 0);
    if (status != TDI_SUCCESS) {
      return status;
    }
    // ceil the size and convert bits to bytes
    auto size_bytes = (key_field->sizeGet() + 7) / 8;
    // endianness conversion
    auto new_value = htobe64(value);
    // Here we did a 64 bit host to big-endian conversion. When copying to
    // the byte array, need to advance the pointer to the value array based
    // on the field size
    // The value array is 8 bytes long. For instance for a 4 byte field, we
    // need to advance the "value" pointer by (8 - 4) bytes, so that it
    // points to the beginning of the field
    // For instance, field value in little endian - 0xCDAB. When invoked
    // with htobe64 becomes
    // 0x00 0x00 0x00 0x00 0x00 0x00 0xAB 0xCD. We need to copy 0x0000ABCD
    // into the array
    // since its a 4 byte field, thus we advance the value pointer by 4
    // bytes
    uint32_t src_offset = (8 - size_bytes);

    uint8_t *val_ptr = ((uint8_t *)&new_value) + src_offset;
    this->setValueInternal(*key_field, val_ptr, size_bytes);
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::setValue(const tdi::KeyFieldInfo *key_field,
                                      const uint8_t *value,
                                      const size_t &size) {
  auto status =
      utils::TableFieldUtils::boundsCheck(*table_, *key_field, 0, value, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field->keyFieldContextInfoGet());
  size_t size_bytes = (key_field->sizeGet() + 7) / 8;
  if (key_context->isMatchPriority()) {
    uint64_t pri = 0;
    utils::TableFieldUtils::toHostOrderData(*key_field, value, &pri);
    this->priority = static_cast<uint32_t>(pri);
  } else {
    this->setValueInternal(*key_field, value, size_bytes);
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::setValueandMask(const tdi::KeyFieldInfo *key_field,
                                             const uint64_t &value,
                                             const uint64_t &mask) {
  auto status = utils::TableFieldUtils::boundsCheck(
      *table_, *key_field, value, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  status = utils::TableFieldUtils::boundsCheck(
      *table_, *key_field, mask, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  auto field_size = key_field->sizeGet();
  // ceil the size and convert bits to bytes
  auto size_bytes = (field_size + 7) / 8;

  // endianness conversion
  auto new_value = htobe64(value);
  auto new_mask = htobe64(mask);
  uint32_t src_offset = (8 - size_bytes);
  uint8_t *val_ptr = ((uint8_t *)&new_value) + src_offset;
  uint8_t *msk_ptr = ((uint8_t *)&new_mask) + src_offset;
  setValueAndMaskInternal(*key_field, val_ptr, msk_ptr, size_bytes, true);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::setValueandMask(const tdi::KeyFieldInfo *key_field,
                                             const uint8_t *value,
                                             const uint8_t *mask,
                                             const size_t &size) {
  size_t size_bytes = (key_field->sizeGet() + 7) / 8;
  auto status =
      utils::TableFieldUtils::boundsCheck(*table_, *key_field, 0, value, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  status =
      utils::TableFieldUtils::boundsCheck(*table_, *key_field, 0, mask, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  setValueAndMaskInternal(*key_field, value, mask, size_bytes, true);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::setValueRange(const tdi::KeyFieldInfo *key_field,
                                           const uint64_t &start,
                                           const uint64_t &end) {
  auto field_size = key_field->sizeGet();
  auto status = utils::TableFieldUtils::boundsCheck(
      *table_, *key_field, start, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  status =
      utils::TableFieldUtils::boundsCheck(*table_, *key_field, end, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  // ceil the size and convert bits to bytes
  auto size_bytes = (field_size + 7) / 8;
  // endianness conversion
  auto new_value = htobe64(start);
  auto new_mask = htobe64(end);
  uint32_t src_offset = (8 - size_bytes);
  uint8_t *val_ptr = ((uint8_t *)&new_value) + src_offset;
  uint8_t *msk_ptr = ((uint8_t *)&new_mask) + src_offset;
  // For Range keyfields, tell setValueAndMaskInternal not to mask the key with
  // the mask
  setValueAndMaskInternal(*key_field, val_ptr, msk_ptr, size_bytes, false);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::setValueRange(const tdi::KeyFieldInfo *key_field,
                                           const uint8_t *start,
                                           const uint8_t *end,
                                           const size_t &size) {
  size_t size_bytes = (key_field->sizeGet() + 7) / 8;
  auto status =
      utils::TableFieldUtils::boundsCheck(*table_, *key_field, 0, start, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  status =
      utils::TableFieldUtils::boundsCheck(*table_, *key_field, 0, end, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  setValueAndMaskInternal(*key_field, start, end, size_bytes, false);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::setValueLpm(const tdi::KeyFieldInfo *key_field,
                                         const uint64_t &value,
                                         const uint16_t &p_length) {
  if (p_length > key_field->sizeGet()) {
    LOG_ERROR(
        "%s:%d %s ERROR : Prefix length (%d) cannot be greater than field "
        "size(%zu "
        "bits) for field %s",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        p_length,
        key_field->sizeGet(),
        key_field->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  auto status = utils::TableFieldUtils::boundsCheck(
      *table_, *key_field, value, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  // The mask initially is kept all FFs. Let prefix_length = 44, field_size = 48
  // Then we right shift 64-44 = 20 bits and then left shift 48-44 = 4 bits
  // so finally (16 x 0s)(44 x 1s)(4 x 0s), which is what we want.
  uint64_t mask = -1;
  auto field_size = key_field->sizeGet();
  if (p_length) {
    mask = (mask >> (64 - p_length)) << (field_size - p_length);
  } else {
    mask = 0;
  }
  status = utils::TableFieldUtils::boundsCheck(
      *table_, *key_field, mask, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }

  priority = field_size - p_length;

  // ceil the size and convert bits to bytes
  auto size_bytes = (field_size + 7) / 8;
  // endianness conversion
  // Here we did a 64 bit host to big-endian conversion. When copying to
  // the byte array, need to advance the pointer to the value array based
  // on the field size
  // The value array is 8 bytes long. For instance for a 2 byte field, we
  // need to advance the "value" pointer by (8 - 2) bytes, so that it
  // points to the beginning of the field
  // For instance, field value in little endian - 0xCDAB. When invoked
  // with htobe64 becomes
  // 0x00 0x00 0x00 0x00 0x00 0x00 0xAB 0xCD. We need to copy 0x0000ABCD
  // into the array
  // since its a 2 byte field, thus we advance the value pointer by 6
  // bytes
  auto new_value = htobe64(value);
  auto new_mask = htobe64(mask);
  uint32_t src_offset = (8 - size_bytes);
  uint8_t *val_ptr = ((uint8_t *)&new_value) + src_offset;
  uint8_t *msk_ptr = ((uint8_t *)&new_mask) + src_offset;
  setValueAndMaskInternal(*key_field, val_ptr, msk_ptr, size_bytes, true);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::setValueLpm(const tdi::KeyFieldInfo *key_field,
                                         const uint8_t *value1,
                                         const uint16_t &p_length,
                                         const size_t &size) {
  if (p_length > key_field->sizeGet()) {
    LOG_ERROR(
        "%s:%d %s ERROR : Prefix length (%u) cannot be greater than length of "
        "field "
        "(%zu) for field %s",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        p_length,
        key_field->sizeGet(),
        key_field->nameGet().c_str());
    return TDI_INVALID_ARG;
  }
  size_t size_bytes = (key_field->sizeGet() + 7) / 8;
  std::vector<uint8_t> mask(size_bytes, 0xff);

  // If prefix length is 115 and the size is 128 bits, then we need to
  // change 13 bits from the end of the vector to 0. So 1 byte and 5 bits

  uint32_t total_bits_flip = key_field->sizeGet() - p_length;  // 128 - 115
  uint32_t bytes_flip = total_bits_flip / 8;                   // 13 / 8
  uint32_t bits_flip = total_bits_flip % 8;                    // 13 % 8

  for (uint32_t i = 0; i < bytes_flip; i++) {
    mask[mask.size() - 1 - i] = 0;
  }
  if (bits_flip != 0) {
    mask[mask.size() - 1 - bytes_flip] <<= bits_flip;
  }
  priority = key_field->sizeGet() - p_length;
  // Check size compatibility for TERNARY
  auto status =
      utils::TableFieldUtils::boundsCheck(*table_, *key_field, 0, value1, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  setValueAndMaskInternal(*key_field, value1, &mask[0], size_bytes, true);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::setValueOptional(
    const tdi::KeyFieldInfo *key_field,
    const uint64_t &value,
    const bool &is_valid) {
  auto status = utils::TableFieldUtils::boundsCheck(
      *table_, *key_field, value, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  auto field_size = key_field->sizeGet();
  // ceil the size and convert bits to bytes
  auto size_bytes = (field_size + 7) / 8;
  uint64_t mask = 0;
  if (is_valid) {
    mask = (2 ^ field_size) - 1;
  }

  // endianness conversion
  auto new_value = htobe64(value);
  auto new_mask = htobe64(mask);
  uint32_t src_offset = (8 - size_bytes);
  uint8_t *val_ptr = ((uint8_t *)&new_value) + src_offset;
  uint8_t *msk_ptr = ((uint8_t *)&new_mask) + src_offset;
  setValueAndMaskInternal(*key_field, val_ptr, msk_ptr, size_bytes, false);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::setValueOptional(
    const tdi::KeyFieldInfo *key_field,
    const uint8_t *value,
    const bool &is_valid,
    const size_t &size) {
  size_t size_bytes = (key_field->sizeGet() + 7) / 8;
  auto status =
      utils::TableFieldUtils::boundsCheck(*table_, *key_field, 0, value, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  if (is_valid) {
    std::vector<uint8_t> mask(size_bytes, 0xff);
    setValueAndMaskInternal(*key_field, value, &mask[0], size_bytes, false);
  } else {
    std::vector<uint8_t> mask(size_bytes, 0);
    setValueAndMaskInternal(*key_field, value, &mask[0], size_bytes, false);
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::getValue(const tdi::KeyFieldInfo *key_field,
                                      uint64_t *value) const {
  if (key_field->isPtrGet()) {
    LOG_ERROR(
        "%s:%d %s ERROR : Field type is ptr. This API not supported for field "
        "%s",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        key_field->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, value, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field->keyFieldContextInfoGet());
  if (key_context->isMatchPriority()) {
    *value = priority;
  } else {
    // Zero out the out param
    *value = 0;
    // ceil the size and convert bits to bytes
    auto size_bytes = (key_field->sizeGet() + 7) / 8;
    // advance pointer so that the data aligns with the end
    uint8_t *temp_ptr = reinterpret_cast<uint8_t *>(value) + (8 - size_bytes);
    this->getValueInternal(*key_field, temp_ptr, size_bytes);
    *value = be64toh(*value);
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::getValue(const tdi::KeyFieldInfo *key_field,
                                      const size_t &size,
                                      uint8_t *value) const {
  // ceil the size and convert bits to bytes
  auto size_bytes = (key_field->sizeGet() + 7) / 8;
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, nullptr, value, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field->keyFieldContextInfoGet());
  if (key_context->isMatchPriority()) {
    auto temp_priority = htobe32(priority);
    std::memcpy(value, &temp_priority, size_bytes);
  } else {
    this->getValueInternal(*key_field, value, size_bytes);
  }
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::getValueandMask(const tdi::KeyFieldInfo *key_field,
                                             uint64_t *value1,
                                             uint64_t *value2) const {
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, value1, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, value2, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  // Zero out the out-params for sanity
  *value1 = 0;
  *value2 = 0;
  // ceil the size and convert bits to bytes
  auto size_bytes = (key_field->sizeGet() + 7) / 8;
  // advance pointer so that the data aligns with the end
  uint8_t *to_get_val = reinterpret_cast<uint8_t *>(value1) + (8 - size_bytes);
  uint8_t *to_get_msk = reinterpret_cast<uint8_t *>(value2) + (8 - size_bytes);
  getValueAndMaskInternal(*key_field, to_get_val, to_get_msk, size_bytes);
  *value1 = be64toh(*value1);
  *value2 = be64toh(*value2);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::getValueandMask(const tdi::KeyFieldInfo *key_field,
                                             const size_t &size,
                                             uint8_t *value1,
                                             uint8_t *value2) const {
  // ceil the size and convert bits to bytes
  auto size_bytes = (key_field->sizeGet() + 7) / 8;
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, nullptr, value1, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, nullptr, value2, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  getValueAndMaskInternal(*key_field, value1, value2, size_bytes);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::getValueRange(const tdi::KeyFieldInfo *key_field,
                                           uint64_t *start,
                                           uint64_t *end) const {
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, start, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, end, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  // Zero out the out-params for sanity
  *start = 0;
  *end = 0;
  // ceil the size and convert bits to bytes
  auto size_bytes = (key_field->sizeGet() + 7) / 8;
  // advance pointer so that the data aligns with the end
  uint8_t *to_get_start = reinterpret_cast<uint8_t *>(start) + (8 - size_bytes);
  uint8_t *to_get_end = reinterpret_cast<uint8_t *>(end) + (8 - size_bytes);
  getValueAndMaskInternal(*key_field, to_get_start, to_get_end, size_bytes);
  *start = be64toh(*start);
  *end = be64toh(*end);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::getValueRange(const tdi::KeyFieldInfo *key_field,
                                           const size_t &size,
                                           uint8_t *start,
                                           uint8_t *end) const {
  // ceil the size and convert bits to bytes
  auto size_bytes = (key_field->sizeGet() + 7) / 8;
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, nullptr, start, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, nullptr, end, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  getValueAndMaskInternal(*key_field, start, end, size_bytes);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::getValueLpm(const tdi::KeyFieldInfo *key_field,
                                         uint64_t *start,
                                         uint16_t *p_length) const {
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, start, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  // Zero out the out-params for sanity
  *start = 0;
  *p_length = 0;
  // ceil the size and convert bits to bytes
  auto size_bytes = (key_field->sizeGet() + 7) / 8;
  uint8_t *to_get_val = reinterpret_cast<uint8_t *>(start) + (8 - size_bytes);

  // get the mask in network order in a local first,
  // To do that, first we have to advance the start pointer
  // by 8 - size_bytes. Pipe_mgr will store
  // 0x00 0x00 0x00 0x00 0x00 0x00 0xAB 0xCD as 0xABCD
  // so we need to ensure that this goes towards the end of the local
  // After that, it will be in proper network order which we
  // change to host again
  uint64_t mask = 0;
  uint8_t *to_get_msk = reinterpret_cast<uint8_t *>(&mask);
  to_get_msk += (8 - size_bytes);

  getValueAndMaskInternal(*key_field, to_get_val, to_get_msk, size_bytes);
  *start = be64toh(*start);
  mask = be64toh(mask);
  // after we get the mask, need to count the number of bits set

  std::bitset<64> mask_bitset(mask);
  *p_length = mask_bitset.count();
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::getValueLpm(const tdi::KeyFieldInfo *key_field,
                                         const size_t &size,
                                         uint8_t *start,
                                         uint16_t *p_length) const {
  // ceil the size and convert bits to bytes
  auto size_bytes = (key_field->sizeGet() + 7) / 8;
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, nullptr, start, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  std::vector<uint8_t> mask(size_bytes, 0);
  getValueAndMaskInternal(*key_field, start, &mask[0], size_bytes);

  // after we get the mask, need to count the number of bits set
  uint16_t count = 0;
  for (auto const &item : mask) {
    std::bitset<8> mask_bitset(item);
    count += mask_bitset.count();
  }
  *p_length = count;
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::getValueOptional(
    const tdi::KeyFieldInfo *key_field,
    uint64_t *value1,
    bool *is_valid) const {
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, value1, nullptr, 0);
  if (status != TDI_SUCCESS) {
    return status;
  }
  uint64_t mask = 0;
  // Zero out the out-params for sanity
  *value1 = 0;
  // ceil the size and convert bits to bytes
  auto size_bytes = (key_field->sizeGet() + 7) / 8;
  // advance pointer so that the data aligns with the end
  uint8_t *to_get_val = reinterpret_cast<uint8_t *>(value1) + (8 - size_bytes);
  uint8_t *to_get_msk = reinterpret_cast<uint8_t *>(&mask) + (8 - size_bytes);
  getValueAndMaskInternal(*key_field, to_get_val, to_get_msk, size_bytes);
  *value1 = be64toh(*value1);
  mask = be64toh(mask);
  *is_valid = mask == 0 ? false : true;
  return TDI_SUCCESS;
}

tdi_status_t MatchActionKey::getValueOptional(
    const tdi::KeyFieldInfo *key_field,
    const size_t &size,
    uint8_t *value1,
    bool *is_valid) const {
  // ceil the size and convert bits to bytes
  auto size_bytes = (key_field->sizeGet() + 7) / 8;
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *table_, *key_field, nullptr, value1, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  uint64_t mask = 0;
  uint8_t *to_get_msk = reinterpret_cast<uint8_t *>(&mask) + (8 - size_bytes);
  getValueAndMaskInternal(*key_field, value1, to_get_msk, size_bytes);
  *is_valid = mask == 0 ? false : true;
  return TDI_SUCCESS;
}

void MatchActionKey::populate_match_spec(
    pipe_tbl_match_spec_t *pipe_match_spec) const {
  pipe_match_spec->num_valid_match_bits = num_valid_match_bits;
  pipe_match_spec->num_match_bytes = num_valid_match_bytes;
  pipe_match_spec->match_value_bits = key_array;
  pipe_match_spec->match_mask_bits = mask_array;
  pipe_match_spec->priority = priority;
  return;
}

void MatchActionKey::set_key_from_match_spec_by_deepcopy(
    const pipe_tbl_match_spec_t *p) {
  this->num_valid_match_bits = p->num_valid_match_bits;
  this->num_valid_match_bytes = p->num_match_bytes;
  this->priority = p->priority;

  if (key_array) delete[] key_array;
  if (mask_array) delete[] mask_array;
  this->key_array = new uint8_t[this->num_valid_match_bytes];
  this->mask_array = new uint8_t[this->num_valid_match_bytes];

  std::memcpy(this->key_array, p->match_value_bits, p->num_match_bytes);
  std::memcpy(this->mask_array, p->match_mask_bits, p->num_match_bytes);

  return;
}

// Be careful while using this function. This assumes that that the
// pipe_match_spec
// will be persistent in memory, since this is shallow copy
// Also, num_valid_match bits and bytes are supposed to be read-only
// which are set at the time of creation of Key Object, so not touching them
void MatchActionKey::populate_key_from_match_spec(
    const pipe_tbl_match_spec_t &pipe_match_spec) {
  key_array = pipe_match_spec.match_value_bits;
  mask_array = pipe_match_spec.match_mask_bits;
  priority = pipe_match_spec.priority;
  return;
}

// This function extracts the value programmed by the user from the match
// spec byte buf, into the field buf, bit by bit. It extracts each bit
// from the correct location (between the start and end bit) from the
// match spec byte buf one by one and then puts it in the field buf.
// All the byte buffers involved are network ordered
void MatchActionKey::unpackFieldFromMatchSpecByteBuffer(
    const KeyFieldInfo &key_field,
    const size_t &size,
    const uint8_t *match_spec_buf,
    uint8_t *field_buf) const {
  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field.keyFieldContextInfoGet());
  // Get the start bit and the end bit of the concerned field
  size_t start_bit = key_context->startBitGet();
  size_t end_bit = start_bit + key_field.sizeGet() - 1;
  // empty field_buf first
  std::memset(field_buf, 0, size);

  // Extract all the bits between start and end bit from the match spec byte
  // buf into the field buf
  size_t j = 0;
  for (size_t i = start_bit; i <= end_bit; i++, j++) {
    uint8_t temp = (match_spec_buf[key_context->parentFieldFullByteSizeGet() -
                                   (i / 8) - 1] >>
                    (i % 8)) &
                   0x01;

    // Left shift the extracted bit into its correct location
    field_buf[size - (j / 8) - 1] |= (temp << (j % 8));
  }
}

// This function slides in the field buf passed in by the user, in the match
// spec byte buffer at the correct location, bit by bit. It extracts each
// bit of the field buf one by one and then puts it in the correct location
// in the match spec byte buffer. One Important thing to remember is that
// all the byte buffers involved are network ordered
void MatchActionKey::packFieldIntoMatchSpecByteBuffer(
    const KeyFieldInfo &key_field,
    const size_t &size,
    const bool &do_masking,
    const uint8_t *field_buf,
    const uint8_t *field_mask_buf,
    uint8_t *match_spec_buf,
    uint8_t *match_mask_spec_buf) {
  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field.keyFieldContextInfoGet());
  // Get the start bit and the end bit of the concerned field
  size_t start_bit = key_context->startBitGet();
  size_t end_bit = start_bit + key_field.sizeGet() - 1;

  // Extract all the bits from field buf into the match spec byte buf between
  // the start bit and the end bit
  size_t j = 0;
  for (size_t i = start_bit; i <= end_bit; i++, j++) {
    uint8_t temp = (field_buf[size - (j / 8) - 1] >> (j % 8)) & 0x01;
    uint8_t temp_mask = 0x01;
    // This function is called even for EXM key fields for which the field mask
    // is defaulted to all zeros. So don't touch the temp_mask
    if (field_mask_buf != nullptr) {
      temp_mask = (field_mask_buf[size - (j / 8) - 1] >> (j % 8)) & 0x01;
      if (do_masking) {
        temp &= temp_mask;
      }
    }

    // Left shift the extracted bit into its correct location
    match_spec_buf[key_context->parentFieldFullByteSizeGet() - (i / 8) - 1] |=
        (temp << (i % 8));
    match_mask_spec_buf[key_context->parentFieldFullByteSizeGet() - (i / 8) -
                        1] |= (temp_mask << (i % 8));
  }
  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
      this->table_->tableInfoGet()->tableContextInfoGet());
  // For exact match fields do_masking is false. Exact match fields need all
  // mask to be ffs. Also, only do this if backend table is exm.
  // Note: Even range do_masking is false, but since field_slices aren't even
  // supported for range right now, we need not think about it
  if (!do_masking && !mat_context_info->isTernaryTableGet()) {
    for (size_t index = 0; index < key_context->parentFieldFullByteSizeGet();
         index++) {
      match_mask_spec_buf[index] = 0xff;
    }
  }
  return;
}

void MatchActionKey::setValueInternal(const KeyFieldInfo &key_field,
                                      const uint8_t *value,
                                      const size_t &num_bytes) {
  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field.keyFieldContextInfoGet());
  // Get the start bit and the end bit of the concerned field
  const size_t field_offset = key_context->fieldOffsetGet();
  if (key_context->isFieldSlice()) {
    this->packFieldIntoMatchSpecByteBuffer(key_field,
                                           num_bytes,
                                           false,
                                           value,
                                           nullptr,
                                           key_array + field_offset,
                                           mask_array + field_offset);
  } else {
    auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
        this->table_->tableInfoGet()->tableContextInfoGet());
    std::memcpy(key_array + field_offset, value, num_bytes);
    // Set the mask to all 1's since this is an exact key field */
    std::memset(mask_array + field_offset, 0xff, num_bytes);
    // For ternary match tables, if we have mix of exact match and
    // ternary match key fields, then mask needs to be set correctly
    // based on the key field size in bits even for exact match key fields.
    if (mat_context_info->isTernaryTableGet()) {
      uint32_t field_size = key_field.sizeGet();
      if (field_size % 8) {
        uint8_t mask = ((1 << (field_size % 8)) - 1);
        mask_array[field_offset] = mask;
      }
    }
  }
}

void MatchActionKey::getValueInternal(const KeyFieldInfo &key_field,
                                      uint8_t *value,
                                      const size_t &num_bytes) const {
  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field.keyFieldContextInfoGet());
  // Get the start bit and the end bit of the concerned field
  const size_t field_offset = key_context->fieldOffsetGet();
  if (key_field.isFieldSlice()) {
    this->unpackFieldFromMatchSpecByteBuffer(
        key_field, num_bytes, key_array + field_offset, value);
  } else {
    std::memcpy(value, key_array + field_offset, num_bytes);
  }
}

void MatchActionKey::setValueAndMaskInternal(const KeyFieldInfo &key_field,
                                             const uint8_t *value,
                                             const uint8_t *mask,
                                             const size_t &num_bytes,
                                             bool do_masking) {
  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field.keyFieldContextInfoGet());
  // Get the start bit and the end bit of the concerned field
  const size_t field_offset = key_context->fieldOffsetGet();
  if (key_field.isFieldSlice()) {
    this->packFieldIntoMatchSpecByteBuffer(key_field,
                                           num_bytes,
                                           do_masking,
                                           value,
                                           mask,
                                           key_array + field_offset,
                                           mask_array + field_offset);
  } else {
    uint8_t *value_ptr = key_array + field_offset;
    uint8_t *mask_ptr = mask_array + field_offset;
    std::memcpy(key_array + field_offset, value, num_bytes);
    std::memcpy(mask_array + field_offset, mask, num_bytes);
    if (do_masking) {
      for (uint32_t i = 0; i < num_bytes; i++) {
        value_ptr[i] = value_ptr[i] & mask_ptr[i];
      }
    }
  }
}

void MatchActionKey::getValueAndMaskInternal(const KeyFieldInfo &key_field,
                                             uint8_t *value,
                                             uint8_t *mask,
                                             const size_t &num_bytes) const {
  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field.keyFieldContextInfoGet());
  // Get the start bit and the end bit of the concerned field
  const size_t field_offset = key_context->fieldOffsetGet();
  if (key_context->isFieldSlice()) {
    this->unpackFieldFromMatchSpecByteBuffer(
        key_field, num_bytes, key_array + field_offset, value);
    this->unpackFieldFromMatchSpecByteBuffer(
        key_field, num_bytes, mask_array + field_offset, mask);
  } else {
    std::memcpy(value, key_array + field_offset, num_bytes);
    std::memcpy(mask, mask_array + field_offset, num_bytes);
  }
}

void MatchActionKey::setKeySz() {
  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
      this->table_->tableInfoGet()->tableContextInfoGet());
  size_t bytes = mat_context_info->keySizeGet().bytes;
  key_array = new uint8_t[bytes];
  std::memset(key_array, 0, bytes);
  mask_array = new uint8_t[bytes];
  std::memset(mask_array, 0, bytes);
  num_valid_match_bytes = bytes;
  num_valid_match_bits = mat_context_info->keySizeGet().bits;
}

tdi_status_t MatchActionKey::reset() {
  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
      this->table_->tableInfoGet()->tableContextInfoGet());
  size_t key_size = mat_context_info->keySizeGet().bytes;
  if (!key_size) {
    // If the key size is ZERO, nothing to do. Think KEYLESS tables and the
    // likes
    return TDI_SUCCESS;
  }

  if (key_array) {
    std::memset(key_array, 0, key_size);
  }
  if (mask_array) {
    std::memset(mask_array, 0, key_size);
  };
  priority = 0;
  return TDI_SUCCESS;
}

// ActionProfileKey *****************
tdi_status_t ActionProfileKey::setValue(const tdi_id_t &field_id,
                                        const tdi::KeyFieldValue &field_value) {
  return setValueHelper<ActionProfileKey>(this, field_id, field_value);
}

tdi_status_t ActionProfileKey::getValue(const tdi_id_t &field_id,
                                        tdi::KeyFieldValue *field_value) const {
  return getValueHelper<ActionProfileKey>(this, field_id, field_value);
}
tdi_status_t ActionProfileKey::setValue(const tdi::KeyFieldInfo * /*key_field*/,
                                        const uint64_t &value) {
  member_id = value;
  return TDI_SUCCESS;
}

tdi_status_t ActionProfileKey::setValue(const tdi::KeyFieldInfo *key_field,
                                        const uint8_t *value,
                                        const size_t &size) {
  if (size != sizeof(tdi_rt_act_mem_id_t)) {
    LOG_ERROR(
        "%s:%d %s Array size %zd bytes passed is not equal to the size of the "
        "field %zd bytes",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        size,
        sizeof(tdi_rt_act_mem_id_t));
    return TDI_INVALID_ARG;
  }

  uint64_t val = 0;
  utils::TableFieldUtils::toHostOrderData(*key_field, value, &val);
  this->member_id = static_cast<uint32_t>(val);

  return TDI_SUCCESS;
}

tdi_status_t ActionProfileKey::getValue(const tdi::KeyFieldInfo * /*key_field*/,
                                        uint64_t *value) const {
  *value = member_id;
  return TDI_SUCCESS;
}

tdi_status_t ActionProfileKey::getValue(const tdi::KeyFieldInfo *key_field,
                                        const size_t &size,
                                        uint8_t *value) const {
  if (size != sizeof(tdi_rt_act_mem_id_t)) {
    LOG_ERROR(
        "%s:%d %s Array size %zd bytes passed is not equal to the size of the "
        "field %zd bytes",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        size,
        sizeof(tdi_rt_act_mem_id_t));
    return TDI_INVALID_ARG;
  }
  uint64_t val = this->member_id;
  utils::TableFieldUtils::toNetworkOrderData(*key_field, val, value);
  return TDI_SUCCESS;
}

/* Snapshot */

#if 0

#endif

// SelectorTableKey *****************
tdi_status_t SelectorTableKey::setValue(const tdi_id_t &field_id,
                                        const tdi::KeyFieldValue &field_value) {
  return setValueHelper<SelectorTableKey>(this, field_id, field_value);
}

tdi_status_t SelectorTableKey::setValue(const tdi::KeyFieldInfo *key_field,
                                        const uint8_t *value,
                                        const size_t &size) {
  if (size != sizeof(tdi_id_t)) {
    LOG_ERROR(
        "%s:%d %s Array size %zd bytes passed is not equal to the size of the "
        "field %zd bytes, for field id %d",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        size,
        sizeof(tdi_id_t),
        key_field->idGet());
    return TDI_INVALID_ARG;
  }

  uint64_t val = 0;
  utils::TableFieldUtils::toHostOrderData(*key_field, value, &val);
  this->group_id = val;

  return TDI_SUCCESS;
}

tdi_status_t SelectorTableKey::setValue(const tdi::KeyFieldInfo * /*key_field*/,
                                        const uint64_t &value) {
  group_id = value;
  return TDI_SUCCESS;
}

tdi_status_t SelectorTableKey::getValue(const tdi_id_t &field_id,
                                        tdi::KeyFieldValue *field_value) const {
  return getValueHelper<SelectorTableKey>(this, field_id, field_value);
}

tdi_status_t SelectorTableKey::getValue(const tdi::KeyFieldInfo * /*key_field*/,
                                        uint64_t *value) const {
  *value = group_id;
  return TDI_SUCCESS;
}

tdi_status_t SelectorTableKey::getValue(const tdi::KeyFieldInfo *key_field,
                                        const size_t &size,
                                        uint8_t *value) const {
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        size,
        sizeof(uint32_t),
        key_field->idGet());
    return TDI_INVALID_ARG;
  }

  uint32_t local_val = htobe32(group_id);
  std::memcpy(value, &local_val, sizeof(uint32_t));
  return TDI_SUCCESS;
}

template <class T>
tdi_status_t getKeyIdxValue(const T &key,
                           const tdi::Table &table,
                           const tdi::KeyFieldInfo *key_field,
                           uint64_t *value) {
  *value = key.getIdxKey();
  return TDI_SUCCESS;
}

template <class T>
tdi_status_t getKeyIdxValue(const T &key,
                           const tdi::Table &table,
                           const tdi::KeyFieldInfo *key_field,
                           uint8_t *value,
                           const size_t &size) {
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        size,
        sizeof(uint32_t),
        key_field->idGet());
    return TDI_INVALID_ARG;
  }
  uint32_t local_val = htobe32(key.getIdxKey());
  std::memcpy(value, &local_val, sizeof(uint32_t));
  return TDI_SUCCESS;
}

tdi_status_t CounterIndirectTableKey::setValue(
    const tdi_id_t &field_id, const tdi::KeyFieldValue &field_value) {
  return setValueHelper<CounterIndirectTableKey>(this, field_id, field_value);
}

tdi_status_t CounterIndirectTableKey::setValue(
    const tdi::KeyFieldInfo * /*key_field*/, const uint64_t &value) {
  counter_id = value;
  return TDI_SUCCESS;
}

tdi_status_t CounterIndirectTableKey::setValue(const tdi::KeyFieldInfo *key_field,
                                          const uint8_t *value,
                                          const size_t &size) {
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        size,
        sizeof(uint32_t),
        key_field->idGet());
    return TDI_INVALID_ARG;
  }
  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  counter_id = be32toh(val);
  return TDI_SUCCESS;
}

tdi_status_t CounterIndirectTableKey::getValue(
    const tdi_id_t &field_id, tdi::KeyFieldValue *field_value) const {
  return getValueHelper<CounterIndirectTableKey>(this, field_id, field_value);
}

tdi_status_t CounterIndirectTableKey::getValue(const tdi::KeyFieldInfo *key_field,
                                               uint64_t *value) const {
  return getKeyIdxValue<CounterIndirectTableKey>(*this, *table_, key_field, value);
}

tdi_status_t CounterIndirectTableKey::getValue(const tdi::KeyFieldInfo *key_field,
                                          const size_t &size,
                                          uint8_t *value) const {
  return getKeyIdxValue<CounterIndirectTableKey>(
      *this, *table_, key_field, value, size);
}

#if 0
tdi_status_t MeterTableKey::getValue(const tdi_id_t &field_id,
                                        uint64_t *value) const {
  return getKeyIdxValue<MeterTableKey>(*this, *table_, field_id, value);
}

tdi_status_t MeterTableKey::getValue(const tdi_id_t &field_id,
                                        const size_t &size,
                                        uint8_t *value) const {
  return getKeyIdxValue<MeterTableKey>(
      *this, *table_, field_id, value, size);
}

tdi_status_t MeterTableKey::setValue(const tdi_id_t &field_id,
                                        const uint64_t &value) {
  // Get the key_field from the table
  const KeyFieldInfo *key_field;
  auto status = utils::TableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_EXACT), table_);
  if (status != TDI_SUCCESS) {
    return status;
  }
  meter_id = value;
  return TDI_SUCCESS;
}

tdi_status_t MeterTableKey::setValue(const tdi_id_t &field_id,
                                        const uint8_t *value,
                                        const size_t &size) {
  // Get the key_field from the table
  const KeyFieldInfo *key_field;
  auto status = utils::TableFieldUtils::getKeyFieldSafe(
      field_id, &key_field, static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_EXACT), table_);
  if (status != TDI_SUCCESS) {
    return status;
  }
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        size,
        sizeof(uint32_t),
        field_id);
    return TDI_INVALID_ARG;
  }
  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  val = be32toh(val);
  meter_id = val;
  return TDI_SUCCESS;
}
#endif
tdi_status_t RegisterTableKey::getValue(const tdi_id_t &field_id,
					tdi::KeyFieldValue *value) const {
	return getValueHelper<RegisterTableKey>(this, field_id, value);
}
tdi_status_t RegisterTableKey::getValue(const tdi::KeyFieldInfo *key_field,
					uint64_t *value) const {
  return getKeyIdxValue<RegisterTableKey>(*this, *table_, key_field, value);
}

tdi_status_t RegisterTableKey::getValue(const tdi::KeyFieldInfo *key_field,
					const size_t &size,
					uint8_t *value) const {
  return getKeyIdxValue<RegisterTableKey>(
      					  *this, *table_, key_field, value, size);
}

tdi_status_t RegisterTableKey::setValue(const tdi_id_t &field_id,
					const tdi::KeyFieldValue &field_value) {
	return setValueHelper<RegisterTableKey>(this, field_id, field_value);
}

tdi_status_t RegisterTableKey::setValue(const tdi::KeyFieldInfo *key_field,
					const uint64_t &value) {
  register_id = value;
  return TDI_SUCCESS;
}

tdi_status_t RegisterTableKey::setValue(const tdi::KeyFieldInfo *key_field,
					const uint8_t *value,
					const size_t &size) {
  if (size != sizeof(uint32_t)) {
    LOG_ERROR(
        "%s:%d %s ERROR Array size of %zd is not equal to the field size %zd "
        "for field id %d",
        __func__,
        __LINE__,
        table_->tableInfoGet()->nameGet().c_str(),
        size,
        sizeof(uint32_t),
        key_field->idGet());
    return TDI_INVALID_ARG;
  }
  uint32_t val = *(reinterpret_cast<const uint32_t *>(value));
  val = be32toh(val);
  register_id = val;
  return TDI_SUCCESS;
}
#if 0
tdi_status_t SelectorGetMemberTableKey::setValue(const tdi_id_t &field_id,
                                                    const uint64_t &value) {
  return this->setValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t SelectorGetMemberTableKey::setValue(const tdi_id_t &field_id,
                                                    const uint8_t *value,
                                                    const size_t &size) {
  return this->setValueInternal(field_id, 0, value, size);
}

tdi_status_t SelectorGetMemberTableKey::setValueInternal(
    const tdi_id_t &field_id,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t &size) {
  tdi_status_t status = TDI_SUCCESS;

  const KeyFieldInfo *tableKeyField = nullptr;
  status = utils::TableFieldUtils::getKeyFieldSafe(
      field_id, &tableKeyField, static_cast<tdi_match_type_e>(TDI_MATCH_TYPE_EXACT), this->table_);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_OBJECT_NOT_FOUND;
  }
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableKeyField, &value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableKeyField->getId());
    return status;
  }
  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableKeyField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableKeyField->getId());
    return status;
  }

  uint64_t val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(
        *tableKeyField, value_ptr, &val);
  } else {
    val = value;
  }

  this->uint64_fields[field_id] = val;
  return TDI_SUCCESS;
}

tdi_status_t SelectorGetMemberTableKey::getValue(const tdi_id_t &field_id,
                                                    uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t SelectorGetMemberTableKey::getValue(const tdi_id_t &field_id,
                                                    const size_t &size,
                                                    uint8_t *value) const {
  return this->getValueInternal(field_id, 0, value, size);
}

tdi_status_t SelectorGetMemberTableKey::getValueInternal(
    const tdi_id_t &field_id,
    uint64_t *value,
    uint8_t *value_ptr,
    const size_t &size) const {
  tdi_status_t status = TDI_SUCCESS;
  const KeyFieldInfo *tableKeyField = nullptr;

  status = this->table_->getKeyField(field_id, &tableKeyField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d Invalid field id %d for table %s",
              __func__,
              __LINE__,
              field_id,
              this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableKeyField, value, value_ptr, size);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableKeyField->getId());
    return sts;
  }
  uint64_t val;
  auto elem = this->uint64_fields.find(field_id);
  if (elem != this->uint64_fields.end()) {
    val = this->uint64_fields.at(field_id);
  } else {
    val = 0;
  }

  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(
        *tableKeyField, val, value_ptr);
  } else {
    *value = val;
  }
  return TDI_SUCCESS;
}
#endif

}  // namespace rt
}  // namespace pna
}  // namespace tdi

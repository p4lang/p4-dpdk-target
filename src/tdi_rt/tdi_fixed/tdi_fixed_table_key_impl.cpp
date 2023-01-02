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
#include <bitset>
#include <arpa/inet.h>
#include <inttypes.h>

#include <tdi/common/tdi_utils.hpp>
#include <tdi_common/tdi_table_field_utils.hpp>

#include "tdi_fixed_table_key_impl.hpp"

namespace tdi {
namespace pna {
namespace rt {
#define BITS_PER_BYTE     8
#define MIN_BITS_TO_CONV  7
/* Macro to convert bits to bytes */
#define CONV_BITS_TO_BYTES(_bits) ((_bits+MIN_BITS_TO_CONV) / BITS_PER_BYTE)

// FixedFunctionTableKey *****************
tdi_status_t FixedFunctionTableKey::setValue(const tdi_id_t &field_id,
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
         auto e_fv =
		 static_cast<const tdi::KeyFieldValueExact<const uint8_t *> *>(
				&field_value);
	 return this->setValue(key_field, e_fv->value_, e_fv->size_);
     } else {
	 auto e_fv =
		 static_cast<const tdi::KeyFieldValueExact<const uint64_t> *>(
				  &field_value);
         return this->setValue(key_field, e_fv->value_);
     }
  }

  return TDI_UNEXPECTED;
}

tdi_status_t FixedFunctionTableKey::getValue(
		const tdi_id_t &field_id,
		tdi::KeyFieldValue *field_value) const {

  const KeyFieldInfo *key_field;

  if (!field_value) {
    LOG_ERROR("%s:%d %s input param passed null for key field_id %d, ",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  auto status = utils::TableFieldUtils::keyFieldSafeGet(
      field_id, &key_field, field_value, table_);

  if (status != TDI_SUCCESS) {
    return status;
  }

  if (field_value->is_pointer()) {
	  auto e_fv =
		  static_cast<tdi::KeyFieldValueExact<uint8_t *> *>(field_value);
	  return this->getValue(key_field, e_fv->size_, e_fv->value_);
  } else {
	  auto e_fv = static_cast<tdi::KeyFieldValueExact<uint64_t> *>(field_value);
	  return this->getValue(key_field, &e_fv->value_);
  }

  return TDI_NOT_SUPPORTED;
}

tdi_status_t FixedFunctionTableKey::setValue(const tdi::KeyFieldInfo *key_field,
                                             const uint64_t &value) {
  auto status = utils::TableFieldUtils::boundsCheck(
		  *table_, *key_field, value, nullptr, 0);

  if (status != TDI_SUCCESS) {
    return status;
  }

  auto size_bytes = CONV_BITS_TO_BYTES(key_field->sizeGet());
  auto new_value = (value);

  uint8_t *val_ptr = ((uint8_t *)&new_value);
  this->setValueInternal(*key_field, val_ptr, size_bytes);

  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionTableKey::setValue(const tdi::KeyFieldInfo *key_field,
                                             const uint8_t *value,
                                             const size_t &size) {
  auto status =
      utils::TableFieldUtils::boundsCheck(*table_, *key_field, 0, value, size);
  if (status != TDI_SUCCESS) {
    return status;
  }
  auto size_bytes = CONV_BITS_TO_BYTES(key_field->sizeGet());
  this->setValueInternal(*key_field, value, size_bytes);

  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionTableKey::getValue(const tdi::KeyFieldInfo *key_field,
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
  // Zero out the out param
  *value = 0;
  auto size_bytes = CONV_BITS_TO_BYTES(key_field->sizeGet());
  uint8_t *temp_ptr = reinterpret_cast<uint8_t *>(value);

  this->getValueInternal(*key_field, temp_ptr, size_bytes);
  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionTableKey::getValue(const tdi::KeyFieldInfo *key_field,
                                             const size_t &size,
                                             uint8_t *value) const {
	// ceil the size and convert bits to bytes
	auto size_bytes = CONV_BITS_TO_BYTES(key_field->sizeGet());
	auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
			*table_, *key_field, nullptr, value, size);
	if (status != TDI_SUCCESS) {
		return status;
	}

	this->getValueInternal(*key_field, value, size_bytes);

	return TDI_SUCCESS;
}


void FixedFunctionTableKey::populate_fixed_fun_key_spec(
		fixed_function_key_spec_t *pipe_match_spec) const {
	pipe_match_spec->num_bytes = num_valid_match_bytes;
	pipe_match_spec->array     = key_array;
}

void FixedFunctionTableKey::set_key_from_match_spec_by_deepcopy(
    const pipe_tbl_match_spec_t *p) {
  this->num_valid_match_bits = p->num_valid_match_bits;
  this->num_valid_match_bytes = p->num_match_bytes;

  if (key_array) delete[] key_array;
  this->key_array = new uint8_t[this->num_valid_match_bytes];

  std::memcpy(this->key_array, p->match_value_bits, p->num_match_bytes);
}

void FixedFunctionTableKey::populate_key_from_match_spec(
    const fixed_function_key_spec_t &fixed_match_spec) {
  key_array = fixed_match_spec.array;
}

void FixedFunctionTableKey::setKeySz() {
  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>
	  (this->table_->tableInfoGet()->tableContextInfoGet());
  size_t bytes =  mat_context_info->keySizeGet().bytes;

  key_array = new uint8_t[bytes];
  std::memset(key_array, 0, bytes);
  num_valid_match_bytes = bytes;
  num_valid_match_bits = mat_context_info->keySizeGet().bits;
}

tdi_status_t FixedFunctionTableKey::reset() {
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
  return TDI_SUCCESS;
}

void FixedFunctionTableKey::getValueInternal(const KeyFieldInfo &key_field,
                                             uint8_t *value,
                                             const size_t &num_bytes) const {
  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field.keyFieldContextInfoGet());
  // Get the start bit and the end bit of the concerned field
  const size_t field_offset = key_context->fieldOffsetGet();
    std::memcpy(value, key_array + field_offset, num_bytes);
}

void FixedFunctionTableKey::setValueInternal(const KeyFieldInfo &key_field,
                                      const uint8_t *value,
                                      const size_t &num_bytes) {

  auto key_context = static_cast<const RtKeyFieldContextInfo *>(
      key_field.keyFieldContextInfoGet());

  if (key_context) {
      // Get the start bit and the end bit of the concerned field
      const size_t field_offset = key_context->fieldOffsetGet();
      std::memcpy(key_array + field_offset, value, num_bytes);
  }
}
}  // namespace rt
}  // namespace pna
}  // namespace tdi

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
#include <arpa/inet.h>
#include <inttypes.h>

#include <tdi/common/tdi_utils.hpp>
#include <tdi/common/tdi_table.hpp>

#include "tdi_fixed_table_impl.hpp"
#include "tdi_fixed_table_data_impl.hpp"
#include <tdi_common/tdi_table_field_utils.hpp>

#include <sstream>

namespace tdi {
namespace pna {
namespace rt {

// FixedFunctionConfigTable ******************
tdi_status_t FixedFunctionTableDataSpec::setFixedFunctionData(
		const tdi::DataFieldInfo &field,
		const uint64_t &value,
		const uint8_t *value_ptr,
		const std::string &value_str,
		pipe_data_spec_t *specData) {

        size_t field_size = field.sizeGet();
        size_t field_offset = 0;
        auto size = (field_size + 7) / 8;

        field_offset = static_cast<const RtDataFieldContextInfo *>
		(field.dataFieldContextInfoGet())->offsetGet();

        if (value_ptr) {
                std::memcpy(specData->data_bytes + field_offset, value_ptr,
				size);
        } else if (!value_str.empty()){
		std::memcpy(specData->data_bytes + field_offset,
				(char*)value_str.data(), value_str.length());
        } else {
		uint8_t *val_ptr_t = ((uint8_t *)&value);
		std::memcpy(specData->data_bytes + field_offset, val_ptr_t,
				size);
	}

        return TDI_SUCCESS;
}

void FixedFunctionTableDataSpec::getFixedFunctionData(
		fixed_function_data_spec_t *value,
                pipe_data_spec_t *specData) const {
	       value->num_bytes = specData->num_data_bytes;
	       value->array     = specData->data_bytes;
}

void FixedFunctionTableDataSpec::getFixedFunctionDataSpec(
		pipe_data_spec_t *value,
                pipe_data_spec_t *specData) const {
               value->num_data_bytes = specData->num_data_bytes;
               value->data_bytes     = specData->data_bytes;
}

void FixedFunctionTableDataSpec::getFixedFunctionDataBytes(
		const tdi::DataFieldInfo &field,
                const size_t size,
                uint8_t *value,
                pipe_data_spec_t *specData) const {
	size_t offset = static_cast<const RtDataFieldContextInfo *>(
			field.dataFieldContextInfoGet())->offsetGet();

        std::memcpy(value, specData->data_bytes+offset, size);
}

tdi_status_t FixedFunctionTableData::setValue(const tdi_id_t &field_id,
                                              const uint64_t &value) {
  return this->setValueInternal(field_id, value, nullptr, 0, std::string(""));
}

tdi_status_t FixedFunctionTableData::setValue(const tdi_id_t &field_id,
                                              const uint8_t *ptr,
                                              const size_t &s) {
  return this->setValueInternal(field_id, 0, ptr, s, std::string(""));
}

tdi_status_t FixedFunctionTableData::setValue(const tdi_id_t &field_id,
                                              const std::string &value_str) {
	return this->setValueInternal(field_id, 0, nullptr, 0, value_str);
}

tdi_status_t FixedFunctionTableData::setValue(const tdi_id_t &field_id,
                                              const float &value) {
  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionTableData::getValue(
		const tdi_id_t &field_id,
                std::vector<uint64_t> *value) const {
	return TDI_SUCCESS;
}

tdi_status_t FixedFunctionTableData::getValueInternal(
                const tdi_id_t &field_id,
                uint64_t *value,
                uint8_t *value_ptr,
                const size_t &size) const {
       return TDI_SUCCESS;
}

tdi_status_t FixedFunctionTableData::getValue(const tdi_id_t &field_id,
                                              uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t FixedFunctionTableData::getValue(const tdi_id_t &field_id,
                                              const size_t &size,
                                              uint8_t *value) const {
  return this->getValueInternal(field_id, nullptr, value, size);
}

tdi_status_t FixedFunctionTableData::getValue(const tdi_id_t &field_id,
                                            std::string *value) const {
  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionTableData::getValue(const tdi_id_t &field_id,
                                              float *value) const {
  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionTableData::setValueInternal(
		const tdi_id_t &field_id,
		const uint64_t &value,
		const uint8_t *value_ptr,
		const size_t &s,
		const std::string &value_str) {
  const tdi::DataFieldInfo *tableDataField =
	  this->table_->tableInfoGet()->dataFieldGet(field_id, 0);

  if (!tableDataField) {
	  LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
			  __func__,
			  __LINE__,
			  this->table_->tableInfoGet()->nameGet().c_str(),
			  field_id);
	  return TDI_OBJECT_NOT_FOUND;
  }

 if (value || value_ptr) {
    // Do some bounds checking using the utility functions
   auto sts = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
		   *this->table_, *tableDataField, &value, value_ptr, s);
   if (sts != TDI_SUCCESS) {
		 LOG_ERROR(
		   "ERROR: %s:%d %s : Input param compatibility check failed for field id "
		   "%d",
		   __func__,
		   __LINE__,
		   this->table_->tableInfoGet()->nameGet().c_str(),
		   tableDataField->idGet());
		 return sts;
   }

   sts = utils::TableFieldUtils::boundsCheck(
		   *this->table_, *tableDataField, value, value_ptr, s);
   if (sts != TDI_SUCCESS) {
      LOG_ERROR(
	  "ERROR: %s:%d %s : Input Param bounds check failed for field id %d "
	  "action id "
	  "%d",
	  __func__,
	  __LINE__,
	  this->table_->tableInfoGet()->nameGet().c_str(),
	  tableDataField->idGet(),
	  this->actionIdGet());
        return sts;
     }
  }
  auto sts = getFixedFunctionTableDataSpecObj().
	 setFixedFunctionData(*tableDataField,
			 value,
			 value_ptr,
			 value_str,
			 &this->fixed_spec_data_->fixed_spec_data);

  if (sts != TDI_SUCCESS) {
	  LOG_ERROR("%s:%d %s Unable to set data",
			  __func__,
			  __LINE__,
			  this->table_->tableInfoGet()->nameGet().c_str());
	  return sts;
  }

  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to set data",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str());
    return sts;
  }
  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionTableData::reset(
    const tdi_id_t &act_id,
    const tdi_id_t &/*container_id*/,
    const std::vector<tdi_id_t> & /*fields*/) {
  std::vector<tdi_id_t> empty;
  // empty vector means set all fields
  return TableData::reset(act_id, 0, empty);
}

tdi_status_t FixedFunctionTableData::resetDerived() {
  return TDI_SUCCESS;
}

}  // namespace rt
}  // namespace pna
}  // namespace tdi

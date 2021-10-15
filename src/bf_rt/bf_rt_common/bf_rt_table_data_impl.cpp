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

#include "bf_rt_table_impl.hpp"
#include "bf_rt_table_data_impl.hpp"
#include "bf_rt_utils.hpp"

namespace bfrt {

fieldDestination BfRtTableDataField::getDataFieldDestination(
    const std::set<DataFieldType> &fieldTypes) {
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::ACTION_PARAM:
      case DataFieldType::ACTION_PARAM_OPTIMIZED_OUT:
      case DataFieldType::ACTION_MEMBER_ID:
      case DataFieldType::SELECTOR_GROUP_ID:
      case DataFieldType::COUNTER_INDEX:
      case DataFieldType::REGISTER_INDEX:
      case DataFieldType::METER_INDEX:
      case DataFieldType::LPF_INDEX:
      case DataFieldType::WRED_INDEX:
        return fieldDestination::ACTION_SPEC;
        break;
      case DataFieldType::COUNTER_SPEC_BYTES:
      case DataFieldType::COUNTER_SPEC_PACKETS:
        return fieldDestination::DIRECT_COUNTER;
        break;
      case DataFieldType::TTL:
        return fieldDestination::TTL;
      case DataFieldType::ENTRY_HIT_STATE:
        return fieldDestination::ENTRY_HIT_STATE;
      case DataFieldType::METER_SPEC_CIR_PPS:
      case DataFieldType::METER_SPEC_PIR_PPS:
      case DataFieldType::METER_SPEC_CBS_PKTS:
      case DataFieldType::METER_SPEC_PBS_PKTS:
      case DataFieldType::METER_SPEC_CIR_KBPS:
      case DataFieldType::METER_SPEC_PIR_KBPS:
      case DataFieldType::METER_SPEC_CBS_KBITS:
      case DataFieldType::METER_SPEC_PBS_KBITS:
        return fieldDestination::DIRECT_METER;
      case LPF_SPEC_GAIN_TIME_CONSTANT:
      case LPF_SPEC_DECAY_TIME_CONSTANT:
      case LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR:
      case LPF_SPEC_TYPE:
        return fieldDestination::DIRECT_LPF;
      case WRED_SPEC_TIME_CONSTANT:
      case WRED_SPEC_MIN_THRESHOLD:
      case WRED_SPEC_MAX_THRESHOLD:
      case WRED_SPEC_MAX_PROBABILITY:
        return fieldDestination::DIRECT_WRED;
      case DataFieldType::REGISTER_SPEC:
      case DataFieldType::REGISTER_SPEC_HI:
      case DataFieldType::REGISTER_SPEC_LO:
        return fieldDestination::DIRECT_REGISTER;
      default:
        return fieldDestination::INVALID;
        break;
    }
  }
  return fieldDestination::INVALID;
}

bf_status_t BfRtTableDataField::containerFieldIdListGet(
    std::vector<bf_rt_id_t> *id_vec) const {
  if (id_vec == nullptr) {
    LOG_ERROR("%s:%d Please pass mem allocated out param", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  if (!isContainerValid()) {
    LOG_ERROR("%s:%d Not a container field", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  for (const auto &kv : this->container) id_vec->push_back(kv.first);
  return BF_SUCCESS;
}

bf_status_t BfRtTableDataField::dataFieldInsertContainer(
    std::unique_ptr<BfRtTableDataField> container_field) {
  bf_rt_id_t container_field_id = container_field->getId();
  std::string container_name = container_field->getName();
  if (this->container.find(container_field_id) != this->container.end()) {
    LOG_ERROR("%s:%d Id \"%u\" Exists for container",
              __func__,
              __LINE__,
              container_field_id);
    // Field-id is repeating
    return BF_INVALID_ARG;
  }
  // Insert the container field
  this->container_names[container_name] = container_field.get();
  this->container[container_field_id] = std::move(container_field);

  return BF_SUCCESS;
}

// BASE BfRtTableDataObj
bf_status_t BfRtTableDataObj::setValue(const bf_rt_id_t &field_id,
                                       const uint64_t & /*value*/) {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::setValue(const bf_rt_id_t &field_id,
                                       const uint8_t * /*value*/,
                                       const size_t & /*size*/) {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::setValue(
    const bf_rt_id_t &field_id, const std::vector<bf_rt_id_t> & /*arr*/) {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::setValue(const bf_rt_id_t &field_id,
                                       const std::vector<bool> & /*arr*/) {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::setValue(
    const bf_rt_id_t &field_id, const std::vector<std::string> & /*arr*/) {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::setValue(const bf_rt_id_t &field_id,
                                       const float & /*value*/) {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::setValue(const bf_rt_id_t &field_id,
                                       const bool & /*value*/) {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::setValue(
    const bf_rt_id_t &field_id,
    const std::vector<std::unique_ptr<BfRtTableData>> /*vec*/) {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::setValue(const bf_rt_id_t &field_id,
                                       const std::string & /*str*/) {
  LOG_ERROR("%s:%d %s Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::getValue(const bf_rt_id_t &field_id,
                                       uint64_t * /*value*/) const {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::getValue(const bf_rt_id_t &field_id,
                                       const size_t & /*size*/,
                                       uint8_t * /*value*/) const {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::getValue(
    const bf_rt_id_t &field_id, std::vector<bf_rt_id_t> * /*arr*/) const {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::getValue(
    const bf_rt_id_t &field_id, std::vector<uint64_t> * /*value*/) const {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::getValue(const bf_rt_id_t &field_id,
                                       std::vector<bool> * /*arr*/) const {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::getValue(
    const bf_rt_id_t &field_id, std::vector<std::string> * /*arr*/) const {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::getValue(const bf_rt_id_t &field_id,
                                       float * /*value*/) const {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::getValue(const bf_rt_id_t &field_id,
                                       bool * /*value*/) const {
  LOG_ERROR("%s:%d %s ERROR : Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::getValue(
    const bf_rt_id_t &field_id,
    std::vector<BfRtTableData *> * /* vector */) const {
  LOG_ERROR("%s:%d %s Not supported for field-id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::getValue(const bf_rt_id_t &field_id,
                                       std::string * /*str*/) const {
  LOG_ERROR("%s:%d %s Not supported for field id %d",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str(),
            field_id);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::actionIdGet(bf_rt_id_t *act_id) const {
  *act_id = this->action_id_;
  return BF_SUCCESS;
}

bf_status_t BfRtTableDataObj::getParent(const BfRtTable **table_ret) const {
  *table_ret = this->table_;
  return BF_SUCCESS;
}

bf_status_t BfRtTableDataObj::isActive(const bf_rt_id_t &field_id,
                                       bool *is_active) const {
  *is_active = active_fields_.find(field_id) != active_fields_.end();
  return BF_SUCCESS;
}

bf_status_t BfRtTableDataObj::reset() {
  LOG_ERROR("%s:%d %s : ERROR API not supported",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::reset(const bf_rt_id_t & /*action_id */) {
  LOG_ERROR("%s:%d %s : ERROR API not supported",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::reset(
    const bf_rt_id_t & /*action_id */,
    const std::vector<bf_rt_id_t> & /* fields */) {
  LOG_ERROR("%s:%d %s : ERROR API not supported",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableDataObj::reset(
    const std::vector<bf_rt_id_t> & /* fields */) {
  LOG_ERROR("%s:%d %s : ERROR API not supported",
            __func__,
            __LINE__,
            this->table_->table_name_get().c_str());
  return BF_NOT_SUPPORTED;
}

size_t BfRtTableDataObj::getdataSzbits(bf_rt_id_t act_id) {
  return this->table_->getdataSzbits(act_id);
}

size_t BfRtTableDataObj::getdataSz(bf_rt_id_t act_id) {
  return this->table_->getdataSz(act_id);
}

size_t BfRtTableDataObj::getMaxdataSz() { return this->table_->getMaxdataSz(); }

size_t BfRtTableDataObj::getMaxdataSzbits() {
  return this->table_->getMaxdataSzbits();
}

/**
 * @brief Set the current active fields. This top class function just goes over
 * all the fields and checks the following
 * *  Whether the input fields are valid or not. Log the error and move on.
 *
 * We set them only during either setValue() or tableEntryGet() if required for
 * those specific use cases.
 *
 */
bf_status_t BfRtTableDataObj::setActiveFields(
    const std::vector<bf_rt_id_t> &fields) {
  std::vector<bf_rt_id_t> all_fields;
  bf_status_t status = BF_SUCCESS;

  // table_ doesn't exist means this might be learn data obj
  if (!this->table_) return BF_SUCCESS;

  // Empty input fields means all fields. If action ID is applicable
  // but action id is 0, then just set active
  // fields empty but mark all_fields_set because this might be for a get
  // request
  if (fields.empty() && this->table_->actionIdApplicable() &&
      this->actionIdGet_() == 0) {
    this->active_fields_ = {};
    this->all_fields_set_ = true;
    return BF_SUCCESS;
  }

  if (this->table_->actionIdApplicable()) {
    status =
        this->table_->dataFieldIdListGet(this->actionIdGet_(), &all_fields);
  } else {
    status = this->table_->dataFieldIdListGet(&all_fields);
  }
  if (status != BF_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d %s : cannot get data fields list, action_id %d",
              __func__,
              __LINE__,
              this->table_->table_name_get().c_str(),
              this->actionIdGet_());
    return BF_INVALID_ARG;
  }

  std::set<bf_rt_id_t> dataFieldsSet(all_fields.begin(), all_fields.end());
  if (fields.empty()) {
    this->active_fields_ = dataFieldsSet;
    this->all_fields_set_ = true;
  } else {
    // check a field is valid for this table data then add into active field
    // list.
    // empty the set first. And all_fields_set_ is false
    active_fields_.clear();
    this->all_fields_set_ = false;
    for (const auto &field : fields) {
      if (dataFieldsSet.find(field) == dataFieldsSet.end()) {
        LOG_ERROR("%s:%d ERROR Invalid Field id %d for action id %d ",
                  __func__,
                  __LINE__,
                  field,
                  this->actionIdGet_());
        continue;
      }
      active_fields_.insert(field);
    }
  }
  return BF_SUCCESS;
}

bool BfRtTableDataObj::checkFieldActive(const bf_rt_id_t &field_id) const {
  return this->active_fields_.find(field_id) != this->active_fields_.end();
}

}  // bfrt

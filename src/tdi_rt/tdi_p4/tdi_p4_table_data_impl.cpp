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

#include <arpa/inet.h>
#include <inttypes.h>

#include <tdi/common/tdi_utils.hpp>
#include <tdi/common/tdi_table.hpp>

#include "tdi_p4_table_impl.hpp"
#include "tdi_p4_table_data_impl.hpp"
#include <tdi_common/tdi_table_field_utils.hpp>

#include <sstream>

namespace tdi {
namespace pna {
namespace rt {

namespace {

#if 0
/*
 * - Get Data Field from the table
 * - Check if the data dield type matched with any of the allowed field types
 * - If the allowed_field_types set is an empty set, that means that the getter
 *   can be used for any field type
 * - Check if the data field size is compatible with the passed-in field size
 * - Check if the value passed-in is within the bounds of the data field type
 * - If the valued is being set using a byte array, then convert it to host
 *   order
 * - Set the field type which can be reused by the function calling this
 */
tdi_status_t indirectResourceSetValueHelper(
    const tdi::Table &table,
    const tdi_id_t &field_id,
    const std::set<DataFieldType> &allowed_field_types,
    const size_t &size,
    const uint64_t &value,
    const uint8_t *value_ptr,
    uint64_t *out_val,
    DataFieldType *field_type) {
  // Get the data_field from the table
  auto tableDataField = table.tableInfoGet()->dataFieldGet(field_id, 0);
  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Data field id %d not found",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *field_type = *(static_cast<const RtDataFieldContextInfo *>(
                    tableDataField->dataFieldContextInfoGet())
                    ->typesGet()
                    .begin());
  if (allowed_field_types.size() &&
      allowed_field_types.find(*field_type) == allowed_field_types.end()) {
    // This indicates that this particular setter can only be used for a
    // specific set of DataFieldTypes and the fieldtype of the field
    // corresponding to the passed in field id does not match with any of
    // them
    LOG_ERROR("%s:%d %s ERROR : This setter cannot be used for field id %d",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      table, *tableDataField, &value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s ERROR : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        field_id);
    return status;
  }
  status = utils::TableFieldUtils::boundsCheck(
      table, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s ERROR : Input Param bounds check failed for field id %d ",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        field_id);
    return status;
  }

  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(
        *tableDataField, value_ptr, out_val);
  } else {
    *out_val = value;
  }
  return TDI_SUCCESS;
}

/*
 * - Get Data Field from the table
 * - Check if the data dield type matched with any of the allowed field types
 * - If the allowed_field_types set is an empty set, that means that the getter
 *   can be used for any field type
 * - Check if the data field size is compatible with the passed-in field size
 * - Set the field type which can be reused by the function calling this
 */
tdi_status_t indirectResourceGetValueHelper(
    const tdi::Table &table,
    const tdi_id_t &field_id,
    const std::set<DataFieldType> &allowed_field_types,
    const size_t &size,
    uint64_t *value,
    uint8_t *value_ptr,
    DataFieldType *field_type) {
  auto tableDataField = table.tableInfoGet()->dataFieldGet(field_id, 0);
  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Data field id %d not found",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  *field_type = *(static_cast<const RtDataFieldContextInfo *>(
                      tableDataField->dataFieldContextInfoGet())
                      ->typesGet()
                      .begin());
  if (allowed_field_types.size() &&
      allowed_field_types.find(*field_type) == allowed_field_types.end()) {
    // This indicates that this particular getter can only be used for a
    // specific set of DataFieldTypes and the fieldtype of the field
    // corresponding to the passed in field id does not match with any of
    // them
    LOG_ERROR("%s:%d %s ERROR : This getter cannot be used for field id %d",
              __func__,
              __LINE__,
              table.tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }
  auto status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      table, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s ERROR : Output param compatibility check failed for field "
        "id "
        "%d",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        field_id);
    return status;
  }
  return TDI_SUCCESS;
}

/* This is a very light function whose only responsibility is to set the input
   to the output depending on wether the output is a 64 bit value or a byte
   array. The functions calling this, need to ensure the sanity of the params
 */

void setInputValToOutputVal(const tdi::Table &table,
                            const tdi_id_t &field_id,
                            const uint64_t &input_val,
                            uint64_t *value,
                            uint8_t *value_ptr) {
  if (value_ptr) {
    auto tableDataField = table.tableInfoGet()->dataFieldGet(field_id, 0);
    utils::TableFieldUtils::toNetworkOrderData(
        *tableDataField, input_val, value_ptr);
  } else {
    *value = input_val;
  }
}
#endif

bool is_indirect_resource(const DataFieldType &type) {
  switch (type) {
    case DataFieldType::COUNTER_INDEX:
    case DataFieldType::REGISTER_INDEX:
    case DataFieldType::METER_INDEX:
      return true;
    default:
      return false;
  }
  return false;
}

// Initialization is required only for direct/indirect resources. For
// ACTION_PARAM type, we don't need to initialize, since they start off a
// zero-initialized byte-arrays.
bool initialization_required(const DataFieldType &type) {
  switch (type) {
    case DataFieldType::COUNTER_INDEX:
    case DataFieldType::METER_INDEX:
    case DataFieldType::REGISTER_INDEX:
    case DataFieldType::COUNTER_SPEC_BYTES:
    case DataFieldType::COUNTER_SPEC_PACKETS:
    case DataFieldType::METER_SPEC_CIR_PPS:
    case DataFieldType::METER_SPEC_PIR_PPS:
    case DataFieldType::METER_SPEC_CBS_PKTS:
    case DataFieldType::METER_SPEC_PBS_PKTS:
    case DataFieldType::METER_SPEC_CIR_KBPS:
    case DataFieldType::METER_SPEC_PIR_KBPS:
    case DataFieldType::METER_SPEC_CBS_KBITS:
    case DataFieldType::METER_SPEC_PBS_KBITS:
      return true;
    default:
      return false;
  }
  return false;
}

template <class TableData>
void reset_action_data(const tdi_id_t &new_act_id, TableData *table_data) {
  const auto &action_id = table_data->actionIdGet();
  const Table *table = nullptr;
  tdi_status_t sts = table_data->getParent(&table);

  if (sts != TDI_SUCCESS) {
    LOG_ERROR(
        "%s:%d ERROR in getting parent for data object", __func__, __LINE__);
    TDI_DBGCHK(0);
    return;
  }
  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
      table->tableInfoGet()->tableContextInfoGet());

  // Following are the cases that are handled
  // 1. new action_id = 0
  //    Allocate pipe_action_spec with max data size possible
  // 2. new action_id != old action id
  //    Existing object's action id is different, then a new PipeActionSpec
  //    is allocated and the existing PipeActionSpec, which is managed by a
  //    unique_ptr underneath, is reset with the newly allocated object.
  //
  // 3.  new action id == old action id
  //    Existing object's action_id is same as what is passed, then we just
  //     need to zero out the PipeActionSpec

  if (!new_act_id) {
    PipeActionSpec *action_spec = nullptr;
    action_spec = new PipeActionSpec(mat_context_info->maxDataSzGet(),
                                     mat_context_info->maxDataSzBitsGet(),
                                     PIPE_ACTION_DATA_TYPE);
    table_data->actionSpecSet(action_spec);
  } else if (new_act_id != action_id) {
    PipeActionSpec *action_spec = nullptr;
    auto action = table->tableInfoGet()->actionGet(new_act_id);
    if (!action || !action->actionContextInfoGet()) {
      TDI_DBGCHK(0);
    }
    auto rt_action_context = static_cast<const RtActionContextInfo *>(
        action->actionContextInfoGet());
    action_spec = new PipeActionSpec(rt_action_context->dataSzGet(),
                                     rt_action_context->dataSzBitsGet(),
                                     PIPE_ACTION_DATA_TYPE);
    table_data->actionSpecSet(action_spec);
  } else {
    // Just ZERO out the pipe action spec
    table_data->actionSpecSet(nullptr);
  }

  return;
}
}  // Anonymous namespace

// PipeActionSpec class
tdi_status_t PipeActionSpec::setValueActionParam(
    const tdi::DataFieldInfo &field,
    const uint64_t &value,
    const uint8_t *value_ptr) {
  size_t field_size = field.sizeGet();
  size_t field_offset = static_cast<const RtDataFieldContextInfo *>(
                            field.dataFieldContextInfoGet())
                            ->offsetGet();

  auto size = (field_size + 7) / 8;
  if (value_ptr) {
    std::memcpy(
        action_spec.act_data.action_data_bits + field_offset, value_ptr, size);
  } else {
    utils::TableFieldUtils::toNetworkOrderData(
        field, value, action_spec.act_data.action_data_bits + field_offset);
  }
  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::getValueActionParam(
    const tdi::DataFieldInfo &field,
    uint64_t *value,
    uint8_t *value_ptr) const {
  size_t field_size = field.sizeGet();
  size_t field_offset = static_cast<const RtDataFieldContextInfo *>(
                            field.dataFieldContextInfoGet())
                            ->offsetGet();

  auto size = (field_size + 7) / 8;
  if (value_ptr) {
    std::memcpy(
        value_ptr, action_spec.act_data.action_data_bits + field_offset, size);
  } else {
    utils::TableFieldUtils::toHostOrderData(
        field, action_spec.act_data.action_data_bits + field_offset, value);
  }
  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::setValueResourceIndex(
    const tdi::DataFieldInfo &field,
    const pipe_tbl_hdl_t &tbl_hdl,
    const uint64_t &value,
    const uint8_t *value_ptr) {
  uint64_t resource_idx = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(field, value_ptr, &resource_idx);
  } else {
    resource_idx = value;
  }

  pipe_res_spec_t *res_spec = nullptr;
  updateResourceSpec(tbl_hdl, false, &res_spec);
  res_spec->tbl_hdl = tbl_hdl;
  res_spec->tbl_idx = static_cast<pipe_res_idx_t>(resource_idx);
  res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::getValueResourceIndex(
    const tdi::DataFieldInfo &field,
    const pipe_tbl_hdl_t &tbl_hdl,
    uint64_t *value,
    uint8_t *value_ptr) const {
  const pipe_res_spec_t *res_spec = getResourceSpec(tbl_hdl);
  if (!res_spec) {
    LOG_ERROR("%s:%d ERROR : Resource for hdl %d does not exist for the table",
              __func__,
              __LINE__,
              tbl_hdl);
    return TDI_INVALID_ARG;
  }
  uint64_t local_val = res_spec->tbl_idx;

  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(field, local_val, value_ptr);
  } else {
    *value = local_val;
  }
  return TDI_SUCCESS;
  ;
}

// This variant is used by tableEntryGet function to populate the counter spec
// in the data object with the counter spec read from the underlying pipe mgr
tdi_status_t PipeActionSpec::setValueCounterSpec(
    const pipe_stat_data_t &counter) {
  // populate the internal counter object
  counter_spec.setCounterDataFromCounterSpec(counter);

  return TDI_SUCCESS;
}

// This variant is used to set the counter spec in the data object by the user
// and also in the pipe action spec
tdi_status_t PipeActionSpec::setValueCounterSpec(
    const tdi::DataFieldInfo &field,
    const DataFieldType &field_type,
    const pipe_tbl_hdl_t &tbl_hdl,
    const uint64_t &value,
    const uint8_t *value_ptr) {
  uint64_t counter_val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(field, value_ptr, &counter_val);
  } else {
    counter_val = value;
  }

  // populate the internal counter spec
  counter_spec.setCounterDataFromValue(field_type, counter_val);

  pipe_res_spec_t *res_spec = nullptr;
  updateResourceSpec(tbl_hdl, true, &res_spec);
  res_spec->tbl_hdl = tbl_hdl;
  res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;

  switch (field_type) {
    case DataFieldType::COUNTER_SPEC_BYTES:
      res_spec->data.counter.bytes = counter_val;
      break;
    case DataFieldType::COUNTER_SPEC_PACKETS:
      res_spec->data.counter.packets = counter_val;
      break;
    default:
      LOG_ERROR(
          "%s:%d ERROR : Invalid field type encountered %d while trying to set "
          "counter data",
          __func__,
          __LINE__,
          static_cast<int>(field_type));
      return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::getValueCounterSpec(
    const tdi::DataFieldInfo &field,
    const DataFieldType &field_type,
    uint64_t *value,
    uint8_t *value_ptr) const {
  uint64_t counter_val = 0;

  // populate the value from the counter spec
  counter_spec.getCounterData(field_type, &counter_val);

  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(field, counter_val, value_ptr);
  } else {
    *value = counter_val;
  }

  return TDI_SUCCESS;
}

// This variant is used by tableEntryGet function to populate the register spec
// in the data object with the register spec read from the underlying pipe mgr
tdi_status_t PipeActionSpec::setValueRegisterSpec(
    const std::vector<pipe_stful_mem_spec_t> &register_data) {
  register_spec.populateDataFromStfulSpec(
      register_data, static_cast<uint32_t>(register_data.size()));

  return TDI_SUCCESS;
}

// This variant is used to set the register spec in the data object by the user
// and also in the pipe action spec
tdi_status_t PipeActionSpec::setValueRegisterSpec(
    const tdi::DataFieldInfo &field,
    const DataFieldType & /*field_type*/,
    const pipe_tbl_hdl_t &tbl_hdl,
    const uint64_t &value,
    const uint8_t *value_ptr,
    const size_t & /*field_size*/) {
  uint64_t register_val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(field, value_ptr, &register_val);
  } else {
    register_val = value;
  }

  // Populate the reg spec data obj
  register_spec.addRegisterToVec(field, register_val);

  pipe_res_spec_t *res_spec = nullptr;
  updateResourceSpec(tbl_hdl, true, &res_spec);
  res_spec->tbl_hdl = tbl_hdl;
  res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;

  // Copy the reg spec into the action spec
  register_spec.populateStfulSpecFromData(&res_spec->data.stful);

  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::getValueRegisterSpec(
    const tdi::DataFieldInfo &field, std::vector<uint64_t> *value) const {
  const auto &tdi_registers = register_spec.getRegisterVec();

  for (const auto &tdi_register_data : tdi_registers) {
    uint64_t reg_val = tdi_register_data.getData(field);
    value->push_back(reg_val);
  }

  return TDI_SUCCESS;
}

// This variant is used by tableEntryGet function to populate the meter spec
// in the data object with the meter spec read from the underlying pipe mgr
tdi_status_t PipeActionSpec::setValueMeterSpec(const pipe_meter_spec_t &meter) {
  // populate the internal meter spec object
  meter_spec.setMeterDataFromMeterSpec(meter);

  return TDI_SUCCESS;
}

// This variant is used to set the meter spec in the data object by the user
// and also in the pipe action spec
tdi_status_t PipeActionSpec::setValueMeterSpec(const tdi::DataFieldInfo &field,
                                               const DataFieldType &field_type,
                                               const pipe_tbl_hdl_t &tbl_hdl,
                                               const uint64_t &value,
                                               const uint8_t *value_ptr) {
  uint64_t meter_val = 0;
  if (value_ptr) {
    utils::TableFieldUtils::toHostOrderData(field, value_ptr, &meter_val);
  } else {
    meter_val = value;
  }

  // populate the meter obj
  meter_spec.setMeterDataFromValue(field_type, meter_val);

  pipe_res_spec_t *res_spec = nullptr;
  updateResourceSpec(tbl_hdl, true, &res_spec);
  res_spec->tbl_hdl = tbl_hdl;
  res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;

  // populate the pipe action spec
  switch (field_type) {
    case METER_SPEC_CIR_PPS:
      res_spec->data.meter.cir.value.pps = meter_val;
      res_spec->data.meter.cir.type = METER_RATE_TYPE_PPS;
      break;
    case METER_SPEC_PIR_PPS:
      res_spec->data.meter.pir.value.pps = meter_val;
      res_spec->data.meter.pir.type = METER_RATE_TYPE_PPS;
      break;
    case METER_SPEC_CBS_PKTS:
      res_spec->data.meter.cburst = meter_val;
      break;
    case METER_SPEC_PBS_PKTS:
      res_spec->data.meter.pburst = meter_val;
      break;
    case METER_SPEC_CIR_KBPS:
      res_spec->data.meter.cir.value.kbps = meter_val;
      res_spec->data.meter.cir.type = METER_RATE_TYPE_KBPS;
      break;
    case METER_SPEC_PIR_KBPS:
      res_spec->data.meter.pir.value.kbps = meter_val;
      res_spec->data.meter.pir.type = METER_RATE_TYPE_KBPS;
      break;
    case METER_SPEC_CBS_KBITS:
      res_spec->data.meter.cburst = meter_val;
      break;
    case METER_SPEC_PBS_KBITS:
      res_spec->data.meter.pburst = meter_val;
      break;
    default:
      LOG_ERROR(
          "%s:%d ERROR : Invalid field type encountered %d while trying to set "
          "meter data",
          __func__,
          __LINE__,
          static_cast<int>(field_type));
      return TDI_INVALID_ARG;
  }

  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::getValueMeterSpec(const tdi::DataFieldInfo &field,
                                               const DataFieldType &field_type,
                                               uint64_t *value,
                                               uint8_t *value_ptr) const {
  uint64_t meter_val = 0;
  // populate the value from the meter spec
  meter_spec.getMeterDataFromValue(field_type, &meter_val);

  if (value_ptr) {
    utils::TableFieldUtils::toNetworkOrderData(field, meter_val, value_ptr);
  } else {
    *value = meter_val;
  }

  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::setValueActionDataHdlType() {
  action_spec.pipe_action_datatype_bmap = PIPE_ACTION_DATA_HDL_TYPE;
  return TDI_SUCCESS;
}

tdi_status_t PipeActionSpec::setValueSelectorGroupHdlType() {
  action_spec.pipe_action_datatype_bmap = PIPE_SEL_GRP_HDL_TYPE;
  return TDI_SUCCESS;
}

// This function will return a non null resource_spec if we have already
// inserted the said resource spec handle in the action spec. Thus all
// the functions trying to just query the resource spec for a given resource
// handle should always expect a non null resource spec and should error
// otherwise. On the other hand, when trying to add a new resource handle in
// the action spec, resource spec returned should be null. Thus all the
// functions trying to insert a resource handle in the action spec should
// expect a null resource spec returned from this function and then insert
// the resource handle in the next unused location
pipe_res_spec_t *PipeActionSpec::getResourceSpec(
    const pipe_tbl_hdl_t &tbl_hdl) {
  pipe_res_spec_t *res_spec = NULL;
  for (auto i = 0; i < action_spec.resource_count; i++) {
    res_spec = &action_spec.resources[i];
    if (res_spec->tbl_hdl == tbl_hdl) {
      break;
    } else {
      res_spec = NULL;
    }
  }
  return res_spec;
}

const pipe_res_spec_t *PipeActionSpec::getResourceSpec(
    const pipe_tbl_hdl_t &tbl_hdl) const {
  const pipe_res_spec_t *res_spec = NULL;
  for (auto i = 0; i < action_spec.resource_count; i++) {
    res_spec = &action_spec.resources[i];
    if (res_spec->tbl_hdl == tbl_hdl) {
      break;
    } else {
      res_spec = NULL;
    }
  }
  return res_spec;
}

void PipeActionSpec::updateResourceSpec(const pipe_tbl_hdl_t &tbl_hdl,
                                        const bool &&is_direct_resource,
                                        pipe_res_spec_t **res_spec) {
  *res_spec = getResourceSpec(tbl_hdl);
  if (!(*res_spec)) {
    *res_spec = &action_spec.resources[action_spec.resource_count++];
    if (is_direct_resource) {
      this->num_direct_resources++;
    } else {
      this->num_indirect_resources++;
    }
  }
}

// RESOURCE SPEC DATA
// REGISTER TABLE
tdi_status_t RegisterSpec::setDataFromStfulSpec(
    const pipe_stful_mem_spec_t &stful_spec) {
  memcpy(&stful, &stful_spec, sizeof(stful));
  return TDI_SUCCESS;
}

tdi_status_t RegisterSpec::getStfulSpecFromData(
    pipe_stful_mem_spec_t *stful_spec) const {
  memcpy(stful_spec, &stful, sizeof(*stful_spec));
  return TDI_SUCCESS;
}

tdi_status_t RegisterSpec::setData(const tdi::DataFieldInfo &tableDataField,
                                   uint64_t value) {
  auto fieldTypes = static_cast<const RtDataFieldContextInfo *>(
                        tableDataField.dataFieldContextInfoGet())
                        ->typesGet();
  size_t field_size = tableDataField.sizeGet();
  // Register table data fields have to be byte alighned except for 1 bit mode
  if (field_size != 1) {
    TDI_ASSERT(field_size % 8 == 0);
  }

  bool hi_value = false;
  bool lo_value = false;

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::REGISTER_SPEC_HI:
        hi_value = true;
        break;
      case DataFieldType::REGISTER_SPEC_LO:
        lo_value = true;
        break;
      case DataFieldType::REGISTER_SPEC:
        break;
      default:
        LOG_ERROR("%s:%d ERROR : Invalid data field type for field id %d",
                  __func__,
                  __LINE__,
                  tableDataField.idGet());
        TDI_ASSERT(0);
    }
  }
  switch (field_size) {
    case 1:
      TDI_ASSERT(hi_value == false);
      TDI_ASSERT(lo_value == false);
      stful.bit = (value & 0x1);
      break;
    case 64:
      TDI_ASSERT(hi_value == false);
      TDI_ASSERT(lo_value == false);
      stful.dbl_word.hi = value >> 32;
      stful.dbl_word.lo = value & 0xffffffff;
      break;
    case 8:
      if (hi_value) {
        stful.dbl_byte.hi = (value & 0xff);
      } else if (lo_value) {
        stful.dbl_byte.lo = (value & 0xff);
      } else {
        stful.byte = (value & 0xff);
      }
      break;
    case 16:
      if (hi_value) {
        stful.dbl_half.hi = (value & 0xffff);
      } else if (lo_value) {
        stful.dbl_half.lo = (value & 0xffff);
      } else {
        stful.half = (value & 0xffff);
      }
      break;
    case 32:
      if (hi_value) {
        stful.dbl_word.hi = (value & 0xffffffff);
      } else if (lo_value) {
        stful.dbl_word.lo = (value & 0xffffffff);
      } else {
        stful.word = (value & 0xffffffff);
      }
      break;
    default:
      LOG_ERROR("%s:%d ERROR : Invalid field size encountered for field id %d",
                __func__,
                __LINE__,
                tableDataField.idGet());
      TDI_ASSERT(0);
  }
  return TDI_SUCCESS;
}

uint64_t RegisterSpec::getData(const tdi::DataFieldInfo &tableDataField) const {
  auto fieldTypes = static_cast<const RtDataFieldContextInfo *>(
                        tableDataField.dataFieldContextInfoGet())
                        ->typesGet();
  size_t field_size = tableDataField.sizeGet();
  // Register table data fields have to be byte alighned except for 1 bit mode
  if (field_size != 1) {
    TDI_ASSERT(field_size % 8 == 0);
  }

  bool hi_value = false;
  bool lo_value = false;

  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::REGISTER_SPEC_HI:
        hi_value = true;
        break;
      case DataFieldType::REGISTER_SPEC_LO:
        lo_value = true;
        break;
      case DataFieldType::REGISTER_SPEC:
        break;
      default:
        LOG_ERROR("%s:%d ERROR :Invalid data field type for field id %d",
                  __func__,
                  __LINE__,
                  tableDataField.idGet());
        TDI_ASSERT(0);
    }
  }

  switch (field_size) {
    case 1:
      TDI_ASSERT(hi_value == false);
      TDI_ASSERT(lo_value == false);
      return stful.bit;
    case 8:
      if (hi_value) {
        return stful.dbl_byte.hi;
      } else if (lo_value) {
        return stful.dbl_byte.lo;
      } else {
        return stful.byte;
      }
    case 16:
      if (hi_value) {
        return stful.dbl_half.hi;
      } else if (lo_value) {
        return stful.dbl_half.lo;
      } else {
        return stful.half;
      }
    case 32:
      if (hi_value) {
        return stful.dbl_word.hi;
      } else if (lo_value) {
        return stful.dbl_word.lo;
      } else {
        return stful.word;
      }
    case 64: {
      return ((uint64_t)stful.dbl_word.hi << 32) | stful.dbl_word.lo;
    }
    default:
      LOG_ERROR("%s:%d ERROR : Invalid field size encountered for field id %d",
                __func__,
                __LINE__,
                tableDataField.idGet());
      TDI_ASSERT(0);
  }
  return 0;
}

// This function is used to set a register and add it to the vector. This
// register will then be retrieved by populateStfulSpecFromData to set the
// set the stful mem spec while adding entry into the table.
void RegisterSpecData::addRegisterToVec(
    const tdi::DataFieldInfo &tableDataField, const uint64_t &value) {
  if (registers_.size()) {
    auto &tdi_register = registers_[0];
    tdi_register.setData(tableDataField, value);
  } else {
    RegisterSpec tdi_register;
    tdi_register.setData(tableDataField, value);
    registers_.push_back(tdi_register);
  }
}

void RegisterSpecData::setFirstRegister(
    const tdi::DataFieldInfo &tableDataField, const uint64_t &value) {
  if (registers_.empty()) {
    RegisterSpec tdi_register;
    tdi_register.setData(tableDataField, value);
    registers_.push_back(tdi_register);
  } else {
    auto &tdi_register = registers_[0];
    tdi_register.setData(tableDataField, value);
  }
}

void RegisterSpecData::populateDataFromStfulSpec(
    const std::vector<pipe_stful_mem_spec_t> &stful_spec, uint32_t size) {
  for (uint32_t i = 0; i < size && i < stful_spec.size(); i++) {
    RegisterSpec tdi_register;
    tdi_register.setDataFromStfulSpec(stful_spec[i]);
    registers_.push_back(tdi_register);
  }
}

void RegisterSpecData::populateStfulSpecFromData(
    pipe_stful_mem_spec_t *stful_spec) const {
  if (!registers_.size()) {
    LOG_ERROR("%s:%d Trying to populate stful spec from an empty register data",
              __func__,
              __LINE__);
    TDI_ASSERT(0);
  }

  // This function is called when we want to populate the stful spec so as to
  // program the pipe mgr during tableEntryAdd. As a result it only makes
  // sense to have one register in the vector. Otherwise which one should be
  // picked to program the pipe mgr? Thus add safety assert here
  if (registers_.size() != 1) {
    LOG_ERROR(
        "%s:%d Cannot have multiple register populated in the vector during "
        "entry add",
        __func__,
        __LINE__);
    TDI_ASSERT(0);
  }
  const auto &register_data = registers_[0];
  register_data.getStfulSpecFromData(stful_spec);
}

void CounterSpecData::setCounterDataFromCounterSpec(
    const pipe_stat_data_t &counter) {
  std::memcpy(&counter_data, &counter, sizeof(counter_data));
}

void CounterSpecData::setCounterDataFromValue(const DataFieldType &field_type,
                                              const uint64_t &value) {
  switch (field_type) {
    case DataFieldType::COUNTER_SPEC_BYTES:
      counter_data.bytes = value;
      break;
    case DataFieldType::COUNTER_SPEC_PACKETS:
      counter_data.packets = value;
      break;
    default:
      LOG_ERROR("%s:%d Invalid Field type %d for counter data",
                __func__,
                __LINE__,
                static_cast<int>(field_type));
      TDI_ASSERT(0);
  }
}

void CounterSpecData::getCounterData(const DataFieldType &field_type,
                                     uint64_t *value) const {
  switch (field_type) {
    case DataFieldType::COUNTER_SPEC_BYTES:
      *value = counter_data.bytes;
      break;
    case DataFieldType::COUNTER_SPEC_PACKETS:
      *value = counter_data.packets;
      break;
    default:
      LOG_ERROR("%s:%d Invalid Field type %d for counter data",
                __func__,
                __LINE__,
                static_cast<int>(field_type));
      TDI_ASSERT(0);
  }
}

void MeterSpecData::setMeterDataFromValue(const DataFieldType &fieldType,
                                          const uint64_t &value) {
  switch (fieldType) {
    case METER_SPEC_CIR_PPS:
      setCIRPps(value);
      break;
    case METER_SPEC_PIR_PPS:
      setPIRPps(value);
      break;
    case METER_SPEC_CBS_PKTS:
      setCBSPkts(value);
      break;
    case METER_SPEC_PBS_PKTS:
      setPBSPkts(value);
      break;
    case METER_SPEC_CIR_KBPS:
      setCIRKbps(value);
      break;
    case METER_SPEC_PIR_KBPS:
      setPIRKbps(value);
      break;
    case METER_SPEC_CBS_KBITS:
      setCBSKbits(value);
      break;
    case METER_SPEC_PBS_KBITS:
      setPBSKbits(value);
      break;
    default:
      TDI_ASSERT(0);
      break;
  }
}

void MeterSpecData::setMeterDataFromMeterSpec(const pipe_meter_spec_t &mspec) {
  if (mspec.cir.type == METER_RATE_TYPE_KBPS) {
    TDI_ASSERT(mspec.pir.type == METER_RATE_TYPE_KBPS);
    setCIRKbps(mspec.cir.value.kbps);
    setPIRKbps(mspec.pir.value.kbps);
    setCBSKbits(mspec.cburst);
    setPBSKbits(mspec.pburst);
  } else if (mspec.cir.type == METER_RATE_TYPE_PPS) {
    TDI_ASSERT(mspec.pir.type == METER_RATE_TYPE_PPS);
    setCIRPps(mspec.cir.value.pps);
    setPIRPps(mspec.pir.value.pps);
    setCBSPkts(mspec.cburst);
    setPBSPkts(mspec.pburst);
  } else {
    TDI_ASSERT(0);
  }
}

void MeterSpecData::getMeterDataFromValue(const DataFieldType &fieldType,
                                          uint64_t *value) const {
  switch (fieldType) {
    case METER_SPEC_CIR_PPS:
      *value = pipe_meter_spec.cir.value.pps;
      break;
    case METER_SPEC_PIR_PPS:
      *value = pipe_meter_spec.pir.value.pps;
      break;
    case METER_SPEC_CBS_PKTS:
      *value = pipe_meter_spec.cburst;
      break;
    case METER_SPEC_PBS_PKTS:
      *value = pipe_meter_spec.pburst;
      break;
    case METER_SPEC_CIR_KBPS:
      *value = pipe_meter_spec.cir.value.kbps;
      break;
    case METER_SPEC_PIR_KBPS:
      *value = pipe_meter_spec.pir.value.kbps;
      break;
    case METER_SPEC_CBS_KBITS:
      *value = pipe_meter_spec.cburst;
      break;
    case METER_SPEC_PBS_KBITS:
      *value = pipe_meter_spec.pburst;
      break;
    default:
      TDI_ASSERT(0);
      break;
  }
}

// MATCH ACTION TABLE DATA

tdi_status_t MatchActionTableData::setValue(const tdi_id_t &field_id,
                                            const uint64_t &value) {
  return this->setValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t MatchActionTableData::setValue(const tdi_id_t &field_id,
                                            const uint8_t *ptr,
                                            const size_t &s) {
  return this->setValueInternal(field_id, 0, ptr, s);
}

tdi_status_t MatchActionTableData::setValue(const tdi_id_t &field_id,
                                            const std::string &value) {
  return TDI_SUCCESS;
}

tdi_status_t MatchActionTableData::setValue(const tdi_id_t &field_id,
                                            const float &value) {
  return TDI_SUCCESS;
}

// Register values are returned one instance per pipe and the stage the table
// lives in. Thus if one wants to query a register for a table which is in
// 4 pipes and 1 stage in each pipe, then the returned vector will have 4
// elements.
tdi_status_t MatchActionTableData::getValue(
    const tdi_id_t &field_id, std::vector<uint64_t> *value) const {
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  size_t field_size = tableDataField->sizeGet();
  if (field_size > 32) {
    LOG_ERROR(
        "ERROR %s:%d %s ERROR : This getter cannot be used since field size of "
        "%zu "
        "is > 32 bits, for field id %d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        field_size,
        field_id);
    return TDI_INVALID_ARG;
  }

  getPipeActionSpecObj().getValueRegisterSpec(*tableDataField, value);

  return TDI_SUCCESS;
}

tdi_status_t MatchActionTableData::getValueInternal(const tdi_id_t &field_id,
                                                    uint64_t *value,
                                                    uint8_t *value_ptr,
                                                    const size_t &size) const {
  tdi_status_t status = TDI_SUCCESS;
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }
 // auto mat_table = static_cast<const MatchActionDirect *>(this->table_);
  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
      this->table_->tableInfoGet()->tableContextInfoGet());
  uint64_t val = 0;
  auto fieldTypes = static_cast<const RtDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::METER_SPEC_CIR_PPS:
      case DataFieldType::METER_SPEC_PIR_PPS:
      case DataFieldType::METER_SPEC_CBS_PKTS:
      case DataFieldType::METER_SPEC_PBS_PKTS:
      case DataFieldType::METER_SPEC_CIR_KBPS:
      case DataFieldType::METER_SPEC_PIR_KBPS:
      case DataFieldType::METER_SPEC_CBS_KBITS:
      case DataFieldType::METER_SPEC_PBS_KBITS: {
        return getPipeActionSpecObj().getValueMeterSpec(
            *tableDataField, fieldType, value, value_ptr);
      }
      case DataFieldType::COUNTER_SPEC_BYTES:
      case DataFieldType::COUNTER_SPEC_PACKETS: {
        return getPipeActionSpecObj().getValueCounterSpec(
            *tableDataField, fieldType, value, value_ptr);
      }
      case DataFieldType::ACTION_PARAM_OPTIMIZED_OUT:
      case DataFieldType::ACTION_PARAM: {
        return getPipeActionSpecObj().getValueActionParam(
            *tableDataField, value, value_ptr);
      }
      case DataFieldType::COUNTER_INDEX:
      case DataFieldType::REGISTER_INDEX:
      case DataFieldType::METER_INDEX: {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        return getPipeActionSpecObj().getValueResourceIndex(
            *tableDataField, res_hdl, value, value_ptr);
      }
      case DataFieldType::ACTION_MEMBER_ID:
        val = get_action_member_id();
        if (value_ptr) {
          utils::TableFieldUtils::toNetworkOrderData(
              *tableDataField, val, value_ptr);
        } else {
          *value = val;
        }
        return TDI_SUCCESS;
      case DataFieldType::SELECTOR_GROUP_ID:
        val = get_selector_group_id();
        if (value_ptr) {
          utils::TableFieldUtils::toNetworkOrderData(
              *tableDataField, val, value_ptr);
        } else {
          *value = val;
        }
        return TDI_SUCCESS;
      case DataFieldType::REGISTER_SPEC:
      case DataFieldType::REGISTER_SPEC_HI:
      case DataFieldType::REGISTER_SPEC_LO:
        LOG_ERROR("%s:%d This getter cannot be used for register fields",
                  __func__,
                  __LINE__);
        return TDI_INVALID_ARG;
      default:
        LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
        return TDI_NOT_SUPPORTED;
    }
  }
  return TDI_OBJECT_NOT_FOUND;
}

tdi_status_t MatchActionTableData::getValue(const tdi_id_t &field_id,
                                            uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t MatchActionTableData::getValue(const tdi_id_t &field_id,
                                            const size_t &size,
                                            uint8_t *value) const {
  return this->getValueInternal(field_id, nullptr, value, size);
}

tdi_status_t MatchActionTableData::getValue(const tdi_id_t &field_id,
                                            std::string *value) const {
  return TDI_SUCCESS;
}

tdi_status_t MatchActionTableData::getValue(const tdi_id_t &field_id,
                                            float *value) const {
  return TDI_SUCCESS;
}

tdi_status_t MatchActionTableData::setValueInternal(const tdi_id_t &field_id,
                                                    const uint64_t &value,
                                                    const uint8_t *value_ptr,
                                                    const size_t &s) {
  const auto &action_id = this->actionIdGet();
  const tdi::DataFieldInfo *tableDataField =
      this->table_->tableInfoGet()->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  //auto mat_table = static_cast<const MatchActionDirect *>(this->table_);
  auto mat_context_info = static_cast<const MatchActionTableContextInfo *>(
      this->table_->tableInfoGet()->tableContextInfoGet());
  auto types_vec = static_cast<const RtDataFieldContextInfo *>(
                       tableDataField->dataFieldContextInfoGet())
                       ->typesGet();
  size_t field_size = tableDataField->sizeGet();

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
  // const auto &oneof_siblings = tableDataField->oneofSiblingsGet();

  uint64_t val = 0;
  for (const auto &fieldType : types_vec) {
    switch (fieldType) {
      case (DataFieldType::ACTION_PARAM): {
        // Set the action param
        sts = getPipeActionSpecObj().setValueActionParam(
            *tableDataField, value, value_ptr);
        break;
      }
      case (DataFieldType::ACTION_PARAM_OPTIMIZED_OUT): {
        // When the action param is optimized out from the context json,
        // we need to just send 0s to the pipe mgr.
        LOG_WARN(
            "WARNING: %s:%d %s : Trying to set value for an optimized out "
            "field with id %d action id %d; Ignoring the user value and "
            "setting the field to zeros",
            __func__,
            __LINE__,
            this->table_->tableInfoGet()->nameGet().c_str(),
            tableDataField->idGet(),
            this->actionIdGet());
        std::vector<uint8_t> data_arr((tableDataField->sizeGet() + 7) / 8, 0);
        sts = getPipeActionSpecObj().setValueActionParam(
            *tableDataField, 0, data_arr.data());
        break;
      }
      case (DataFieldType::COUNTER_INDEX):
      case (DataFieldType::REGISTER_INDEX):
      case (DataFieldType::METER_INDEX): {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueResourceIndex(
            *tableDataField, res_hdl, value, value_ptr);
        break;
      }
      case (DataFieldType::COUNTER_SPEC_BYTES):
      case (DataFieldType::COUNTER_SPEC_PACKETS): {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueCounterSpec(
            *tableDataField, fieldType, res_hdl, value, value_ptr);
        break;
      }

      case (DataFieldType::REGISTER_SPEC):
      case (DataFieldType::REGISTER_SPEC_HI):
      case (DataFieldType::REGISTER_SPEC_LO): {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueRegisterSpec(
            *tableDataField, fieldType, res_hdl, value, value_ptr, field_size);
        break;
      }
      case (DataFieldType::METER_SPEC_CIR_PPS):
      case (DataFieldType::METER_SPEC_PIR_PPS):
      case (DataFieldType::METER_SPEC_CBS_PKTS):
      case (DataFieldType::METER_SPEC_PBS_PKTS):
      case (DataFieldType::METER_SPEC_CIR_KBPS):
      case (DataFieldType::METER_SPEC_PIR_KBPS):
      case (DataFieldType::METER_SPEC_CBS_KBITS):
      case (DataFieldType::METER_SPEC_PBS_KBITS): {
        pipe_tbl_hdl_t res_hdl = mat_context_info->resourceHdlGet(fieldType);
        sts = getPipeActionSpecObj().setValueMeterSpec(
            *tableDataField, fieldType, res_hdl, value, value_ptr);
        break;
      }
      case DataFieldType::ACTION_MEMBER_ID: {
        auto act_data_type_bitmap =
            getPipeActionSpecObj().getPipeActionDatatypeBitmap();
        if (act_data_type_bitmap == PIPE_SEL_GRP_HDL_TYPE) {
          LOG_ERROR(
              "%s:%d %s : ERROR : Groupr ID is set probably as %d"
              ". Cannot set Action member ID",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              this->group_id);
          return TDI_INVALID_ARG;
        }
        sts = getPipeActionSpecObj().setValueActionDataHdlType();
        if (value_ptr) {
          utils::TableFieldUtils::toHostOrderData(
              *tableDataField, value_ptr, &val);
        } else {
          val = value;
        }
        set_action_member_id(val);
        // Remove oneof sibling from active fields
        // this->removeActiveFields(oneof_siblings);
        break;
      }
      case DataFieldType::SELECTOR_GROUP_ID: {
        auto act_data_type_bitmap =
            getPipeActionSpecObj().getPipeActionDatatypeBitmap();
        if (act_data_type_bitmap == PIPE_ACTION_DATA_HDL_TYPE) {
          LOG_ERROR(
              "%s:%d %s : ERROR : Action Member ID is set probably as %d"
              ". Cannot set Group ID",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              this->action_mbr_id);
          return TDI_INVALID_ARG;
        }
        sts = getPipeActionSpecObj().setValueSelectorGroupHdlType();
        if (value_ptr) {
          utils::TableFieldUtils::toHostOrderData(
              *tableDataField, value_ptr, &val);
        } else {
          val = value;
        }
        set_selector_group_id(val);
        // Remove oneof sibling from active fields
        // this->removeActiveFields(oneof_siblings);
        break;
      }
      default:
        LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
        return TDI_INVALID_ARG;
    }
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

void MatchActionTableData::set_action_member_id(uint64_t val) {
  action_mbr_id = (tdi_id_t)val;
}

void MatchActionTableData::set_selector_group_id(uint64_t val) {
  group_id = val;
}

void MatchActionTableData::initializeDataFields() {
  const auto &all_fields =
      this->table_->tableInfoGet()->dataFieldIdListGet(this->actionIdGet());

  // Count the number of action only fields. This is equal to
  // num(all_fields) - num(common_fields)
  this->num_action_only_fields =
      all_fields.size() - this->table_->tableInfoGet()->name_data_map_.size();

  // Count the number of direct and indirect resources assoicated with
  // this data object for all fields because we do not want to miss out
  // on checking any fields.
  this->getIndirectResourceCounts(all_fields);

  for (const auto &each_field : all_fields) {
    auto *tableDataField = this->table_->tableInfoGet()->dataFieldGet(
        each_field, this->actionIdGet());
    if (!tableDataField) {
      LOG_ERROR("%s:%d %s ERROR in getting data field info  for action id %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                this->actionIdGet());
      TDI_DBGCHK(0);
      return;
    }

    auto types_vec = static_cast<const RtDataFieldContextInfo *>(
                         tableDataField->dataFieldContextInfoGet())
                         ->typesGet();
    for (const auto &fieldType : types_vec) {
      if (initialization_required(fieldType)) {
        // initialize_data_field(each_field, *table_, this->actionIdGet(),
        // this);
      }
    }
  }
  return;
}

void MatchActionTableData::getIndirectResourceCounts(
    const std::vector<tdi_id_t> field_list) {
  for (auto each_field : field_list) {
    auto *tableDataField = this->table_->tableInfoGet()->dataFieldGet(
        each_field, this->actionIdGet());
    if (!tableDataField) {
      LOG_ERROR("%s:%d %s ERROR in getting data field info  for action id %d",
                __func__,
                __LINE__,
                this->table_->tableInfoGet()->nameGet().c_str(),
                this->actionIdGet());
      TDI_DBGCHK(0);
      return;
    }
    auto types_vec = static_cast<const RtDataFieldContextInfo *>(
                         tableDataField->dataFieldContextInfoGet())
                         ->typesGet();
    this->num_indirect_resource_count +=
        std::count_if(types_vec.begin(), types_vec.end(), is_indirect_resource);
  }
}

pipe_act_fn_hdl_t MatchActionTableData::getActFnHdl() const {
  auto action = this->table_->tableInfoGet()->actionGet(this->actionIdGet());
  if (!action || !action->actionContextInfoGet()) return 0;
  auto rt_action_context = static_cast<const RtActionContextInfo *>(
      action->actionContextInfoGet());
  return rt_action_context->actionFnHdlGet();
}

tdi_status_t MatchActionTableData::reset() {
  // empty vector means set all fields
  std::vector<tdi_id_t> empty;
  this->reset(0, empty);
  return TDI_SUCCESS;
}

tdi_status_t MatchActionTableData::reset(
    const tdi_id_t &act_id, const std::vector<tdi_id_t> & /*fields*/) {
  reset_action_data<MatchActionTableData>(act_id, this);
  return TDI_SUCCESS;
}

// ACTION TABLE DATA

#if 0
tdi_status_t TdiActionTableData::setValue(const tdi_id_t &field_id,
                                          const uint64_t &value) {
  auto status = this->setValueInternal(field_id, value, nullptr, 0);

  return status;
}

tdi_status_t TdiActionTableData::setValue(const tdi_id_t &field_id,
                                          const uint8_t *ptr,
                                          const size_t &size) {
  auto status = this->setValueInternal(field_id, 0, ptr, size);

  return status;
}

tdi_status_t TdiActionTableData::getValueInternal(const tdi_id_t &field_id,
                                                  uint64_t *value,
                                                  uint8_t *value_ptr,
                                                  const size_t &s) const {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;
  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d, for action id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id,
              this->actionIdGet_());
    return TDI_INVALID_ARG;
  }

  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, value, value_ptr, s);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  auto fieldTypes = static_cast<const RtDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::ACTION_PARAM_OPTIMIZED_OUT:
      case DataFieldType::ACTION_PARAM: {
        return getPipeActionSpecObj().getValueActionParam(
            *tableDataField, value, value_ptr);
      }
      case (DataFieldType::COUNTER_INDEX):
      case (DataFieldType::REGISTER_INDEX):
      case (DataFieldType::METER_INDEX): {
        pipe_tbl_hdl_t res_hdl = this->table_->getResourceHdl(fieldType);
        return getPipeActionSpecObj().getValueResourceIndex(
            *tableDataField, res_hdl, value, value_ptr);
      }
      default:
        LOG_ERROR("ERROR: %s:%d %s : This API is not supported for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  tableDataField->idGet());
        return TDI_INVALID_ARG;
    }
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiActionTableData::getValue(const tdi_id_t &field_id,
                                          uint64_t *value) const {
  return this->getValueInternal(field_id, value, nullptr, 0);
}

tdi_status_t TdiActionTableData::getValue(const tdi_id_t &field_id,
                                          const size_t &size,
                                          uint8_t *value) const {
  return this->getValueInternal(field_id, 0, value, size);
}

tdi_status_t TdiActionTableData::setValueInternal(const tdi_id_t &field_id,
                                                  const uint64_t &value,
                                                  const uint8_t *value_ptr,
                                                  const size_t &s) {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(
      field_id, this->actionIdGet_(), &tableDataField);
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
        "%s:%d %s : Input Param bounds check failed for field id %d action id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet(),
        this->actionIdGet_());
    return sts;
  }

  auto fieldTypes = static_cast<const RtDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case (DataFieldType::ACTION_PARAM): {
        sts = getPipeActionSpecObj().setValueActionParam(
            *tableDataField, value, value_ptr);
        break;
      }
      case (DataFieldType::ACTION_PARAM_OPTIMIZED_OUT): {
        // When the action param is optimized out from the context json,
        // we need to just send 0s to the pipe mgr.
        LOG_WARN(
            "WARNING: %s:%d %s : Trying to set value for an optimized out "
            "field with id %d action id %d; Ignoring the user value and "
            "setting the field to zeros",
            __func__,
            __LINE__,
            this->table_->tableInfoGet()->nameGet().c_str(),
            tableDataField->idGet(),
            this->actionIdGet_());
        std::vector<uint8_t> data_arr((tableDataField->sizeGet() + 7) / 8, 0);
        sts = getPipeActionSpecObj().setValueActionParam(
            *tableDataField, 0, data_arr.data());
        break;
      }
      case (DataFieldType::COUNTER_INDEX):
      case (DataFieldType::REGISTER_INDEX):
      case (DataFieldType::METER_INDEX): {
        pipe_tbl_hdl_t res_hdl = this->table_->getResourceHdl(fieldType);
        sts = getPipeActionSpecObj().setValueResourceIndex(
            *tableDataField, res_hdl, value, value_ptr);
        if (sts != TDI_SUCCESS) {
          LOG_ERROR("%s:%d %s : Unable set resource index for field with id %d",
                    __func__,
                    __LINE__,
                    this->table_->tableInfoGet()->nameGet().c_str(),
                    tableDataField->idGet());
          return sts;
        }
        // This is an indirect resource. Add this to the indirect resource Map.
        // This map is retrieved during MatchActionIndirect table entry add to
        // retrieve the indirect resources, since its required during match
        // entry add
        uint64_t resource_idx = 0;
        if (value_ptr) {
          utils::TableFieldUtils::toHostOrderData(
              *tableDataField, value_ptr, &resource_idx);
        } else {
          resource_idx = value;
        }
        auto elem = resource_map.find(fieldType);
        if (elem != resource_map.end()) {
          // This should not happen. This means that there are two action
          // parameters for the same indirect resource. NOT supported.
          TDI_ASSERT(0);
        }
        resource_map[fieldType] = resource_idx;
        break;
      }
      default:
        LOG_ERROR("%s:%d %s ERROR : This API is not supported for field id %d",
                  __func__,
                  __LINE__,
                  this->table_->tableInfoGet()->nameGet().c_str(),
                  tableDataField->idGet());
        return TDI_INVALID_ARG;
    }
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

pipe_act_fn_hdl_t TdiActionTableData::getActFnHdl() const {
  return this->table_->getActFnHdl(this->actionIdGet_());
}

tdi_status_t TdiActionTableData::reset(const tdi_id_t &act_id) {
  reset_action_data<TdiActionTableData>(act_id, this);
  this->init(act_id);
  return TDI_SUCCESS;
}

tdi_status_t TdiActionTableData::reset() { return this->reset(0); }

// SELECTOR TABLE DATA
TdiSelectorTableData::TdiSelectorTableData(
    const tdi::Table *tbl_obj, const std::vector<tdi_id_t> &fields)
    : TdiTableDataObj(tbl_obj), act_fn_hdl_() {
  tdi_id_t max_grp_size_id;
  tdi_status_t sts =
      tbl_obj->dataFieldIdGet("$MAX_GROUP_SIZE", &max_grp_size_id);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d ERROR Field Id for \"$MAX_GROUP_SIZE\" field not found",
              __func__,
              __LINE__);
    TDI_DBGCHK(0);
  }
  if (fields.empty() ||
      (std::find(fields.begin(), fields.end(), max_grp_size_id) !=
       fields.end())) {
    // Set the max group size to the default value parsed from tdi json
    // We don't need to worry about the members and member sts arrays because
    // they will be empty anyway by default
    uint64_t default_value;
    sts = tbl_obj->defaultDataValueGet(
        max_grp_size_id, 0 /* this->actionIdGet_() */, &default_value);
    if (sts != TDI_SUCCESS) {
      // For some reason we were unable to get the default value from the
      // tdi json. So just set it to 0
      LOG_ERROR("%s:%d ERROR Unable to get the default value for field %d",
                __func__,
                __LINE__,
                max_grp_size_id);
      TDI_DBGCHK(0);
      default_value = 0;
    }
    sts = this->setValueInternal(max_grp_size_id, 0, default_value, nullptr);
    if (sts != TDI_SUCCESS) {
      // Unable to set the default value
      LOG_ERROR("%s:%d ERROR Unable to set the default value for field %d",
                __func__,
                __LINE__,
                max_grp_size_id);
      TDI_DBGCHK(0);
    }
  }
}

std::vector<tdi_id_t> TdiSelectorTableData::get_members_from_array(
    const uint8_t *value, const size_t &size) {
  std::vector<tdi_id_t> grp_members;
  for (unsigned i = 0; i < size; i += sizeof(tdi_id_t)) {
    tdi_id_t this_member = *(reinterpret_cast<const tdi_id_t *>(value + i));
    this_member = ntohl(this_member);
    grp_members.push_back(this_member);
  }
  return grp_members;
}

tdi_status_t TdiSelectorTableData::setValueInternal(const tdi_id_t &field_id,
                                                    const size_t &size,
                                                    const uint64_t &value,
                                                    const uint8_t *value_ptr) {
  uint64_t val = 0;
  std::set<DataFieldType> allowed_field_types = {DataFieldType::MAX_GROUP_SIZE};
  DataFieldType field_type;
  auto sts = indirectResourceSetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &val,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d %s : Set value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  max_grp_size_ = static_cast<uint32_t>(val);
  return TDI_SUCCESS;
}

tdi_status_t TdiSelectorTableData::setValue(const tdi_id_t &field_id,
                                            const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiSelectorTableData::setValue(const tdi_id_t &field_id,
                                            const uint8_t *value_ptr,
                                            const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t TdiSelectorTableData::setValue(
    const tdi_id_t &field_id, const std::vector<tdi_id_t> &arr) {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;
  status = this->table_->getDataField(field_id, &tableDataField);

  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }
  // Next, check if this setter can be used
  if (!tableDataField->isIntArr()) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not an integer array",
        __func__,
        __LINE__,
        field_id,
        this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  members_ = arr;
  return TDI_SUCCESS;
}

tdi_status_t TdiSelectorTableData::setValue(const tdi_id_t &field_id,
                                            const std::vector<bool> &arr) {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return status;
  }
  // Next, check if this setter can be used
  if (!tableDataField->isBoolArr()) {
    LOG_ERROR(
        "%s:%d This setter cannot be used for field id %d, for table %s, since "
        "the field is not a bool array",
        __func__,
        __LINE__,
        field_id,
        this->table_->tableInfoGet()->nameGet().c_str());
    return TDI_NOT_SUPPORTED;
  }

  member_status_ = arr;

  return TDI_SUCCESS;
}

tdi_status_t TdiSelectorTableData::getValueInternal(const tdi_id_t &field_id,
                                                    const size_t &size,
                                                    uint64_t *value,
                                                    uint8_t *value_ptr) const {
  DataFieldType field_type;
  std::set<DataFieldType> allowed_field_types = {DataFieldType::MAX_GROUP_SIZE};
  auto sts = indirectResourceGetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("ERROR: %s:%d %s : Get value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  uint64_t grp_size = 0;
  grp_size = max_grp_size_;
  setInputValToOutputVal(*table_, field_id, grp_size, value, value_ptr);
  return TDI_SUCCESS;
}

tdi_status_t TdiSelectorTableData::getValue(const tdi_id_t &field_id,
                                            uint64_t *value) const {
  return this->getValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiSelectorTableData::getValue(const tdi_id_t &field_id,
                                            const size_t &size,
                                            uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, size, nullptr, value_ptr);
}

tdi_status_t TdiSelectorTableData::getValue(
    const tdi_id_t &field_id, std::vector<tdi_id_t> *arr) const {
  // Get the data_field from the table
  tdi_id_t action_id;
  status = this->actionIdGet(&action_id);
  const tdi::DataFieldInfo *tableDataField =
      this->table_->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  auto fieldTypes = static_cast<const RtDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    if (fieldType == DataFieldType::SELECTOR_MEMBERS &&
        tableDataField->isIntArr()) {
      *arr = members_;
      status = TDI_SUCCESS;
    } else {
      LOG_ERROR(
          "%s:%d %s Field type other than SELECTOR_MEMBERS"
          " Not supported. Field type received %d",
          __func__,
          __LINE__,
          this->table_->tableInfoGet()->nameGet().c_str(),
          int(fieldType));
      status = TDI_NOT_SUPPORTED;
    }
  }
  return status;
}

tdi_status_t TdiSelectorTableData::getValue(const tdi_id_t &field_id,
                                            std::vector<bool> *arr) const {
  // Get the data_field from the table
  tdi_id_t action_id;
  status = this->actionIdGet(&action_id);
  const tdi::DataFieldInfo *tableDataField =
      this->table_->dataFieldGet(field_id, action_id);

  if (!tableDataField) {
    LOG_ERROR("%s:%d %s ERROR : Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }

  auto fieldTypes = static_cast<const RtDataFieldContextInfo *>(
                        tableDataField->dataFieldContextInfoGet())
                        ->typesGet();
  for (const auto &fieldType : fieldTypes) {
    if (fieldType == DataFieldType::ACTION_MEMBER_STATUS &&
        tableDataField->isBoolArr()) {
      *arr = member_status_;
      status = TDI_SUCCESS;
    } else {
      LOG_ERROR(
          "%s:%d %s Field type other than ACTION_MEMBER_STATUS"
          " Not supported. Field type received %d",
          __func__,
          __LINE__,
          this->table_->tableInfoGet()->nameGet().c_str(),
          int(fieldType));
      status = TDI_NOT_SUPPORTED;
    }
  }
  return status;
}

tdi_status_t TdiSelectorTableData::reset() {
  this->actionIdSet(0);
  act_fn_hdl_ = 0;
  members_.clear();
  member_status_.clear();
  max_grp_size_ = 0;
  return TDI_SUCCESS;
}

// Counter Table Data

tdi_status_t TdiCounterTableData::setValueInternal(const tdi_id_t &field_id,
                                                   const size_t &size,
                                                   const uint64_t &value,
                                                   const uint8_t *value_ptr) {
  uint64_t val = 0;
  DataFieldType field_type;
  // Here we pass an empty set because this can be used for any field type
  // of counter table
  std::set<DataFieldType> allowed_field_types;
  auto sts = indirectResourceSetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &val,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Set value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  getCounterSpecObj().setCounterDataFromValue(field_type, val);
  return TDI_SUCCESS;
}

tdi_status_t TdiCounterTableData::setValue(const tdi_id_t &field_id,
                                           const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiCounterTableData::setValue(const tdi_id_t &field_id,
                                           const uint8_t *value_ptr,
                                           const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t TdiCounterTableData::getValueInternal(const tdi_id_t &field_id,
                                                   const size_t &size,
                                                   uint64_t *value,
                                                   uint8_t *value_ptr) const {
  uint64_t counter = 0;
  DataFieldType field_type;
  // Here we pass an empty set because this can be used for any field type
  // of counter table
  std::set<DataFieldType> allowed_field_types;
  auto sts = indirectResourceGetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Get value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }

  getCounterSpecObj().getCounterData(field_type, &counter);
  setInputValToOutputVal(*table_, field_id, counter, value, value_ptr);

  return TDI_SUCCESS;
}

tdi_status_t TdiCounterTableData::getValue(const tdi_id_t &field_id,
                                           uint64_t *value) const {
  return this->getValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiCounterTableData::getValue(const tdi_id_t &field_id,
                                           const size_t &size,
                                           uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, size, nullptr, value_ptr);
}

tdi_status_t TdiCounterTableData::reset() {
  counter_spec_.reset();
  return TDI_SUCCESS;
}

// METER TABLE DATA

tdi_status_t TdiMeterTableData::setValueInternal(const tdi_id_t &field_id,
                                                 const size_t &size,
                                                 const uint64_t &value,
                                                 const uint8_t *value_ptr) {
  uint64_t val = 0;
  DataFieldType field_type;
  // Here we pass an empty set because this can be used for any field type
  // under meter table
  std::set<DataFieldType> allowed_field_types;
  auto sts = indirectResourceSetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &val,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Set value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  meter_spec_.setMeterDataFromValue(field_type, val);
  return TDI_SUCCESS;
}

tdi_status_t TdiMeterTableData::setValue(const tdi_id_t &field_id,
                                         const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiMeterTableData::setValue(const tdi_id_t &field_id,
                                         const uint8_t *value_ptr,
                                         const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t TdiMeterTableData::getValueInternal(const tdi_id_t &field_id,
                                                 const size_t &size,
                                                 uint64_t *value,
                                                 uint8_t *value_ptr) const {
  uint64_t meter = 0;
  DataFieldType field_type;
  // Here we pass an empty set because this can be used for any field type
  // under meter table
  std::set<DataFieldType> allowed_field_types;
  auto sts = indirectResourceGetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Get value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }
  meter_spec_.getMeterDataFromValue(field_type, &meter);
  setInputValToOutputVal(*table_, field_id, meter, value, value_ptr);
  return TDI_SUCCESS;
}

tdi_status_t TdiMeterTableData::getValue(const tdi_id_t &field_id,
                                         uint64_t *value) const {
  return this->getValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiMeterTableData::getValue(const tdi_id_t &field_id,
                                         const size_t &size,
                                         uint8_t *value_ptr) const {
  return this->getValueInternal(field_id, size, nullptr, value_ptr);
}

tdi_status_t TdiMeterTableData::reset() {
  meter_spec_.reset();
  return TDI_SUCCESS;
}

// REGISTER TABLE DATA
tdi_status_t TdiRegisterTableData::setValueInternal(const tdi_id_t &field_id,
                                                    const size_t &size,
                                                    const uint64_t &value,
                                                    const uint8_t *value_ptr) {
  uint64_t val = 0;
  DataFieldType field_type;
  // Here we pass an empty set because this can be used for any field type
  // under reggister table
  std::set<DataFieldType> allowed_field_types;
  auto sts = indirectResourceSetValueHelper(*table_,
                                            field_id,
                                            allowed_field_types,
                                            size,
                                            value,
                                            value_ptr,
                                            &val,
                                            &field_type);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Set value failed for field_id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return sts;
  }

  const tdi::DataFieldInfo *tableDataField = nullptr;
  this->table_->getDataField(field_id, &tableDataField);
  getRegisterSpecObj().setFirstRegister(*tableDataField, val);
  return TDI_SUCCESS;
}

tdi_status_t TdiRegisterTableData::setValue(const tdi_id_t &field_id,
                                            const uint64_t &value) {
  return this->setValueInternal(field_id, 0, value, nullptr);
}

tdi_status_t TdiRegisterTableData::setValue(const tdi_id_t &field_id,
                                            const uint8_t *value_ptr,
                                            const size_t &size) {
  return this->setValueInternal(field_id, size, 0, value_ptr);
}

tdi_status_t TdiRegisterTableData::getValue(
    const tdi_id_t &field_id, std::vector<uint64_t> *value) const {
  // Get the data_field from the table
  const tdi::DataFieldInfo *tableDataField = nullptr;
  auto status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR : Data field id %d not found",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_OBJECT_NOT_FOUND;
  }
  const auto &register_obj = getRegisterSpecObj();
  const auto &tdi_registers = register_obj.getRegisterVec();
  for (const auto &tdi_register_data : tdi_registers) {
    value->push_back(tdi_register_data.getData(*tableDataField));
  }
  return TDI_SUCCESS;
}

tdi_status_t TdiRegisterTableData::reset() {
  register_spec_.reset();
  return TDI_SUCCESS;
}

// RegisterParam
tdi_status_t TdiRegisterParamTableData::reset() {
  this->value = 0;
  return TDI_SUCCESS;
}

tdi_status_t TdiRegisterParamTableData::setValue(const tdi_id_t &field_id,
                                                 const uint8_t *value_ptr,
                                                 const size_t &size) {
  tdi_status_t status = TDI_SUCCESS;
  const tdi::DataFieldInfo *tableDataField = nullptr;

  status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, nullptr, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  status = utils::TableFieldUtils::boundsCheck(
      *this->table_, *tableDataField, 0, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s : Input Param bounds check failed for field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case DataType::BYTE_STREAM: {
      if (!value_ptr) {
        LOG_ERROR("Field data type is byte stream, but provided value is not.");
        return TDI_INVALID_ARG;
      }
      size_t size_bytes = (tableDataField->sizeGet() + 7) / 8;
      if (size > size_bytes) {
        LOG_ERROR(
            "%s:%d Error. Length given is %zu bytes. Field length "
            "is %zu bits. Expecting <= %zu bytes",
            __func__,
            __LINE__,
            size,
            tableDataField->sizeGet(),
            size_bytes);
        return TDI_INVALID_ARG;
      }
      utils::TableFieldUtils::toHostOrderData(
          *tableDataField,
          value_ptr,
          reinterpret_cast<uint64_t *>(&this->value));
      // Fix the sign
      uint8_t shift = (sizeof(this->value) - size) * 8;
      this->value = (this->value << shift) >> shift;
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this setter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}

tdi_status_t TdiRegisterParamTableData::getValue(const tdi_id_t &field_id,
                                                 const size_t &size,
                                                 uint8_t *value_ptr) const {
  const tdi::DataFieldInfo *tableDataField = nullptr;
  tdi_status_t status = this->table_->getDataField(field_id, &tableDataField);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d %s ERROR Invalid field id %d",
              __func__,
              __LINE__,
              this->table_->tableInfoGet()->nameGet().c_str(),
              field_id);
    return TDI_INVALID_ARG;
  }

  // Do some bounds checking using the utility functions
  status = utils::TableFieldUtils::fieldTypeCompatibilityCheck(
      *this->table_, *tableDataField, 0, value_ptr, size);
  if (status != TDI_SUCCESS) {
    LOG_ERROR(
        "ERROR: %s:%d %s : Input param compatibility check failed for field id "
        "%d",
        __func__,
        __LINE__,
        this->table_->tableInfoGet()->nameGet().c_str(),
        tableDataField->idGet());
    return status;
  }

  switch (tableDataField->getDataType()) {
    case DataType::BYTE_STREAM: {
      if (!value_ptr) {
        LOG_ERROR("Field data type is byte stream, but provided value is not.");
        return TDI_INVALID_ARG;
      }

      size_t size_bytes = (tableDataField->sizeGet() + 7) / 8;
      if (size > size_bytes) {
        LOG_ERROR(
            "%s:%d Error. Length given is %zu bytes. Field length "
            "is %zu bits. Expecting <= %zu bytes",
            __func__,
            __LINE__,
            size,
            tableDataField->sizeGet(),
            size_bytes);
        return TDI_INVALID_ARG;
      }
      uint64_t temp;
      std::memcpy(&temp, &this->value, sizeof(temp));
      utils::TableFieldUtils::toNetworkOrderData(
          *tableDataField, temp, value_ptr);
      break;
    }
    default:
      LOG_ERROR("Data type not supported by this getter");
      return TDI_INVALID_ARG;
      break;
  }

  return TDI_SUCCESS;
}
#endif

}  // namespace rt
}  // namespace pna
}  // namespace tdi

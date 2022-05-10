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

#ifndef _TDI_P4_TABLE_DATA_IMPL_HPP
#define _TDI_P4_TABLE_DATA_IMPL_HPP

#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_table_data.hpp>

#include <tdi_common/tdi_context_info.hpp>

namespace tdi {
namespace pna {
namespace rt {

namespace {
#if 0
void initialize_data_field(const tdi_id_t &field_id,
                           const tdi::Table &table,
                           const tdi_id_t &action_id,
                           tdi::TableData * /*data_obj*/) {

  // No need to initialize mandatory fields, as those will not have
  // default value in JSON.
  const tdi::DataFieldInfo *field_obj =
      table.tableInfoGet()->dataFieldGet(field_id, action_id);
  if (!field_obj) {
    LOG_ERROR(
        "%s:%d %s ERROR in getting data field object for field id %d, "
        "action id %d",
        __func__,
        __LINE__,
        table.tableInfoGet()->nameGet().c_str(),
        field_id,
        action_id);
    TDI_DBGCHK(0);
    return;
  }
  if (field_obj->mandatoryGet()) return;

  uint64_t default_value;
  std::string default_str_value;
  float default_fl_value;
  switch (field_obj->dataTypeGet()) {
    case TDI_FIELD_DATA_TYPE_UINT64:
      default_value = field_obj->defaultValueGet();
      status = data_obj->setValue(field_id, default_value);
      if (status != TDI_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            field_id,
            action_id,
            status);
        TDI_DBGCHK(0);
        return;
      }
      break;
    case TDI_FIELD_DATA_TYPE_FLOAT:
      default_fl_value = field_obj->defaultFlValueGet();
      status = data_obj->setValue(field_id, default_fl_value);
      if (status != TDI_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            field_id,
            action_id,
            status);
        TDI_DBGCHK(0);
        return;
      }
      break;
    case TDI_FIELD_DATA_TYPE_BOOL:
      default_value = field_obj->defaultValueGet();
      status = data_obj->setValue(field_id, static_cast<bool>(default_value));
      if (status != TDI_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            field_id,
            action_id,
            status);
        TDI_DBGCHK(0);
        return;
      }
      break;
    case TDI_FIELD_DATA_TYPE_STRING:
      default_str_value = field_obj->defaultStrValueGet();
      status = data_obj->setValue(field_id, default_str_value);
      if (status != TDI_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            field_id,
            action_id,
            status);
        TDI_DBGCHK(0);
        return;
      }
      break;
    default:
      break;
  }
  return;
}
#endif
}  // namespace

// This class is a software representation of how a register data field
// can be programmed in hardware. A hardware 'register' can be of size
// 1, 8, 16, 32 or 64. When in 1 size mode, the register acts in non dual
// mode and thus has only one instance published in the tdi json. When
// in 8, 16, 32 size mode, it can operate in non dual or dual mode.
// When in 64 size mode, the register always acts in dual mode.
//
// In non dual, there is only one instance published in the json while
// in dual mode there are two instances published in the json (each
// of the 8, 16 or 32 sizes respectively)
class RegisterSpec {
 public:
  tdi_status_t setDataFromStfulSpec(const pipe_stful_mem_spec_t &stful_spec);
  tdi_status_t getStfulSpecFromData(pipe_stful_mem_spec_t *stful_spec) const;

  tdi_status_t setData(const tdi::DataFieldInfo &tableDataField,
                       uint64_t value);
  uint64_t getData(const tdi::DataFieldInfo &tableDataField) const;

 private:
  pipe_stful_mem_spec_t stful = {};
};

class RegisterSpecData {
 public:
  void populateDataFromStfulSpec(
      const std::vector<pipe_stful_mem_spec_t> &stful_spec, uint32_t size);
  void reset() { registers_.clear(); }
  void populateStfulSpecFromData(pipe_stful_mem_spec_t *stful_spec) const;
  void setFirstRegister(const tdi::DataFieldInfo &tableDataField,
                        const uint64_t &value);
  void addRegisterToVec(const tdi::DataFieldInfo &tableDataField,
                        const uint64_t &value);
  const std::vector<RegisterSpec> &getRegisterVec() const { return registers_; }

 private:
  std::vector<RegisterSpec> registers_;
};

class CounterSpecData {
 public:
  CounterSpecData() { counter_data = {}; };
  void setCounterDataFromCounterSpec(const pipe_stat_data_t &counter);
  void setCounterDataFromValue(const DataFieldType &field_type,
                               const uint64_t &count);

  void getCounterData(const DataFieldType &field_type, uint64_t *count) const;

  const pipe_stat_data_t *getPipeCounterSpec() const { return &counter_data; }
  void reset() { counter_data = {0}; }

 private:
  pipe_stat_data_t counter_data;
};

class MeterSpecData {
 public:
  MeterSpecData() { pipe_meter_spec = {}; };
  void reset() { pipe_meter_spec = {}; }

  void setCIRKbps(uint64_t value) {
    pipe_meter_spec.cir.value.kbps = value;
    pipe_meter_spec.cir.type = METER_RATE_TYPE_KBPS;
  }
  void setPIRKbps(uint64_t value) {
    pipe_meter_spec.pir.value.kbps = value;
    pipe_meter_spec.pir.type = METER_RATE_TYPE_KBPS;
  }
  void setCBSKbits(uint64_t value) { pipe_meter_spec.cburst = value; }
  void setPBSKbits(uint64_t value) { pipe_meter_spec.pburst = value; }
  void setCIRPps(uint64_t value) {
    pipe_meter_spec.cir.value.pps = value;
    pipe_meter_spec.cir.type = METER_RATE_TYPE_PPS;
  }
  void setPIRPps(uint64_t value) {
    pipe_meter_spec.pir.value.pps = value;
    pipe_meter_spec.pir.type = METER_RATE_TYPE_PPS;
  }
  void setCBSPkts(uint64_t value) { pipe_meter_spec.cburst = value; }
  void setPBSPkts(uint64_t value) { pipe_meter_spec.pburst = value; }

  const pipe_meter_spec_t *getPipeMeterSpec() const { return &pipe_meter_spec; }

  void setMeterDataFromValue(const DataFieldType &type, const uint64_t &value);
  void setMeterDataFromMeterSpec(const pipe_meter_spec_t &mspec);

  void getMeterDataFromValue(const DataFieldType &type, uint64_t *value) const;

 private:
  pipe_meter_spec_t pipe_meter_spec{};
};

// This class manages the pipe action spec. The responsibilities of
// this class include
// 1. Given a DataField, set/get the value in/from the the action spec
// 2. Expose APIs to set/get the action spec through a variety of possibilities
// 3. This class internally has a few sub-composite classes (one for each
//    type of resource)
class PipeActionSpec {
 public:
  PipeActionSpec(const size_t &data_sz,
                 const size_t &data_sz_bits,
                 const uint8_t &pipe_action_datatype_bmap) {
    std::memset(&action_spec, 0, sizeof(action_spec));
    std::memset(&action_spec.act_data, 0, sizeof(action_spec.act_data));
    if (data_sz) {
      action_data_bits.reset(new uint8_t[data_sz]());
      action_spec.act_data.action_data_bits = action_data_bits.get();
      action_spec.act_data.num_action_data_bytes = data_sz;
      action_spec.act_data.num_valid_action_data_bits = data_sz_bits;
    } else {
      action_data_bits = nullptr;
      action_spec.act_data.action_data_bits = nullptr;
      action_spec.act_data.num_action_data_bytes = 0;
      action_spec.act_data.num_valid_action_data_bits = 0;
    }
    action_spec.pipe_action_datatype_bmap = pipe_action_datatype_bmap;
  }

  void resetPipeActionSpec() {
    if (action_spec.act_data.action_data_bits) {
      std::memset(action_spec.act_data.action_data_bits,
                  0,
                  action_spec.act_data.num_action_data_bytes);
    }
    std::memset(action_spec.resources,
                0,
                sizeof(pipe_res_spec_t) * PIPE_NUM_TBL_RESOURCES);
    action_spec.resource_count = 0;
    action_spec.sel_grp_hdl = 0;
    action_spec.adt_ent_hdl = 0;
    counter_spec.reset();
    meter_spec.reset();
    register_spec.reset();
    num_direct_resources = 0;
    num_indirect_resources = 0;
    action_spec.pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
  }

  ~PipeActionSpec() {}

  // Action Param
  tdi_status_t setValueActionParam(const tdi::DataFieldInfo &field,
                                   const uint64_t &value,
                                   const uint8_t *value_ptr);
  tdi_status_t getValueActionParam(const tdi::DataFieldInfo &field,
                                   uint64_t *value,
                                   uint8_t *value_ptr) const;

  // Resource Index
  tdi_status_t setValueResourceIndex(const tdi::DataFieldInfo &field,
                                     const pipe_tbl_hdl_t &tbl_hdl,
                                     const uint64_t &value,
                                     const uint8_t *value_ptr);
  tdi_status_t getValueResourceIndex(const tdi::DataFieldInfo &field,
                                     const pipe_tbl_hdl_t &tbl_hdl,
                                     uint64_t *value,
                                     uint8_t *value_ptr) const;

  // Counter
  tdi_status_t setValueCounterSpec(const pipe_stat_data_t &counter);
  tdi_status_t setValueCounterSpec(const tdi::DataFieldInfo &field,
                                   const DataFieldType &field_type,
                                   const pipe_tbl_hdl_t &tbl_hdl,
                                   const uint64_t &value,
                                   const uint8_t *value_ptr);
  tdi_status_t getValueCounterSpec(const tdi::DataFieldInfo &field,
                                   const DataFieldType &field_type,
                                   uint64_t *value,
                                   uint8_t *value_ptr) const;

  // Register
  tdi_status_t setValueRegisterSpec(
      const std::vector<pipe_stful_mem_spec_t> &register_data);
  tdi_status_t setValueRegisterSpec(const tdi::DataFieldInfo &field,
                                    const DataFieldType &field_type,
                                    const pipe_tbl_hdl_t &tbl_hdl,
                                    const uint64_t &value,
                                    const uint8_t *value_ptr,
                                    const size_t &field_size);
  tdi_status_t getValueRegisterSpec(const tdi::DataFieldInfo &field,
                                    std::vector<uint64_t> *value) const;

  // Meter
  tdi_status_t setValueMeterSpec(const pipe_meter_spec_t &meter);
  tdi_status_t setValueMeterSpec(const tdi::DataFieldInfo &field,
                                 const DataFieldType &field_type,
                                 const pipe_tbl_hdl_t &tbl_hdl,
                                 const uint64_t &value,
                                 const uint8_t *value_ptr);
  tdi_status_t getValueMeterSpec(const tdi::DataFieldInfo &field,
                                 const DataFieldType &field_type,
                                 uint64_t *value,
                                 uint8_t *value_ptr) const;

  // Action member
  tdi_status_t setValueActionDataHdlType();

  // Selector Group
  tdi_status_t setValueSelectorGroupHdlType();

  uint8_t getPipeActionDatatypeBitmap() const {
    return action_spec.pipe_action_datatype_bmap;
  }

  pipe_action_spec_t *getPipeActionSpec() { return &action_spec; }

  const pipe_action_spec_t *getPipeActionSpec() const { return &action_spec; }

  const CounterSpecData &getCounterSpecObj() const { return counter_spec; }
  CounterSpecData &getCounterSpecObj() { return counter_spec; }
  const MeterSpecData &getMeterSpecObj() const { return meter_spec; };
  MeterSpecData &getMeterSpecObj() { return meter_spec; };
  const RegisterSpecData &getRegisterSpecObj() const { return register_spec; }
  RegisterSpecData &getRegisterSpecObj() { return register_spec; }
  const uint32_t &directResCountGet() { return num_direct_resources; };
  const uint32_t &indirectResCountGet() { return num_indirect_resources; };

 private:
  pipe_res_spec_t *getResourceSpec(const pipe_tbl_hdl_t &tbl_hdl);
  const pipe_res_spec_t *getResourceSpec(const pipe_tbl_hdl_t &tbl_hdl) const;

  void updateResourceSpec(const pipe_tbl_hdl_t &tbl_hdl,
                          const bool &&is_direct_resource,
                          pipe_res_spec_t **res_spec);

  CounterSpecData counter_spec;
  MeterSpecData meter_spec;
  RegisterSpecData register_spec;

  pipe_action_spec_t action_spec{0};
  std::unique_ptr<uint8_t[]> action_data_bits{nullptr};
  uint32_t num_direct_resources = 0;
  uint32_t num_indirect_resources = 0;
};

class MatchActionTableData : public tdi::TableData {
 public:
  MatchActionTableData(const tdi::Table *table,
                       tdi_id_t act_id,
                       const std::vector<tdi_id_t> &fields)
      : tdi::TableData(table, act_id, fields) {
    size_t data_sz = 0;
    size_t data_sz_bits = 0;
    if (act_id) {
      data_sz = static_cast<const RtActionContextInfo *>(
                    this->table_->tableInfoGet()
                        ->actionGet(act_id)
                        ->actionContextInfoGet())
                    ->dataSzGet();
      data_sz_bits = static_cast<const RtActionContextInfo *>(
                         this->table_->tableInfoGet()
                             ->actionGet(act_id)
                             ->actionContextInfoGet())
                         ->dataSzBitsGet();
    } else {
      data_sz = static_cast<const RtTableContextInfo *>(
                    this->table_->tableInfoGet()->tableContextInfoGet())
                    ->maxDataSzGet();
      data_sz_bits = static_cast<const RtTableContextInfo *>(
                         this->table_->tableInfoGet()->tableContextInfoGet())
                         ->maxDataSzBitsGet();
    }

    PipeActionSpec *action_spec =
        new PipeActionSpec(data_sz, data_sz_bits, PIPE_ACTION_DATA_TYPE);
    action_spec_wrapper.reset(action_spec);
    this->initializeDataFields();
  }

  virtual ~MatchActionTableData() {}

  MatchActionTableData(const tdi::Table *tbl_obj,
                       const std::vector<tdi_id_t> &fields)
      : MatchActionTableData(tbl_obj, 0, fields) {}

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override;
  tdi_status_t setValue(const tdi_id_t &field_id,
                        const std::string &value) override;
  tdi_status_t setValue(const tdi_id_t &field_id, const float &value) override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        std::string *value) const override;
  tdi_status_t getValue(const tdi_id_t &field_id, float *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        std::vector<uint64_t> *value) const override;

  // This gives out a copy of action spec. This is needed for the indirect Match
  // tables to fill in the action member ID or selector group ID based
  // information in the action spec.
  // Note that for match indirect tables, the copy of action spec is not too
  // expensive since the byte array representing the action parameters does not
  // exist
  virtual void copy_pipe_action_spec(pipe_action_spec_t *act_spec) const {
    std::memcpy(act_spec,
                getPipeActionSpecObj().getPipeActionSpec(),
                sizeof(*act_spec));
    return;
  }

  virtual const pipe_action_spec_t *get_pipe_action_spec() const {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  virtual pipe_action_spec_t *get_pipe_action_spec() {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  const uint32_t &indirectResCountGet() const {
    return num_indirect_resource_count;
  }

  PipeActionSpec &getPipeActionSpecObj() {
    return *(action_spec_wrapper.get());
  }
  const PipeActionSpec &getPipeActionSpecObj() const {
    return *(action_spec_wrapper.get());
  }

  tdi_status_t resetDerived();
  tdi_status_t reset(const tdi_id_t &action_id,
                     const tdi_id_t &/*container_id*/,
                     const std::vector<tdi_id_t> &fields);

  // A public setter for action_spec_wrapper
  void actionSpecSet(PipeActionSpec *action_spec) {
    if (action_spec == nullptr) {
      action_spec_wrapper->resetPipeActionSpec();
    } else {
      action_spec_wrapper.reset(action_spec);
    }
    return;
  }

  const uint32_t &numActionOnlyFieldsGet() const {
    return num_action_only_fields;
  }
  pipe_act_fn_hdl_t getActFnHdl() const;

 protected:
  tdi_id_t action_mbr_id{0};
  tdi_id_t group_id{0};
  std::unique_ptr<PipeActionSpec> action_spec_wrapper;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                                const uint64_t &value,
                                const uint8_t *value_ptr,
                                const size_t &s);

  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                                uint64_t *value,
                                uint8_t *value_ptr,
                                const size_t &size) const;

  void set_action_member_id(uint64_t val);
  tdi_id_t get_action_member_id() const { return action_mbr_id; }
  tdi_id_t get_selector_group_id() const { return group_id; }
  void set_selector_group_id(uint64_t val);
  void initializeDataFields();
  void getIndirectResourceCounts(const std::vector<tdi_id_t> field_list);

  uint32_t num_indirect_resource_count = 0;
  uint32_t num_action_only_fields = 0;
};

class MatchActionIndirectTableData : public MatchActionTableData {
 public:
  MatchActionIndirectTableData(const tdi::Table *tbl_obj,
                               const std::vector<tdi_id_t> &fields)
      : MatchActionTableData(tbl_obj, fields){};

  tdi_id_t getActionMbrId() const {
    return MatchActionTableData::action_mbr_id;
  }

  void setActionMbrId(tdi_id_t act_mbr_id) {
    MatchActionTableData::action_mbr_id = act_mbr_id;
  }

  tdi_id_t getGroupId() const { return MatchActionTableData::group_id; }

  void setGroupId(tdi_id_t sel_grp_id) {
    MatchActionTableData::group_id = sel_grp_id;
  }

  bool isGroup() const {
    return (MatchActionTableData::getPipeActionSpecObj()
                .getPipeActionDatatypeBitmap() == PIPE_SEL_GRP_HDL_TYPE);
  }

  static const tdi_id_t invalid_group = 0xdeadbeef;
  static const tdi_id_t invalid_action_entry_hdl = 0xdeadbeef;
};

#if 0
class ActionTableData : public tdi::TableData {
 public:
  ActionTableData(const tdi::Table *tbl_obj, tdi_id_t act_id)
      : tdi::TableData(tbl_obj, act_id) {
    size_t data_sz = 0;
    size_t data_sz_bits = 0;
    if (act_id) {
      data_sz = getdataSz(act_id);
      data_sz_bits = getdataSzbits(act_id);
    } else {
      data_sz = getMaxdataSz();
      data_sz_bits = getMaxdataSzbits();
    }
    // Prime the pipe mgr action spec structure
    PipeActionSpec *action_spec =
        new PipeActionSpec(data_sz, data_sz_bits, PIPE_ACTION_DATA_TYPE);
    action_spec_wrapper.reset(action_spec);
    this->init(act_id);
  }

  ActionTableData(const tdi::Table *tbl_obj)
      : ActionTableData(tbl_obj, 0) {}

  void init(const tdi_id_t &act_id) { this->actionIdSet(act_id); }

  tdi_status_t reset();

  tdi_status_t reset(const tdi_id_t &action_id);

  virtual ~ActionTableData(){};

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  const pipe_action_spec_t *get_pipe_action_spec() const {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  pipe_action_spec_t *mutable_pipe_action_spec() {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  const std::map<DataFieldType, tdi_id_t> &get_res_map() const {
    return resource_map;
  }

  // A public setter for action_spec_wrapper
  void actionSpecSet(PipeActionSpec *action_spec) {
    if (action_spec == nullptr) {
      action_spec_wrapper->resetPipeActionSpec();
    } else {
      action_spec_wrapper.reset(action_spec);
    }
    return;
  }

  pipe_act_fn_hdl_t getActFnHdl() const;

 private:
  PipeActionSpec &getPipeActionSpecObj() {
    return *(action_spec_wrapper.get());
  }
  const PipeActionSpec &getPipeActionSpecObj() const {
    return *(action_spec_wrapper.get());
  }

  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &s);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &s) const;

  std::unique_ptr<PipeActionSpec> action_spec_wrapper;
  // A map of indirect resource type to the resource index
  // Used to maintain state required for action profile management
  std::map<DataFieldType, tdi_id_t> resource_map;
};

class SelectorTableData : public tdi::TableData {
 public:
  SelectorTableData(const tdi::Table *tbl_obj,
                        const std::vector<tdi_id_t> &fields);

  ~SelectorTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::vector<tdi_id_t> &arr);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const std::vector<bool> &arr);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       std::vector<tdi_id_t> *arr) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       std::vector<bool> *arr) const;

  pipe_act_fn_hdl_t get_pipe_act_fn_hdl() const { return act_fn_hdl_; }
  uint32_t get_max_grp_size() const { return max_grp_size_; }

  const std::vector<uint32_t> &getMembers() const { return members_; }
  const std::vector<bool> &getMemberStatus() const { return member_status_; }

  void setMembers(std::vector<uint32_t> &members) { members_ = members; }
  void setMemberStatus(std::vector<bool> &member_status) {
    member_status_ = member_status;
  }
  void setMaxGrpSize(const uint32_t &max_size) { max_grp_size_ = max_size; }

  tdi_status_t reset() override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               uint64_t *value,
                               uint8_t *value_ptr) const;
  std::vector<tdi_id_t> get_members_from_array(const uint8_t *value,
                                                 const size_t &size);
  pipe_act_fn_hdl_t act_fn_hdl_;
  std::vector<uint32_t> members_;
  std::vector<bool> member_status_;
  uint32_t max_grp_size_{0};
};

class CounterTableData : public tdi::TableData {
 public:
  CounterTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj){};
  ~CounterTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  // Functions not exposed
  const CounterSpecData &getCounterSpecObj() const { return counter_spec_; }
  CounterSpecData &getCounterSpecObj() { return counter_spec_; }

  tdi_status_t reset() override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               uint64_t *value,
                               uint8_t *value_ptr) const;
  CounterSpecData counter_spec_;
};

class MeterTableData : public tdi::TableData {
 public:
  MeterTableData(const tdi::Table *tbl_obj) : tdi::TableData(tbl_obj){};
  ~MeterTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  // Functions not exposed
  const MeterSpecData &getMeterSpecObj() const { return meter_spec_; };
  MeterSpecData &getMeterSpecObj() { return meter_spec_; };

  tdi_status_t reset() override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               uint64_t *value,
                               uint8_t *value_ptr) const;

  MeterSpecData meter_spec_;
};

class RegisterTableData : public tdi::TableData {
 public:
  RegisterTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj){};
  ~RegisterTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id,
                       std::vector<uint64_t> *value) const;

  // Unexposed APIs
  const RegisterSpecData &getRegisterSpecObj() const { return register_spec_; }
  RegisterSpecData &getRegisterSpecObj() { return register_spec_; }

  tdi_status_t reset() override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);

  RegisterSpecData register_spec_;
};

class EmptyTableData : public tdi::TableData {
 public:
  EmptyTableData(const tdi::Table *tbl_obj) : tdi::TableData(tbl_obj){};
  ~EmptyTableData() = default;
};

class SelectorGetMemberTableData : public tdi::TableData {
 public:
  SelectorGetMemberTableData(const tdi::Table *tbl_obj,
                                 tdi_id_t act_id,
                                 const std::vector<tdi_id_t> &fields)
      : tdi::TableData(tbl_obj, act_id) {
    this->actionIdSet(act_id);
    this->setActiveFields(fields);
  }
  SelectorGetMemberTableData(const tdi::Table *tbl_obj,
                                 const std::vector<tdi_id_t> &fields)
      : SelectorGetMemberTableData(tbl_obj, 0, fields) {}
  ~SelectorGetMemberTableData() {}

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint64_t &value) override final;
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       uint64_t *value) const override final;
  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;
  uint32_t act_mbr_id;
};

class RegisterParamTableData : public tdi::TableData {
 public:
  RegisterParamTableData(const tdi::Table *tbl_obj)
      : tdi::TableData(tbl_obj){};
  ~RegisterParamTableData() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value_ptr,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value_int) const;

  tdi_status_t reset() override final;

  // Unpublished
  int64_t value;
};
#endif

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  // _TDI_P4_TABLE_DATA_IMPL_HPP

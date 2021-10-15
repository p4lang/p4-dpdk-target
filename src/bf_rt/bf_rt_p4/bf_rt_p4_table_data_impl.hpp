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

#ifndef _BF_RT_P4_TABLE_DATA_IMPL_HPP
#define _BF_RT_P4_TABLE_DATA_IMPL_HPP

#include <bf_rt_common/bf_rt_table_data_impl.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>

namespace bfrt {

namespace {
void initialize_data_field(const bf_rt_id_t &field_id,
                           const BfRtTableObj &table,
                           const bf_rt_id_t &action_id,
                           BfRtTableDataObj *data_obj) {
  const bfrt::BfRtTableDataField *field_obj;
  bf_status_t status = BF_SUCCESS;

  // No need to initialize mandatory fields, as those will not have
  // default value in JSON.
  status = table.getDataField(field_id, action_id, &field_obj);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s ERROR in getting data field object for field id %d, "
        "action id %d",
        __func__,
        __LINE__,
        table.table_name_get().c_str(),
        field_id,
        action_id);
    BF_RT_DBGCHK(0);
    return;
  }
  if (field_obj->isMandatory()) return;

  uint64_t default_value;
  std::string default_str_value;
  float default_fl_value;
  switch (field_obj->getDataType()) {
    case bfrt::DataType::UINT64:
      default_value = field_obj->defaultValueGet();
      status = data_obj->setValue(field_id, default_value);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.table_name_get().c_str(),
            field_id,
            action_id,
            status);
        BF_RT_DBGCHK(0);
        return;
      }
      break;
    case bfrt::DataType::FLOAT:
      default_fl_value = field_obj->defaultFlValueGet();
      status = data_obj->setValue(field_id, default_fl_value);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.table_name_get().c_str(),
            field_id,
            action_id,
            status);
        BF_RT_DBGCHK(0);
        return;
      }
      break;
    case bfrt::DataType::BOOL:
      default_value = field_obj->defaultValueGet();
      status = data_obj->setValue(field_id, static_cast<bool>(default_value));
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.table_name_get().c_str(),
            field_id,
            action_id,
            status);
        BF_RT_DBGCHK(0);
        return;
      }
      break;
    case bfrt::DataType::STRING:
      default_str_value = field_obj->defaultStrValueGet();
      status = data_obj->setValue(field_id, default_str_value);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in setting default value for field id %d, action "
            "id %d , err %d",
            __func__,
            __LINE__,
            table.table_name_get().c_str(),
            field_id,
            action_id,
            status);
        BF_RT_DBGCHK(0);
        return;
      }
      break;
    default:
      break;
  }
  return;
}
}  // Annonymous namespace

class DataFieldStringMapper {
 public:
  // Map of allowed string values for pipe_lpf_type_e
  static const std::map<std::string, pipe_lpf_type_e>
      string_to_lpf_spec_type_map;
  static const std::map<pipe_lpf_type_e, std::string>
      lpf_spec_type_to_string_map;

  // Map of allowed string values for pipe_idle_time_hit_state_e
  static const std::map<pipe_idle_time_hit_state_e, std::string>
      idle_time_hit_state_to_string_map;

  static bf_status_t lpfSpecTypeFromStringGet(const std::string &val,
                                              pipe_lpf_type_e *lpf_val) {
    const auto kv = string_to_lpf_spec_type_map.find(val);
    if (kv == string_to_lpf_spec_type_map.end()) {
      return BF_INVALID_ARG;
    }
    *lpf_val = kv->second;
    return BF_SUCCESS;
  }

  static bf_status_t lpfStringFromSpecTypeGet(const pipe_lpf_type_e &lpf_val,
                                              std::string *val) {
    const auto kv = lpf_spec_type_to_string_map.find(lpf_val);
    if (kv == lpf_spec_type_to_string_map.end()) {
      return BF_INVALID_ARG;
    }
    *val = kv->second;
    return BF_SUCCESS;
  }

  static bf_status_t idleHitStateToString(const pipe_idle_time_hit_state_e &hs,
                                          std::string *val) {
    const auto kv = idle_time_hit_state_to_string_map.find(hs);
    if (kv == idle_time_hit_state_to_string_map.end()) {
      return BF_INVALID_ARG;
    }
    *val = kv->second;
    return BF_SUCCESS;
  }

  static bf_status_t idleHitStateFromString(const std::string &val,
                                            pipe_idle_time_hit_state_e *hs) {
    auto map = idle_time_hit_state_to_string_map;
    for (auto kv = map.begin(); kv != map.end(); kv++) {
      if (kv->second == val) {
        *hs = kv->first;
        return BF_SUCCESS;
      }
    }
    return BF_INVALID_ARG;
  }
};  // DataFieldStringMapper

// This class is a software representation of how a register data field
// can be programmed in hardware. A hardware 'register' can be of size
// 1, 8, 16, 32 or 64. When in 1 size mode, the register acts in non dual
// mode and thus has only one instance published in the bfrt json. When
// in 8, 16, 32 size mode, it can operate in non dual or dual mode.
// When in 64 size mode, the register always acts in dual mode.
//
// In non dual, there is only one instance published in the json while
// in dual mode there are two instances published in the json (each
// of the 8, 16 or 32 sizes respectively)
class BfRtRegister {
 public:
  bf_status_t setDataFromStfulSpec(const pipe_stful_mem_spec_t &stful_spec);
  bf_status_t getStfulSpecFromData(pipe_stful_mem_spec_t *stful_spec) const;

  bf_status_t setData(const BfRtTableDataField &tableDataField, uint64_t value);
  uint64_t getData(const BfRtTableDataField &tableDataField) const;

 private:
  pipe_stful_mem_spec_t stful = {};
};

class RegisterSpecData {
 public:
  void populateDataFromStfulSpec(
      const std::vector<pipe_stful_mem_spec_t> &stful_spec, uint32_t size);
  void reset() { registers_.clear(); }
  void populateStfulSpecFromData(pipe_stful_mem_spec_t *stful_spec) const;
  void setFirstRegister(const BfRtTableDataField &tableDataField,
                        const uint64_t &value);
  void addRegisterToVec(const BfRtTableDataField &tableDataField,
                        const uint64_t &value);
  const std::vector<BfRtRegister> &getRegisterVec() const { return registers_; }

 private:
  std::vector<BfRtRegister> registers_;
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
class LPFSpecData {
 public:
  LPFSpecData() { pipe_lpf_spec = {}; };
  void reset() { pipe_lpf_spec = {}; }

  void setGainTimeConstant(float value) {
    pipe_lpf_spec.gain_time_constant = value;
    if (pipe_lpf_spec.gain_time_constant != pipe_lpf_spec.decay_time_constant) {
      pipe_lpf_spec.gain_decay_separate_time_constant = true;
    } else {
      pipe_lpf_spec.gain_decay_separate_time_constant = false;
      pipe_lpf_spec.time_constant = value;
    }
  }
  void setDecayTimeConstant(float value) {
    pipe_lpf_spec.decay_time_constant = value;
    if (pipe_lpf_spec.gain_time_constant != pipe_lpf_spec.decay_time_constant) {
      pipe_lpf_spec.gain_decay_separate_time_constant = true;
    } else {
      pipe_lpf_spec.gain_decay_separate_time_constant = false;
      pipe_lpf_spec.time_constant = value;
    }
  }
  void setRateEnable(bool enable) {
    pipe_lpf_spec.lpf_type = enable ? LPF_TYPE_RATE : LPF_TYPE_SAMPLE;
  }
  void setOutputScaleDownFactor(uint32_t value) {
    pipe_lpf_spec.output_scale_down_factor = value;
  }

  const pipe_lpf_spec_t *getPipeLPFSpec() const { return &pipe_lpf_spec; }
  pipe_lpf_spec_t *getPipeLPFSpec() { return &pipe_lpf_spec; }

  template <class T>
  bf_status_t setLPFDataFromValue(const DataFieldType &type, const T &value);
  void setLPFDataFromLPFSpec(const pipe_lpf_spec_t *lpf_spec);

  template <class T>
  bf_status_t getLPFDataFromValue(const DataFieldType &type, T *value) const;

 private:
  pipe_lpf_spec_t pipe_lpf_spec;
};

class WREDSpecData {
 public:
  WREDSpecData() { pipe_wred_spec = {}; };
  void reset() { pipe_wred_spec = {}; }
  void setTimeConstant(float value) { pipe_wred_spec.time_constant = value; }
  void setMinThreshold(uint32_t value) {
    pipe_wred_spec.red_min_threshold = value;
  }
  void setMaxThreshold(uint32_t value) {
    pipe_wred_spec.red_max_threshold = value;
  }
  void setMaxProbability(float value) {
    pipe_wred_spec.max_probability = value;
  }

  const pipe_wred_spec_t *getPipeWREDSpec() const { return &pipe_wred_spec; };
  pipe_wred_spec_t *getPipeWREDSpec() { return &pipe_wred_spec; };
  template <class T>
  bf_status_t setWREDDataFromValue(const DataFieldType &type, const T &value);
  void setWREDDataFromWREDSpec(const pipe_wred_spec_t *wred_spec);

  template <class T>
  bf_status_t getWREDDataFromValue(const DataFieldType &type, T *value) const;

 private:
  pipe_wred_spec_t pipe_wred_spec;
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
    lpf_spec.reset();
    wred_spec.reset();
    register_spec.reset();
    num_direct_resources = 0;
    num_indirect_resources = 0;
    action_spec.pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
  }

  ~PipeActionSpec() {}

  // Action Param
  bf_status_t setValueActionParam(const BfRtTableDataField &field,
                                  const uint64_t &value,
                                  const uint8_t *value_ptr);
  bf_status_t getValueActionParam(const BfRtTableDataField &field,
                                  uint64_t *value,
                                  uint8_t *value_ptr) const;

  // Resource Index
  bf_status_t setValueResourceIndex(const BfRtTableDataField &field,
                                    const pipe_tbl_hdl_t &tbl_hdl,
                                    const uint64_t &value,
                                    const uint8_t *value_ptr);
  bf_status_t getValueResourceIndex(const BfRtTableDataField &field,
                                    const pipe_tbl_hdl_t &tbl_hdl,
                                    uint64_t *value,
                                    uint8_t *value_ptr) const;

  // Counter
  bf_status_t setValueCounterSpec(const pipe_stat_data_t &counter);
  bf_status_t setValueCounterSpec(const BfRtTableDataField &field,
                                  const DataFieldType &field_type,
                                  const pipe_tbl_hdl_t &tbl_hdl,
                                  const uint64_t &value,
                                  const uint8_t *value_ptr);
  bf_status_t getValueCounterSpec(const BfRtTableDataField &field,
                                  const DataFieldType &field_type,
                                  uint64_t *value,
                                  uint8_t *value_ptr) const;

  // Register
  bf_status_t setValueRegisterSpec(
      const std::vector<pipe_stful_mem_spec_t> &register_data);
  bf_status_t setValueRegisterSpec(const BfRtTableDataField &field,
                                   const DataFieldType &field_type,
                                   const pipe_tbl_hdl_t &tbl_hdl,
                                   const uint64_t &value,
                                   const uint8_t *value_ptr,
                                   const size_t &field_size);
  bf_status_t getValueRegisterSpec(const BfRtTableDataField &field,
                                   std::vector<uint64_t> *value) const;

  // Meter
  bf_status_t setValueMeterSpec(const pipe_meter_spec_t &meter);
  bf_status_t setValueMeterSpec(const BfRtTableDataField &field,
                                const DataFieldType &field_type,
                                const pipe_tbl_hdl_t &tbl_hdl,
                                const uint64_t &value,
                                const uint8_t *value_ptr);
  bf_status_t getValueMeterSpec(const BfRtTableDataField &field,
                                const DataFieldType &field_type,
                                uint64_t *value,
                                uint8_t *value_ptr) const;

  // LPF
  bf_status_t setValueLPFSpec(const pipe_lpf_spec_t &lpf);
  bf_status_t setValueLPFSpec(const BfRtTableDataField &field,
                              const DataFieldType &field_type,
                              const pipe_tbl_hdl_t &tbl_hdl,
                              const uint64_t &value,
                              const uint8_t *value_ptr);
  template <typename T>
  bf_status_t setValueLPFSpec(const DataFieldType &field_type,
                              const pipe_tbl_hdl_t &tbl_hdl,
                              const T &value);
  bf_status_t getValueLPFSpec(const BfRtTableDataField &field,
                              const DataFieldType &field_type,
                              uint64_t *value,
                              uint8_t *value_ptr) const;
  template <typename T>
  bf_status_t getValueLPFSpec(const DataFieldType &field_type, T *value) const;

  // WRED
  bf_status_t setValueWREDSpec(const pipe_wred_spec_t &wred);
  bf_status_t setValueWREDSpec(const BfRtTableDataField &field,
                               const DataFieldType &field_type,
                               const pipe_tbl_hdl_t &tbl_hdl,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  template <typename T>
  bf_status_t setValueWREDSpec(const DataFieldType &field_type,
                               const pipe_tbl_hdl_t &tbl_hdl,
                               const T &value);
  bf_status_t getValueWREDSpec(const BfRtTableDataField &field,
                               const DataFieldType &field_type,
                               uint64_t *value,
                               uint8_t *value_ptr) const;
  template <typename T>
  bf_status_t getValueWREDSpec(const DataFieldType &field_type, T *value) const;

  // Action member
  bf_status_t setValueActionDataHdlType();

  // Selector Group
  bf_status_t setValueSelectorGroupHdlType();

  uint8_t getPipeActionDatatypeBitmap() const {
    return action_spec.pipe_action_datatype_bmap;
  }

  pipe_action_spec_t *getPipeActionSpec() { return &action_spec; }

  const pipe_action_spec_t *getPipeActionSpec() const { return &action_spec; }

  const CounterSpecData &getCounterSpecObj() const { return counter_spec; }
  CounterSpecData &getCounterSpecObj() { return counter_spec; }
  const MeterSpecData &getMeterSpecObj() const { return meter_spec; };
  MeterSpecData &getMeterSpecObj() { return meter_spec; };
  const LPFSpecData &getLPFSpecObj() const { return lpf_spec; };
  LPFSpecData &getLPFSpecObj() { return lpf_spec; };
  const WREDSpecData &getWREDSpecObj() const { return wred_spec; };
  WREDSpecData &getWREDSpecObj() { return wred_spec; };
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

  template <typename T>
  bf_status_t setValueLPFSpecHelper(const DataFieldType &field_type,
                                    const pipe_tbl_hdl_t &tbl_hdl,
                                    const T &value);
  template <typename T>
  bf_status_t setValueWREDSpecHelper(const DataFieldType &field_type,
                                     const pipe_tbl_hdl_t &tbl_hdl,
                                     const T &value);

  CounterSpecData counter_spec;
  MeterSpecData meter_spec;
  LPFSpecData lpf_spec;
  WREDSpecData wred_spec;
  RegisterSpecData register_spec;

  pipe_action_spec_t action_spec{0};
  std::unique_ptr<uint8_t[]> action_data_bits{nullptr};
  uint32_t num_direct_resources = 0;
  uint32_t num_indirect_resources = 0;
};

class BfRtMatchActionTableData : public BfRtTableDataObj {
 public:
  BfRtMatchActionTableData(const BfRtTableObj *tbl_obj,
                           bf_rt_id_t act_id,
                           const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj, act_id) {
    size_t data_sz = 0;
    size_t data_sz_bits = 0;
    if (act_id) {
      data_sz = getdataSz(act_id);
      data_sz_bits = getdataSzbits(act_id);
    } else {
      data_sz = getMaxdataSz();
      data_sz_bits = getMaxdataSzbits();
    }

    PipeActionSpec *action_spec =
        new PipeActionSpec(data_sz, data_sz_bits, PIPE_ACTION_DATA_TYPE);
    action_spec_wrapper.reset(action_spec);
    this->init(act_id, fields);
  }

  virtual ~BfRtMatchActionTableData() {}

  BfRtMatchActionTableData(const BfRtTableObj *tbl_obj,
                           const std::vector<bf_rt_id_t> &fields)
      : BfRtMatchActionTableData(tbl_obj, 0, fields) {}

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &value) override;
  bf_status_t setValue(const bf_rt_id_t &field_id, const float &value) override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *value) const override;
  bf_status_t getValue(const bf_rt_id_t &field_id, float *value) const override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
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

  uint32_t get_ttl() const { return ttl; }

  pipe_idle_time_hit_state_e get_entry_hit_state() const { return hit_state; }

  void set_ttl_from_read(uint32_t value) { ttl = value; }

  void set_entry_hit_state(pipe_idle_time_hit_state_e state) {
    hit_state = state;
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

  bf_status_t reset() override final;
  bf_status_t reset(const bf_rt_id_t &action_id,
                    const std::vector<bf_rt_id_t> &fields) override final;

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
  bf_rt_id_t action_mbr_id{0};
  bf_rt_id_t group_id{0};
  std::unique_ptr<PipeActionSpec> action_spec_wrapper;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &s);

  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;

  void set_action_member_id(uint64_t val);
  bf_rt_id_t get_action_member_id() const { return action_mbr_id; }
  bf_rt_id_t get_selector_group_id() const { return group_id; }
  void set_selector_group_id(uint64_t val);
  void set_ttl(uint64_t val);
  void init(const bf_rt_id_t &act_id, const std::vector<bf_rt_id_t> &fields) {
    this->actionIdSet(act_id);
    this->ttl = 0;
    this->hit_state = ENTRY_IDLE;
    this->setActiveFields(fields);
    this->initializeDataFields();
  }
  void initializeDataFields();
  void getIndirectResourceCounts(const std::vector<bf_rt_id_t> field_list);

  uint32_t ttl = 0;
  pipe_idle_time_hit_state_e hit_state = ENTRY_IDLE;
  uint32_t num_indirect_resource_count = 0;
  uint32_t num_action_only_fields = 0;
};

class BfRtMatchActionIndirectTableData : public BfRtMatchActionTableData {
 public:
  BfRtMatchActionIndirectTableData(const BfRtTableObj *tbl_obj,
                                   const std::vector<bf_rt_id_t> &fields)
      : BfRtMatchActionTableData(tbl_obj, fields){};

  bf_rt_id_t getActionMbrId() const {
    return BfRtMatchActionTableData::action_mbr_id;
  }

  void setActionMbrId(bf_rt_id_t act_mbr_id) {
    BfRtMatchActionTableData::action_mbr_id = act_mbr_id;
  }

  bf_rt_id_t getGroupId() const { return BfRtMatchActionTableData::group_id; }

  void setGroupId(bf_rt_id_t sel_grp_id) {
    BfRtMatchActionTableData::group_id = sel_grp_id;
  }

  bool isGroup() const {
    return (BfRtMatchActionTableData::getPipeActionSpecObj()
                .getPipeActionDatatypeBitmap() == PIPE_SEL_GRP_HDL_TYPE);
  }

  static const bf_rt_id_t invalid_group = 0xdeadbeef;
  static const bf_rt_id_t invalid_action_entry_hdl = 0xdeadbeef;
};

class BfRtActionTableData : public BfRtTableDataObj {
 public:
  BfRtActionTableData(const BfRtTableObj *tbl_obj, bf_rt_id_t act_id)
      : BfRtTableDataObj(tbl_obj, act_id) {
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

  BfRtActionTableData(const BfRtTableObj *tbl_obj)
      : BfRtActionTableData(tbl_obj, 0) {}

  void init(const bf_rt_id_t &act_id) { this->actionIdSet(act_id); }

  bf_status_t reset();

  bf_status_t reset(const bf_rt_id_t &action_id);

  virtual ~BfRtActionTableData(){};

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  const pipe_action_spec_t *get_pipe_action_spec() const {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  pipe_action_spec_t *mutable_pipe_action_spec() {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  const std::map<DataFieldType, bf_rt_id_t> &get_res_map() const {
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

  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &s);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &s) const;

  std::unique_ptr<PipeActionSpec> action_spec_wrapper;
  // A map of indirect resource type to the resource index
  // Used to maintain state required for action profile management
  std::map<DataFieldType, bf_rt_id_t> resource_map;
};

class BfRtSelectorTableData : public BfRtTableDataObj {
 public:
  BfRtSelectorTableData(const BfRtTableObj *tbl_obj,
                        const std::vector<bf_rt_id_t> &fields);

  ~BfRtSelectorTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<bf_rt_id_t> &arr);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<bool> &arr);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bf_rt_id_t> *arr) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bool> *arr) const;

  pipe_act_fn_hdl_t get_pipe_act_fn_hdl() const { return act_fn_hdl_; }
  uint16_t get_max_grp_size() const { return max_grp_size_; }

  const std::vector<uint32_t> &getMembers() const { return members_; }
  const std::vector<bool> &getMemberStatus() const { return member_status_; }

  void setMembers(std::vector<uint32_t> &members) { members_ = members; }
  void setMemberStatus(std::vector<bool> &member_status) {
    member_status_ = member_status;
  }
  void setMaxGrpSize(const uint32_t &max_size) { max_grp_size_ = max_size; }

  bf_status_t reset() override final;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               uint64_t *value,
                               uint8_t *value_ptr) const;
  std::vector<bf_rt_id_t> get_members_from_array(const uint8_t *value,
                                                 const size_t &size);
  pipe_act_fn_hdl_t act_fn_hdl_;
  std::vector<uint32_t> members_;
  std::vector<bool> member_status_;
  uint32_t max_grp_size_{0};
};

class BfRtCounterTableData : public BfRtTableDataObj {
 public:
  BfRtCounterTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};
  ~BfRtCounterTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  // Functions not exposed
  const CounterSpecData &getCounterSpecObj() const { return counter_spec_; }
  CounterSpecData &getCounterSpecObj() { return counter_spec_; }

  bf_status_t reset() override final;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               uint64_t *value,
                               uint8_t *value_ptr) const;
  CounterSpecData counter_spec_;
};

class BfRtMeterTableData : public BfRtTableDataObj {
 public:
  BfRtMeterTableData(const BfRtTableObj *tbl_obj) : BfRtTableDataObj(tbl_obj){};
  ~BfRtMeterTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  // Functions not exposed
  const MeterSpecData &getMeterSpecObj() const { return meter_spec_; };
  MeterSpecData &getMeterSpecObj() { return meter_spec_; };

  bf_status_t reset() override final;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               uint64_t *value,
                               uint8_t *value_ptr) const;

  MeterSpecData meter_spec_;
};

class BfRtLPFTableData : public BfRtTableDataObj {
 public:
  BfRtLPFTableData(const BfRtTableObj *tbl_obj) : BfRtTableDataObj(tbl_obj){};
  ~BfRtLPFTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override;

  bf_status_t setValue(const bf_rt_id_t &field_id, const float &value) override;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &value) override;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override;

  bf_status_t getValue(const bf_rt_id_t &field_id, float *value) const override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *value) const override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override;

  // Functions not exposed
  const LPFSpecData &getLPFSpecObj() const { return lpf_spec_; };
  LPFSpecData &getLPFSpecObj() { return lpf_spec_; };

  bf_status_t reset() override final;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               uint64_t *value,
                               uint8_t *value_ptr) const;
  LPFSpecData lpf_spec_;
};

class BfRtWREDTableData : public BfRtTableDataObj {
 public:
  BfRtWREDTableData(const BfRtTableObj *tbl_obj) : BfRtTableDataObj(tbl_obj){};
  ~BfRtWREDTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id, const float &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id, float *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;
  // Functions not exposed
  const WREDSpecData &getWREDSpecObj() const { return wred_spec_; };
  WREDSpecData &getWREDSpecObj() { return wred_spec_; };

  bf_status_t reset() override final;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               uint64_t *value,
                               uint8_t *value_ptr) const;
  uint64_t wred = 0;
  WREDSpecData wred_spec_;
};

class BfRtRegisterTableData : public BfRtTableDataObj {
 public:
  BfRtRegisterTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};
  ~BfRtRegisterTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<uint64_t> *value) const;

  // Unexposed APIs
  const RegisterSpecData &getRegisterSpecObj() const { return register_spec_; }
  RegisterSpecData &getRegisterSpecObj() { return register_spec_; }

  bf_status_t reset() override final;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);

  RegisterSpecData register_spec_;
};

class BfRtEmptyTableData : public BfRtTableDataObj {
 public:
  BfRtEmptyTableData(const BfRtTableObj *tbl_obj) : BfRtTableDataObj(tbl_obj){};
  ~BfRtEmptyTableData() = default;
};

class BfRtPhase0TableData : public BfRtTableDataObj {
 public:
  BfRtPhase0TableData(const BfRtTableObj *tbl_obj, const bf_rt_id_t &act_id)
      : BfRtTableDataObj(tbl_obj) {
    this->actionIdSet(act_id);
    // Prime the pipe mgr action spec structure
    size_t data_sz = getdataSz(act_id);
    size_t data_sz_bits = getdataSzbits(act_id);
    PipeActionSpec *action_spec =
        new PipeActionSpec(data_sz, data_sz_bits, PIPE_ACTION_DATA_TYPE);
    action_spec_wrapper.reset(action_spec);
  }
  ~BfRtPhase0TableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  // Unexposed APIs
  const pipe_action_spec_t *get_pipe_action_spec() const {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  pipe_action_spec_t *get_pipe_action_spec() {
    return getPipeActionSpecObj().getPipeActionSpec();
  }

  bf_status_t reset(const bf_rt_id_t &act_id) override final;

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

  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;

  std::unique_ptr<PipeActionSpec> action_spec_wrapper;
};

class DynHashField {
 public:
  DynHashField(std::string hash_field_name,
               int start_bit,
               int length,
               int order,
               int field_width)
      : hash_field_name_(hash_field_name),
        start_bit_(start_bit),
        length_(length),
        order_(order),
        field_width_(field_width){};
  DynHashField() : DynHashField("", -1, -1, -1, -1){};
  static bool isEqual(const DynHashField &lhs, const DynHashField &rhs) {
    return (lhs.hash_field_name_ == rhs.hash_field_name_ &&
            lhs.start_bit_ == rhs.start_bit_ && lhs.length_ == rhs.length_);
  }
  static bool dynHashCompareOverlap(const DynHashField &lhs,
                                    const DynHashField &rhs) {
    // overlap condition 1: if rhs start_bit is inside lhs
    if (rhs.hash_field_name_ == lhs.hash_field_name_ &&
        rhs.start_bit_ < lhs.start_bit_ + lhs.length_ &&
        rhs.start_bit_ >= lhs.start_bit_)
      return true;
    // overlap condition 2:  if lhs start_bit_ is inside rhs
    if (lhs.hash_field_name_ == rhs.hash_field_name_ &&
        lhs.start_bit_ < rhs.start_bit_ + rhs.length_ &&
        lhs.start_bit_ >= rhs.start_bit_)
      return true;
    return false;
  }
  static bool dynHashCompareSymmetricOrderOverlap(const DynHashField &lhs,
                                                  const DynHashField &rhs) {
    if (lhs.order_ == rhs.order_ && (lhs.length_ != rhs.length_)) return true;
    return false;
  }
  const int &orderGet() const { return order_; }
  void hashFieldNameSet(const std::string &name) {
    if (name == "") return;
    this->hash_field_name_ = name;
  };
  bf_status_t preProcess(const uint64_t &default_length);

  friend std::ostream &operator<<(std::ostream &os, const DynHashField &dt);
  friend class BfRtDynHashCfgTableData;

 private:
  std::string hash_field_name_;
  int start_bit_ = -1;
  int length_ = -1;
  int order_ = -1;
  int field_width_ = -1;
};

class DynHashFieldSliceList {
 public:
  bf_status_t addSlice(const DynHashField &lhs);
  bf_status_t removeSlice(const DynHashField &lhs);

  void clear();
  void sort();
  const std::vector<DynHashField> &listGet() const { return this->list_; };

 private:
  std::vector<DynHashField> list_;
  // Count of each order
  std::unordered_map<int, int> order_count_;
};

class BfRtDynHashCfgTableData : public BfRtTableDataObj {
 public:
  BfRtDynHashCfgTableData(const BfRtTableObj *tbl_obj,
                          bf_rt_id_t container_id,
                          const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj, container_id, 0 /*act_id*/) {
    this->containerIdSet(container_id);
    this->setActiveFields(fields);
  }
  BfRtDynHashCfgTableData(const BfRtTableObj *tbl_obj,
                          const std::vector<bf_rt_id_t> &fields)
      : BfRtDynHashCfgTableData(tbl_obj, 0, fields) {}
  ~BfRtDynHashCfgTableData() {}

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;
  bf_status_t setValue(
      const bf_rt_id_t &field_id,
      std::vector<std::unique_ptr<BfRtTableData>> container_v) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;
  bf_status_t getValue(
      const bf_rt_id_t &field_id,
      std::vector<BfRtTableData *> *container_v) const override final;

  // unexposed funcs
  void setDynHashFieldName(const std::string &name) {
    this->dyn_hash_field_.hashFieldNameSet(name);
  }
  const DynHashField &dynHashFieldGet() { return this->dyn_hash_field_; }
  bf_status_t attrListGet(
      std::vector<pipe_hash_calc_input_field_attribute_t> *attr_list) const;

 private:
  // These internal functions are only for leaf data objects
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;
  // Top object = data object containing info
  // for all hash fields
  // leaf object = data object containing info of only one container
  // i.e, one container object

  // This contains list of all slices. This is constructed when
  // called setValue with the vec of Data objects. Only applicable
  // for top data object
  DynHashFieldSliceList slice_list_;
  // This contains info about a single slice. Applicable only to
  // leaf data objects
  DynHashField dyn_hash_field_;
  // This map of lists is applicable for top data object. This keeps
  // ownership of leaf data objects. During get, driver creates these
  // data objects and moves into this vec. During set, driver takes
  // over ownership of the passed-in leaf data objects and keeps them
  // here.
  std::map<bf_rt_id_t, std::vector<std::unique_ptr<BfRtTableData>>>
      con_obj_map_;
  // This map contains hash field name to field ID (container fieldID)
  std::map<std::string, bf_rt_id_t> name_id_map_;
};

class BfRtDynHashAlgoTableData : public BfRtTableDataObj {
 public:
  BfRtDynHashAlgoTableData(const BfRtTableObj *tbl_obj,
                           bf_rt_id_t act_id,
                           const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj, act_id) {
    this->actionIdSet(act_id);
    this->setActiveFields(fields);
  }
  BfRtDynHashAlgoTableData(const BfRtTableObj *tbl_obj,
                           const std::vector<bf_rt_id_t> &fields)
      : BfRtDynHashAlgoTableData(tbl_obj, 0, fields) {}
  ~BfRtDynHashAlgoTableData() {}

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &value) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const bool &value) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       bool *value) const override final;

 private:
  struct user_defined_s {
    bool reverse = false;
    uint64_t polynomial = 0;
    uint64_t init = 0;
    uint64_t final_xor = 0;
    uint64_t hash_bit_width = 0;
  };

  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;
  std::string algo_crc_name = "";
  user_defined_s user;
  bool msb = false;
  bool extend = false;
  uint64_t seed = 0;
};

class BfRtDynHashComputeTableData : public BfRtTableDataObj {
 public:
  BfRtDynHashComputeTableData(const BfRtTableObj *tbl_obj,
                              bf_rt_id_t act_id,
                              const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj, act_id) {
    this->actionIdSet(act_id);
    this->setActiveFields(fields);
  }
  BfRtDynHashComputeTableData(const BfRtTableObj *tbl_obj,
                              const std::vector<bf_rt_id_t> &fields)
      : BfRtDynHashComputeTableData(tbl_obj, 0, fields) {}
  ~BfRtDynHashComputeTableData() {}

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;
  std::vector<uint8_t> hash_value;
};

class BfRtSelectorGetMemberTableData : public BfRtTableDataObj {
 public:
  BfRtSelectorGetMemberTableData(const BfRtTableObj *tbl_obj,
                                 bf_rt_id_t act_id,
                                 const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj, act_id) {
    this->actionIdSet(act_id);
    this->setActiveFields(fields);
  }
  BfRtSelectorGetMemberTableData(const BfRtTableObj *tbl_obj,
                                 const std::vector<bf_rt_id_t> &fields)
      : BfRtSelectorGetMemberTableData(tbl_obj, 0, fields) {}
  ~BfRtSelectorGetMemberTableData() {}

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;
  uint32_t act_mbr_id;
};

class BfRtSnapshotConfigTableData : public BfRtTableDataObj {
 public:
  BfRtSnapshotConfigTableData(const BfRtTableObj *tbl_obj,
                              const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj) {
    std::vector<bf_rt_id_t> field_list = fields;
    if (field_list.empty()) {
      tbl_obj->dataFieldIdListGet(&field_list);
    }
    for (auto it = field_list.begin(); it != field_list.end(); it++) {
      initialize_data_field(*it, *table_, 0, this);
    }
    this->setActiveFields(fields);
  }

  ~BfRtSnapshotConfigTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value_int);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value_ptr,
                       const size_t &size);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str_value);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<bf_rt_id_t> &arr);
  bf_status_t setValue(const bf_rt_id_t &field_id, const bool &value_int);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value_int) const;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value_int) const;
  bf_status_t getValue(const bf_rt_id_t &field_id, bool *value_bool) const;
  bf_status_t getValue(const bf_rt_id_t &field_id, std::string *str) const;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bf_rt_id_t> *value) const;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value_int,
                               const uint8_t *value_ptr,
                               const size_t &size,
                               const std::string &str_value);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value_int,
                               uint8_t *value_ptr,
                               const size_t &size,
                               std::string *str_value) const;
  std::map<bf_rt_id_t, uint32_t> uint32_fields;
  std::map<bf_rt_id_t, std::string> str_fields;
  std::map<bf_rt_id_t, std::vector<uint32_t>> capture_pipes;
};

class BfRtSnapshotTriggerTableData : public BfRtTableDataObj {
 public:
  BfRtSnapshotTriggerTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};
  ~BfRtSnapshotTriggerTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value_int);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value_ptr,
                       const size_t &size);
  bf_status_t setValue(const bf_rt_id_t &field_id, const bool &value_bool);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str_value);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<std::string> &arr);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value_int) const;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value_int) const;
  bf_status_t getValue(const bf_rt_id_t &field_id, bool *value_bool) const;
  bf_status_t getValue(const bf_rt_id_t &field_id, std::string *str) const;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<std::string> *arr) const;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value_int,
                               const uint8_t *value_ptr,
                               const size_t &size,
                               const std::string &str_value);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value_int,
                               uint8_t *value_ptr,
                               const size_t &size,
                               std::string *str_value) const;

  std::map<bf_rt_id_t, std::vector<uint8_t>> bs_fields;
  std::map<bf_rt_id_t, uint32_t> uint32_fields;
  std::map<bf_rt_id_t, std::string> str_fields;
  std::map<bf_rt_id_t, std::vector<std::string>> str_arr_fields;
};

class BfRtSnapshotDataTableData : public BfRtTableDataObj {
 public:
  BfRtSnapshotDataTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj), container_valid(), field_type() {
    bfrt_table_obj = tbl_obj;
  }
  ~BfRtSnapshotDataTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value_int);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value_ptr,
                       const size_t &size);
  bf_status_t setValue(const bf_rt_id_t &field_id, const bool &value_bool);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str_value);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<std::string> &arr);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       std::unique_ptr<BfRtSnapshotDataTableData> tableDataObj);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value_int) const;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value_int) const;
  bf_status_t getValue(const bf_rt_id_t &field_id, bool *value_bool) const;
  bf_status_t getValue(const bf_rt_id_t &field_id, std::string *str) const;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<std::string> *arr) const;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<BfRtTableData *> *ret_vec) const;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value_int,
                               const uint8_t *value_ptr,
                               const size_t &size,
                               const std::string &str_value);

  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value_int,
                               uint8_t *value_ptr,
                               const size_t &size,
                               std::string *str_value) const;

  // Container fields
  void setContainerValid(bool v) { this->container_valid = v; };
  bool isContainerValid() const { return container_valid; };
  bf_status_t setContainer(
      std::unique_ptr<BfRtSnapshotDataTableData> tableDataObj);
  bool container_valid;
  std::vector<std::unique_ptr<BfRtSnapshotDataTableData>> container_items;
  std::map<bf_rt_id_t, std::unique_ptr<BfRtSnapshotDataTableData>> c_fields;

  // Field type
  DataFieldType getDataFieldType() const { return this->field_type; };
  void setDataFieldType(DataFieldType d) { this->field_type = d; };
  DataFieldType field_type;
  const BfRtTableObj *bfrt_table_obj;

  // Standard fields
  std::map<bf_rt_id_t, std::vector<uint8_t>> bs_fields;
  std::map<bf_rt_id_t, uint32_t> uint32_fields;
  std::map<bf_rt_id_t, std::string> str_fields;
  std::map<bf_rt_id_t, std::vector<std::string>> str_arr_fields;
};

class BfRtSnapshotLivenessTableData : public BfRtTableDataObj {
 public:
  BfRtSnapshotLivenessTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};
  ~BfRtSnapshotLivenessTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bf_rt_id_t> *value) const;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const size_t &size,
                               const uint64_t &value,
                               const uint8_t *value_ptr);
  std::vector<uint32_t> stages;
};

class BfRtSnapshotPhvTableData : public BfRtTableDataObj {
 public:
  BfRtSnapshotPhvTableData(const BfRtTableObj *tbl_obj,
                           const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj) {
    this->setActiveFields(fields);
  };
  BfRtSnapshotPhvTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};

  ~BfRtSnapshotPhvTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_status_t reset() override final;
  bf_status_t reset(const std::vector<bf_rt_id_t> &fields) override final;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;
  std::map<bf_rt_id_t, uint32_t> fields_;
};

class BfRtTblDbgCntTableData : public BfRtTableDataObj {
 public:
  BfRtTblDbgCntTableData(const BfRtTableObj *tbl_obj,
                         const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj) {
    std::vector<bf_rt_id_t> field_list = fields;
    if (field_list.empty()) {
      tbl_obj->dataFieldIdListGet(&field_list);
    }
    for (auto it = field_list.begin(); it != field_list.end(); it++) {
      initialize_data_field(*it, *table_, 0, this);
    }
    this->setActiveFields(fields);
  };
  BfRtTblDbgCntTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};

  ~BfRtTblDbgCntTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str_value);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id, std::string *str) const;

  bf_status_t reset() override final;
  bf_status_t reset(const std::vector<bf_rt_id_t> &fields) override final;

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value_int,
                               const uint8_t *value_ptr,
                               const size_t &size,
                               const std::string &str_value);

  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value_int,
                               uint8_t *value_ptr,
                               const size_t &size,
                               std::string *str_value) const;

  std::map<bf_rt_id_t, uint64_t> fields_;
  std::map<bf_rt_id_t, std::string> str_fields_;
};

class BfRtRegisterParamTableData : public BfRtTableDataObj {
 public:
  BfRtRegisterParamTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};
  ~BfRtRegisterParamTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value_ptr,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value_int) const;

  bf_status_t reset() override final;

  // Unpublished
  int64_t value;
};

}  // bfrt

#endif  // _BF_RT_P4_TABLE_DATA_IMPL_HPP

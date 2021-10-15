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

#ifndef _BF_RT_TABLE_DATA_IMPL_HPP
#define _BF_RT_TABLE_DATA_IMPL_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

#include <bf_rt/bf_rt_session.hpp>
#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>

namespace bfrt {

// forward declarations
class BfRtLearn;
class BfRtTableObj;

enum DataFieldType {
  INVALID,
  ACTION_PARAM,
  ACTION_PARAM_OPTIMIZED_OUT,
  COUNTER_INDEX,
  METER_INDEX,
  REGISTER_INDEX,
  LPF_INDEX,
  WRED_INDEX,
  COUNTER_SPEC_BYTES,
  COUNTER_SPEC_PACKETS,
  METER_SPEC_CIR_PPS,
  METER_SPEC_PIR_PPS,
  METER_SPEC_CBS_PKTS,
  METER_SPEC_PBS_PKTS,
  METER_SPEC_CIR_KBPS,
  METER_SPEC_PIR_KBPS,
  METER_SPEC_CBS_KBITS,
  METER_SPEC_PBS_KBITS,
  ACTION_MEMBER_ID,
  SELECTOR_GROUP_ID,
  SELECTOR_MEMBERS,
  ACTION_MEMBER_STATUS,
  MAX_GROUP_SIZE,
  TTL,
  ENTRY_HIT_STATE,
  LPF_SPEC_GAIN_TIME_CONSTANT,
  LPF_SPEC_DECAY_TIME_CONSTANT,
  LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR,
  LPF_SPEC_TYPE,
  WRED_SPEC_TIME_CONSTANT,
  WRED_SPEC_MIN_THRESHOLD,
  WRED_SPEC_MAX_THRESHOLD,
  WRED_SPEC_MAX_PROBABILITY,
  REGISTER_SPEC_HI,
  REGISTER_SPEC_LO,
  REGISTER_SPEC,
  SNAPSHOT_ENABLE,
  SNAPSHOT_TIMER_ENABLE,
  SNAPSHOT_TIMER_VALUE_USECS,
  SNAPSHOT_STAGE_ID,
  SNAPSHOT_PREV_STAGE_TRIGGER,
  SNAPSHOT_TIMER_TRIGGER,
  SNAPSHOT_LOCAL_STAGE_TRIGGER,
  SNAPSHOT_NEXT_TABLE_NAME,
  SNAPSHOT_ENABLED_NEXT_TABLES,
  SNAPSHOT_TABLE_ID,
  SNAPSHOT_TABLE_NAME,
  SNAPSHOT_MATCH_HIT_ADDRESS,
  SNAPSHOT_MATCH_HIT_HANDLE,
  SNAPSHOT_TABLE_HIT,
  SNAPSHOT_TABLE_INHIBITED,
  SNAPSHOT_TABLE_EXECUTED,
  SNAPSHOT_FIELD_INFO,
  SNAPSHOT_CONTROL_INFO,
  SNAPSHOT_METER_ALU_INFO,
  SNAPSHOT_METER_ALU_OPERATION_TYPE,
  SNAPSHOT_TABLE_INFO,
  SNAPSHOT_LIVENESS_VALID_STAGES,
  SNAPSHOT_GBL_EXECUTE_TABLES,
  SNAPSHOT_ENABLED_GBL_EXECUTE_TABLES,
  SNAPSHOT_LONG_BRANCH_TABLES,
  SNAPSHOT_ENABLED_LONG_BRANCH_TABLES,
  MULTICAST_NODE_ID,
  MULTICAST_NODE_L1_XID_VALID,
  MULTICAST_NODE_L1_XID,
  MULTICAST_ECMP_ID,
  MULTICAST_ECMP_L1_XID_VALID,
  MULTICAST_ECMP_L1_XID,
  MULTICAST_RID,
  MULTICAST_LAG_ID,
  MULTICAST_LAG_REMOTE_MSB_COUNT,
  MULTICAST_LAG_REMOTE_LSB_COUNT,
  DEV_PORT,
  DYN_HASH_CFG_START_BIT,
  DYN_HASH_CFG_LENGTH,
  DYN_HASH_CFG_ORDER,
};

enum class fieldDestination {
  ACTION_SPEC,
  DIRECT_COUNTER,
  DIRECT_METER,
  DIRECT_LPF,
  DIRECT_WRED,
  DIRECT_REGISTER,
  TTL,
  ENTRY_HIT_STATE,
  INVALID
};

class BfRtTableDataField {
 public:
  BfRtTableDataField(const bf_rt_id_t &tbl_id,
                     const bf_rt_id_t &id,
                     const std::string &data_name,
                     const bf_rt_id_t &act_id,
                     const size_t &s,
                     const DataType &type,
                     const std::vector<std::string> &&choices,
                     const uint64_t def_value,
                     const float def_fl_value,
                     const std::string &def_str_value,
                     const size_t offset,
                     const bool &repeated,
                     const std::set<DataFieldType> &field_types,
                     const bool &mandatory_v,
                     const bool &read_only_v,
                     const bool &container_valid_v,
                     const std::set<Annotation> &annotations_v,
                     const std::set<bf_rt_id_t> &oneof_siblings)
      : table_id(tbl_id),
        field_id(id),
        name(data_name),
        action_id(act_id),
        ptr_valid(!repeated && s > 64),
        size(s),
        field_offset(offset),
        types(field_types),
        dataType(type),
        enum_choices(choices),
        default_value(def_value),
        default_fl_value(def_fl_value),
        default_str_value(def_str_value),
        mandatory(mandatory_v),
        read_only(read_only_v),
        container_valid(container_valid_v),
        annotations(annotations_v),
        oneof_siblings_(oneof_siblings) {}

  virtual ~BfRtTableDataField() = default;

  const std::set<DataFieldType> &getTypes() const { return types; }

  const bf_rt_id_t &getId() const { return field_id; }

  const bf_rt_id_t &getTableId() const { return table_id; }

  const size_t &getSize() const { return size; }

  const std::string &getName() const { return name; }

  const bool &isPtr() const { return ptr_valid; }

  bool isIntArr() const { return (dataType == DataType::INT_ARR); }

  bool isBoolArr() const { return (dataType == DataType::BOOL_ARR); }

  const bool &isMandatory() const { return mandatory; }

  const bool &isReadOnly() const { return read_only; }

  const std::set<bf_rt_id_t> &oneofSiblings() const { return oneof_siblings_; }

  const size_t &getOffset() const { return field_offset; }

  uint64_t defaultValueGet() const { return default_value; }

  std::string defaultStrValueGet() const { return default_str_value; }

  float defaultFlValueGet() const { return default_fl_value; }

  DataType getDataType() const { return dataType; }

  void addDataFieldType(DataFieldType type) { types.insert(type); }

  static fieldDestination getDataFieldDestination(
      const std::set<DataFieldType> &fieldTypes);

  const bool &isContainerValid() const { return container_valid; }

  const std::set<Annotation> &getAnnotations() const { return annotations; }

  bf_status_t containerFieldIdListGet(std::vector<bf_rt_id_t> *id_vec) const;

  const std::vector<std::string> &getEnumChoices() const {
    return enum_choices;
  }
  bf_status_t dataFieldInsertContainer(
      std::unique_ptr<BfRtTableDataField> container_field);
  const std::map<bf_rt_id_t, std::unique_ptr<BfRtTableDataField>> &
  containerMapGet() const {
    return this->container;
  }
  size_t containerSizeGet() const { return container.size(); }
  const std::map<std::string, BfRtTableDataField *> &containerNameMapGet()
      const {
    return this->container_names;
  }

 private:
  const bf_rt_id_t table_id;
  const bf_rt_id_t field_id;
  const std::string name;
  const bf_rt_id_t action_id;
  bool ptr_valid;
  const size_t size;
  const size_t field_offset;
  std::set<DataFieldType> types;
  const DataType dataType;
  // Vector of allowed choices for this field
  const std::vector<std::string> enum_choices;
  // Default value for this data field
  const uint64_t default_value;
  const float default_fl_value;
  const std::string default_str_value;
  const bool mandatory;
  const bool read_only;
  const bool container_valid{false};
  /* Map of Objects within container */
  std::map<bf_rt_id_t, std::unique_ptr<BfRtTableDataField>> container;
  std::map<std::string, BfRtTableDataField *> container_names;
  const std::set<Annotation> annotations;
  const std::set<bf_rt_id_t> oneof_siblings_;
};

class BfRtTableDataObj : public BfRtTableData {
 public:
  BfRtTableDataObj(const BfRtTableObj *tbl_obj,
                   bf_rt_id_t action_id,
                   bf_rt_id_t container_id)
      : table_(tbl_obj), action_id_(action_id), container_id_(container_id) {
    this->actionIdSet(action_id);
    std::vector<bf_rt_id_t> empty;
    this->setActiveFields(empty);
  };
  BfRtTableDataObj(const BfRtTableObj *tbl_obj, bf_rt_id_t action_id)
      : table_(tbl_obj), action_id_(action_id) {
    this->actionIdSet(action_id);
    std::vector<bf_rt_id_t> empty;
    this->setActiveFields(empty);
  };
  BfRtTableDataObj(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj, 0){};

  virtual ~BfRtTableDataObj() = default;

  virtual bf_status_t setValue(const bf_rt_id_t &field_id,
                               const uint64_t &value) override;

  virtual bf_status_t setValue(const bf_rt_id_t &field_id,
                               const uint8_t *value,
                               const size_t &size) override;

  virtual bf_status_t setValue(const bf_rt_id_t &field_id,
                               const std::vector<bf_rt_id_t> &arr) override;

  virtual bf_status_t setValue(const bf_rt_id_t &field_id,
                               const std::vector<bool> &arr) override;

  virtual bf_status_t setValue(const bf_rt_id_t &field_id,
                               const std::vector<std::string> &arr) override;

  virtual bf_status_t setValue(const bf_rt_id_t &field_id,
                               const float &value) override;

  virtual bf_status_t setValue(const bf_rt_id_t &field_id,
                               const bool &value) override;

  virtual bf_status_t setValue(
      const bf_rt_id_t &field_id,
      std::vector<std::unique_ptr<BfRtTableData>> ret_vec) override;

  virtual bf_status_t setValue(const bf_rt_id_t &field_id,
                               const std::string &str) override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               uint64_t *value) const override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               const size_t &size,
                               uint8_t *value) const override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               std::vector<bf_rt_id_t> *arr) const override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               std::vector<bool> *arr) const override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               std::vector<std::string> *arr) const override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               float *value) const override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               bool *value) const override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               std::vector<uint64_t> *arr) const override;

  virtual bf_status_t getValue(
      const bf_rt_id_t &field_id,
      std::vector<BfRtTableData *> *ret_vec) const override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               std::string *str) const override;

  bf_status_t actionIdGet(bf_rt_id_t *act_id) const override;
  virtual bf_status_t getParent(const BfRtTable **table) const override;

  virtual bf_status_t isActive(const bf_rt_id_t &field_id,
                               bool *is_active) const override;
  virtual bf_status_t reset();

  virtual bf_status_t reset(const bf_rt_id_t &action_id);

  virtual bf_status_t reset(const bf_rt_id_t &action_id,
                            const std::vector<bf_rt_id_t> &fields);

  virtual bf_status_t reset(const std::vector<bf_rt_id_t> &fields);

  size_t getMaxdataSz();
  size_t getMaxdataSzbits();
  size_t getdataSzbits(bf_rt_id_t act_id);
  size_t getdataSz(bf_rt_id_t act_id);

  void actionIdSet(bf_rt_id_t act_id) { this->action_id_ = act_id; }
  void containerIdSet(bf_rt_id_t c_id) { this->container_id_ = c_id; }
  const bf_rt_id_t &actionIdGet_() const { return action_id_; };
  const bf_rt_id_t &containerIdGet_() const { return container_id_; };
  bf_status_t setActiveFields(const std::vector<bf_rt_id_t> &fields);
  const std::set<bf_rt_id_t> &getActiveFields() const { return active_fields_; }
  bool allFieldsSet() const { return all_fields_set_; }
  bool checkFieldActive(const bf_rt_id_t &field_id) const;
  void addToActiveFields(const bf_rt_id_t &field_id) {
    this->active_fields_.insert(field_id);
  }
  void removeActiveFields(const std::set<bf_rt_id_t> fields) {
    for (const auto &field_id : fields) {
      this->active_fields_.erase(field_id);
    }
  }
  void removeActiveField(const bf_rt_id_t &field_id) {
    this->active_fields_.erase(field_id);
  }

 protected:
  // const backpointer to the table object
  const BfRtTableObj *table_;

 private:
  bf_rt_id_t action_id_{0};
  bf_rt_id_t container_id_{0};
  std::set<bf_rt_id_t> active_fields_;
  bool all_fields_set_;
};

}  // bfrt
#endif  // _BF_RT_TABLE_DATA_IMPL_HPP

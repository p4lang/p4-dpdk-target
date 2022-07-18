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
#ifndef _BF_RT_P4_TABLE_KEY_IMPL_HPP
#define _BF_RT_P4_TABLE_KEY_IMPL_HPP

#include <bf_rt_common/bf_rt_table_key_impl.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>

namespace bfrt {

class BfRtMatchActionKey : public BfRtTableKeyObj {
 public:
  BfRtMatchActionKey(const BfRtTableObj *tbl_obj) : BfRtTableKeyObj(tbl_obj) {
    // Allocate the key array and mask array based on the key size
    setKeySz();
    partition_index = 0;
    priority = 0;
  }

  BfRtMatchActionKey(BfRtMatchActionKey const &) = delete;
  BfRtMatchActionKey &operator=(const BfRtMatchActionKey&) = delete;

  ~BfRtMatchActionKey() {
    if (key_array) {
      delete[] key_array;
    }
    if (mask_array) {
      delete[] mask_array;
    }
  }
  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t setValueandMask(const bf_rt_id_t &field_id,
                              const uint64_t &value,
                              const uint64_t &mask);

  bf_status_t setValueandMask(const bf_rt_id_t &field_id,
                              const uint8_t *value1,
                              const uint8_t *value2,
                              const size_t &size);

  bf_status_t setValueRange(const bf_rt_id_t &field_id,
                            const uint64_t &start,
                            const uint64_t &end);

  bf_status_t setValueRange(const bf_rt_id_t &field_id,
                            const uint8_t *start,
                            const uint8_t *end,
                            const size_t &size);

  bf_status_t setValueLpm(const bf_rt_id_t &field_id,
                          const uint64_t &value1,
                          const uint16_t &p_length);

  bf_status_t setValueLpm(const bf_rt_id_t &field_id,
                          const uint8_t *value1,
                          const uint16_t &p_length,
                          const size_t &size);

  bf_status_t setValueOptional(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const bool &is_valid);
  bf_status_t setValueOptional(const bf_rt_id_t &field_id,
                               const uint8_t *value,
                               const bool &is_valid,
                               const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_status_t getValueandMask(const bf_rt_id_t &field_id,
                              uint64_t *value1,
                              uint64_t *value2) const;

  bf_status_t getValueandMask(const bf_rt_id_t &field_id,
                              const size_t &size,
                              uint8_t *value1,
                              uint8_t *value2) const;

  bf_status_t getValueRange(const bf_rt_id_t &field_id,
                            uint64_t *start,
                            uint64_t *end) const;

  bf_status_t getValueRange(const bf_rt_id_t &field_id,
                            const size_t &size,
                            uint8_t *start,
                            uint8_t *end) const;

  bf_status_t getValueLpm(const bf_rt_id_t &field_id,
                          uint64_t *start,
                          uint16_t *pLength) const;

  bf_status_t getValueLpm(const bf_rt_id_t &field_id,
                          const size_t &size,
                          uint8_t *value1,
                          uint16_t *p_length) const;

  bf_status_t getValueOptional(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               bool *is_valid) const;
  bf_status_t getValueOptional(const bf_rt_id_t &field_id,
                               const size_t &size,
                               uint8_t *value,
                               bool *is_valid) const;

  void setPriority(uint32_t pri) { priority = pri; }
  void setPartitionIndex(uint32_t p) { partition_index = p; }
  // Hidden
  void populate_match_spec(pipe_tbl_match_spec_t *pipe_match_spec) const;

  void populate_key_from_match_spec(
      const pipe_tbl_match_spec_t &pipe_match_spec);

  void set_key_from_match_spec_by_deepcopy(
      const pipe_tbl_match_spec_t *pipe_match_spec);

  bf_status_t reset();

 private:
  void setValueInternal(const BfRtTableKeyField &key_field,
                        const uint8_t *value,
                        const size_t &num_bytes);
  void getValueInternal(const BfRtTableKeyField &key_field,
                        uint8_t *value,
                        const size_t &num_bytes) const;
  void setValueAndMaskInternal(const BfRtTableKeyField &key_field,
                               const uint8_t *value,
                               const uint8_t *mask,
                               const size_t &num_bytes,
                               bool do_masking);
  void getValueAndMaskInternal(const BfRtTableKeyField &key_field,
                               uint8_t *value,
                               uint8_t *mask,
                               const size_t &num_bytes) const;
  void setKeySz();

  void packFieldIntoMatchSpecByteBuffer(const BfRtTableKeyField &key_field,
                                        const size_t &size,
                                        const bool &do_masking,
                                        const uint8_t *field_buf,
                                        const uint8_t *field_mask_buf,
                                        uint8_t *match_spec_buf,
                                        uint8_t *match_mask_spec_buf);
  void unpackFieldFromMatchSpecByteBuffer(const BfRtTableKeyField &key_field,
                                          const size_t &size,
                                          const uint8_t *match_spec_buf,
                                          uint8_t *field_buf) const;

  uint16_t num_valid_match_bits;
  uint16_t num_valid_match_bytes;
  uint8_t *key_array;
  uint8_t *mask_array;
  uint32_t partition_index;
  uint32_t priority;
};

class BfRtActionTableKey : public BfRtTableKeyObj {
 public:
  BfRtActionTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj), member_id(){};

  ~BfRtActionTableKey() = default;

  bf_rt_id_t getMemberId() const { return member_id; }
  void setMemberId(bf_rt_id_t mbr_id) { member_id = mbr_id; }
  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_status_t reset() {
    member_id = 0;
    return BF_SUCCESS;
  }

 private:
  bf_rt_id_t member_id;
};

class BfRtSelectorTableKey : public BfRtTableKeyObj {
 public:
  BfRtSelectorTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj), group_id(){};

  ~BfRtSelectorTableKey() = default;
  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_rt_id_t getGroupId() const { return group_id; }
  void setGroupId(bf_rt_id_t grp_id) { group_id = grp_id; }

  bf_status_t reset() {
    group_id = 0;
    return BF_SUCCESS;
  };

 private:
  bf_rt_id_t group_id;
};

class BfRtCounterTableKey : public BfRtTableKeyObj {
 public:
  BfRtCounterTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj), counter_id(){};
  ~BfRtCounterTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  uint32_t getCounterId() const { return counter_id; }

  void setCounterId(uint32_t id) { counter_id = id; }

  void setIdxKey(uint32_t idx) { counter_id = idx; }

  uint32_t getIdxKey() const { return counter_id; }

  bf_status_t reset() {
    counter_id = 0;
    return BF_SUCCESS;
  }

 private:
  uint32_t counter_id;
};

class BfRtMeterTableKey : public BfRtTableKeyObj {
 public:
  BfRtMeterTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj), meter_id(){};
  ~BfRtMeterTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  void setIdxKey(uint32_t idx) { meter_id = idx; }

  uint32_t getIdxKey() const { return meter_id; }

  bf_status_t reset() {
    meter_id = 0;
    return BF_SUCCESS;
  }

 private:
  uint32_t meter_id;
};

class BfRtLPFTableKey : public BfRtTableKeyObj {
 public:
  BfRtLPFTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj), lpf_id(){};
  ~BfRtLPFTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  void setIdxKey(uint32_t idx) { lpf_id = idx; }

  uint32_t getIdxKey() const { return lpf_id; }

  bf_status_t reset() {
    lpf_id = 0;
    return BF_SUCCESS;
  }

 private:
  uint32_t lpf_id;
};

class BfRtWREDTableKey : public BfRtTableKeyObj {
 public:
  BfRtWREDTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj), wred_id(){};
  ~BfRtWREDTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  void setIdxKey(uint32_t idx) { wred_id = idx; }

  uint32_t getIdxKey() const { return wred_id; }

  bf_status_t reset() {
    wred_id = 0;
    return BF_SUCCESS;
  }

 private:
  uint32_t wred_id;
};

class BfRtRegisterTableKey : public BfRtTableKeyObj {
 public:
  BfRtRegisterTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj), register_id(){};
  ~BfRtRegisterTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  void setIdxKey(uint32_t idx) { register_id = idx; }

  uint32_t getIdxKey() const { return register_id; }

  bf_status_t reset() {
    register_id = 0;
    return BF_SUCCESS;
  }

 private:
  uint32_t register_id;
};

class BfRtPVSTableKey : public BfRtTableKeyObj {
 public:
  BfRtPVSTableKey(const BfRtTableObj *tbl_obj) : BfRtTableKeyObj(tbl_obj){};
  ~BfRtPVSTableKey() = default;

  bf_status_t setValueandMask(const bf_rt_id_t &field_id,
                              const uint8_t *value1,
                              const uint8_t *value2,
                              const size_t &size);

  bf_status_t getValueandMask(const bf_rt_id_t &field_id,
                              const size_t &size,
                              uint8_t *value1,
                              uint8_t *value2) const;

  bf_status_t setValueandMask(const bf_rt_id_t &field_id,
                              const uint64_t &value,
                              const uint64_t &mask);

  bf_status_t getValueandMask(const bf_rt_id_t &field_id,
                              uint64_t *value,
                              uint64_t *mask) const;
  // Hidden
  void populate_match_spec(uint32_t *key_p, uint32_t *mask_p) const {
    // match spec in pvs_table is key and mask. It's not a struct include all
    // config as MatchAction table.
    *key_p = key_;
    *mask_p = mask_;
  };

  // be careful while using this function, may change the saved mask and key
  void populate_key_from_match_spec(uint32_t &key, uint32_t &mask) {
    key_ = key;
    mask_ = mask;
  };

  bf_status_t reset() {
    key_ = 0;
    mask_ = 0;
    return BF_SUCCESS;
  }

 private:
  void setValueAndMaskInternal(const size_t &field_offset,
                               const uint8_t *value,
                               const uint8_t *mask,
                               const size_t &num_bytes);

  void getValueAndMaskInternal(const size_t &field_offset,
                               uint8_t *value,
                               uint8_t *mask,
                               const size_t &num_bytes) const;

  uint32_t key_{0};
  uint32_t mask_{0};
};

class BfRtTblDbgCntTableKey : public BfRtTableKeyObj {
 public:
  BfRtTblDbgCntTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj){};
  ~BfRtTblDbgCntTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &str_value);

  bf_status_t getValue(const bf_rt_id_t &field_id, std::string *value) const;

  const std::string getTblName() const { return this->tbl_name; };
  void setTblName(const std::string &name) { this->tbl_name = name; };

 private:
  std::string tbl_name;
};

class BfRtLogDbgCntTableKey : public BfRtTableKeyObj {
 public:
  BfRtLogDbgCntTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj){};
  ~BfRtLogDbgCntTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);
  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

 private:
  std::map<bf_rt_id_t, uint32_t> fields_;
};

class BfRtDynHashComputeTableKey : public BfRtTableKeyObj {
 public:
  BfRtDynHashComputeTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj){};
  ~BfRtDynHashComputeTableKey() = default;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override;

  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;

  // unexposed funcs
  bf_status_t attrListGet(
      const BfRtTable *cfg_tbl,
      std::vector<pipe_hash_calc_input_field_attribute_t> *attr_list) const;

 private:
  bool isConstant(const bf_rt_id_t &field_id, const BfRtTable *cfg_tbl) const;
  std::map<bf_rt_id_t, uint64_t> uint64_fields;
};

class BfRtSelectorGetMemberTableKey : public BfRtTableKeyObj {
 public:
  BfRtSelectorGetMemberTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj){};
  ~BfRtSelectorGetMemberTableKey() = default;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override;

  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;

 private:
  std::map<bf_rt_id_t, uint64_t> uint64_fields;
};

// Wrapper Class for Pipe mgr match spec
class PipeMgrMatchSpec {
 public:
  PipeMgrMatchSpec(size_t num_bytes) {
    std::memset(&pipe_match_spec, 0, sizeof(pipe_match_spec));
    pipe_match_spec.match_value_bits = new uint8_t[num_bytes];
    pipe_match_spec.match_mask_bits = new uint8_t[num_bytes];
  }
  ~PipeMgrMatchSpec() {
    delete[] pipe_match_spec.match_value_bits;
    delete[] pipe_match_spec.match_mask_bits;
  }
  pipe_tbl_match_spec_t *getPipeMatchSpec();

 private:
  pipe_tbl_match_spec_t pipe_match_spec;
};

}  // bfrt

#endif  // _BF_RT_P4_TABLE_KEY_IMPL_HPP

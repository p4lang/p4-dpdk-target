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
#ifndef _TDI_P4_TABLE_KEY_IMPL_HPP
#define _TDI_P4_TABLE_KEY_IMPL_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

#include <tdi/common/tdi_info.hpp>
#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_table_key.hpp>

#include <tdi/arch/pna/pna_defs.h>

#include <tdi_common/tdi_context_info.hpp>

typedef uint32_t tdi_rt_act_mem_id_t;
typedef uint32_t tdi_rt_sel_grp_id_t;

namespace tdi {
namespace pna {
namespace rt {

class MatchActionKey : public tdi::TableKey {
 public:
  MatchActionKey(const Table *table) : TableKey(table) {
    // Allocate the key array and mask array based on the key size
    setKeySz();
    partition_index = 0;
    priority = 0;
  }

  MatchActionKey(MatchActionKey const &) = delete;
  MatchActionKey &operator=(const MatchActionKey&) = delete;

  ~MatchActionKey() {
    if (key_array) {
      delete[] key_array;
    }
    if (mask_array) {
      delete[] mask_array;
    }
  }

  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) const override;

  virtual tdi_status_t reset() override;

  void setPriority(uint32_t pri) { priority = pri; }
  void setPartitionIndex(uint32_t p) { partition_index = p; }
  // Hidden
  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint64_t &value);
  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint8_t *value,
                        const size_t &size);
  tdi_status_t setValueandMask(const tdi::KeyFieldInfo *key_field,
                               const uint64_t &value,
                               const uint64_t &mask);
  tdi_status_t setValueandMask(const tdi::KeyFieldInfo *key_field,
                               const uint8_t *value,
                               const uint8_t *mask,
                               const size_t &size);
  tdi_status_t setValueRange(const tdi::KeyFieldInfo *key_field,
                             const uint64_t &start,
                             const uint64_t &end);
  tdi_status_t setValueRange(const tdi::KeyFieldInfo *key_field,
                             const uint8_t *start,
                             const uint8_t *end,
                             const size_t &size);
  tdi_status_t setValueLpm(const tdi::KeyFieldInfo *key_field,
                           const uint64_t &value,
                           const uint16_t &p_length);
  tdi_status_t setValueLpm(const tdi::KeyFieldInfo *key_field,
                           const uint8_t *value1,
                           const uint16_t &p_length,
                           const size_t &size);
  tdi_status_t setValueOptional(const tdi::KeyFieldInfo *key_field,
                                const uint64_t &value,
                                const bool &is_valid);
  tdi_status_t setValueOptional(const tdi::KeyFieldInfo *key_field,
                                const uint8_t *value,
                                const bool &is_valid,
                                const size_t &size);
  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        uint64_t *value) const;
  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        const size_t &size,
                        uint8_t *value) const;
  tdi_status_t getValueandMask(const tdi::KeyFieldInfo *key_field,
                               uint64_t *value1,
                               uint64_t *value2) const;
  tdi_status_t getValueandMask(const tdi::KeyFieldInfo *key_field,
                               const size_t &size,
                               uint8_t *value1,
                               uint8_t *value2) const;
  tdi_status_t getValueRange(const tdi::KeyFieldInfo *key_field,
                             uint64_t *start,
                             uint64_t *end) const;
  tdi_status_t getValueRange(const tdi::KeyFieldInfo *key_field,
                             const size_t &size,
                             uint8_t *start,
                             uint8_t *end) const;
  tdi_status_t getValueLpm(const tdi::KeyFieldInfo *key_field,
                           uint64_t *start,
                           uint16_t *p_length) const;
  tdi_status_t getValueLpm(const tdi::KeyFieldInfo *key_field,
                           const size_t &size,
                           uint8_t *start,
                           uint16_t *p_length) const;
  tdi_status_t getValueOptional(const tdi::KeyFieldInfo *key_field,
                                uint64_t *value1,
                                bool *is_valid) const;
  tdi_status_t getValueOptional(const tdi::KeyFieldInfo *key_field,
                                const size_t &size,
                                uint8_t *value1,
                                bool *is_valid) const;

  void populate_match_spec(pipe_tbl_match_spec_t *pipe_match_spec) const;

  void populate_key_from_match_spec(
      const pipe_tbl_match_spec_t &pipe_match_spec);

  void set_key_from_match_spec_by_deepcopy(
      const pipe_tbl_match_spec_t *pipe_match_spec);

 private:
  void setValueInternal(const tdi::KeyFieldInfo &key_field,
                        const uint8_t *value,
                        const size_t &num_bytes);
  void getValueInternal(const tdi::KeyFieldInfo &key_field,
                        uint8_t *value,
                        const size_t &num_bytes) const;
  void setValueAndMaskInternal(const tdi::KeyFieldInfo &key_field,
                               const uint8_t *value,
                               const uint8_t *mask,
                               const size_t &num_bytes,
                               bool do_masking);
  void getValueAndMaskInternal(const tdi::KeyFieldInfo &key_field,
                               uint8_t *value,
                               uint8_t *mask,
                               const size_t &num_bytes) const;
  void setKeySz();

  void packFieldIntoMatchSpecByteBuffer(const tdi::KeyFieldInfo &key_field,
                                        const size_t &size,
                                        const bool &do_masking,
                                        const uint8_t *field_buf,
                                        const uint8_t *field_mask_buf,
                                        uint8_t *match_spec_buf,
                                        uint8_t *match_mask_spec_buf);
  void unpackFieldFromMatchSpecByteBuffer(const tdi::KeyFieldInfo &key_field,
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

class ActionProfileKey : public tdi::TableKey {
 public:
  ActionProfileKey(const Table *tbl_obj) : TableKey(tbl_obj), member_id(){};

  ~ActionProfileKey() = default;
  tdi_status_t setValue(const tdi_id_t &field_id,
                        const tdi::KeyFieldValue &field_value) override;
  tdi_status_t getValue(const tdi_id_t &field_id,
                        tdi::KeyFieldValue *field_value) const override;

  tdi_id_t getMemberId() const { return member_id; }
  void setMemberId(tdi_id_t mbr_id) { member_id = mbr_id; }

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint64_t &value);
  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint8_t *value,
                        const size_t &size);
  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        uint64_t *value) const;
  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        const size_t &size,
                        uint8_t *value) const;

  tdi_status_t reset() {
    member_id = 0;
    return TDI_SUCCESS;
  }

 private:
  tdi_rt_act_mem_id_t member_id;
};

class SelectorTableKey : public tdi::TableKey {
 public:
  SelectorTableKey(const Table *table) : tdi::TableKey(table), group_id(){};

  ~SelectorTableKey() = default;
  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) const override;

  // Hidden
  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint64_t &value);

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint8_t *value,
                        const size_t &size);

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        uint64_t *value) const;

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        const size_t &size,
                        uint8_t *value) const;

  tdi_id_t getGroupId() const { return group_id; }

  void setGroupId(tdi_id_t grp_id) { group_id = grp_id; }

  virtual tdi_status_t reset() override {
    group_id = 0;
    return TDI_SUCCESS;
  };

 private:
  tdi_id_t group_id;
};

class CounterIndirectTableKey : public tdi::TableKey {
 public:
  CounterIndirectTableKey(const tdi::Table *table)
      : tdi::TableKey(table), counter_id(){};
  ~CounterIndirectTableKey() = default;

  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) const override;

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field, const uint64_t &value);

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field, uint64_t *value) const;

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                       const size_t &size,
                       uint8_t *value) const;

  uint32_t getCounterId() const { return counter_id; }

  void setCounterId(uint32_t id) { counter_id = id; }

  void setIdxKey(uint32_t idx) { counter_id = idx; }

  uint32_t getIdxKey() const { return counter_id; }

  virtual tdi_status_t reset() override{
    counter_id = 0;
    return TDI_SUCCESS;
  }

 private:
  uint32_t counter_id;
};

#if 0
class TdiMeterTableKey : public TdiTableKeyObj {
 public:
  TdiMeterTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj), meter_id(){};
  ~TdiMeterTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id, const uint64_t &value);

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  tdi_status_t getValue(const tdi_id_t &field_id, uint64_t *value) const;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  void setIdxKey(uint32_t idx) { meter_id = idx; }

  uint32_t getIdxKey() const { return meter_id; }

  tdi_status_t reset() {
    meter_id = 0;
    return TDI_SUCCESS;
  }

 private:
  uint32_t meter_id;
};
#endif

class RegisterTableKey : public tdi::TableKey {
 public:
  RegisterTableKey(const tdi::Table *table)
      : tdi::TableKey(table), register_id(){};
  ~RegisterTableKey() = default;

  virtual tdi_status_t setValue(const tdi_id_t &field_id,
		                const tdi::KeyFieldValue &field_value) override;
  virtual tdi_status_t getValue(const tdi_id_t &field_id,
		                tdi::KeyFieldValue *value) const override;

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field, const uint64_t &value);

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
			const uint8_t *value,
			const size_t &size);

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field, uint64_t *value) const;

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
			const size_t &size,
			uint8_t *value) const;

  void setIdxKey(uint32_t idx) { register_id = idx; }

  uint32_t getIdxKey() const { return register_id; }

  tdi_status_t reset() {
    register_id = 0;
    return TDI_SUCCESS;
  }

 private:
  uint32_t register_id;
};

#if 0
class TdiSelectorGetMemberTableKey : public TdiTableKeyObj {
 public:
  TdiSelectorGetMemberTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj){};
  ~TdiSelectorGetMemberTableKey() = default;
  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint64_t &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       uint64_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override;

  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &size);
  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &size) const;

 private:
  std::map<tdi_id_t, uint64_t> uint64_fields;
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
#endif

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  // _TDI_P4_TABLE_KEY_IMPL_HPP

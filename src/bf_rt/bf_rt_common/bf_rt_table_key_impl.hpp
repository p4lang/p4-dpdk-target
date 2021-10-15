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
#ifndef _BF_RT_TABLE_KEY_IMPL_HPP
#define _BF_RT_TABLE_KEY_IMPL_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_key.hpp>

namespace bfrt {

class BfRtTableObj;

class BfRtTableKeyField {
 public:
  BfRtTableKeyField(bf_rt_id_t id,
                    std::string n,
                    size_t s,
                    KeyFieldType f_type,
                    DataType d_type,
                    std::vector<std::string> &&choices,
                    size_t offset,
                    const size_t &startbit,
                    const bool &fieldslice,
                    const size_t &fieldfullbytesize,
                    bool is_partition_id = false)
      : field_id(id),
        name(n),
        size_bits(s),
        field_type(f_type),
        data_type(d_type),
        enum_choices(choices),
        field_offset(offset),
        start_bit(startbit),
        is_field_slice(fieldslice),
        parent_field_full_byte_size(fieldfullbytesize),
        is_ptr((s > 64) ? true : false),
        is_partition(is_partition_id),
        match_priority((n == "$MATCH_PRIORITY") ? true : false) {}

  virtual ~BfRtTableKeyField() = default;

  const KeyFieldType &getType() const { return field_type; }

  const DataType &getDataType() const { return data_type; }

  const bf_rt_id_t &getId() const { return field_id; }

  const bool &isPtr() const { return is_ptr; }

  const size_t &getSize() const { return size_bits; }

  const std::string &getName() const { return name; }

  const size_t &getOffset() const { return field_offset; }

  size_t getStartBit() const { return start_bit; }

  bool isFieldSlice() const { return is_field_slice; }

  bool isMatchPriority() const { return match_priority; }

  bool isPartitionIndex() const { return is_partition; }

  const std::vector<std::string> &getEnumChoices() const {
    return enum_choices;
  }

  size_t getParentFieldFullByteSize() const {
    return parent_field_full_byte_size;
  }

 private:
  const bf_rt_id_t field_id;
  const std::string name;
  const size_t size_bits;
  const KeyFieldType field_type;
  const DataType data_type;
  const std::vector<std::string> enum_choices;
  const size_t field_offset;
  const size_t start_bit{0};
  // Flag to indicate if this is a field slice or not
  const bool is_field_slice{false};
  // This might vary from the 'size_bits' in case of field slices when the field
  // slice
  // width does not equal the size of the entire key field
  const size_t parent_field_full_byte_size{0};
  const bool is_ptr;
  // flag to indicate whether it is a priority index or not
  const bool is_partition;
  // A flag to indicate this is a match priority field
  const bool match_priority;
};

class BfRtTableKeyObj : public BfRtTableKey {
 public:
  BfRtTableKeyObj(const BfRtTableObj *tbl_obj) : table_(tbl_obj) {}
  virtual ~BfRtTableKeyObj() = default;

  virtual bf_status_t setValue(const bf_rt_id_t &field_id,
                               const uint64_t &value) override;

  virtual bf_status_t setValue(const bf_rt_id_t &field_id,
                               const uint8_t *value,
                               const size_t &size) override;

  virtual bf_status_t setValue(const bf_rt_id_t &field_id,
                               const std::string &value) override;

  virtual bf_status_t setValueandMask(const bf_rt_id_t &field_id,
                                      const uint64_t &value,
                                      const uint64_t &mask) override;

  virtual bf_status_t setValueandMask(const bf_rt_id_t &field_id,
                                      const uint8_t *value1,
                                      const uint8_t *value2,
                                      const size_t &size) override;

  virtual bf_status_t setValueRange(const bf_rt_id_t &field_id,
                                    const uint64_t &start,
                                    const uint64_t &end) override;

  virtual bf_status_t setValueRange(const bf_rt_id_t &field_id,
                                    const uint8_t *start,
                                    const uint8_t *end,
                                    const size_t &size) override;

  virtual bf_status_t setValueLpm(const bf_rt_id_t &field_id,
                                  const uint64_t &value1,
                                  const uint16_t &p_length) override;

  virtual bf_status_t setValueLpm(const bf_rt_id_t &field_id,
                                  const uint8_t *value1,
                                  const uint16_t &p_length,
                                  const size_t &size) override;

  virtual bf_status_t setValueOptional(const bf_rt_id_t &field_id,
                                       const uint64_t &value,
                                       const bool &is_valid) override;
  virtual bf_status_t setValueOptional(const bf_rt_id_t &field_id,
                                       const uint8_t *value,
                                       const bool &is_valid,
                                       const size_t &size) override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               uint64_t *value) const override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               const size_t &size,
                               uint8_t *value) const override;

  virtual bf_status_t getValue(const bf_rt_id_t &field_id,
                               std::string *value) const override;

  virtual bf_status_t getValueandMask(const bf_rt_id_t &field_id,
                                      uint64_t *value1,
                                      uint64_t *value2) const override;

  virtual bf_status_t getValueandMask(const bf_rt_id_t &field_id,
                                      const size_t &size,
                                      uint8_t *value1,
                                      uint8_t *value2) const override;
  virtual bf_status_t getValueRange(const bf_rt_id_t &field_id,
                                    uint64_t *start,
                                    uint64_t *end) const override;

  virtual bf_status_t getValueRange(const bf_rt_id_t &field_id,
                                    const size_t &size,
                                    uint8_t *start,
                                    uint8_t *end) const override;

  virtual bf_status_t getValueLpm(const bf_rt_id_t &field_id,
                                  uint64_t *start,
                                  uint16_t *pLength) const override;

  virtual bf_status_t getValueLpm(const bf_rt_id_t &field_id,
                                  const size_t &size,
                                  uint8_t *value1,
                                  uint16_t *p_length) const override;

  virtual bf_status_t getValueOptional(const bf_rt_id_t &field_id,
                                       uint64_t *value,
                                       bool *is_valid) const override;
  virtual bf_status_t getValueOptional(const bf_rt_id_t &field_id,
                                       const size_t &size,
                                       uint8_t *value,
                                       bool *is_valid) const override;

  virtual bf_status_t reset();

  bf_status_t tableGet(const BfRtTable **table) const override final;

 protected:
  // const Backpointer to the table object
  const BfRtTableObj *table_;
};

}  // bfrt

#endif  //_BF_RT_TABLE_KEY_IMPL_HPP

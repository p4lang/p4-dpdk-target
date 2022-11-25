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
#ifndef _TDI_FIXED_TABLE_KEY_IMPL_HPP
#define _TDI_FIXED_TABLE_KEY_IMPL_HPP

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
#include <tdi_common/tdi_context_info.hpp>
#include <tdi/arch/pna/pna_defs.h>

namespace tdi {
namespace pna {
namespace rt {

class FixedFunctionTableKey : public tdi::TableKey {
 public:
  FixedFunctionTableKey(const Table *table) : TableKey(table) {
    // Allocate the key array and mask array based on the key size
    setKeySz();
  }

  FixedFunctionTableKey(FixedFunctionTableKey const &) = delete;
  FixedFunctionTableKey &operator=(const FixedFunctionTableKey &) = delete;

  ~FixedFunctionTableKey () {
    if (key_array) {
      delete[] key_array;
    }
  }

  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) const override;

  virtual tdi_status_t reset() override;

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


  void populate_fixed_fun_key_spec
	  (fixed_function_key_spec_t *fixed_match_spec) const;

  void populate_key_from_match_spec(
      const fixed_function_key_spec_t &fixed_match_spec);

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
};

}  // namespace rt
}  // namespace pna
}  // namespace tdi
#endif  // _TDI_FIXED_TABLE_KEY_IMPL_HPP

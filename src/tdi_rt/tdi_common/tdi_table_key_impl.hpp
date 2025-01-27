/*******************************************************************************
 * Copyright (c) 2017-2018 Barefoot Networks, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * $Id: $
 *
 ******************************************************************************/
#ifndef _TDI_TABLE_KEY_IMPL_HPP
#define _TDI_TABLE_KEY_IMPL_HPP

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

namespace tdi {
namespace pna {
namespace rt {

class MatchActionKey : public tdi::TableKey {
 public:
  MatchActionKey(const Table *table) : TableKey(table){
  // Allocate the key array and mask array based on the key size
  setKeySz();
  priority = 0;
}

~MatchActionKey() {
  if (key_array) {
    delete[] key_array;
  }
  if (mask_array) {
    delete[] mask_array;
  }
}

virtual tdi_status_t setValue(const tdi_id_t &field_id,
                              const tdi::KeyFieldValue &&field_value) override;
virtual tdi_status_t setValue(const tdi_id_t &field_id,
                              const tdi::KeyFieldValue &field_value) override;

virtual tdi_status_t getValue(const tdi_id_t &field_id,
                              tdi::KeyFieldValue *value) override;
virtual tdi_status_t tableGet(const tdi::Table **table) override;

virtual tdi_status_t reset();

void setPriority(uint32_t pri) { priority = pri; }
// Hidden
void populate_match_spec(pipe_tbl_match_spec_t *pipe_match_spec) const;

void populate_key_from_match_spec(const pipe_tbl_match_spec_t &pipe_match_spec);

void set_key_from_match_spec_by_deepcopy(
    const pipe_tbl_match_spec_t *pipe_match_spec);

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
uint32_t priority;
};

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  //_TDI_TABLE_KEY_IMPL_HPP

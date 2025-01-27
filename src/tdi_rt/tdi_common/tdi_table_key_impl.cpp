/*******************************************************************************
 * Copyright (c) 2017-2018 Barefoot Networks, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * $Id: $
 *
 ******************************************************************************/

#include "tdi_table_key_impl.hpp"

namespace tdi {
namespace pna {
namespace rt {

class MatchTableKey : Public tdi::TableKey {
 public:
  MatchTableKey(const Table *table) : TableKey(table){};

  virtual tdi_status_t setValue(
      const tdi_id_t &field_id,
      const tdi::KeyFieldValue &&field_value) override;
  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) override;
  virtual tdi_status_t tableGet(const tdi::Table **table) override;

  virtual tdi_status_t reset();
};

}  // namespace rt
}  // namespace pna
}  // namespace tdi

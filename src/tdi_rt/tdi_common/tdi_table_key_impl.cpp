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

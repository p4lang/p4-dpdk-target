// SPDX-FileCopyrightText: 2021 Intel Corporation
// Copyright (C) 2021 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0
#ifndef _TDI_TABLE_DATA_UTILS_HPP_
#define _TDI_TABLE_DATA_UTILS_HPP_

namespace tdi {

namespace utils {

class TableDataUtils {
 public:
  TableDataUtils() = delete;

  static tdi_status_t boundsCheck(const DataFieldInfo &field,
                                 const uint64_t &value,
                                 const uint8_t *value_ptr,
                                 const size_t &size);

  static tdi_status_t fieldTypeCompatibilityCheck(
      const DataFieldInfo &field,
      const uint64_t *value,
      const uint8_t *value_ptr,
      const size_t &size);

  static void toHostOrderData(const DataFieldInfo &field,
                              const uint8_t *value_ptr,
                              uint64_t *out_data);

  static void toNetworkOrderData(const DataFieldInfo &field,
                                 const uint64_t &in_data,
                                 uint8_t *value_ptr);
};  // class TableDataUtils

}  // namespace utils
}  // namespace tdi

#endif  // _TDI_TABLE_DATA_UTILS_HPP_

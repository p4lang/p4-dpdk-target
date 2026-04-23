// SPDX-FileCopyrightText: 2021 Intel Corporation
// Copyright (C) 2021 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0
#ifndef _BF_RT_TABLE_DATA_UTILS_HPP_
#define _BF_RT_TABLE_DATA_UTILS_HPP_

namespace bfrt {

namespace utils {

class BfRtTableDataUtils {
 public:
  BfRtTableDataUtils() = delete;

  static bf_status_t boundsCheck(const BfRtTableDataField &field,
                                 const uint64_t &value,
                                 const uint8_t *value_ptr,
                                 const size_t &size);

  static bf_status_t fieldTypeCompatibilityCheck(
      const BfRtTableDataField &field,
      const uint64_t *value,
      const uint8_t *value_ptr,
      const size_t &size);

  static void toHostOrderData(const BfRtTableDataField &field,
                              const uint8_t *value_ptr,
                              uint64_t *out_data);

  static void toNetworkOrderData(const BfRtTableDataField &field,
                                 const uint64_t &in_data,
                                 uint8_t *value_ptr);
};  // class BfRtTableDataUtils

}  // namespace utils
}  // namespace bfrt

#endif  // _BF_RT_TABLE_DATA_UTILS_HPP_

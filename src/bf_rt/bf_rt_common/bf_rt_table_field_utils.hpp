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
#ifndef _BF_RT_TABLE_FIELD_UTILS_HPP_
#define _BF_RT_TABLE_FIELD_UTILS_HPP_

#include "bf_rt_utils.hpp"
#include "bf_rt_table_impl.hpp"

namespace bfrt {

namespace utils {

class BfRtTableFieldUtils {
 public:
  BfRtTableFieldUtils() = delete;

  // Template classes must be defined whole in headers
  template <class T>
  static bf_status_t boundsCheck(const BfRtTableObj &table,
                                 const T &field,
                                 const uint64_t &value,
                                 const uint8_t *value_ptr,
                                 const size_t &s) {
    size_t field_size = field.getSize();

    if (!value_ptr) {
      // if field_size == 64 - automatically will fit,
      // if > 64 fieldTypeCompatibilityCheck check will catch it
      if (field_size < 64) {
        auto limit = (1ULL << field_size) - 1;
        if (value > limit) {
          LOG_ERROR("ERROR: %s:%d %s : Value of %" PRIu64
                    " exceeds the size of the field with id %d ",
                    __func__,
                    __LINE__,
                    table.table_name_get().c_str(),
                    value,
                    field.getId());
          return BF_INVALID_ARG;
        }
      }
    } else {
      if (s != (field_size + 7) / 8) {
        LOG_ERROR(
            "ERROR: %s:%d %s: Array size of %zu bytes isn't ==  size of the "
            "field %zu with id %d ",
            __func__,
            __LINE__,
            table.table_name_get().c_str(),
            s,
            (field_size + 7) / 8,
            field.getId());
        return BF_INVALID_ARG;
      }
      int shift = field_size % 8;
      if (shift != 0 && (value_ptr[0] >> shift) > 0) {
        LOG_ERROR(
            "ERROR: %s:%d %s: Specified value is greater than what field size"
            " %zu bits allows for field id %d",
            __func__,
            __LINE__,
            table.table_name_get().c_str(),
            field_size,
            field.getId());
        return BF_INVALID_ARG;
      }
    }

    return BF_SUCCESS;
  }

  template <class T>
  static bf_status_t fieldTypeCompatibilityCheck(const BfRtTableObj &table,
                                                 const T &field,
                                                 const uint64_t * /*value*/,
                                                 const uint8_t *value_ptr,
                                                 const size_t &size) {
    size_t field_size = field.getSize();

    if (!value_ptr) {
      // The field size in this case must be less than 64
      if (field_size > 64) {
        LOG_ERROR(
            "ERROR: %s:%d %s: Field size %zu for field with id %d is greater "
            "than 64. Hence please use the pointer variant of the API",
            __func__,
            __LINE__,
            table.table_name_get().c_str(),
            field_size,
            field.getId());
        return BF_INVALID_ARG;
      }
    } else {
      // Now the field_size must be equal to the size passed in
      if (size != (field_size + 7) / 8) {
        LOG_ERROR(
            "ERROR: %s:%d %s: Array size of %zu bytes isn't == size of the "
            "field %zu with id %d",
            __func__,
            __LINE__,
            table.table_name_get().c_str(),
            size,
            (field_size + 7) / 8,
            field.getId());
        return BF_INVALID_ARG;
      }
    }

    return BF_SUCCESS;
  }

  template <class T>
  static void toHostOrderData(const T &field,
                              const uint8_t *value_ptr,
                              uint64_t *out_data) {
    auto field_size = field.getSize();
    auto size = (field_size + 7) / 8;
    BfRtEndiannessHandler::toHostOrder(size, value_ptr, out_data);
  }

  template <class T>
  static void toNetworkOrderData(const T &field,
                                 const uint64_t &in_data,
                                 uint8_t *value_ptr) {
    auto field_size = field.getSize();
    auto size = (field_size + 7) / 8;
    BfRtEndiannessHandler::toNetworkOrder(size, in_data, value_ptr);
  }
};  // class BfRtTableFieldUtils

}  // namespace utils
}  // namespace bfrt

#endif  // _BF_RT_TABLE_FIELD_UTILS_HPP_

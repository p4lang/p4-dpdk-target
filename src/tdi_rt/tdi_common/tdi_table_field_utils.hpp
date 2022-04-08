/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) 2017-2021 Barefoot Networks, Inc.
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
#ifndef _TDI_TABLE_FIELD_UTILS_HPP_
#define _TDI_TABLE_FIELD_UTILS_HPP_

#include <tdi/common/tdi_utils.hpp>
#include <tdi/common/tdi_table.hpp>

namespace tdi {

namespace utils {

class TableFieldUtils {
 public:
  TableFieldUtils() = delete;

  static inline tdi_status_t keyFieldSafeGet(
      const tdi_id_t &field_id,
      const tdi::KeyFieldInfo **key_field,
      const tdi::KeyFieldValue *field_value,
      const tdi::Table *table) {
    *key_field = table->tableInfoGet()->keyFieldGet(field_id);
    if (!key_field) {
      LOG_ERROR("%s:%d %s Unable to find key for key field_id %d, ",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id);
      return TDI_OBJECT_NOT_FOUND;
    }

    // If the key_field type is not what is required here, then just return
    // error
    if ((*key_field)->matchTypeGet() != field_value->matchTypeGet()) {
      LOG_ERROR("%s:%d %s Incorrect key type provided for key field_id %d, ",
                //        "expected type: %s received type: %s",
                __func__,
                __LINE__,
                table->tableInfoGet()->nameGet().c_str(),
                field_id
                // KeyFieldTypeStr.at((*key_field)->dataTypeGet()),
                // KeyFieldTypeStr.at(key_field_type_req)
      );
      return TDI_INVALID_ARG;
    }
    if ((*key_field)->isPtrGet() && field_value->size_ < 64) {
      LOG_ERROR(
          "%s:%d Field size is greater than 64 bits. Please use byte arrays in "
          "the KeyFieldValue for field %d",
          __func__,
          __LINE__,
          field_id);
      return TDI_NOT_SUPPORTED;
    }
    return TDI_SUCCESS;
  }

  // Template classes must be defined whole in headers
  template <class T>
  static tdi_status_t boundsCheck(const tdi::Table &table,
                                  const T &field,
                                  const uint64_t &value,
                                  const uint8_t *value_ptr,
                                  const size_t &s) {
    size_t field_size = field.sizeGet();

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
                    table.tableInfoGet()->nameGet().c_str(),
                    value,
                    field.idGet());
          return TDI_INVALID_ARG;
        }
      }
    } else {
      if (s != (field_size + 7) / 8) {
        LOG_ERROR(
            "ERROR: %s:%d %s: Array size of %zu bytes isn't ==  size of the "
            "field %zu with id %d ",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            s,
            (field_size + 7) / 8,
            field.idGet());
        return TDI_INVALID_ARG;
      }
      int shift = field_size % 8;
      if (shift != 0 && (value_ptr[0] >> shift) > 0) {
        LOG_ERROR(
            "ERROR: %s:%d %s: Specified value is greater than what field size"
            " %zu bits allows for field id %d",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            field_size,
            field.idGet());
        return TDI_INVALID_ARG;
      }
    }

    return TDI_SUCCESS;
  }

  template <class T>
  static tdi_status_t fieldTypeCompatibilityCheck(const tdi::Table &table,
                                                  const T &field,
                                                  const uint64_t * /*value*/,
                                                  const uint8_t *value_ptr,
                                                  const size_t &size) {
    size_t field_size = field.sizeGet();

    if (!value_ptr) {
      // The field size in this case must be less than 64
      if (field_size > 64) {
        LOG_ERROR(
            "ERROR: %s:%d %s: Field size %zu for field with id %d is greater "
            "than 64. Hence please use the pointer variant of the API",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            field_size,
            field.idGet());
        return TDI_INVALID_ARG;
      }
    } else {
      // Now the field_size must be equal to the size passed in
      if (size != (field_size + 7) / 8) {
        LOG_ERROR(
            "ERROR: %s:%d %s: Array size of %zu bytes isn't == size of the "
            "field %zu with id %d",
            __func__,
            __LINE__,
            table.tableInfoGet()->nameGet().c_str(),
            size,
            (field_size + 7) / 8,
            field.idGet());
        return TDI_INVALID_ARG;
      }
    }

    return TDI_SUCCESS;
  }

  template <class T>
  static void toHostOrderData(const T &field,
                              const uint8_t *value_ptr,
                              uint64_t *out_data) {
    auto field_size = field.sizeGet();
    auto size = (field_size + 7) / 8;
    TdiEndiannessHandler::toHostOrder(size, value_ptr, out_data);
  }

  template <class T>
  static void toNetworkOrderData(const T &field,
                                 const uint64_t &in_data,
                                 uint8_t *value_ptr) {
    auto field_size = field.sizeGet();
    auto size = (field_size + 7) / 8;
    TdiEndiannessHandler::toNetworkOrder(size, in_data, value_ptr);
  }
};  // class TableFieldUtils

}  // namespace utils
}  // namespace tdi

#endif  // _TDI_TABLE_FIELD_UTILS_HPP_

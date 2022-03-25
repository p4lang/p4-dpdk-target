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

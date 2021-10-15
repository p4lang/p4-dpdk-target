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
#ifdef __cplusplus
extern "C" {
#endif

#include <bf_rt/bf_rt_table_data.h>

#ifdef __cplusplus
}
#endif

#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt_common/bf_rt_table_data_impl.hpp>

/* Data field setters/getter */
bf_status_t bf_rt_data_field_set_value(bf_rt_table_data_hdl *data_hdl,
                                       const bf_rt_id_t field_id,
                                       const uint64_t val) {
  auto data_field = reinterpret_cast<bfrt::BfRtTableData *>(data_hdl);
  return data_field->setValue(field_id, val);
}

bf_status_t bf_rt_data_field_set_float(bf_rt_table_data_hdl *data_hdl,
                                       const bf_rt_id_t field_id,
                                       const float val) {
  auto data_field = reinterpret_cast<bfrt::BfRtTableData *>(data_hdl);
  return data_field->setValue(field_id, val);
}

bf_status_t bf_rt_data_field_set_value_ptr(bf_rt_table_data_hdl *data_hdl,
                                           const bf_rt_id_t field_id,
                                           const uint8_t *val,
                                           const size_t s) {
  auto data_field = reinterpret_cast<bfrt::BfRtTableData *>(data_hdl);
  return data_field->setValue(field_id, val, s);
}

bf_status_t bf_rt_data_field_set_value_array(bf_rt_table_data_hdl *data_hdl,
                                             const bf_rt_id_t field_id,
                                             const uint32_t *val,
                                             const uint32_t num_array) {
  auto data_field = reinterpret_cast<bfrt::BfRtTableData *>(data_hdl);
  // array pointers work as iterators
  const auto vec = std::vector<bf_rt_id_t>(val, val + num_array);
  return data_field->setValue(field_id, vec);
}

bf_status_t bf_rt_data_field_set_value_bool_array(
    bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    const bool *val,
    const uint32_t num_array) {
  auto data_field = reinterpret_cast<bfrt::BfRtTableData *>(data_hdl);
  // array pointers work as iterators
  const auto vec = std::vector<bool>(val, val + num_array);
  return data_field->setValue(field_id, vec);
}

bf_status_t bf_rt_data_field_set_value_str_array(bf_rt_table_data_hdl *data_hdl,
                                                 const bf_rt_id_t field_id,
                                                 const char *val) {
  auto data_field = reinterpret_cast<bfrt::BfRtTableData *>(data_hdl);
  std::vector<std::string> vec;
  char *token = strtok(strdup(val), " ");
  while (token != NULL) {
    vec.push_back(std::string(token));
    token = strtok(NULL, " ");
  }
  return data_field->setValue(field_id, vec);
}

bf_status_t bf_rt_data_field_set_value_data_field_array(
    bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    bf_rt_table_data_hdl *val[],
    const uint32_t num_array) {
  std::vector<std::unique_ptr<bfrt::BfRtTableData>> inner_vec;
  for (uint32_t i = 0; i < num_array; i++) {
    auto inner_data = std::unique_ptr<bfrt::BfRtTableData>(
        reinterpret_cast<bfrt::BfRtTableData *>(val[i]));
    inner_vec.push_back(std::move(inner_data));
  }
  auto data_field = reinterpret_cast<bfrt::BfRtTableData *>(data_hdl);
  return data_field->setValue(field_id, std::move(inner_vec));
}

bf_status_t bf_rt_data_field_set_bool(bf_rt_table_data_hdl *data_hdl,
                                      const bf_rt_id_t field_id,
                                      const bool val) {
  auto data_field = reinterpret_cast<bfrt::BfRtTableData *>(data_hdl);
  return data_field->setValue(field_id, val);
}

bf_status_t bf_rt_data_field_set_string(bf_rt_table_data_hdl *data_hdl,
                                        const bf_rt_id_t field_id,
                                        const char *val) {
  auto data_field = reinterpret_cast<bfrt::BfRtTableData *>(data_hdl);
  const std::string str_val(val);
  return data_field->setValue(field_id, str_val);
}

bf_status_t bf_rt_data_field_get_value(const bf_rt_table_data_hdl *data_hdl,
                                       const bf_rt_id_t field_id,
                                       uint64_t *val) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  return data_field->getValue(field_id, val);
}

bf_status_t bf_rt_data_field_get_value_ptr(const bf_rt_table_data_hdl *data_hdl,
                                           const bf_rt_id_t field_id,
                                           const size_t size,
                                           uint8_t *val) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  return data_field->getValue(field_id, size, val);
}

bf_status_t bf_rt_data_field_get_value_array(
    const bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    uint32_t *val) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::vector<bf_rt_id_t> vec;
  auto status = data_field->getValue(field_id, &vec);
  int i = 0;
  for (auto const &item : vec) {
    val[i++] = item;
  }
  return status;
}

bf_status_t bf_rt_data_field_get_value_array_size(
    const bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    uint32_t *array_size) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::vector<bf_rt_id_t> vec;
  auto status = data_field->getValue(field_id, &vec);
  *array_size = vec.size();
  return status;
}

bf_status_t bf_rt_data_field_get_value_bool_array(
    const bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    bool *val) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::vector<bool> vec;
  auto status = data_field->getValue(field_id, &vec);
  int i = 0;
  for (auto const &item : vec) {
    val[i++] = item;
  }
  return status;
}

bf_status_t bf_rt_data_field_get_value_bool_array_size(
    const bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    uint32_t *array_size) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::vector<bool> vec;
  auto status = data_field->getValue(field_id, &vec);
  *array_size = vec.size();
  return status;
}

bf_status_t bf_rt_data_field_get_value_str_array(
    const bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    uint32_t size,
    char *val) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::vector<std::string> vec;
  auto status = data_field->getValue(field_id, &vec);
  if ((size == 0) && (vec.size() == 0)) return status;
  bool first_item = true;
  for (auto const &item : vec) {
    if (first_item) {
      first_item = false;
    } else {
      if (size == 0) return BF_INVALID_ARG;
      val[0] = ' ';
      val++;
      size--;
    }
    if (size <= item.size()) return BF_INVALID_ARG;
    item.copy(val, item.size());
    val += item.size();
    size -= item.size();
  }
  if (size == 0) return BF_INVALID_ARG;
  val[0] = '\0';
  return status;
}

bf_status_t bf_rt_data_field_get_value_str_array_size(
    const bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    uint32_t *array_size) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::vector<std::string> vec;
  auto status = data_field->getValue(field_id, &vec);
  /* Return the size of all strings plus white spaces between them. */
  for (auto const &item : vec) *array_size += (item.size() + 1);
  return status;
}

bf_status_t bf_rt_data_field_get_float(const bf_rt_table_data_hdl *data_hdl,
                                       const bf_rt_id_t field_id,
                                       float *val) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  return data_field->getValue(field_id, val);
}

bf_status_t bf_rt_data_field_get_bool(const bf_rt_table_data_hdl *data_hdl,
                                      const bf_rt_id_t field_id,
                                      bool *val) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  return data_field->getValue(field_id, val);
}

bf_status_t bf_rt_data_field_get_string_size(
    const bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    uint32_t *str_size) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::string str;
  auto status = data_field->getValue(field_id, &str);
  *str_size = str.size();
  return status;
}

bf_status_t bf_rt_data_field_get_string(const bf_rt_table_data_hdl *data_hdl,
                                        const bf_rt_id_t field_id,
                                        char *val) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::string str;
  auto status = data_field->getValue(field_id, &str);
  str.copy(val, str.size());
  return status;
}

bf_status_t bf_rt_data_field_get_value_u64_array(
    const bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    uint64_t *val) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::vector<uint64_t> vec;
  auto status = data_field->getValue(field_id, &vec);
  int i = 0;
  for (auto const &item : vec) {
    val[i++] = item;
  }
  return status;
}

bf_status_t bf_rt_data_field_get_value_u64_array_size(
    const bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    uint32_t *array_size) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::vector<uint64_t> vec;
  auto status = data_field->getValue(field_id, &vec);
  *array_size = vec.size();
  return status;
}

bf_status_t bf_rt_data_field_get_value_data_field_array(
    const bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    bf_rt_table_data_hdl **val) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::vector<bfrt::BfRtTableData *> vec;
  auto status = data_field->getValue(field_id, &vec);
  int i = 0;
  for (auto const &item : vec) {
    val[i++] = reinterpret_cast<bf_rt_table_data_hdl *>(item);
  }
  return status;
}

bf_status_t bf_rt_data_field_get_value_data_field_array_size(
    const bf_rt_table_data_hdl *data_hdl,
    const bf_rt_id_t field_id,
    uint32_t *array_size) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  std::vector<bfrt::BfRtTableData *> vec;
  auto status = data_field->getValue(field_id, &vec);
  *array_size = vec.size();
  return status;
}

bf_status_t bf_rt_data_action_id_get(const bf_rt_table_data_hdl *data_hdl,
                                     uint32_t *action_id) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  return data_field->actionIdGet(action_id);
}

bf_status_t bf_rt_data_field_is_active(const bf_rt_table_data_hdl *data_hdl,
                                       const bf_rt_id_t field_id,
                                       bool *is_active) {
  auto data_field = reinterpret_cast<const bfrt::BfRtTableData *>(data_hdl);
  return data_field->isActive(field_id, is_active);
}

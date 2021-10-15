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

#include <bf_rt/bf_rt_table.h>
#include <stdio.h>

#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_session.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_attributes.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>
#include <bf_rt/bf_rt_table_operations.hpp>
#include <bf_rt_common/bf_rt_pipe_mgr_intf.hpp>
#include <bf_rt_common/bf_rt_table_data_impl.hpp>
#include <bf_rt_common/bf_rt_table_impl.hpp>
#include <bf_rt_common/bf_rt_table_key_impl.hpp>
#include <bf_rt_common/bf_rt_utils.hpp>

namespace {
template <typename T>
bf_status_t key_field_allowed_choices_get_helper(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t &field_id,
    std::vector<std::reference_wrapper<const std::string>> *cpp_choices,
    T out_param) {
  if (out_param == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bf_status_t sts;
  sts = table->keyFieldAllowedChoicesGet(field_id, cpp_choices);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  return BF_SUCCESS;
}
template <typename T>
bf_status_t data_field_allowed_choices_get_helper(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t &field_id,
    std::vector<std::reference_wrapper<const std::string>> *cpp_choices,
    T out_param,
    const bf_rt_id_t &action_id = 0) {
  if (out_param == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bf_status_t sts;
  if (!action_id) {
    sts = table->dataFieldAllowedChoicesGet(field_id, cpp_choices);
  } else {
    sts = table->dataFieldAllowedChoicesGet(field_id, action_id, cpp_choices);
  }
  if (sts != BF_SUCCESS) {
    return sts;
  }
  return BF_SUCCESS;
}

bf_rt_annotation_t convert_annotation(const bfrt::Annotation &annotation) {
  bf_rt_annotation_t annotations_c;
  annotations_c.name = annotation.name_.c_str();
  annotations_c.value = annotation.value_.c_str();
  return annotations_c;
}

}  // anonymous namespace

bf_status_t bf_rt_table_id_from_handle_get(const bf_rt_table_hdl *table_hdl,
                                           bf_rt_id_t *id) {
  if (id == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  const bfrt::BfRtTable *table =
      reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  auto status = table->tableIdGet(id);
  return status;
}

bf_status_t bf_rt_table_name_get(const bf_rt_table_hdl *table_hdl,
                                 const char **table_name_ret) {
  if (table_name_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  // We need the internal function which returns a const string ref
  auto table = reinterpret_cast<const bfrt::BfRtTableObj *>(table_hdl);
  *table_name_ret = table->table_name_get().c_str();
  return BF_SUCCESS;
}

bool bf_rt_generic_flag_support(void) {
#ifdef BFRT_GENERIC_FLAGS
  return true;
#else
  return false;
#endif
}

#ifdef BFRT_GENERIC_FLAGS
bf_status_t bf_rt_table_entry_add(const bf_rt_table_hdl *table_hdl,
                                  const bf_rt_session_hdl *session,
                                  const bf_rt_target_t *dev_tgt,
                                  const uint64_t flags,
                                  const bf_rt_table_key_hdl *key,
                                  const bf_rt_table_data_hdl *data) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryAdd(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      *reinterpret_cast<const bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_entry_mod(const bf_rt_table_hdl *table_hdl,
                                  const bf_rt_session_hdl *session,
                                  const bf_rt_target_t *dev_tgt,
                                  const uint64_t flags,
                                  const bf_rt_table_key_hdl *key,
                                  const bf_rt_table_data_hdl *data) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryMod(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      *reinterpret_cast<const bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_entry_mod_inc(const bf_rt_table_hdl *table_hdl,
                                      const bf_rt_session_hdl *session,
                                      const bf_rt_target_t *dev_tgt,
                                      const uint64_t flags,
                                      const bf_rt_table_key_hdl *key,
                                      const bf_rt_table_data_hdl *data) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryModInc(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      *reinterpret_cast<const bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_entry_del(const bf_rt_table_hdl *table_hdl,
                                  const bf_rt_session_hdl *session,
                                  const bf_rt_target_t *dev_tgt,
                                  const uint64_t flags,
                                  const bf_rt_table_key_hdl *key) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryDel(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key));
}

bf_status_t bf_rt_table_clear(const bf_rt_table_hdl *table_hdl,
                              const bf_rt_session_hdl *session,
                              const bf_rt_target_t *dev_tgt,
                              const uint64_t flags) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableClear(
      *reinterpret_cast<const bfrt::BfRtSession *>(session), *dev_tgt, flags);
}

bf_status_t bf_rt_table_entry_get(const bf_rt_table_hdl *table_hdl,
                                  const bf_rt_session_hdl *session,
                                  const bf_rt_target_t *dev_tgt,
                                  const uint64_t flags,
                                  const bf_rt_table_key_hdl *key,
                                  bf_rt_table_data_hdl *data) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      reinterpret_cast<bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_entry_get_by_handle(const bf_rt_table_hdl *table_hdl,
                                            const bf_rt_session_hdl *session,
                                            const bf_rt_target_t *dev_tgt,
                                            const uint64_t flags,
                                            const uint32_t entry_handle,
                                            bf_rt_table_key_hdl *key,
                                            bf_rt_table_data_hdl *data) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      static_cast<bf_rt_handle_t>(entry_handle),
      reinterpret_cast<bfrt::BfRtTableKey *>(key),
      reinterpret_cast<bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_entry_key_get(const bf_rt_table_hdl *table_hdl,
                                      const bf_rt_session_hdl *session,
                                      const bf_rt_target_t *dev_tgt_in,
                                      const uint64_t flags,
                                      const uint32_t entry_handle,
                                      bf_rt_target_t *dev_tgt_out,
                                      bf_rt_table_key_hdl *key) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryKeyGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt_in,
      flags,
      static_cast<bf_rt_handle_t>(entry_handle),
      dev_tgt_out,
      reinterpret_cast<bfrt::BfRtTableKey *>(key));
}

bf_status_t bf_rt_table_entry_handle_get(const bf_rt_table_hdl *table_hdl,
                                         const bf_rt_session_hdl *session,
                                         const bf_rt_target_t *dev_tgt,
                                         const uint64_t flags,
                                         const bf_rt_table_key_hdl *key,
                                         uint32_t *entry_handle) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryHandleGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      entry_handle);
}

bf_status_t bf_rt_table_entry_get_first(const bf_rt_table_hdl *table_hdl,
                                        const bf_rt_session_hdl *session,
                                        const bf_rt_target_t *dev_tgt,
                                        const uint64_t flags,
                                        bf_rt_table_key_hdl *key,
                                        bf_rt_table_data_hdl *data) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryGetFirst(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      reinterpret_cast<bfrt::BfRtTableKey *>(key),
      reinterpret_cast<bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_entry_get_next_n(const bf_rt_table_hdl *table_hdl,
                                         const bf_rt_session_hdl *session,
                                         const bf_rt_target_t *dev_tgt,
                                         const uint64_t flags,
                                         const bf_rt_table_key_hdl *key,
                                         bf_rt_table_key_hdl **output_keys,
                                         bf_rt_table_data_hdl **output_data,
                                         uint32_t n,
                                         uint32_t *num_returned) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bfrt::BfRtTable::keyDataPairs key_data_pairs;
  unsigned i = 0;
  for (i = 0; i < n; i++) {
    key_data_pairs.push_back(std::make_pair(
        reinterpret_cast<bfrt::BfRtTableKey *>(output_keys[i]),
        reinterpret_cast<bfrt::BfRtTableData *>(output_data[i])));
  }

  return table->tableEntryGetNext_n(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      n,
      &key_data_pairs,
      num_returned);
}

bf_status_t bf_rt_table_usage_get(const bf_rt_table_hdl *table_hdl,
                                  const bf_rt_session_hdl *session,
                                  const bf_rt_target_t *dev_tgt,
                                  const uint64_t flags,
                                  uint32_t *count) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableUsageGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      count);
}

bf_status_t bf_rt_table_default_entry_set(const bf_rt_table_hdl *table_hdl,
                                          const bf_rt_session_hdl *session,
                                          const bf_rt_target_t *dev_tgt,
                                          const uint64_t flags,
                                          const bf_rt_table_data_hdl *data) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableDefaultEntrySet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      *reinterpret_cast<const bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_default_entry_get(const bf_rt_table_hdl *table_hdl,
                                          const bf_rt_session_hdl *session,
                                          const bf_rt_target_t *dev_tgt,
                                          const uint64_t flags,
                                          bf_rt_table_data_hdl *data) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableDefaultEntryGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      reinterpret_cast<bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_default_entry_reset(const bf_rt_table_hdl *table_hdl,
                                            const bf_rt_session_hdl *session,
                                            const bf_rt_target_t *dev_tgt,
                                            const uint64_t flags) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableDefaultEntryReset(
      *reinterpret_cast<const bfrt::BfRtSession *>(session), *dev_tgt, flags);
}

bf_status_t bf_rt_table_size_get(const bf_rt_table_hdl *table_hdl,
                                 const bf_rt_session_hdl *session,
                                 const bf_rt_target_t *dev_tgt,
                                 const uint64_t flags,
                                 size_t *count) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableSizeGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      count);
}
#else
bf_status_t bf_rt_table_entry_add(const bf_rt_table_hdl *table_hdl,
                                  const bf_rt_session_hdl *session,
                                  const bf_rt_target_t *dev_tgt,
                                  const bf_rt_table_key_hdl *key,
                                  const bf_rt_table_data_hdl *data) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryAdd(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      *reinterpret_cast<const bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_entry_mod(const bf_rt_table_hdl *table_hdl,
                                  const bf_rt_session_hdl *session,
                                  const bf_rt_target_t *dev_tgt,
                                  const bf_rt_table_key_hdl *key,
                                  const bf_rt_table_data_hdl *data) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryMod(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      *reinterpret_cast<const bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_entry_mod_inc(const bf_rt_table_hdl *table_hdl,
                                      const bf_rt_session_hdl *session,
                                      const bf_rt_target_t *dev_tgt,
                                      const bf_rt_table_key_hdl *key,
                                      const bf_rt_table_data_hdl *data,
                                      const bf_rt_entry_mod_inc_flag_e flag) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  const bfrt::BfRtTable::BfRtTableModIncFlag mod_inc_flag =
      (flag == MOD_INC_ADD)
          ? bfrt::BfRtTable::BfRtTableModIncFlag::MOD_INC_ADD
          : bfrt::BfRtTable::BfRtTableModIncFlag::MOD_INC_DELETE;

  return table->tableEntryModInc(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      *reinterpret_cast<const bfrt::BfRtTableData *>(data),
      mod_inc_flag);
}

bf_status_t bf_rt_table_entry_del(const bf_rt_table_hdl *table_hdl,
                                  const bf_rt_session_hdl *session,
                                  const bf_rt_target_t *dev_tgt,
                                  const bf_rt_table_key_hdl *key) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryDel(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key));
}

bf_status_t bf_rt_table_clear(const bf_rt_table_hdl *table_hdl,
                              const bf_rt_session_hdl *session,
                              const bf_rt_target_t *dev_tgt) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableClear(
      *reinterpret_cast<const bfrt::BfRtSession *>(session), *dev_tgt);
}

bf_status_t bf_rt_table_entry_get(const bf_rt_table_hdl *table_hdl,
                                  const bf_rt_session_hdl *session,
                                  const bf_rt_target_t *dev_tgt,
                                  const bf_rt_table_key_hdl *key,
                                  bf_rt_table_data_hdl *data,
                                  bf_rt_entry_read_flag_e flag) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bfrt::BfRtTable::BfRtTableGetFlag read_flag;

  if (flag == ENTRY_READ_FROM_SW) {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;
  } else {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;
  }

  return table->tableEntryGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      read_flag,
      reinterpret_cast<bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_entry_get_by_handle(const bf_rt_table_hdl *table_hdl,
                                            const bf_rt_session_hdl *session,
                                            const bf_rt_target_t *dev_tgt,
                                            const uint32_t entry_handle,
                                            bf_rt_table_key_hdl *key,
                                            bf_rt_table_data_hdl *data,
                                            bf_rt_entry_read_flag_e flag) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bfrt::BfRtTable::BfRtTableGetFlag read_flag;

  if (flag == ENTRY_READ_FROM_SW) {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;
  } else {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;
  }

  return table->tableEntryGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      read_flag,
      static_cast<bf_rt_handle_t>(entry_handle),
      reinterpret_cast<bfrt::BfRtTableKey *>(key),
      reinterpret_cast<bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_entry_key_get(const bf_rt_table_hdl *table_hdl,
                                      const bf_rt_session_hdl *session,
                                      const bf_rt_target_t *dev_tgt_in,
                                      const uint32_t entry_handle,
                                      bf_rt_target_t *dev_tgt_out,
                                      bf_rt_table_key_hdl *key) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryKeyGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt_in,
      static_cast<bf_rt_handle_t>(entry_handle),
      dev_tgt_out,
      reinterpret_cast<bfrt::BfRtTableKey *>(key));
}

bf_status_t bf_rt_table_entry_handle_get(const bf_rt_table_hdl *table_hdl,
                                         const bf_rt_session_hdl *session,
                                         const bf_rt_target_t *dev_tgt,
                                         const bf_rt_table_key_hdl *key,
                                         uint32_t *entry_handle) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableEntryHandleGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      entry_handle);
}

bf_status_t bf_rt_table_entry_get_first(const bf_rt_table_hdl *table_hdl,
                                        const bf_rt_session_hdl *session,
                                        const bf_rt_target_t *dev_tgt,
                                        bf_rt_table_key_hdl *key,
                                        bf_rt_table_data_hdl *data,
                                        bf_rt_entry_read_flag_e flag) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bfrt::BfRtTable::BfRtTableGetFlag read_flag;

  if (flag == ENTRY_READ_FROM_SW) {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;
  } else {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;
  }

  return table->tableEntryGetFirst(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      read_flag,
      reinterpret_cast<bfrt::BfRtTableKey *>(key),
      reinterpret_cast<bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_entry_get_next_n(const bf_rt_table_hdl *table_hdl,
                                         const bf_rt_session_hdl *session,
                                         const bf_rt_target_t *dev_tgt,
                                         const bf_rt_table_key_hdl *key,
                                         bf_rt_table_key_hdl **output_keys,
                                         bf_rt_table_data_hdl **output_data,
                                         uint32_t n,
                                         uint32_t *num_returned,
                                         bf_rt_entry_read_flag_e flag) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bfrt::BfRtTable::BfRtTableGetFlag read_flag;

  if (flag == ENTRY_READ_FROM_SW) {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;
  } else {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;
  }
  bfrt::BfRtTable::keyDataPairs key_data_pairs;
  unsigned i = 0;
  for (i = 0; i < n; i++) {
    key_data_pairs.push_back(std::make_pair(
        reinterpret_cast<bfrt::BfRtTableKey *>(output_keys[i]),
        reinterpret_cast<bfrt::BfRtTableData *>(output_data[i])));
  }

  return table->tableEntryGetNext_n(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      *reinterpret_cast<const bfrt::BfRtTableKey *>(key),
      n,
      read_flag,
      &key_data_pairs,
      num_returned);
}

bf_status_t bf_rt_table_usage_get(const bf_rt_table_hdl *table_hdl,
                                  const bf_rt_session_hdl *session,
                                  const bf_rt_target_t *dev_tgt,
                                  uint32_t *count,
                                  bf_rt_entry_read_flag_e flag) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bfrt::BfRtTable::BfRtTableGetFlag read_flag;

  if (flag == ENTRY_READ_FROM_SW) {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;
  } else {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;
  }

  return table->tableUsageGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      read_flag,
      count);
}

bf_status_t bf_rt_table_default_entry_set(const bf_rt_table_hdl *table_hdl,
                                          const bf_rt_session_hdl *session,
                                          const bf_rt_target_t *dev_tgt,
                                          const bf_rt_table_data_hdl *data) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableDefaultEntrySet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      *reinterpret_cast<const bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_default_entry_get(const bf_rt_table_hdl *table_hdl,
                                          const bf_rt_session_hdl *session,
                                          const bf_rt_target_t *dev_tgt,
                                          bf_rt_table_data_hdl *data,
                                          const bf_rt_entry_read_flag_e flag) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bfrt::BfRtTable::BfRtTableGetFlag read_flag;

  if (flag == ENTRY_READ_FROM_SW) {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;
  } else {
    read_flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;
  }

  return table->tableDefaultEntryGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      read_flag,
      reinterpret_cast<bfrt::BfRtTableData *>(data));
}

bf_status_t bf_rt_table_default_entry_reset(const bf_rt_table_hdl *table_hdl,
                                            const bf_rt_session_hdl *session,
                                            const bf_rt_target_t *dev_tgt) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableDefaultEntryReset(
      *reinterpret_cast<const bfrt::BfRtSession *>(session), *dev_tgt);
}

bf_status_t bf_rt_table_size_get(const bf_rt_table_hdl *table_hdl,
                                 const bf_rt_session_hdl *session,
                                 const bf_rt_target_t *dev_tgt,
                                 size_t *count) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableSizeGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session), *dev_tgt, count);
}
#endif

bf_status_t bf_rt_table_type_get(const bf_rt_table_hdl *table_hdl,
                                 bf_rt_table_type_t *table_type) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableTypeGet(
      reinterpret_cast<bfrt::BfRtTable::TableType *>(table_type));
}

bf_status_t bf_rt_table_has_const_default_action(
    const bf_rt_table_hdl *table_hdl, bool *has_const_default_action) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableHasConstDefaultAction(has_const_default_action);
}

bf_status_t bf_rt_table_num_annotations_get(const bf_rt_table_hdl *table_hdl,
                                            uint32_t *num_annotations) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bfrt::AnnotationSet cpp_annotations;
  bf_status_t sts = table->tableAnnotationsGet(&cpp_annotations);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  *num_annotations = cpp_annotations.size();
  return BF_SUCCESS;
}

bf_status_t bf_rt_table_annotations_get(const bf_rt_table_hdl *table_hdl,
                                        bf_rt_annotation_t *annotations_c) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bfrt::AnnotationSet cpp_annotations;
  bf_status_t sts = table->tableAnnotationsGet(&cpp_annotations);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  int i = 0;
  for (const auto &annotation : cpp_annotations) {
    annotations_c[i++] = convert_annotation(annotation.get());
  }
  return BF_SUCCESS;
}

bf_status_t bf_rt_action_id_from_data_get(const bf_rt_table_data_hdl *data,
                                          bf_rt_id_t *id_ret) {
  auto data_obj = reinterpret_cast<const bfrt::BfRtTableData *>(data);
  return data_obj->actionIdGet(id_ret);
}

// Allocate APIs
bf_status_t bf_rt_table_key_allocate(const bf_rt_table_hdl *table_hdl,
                                     bf_rt_table_key_hdl **key_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableKey> key_hdl;
  auto status = table->keyAllocate(&key_hdl);
  *key_hdl_ret = reinterpret_cast<bf_rt_table_key_hdl *>(key_hdl.release());
  return status;
}

bf_status_t bf_rt_table_data_allocate(const bf_rt_table_hdl *table_hdl,
                                      bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableData> data_hdl;
  auto status = table->dataAllocate(&data_hdl);
  *data_hdl_ret = reinterpret_cast<bf_rt_table_data_hdl *>(data_hdl.release());

  return status;
}

bf_status_t bf_rt_table_action_data_allocate(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t action_id,
    bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableData> data_hdl;
  auto status = table->dataAllocate(action_id, &data_hdl);
  *data_hdl_ret = reinterpret_cast<bf_rt_table_data_hdl *>(data_hdl.release());
  return status;
}

bf_status_t bf_rt_table_data_allocate_with_fields(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t *fields,
    const uint32_t num_array,
    bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableData> data_hdl;
  const auto vec = std::vector<bf_rt_id_t>(fields, fields + num_array);

  auto status = table->dataAllocate(vec, &data_hdl);
  *data_hdl_ret = reinterpret_cast<bf_rt_table_data_hdl *>(data_hdl.release());

  return status;
}

bf_status_t bf_rt_table_action_data_allocate_with_fields(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t action_id,
    const bf_rt_id_t *fields,
    const uint32_t num_array,
    bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableData> data_hdl;
  const auto vec = std::vector<bf_rt_id_t>(fields, fields + num_array);

  auto status = table->dataAllocate(vec, action_id, &data_hdl);
  *data_hdl_ret = reinterpret_cast<bf_rt_table_data_hdl *>(data_hdl.release());

  return status;
}
bf_status_t bf_rt_table_data_allocate_container(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t c_field_id,
    bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableData> data_hdl;
  auto status = table->dataAllocateContainer(c_field_id, &data_hdl);
  *data_hdl_ret = reinterpret_cast<bf_rt_table_data_hdl *>(data_hdl.release());
  return status;
}
bf_status_t bf_rt_table_action_data_allocate_container(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t c_field_id,
    const bf_rt_id_t action_id,
    bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableData> data_hdl;
  auto status = table->dataAllocateContainer(c_field_id, action_id, &data_hdl);
  *data_hdl_ret = reinterpret_cast<bf_rt_table_data_hdl *>(data_hdl.release());
  return status;
}
bf_status_t bf_rt_table_data_allocate_container_with_fields(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t c_field_id,
    const bf_rt_id_t *fields,
    const uint32_t num_array,
    bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableData> data_hdl;
  const auto vec = std::vector<bf_rt_id_t>(fields, fields + num_array);

  auto status = table->dataAllocateContainer(c_field_id, vec, &data_hdl);
  *data_hdl_ret = reinterpret_cast<bf_rt_table_data_hdl *>(data_hdl.release());

  return status;
}
bf_status_t bf_rt_table_action_data_allocate_container_with_fields(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t c_field_id,
    const bf_rt_id_t action_id,
    const bf_rt_id_t *fields,
    const uint32_t num_array,
    bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableData> data_hdl;
  const auto vec = std::vector<bf_rt_id_t>(fields, fields + num_array);

  auto status =
      table->dataAllocateContainer(c_field_id, vec, action_id, &data_hdl);
  *data_hdl_ret = reinterpret_cast<bf_rt_table_data_hdl *>(data_hdl.release());

  return status;
}

bf_status_t bf_rt_table_entry_scope_attributes_allocate(
    const bf_rt_table_hdl *table_hdl, bf_rt_table_attributes_hdl **tbl_attr) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableAttributes> attr;
  auto status =
      table->attributeAllocate(bfrt::TableAttributesType::ENTRY_SCOPE, &attr);
  *tbl_attr = reinterpret_cast<bf_rt_table_attributes_hdl *>(attr.release());
  return status;
}

bf_status_t bf_rt_table_idle_table_attributes_allocate(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_attributes_idle_table_mode_t idle_mode,
    bf_rt_table_attributes_hdl **tbl_attr) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableAttributes> attr;
  auto status = table->attributeAllocate(
      bfrt::TableAttributesType::IDLE_TABLE_RUNTIME,
      static_cast<bfrt::TableAttributesIdleTableMode>(idle_mode),
      &attr);
  *tbl_attr = reinterpret_cast<bf_rt_table_attributes_hdl *>(attr.release());
  return status;
}

bf_status_t bf_rt_table_port_status_notif_attributes_allocate(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_table_attributes_hdl **tbl_attr_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableAttributes> attr;
  auto status = table->attributeAllocate(
      bfrt::TableAttributesType::PORT_STATUS_NOTIF, &attr);
  *tbl_attr_hdl_ret =
      reinterpret_cast<bf_rt_table_attributes_hdl *>(attr.release());
  return status;
}

bf_status_t bf_rt_table_port_stats_poll_intv_attributes_allocate(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_table_attributes_hdl **tbl_attr_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableAttributes> attr;
  auto status = table->attributeAllocate(
      bfrt::TableAttributesType::PORT_STAT_POLL_INTVL_MS, &attr);
  *tbl_attr_hdl_ret =
      reinterpret_cast<bf_rt_table_attributes_hdl *>(attr.release());
  return status;
}
bf_status_t bf_rt_table_dyn_hashing_attributes_allocate(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_table_attributes_hdl **tbl_attr_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableAttributes> attr;
  auto status = table->attributeAllocate(
      bfrt::TableAttributesType::DYNAMIC_HASH_ALG_SEED, &attr);
  *tbl_attr_hdl_ret =
      reinterpret_cast<bf_rt_table_attributes_hdl *>(attr.release());
  return status;
}
bf_status_t bf_rt_table_meter_byte_count_adjust_attributes_allocate(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_table_attributes_hdl **tbl_attr_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableAttributes> attr;
  auto status = table->attributeAllocate(
      bfrt::TableAttributesType::METER_BYTE_COUNT_ADJ, &attr);
  *tbl_attr_hdl_ret =
      reinterpret_cast<bf_rt_table_attributes_hdl *>(attr.release());
  return status;
}
bf_status_t bf_rt_table_selector_table_update_cb_attributes_allocate(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_table_attributes_hdl **tbl_attr_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableAttributes> attr;
  auto status = table->attributeAllocate(
      bfrt::TableAttributesType::SELECTOR_UPDATE_CALLBACK, &attr);
  *tbl_attr_hdl_ret =
      reinterpret_cast<bf_rt_table_attributes_hdl *>(attr.release());
  return status;
}

bf_status_t bf_rt_table_operations_allocate(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_table_operations_mode_t op_type,
    bf_rt_table_operations_hdl **tbl_ops) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableOperations> ops;
  auto status = table->operationsAllocate(
      static_cast<bfrt::TableOperationsType>(op_type), &ops);
  *tbl_ops = reinterpret_cast<bf_rt_table_operations_hdl *>(ops.release());
  return status;
}

// Reset APIs
bf_status_t bf_rt_table_key_reset(const bf_rt_table_hdl *table_hdl,
                                  bf_rt_table_key_hdl **key_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->keyReset(reinterpret_cast<bfrt::BfRtTableKey *>(*key_hdl_ret));
}

bf_status_t bf_rt_table_data_reset(const bf_rt_table_hdl *table_hdl,
                                   bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataReset(
      reinterpret_cast<bfrt::BfRtTableData *>(*data_hdl_ret));
}

bf_status_t bf_rt_table_action_data_reset(const bf_rt_table_hdl *table_hdl,
                                          const bf_rt_id_t action_id,
                                          bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataReset(
      action_id, reinterpret_cast<bfrt::BfRtTableData *>(*data_hdl_ret));
}

bf_status_t bf_rt_table_data_reset_with_fields(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t *fields,
    const uint32_t num_array,
    bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  const auto vec = std::vector<bf_rt_id_t>(fields, fields + num_array);

  return table->dataReset(
      vec, reinterpret_cast<bfrt::BfRtTableData *>(*data_hdl_ret));
}

bf_status_t bf_rt_table_action_data_reset_with_fields(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t action_id,
    const bf_rt_id_t *fields,
    const uint32_t num_array,
    bf_rt_table_data_hdl **data_hdl_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::unique_ptr<bfrt::BfRtTableData> data_hdl;
  const auto vec = std::vector<bf_rt_id_t>(fields, fields + num_array);

  return table->dataReset(
      vec, action_id, reinterpret_cast<bfrt::BfRtTableData *>(*data_hdl_ret));
}

// De-allocate APIs
bf_status_t bf_rt_table_key_deallocate(bf_rt_table_key_hdl *key_hdl) {
  auto key = reinterpret_cast<bfrt::BfRtTableKey *>(key_hdl);
  if (key == nullptr) {
    LOG_ERROR("%s:%d null param passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  delete key;
  return BF_SUCCESS;
}

bf_status_t bf_rt_table_data_deallocate(bf_rt_table_data_hdl *data_hdl) {
  auto data = reinterpret_cast<bfrt::BfRtTableData *>(data_hdl);
  if (data == nullptr) {
    LOG_ERROR("%s:%d null param passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  delete data;
  return BF_SUCCESS;
}

bf_status_t bf_rt_table_attributes_deallocate(
    bf_rt_table_attributes_hdl *tbl_attr_hdl) {
  auto tbl_attr = reinterpret_cast<bfrt::BfRtTableAttributes *>(tbl_attr_hdl);
  if (tbl_attr == nullptr) {
    LOG_ERROR("%s:%d null param passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  delete tbl_attr;
  return BF_SUCCESS;
}

bf_status_t bf_rt_table_operations_deallocate(
    bf_rt_table_operations_hdl *tbl_op_hdl) {
  auto tbl_op = reinterpret_cast<bfrt::BfRtTableOperations *>(tbl_op_hdl);
  if (tbl_op == nullptr) {
    LOG_ERROR("%s:%d null param passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  delete tbl_op;
  return BF_SUCCESS;
}

// KeyField APIs
bf_status_t bf_rt_key_field_id_list_size_get(const bf_rt_table_hdl *table_hdl,
                                             uint32_t *num) {
  // Only here are we using BfRtTableObj rather than BfRtTable
  // since we need the hidden impl function
  auto table = reinterpret_cast<const bfrt::BfRtTableObj *>(table_hdl);
  *num = table->keyFieldListSize();
  return BF_SUCCESS;
}

bf_status_t bf_rt_key_field_id_list_get(const bf_rt_table_hdl *table_hdl,
                                        bf_rt_id_t *id_vec_ret) {
  if (id_vec_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::vector<bf_rt_id_t> temp_vec;
  auto status = table->keyFieldIdListGet(&temp_vec);

  if (status == BF_SUCCESS) {
    for (auto it = temp_vec.begin(); it != temp_vec.end(); ++it) {
      bf_rt_id_t field_id = *it;
      id_vec_ret[it - temp_vec.begin()] = field_id;
    }
  }

  return status;
}

bf_status_t bf_rt_key_field_type_get(const bf_rt_table_hdl *table_hdl,
                                     const bf_rt_id_t field_id,
                                     bf_rt_key_field_type_t *field_type_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);

  return table->keyFieldTypeGet(
      field_id, reinterpret_cast<bfrt::KeyFieldType *>(field_type_ret));
}

bf_status_t bf_rt_key_field_data_type_get(const bf_rt_table_hdl *table_hdl,
                                          const bf_rt_id_t field_id,
                                          bf_rt_data_type_t *field_type_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);

  return table->keyFieldDataTypeGet(
      field_id, reinterpret_cast<bfrt::DataType *>(field_type_ret));
}

bf_status_t bf_rt_key_field_id_get(const bf_rt_table_hdl *table_hdl,
                                   const char *key_field_name,
                                   bf_rt_id_t *field_id) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->keyFieldIdGet(key_field_name, field_id);
}

bf_status_t bf_rt_key_field_size_get(const bf_rt_table_hdl *table_hdl,
                                     const bf_rt_id_t field_id,
                                     size_t *field_size_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->keyFieldSizeGet(field_id, field_size_ret);
}

bf_status_t bf_rt_key_field_is_ptr_get(const bf_rt_table_hdl *table_hdl,
                                       const bf_rt_id_t field_id,
                                       bool *is_ptr_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->keyFieldIsPtrGet(field_id, is_ptr_ret);
}

bf_status_t bf_rt_key_field_name_get(const bf_rt_table_hdl *table_hdl,
                                     const bf_rt_id_t field_id,
                                     const char **name_ret) {
  if (name_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  auto table = reinterpret_cast<const bfrt::BfRtTableObj *>(table_hdl);
  *name_ret = table->key_field_name_get(field_id).c_str();
  return BF_SUCCESS;
}

bf_status_t bf_rt_key_field_num_allowed_choices_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    uint32_t *num_choices) {
  std::vector<std::reference_wrapper<const std::string>> cpp_choices;
  bf_status_t sts = ::key_field_allowed_choices_get_helper<uint32_t *>(
      table_hdl, field_id, &cpp_choices, num_choices);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  *num_choices = cpp_choices.size();
  return BF_SUCCESS;
}

bf_status_t bf_rt_key_field_allowed_choices_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const char *choices[]) {
  std::vector<std::reference_wrapper<const std::string>> cpp_choices;
  bf_status_t sts = ::key_field_allowed_choices_get_helper<const char *[]>(
      table_hdl, field_id, &cpp_choices, choices);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  for (auto iter = cpp_choices.begin(); iter != cpp_choices.end(); iter++) {
    choices[iter - cpp_choices.begin()] = iter->get().c_str();
  }
  return BF_SUCCESS;
}

// DataField APIs
bf_status_t bf_rt_data_field_id_list_size_get(const bf_rt_table_hdl *table_hdl,
                                              uint32_t *num) {
  if (num == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  // Only here are we using BfRtTableObj rather than BfRtTable
  // since we need the hidden impl function
  auto table = reinterpret_cast<const bfrt::BfRtTableObj *>(table_hdl);
  *num = table->dataFieldListSize();
  return BF_SUCCESS;
}

bf_status_t bf_rt_data_field_id_list_size_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t action_id,
    uint32_t *num) {
  // Only here are we using BfRtTableObj rather than BfRtTable
  // since we need the hidden impl function
  auto table = reinterpret_cast<const bfrt::BfRtTableObj *>(table_hdl);
  *num = table->dataFieldListSize(action_id);
  return BF_SUCCESS;
}

bf_status_t bf_rt_data_field_list_get(const bf_rt_table_hdl *table_hdl,
                                      bf_rt_id_t *id_vec_ret) {
  if (id_vec_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::vector<bf_rt_id_t> field_ids;
  bf_status_t sts = table->dataFieldIdListGet(&field_ids);
  if (sts == BF_SUCCESS) {
    for (auto it = field_ids.begin(); it != field_ids.end(); ++it) {
      bf_rt_id_t field_id = *it;
      id_vec_ret[it - field_ids.begin()] = field_id;
    }
  }

  return sts;
}

bf_status_t bf_rt_container_data_field_list_get(
    const bf_rt_table_hdl *table_hdl,
    bf_rt_id_t container_field_id,
    bf_rt_id_t *id_vec_ret) {
  if (id_vec_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::vector<bf_rt_id_t> field_ids;
  bf_status_t sts =
      table->containerDataFieldIdListGet(container_field_id, &field_ids);
  if (sts == BF_SUCCESS) {
    for (auto it = field_ids.begin(); it != field_ids.end(); ++it) {
      bf_rt_id_t field_id = *it;
      id_vec_ret[it - field_ids.begin()] = field_id;
    }
  }

  return sts;
}

bf_status_t bf_rt_container_data_field_list_size_get(
    const bf_rt_table_hdl *table_hdl,
    bf_rt_id_t container_field_id,
    size_t *field_list_size_ret) {
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  *field_list_size_ret = 0;
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::vector<bf_rt_id_t> field_ids;
  bf_status_t sts =
      table->containerDataFieldIdListGet(container_field_id, &field_ids);
  if (sts == BF_SUCCESS) {
    *field_list_size_ret = field_ids.size();
  }

  return sts;
}

bf_status_t bf_rt_data_field_list_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t action_id,
    bf_rt_id_t *id_vec_ret) {
  if (id_vec_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::vector<bf_rt_id_t> field_ids;
  bf_status_t sts = table->dataFieldIdListGet(action_id, &field_ids);
  if (sts == BF_SUCCESS) {
    for (auto it = field_ids.begin(); it != field_ids.end(); ++it) {
      bf_rt_id_t field_id = *it;
      id_vec_ret[it - field_ids.begin()] = field_id;
    }
  }

  return sts;
}

bf_status_t bf_rt_data_field_id_get(const bf_rt_table_hdl *table_hdl,
                                    const char *data_field_name,
                                    bf_rt_id_t *field_id_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataFieldIdGet(data_field_name, field_id_ret);
}
bf_status_t bf_rt_data_field_id_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const char *data_field_name,
    const bf_rt_id_t action_id,
    bf_rt_id_t *field_id_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataFieldIdGet(data_field_name, action_id, field_id_ret);
}

bf_status_t bf_rt_data_field_size_get(const bf_rt_table_hdl *table_hdl,
                                      const bf_rt_id_t field_id,
                                      size_t *field_size_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataFieldSizeGet(field_id, field_size_ret);
}
bf_status_t bf_rt_data_field_size_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    size_t *field_size_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataFieldSizeGet(field_id, action_id, field_size_ret);
}

bf_status_t bf_rt_data_field_is_ptr_get(const bf_rt_table_hdl *table_hdl,
                                        const bf_rt_id_t field_id,
                                        bool *is_ptr_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataFieldIsPtrGet(field_id, is_ptr_ret);
}
bf_status_t bf_rt_data_field_is_ptr_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    bool *is_ptr_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataFieldIsPtrGet(field_id, action_id, is_ptr_ret);
}

bf_status_t bf_rt_data_field_is_mandatory_get(const bf_rt_table_hdl *table_hdl,
                                              const bf_rt_id_t field_id,
                                              bool *is_mandatory_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataFieldMandatoryGet(field_id, is_mandatory_ret);
}
bf_status_t bf_rt_data_field_is_mandatory_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    bool *is_mandatory_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataFieldMandatoryGet(field_id, action_id, is_mandatory_ret);
}

bf_status_t bf_rt_data_field_is_read_only_get(const bf_rt_table_hdl *table_hdl,
                                              const bf_rt_id_t field_id,
                                              bool *is_read_only_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataFieldReadOnlyGet(field_id, is_read_only_ret);
}
bf_status_t bf_rt_data_field_is_read_only_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    bool *is_read_only_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->dataFieldReadOnlyGet(field_id, action_id, is_read_only_ret);
}

bf_status_t bf_rt_data_field_name_get(const bf_rt_table_hdl *table_hdl,
                                      const bf_rt_id_t field_id,
                                      const char **name_ret) {
  if (name_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  auto table = reinterpret_cast<const bfrt::BfRtTableObj *>(table_hdl);
  *name_ret = table->data_field_name_get(field_id).c_str();
  return BF_SUCCESS;
}

bf_status_t bf_rt_data_field_name_copy_get(const bf_rt_table_hdl *table_hdl,
                                           const bf_rt_id_t field_id,
                                           const uint32_t buf_sz,
                                           char *name_ret) {
  if (name_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  auto table = reinterpret_cast<const bfrt::BfRtTableObj *>(table_hdl);
  std::string fname;
  auto status = table->dataFieldNameGet(field_id, &fname);
  if (status) return status;

  if (fname.size() >= buf_sz) {
    LOG_ERROR("%s:%d Provided buffer too small", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  std::strncpy(name_ret, fname.c_str(), buf_sz);

  return status;
}

bf_status_t bf_rt_data_field_name_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    const char **name_ret) {
  if (name_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  auto table = reinterpret_cast<const bfrt::BfRtTableObj *>(table_hdl);
  *name_ret = table->data_field_name_get(field_id, action_id).c_str();
  return BF_SUCCESS;
}

bf_status_t bf_rt_data_field_type_get(const bf_rt_table_hdl *table_hdl,
                                      const bf_rt_id_t field_id,
                                      bf_rt_data_type_t *field_type_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);

  return table->dataFieldDataTypeGet(
      field_id, reinterpret_cast<bfrt::DataType *>(field_type_ret));
}

bf_status_t bf_rt_data_field_type_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    bf_rt_data_type_t *field_type_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);

  return table->dataFieldDataTypeGet(
      field_id, action_id, reinterpret_cast<bfrt::DataType *>(field_type_ret));
}

bf_status_t bf_rt_data_field_num_allowed_choices_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    uint32_t *num_choices) {
  std::vector<std::reference_wrapper<const std::string>> cpp_choices;
  bf_status_t sts = ::data_field_allowed_choices_get_helper<uint32_t *>(
      table_hdl, field_id, &cpp_choices, num_choices);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  *num_choices = cpp_choices.size();
  return BF_SUCCESS;
}

bf_status_t bf_rt_data_field_allowed_choices_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const char *choices[]) {
  std::vector<std::reference_wrapper<const std::string>> cpp_choices;
  bf_status_t sts = ::data_field_allowed_choices_get_helper<const char *[]>(
      table_hdl, field_id, &cpp_choices, choices);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  for (auto iter = cpp_choices.begin(); iter != cpp_choices.end(); iter++) {
    choices[iter - cpp_choices.begin()] = iter->get().c_str();
  }
  return BF_SUCCESS;
}

bf_status_t bf_rt_data_field_num_allowed_choices_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    uint32_t *num_choices) {
  std::vector<std::reference_wrapper<const std::string>> cpp_choices;
  bf_status_t sts = ::data_field_allowed_choices_get_helper<uint32_t *>(
      table_hdl, field_id, &cpp_choices, num_choices, action_id);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  *num_choices = cpp_choices.size();
  return BF_SUCCESS;
}

bf_status_t bf_rt_data_field_allowed_choices_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    const char *choices[]) {
  std::vector<std::reference_wrapper<const std::string>> cpp_choices;
  bf_status_t sts = ::data_field_allowed_choices_get_helper<const char *[]>(
      table_hdl, field_id, &cpp_choices, choices, action_id);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  for (auto iter = cpp_choices.begin(); iter != cpp_choices.end(); iter++) {
    choices[iter - cpp_choices.begin()] = iter->get().c_str();
  }
  return BF_SUCCESS;
}

bf_status_t bf_rt_data_field_num_annotations_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    uint32_t *num_annotations) {
  bfrt::AnnotationSet annotations;
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  auto bf_status = table->dataFieldAnnotationsGet(field_id, &annotations);
  if (bf_status != BF_SUCCESS) {
    return bf_status;
  }
  *num_annotations = annotations.size();
  return BF_SUCCESS;
}
bf_status_t bf_rt_data_field_annotations_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    bf_rt_annotation_t *annotations_c) {
  bfrt::AnnotationSet annotations;
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  auto bf_status = table->dataFieldAnnotationsGet(field_id, &annotations);
  if (bf_status != BF_SUCCESS) {
    return bf_status;
  }
  int i = 0;
  for (const auto &annotation : annotations) {
    annotations_c[i++] = convert_annotation(annotation.get());
  }
  return BF_SUCCESS;
}

bf_status_t bf_rt_data_field_num_annotations_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    uint32_t *num_annotations) {
  bfrt::AnnotationSet annotations;
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  auto bf_status =
      table->dataFieldAnnotationsGet(field_id, action_id, &annotations);
  if (bf_status != BF_SUCCESS) {
    return bf_status;
  }
  *num_annotations = annotations.size();
  return BF_SUCCESS;
}
bf_status_t bf_rt_data_field_annotations_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    bf_rt_annotation_t *annotations_c) {
  bfrt::AnnotationSet annotations;
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  auto bf_status =
      table->dataFieldAnnotationsGet(field_id, action_id, &annotations);
  if (bf_status != BF_SUCCESS) {
    return bf_status;
  }
  int i = 0;
  for (const auto &annotation : annotations) {
    annotations_c[i++] = convert_annotation(annotation.get());
  }
  return BF_SUCCESS;
}

bf_status_t bf_rt_data_field_num_oneof_siblings_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    uint32_t *num_oneof_siblings) {
  std::set<bf_rt_id_t> oneof_siblings;
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  auto bf_status = table->dataFieldOneofSiblingsGet(field_id, &oneof_siblings);
  if (bf_status != BF_SUCCESS) {
    return bf_status;
  }
  *num_oneof_siblings = oneof_siblings.size();
  return BF_SUCCESS;
}
bf_status_t bf_rt_data_field_oneof_siblings_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    bf_rt_id_t *oneof_siblings_c) {
  std::set<bf_rt_id_t> oneof_siblings;
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  auto bf_status = table->dataFieldOneofSiblingsGet(field_id, &oneof_siblings);
  if (bf_status != BF_SUCCESS) {
    return bf_status;
  }
  int i = 0;
  for (const auto &oneof_sibling : oneof_siblings) {
    oneof_siblings_c[i++] = oneof_sibling;
  }
  return BF_SUCCESS;
}

bf_status_t bf_rt_data_field_num_oneof_siblings_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    uint32_t *num_oneof_siblings) {
  std::set<bf_rt_id_t> oneof_siblings;
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  auto bf_status =
      table->dataFieldOneofSiblingsGet(field_id, action_id, &oneof_siblings);
  if (bf_status != BF_SUCCESS) {
    return bf_status;
  }
  *num_oneof_siblings = oneof_siblings.size();
  return BF_SUCCESS;
}
bf_status_t bf_rt_data_field_oneof_siblings_with_action_get(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_id_t field_id,
    const bf_rt_id_t action_id,
    bf_rt_id_t *oneof_siblings_c) {
  std::set<bf_rt_id_t> oneof_siblings;
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  auto bf_status =
      table->dataFieldOneofSiblingsGet(field_id, action_id, &oneof_siblings);
  if (bf_status != BF_SUCCESS) {
    return bf_status;
  }
  int i = 0;
  for (const auto &oneof_sibling : oneof_siblings) {
    oneof_siblings_c[i++] = oneof_sibling;
  }
  return BF_SUCCESS;
}

bf_status_t bf_rt_action_id_list_size_get(const bf_rt_table_hdl *table_hdl,
                                          uint32_t *num) {
  if (num == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::vector<bf_rt_id_t> action_id;
  bf_status_t sts = table->actionIdListGet(&action_id);
  if (sts == BF_SUCCESS) {
    *num = action_id.size();
  }

  return sts;
}

bf_status_t bf_rt_action_id_list_get(const bf_rt_table_hdl *table_hdl,
                                     bf_rt_id_t *id_vec_ret) {
  if (id_vec_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::vector<bf_rt_id_t> action_ids;
  bf_status_t sts = table->actionIdListGet(&action_ids);
  if (sts == BF_SUCCESS) {
    for (auto it = action_ids.begin(); it != action_ids.end(); ++it) {
      bf_rt_id_t action_id = *it;
      id_vec_ret[it - action_ids.begin()] = action_id;
    }
  }

  return sts;
}

bf_status_t bf_rt_action_name_get(const bf_rt_table_hdl *table_hdl,
                                  const bf_rt_id_t action_id,
                                  const char **name_ret) {
  if (name_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!table_hdl) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  auto table = reinterpret_cast<const bfrt::BfRtTableObj *>(table_hdl);
  *name_ret = table->action_name_get(action_id).c_str();

  return BF_SUCCESS;
}

bf_status_t bf_rt_action_name_to_id(const bf_rt_table_hdl *table_hdl,
                                    const char *action_name,
                                    bf_rt_id_t *action_id_ret) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->actionIdGet(action_name, action_id_ret);
}

bf_status_t bf_rt_action_id_applicable(const bf_rt_table_hdl *table_hdl,
                                       bool *ret_val) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  *ret_val = table->actionIdApplicable();
  return BF_SUCCESS;
}

bf_status_t bf_rt_action_num_annotations_get(const bf_rt_table_hdl *table_hdl,
                                             const bf_rt_id_t action_id,
                                             uint32_t *num_annotations) {
  bfrt::AnnotationSet annotations;
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  auto bf_status = table->actionAnnotationsGet(action_id, &annotations);
  if (bf_status != BF_SUCCESS) {
    return bf_status;
  }
  *num_annotations = annotations.size();
  return BF_SUCCESS;
}
bf_status_t bf_rt_action_annotations_get(const bf_rt_table_hdl *table_hdl,
                                         const bf_rt_id_t action_id,
                                         bf_rt_annotation_t *annotations_c) {
  bfrt::AnnotationSet annotations;
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  auto bf_status = table->actionAnnotationsGet(action_id, &annotations);
  if (bf_status != BF_SUCCESS) {
    return bf_status;
  }
  int i = 0;
  for (const auto &annotation : annotations) {
    annotations_c[i++] = convert_annotation(annotation.get());
  }
  return BF_SUCCESS;
}

#ifdef BFRT_GENERIC_FLAGS
bf_status_t bf_rt_table_attributes_set(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_session_hdl *session,
    const bf_rt_target_t *dev_tgt,
    const uint64_t flags,
    const bf_rt_table_attributes_hdl *tbl_attr) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableAttributesSet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      *reinterpret_cast<const bfrt::BfRtTableAttributes *>(tbl_attr));
}

bf_status_t bf_rt_table_attributes_get(const bf_rt_table_hdl *table_hdl,
                                       const bf_rt_session_hdl *session,
                                       const bf_rt_target_t *dev_tgt,
                                       const uint64_t flags,
                                       bf_rt_table_attributes_hdl *tbl_attr) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableAttributesGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      flags,
      reinterpret_cast<bfrt::BfRtTableAttributes *>(tbl_attr));
}
#else
bf_status_t bf_rt_table_attributes_set(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_session_hdl *session,
    const bf_rt_target_t *dev_tgt,
    const bf_rt_table_attributes_hdl *tbl_attr) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableAttributesSet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      *reinterpret_cast<const bfrt::BfRtTableAttributes *>(tbl_attr));
}

bf_status_t bf_rt_table_attributes_get(const bf_rt_table_hdl *table_hdl,
                                       const bf_rt_session_hdl *session,
                                       const bf_rt_target_t *dev_tgt,
                                       bf_rt_table_attributes_hdl *tbl_attr) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableAttributesGet(
      *reinterpret_cast<const bfrt::BfRtSession *>(session),
      *dev_tgt,
      reinterpret_cast<bfrt::BfRtTableAttributes *>(tbl_attr));
}
#endif
bf_status_t bf_rt_table_num_attributes_supported(
    const bf_rt_table_hdl *table_hdl, uint32_t *num_attributes) {
  if (table_hdl == nullptr || num_attributes == nullptr) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::set<bfrt::TableAttributesType> type_set;
  auto sts = table->tableAttributesSupported(&type_set);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  *num_attributes = type_set.size();
  return BF_SUCCESS;
}

bf_status_t bf_rt_table_attributes_supported(
    const bf_rt_table_hdl *table_hdl,
    bf_rt_table_attributes_type_t *attributes,
    uint32_t *num_returned) {
  if (table_hdl == nullptr || num_returned == nullptr ||
      attributes == nullptr) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::set<bfrt::TableAttributesType> type_set;
  auto sts = table->tableAttributesSupported(&type_set);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  int i = 0;
  for (const auto &iter : type_set) {
    attributes[i] = static_cast<bf_rt_table_attributes_type_t>(iter);
    i++;
  }
  *num_returned = i;
  return BF_SUCCESS;
}

bf_status_t bf_rt_table_operations_execute(
    const bf_rt_table_hdl *table_hdl,
    const bf_rt_table_operations_hdl *tbl_ops) {
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  return table->tableOperationsExecute(
      *reinterpret_cast<const bfrt::BfRtTableOperations *>(tbl_ops));
}

bf_status_t bf_rt_table_num_operations_supported(
    const bf_rt_table_hdl *table_hdl, uint32_t *num_operations) {
  if (table_hdl == nullptr || num_operations == nullptr) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::set<bfrt::TableOperationsType> type_set;
  auto sts = table->tableOperationsSupported(&type_set);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  *num_operations = type_set.size();
  return BF_SUCCESS;
}

bf_status_t bf_rt_table_operations_supported(
    const bf_rt_table_hdl *table_hdl,
    bf_rt_table_operations_mode_t *operations,
    uint32_t *num_returned) {
  if (table_hdl == nullptr || num_returned == nullptr ||
      operations == nullptr) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  std::set<bfrt::TableOperationsType> type_set;
  auto sts = table->tableOperationsSupported(&type_set);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  int i = 0;
  for (const auto &iter : type_set) {
    operations[i] = static_cast<bf_rt_table_operations_mode_t>(iter);
    i++;
  }
  *num_returned = i;
  return BF_SUCCESS;
}

bf_status_t bf_rt_table_num_api_supported(const bf_rt_table_hdl *table_hdl,
                                          uint32_t *num_apis) {
  if (table_hdl == nullptr || num_apis == nullptr) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bfrt::BfRtTable::TableApiSet type_set;
  auto sts = table->tableApiSupportedGet(&type_set);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  *num_apis = type_set.size();
  return BF_SUCCESS;
}

bf_status_t bf_rt_table_api_supported(const bf_rt_table_hdl *table_hdl,
                                      bf_rt_table_api_type_t *apis,
                                      uint32_t *num_returned) {
  if (table_hdl == nullptr || num_returned == nullptr || apis == nullptr) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto table = reinterpret_cast<const bfrt::BfRtTable *>(table_hdl);
  bfrt::BfRtTable::TableApiSet type_set;
  auto sts = table->tableApiSupportedGet(&type_set);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  int i = 0;
  for (const auto &iter : type_set) {
    apis[i] = static_cast<bf_rt_table_api_type_t>(iter.get());
    i++;
  }
  *num_returned = i;
  return BF_SUCCESS;
}

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

#include <stdio.h>
#include <bf_rt/bf_rt_init.h>
#include <bf_rt/bf_rt_info.h>

// bf_rt includes
#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_table.hpp>

// local includes
#include <bf_rt_common/bf_rt_utils.hpp>

bf_status_t bf_rt_num_tables_get(const bf_rt_info_hdl *bf_rt, int *num_tables) {
  if (!bf_rt) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto bfRtInfo = reinterpret_cast<const bfrt::BfRtInfo *>(bf_rt);
  std::vector<const bfrt::BfRtTable *> tables;
  auto status = bfRtInfo->bfrtInfoGetTables(&tables);
  if (status != BF_SUCCESS) {
    return status;
  }
  *num_tables = static_cast<int>(tables.size());
  return status;
}

bf_status_t bf_rt_tables_get(const bf_rt_info_hdl *bf_rt,
                             const bf_rt_table_hdl **bf_rt_table_hdl_ret) {
  if (!bf_rt) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto bfRtInfo = reinterpret_cast<const bfrt::BfRtInfo *>(bf_rt);
  std::vector<const bfrt::BfRtTable *> tables;
  auto status = bfRtInfo->bfrtInfoGetTables(&tables);
  if (status != BF_SUCCESS) {
    return status;
  }

  std::vector<const bfrt::BfRtTable *>::iterator it;
  /*
   * Iterate through the tables and fill the handle array.
   * We use iterators rather than auto so we can use iterator
   * arithmetic to increment our array index rather than a variable.
   */
  for (it = tables.begin(); it != tables.end(); ++it) {
    const bfrt::BfRtTable *table = *it;
    bf_rt_table_hdl_ret[it - tables.begin()] =
        reinterpret_cast<const bf_rt_table_hdl *>(table);
  }

  return status;
}

bf_status_t bf_rt_table_from_name_get(const bf_rt_info_hdl *bf_rt,
                                      const char *table_name,
                                      const bf_rt_table_hdl **table_hdl_ret) {
  if (table_hdl_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!bf_rt || !table_name) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto bfRtInfo = reinterpret_cast<const bfrt::BfRtInfo *>(bf_rt);
  const bfrt::BfRtTable *table;
  auto status = bfRtInfo->bfrtTableFromNameGet(table_name, &table);
  *table_hdl_ret = reinterpret_cast<const bf_rt_table_hdl *>(table);
  return status;
}

bf_status_t bf_rt_table_from_id_get(const bf_rt_info_hdl *bf_rt,
                                    bf_rt_id_t id,
                                    const bf_rt_table_hdl **table_hdl_ret) {
  if (table_hdl_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  auto bfRtInfo = reinterpret_cast<const bfrt::BfRtInfo *>(bf_rt);
  const bfrt::BfRtTable *table;
  auto status = bfRtInfo->bfrtTableFromIdGet(id, &table);
  *table_hdl_ret = reinterpret_cast<const bf_rt_table_hdl *>(table);
  return status;
}

bf_status_t bf_rt_table_name_to_id(const bf_rt_info_hdl *bf_rt,
                                   const char *table_name,
                                   bf_rt_id_t *id_ret) {
  if (id_ret == nullptr) {
    LOG_ERROR("%s:%d Invalid arg. Please allocate mem for out param",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  if (!bf_rt || !table_name) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto bfRtInfo = reinterpret_cast<const bfrt::BfRtInfo *>(bf_rt);
  const bfrt::BfRtTable *table;
  auto status = bfRtInfo->bfrtTableFromNameGet(table_name, &table);
  if (table == nullptr) {
    LOG_ERROR("%s:%d Table %s not found", __func__, __LINE__, table_name);
    return status;
  }
  return table->tableIdGet(id_ret);
}

bf_status_t bf_rt_num_tables_dependent_on_this_table_get(
    const bf_rt_info_hdl *bf_rt, const bf_rt_id_t tbl_id, int *num_tables) {
  if (!bf_rt || !num_tables) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto bfrt_info = reinterpret_cast<const bfrt::BfRtInfo *>(bf_rt);
  std::vector<bf_rt_id_t> table_ids;
  auto status =
      bfrt_info->bfrtInfoTablesDependentOnThisTableGet(tbl_id, &table_ids);
  if (status != BF_SUCCESS) {
    return status;
  }
  *num_tables = static_cast<int>(table_ids.size());
  return BF_SUCCESS;
}

bf_status_t bf_rt_tables_dependent_on_this_table_get(
    const bf_rt_info_hdl *bf_rt,
    const bf_rt_id_t tbl_id,
    bf_rt_id_t *table_list) {
  if (!bf_rt || !table_list) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto bfrt_info = reinterpret_cast<const bfrt::BfRtInfo *>(bf_rt);
  std::vector<bf_rt_id_t> table_ids;
  auto status =
      bfrt_info->bfrtInfoTablesDependentOnThisTableGet(tbl_id, &table_ids);
  if (status != BF_SUCCESS) {
    return status;
  }
  for (auto it = table_ids.begin(); it != table_ids.end(); it++) {
    table_list[it - table_ids.begin()] = *it;
  }
  return BF_SUCCESS;
}

bf_status_t bf_rt_num_tables_this_table_depends_on_get(
    const bf_rt_info_hdl *bf_rt, const bf_rt_id_t tbl_id, int *num_tables) {
  if (!bf_rt || !num_tables) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto bfrt_info = reinterpret_cast<const bfrt::BfRtInfo *>(bf_rt);
  std::vector<bf_rt_id_t> table_ids;
  auto status =
      bfrt_info->bfrtInfoTablesThisTableDependsOnGet(tbl_id, &table_ids);
  if (status != BF_SUCCESS) {
    return status;
  }
  *num_tables = static_cast<int>(table_ids.size());
  return BF_SUCCESS;
}

bf_status_t bf_rt_tables_this_table_depends_on_get(const bf_rt_info_hdl *bf_rt,
                                                   const bf_rt_id_t tbl_id,
                                                   bf_rt_id_t *table_list) {
  if (!bf_rt || !table_list) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto bfrt_info = reinterpret_cast<const bfrt::BfRtInfo *>(bf_rt);
  std::vector<bf_rt_id_t> table_ids;
  auto status =
      bfrt_info->bfrtInfoTablesThisTableDependsOnGet(tbl_id, &table_ids);
  if (status != BF_SUCCESS) {
    return status;
  }
  for (auto it = table_ids.begin(); it != table_ids.end(); it++) {
    table_list[it - table_ids.begin()] = *it;
  }
  return BF_SUCCESS;
}

bf_status_t bf_rt_info_num_pipeline_info_get(const bf_rt_info_hdl *bf_rt,
                                             int *num) {
  if (!bf_rt || !num) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto bfrt_info = reinterpret_cast<const bfrt::BfRtInfo *>(bf_rt);

  bfrt::PipelineProfInfoVec prof_vec;
  bf_status_t sts = bfrt_info->bfRtInfoPipelineInfoGet(&prof_vec);

  *num = static_cast<int>(prof_vec.size());
  return sts;
}

bf_status_t bf_rt_info_pipeline_info_get(const bf_rt_info_hdl *bf_rt,
                                         const char **prof_names,
                                         const bf_dev_pipe_t **pipes) {
  if (!bf_rt || !prof_names || !pipes) {
    LOG_ERROR("%s:%d Invalid arg", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  auto bfrt_info = reinterpret_cast<const bfrt::BfRtInfo *>(bf_rt);

  bfrt::PipelineProfInfoVec prof_vec;
  bf_status_t sts = bfrt_info->bfRtInfoPipelineInfoGet(&prof_vec);

  /*
   * Iterate through the names and fill the name string array.
   * We use iterators rather than auto so we can use iterator
   * arithmetic to increment our array index rather than a variable.
   * We use the reference_wrapper because we iterate through a list
   * of string references, not string values.
   */
  bfrt::PipelineProfInfoVec::iterator it;
  for (it = prof_vec.begin(); it != prof_vec.end(); ++it) {
    prof_names[it - prof_vec.begin()] = it->first.get().c_str();
    pipes[it - prof_vec.begin()] = it->second.get().data();
  }
  return sts;
}

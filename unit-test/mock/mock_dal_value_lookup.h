/*
 * Copyright(c) 2022 Intel Corporation.
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

MOCK_GLOBAL_FUNC2(pipe_mgr_is_pipe_valid, int(int, uint32_t));
MOCK_GLOBAL_FUNC2(pipe_mgr_api_prologue, int(uint32_t, struct bf_dev_target_t));
MOCK_GLOBAL_FUNC2(pipe_mgr_api_epilogue, void(uint32_t, struct bf_dev_target_t));
MOCK_GLOBAL_FUNC4(pipe_mgr_ctx_get_table, int(struct bf_dev_target_t, uint32_t, enum pipe_mgr_table_type, void **));
MOCK_GLOBAL_FUNC4(pipe_mgr_table_key_delete, int(struct bf_dev_target_t, void *, enum pipe_mgr_table_type, struct pipe_tbl_match_spec *));
MOCK_GLOBAL_FUNC7(pipe_mgr_table_key_exists, int(void *, enum pipe_mgr_table_type, struct pipe_tbl_match_spec *, bf_dev_pipe_t, bool *, uint32_t *, void **));
MOCK_GLOBAL_FUNC5(pipe_mgr_table_key_insert, int(struct bf_dev_target_t, void *, enum pipe_mgr_table_type, void *, uint32_t *));
MOCK_GLOBAL_FUNC5(pipe_mgr_table_get, int(void *, enum pipe_mgr_table_type, uint32_t, uint32_t, void **));
MOCK_GLOBAL_FUNC6(dal_value_lookup_ent_add, int(uint32_t, struct bf_dev_target_t, uint32_t, struct pipe_tbl_match_spec *, struct pipe_data_spec *, struct pipe_mgr_value_lookup_ctx *));
MOCK_GLOBAL_FUNC5(dal_value_lookup_ent_del, int(uint32_t, struct bf_dev_target_t, uint32_t, struct pipe_tbl_match_spec *, struct pipe_mgr_value_lookup_ctx *));
MOCK_GLOBAL_FUNC0(dal_value_lookup_ent_get_first, int(void));
MOCK_GLOBAL_FUNC0(dal_value_lookup_ent_get_next_n_by_key, int(void));

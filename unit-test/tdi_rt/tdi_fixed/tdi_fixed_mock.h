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
MOCK_GLOBAL_FUNC5(fixed_func_mgr_ent_add, int (u32 sess_hdl, 
			               bf_dev_target_t dev_tgt,
				       const char *table_name,
				       struct fixed_function_key_spec  *key_spec,
				       struct fixed_function_data_spec *data_spec));

MOCK_GLOBAL_FUNC5(fixed_func_mgr_get_stats, int (u32 sess_hdl,
			                         bf_dev_target_t dev_tgt,
				                 const char *table_name,
				                 struct fixed_function_key_spec  *key_spec,
				                 struct fixed_function_data_spec *data_spec));

MOCK_GLOBAL_FUNC4(fixed_func_mgr_ent_del, int (u32 sess_hdl,
			                  bf_dev_target_t dev_tgt,
				          const char *table_name,
				          struct fixed_function_key_spec  *key_spec));

MOCK_GLOBAL_FUNC4(fixed_func_mgr_get_default_entry, int (u32 sess_hdl,
			                             bf_dev_target_t dev_tgt,
				                     const char *table_name,
				                     struct fixed_function_data_spec *data_spec));

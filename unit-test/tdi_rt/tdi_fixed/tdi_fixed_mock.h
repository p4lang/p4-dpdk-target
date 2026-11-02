/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
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

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

MOCK_GLOBAL_FUNC1(crypto_cfg_spi_add,
		bf_status_t (struct fixed_function_data_spec*));

MOCK_GLOBAL_FUNC2(crypto_cfg_table_del,
		bf_status_t (bf_dev_id_t, struct fixed_function_key_spec*));

MOCK_GLOBAL_FUNC3(port_cfg_table_add,
		bf_status_t (bf_dev_id_t,
			     struct fixed_function_key_spec*,
			     struct fixed_function_data_spec*));

MOCK_GLOBAL_FUNC3(crypto_cfg_table_add,
		bf_status_t (bf_dev_id_t,
			     struct fixed_function_key_spec*,
			     struct fixed_function_data_spec*));

MOCK_GLOBAL_FUNC3(port_all_stats_get,
		bf_status_t (bf_dev_id_t,
			     struct fixed_function_key_spec*,
			     struct fixed_function_data_spec*));

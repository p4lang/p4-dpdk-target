/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
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

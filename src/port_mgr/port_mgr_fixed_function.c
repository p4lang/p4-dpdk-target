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

#include "port_mgr/dal/dal_port_mgr.h"
#include "port_mgr/port_mgr_log.h"

bf_status_t port_cfg_table_add(bf_dev_id_t dev_id,
                               struct fixed_function_key_spec *key,
                               struct fixed_function_data_spec *data)
{
	struct fixed_function_table_ctx *tbl_ctx = NULL;
	struct fixed_function_mgr_ctx *ff_ctx = NULL;

	ff_ctx = get_fixed_function_mgr_ctx(FF_MGR_PORT);
	if (!ff_ctx)
		return BF_INVALID_ARG;

	tbl_ctx = get_fixed_function_table_ctx(ff_ctx,
				FIXED_FUNCTION_TABLE_TYPE_CONFIG);
	if (!tbl_ctx)
		return BF_INVALID_ARG;

	return dal_port_cfg_table_add(dev_id, key, data, tbl_ctx);
}

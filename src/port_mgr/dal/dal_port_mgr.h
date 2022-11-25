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

#ifndef __DAL_PORT_MGR_H__
#define __DAL_PORT_MGR_H__

#include "fixed_function/fixed_function_int.h"

bf_status_t dal_port_cfg_table_add(bf_dev_id_t dev_id,
                                   struct fixed_function_key_spec *key,
                                   struct fixed_function_data_spec *data,
                                   struct fixed_function_table_ctx *tbl_ctx);

#endif /* __DAL_PORT_MGR_H__ */

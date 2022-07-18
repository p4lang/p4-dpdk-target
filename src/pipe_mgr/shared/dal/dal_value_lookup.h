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

/*!
 * @file dal_value_lookup.h
 *
 * @Description Declarations for dal layer interfaces to value lookup table.
 */

#ifndef __DAL_VALUE_LOOKUP_H__
#define __DAL_VALUE_LOOKUP_H__

#include "../infra/pipe_mgr_int.h"

int
dal_value_lookup_ent_add(uint32_t sess_hdl,
			 struct bf_dev_target_t dev_tgt,
			 uint32_t tbl_hdl,
			 struct pipe_tbl_match_spec *match_spec,
			 struct pipe_data_spec *data_spec,
			 struct pipe_mgr_value_lookup_ctx *tbl_ctx);

int
dal_value_lookup_ent_del(uint32_t sess_hdl,
			 struct bf_dev_target_t dev_tgt,
			 uint32_t tbl_hdl,
			 struct pipe_tbl_match_spec *match_spec,
			 struct pipe_mgr_value_lookup_ctx *tbl_ctx);

int
dal_value_lookup_ent_get_first(void);

int
dal_value_lookup_ent_get_next_n_by_key(void);

#endif /* __DAL_VALUE_LOOKUP_H__ */

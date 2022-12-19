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
 * @file dal_registers.h
 *
 * @description Utilities for regsiters
 */

#ifndef __DAL_REGISTERS_H__
#define __DAL_REGISTERS_H__

#include <bf_types/bf_types.h>
#include "../../core/pipe_mgr_log.h"
#include "../infra/pipe_mgr_int.h"
#include <pipe_mgr/pipe_mgr_intf.h>

bf_status_t
dal_reg_read_indirect_register_set(bf_dev_target_t dev_tgt,
				   const char *table_name,
				   int id,
				   pipe_stful_mem_query_t *stats);
bf_status_t
dal_reg_write_assignable_register_set(bf_dev_target_t dev_tgt,
				      const char *name,
				      int id,
				      pipe_stful_mem_spec_t *stats);
#endif /* __DAL_REGISTERS_H__ */

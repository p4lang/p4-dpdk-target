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
 * @file pipe_mgr_registers.h
 *
 * @description Utilities for registers
 */

#ifndef __PIPE_MGR_REGISTERS_H__
#define __PIPE_MGR_REGISTERS_H__

#include <bf_types/bf_types.h>

bf_status_t pipe_mgr_reg_read_indirect_register_set(dev_target_t dev_tgt,
						    const char *table_name,
						    pipe_stful_tbl_hdl_t stful_tbl_hdl,
						    pipe_stful_mem_idx_t stful_ent_idx,
						    pipe_stful_mem_query_t *stful_query,
						    uint32_t pipe_api_flags);
bf_status_t pipe_mgr_reg_mod_assignable_register_set(dev_target_t dev_tgt,
						     const char *name,
						     pipe_stful_mem_idx_t stful_ent_idx,
						     pipe_stful_mem_spec_t *stful_spec);
#endif /* __PIPE_MGR_REGISTERS_H__ */

/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
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

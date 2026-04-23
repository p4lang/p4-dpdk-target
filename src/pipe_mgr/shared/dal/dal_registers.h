/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
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

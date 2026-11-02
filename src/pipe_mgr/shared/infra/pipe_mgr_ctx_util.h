/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*!
 * @file pipe_mgr_ctx_util.h
 * @date
 *
 * Utilities for retrieving pipeline's context objects.
 */
#ifndef __PIPE_MGR_CTX_UTIL_H__
#define __PIPE_MGR_CTX_UTIL_H__

#include "pipe_mgr_int.h"

/* API to get the table pointer.
 * @param dev_tgt device target
 * @param tbl_hdl table handle
 * @param tbl_type type of table for which pointer to be filled
 * @param tbl is the pointer to be filled for table
 * return API return status
 */
int
pipe_mgr_ctx_get_table(struct bf_dev_target_t dev_tgt,
		       u32 tbl_hdl,
		       enum pipe_mgr_table_type tbl_type,
		       void **tbl);
#endif

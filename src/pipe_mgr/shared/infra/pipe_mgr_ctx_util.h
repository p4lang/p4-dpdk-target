/*
 * Copyright(c) 2021 Intel Corporation.
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

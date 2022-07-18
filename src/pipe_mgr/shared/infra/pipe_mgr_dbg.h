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
 * @file pipe_mgr_dbg.h
 * @date
 *
 * Debug utilities for pipe_mgr.
 */
#ifndef __PIPE_MGR_DBG_H__
#define __PIPE_MGR_DBG_H__

#include "pipe_mgr/shared/pipe_mgr_infra.h"
#include "pipe_mgr/shared/pipe_mgr_mat.h"
#include "pipe_mgr/shared/infra/pipe_mgr_int.h"
#include "pipe_mgr/shared/pipe_mgr_value_lookup.h"

void pipe_mgr_print_match_spec(struct pipe_tbl_match_spec *match_spec);
void pipe_mgr_print_action_spec(struct pipe_action_spec *action_spec);
void pipe_mgr_print_dal_mat_buf(struct pipe_mgr_mat_ctx *mat_ctx);
void pipe_mgr_print_data_spec(struct pipe_data_spec *data_spec);

#endif

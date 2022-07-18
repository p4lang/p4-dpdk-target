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
 * @file pipe_mgr_tbl.h
 * @date
 *
 * definitions for pipe_mgr table entry handling.
 */
#ifndef __PIPE_MGR_TBL_H__
#define __PIPE_MGR_TBL_H__

#include <pipe_mgr/shared/pipe_mgr_infra.h>
#include "pipe_mgr_int.h"

int pipe_mgr_table_key_exists(void *tbl,
			      enum pipe_mgr_table_type tbl_type,
			      struct pipe_tbl_match_spec *ms,
			      bf_dev_pipe_t pipe_id,
			      bool *exists,
			      u32 *ent_hdl,
			      void **entry);

int pipe_mgr_table_key_insert(struct bf_dev_target_t dev_tgt,
			      void *tbl,
			      enum pipe_mgr_table_type tbl_type,
			      void *entry,
			      u32 *ent_hdl);

int pipe_mgr_table_key_delete(struct bf_dev_target_t dev_tgt,
			      void *tbl,
			      enum pipe_mgr_table_type tbl_type,
			      struct pipe_tbl_match_spec *match_spec);

int pipe_mgr_table_get_first(void *tbl,
			     enum pipe_mgr_table_type tbl_type,
			     bf_dev_pipe_t pipe_id,
			     u32 *ent_hdl);

int pipe_mgr_table_get_next_n(void *tbl,
			      enum pipe_mgr_table_type tbl_type,
			      bf_dev_pipe_t pipe_id,
			      u32 ent_hdl,
			      int n,
			      u32 *next_ent_hdls);

int pipe_mgr_table_get(void *tbl,
		       enum pipe_mgr_table_type tbl_type,
		       bf_dev_pipe_t pipe_id,
		       u32 ent_hdl,
		       void **entry);

#endif /* __PIPE_MGR_TBL_H__ */

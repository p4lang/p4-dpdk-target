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
 * @file dal_tbl.h
 *
 * @Description Declarations for interfaces for tables (DPDK).
 */

#ifndef __DAL_DPDK_TBL_H__
#define __DAL_DPDK_TBL_H__

#include "../../infra/pipe_mgr_int.h"

int dal_dpdk_table_metadata_get(void *tbl, enum pipe_mgr_table_type tbl_type,
				char *pipeline_name, char *table_name);

int dal_dpdk_table_entry_alloc(struct rte_swx_table_entry **ent,
			       struct dal_dpdk_table_metadata *meta,
			       int match_type);

#endif /* __DAL_DPDK_TBL_H__ */

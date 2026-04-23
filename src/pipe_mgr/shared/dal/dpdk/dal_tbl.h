/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
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

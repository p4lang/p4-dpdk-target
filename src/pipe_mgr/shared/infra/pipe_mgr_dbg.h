/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
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

/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*!
 * @file dal_value_lookup.h
 *
 * @Description Declarations for dal layer interfaces to value lookup table.
 */

#ifndef __DAL_VALUE_LOOKUP_H__
#define __DAL_VALUE_LOOKUP_H__

#include "../infra/pipe_mgr_int.h"

int
dal_value_lookup_ent_add(uint32_t sess_hdl,
			 struct bf_dev_target_t dev_tgt,
			 uint32_t tbl_hdl,
			 struct pipe_tbl_match_spec *match_spec,
			 struct pipe_data_spec *data_spec,
			 struct pipe_mgr_value_lookup_ctx *tbl_ctx);

int
dal_value_lookup_ent_del(uint32_t sess_hdl,
			 struct bf_dev_target_t dev_tgt,
			 uint32_t tbl_hdl,
			 struct pipe_tbl_match_spec *match_spec,
			 struct pipe_mgr_value_lookup_ctx *tbl_ctx);

int
dal_value_lookup_ent_get_first(void);

int
dal_value_lookup_ent_get_next_n_by_key(void);

#endif /* __DAL_VALUE_LOOKUP_H__ */

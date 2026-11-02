/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
MOCK_GLOBAL_FUNC7(pipe_stful_ent_set, bf_status_t(pipe_sess_hdl_t sess_hdl,
						  dev_target_t dev_tgt,
						  const char *table_name,
						  pipe_stful_tbl_hdl_t stful_tbl_hdl,
						  pipe_stful_mem_idx_t stful_ent_idx,
						  pipe_stful_mem_spec_t *stful_spec,
						  uint32_t pipe_api_flags));
MOCK_GLOBAL_FUNC7(pipe_stful_ent_query, bf_status_t(pipe_sess_hdl_t sess_hdl,
						    dev_target_t dev_tgt,
						    const char *table_name,
						    pipe_stful_tbl_hdl_t stful_tbl_hdl,
						    pipe_stful_mem_idx_t stful_ent_idx,
						    pipe_stful_mem_query_t *stful_query,
						    uint32_t pipe_api_flags));

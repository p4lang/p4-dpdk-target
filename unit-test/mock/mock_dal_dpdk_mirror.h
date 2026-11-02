/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
//Mocking rte_swx_ctl_pipeline_mirroring_session_set function which is defined
MOCK_GLOBAL_FUNC3(rte_swx_ctl_pipeline_mirroring_session_set, int(struct rte_swx_pipeline *p, uint32_t id, struct rte_swx_pipeline_mirroring_session_params *mir_params));

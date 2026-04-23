/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
//Mocking rte_swx_pipeline_mirroring_config function which is defined
MOCK_GLOBAL_FUNC2(rte_swx_pipeline_mirroring_config, int(struct rte_swx_pipeline *p, struct rte_swx_pipeline_mirroring_params *mir_params));

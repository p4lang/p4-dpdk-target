/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
//Mocking pipeline_find function which is defined
//in src/lld/dpdk/infra/dpdk_obj.c
MOCK_GLOBAL_FUNC1(pipeline_find, struct pipeline *(const char *name));

//Mocking mempool_find function which is defined
//in src/lld/dpdk/infra/dpdk_obj.c
MOCK_GLOBAL_FUNC1(mempool_find, struct mempool *(const char *name));

//Mocking tap_find function which is defined
//in src/lld/dpdk/infra/dpdk_obj.c
MOCK_GLOBAL_FUNC1(tap_find, struct tap *(const char *name));

//Mocking link_find function which is defined
//in src/lld/dpdk/infra/dpdk_obj.c
MOCK_GLOBAL_FUNC1(link_find, struct link *(const char *name));

//Mocking ring_find function which is defined
//in src/lld/dpdk/infra/dpdk_obj.c
MOCK_GLOBAL_FUNC1(ring_find, struct ring *(const char *name));


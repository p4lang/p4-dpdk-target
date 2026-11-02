// SPDX-FileCopyrightText: 2022 Intel Corporation
// Copyright (C) 2022 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0

/*Each testcase file can atmost have 5k checks
 *Note: Please update the number of checks included in the below field
 *Number of checks = 5*/

#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include <gmock-global.h>

extern "C"{
    #include "lld_dpdk_lib.c"
    #include "mock.h"
    #include "../mock_lld_dpdk_lib.h"
}

using namespace std;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;

TEST(MIRROR_DPDK, case0) {
	struct rte_swx_pipeline_mirroring_params mir_params;
	int actual_res, expected_res;
	int res;

	mir_params.n_slots = 4;
	mir_params.n_sessions = 16;

	actual_res = lld_dpdk_pipeline_mirror_config(NULL, &mir_params);
	expected_res = 0;

	ASSERT_EQ(actual_res, expected_res);
}

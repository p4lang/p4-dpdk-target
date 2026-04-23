// SPDX-FileCopyrightText: 2022 Intel Corporation
// Copyright (C) 2022 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0

/*Each testcase file can atmost have 5k checks
 *Note: Please update the number of checks included in the below field
 *Number of checks = 1*/

#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include <gmock-global.h>

extern "C"{
    #include "dal_mirror.c"
    #include "mock.h"
    #include "../../../../mock/mock_dal_dpdk_mirror.h"
}

using namespace std;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;

TEST(MIRROR_DPDK, case1) {
	struct pipe_mgr_mir_prof mir_params = {0};
	int dev_id;
	int mir_id;
	int actual_res;
	int expect_res;

	dev_id = 0;
	mir_id = 1;
	expect_res = 0;

	mir_params.port_id = 1;
	mir_params.fast_clone = 0;
	mir_params.truncate_length = 1024;

	EXPECT_GLOBAL_CALL(rte_swx_ctl_pipeline_mirroring_session_set, rte_swx_ctl_pipeline_mirroring_session_set(_,_,_))
		.Times(1).
		WillOnce(Return(0));

	actual_res = dal_mirror_session_set(mir_id, (struct pipe_mgr_mir_prof *)&mir_params, NULL);

	ASSERT_EQ(actual_res, expect_res);
}

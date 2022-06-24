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

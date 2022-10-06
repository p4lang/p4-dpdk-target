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

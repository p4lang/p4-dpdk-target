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
    #include "dal_mat.c"
    #include "mock.h"
}

using namespace std;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;

/*
 * Test Case for Exact Match Table with Add-on-Miss Enabled
 */
TEST(DPDK_MAT_CTX, case1) {
	struct pipe_mgr_mat_ctx mat_ctx = {0};
	mat_ctx.match_attr.match_type == PIPE_MGR_MATCH_TYPE_EXACT;
	mat_ctx.add_on_miss = true;
	int actual_result, expected_result;

	/* Entries should not be stored for Add-on-Miss Table
	 * Expected Result is False
	 */
	expected_result = false;
	actual_result = dal_mat_store_entries(&mat_ctx);

	ASSERT_EQ(actual_result, expected_result);
}

/*
 * Test Case for Exact Match Table with Add-on-Miss Disabled
 */
TEST(DPDK_MAT_CTX, case2) {
        struct pipe_mgr_mat_ctx mat_ctx = {0};
        mat_ctx.match_attr.match_type == PIPE_MGR_MATCH_TYPE_EXACT;
        mat_ctx.add_on_miss = false;
        int actual_result, expected_result;

        /* Entries should be stored for Normal Tables
         * Expected Result is True
         */
        expected_result = true;
        actual_result = dal_mat_store_entries(&mat_ctx);

        ASSERT_EQ(actual_result, expected_result);
}

/*
 * Test Case for Ternary Match Table with Add-on-Miss Disabled
 */
TEST(DPDK_MAT_CTX, case3) {
        struct pipe_mgr_mat_ctx mat_ctx = {0};
        mat_ctx.match_attr.match_type == PIPE_MGR_MATCH_TYPE_TERNARY;
        mat_ctx.add_on_miss = false;
        int actual_result, expected_result;

        /* Entries should be stored for Ternary Table
         * Expected Result is True
         */
        expected_result = true;
        actual_result = dal_mat_store_entries(&mat_ctx);

        ASSERT_EQ(actual_result, expected_result);
}


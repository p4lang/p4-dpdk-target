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
 *Number of checks = 2
 */

#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include <gmock-global.h>

extern "C"{
    #include "pipe_mgr_value_lookup.c"
    #include "mock.h"
    #include "../../../mock/mock_dal_value_lookup.h"
}

using namespace std;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;

TEST(ValueLookupTable, case1) {
	EXPECT_GLOBAL_CALL(pipe_mgr_is_pipe_valid, pipe_mgr_is_pipe_valid(_,_))
		.Times(1).
		WillOnce(Return(0));
	EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
		.Times(1).
		WillOnce(Return(0));
	EXPECT_GLOBAL_CALL(pipe_mgr_ctx_get_table, pipe_mgr_ctx_get_table(_,_,_,_))
		.Times(1).
		WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_api_epilogue, pipe_mgr_api_epilogue(_,_))
                .Times(1);

	struct pipe_tbl_match_spec match_spec;
	struct pipe_data_spec data_spec;
	struct bf_dev_target_t dev_tgt;
	uint32_t sess_hdl = 0;
	int a_res, e_res = 0;
	uint32_t tbl_hdl;
	uint32_t ent_hdl;

	a_res = pipe_mgr_value_lookup_ent_get(sess_hdl, dev_tgt, tbl_hdl, ent_hdl, &match_spec, &data_spec);
}

TEST(ValueLookupTable, case2) {
	EXPECT_GLOBAL_CALL(pipe_mgr_is_pipe_valid, pipe_mgr_is_pipe_valid(_,_))
		.Times(1).
		WillOnce(Return(0));
	EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
		.Times(1).
		WillOnce(Return(0));
	EXPECT_GLOBAL_CALL(pipe_mgr_ctx_get_table, pipe_mgr_ctx_get_table(_,_,_,_))
		.Times(1).
		WillOnce(Return(0));
	EXPECT_GLOBAL_CALL(pipe_mgr_table_key_exists, pipe_mgr_table_key_exists(_,_,_,_,_,_,_))
		.Times(1).
		WillOnce(Return(0));
       EXPECT_GLOBAL_CALL(pipe_mgr_api_epilogue, pipe_mgr_api_epilogue(_,_))
                .Times(1);


	struct pipe_tbl_match_spec match_spec;
	struct pipe_data_spec data_spec;
	struct bf_dev_target_t dev_tgt;
	uint32_t sess_hdl = 0;
	int a_res, e_res = 0;
	uint32_t tbl_hdl;
	uint32_t ent_hdl;

	a_res = pipe_mgr_value_lookup_ent_del(sess_hdl, dev_tgt, tbl_hdl, &match_spec);
}

TEST(ValueLookupTable, case3) {
	EXPECT_GLOBAL_CALL(pipe_mgr_is_pipe_valid, pipe_mgr_is_pipe_valid(_,_))
		.Times(1).
		WillOnce(Return(0));
	EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
		.Times(1).
		WillOnce(Return(0));
	EXPECT_GLOBAL_CALL(pipe_mgr_ctx_get_table, pipe_mgr_ctx_get_table(_,_,_,_))
		.Times(1).
		WillOnce(Return(0));
	EXPECT_GLOBAL_CALL(pipe_mgr_table_key_exists, pipe_mgr_table_key_exists(_,_,_,_,_,_,_))
		.Times(1).
		WillOnce(Return(0));
	EXPECT_GLOBAL_CALL(pipe_mgr_table_key_insert, pipe_mgr_table_key_insert(_,_,_,_,_))
		.Times(1).
		WillOnce(Return(0));
	EXPECT_GLOBAL_CALL(dal_value_lookup_ent_add, dal_value_lookup_ent_add(_,_,_,_,_,_))
		.Times(1).
		WillOnce(Return(0));
       EXPECT_GLOBAL_CALL(pipe_mgr_api_epilogue, pipe_mgr_api_epilogue(_,_))
                .Times(1);

	struct pipe_tbl_match_spec match_spec = {0};
	struct pipe_data_spec data_spec = {0};
	struct bf_dev_target_t dev_tgt;
	uint32_t sess_hdl = 0;
	int a_res, e_res = 0;
	uint32_t tbl_hdl;
	uint32_t ent_hdl;

	a_res = pipe_mgr_value_lookup_ent_add(sess_hdl, dev_tgt, tbl_hdl, &match_spec, &data_spec, &ent_hdl);

	ASSERT_EQ(a_res, e_res);
}

TEST(ValueLookupTable, case4) {
        EXPECT_GLOBAL_CALL(pipe_mgr_is_pipe_valid, pipe_mgr_is_pipe_valid(_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_ctx_get_table, pipe_mgr_ctx_get_table(_,_,_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_api_epilogue, pipe_mgr_api_epilogue(_,_))
                .Times(1);
      EXPECT_GLOBAL_CALL(dal_value_lookup_ent_get_first, dal_value_lookup_ent_get_first())
                .Times(1).
                WillOnce(Return(0));

        struct pipe_tbl_match_spec match_spec;
        struct pipe_data_spec data_spec;
        struct bf_dev_target_t dev_tgt;
        uint32_t sess_hdl = 0;
        int a_res, e_res = 0;
        uint32_t tbl_hdl;
        uint32_t ent_hdl;

        a_res = pipe_mgr_value_lookup_ent_get_first(sess_hdl, dev_tgt, tbl_hdl, &match_spec, &data_spec, &ent_hdl);
	ASSERT_EQ(a_res, e_res);
}

TEST(ValueLookupTable, case5) {
        EXPECT_GLOBAL_CALL(pipe_mgr_is_pipe_valid, pipe_mgr_is_pipe_valid(_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_ctx_get_table, pipe_mgr_ctx_get_table(_,_,_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_api_epilogue, pipe_mgr_api_epilogue(_,_))
                .Times(1);

      EXPECT_GLOBAL_CALL(dal_value_lookup_ent_get_next_n_by_key, dal_value_lookup_ent_get_next_n_by_key())
                .Times(1).
                WillOnce(Return(0));

        struct pipe_tbl_match_spec match_spec, curr_spec;
        struct pipe_data_spec *data_spec;
        uint32_t sess_hdl = 0, num = 0;
        struct bf_dev_target_t dev_tgt;
        int a_res, e_res = 0, n = 0;
        uint32_t tbl_hdl;
        uint32_t ent_hdl;

        a_res = pipe_mgr_value_lookup_ent_get_next_n_by_key(sess_hdl, dev_tgt, tbl_hdl, &curr_spec, n, &match_spec, &data_spec, &num);
	ASSERT_EQ(a_res, e_res);
}

TEST(ValueLookupTable, case6) {
        EXPECT_GLOBAL_CALL(pipe_mgr_is_pipe_valid, pipe_mgr_is_pipe_valid(_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_ctx_get_table, pipe_mgr_ctx_get_table(_,_,_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_api_epilogue, pipe_mgr_api_epilogue(_,_))
                .Times(1);
        int a_res = BF_NOT_SUPPORTED, e_res = BF_NOT_SUPPORTED;
        struct bf_dev_target_t dev_tgt;
        uint32_t sess_hdl = 0;
        uint32_t tbl_hdl;
        uint32_t ent_hdl;

        a_res = pipe_mgr_value_lookup_get_first_ent_handle(sess_hdl, dev_tgt, tbl_hdl, &ent_hdl);
        ASSERT_EQ(a_res, e_res);
}

TEST(ValueLookupTable, case7) {
        EXPECT_GLOBAL_CALL(pipe_mgr_is_pipe_valid, pipe_mgr_is_pipe_valid(_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_ctx_get_table, pipe_mgr_ctx_get_table(_,_,_,_))
                .Times(1).
                WillOnce(Return(0));
        EXPECT_GLOBAL_CALL(pipe_mgr_api_epilogue, pipe_mgr_api_epilogue(_,_))
                .Times(1);

        int a_res = BF_NOT_SUPPORTED, e_res = BF_NOT_SUPPORTED;
        uint32_t sess_hdl = 0, next_ent_hdl;
        struct bf_dev_target_t dev_tgt;
        uint32_t ent_hdl, num_hdl;
        uint32_t tbl_hdl;

        a_res = pipe_mgr_value_lookup_get_next_n_ent_handle(sess_hdl, dev_tgt, tbl_hdl, ent_hdl, num_hdl, &next_ent_hdl);
        ASSERT_EQ(a_res, e_res);
}


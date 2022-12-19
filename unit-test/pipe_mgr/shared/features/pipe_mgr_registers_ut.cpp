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
    #include "pipe_mgr_registers.c"
    #include "mock.h"
}

using namespace std;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;

MOCK_GLOBAL_FUNC2(pipe_mgr_api_prologue, int(uint32_t, struct bf_dev_target_t));
MOCK_GLOBAL_FUNC2(pipe_mgr_api_epilogue, void(uint32_t, struct bf_dev_target_t));
MOCK_GLOBAL_FUNC4(dal_reg_read_indirect_register_set, bf_status_t(dev_target_t dev_tgt, const char *name, int id, pipe_stful_mem_query_t *stful_query));
MOCK_GLOBAL_FUNC4(dal_reg_write_assignable_register_set, bf_status_t(bf_dev_target_t dev_tgt, const char *name, int id,  pipe_stful_mem_spec_t *stful_spec));

TEST(Register, case1) {
    pipe_stful_tbl_hdl_t stful_tbl_hdl = {0};
    pipe_stful_mem_idx_t stful_ent_idx = {0};
    pipe_stful_mem_query_t stful_query = {0};
    pipe_sess_hdl_t sess_hdl;
    uint32_t pipe_api_flags;
    const char *table_name;
    dev_target_t dev_tgt;
    int a_res, e_res = 0;

    EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
      .Times(1).
      WillOnce(Return(0));
    EXPECT_GLOBAL_CALL(pipe_mgr_api_epilogue, pipe_mgr_api_epilogue(_,_))
      .Times(1);
    EXPECT_GLOBAL_CALL(dal_reg_read_indirect_register_set,
        dal_reg_read_indirect_register_set(_,_,_,_))
      .Times(1).
      WillOnce(Return(0));

    a_res = pipe_stful_ent_query(sess_hdl, dev_tgt, table_name, stful_tbl_hdl,
                               stful_ent_idx, &stful_query, pipe_api_flags);
    ASSERT_EQ(a_res, e_res);
}

TEST(Register, case2) {
   pipe_stful_tbl_hdl_t stful_tbl_hdl;
   pipe_stful_mem_idx_t stful_ent_idx;
   pipe_stful_mem_spec_t* stful_spec;
   pipe_sess_hdl_t sess_hdl;
   uint32_t pipe_api_flags;
   const char *table_name;
   dev_target_t dev_tgt;
   int a_res, e_res = 0;

   EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
     .Times(1).
     WillOnce(Return(0));
   EXPECT_GLOBAL_CALL(pipe_mgr_api_epilogue, pipe_mgr_api_epilogue(_,_))
     .Times(1);
   EXPECT_GLOBAL_CALL(dal_reg_write_assignable_register_set,
         dal_reg_write_assignable_register_set(_,_,_,_))
     .Times(1).
     WillOnce(Return(0));

   a_res = pipe_stful_ent_set(sess_hdl, dev_tgt, table_name, stful_tbl_hdl,
                              stful_ent_idx, stful_spec, pipe_api_flags);
   ASSERT_EQ(a_res, e_res);
}

TEST(Register, case3) {

    pipe_stful_tbl_hdl_t stful_tbl_hdl = {0};
    pipe_stful_mem_idx_t stful_ent_idx = {0};
    pipe_stful_mem_query_t stful_query = {0};
    pipe_sess_hdl_t sess_hdl;
    uint32_t pipe_api_flags;
    const char *table_name;
    dev_target_t dev_tgt;
    int a_res, e_res = 0;

    EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
      .Times(1).
      WillOnce(Return(1));
    e_res = 1;
    a_res = pipe_stful_ent_query(sess_hdl, dev_tgt, table_name, stful_tbl_hdl,
                               stful_ent_idx, &stful_query, pipe_api_flags);
    ASSERT_EQ(a_res, e_res);

    EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
      .Times(1).
      WillOnce(Return(0));
    EXPECT_GLOBAL_CALL(dal_reg_read_indirect_register_set,
        dal_reg_read_indirect_register_set(_,_,_,_))
      .Times(1).
      WillOnce(Return(1));

    e_res = 1;
    a_res = pipe_stful_ent_query(sess_hdl, dev_tgt, table_name, stful_tbl_hdl,
                               stful_ent_idx, &stful_query, pipe_api_flags);
    ASSERT_EQ(a_res, e_res);

}

TEST(Register, case4) {

   pipe_stful_tbl_hdl_t stful_tbl_hdl;
   pipe_stful_mem_idx_t stful_ent_idx;
   pipe_stful_mem_spec_t* stful_spec;
   pipe_sess_hdl_t sess_hdl;
   uint32_t pipe_api_flags;
   const char *table_name;
   dev_target_t dev_tgt;
   int a_res, e_res = 0;

   EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
     .Times(1).
     WillOnce(Return(1));
   e_res = 1;
   a_res = pipe_stful_ent_set(sess_hdl, dev_tgt, table_name, stful_tbl_hdl,
                              stful_ent_idx, stful_spec, pipe_api_flags);
   ASSERT_EQ(a_res, e_res);

   EXPECT_GLOBAL_CALL(pipe_mgr_api_prologue, pipe_mgr_api_prologue(_,_))
     .Times(1).
     WillOnce(Return(0));
   EXPECT_GLOBAL_CALL(dal_reg_write_assignable_register_set,
         dal_reg_write_assignable_register_set(_,_,_,_))
     .Times(1).
     WillOnce(Return(1));
   e_res = 1;
   a_res = pipe_stful_ent_set(sess_hdl, dev_tgt, table_name, stful_tbl_hdl,
                              stful_ent_idx, stful_spec, pipe_api_flags);
   ASSERT_EQ(a_res, e_res);
}


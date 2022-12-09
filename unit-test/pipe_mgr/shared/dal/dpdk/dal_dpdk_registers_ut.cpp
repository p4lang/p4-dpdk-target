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
 *Number of checks = 2*/

#include <stdio.h>
#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include <gmock-global.h>

extern "C"{
    #include "dal_registers.c"
    #include "mock.h"
}

using namespace std;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;

MOCK_GLOBAL_FUNC2(bf_hashtbl_search,
                   void *(bf_hashtable_t *htbl, void *key));
MOCK_GLOBAL_FUNC1(pipeline_find,
                  struct pipeline *(const char *name));
MOCK_GLOBAL_FUNC3(pipe_mgr_get_profile,
                  int(int dev_id, int profile_id,
                  struct pipe_mgr_profile **profile));
MOCK_GLOBAL_FUNC2(pipe_mgr_get_profile_ctx,
                  int(struct bf_dev_target_t dev_tgt,
                  struct pipe_mgr_p4_pipeline **parsed_pipe_ctx));
MOCK_GLOBAL_FUNC4(rte_swx_ctl_pipeline_regarray_read,
                  int(struct rte_swx_pipeline *p,
                  const char *regarray_name,
                  uint32_t regarray_index,
                  uint64_t *value));
MOCK_GLOBAL_FUNC4(rte_swx_ctl_pipeline_regarray_write,
                  int(struct rte_swx_pipeline *p,
                  const char *regarray_name,
                  uint32_t regarray_index,
                  uint64_t value));
MOCK_GLOBAL_FUNC1(trim_classifier_str, char *(char *str));
int pipe_mgr_get_profile_dummy(int dev_id, int profile_id,
                               struct pipe_mgr_profile **profile)
{
  *profile = (struct pipe_mgr_profile *)calloc(1, sizeof(*profile));
  (*profile)->pipe_ctx.mat_tables =
    calloc(1, sizeof((*profile)->pipe_ctx.mat_tables));
  memcpy((*profile)->pipe_ctx.mat_tables->ctx.name, "ipv4_host",
               sizeof((*profile)->pipe_ctx.mat_tables->ctx.name));
  (*profile)->pipe_ctx.mat_tables->ctx.handle = 65536;
  return 0;
}

int pipe_mgr_get_profile_ctx_dummy(struct bf_dev_target_t dev_tgt,
                                   struct pipe_mgr_p4_pipeline **ctx_obj)
{
  *ctx_obj = (struct pipe_mgr_p4_pipeline *)calloc(1, sizeof(*ctx_obj));
  (*ctx_obj)->mat_tables = calloc(1, sizeof((*ctx_obj)->mat_tables));
  (*ctx_obj)->mat_tables->ctx.match_attr.match_type =
               PIPE_MGR_MATCH_TYPE_EXACT;
  (*ctx_obj)->num_externs_tables = 1;
  (*ctx_obj)->externs_tables_name =
    malloc((*ctx_obj)->num_externs_tables * sizeof(char*));
  (*ctx_obj)->externs_tables_name[0] =
    malloc(P4_SDE_TABLE_NAME_LEN);
  strncpy((*ctx_obj)->externs_tables_name[0],
    "per_prefix_pkt_count", P4_SDE_TABLE_NAME_LEN-1);
  (*ctx_obj)->bf_externs_htbl =
    calloc(1, sizeof((*ctx_obj)->bf_externs_htbl));
  return 0;
}

void *bf_hashtbl_search_dummy(char *str)
{
  struct pipe_mgr_externs_ctx *externs_entry  = NULL;
  externs_entry = calloc(1, sizeof(*externs_entry));
  return (void *)externs_entry;
}
// register indirect read
TEST(DPDK_INDIRECT_REG, case0) {

   struct pipe_mgr_externs_ctx *externs_entry  = NULL;
   struct pipe_mgr_p4_pipeline *ctx_obj = NULL;
   pipe_stful_mem_query_t stful_query = {0};
   const char *table_name = "ip.reg";
   pipe_stful_mem_spec_t data = {0};
   struct pipeline *pipe = NULL;
   stful_query.pipe_count = 1;
   stful_query.data = &data;
   bf_dev_target_t dev_tgt;
   char *name = "ip.reg";
   int a_res, e_res = 0;
   int id;

   pipe = (struct pipeline *)calloc(1, sizeof(*pipe));
   ctx_obj = calloc(1, sizeof(*ctx_obj));
   ctx_obj->mat_tables = calloc(1, sizeof(*ctx_obj->mat_tables));
   externs_entry = calloc(1, sizeof(*externs_entry));
   memcpy(externs_entry->target_name, "registers",
     sizeof(externs_entry->target_name));

   EXPECT_GLOBAL_CALL(pipe_mgr_get_profile, pipe_mgr_get_profile(_,_,_))
     .Times(AtLeast(1)).
     WillOnce(Return(1)).WillRepeatedly(&pipe_mgr_get_profile_dummy);
   EXPECT_GLOBAL_CALL(pipeline_find, pipeline_find(_))
     .Times(AtLeast(1)).
     WillOnce(Return(NULL)).WillRepeatedly(Return(pipe));
   EXPECT_GLOBAL_CALL(pipe_mgr_get_profile_ctx, pipe_mgr_get_profile_ctx(_,_))
     .Times(AtLeast(1)).
     WillRepeatedly(&pipe_mgr_get_profile_ctx_dummy);
   EXPECT_GLOBAL_CALL(bf_hashtbl_search, bf_hashtbl_search(_,_))
     .Times(AtLeast(1)).
     WillOnce(Return((struct pipe_mgr_externs_ctx *)NULL))
     .WillRepeatedly(Return((struct pipe_mgr_externs_ctx *)externs_entry));
   EXPECT_GLOBAL_CALL(trim_classifier_str, trim_classifier_str(_))
       .Times(AtLeast(1)).
        WillRepeatedly(Return(name));
   EXPECT_GLOBAL_CALL(rte_swx_ctl_pipeline_regarray_read,
                     rte_swx_ctl_pipeline_regarray_read(_,_,_,_))
     .Times(AtLeast(1)).
     WillOnce(Return(1)).WillRepeatedly(Return(0));

   a_res = dal_reg_read_indirect_register_set(dev_tgt, table_name, id, &stful_query);
   ASSERT_EQ(a_res, BF_OBJECT_NOT_FOUND);
   a_res = dal_reg_read_indirect_register_set(dev_tgt, table_name, id, &stful_query);
   ASSERT_EQ(a_res, BF_OBJECT_NOT_FOUND);
   a_res = dal_reg_read_indirect_register_set(dev_tgt, table_name, id, &stful_query);
   ASSERT_EQ(a_res, BF_OBJECT_NOT_FOUND);
   a_res = dal_reg_read_indirect_register_set(dev_tgt, table_name, id, &stful_query);
   ASSERT_EQ(a_res, BF_OBJECT_NOT_FOUND);
   a_res = dal_reg_read_indirect_register_set(dev_tgt, table_name, id, &stful_query);
   ASSERT_EQ(a_res, e_res);
   free(pipe);
   free(externs_entry);
}
// register indirect write
TEST(DPDK_INDIRECT_REG, case1) {

   struct pipe_mgr_externs_ctx *externs_entry  = NULL;
   struct pipe_mgr_p4_pipeline *ctx_obj = NULL;
   pipe_stful_mem_spec_t stful_spec = {0};
   const char *table_name = "ip.reg";
   struct pipeline *pipe = NULL;
   bf_dev_target_t dev_tgt;
   char *name = "ip.reg";
   int a_res, e_res = 0;
   int id;

   pipe = (struct pipeline *)calloc(1, sizeof(*pipe));
   ctx_obj = calloc(1, sizeof(*ctx_obj));
   ctx_obj->mat_tables = calloc(1, sizeof(*ctx_obj->mat_tables));
   externs_entry = calloc(1, sizeof(*externs_entry));
   memcpy(externs_entry->target_name, "registers",
     sizeof(externs_entry->target_name));

   EXPECT_GLOBAL_CALL(pipe_mgr_get_profile, pipe_mgr_get_profile(_,_,_))
     .Times(AtLeast(1)).
     WillOnce(Return(1))
     .WillRepeatedly(&pipe_mgr_get_profile_dummy);
   EXPECT_GLOBAL_CALL(pipeline_find, pipeline_find(_))
     .Times(AtLeast(1)).
     WillOnce(Return(NULL))
     .WillRepeatedly(Return(pipe));
   EXPECT_GLOBAL_CALL(pipe_mgr_get_profile_ctx, pipe_mgr_get_profile_ctx(_,_))
     .Times(AtLeast(1)).
     WillRepeatedly(&pipe_mgr_get_profile_ctx_dummy);
   EXPECT_GLOBAL_CALL(bf_hashtbl_search, bf_hashtbl_search(_,_))
     .Times(AtLeast(1)).
     WillOnce(Return((struct pipe_mgr_externs_ctx *)NULL))
     .WillRepeatedly(Return((struct pipe_mgr_externs_ctx *)externs_entry));
   EXPECT_GLOBAL_CALL(trim_classifier_str, trim_classifier_str(_))
       .Times(AtLeast(1)).
        WillRepeatedly(Return(name));
   EXPECT_GLOBAL_CALL(rte_swx_ctl_pipeline_regarray_write,
                     rte_swx_ctl_pipeline_regarray_write(_,_,_,_))
     .Times(AtLeast(1)).
     WillOnce(Return(1)).WillRepeatedly(Return(0));

   a_res = dal_reg_write_assignable_register_set(dev_tgt, table_name, id, &stful_spec);
   ASSERT_EQ(a_res, BF_OBJECT_NOT_FOUND);
   a_res = dal_reg_write_assignable_register_set(dev_tgt, table_name, id, &stful_spec);
   ASSERT_EQ(a_res, BF_OBJECT_NOT_FOUND);
   a_res = dal_reg_write_assignable_register_set(dev_tgt, table_name, id, &stful_spec);
   ASSERT_EQ(a_res, BF_OBJECT_NOT_FOUND);
   a_res = dal_reg_write_assignable_register_set(dev_tgt, table_name, id, &stful_spec);
   ASSERT_EQ(a_res, BF_OBJECT_NOT_FOUND);
   a_res = dal_reg_write_assignable_register_set(dev_tgt, table_name, id, &stful_spec);
   ASSERT_EQ(a_res, e_res);
   free(pipe);
   free(externs_entry);
}


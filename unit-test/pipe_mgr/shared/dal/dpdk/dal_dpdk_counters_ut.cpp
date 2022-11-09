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

#include <stdio.h>
#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include <gmock-global.h>

extern "C"{
    #include "dal_counters.c"
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

int rte_swx_ctl_pipeline_regarray_read_with_key_dummy(struct rte_swx_pipeline *p,
                                                      const char *regarray_name,
                                                      const char *table_name,
                                                      uint8_t *table_key,
                                                      uint64_t *value)
{
	*value = 4;
	return 0;
}

MOCK_GLOBAL_FUNC5(rte_swx_ctl_pipeline_regarray_read_with_key,
                  int(struct rte_swx_pipeline *, const char *,
                      const char *, uint8_t *, uint64_t *));

TEST(DPDK_DIRECT_CNT, case0) {

        printf("UT for Direct Counter.");
        struct pipe_mgr_externs_ctx *externs_entry  = NULL;
        struct pipe_mgr_p4_pipeline *ctx_obj = NULL;
        struct pipe_mgr_profile *profile = NULL;
        struct pipe_tbl_match_spec match_spec;
        struct pipeline *pipe = NULL;
        pipe_stat_data_t *stat_data;
        bf_dev_target_t dev_tgt;
        void *dal_data = NULL;
        uint64_t value = 0;
        int exp_res = 0;
        int status = 0;
        int index = 0;

        dev_tgt.dev_pipe_id = 0;
        dev_tgt.device_id = 0;
        profile = calloc(1, sizeof(*profile));
        ctx_obj = calloc(1, sizeof(*ctx_obj));
        externs_entry = calloc(1, sizeof(*externs_entry));
        pipe = calloc(1, sizeof(*pipe));
        stat_data = calloc(1, sizeof(*stat_data));
        stat_data->packets = 5;
        match_spec.num_match_bytes = 4 ;
        match_spec.num_valid_match_bits = 32;
        match_spec.match_value_bits = 0x7fffd0755a60;
        ctx_obj->mat_tables = calloc(1, sizeof(*ctx_obj->mat_tables));
        ctx_obj->mat_tables->ctx.match_attr.match_type =
		PIPE_MGR_MATCH_TYPE_EXACT;
        memcpy(profile->pipeline_name, "pipe", sizeof(P4_SDE_PROG_NAME_LEN));
        profile->pipe_ctx.mat_tables =
		calloc(1, sizeof(*profile->pipe_ctx.mat_tables));
        memcpy(profile->pipe_ctx.mat_tables->ctx.name, "ipv4_host",
		sizeof(profile->pipe_ctx.mat_tables->ctx.name));
        profile->pipe_ctx.mat_tables->ctx.handle = 65536;
        externs_entry->externs_attr_table_id = 65536;
        externs_entry->attr_type = EXTERNS_ATTR_TYPE_PACKETS;
        memcpy(externs_entry->target_name, "per_prefix_pkt_count",
		sizeof(externs_entry->target_name));

        EXPECT_GLOBAL_CALL(pipe_mgr_get_profile, pipe_mgr_get_profile(_,_,_))
                .Times(1).
                WillOnce(&pipe_mgr_get_profile_dummy);

        EXPECT_GLOBAL_CALL(pipeline_find, pipeline_find(_))
                .Times(1).
                WillOnce(Return(pipe));

        EXPECT_GLOBAL_CALL(pipe_mgr_get_profile_ctx,
                           pipe_mgr_get_profile_ctx(_,_))
                .Times(1).
                WillOnce(&pipe_mgr_get_profile_ctx_dummy);

        EXPECT_GLOBAL_CALL(bf_hashtbl_search, bf_hashtbl_search(_,_))
                .Times(1).
                WillOnce(Return((void *)externs_entry));

        EXPECT_GLOBAL_CALL(rte_swx_ctl_pipeline_regarray_read_with_key,
		            rte_swx_ctl_pipeline_regarray_read_with_key(_,
				                                       _,_,_,_))
                .Times(1).
                WillOnce(&rte_swx_ctl_pipeline_regarray_read_with_key_dummy);

        status = dal_cnt_read_flow_direct_counter_set(dal_data,
						       (void *)stat_data,
						       dev_tgt, &match_spec);
        ASSERT_EQ(status, exp_res);
        free(profile);
        free(ctx_obj);
        free(externs_entry);
        free(pipe);
        free(stat_data);
        free(ctx_obj->mat_tables);
        free(profile->pipe_ctx.mat_tables);
        printf("end of Direct counter UT");
}


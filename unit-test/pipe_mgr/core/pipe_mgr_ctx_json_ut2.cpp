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
 *Number of checks = 7*/

#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include <gmock-global.h>

extern "C"{
    #include "pipe_mgr_ctx_json.c"
    #include "mock.h"
    #include "../mock_pipe_mgr_ctx_json.h"
}

using namespace std;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;

TEST(ValueLookupTable, case0){

    cJSON *test = cJSON_Parse("{\"match_key_fields\": [ {\"name\": \"hdrs.mac[meta.depth].da\", \"instance_name\": \"hdrs.mac[meta.depth]\", \"field_name\": \"da\",\"match_type\": \"exact\", \"start_bit\": 0, \"bit_width\": 48, \"bit_width_full\": 48, \"index\": 0, \"position\": 0 } ]}");
    char *name = "NoAction";

    EXPECT_GLOBAL_CALL(bf_cjson_get_handle, bf_cjson_get_handle(_,_,_,_,_))
    .Times(1).
    WillOnce(Return(0));
    EXPECT_GLOBAL_CALL(bf_cjson_get_string, bf_cjson_get_string(_,_,_))
    .WillRepeatedly(testing::DoAll(testing::SetArgPointee<2>(name), Return(0)));
    EXPECT_GLOBAL_CALL(bf_cjson_try_get_int, bf_cjson_try_get_int(_,_,_))
    .WillRepeatedly(Return(0));
    EXPECT_GLOBAL_CALL(bf_cjson_try_get_string, bf_cjson_try_get_string(_,_,_))
    .WillRepeatedly(Return(0));
   EXPECT_GLOBAL_CALL(bf_cjson_get_int, bf_cjson_get_int(_,_,_))
    .WillRepeatedly(Return(0));
    EXPECT_GLOBAL_CALL(bf_cjson_try_get_bool, bf_cjson_try_get_bool(_,_,_))
    .WillRepeatedly(Return(0));

    EXPECT_GLOBAL_CALL(bf_cjson_get_object, bf_cjson_get_object(_,_,_))
    .WillRepeatedly(Return(0));

    EXPECT_GLOBAL_CALL(bf_cjson_try_get_object, bf_cjson_try_get_object(_,_,_))
    .Times(2).
    WillRepeatedly(testing::DoAll(testing::SetArgPointee<2>(test),Return(0)));
    EXPECT_GLOBAL_CALL(dal_parse_ctx_json_parse_value_lookup_stage_tables, dal_parse_ctx_json_parse_value_lookup_stage_tables(_,_,_,_,_,_))
    .Times(1).
    WillOnce(Return(0));

    struct pipe_mgr_value_lookup_ctx tbl_ctx;
    int prof_id = 0;
    int dev_id = 0;
    cJSON input;
    memset((void *)&tbl_ctx, 0, sizeof(struct pipe_mgr_value_lookup_ctx));
    //Function call
    int actual_result = ctx_json_parse_value_lookup_table_json(dev_id, prof_id, &input, &tbl_ctx);
    int expected_result = BF_SUCCESS;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);
}

TEST(ValueLookupTable, ctx_parse_value_lookup_match_attribute){

    EXPECT_GLOBAL_CALL(bf_cjson_try_get_string, bf_cjson_try_get_string(_,_,_))
    .Times(1).
    WillOnce(Return(0));

    EXPECT_GLOBAL_CALL(bf_cjson_get_object, bf_cjson_get_object(_,_,_))
    .Times(1).
    WillOnce(Return(0));
    EXPECT_GLOBAL_CALL(dal_parse_ctx_json_parse_value_lookup_stage_tables, dal_parse_ctx_json_parse_value_lookup_stage_tables(_,_,_,_,_,_))
    .Times(1).
    WillOnce(Return(0));

    struct pipe_mgr_value_lookup_ctx tbl_ctx;
    int prof_id = 0;
    int dev_id = 0;
    cJSON input;
        //Function call
    memset((void *)&tbl_ctx, 0, sizeof(struct pipe_mgr_value_lookup_ctx));
    int actual_result = ctx_json_parse_value_lookup_match_attribute_json(dev_id, prof_id, &input, &tbl_ctx);
    int expected_result = BF_SUCCESS;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);
}

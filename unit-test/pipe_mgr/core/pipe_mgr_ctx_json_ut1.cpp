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

//Valid JSON file with version
TEST(ParsVersion, case0){
    EXPECT_GLOBAL_CALL(bf_cjson_get_string, bf_cjson_get_string(_,_,_))
    .Times(1).
    WillOnce(Return(0));

    cJSON* input;
    //Function call
    int actual_result = ctx_json_parse_version(input);
    int expected_result = BF_SUCCESS;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);
}

//Valid JSON file without version
TEST(ParsVersion, case1){
    EXPECT_GLOBAL_CALL(bf_cjson_get_string, bf_cjson_get_string(_,_,_))
    .Times(1)
    .WillOnce(Return(-1));

    cJSON* input;
    //Function call
    int actual_result = ctx_json_parse_version(input);
    int expected_result = BF_OBJECT_NOT_FOUND;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);
}

//NULL
TEST(ParsVersion, case2){
    EXPECT_GLOBAL_CALL(bf_cjson_get_string, bf_cjson_get_string(_,_,_))
    .Times(1)
    .WillOnce(Return(-1));

    //Function call
    int actual_result = ctx_json_parse_version(NULL);
    int expected_result = BF_OBJECT_NOT_FOUND;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);
}

//Valid string
TEST(MatchType, case0){
    char input[128];
    strcpy(input,"exact");
    //Function Call
    enum pipe_mgr_match_type actual_result = get_match_type_enum(input);
    enum pipe_mgr_match_type expected_result = PIPE_MGR_MATCH_TYPE_EXACT;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);

    strcpy(input, "ternary");
    //Function Call
    actual_result = get_match_type_enum(input);
    expected_result = PIPE_MGR_MATCH_TYPE_TERNARY;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);

    strcpy(input, "lpm");
    //Function Call
    actual_result = get_match_type_enum(input);
    expected_result = PIPE_MGR_MATCH_TYPE_LPM;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);
}

//Passing invalid string
TEST(MatchType, case1){
    char input[20];
    strcpy(input, "selector");
    //Function Call
    enum pipe_mgr_match_type actual_result = get_match_type_enum(input);
    enum pipe_mgr_match_type expected_result = PIPE_MGR_MATCH_TYPE_INVALID;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);
}

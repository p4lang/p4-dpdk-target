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
    #include "mock.h"
    #include "pipe_mgr_ctx_dpdk_json.c"
}
using namespace std;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;

MOCK_GLOBAL_FUNC3(bf_cjson_get_string, int (cJSON *, char*, char**));
MOCK_GLOBAL_FUNC3(bf_cjson_try_get_object, int (cJSON *, char *, cJSON **));
MOCK_GLOBAL_FUNC1(trim_classifier_str, char* (char*));

int bf_cjson_get_string_dummy(cJSON *cjson , char *property , char **ret)
{
  *ret = "MyIC.reg";
  return 0;
}
int bf_cjson_get_string_dummy1(cJSON *cjson , char *property , char **ret)
{
  *ret = "reg_0";
  return 0;
}
int bf_cjson_get_string_dummy2(cJSON *cjson , char *property , char **ret)
{
  *ret = "Register";
  return 0;
}
TEST(ParsRegExtern, case0) {
    EXPECT_GLOBAL_CALL(bf_cjson_get_string, bf_cjson_get_string(_,_,_))
      .Times(3).
      WillOnce(&bf_cjson_get_string_dummy)
      .WillOnce(&bf_cjson_get_string_dummy1)
      .WillOnce(&bf_cjson_get_string_dummy2);
    EXPECT_GLOBAL_CALL(bf_cjson_try_get_object, bf_cjson_try_get_object(_,_,_))
      .Times(1).
      WillOnce(Return(0));
    EXPECT_GLOBAL_CALL(trim_classifier_str, trim_classifier_str(_))
      .Times(1).
      WillOnce(Return("reg_0"));

    char *name = "reg_0";
    int dev_id;
    int prof_id;
    struct pipe_mgr_externs_ctx entry = {0};
    cJSON *extern_cjson;
    int expected_result = BF_SUCCESS;

    int actual_result = ctx_json_parse_externs_entry(dev_id, prof_id,
                         extern_cjson, &entry);

    ASSERT_EQ(actual_result, expected_result);
}

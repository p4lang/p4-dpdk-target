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
 *Number of checks = 4*/

#include <gtest/gtest.h>
#include <fstream>
#include "bf_rt_info_impl.cpp"

//Valid Table type
TEST(FieldType, case0){
    //Preparing function argument data
    bfrt::BfRtTable::TableType type = bfrt::BfRtTable::TableType::METER;

    //Calling function
    bfrt::DataFieldType actual_res = bfrt::getDataFieldTypeFrmRes(type);

    //Defining expected result
    bfrt::DataFieldType expected_res = bfrt::DataFieldType::METER_INDEX;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_res, expected_res);

    type = bfrt::BfRtTable::TableType::COUNTER;
    actual_res = bfrt::getDataFieldTypeFrmRes(type);
    expected_res = bfrt::DataFieldType::COUNTER_INDEX;
    
    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_res, expected_res);
}

//Invalid Table Type
TEST(FieldType, case1){
    //Preparing function argument data
    bfrt::BfRtTable::TableType type = 
            bfrt::BfRtTable::TableType::PORT_METADATA;
    //Calling function
    bfrt::DataFieldType actual_res = bfrt::getDataFieldTypeFrmRes(type);

    //Defining expected result
    bfrt::DataFieldType expected_res = bfrt::DataFieldType::INVALID;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_res, expected_res);
}

//Invalid Table Type
TEST(FieldType, case2){
    //Preparing function argument data
    bfrt::BfRtTable::TableType type = 
            bfrt::BfRtTable::TableType::PORT_METADATA;
    //Calling function
    bfrt::DataFieldType actual_res = bfrt::getDataFieldTypeFrmRes(type);

    //Defining expected result
    bfrt::DataFieldType expected_res = bfrt::DataFieldType::INVALID;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_res, expected_res);
}

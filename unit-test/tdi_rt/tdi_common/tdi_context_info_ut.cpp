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

#include <gtest/gtest.h>
#include <fstream>
#include <tdi_context_info.cpp>

namespace tdi {
namespace pna {
namespace rt {
//Valid Table type
TEST(RegisterFieldType, case0){
    //Preparing function argument data
    std::string data_name = "REGISTER_INDEX";
    tdi_rt_table_type_e table_type = TDI_RT_TABLE_TYPE_REGISTER;

    //Calling function
    DataFieldType actual_res = getDataFieldTypeFrmName(data_name, table_type);

    //Defining expected result
    DataFieldType expected_res = DataFieldType::REGISTER_INDEX;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_res, expected_res);

    data_name = "REGISTER_SPEC";
    actual_res = getDataFieldTypeFrmName(data_name, table_type);
    expected_res = DataFieldType::REGISTER_SPEC;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_res, expected_res);

    data_name = "REGISTER_SPEC_LO";
    actual_res = getDataFieldTypeFrmName(data_name, table_type);
    expected_res = DataFieldType::REGISTER_SPEC_LO;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_res, expected_res);

    data_name = "REGISTER_SPEC_HI";
    actual_res = getDataFieldTypeFrmName(data_name, table_type);
    expected_res = DataFieldType::REGISTER_SPEC_HI;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_res, expected_res);
}

//Invalid Table Type
TEST(RegisterFieldType, case1){
    //Preparing function argument data
    std::string data_name = "PORT_METADATA";
    tdi_rt_table_type_e table_type = TDI_RT_TABLE_TYPE_REGISTER;

    //Calling function
    DataFieldType actual_res = getDataFieldTypeFrmName(data_name, table_type);

    //Defining expected result
    DataFieldType expected_res = DataFieldType::INVALID;

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_res, expected_res);
}

} //namespace rt
} //namespace pna
} //namespace tdi

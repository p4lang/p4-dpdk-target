// SPDX-FileCopyrightText: 2022 Intel Corporation
// Copyright (C) 2022 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0

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

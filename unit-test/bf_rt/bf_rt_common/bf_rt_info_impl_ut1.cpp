/*
 * Copyright(c) 2021 Intel Corporation.
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

#define ENV (std::string)std::getenv("SDE")
#define FILE1 "bf-rt1.json"
#define FILE2 "bf-rt2.json"
#define FILE3 "bf-rt3.json"
#define SRC_PATH "/p4_sde-nat-p4-driver/unit-test/bf_rt/bf_rt_common/"

//Testable function here is "parseAnnotations" using getCjsonChildVec()
//internally. It is difficult to set it's behavior because it returns
//complex data. So, Mocking is not performed here and the respective 
//libraries are linked in CMakeLists.txt.

//bf-rt json File with annotations
TEST(parseAnnot, case0){
    //Data Preparation
    //Getting the content of a file to a string
    std::ifstream f_in;
    std::string file = ENV + SRC_PATH + FILE1;
    f_in.open(file);
    if(!f_in.is_open()){
        std::cout << "Error opening file\n";
        exit(1);
    }
    std::string contents((std::istreambuf_iterator<char>(f_in)), 
            std::istreambuf_iterator<char>());
    f_in.close();

    //Creating a json file from the string & getting its root node
    bfrt::Cjson root =  bfrt::Cjson::createCjsonFromFile(contents);

    //Moving to table node from root
    bfrt::Cjson table = root["tables"];

    //Moving to annotation node from table
    bfrt::Cjson annotation = table[0]["annotations"];

    //Passing the annotation node to the parseAnnotation function
    //Getting the set of annotations
    std::set<bfrt::Annotation> res = 
            bfrt::parseAnnotations(table[0]["annotations"]);
    
    //Creating set with expected result
    std::set<bfrt::Annotation> expected;
    bfrt::Annotation eth_obj
            ("@rte_flow_item_type(\"RTE_FLOW_ITEM_TYPE_ETH\")", "");
    bfrt::Annotation ipv4_obj
            ("@rte_flow_item_type(\"RTE_FLOW_ITEM_TYPE_IPV4\")", "");
    bfrt::Annotation ipv6_obj
            ("@rte_flow_item_type(\"RTE_FLOW_ITEM_TYPE_IPV6\")", "");
    expected.insert(eth_obj);
    expected.insert(ipv4_obj);
    expected.insert(ipv6_obj);

    //Macro checking expected result against the actual result
    ASSERT_EQ(res, expected);
}

//bf-rt json File without annotations
TEST(parseAnnot, case1){
    //Data Preparation
    //Getting the content of a file to a string
    std::ifstream f_in;
    std::string file = ENV + SRC_PATH + FILE2;
    f_in.open(file);
    if(!f_in.is_open()){
        std::cout << "Error opening file\n";
        exit(1);
    }
    std::string contents((std::istreambuf_iterator<char>(f_in)), 
            std::istreambuf_iterator<char>());
    f_in.close();

    //Creating a json file from the string & getting its root node
    bfrt::Cjson root =  bfrt::Cjson::createCjsonFromFile(contents);

    //Moving to table node from root
    bfrt::Cjson table = root["tables"];

    //Moving to annotation node from table
    bfrt::Cjson annotation = table[0]["annotations"];

    //Passing the annotation node to the parseAnnotation function
    //Getting the set of annotations
    std::set<bfrt::Annotation> res = 
            bfrt::parseAnnotations(table[0]["annotations"]);

    //Macro checking whether the set is empty
    ASSERT_TRUE(res.empty());
}

//bf-rt json File with empty annotations
TEST(parseAnnot, case2){
    //Data Preparation
    //Getting the content of a file to a string
    std::ifstream f_in;
    std::string file = ENV + SRC_PATH + FILE3;
    f_in.open(file);
    if(!f_in.is_open()){
        std::cout << "Error opening file\n";
        exit(1);
    }
    std::string contents((std::istreambuf_iterator<char>(f_in)), 
            std::istreambuf_iterator<char>());
    f_in.close();
    
    //Creating a json file from the string & getting its root node
    bfrt::Cjson root =  bfrt::Cjson::createCjsonFromFile(contents);

    //Moving to table node from root
    bfrt::Cjson table = root["tables"];

    //Moving to annotation node from table
    bfrt::Cjson annotation = table[0]["annotations"];

    //Passing the annotation node to the parseAnnotation function
    //Getting the set of annotations
    std::set<bfrt::Annotation> res = 
            bfrt::parseAnnotations(table[0]["annotations"]);
    std::set<bfrt::Annotation> empty;

    //Macro checking expected result against the actual result
    ASSERT_EQ(res, empty);
}

//NULL as file
TEST(parseAnnot, case4){
    //Creating a json file from the nullptr & getting its root node
    bfrt::Cjson root =  bfrt::Cjson::createCjsonFromFile("");

    //Moving to table node from root
    bfrt::Cjson table = root["tables"];

    //Moving to annotation node from table
    bfrt::Cjson annotation = table[0]["annotations"];

    //Passing the annotation node to the parseAnnotation function
    //Getting the set of annotations
    std::set<bfrt::Annotation> res = 
            bfrt::parseAnnotations(table[0]["annotations"]);
    
    //Macro checking whether the set is empty
    ASSERT_TRUE(res.empty());
}
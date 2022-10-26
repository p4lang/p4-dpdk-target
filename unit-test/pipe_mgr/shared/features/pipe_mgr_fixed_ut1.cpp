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
    #include "pipe_mgr_fixed.c"
}

using namespace std;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;

MOCK_GLOBAL_FUNC3(port_cfg_table_add, bf_status_t
		(bf_dev_id_t, struct fixed_function_key_spec*, struct fixed_function_data_spec*));

/* ff_mgr_ent_add */
TEST(fixedFunctionConfigTable, case1) {
	const char *table_name =   "ipsec_offload.ipsec-offload.sad.sad-entry.ipsec-sa-config";
	const char *table_name_1 = "ipsec-offload.ipsec-offload.sad.sad-entry.ipsec-sa-config";
        struct fixed_function_data_spec data_spec = {0};
        struct fixed_function_key_spec  key_spec  = {0};
        struct bf_dev_target_t dev_tgt = {0};
        uint32_t sess_hdl = 0;
        int a_res, e_res = 0;

        a_res = ff_mgr_ent_add(sess_hdl, dev_tgt, table_name, &key_spec, &data_spec);

	EXPECT_EQ(a_res, e_res);

        a_res = ff_mgr_ent_add(sess_hdl, dev_tgt, table_name_1, &key_spec, &data_spec);
	ASSERT_EQ(a_res, e_res);
}

TEST(fixedFunctionConfigTable, case2) {
        struct fixed_function_data_spec data_spec = {0};
        struct fixed_function_key_spec  key_spec  = {0};
	const char *table_name = "port.port";
        struct bf_dev_target_t dev_tgt = {0};
        uint32_t sess_hdl = 0;
        int a_res, e_res = 0;

	EXPECT_GLOBAL_CALL(port_cfg_table_add, port_cfg_table_add(_,_,_))
		.Times(1).
		WillOnce(Return(BF_SUCCESS));

        a_res = ff_mgr_ent_add(sess_hdl, dev_tgt, table_name, &key_spec, &data_spec);

	ASSERT_EQ(a_res, e_res);
}

 TEST(fixedFunctionConfigTable, case3) {
        struct fixed_function_data_spec data_spec = {0};
        struct fixed_function_key_spec  key_spec  = {0};
	const char *table_name = "vport.test_table";
        struct bf_dev_target_t dev_tgt = {0};
        uint32_t sess_hdl = 0;
        int a_res, e_res = 0;

        a_res = ff_mgr_ent_add(sess_hdl, dev_tgt, table_name, &key_spec, &data_spec);

	ASSERT_EQ(a_res, e_res);
}

/* test case covers invalid table name scenario, "." is expected in table name */
TEST(fixedFunctionConfigTable, case4) {
        struct fixed_function_data_spec data_spec = {0};
        struct fixed_function_key_spec  key_spec  = {0};
	const char *table_name_1 = "invalid.sad_entry";
	const char *table_name   = "invalid_table";
        struct bf_dev_target_t dev_tgt = {0};
        uint32_t sess_hdl = 0;
        int a_res, e_res = BF_INVALID_ARG;

        a_res = ff_mgr_ent_add(sess_hdl, dev_tgt, table_name, &key_spec, &data_spec);

	EXPECT_EQ(a_res, e_res);

        a_res = ff_mgr_ent_add(sess_hdl, dev_tgt, table_name_1, &key_spec, &data_spec);
	EXPECT_EQ(a_res, e_res);
}

/* ff_mgr_ent_del */
/* case5 - test ipsec-offload/crypto manager */
TEST(fixedFunctionConfigTable, case5) {
	const char *table_name =   "ipsec_offload.ipsec-offload.sad.sad-entry.ipsec-sa-config";
	const char *table_name_1 = "ipsec-offload.ipsec-offload.sad.sad-entry.ipsec-sa-config";
        struct fixed_function_key_spec  key_spec  = {0};
        struct bf_dev_target_t dev_tgt = {0};
        uint32_t sess_hdl = 0;
        int a_res, e_res = 0;

        a_res = ff_mgr_ent_del(sess_hdl, dev_tgt, table_name, &key_spec);

	EXPECT_EQ(a_res, e_res);

        a_res = ff_mgr_ent_del(sess_hdl, dev_tgt, table_name_1, &key_spec);
	ASSERT_EQ(a_res, e_res);
}

/* case6 - test port manager */
TEST(fixedFunctionConfigTable, case6) {
        struct fixed_function_key_spec  key_spec  = {0};
	const char *table_name = "port.port";
        struct bf_dev_target_t dev_tgt = {0};
        uint32_t sess_hdl = 0;
        int a_res, e_res = 0;

        a_res = ff_mgr_ent_del(sess_hdl, dev_tgt, table_name, &key_spec);

	ASSERT_EQ(a_res, e_res);
}

/* case7 - test vport manager */
TEST(fixedFunctionConfigTable, case7) {
        struct fixed_function_key_spec  key_spec  = {0};
	const char *table_name = "vport.temp";
        struct bf_dev_target_t dev_tgt = {0};
        uint32_t sess_hdl = 0;
        int a_res, e_res = 0;

        a_res = ff_mgr_ent_del(sess_hdl, dev_tgt, table_name, &key_spec);

	ASSERT_EQ(a_res, e_res);
}

TEST(fixedFunctionConfigTable, case8) {
        struct fixed_function_key_spec  key_spec  = {0};
	const char *table_name_1 = "invalid.sad_entry";
	const char *table_name   = "invalid_table";
        struct bf_dev_target_t dev_tgt = {0};
        int a_res, e_res = BF_INVALID_ARG;
        uint32_t sess_hdl = 0;

        a_res = ff_mgr_ent_del(sess_hdl, dev_tgt, table_name, &key_spec);

	EXPECT_EQ(a_res, e_res);

        a_res = ff_mgr_ent_del(sess_hdl, dev_tgt, table_name, &key_spec);
	EXPECT_EQ(a_res, e_res);
}

/* ff_mgr_ent_get_default_entry */
TEST(fixedFunctionConfigTable, case9) {
	const char *table_name_1 = "ipsec-offload.ipsec-offload.sad.sad-entry.ipsec-sa-config";
	const char *table_name   = "ipsec_offload.ipsec-offload.sad.sad-entry.ipsec-sa-config";
	const char *table_name_invalid   = "invalid_table";
        struct fixed_function_data_spec data_spec = {0};
	const char *table_name_vport = "vport.port";
	const char *table_name_port = "port.port";
        struct bf_dev_target_t dev_tgt = {0};
        uint32_t sess_hdl = 0;
        int a_res, e_res = 0;

	/* test for crypto manager */
	/* test for port manager which is not supported */
	e_res = BF_NOT_SUPPORTED;
        a_res = ff_mgr_ent_get_default_entry(sess_hdl, dev_tgt, table_name, &data_spec);

	EXPECT_EQ(a_res, e_res);

        a_res = ff_mgr_ent_get_default_entry(sess_hdl, dev_tgt, table_name, &data_spec);

	EXPECT_EQ(a_res, e_res);

	/* test for port manager which is not supported */
	e_res = BF_NOT_SUPPORTED;
        a_res = ff_mgr_ent_get_default_entry(sess_hdl, dev_tgt, table_name_port, &data_spec);

	EXPECT_EQ(a_res, e_res);

	/* test for vort manager which is not supported */
	e_res = BF_NOT_SUPPORTED;
        a_res = ff_mgr_ent_get_default_entry(sess_hdl, dev_tgt, table_name_vport, &data_spec);

	EXPECT_EQ(a_res, e_res);

	/* test for invalid backend manager */
	e_res = BF_INVALID_ARG;
        a_res = ff_mgr_ent_get_default_entry(sess_hdl, dev_tgt, table_name_invalid, &data_spec);

	EXPECT_EQ(a_res, e_res);
}

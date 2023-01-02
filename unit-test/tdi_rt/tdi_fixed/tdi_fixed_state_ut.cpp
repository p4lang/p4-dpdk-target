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

#include "tdi_fixed_state_ut.hpp"
#include "tdi_fixed_table_data_impl.cpp"
#include "tdi_fixed_table_key_impl.cpp"
#include "tdi_fixed_table_impl.cpp"

namespace tdi {
namespace pna {
namespace rt {

using namespace std;
using ::testing::Return;
using ::testing::_;

/**
 * @brief Test table.entryAdd()
 * Test whether the rule addition is successful.
 */
tdi_status_t FixedFunctionStateTableTest::table_state_get(const tdi::Table &table)
{
	std::unique_ptr<tdi::TableKey> key_hdl;
	std::unique_ptr<tdi::TableData> data_hdl;

	auto table_type = static_cast<tdi_rt_table_type_e>(table.tableInfoGet()->tableTypeGet());

	EXPECT_EQ(table_type, TDI_RT_TABLE_TYPE_FIXED_FUNC_STATE);

	//allocate key
	table.keyAllocate(&key_hdl);
	//allocate data
	table.dataAllocate(&data_hdl);

	auto data = reinterpret_cast<tdi::TableData *>(data_hdl.release());

	auto tableInfo = table.tableInfoGet();
	/* TODO - currently hard coding the port key and data params.
	 * It should be extended to read key/data params from test suite params.
	 */
	//prepare key spec and set
	{
		//retreive key field value from string name
		std::string key_name = "DEV_PORT";
		uint64_t value  = 3;
		auto keyFieldInfo = tableInfo->keyFieldGet(key_name.c_str());

		EXPECT_TRUE(keyFieldInfo != nullptr);
		tdi_id_t field_id = keyFieldInfo->idGet();
		tdi::KeyFieldValueExact <const uint64_t> keyFieldValue(value);

		EXPECT_EQ(key_hdl->setValue(field_id, keyFieldValue), TDI_SUCCESS);
	}

        EXPECT_GLOBAL_CALL(fixed_func_mgr_get_stats, fixed_func_mgr_get_stats(_,_,_,_,_))
                .Times(1).
                WillOnce(Return(BF_NO_SYS_RESOURCES));
	//get stats
	EXPECT_EQ(table.entryGet((*session), (*target), (*flags), (*key_hdl), (data)), BF_NO_SYS_RESOURCES);

        EXPECT_GLOBAL_CALL(fixed_func_mgr_get_stats, fixed_func_mgr_get_stats(_,_,_,_,_))
                .Times(1).
                WillOnce(Return(BF_SUCCESS));

	//get stats
	EXPECT_EQ(table.entryGet((*session), (*target), (*flags), (*key_hdl), (data)), TDI_SUCCESS);

	//retrieve key from key_spec
	{
		//retreive key field value from string name
		std::string key_name = "DEV_PORT";
		uint64_t value = 0;
		auto keyFieldInfo = tableInfo->keyFieldGet(key_name.c_str());

		EXPECT_TRUE(keyFieldInfo != nullptr);
		tdi_id_t field_id = keyFieldInfo->idGet();
		tdi::KeyFieldValueExact <const uint64_t> keyFieldValue(value);

		EXPECT_EQ(key_hdl->getValue(field_id, &keyFieldValue), TDI_SUCCESS);
		value = keyFieldValue.value_;
	}
	//retreive RX Stats from data spec
	{
		std::string key_name = "RX_BYTES";
		uint64_t value = 0;

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data->getValue(field_id, &value), TDI_SUCCESS);
	}

	//free up the memory
	delete (data);

	return TDI_SUCCESS;
}

TEST_P(FixedFunctionStateTableTest, StatGet) {
	int a_res = 0;
	int b_res = 0;

	if (!strcmp(prog_name.c_str(), "fixed_port")) {
		std::string table_name = "port.PORT_STAT";
		const tdi::Table *ptable;

		ASSERT_EQ(tdiInfo->tableFromNameGet(table_name.c_str(), &ptable), TDI_SUCCESS);

		ASSERT_EQ(table_state_get(*ptable), TDI_SUCCESS);
	} else {
	   EXPECT_TRUE(1);
	}
}

} //namespace rt
} //namespace pna
} //namespace tdi

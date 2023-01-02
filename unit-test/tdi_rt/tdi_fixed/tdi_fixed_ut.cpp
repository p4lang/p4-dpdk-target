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

#include "tdi_fixed_ut.hpp"
#include "tdi_fixed_table_data_impl.cpp"
#include "tdi_fixed_table_key_impl.cpp"
#include "tdi_fixed_table_impl.cpp"

namespace tdi {
namespace pna {
namespace rt {

using namespace std;
using ::testing::Return;
using ::testing::_;

void test_notif_cb(uint32_t dev_id,
                   uint32_t ipsec_sa_api,
                   bool soft_lifetime_expire,
                   uint8_t ipsec_sa_protocol,
                   char *ipsec_sa_dest_address,
                   bool ipv4,
                   void *cookie) {
}

/**
 * @brief Test table.entryAdd()
 * Test whether the rule addition is successful.
 */
tdi_status_t FixedFunctionConfigTableTest::table_entry_add(const tdi::Table &table)
{
	std::unique_ptr<tdi::TableKey> key_hdl;
	std::unique_ptr<tdi::TableData> data_hdl;

	auto table_type = static_cast<tdi_rt_table_type_e>(table.tableInfoGet()->tableTypeGet());

	EXPECT_EQ(table_type, TDI_RT_TABLE_TYPE_FIXED_FUNC);

	//allocate key
	table.keyAllocate(&key_hdl);
	//allocate data
	table.dataAllocate(&data_hdl);

	auto tableInfo = table.tableInfoGet();
	/* TODO - currently hard coding the port key and data params.
	 * It should be extended to read key/data params from test suite params.
	 */
	//prepare key spec and set
	{
		//retreive key field value from string name
		std::string key_name = "DEV_PORT";
		uint64_t value  = 2;
		auto keyFieldInfo = tableInfo->keyFieldGet(key_name.c_str());

		EXPECT_TRUE(keyFieldInfo != nullptr);
		tdi_id_t field_id = keyFieldInfo->idGet();
		tdi::KeyFieldValueExact <const uint64_t> keyFieldValue(value);

		EXPECT_EQ(key_hdl->setValue(field_id, keyFieldValue), TDI_SUCCESS);
	}
	//prepare and set data spec
	{
		std::string key_name = "PORT_TYPE";
		std::string value    = "tap";

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data_hdl->setValue(field_id, value), TDI_SUCCESS);
	}
	{
		std::string key_name = "PORT_DIR";
		std::string value    = "default";

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data_hdl->setValue(field_id, value), TDI_SUCCESS);
	}
	{
		std::string key_name = "PORT_IN_ID";
		uint64_t value  = 0;

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data_hdl->setValue(field_id, value), TDI_SUCCESS);
	}
	{
		std::string key_name = "PORT_OUT_ID";
		uint64_t value  = 0;

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data_hdl->setValue(field_id, value), TDI_SUCCESS);
	}
	{
		std::string key_name = "PIPE_IN";
		std::string value    = "pipe";

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data_hdl->setValue(field_id, value), TDI_SUCCESS);
	}
	{
		std::string key_name = "PIPE_OUT";
		std::string value    = "pipe";

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data_hdl->setValue(field_id, value), TDI_SUCCESS);
	}
	{
		std::string key_name = "MEMPOOL";
		std::string value    = "MEMPOOL0";

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data_hdl->setValue(field_id, value), TDI_SUCCESS);
	}
	{
		std::string key_name = "PORT_NAME";
		std::string value    = "TAP0";

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data_hdl->setValue(field_id, value), TDI_SUCCESS);
	}
	{
		std::string key_name = "MTU";
		uint64_t value    =  1500;

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data_hdl->setValue(field_id, value), TDI_SUCCESS);
	}

        EXPECT_GLOBAL_CALL(fixed_func_mgr_ent_add, fixed_func_mgr_ent_add(_,_,_,_,_))
                .Times(1).
                WillOnce(Return(BF_NO_SYS_RESOURCES));
	//add an entry
	EXPECT_EQ(table.entryAdd((*session), (*target), (*flags), (*key_hdl), (*data_hdl)), BF_NO_SYS_RESOURCES);

        EXPECT_GLOBAL_CALL(fixed_func_mgr_ent_add, fixed_func_mgr_ent_add(_,_,_,_,_))
                .Times(1).
                WillOnce(Return(BF_SUCCESS));

	//add an entry
	EXPECT_EQ(table.entryAdd((*session), (*target), (*flags), (*key_hdl), (*data_hdl)), TDI_SUCCESS);

	//following test to retreive and match value set above
	{
		//retreive key field value from string name
		std::string key_name = "DEV_PORT";
		uint64_t e_value    =  2;
		uint64_t value = 0;
		auto keyFieldInfo = tableInfo->keyFieldGet(key_name.c_str());

		EXPECT_TRUE(keyFieldInfo != nullptr);
		tdi_id_t field_id = keyFieldInfo->idGet();
		tdi::KeyFieldValueExact <const uint64_t> keyFieldValue(value);

		EXPECT_EQ(key_hdl->getValue(field_id, &keyFieldValue), TDI_SUCCESS);
		value = keyFieldValue.value_;
		EXPECT_EQ(value, e_value);
	}
	{
		std::string key_name = "MTU";
		uint64_t e_value    =  1500;
		uint64_t mtu        =  0;

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data_hdl->getValue(field_id, &mtu), TDI_SUCCESS);
		EXPECT_EQ(mtu, e_value);
	}
	{
		std::string key_name = "PORT_TYPE";
		std::string e_value   = "tap";
		std::string value;

		//retrieve key field info for a field name
		auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
		EXPECT_TRUE(dataFieldInfo != nullptr);

		tdi_id_t field_id = dataFieldInfo->idGet();

		EXPECT_EQ(data_hdl->getValue(field_id, &value), TDI_SUCCESS);
	}

	return TDI_SUCCESS;
}

/**
 * @brief Test table.entryDel()
 * Test whether the rule deletion is successful.
 */
tdi_status_t FixedFunctionConfigTableTest::table_entry_delete(const tdi::Table &table)
{
	std::unique_ptr<tdi::TableKey> key_hdl;

	auto table_type = static_cast<tdi_rt_table_type_e>(table.tableInfoGet()->tableTypeGet());

	EXPECT_EQ(table_type, TDI_RT_TABLE_TYPE_FIXED_FUNC);

	//allocate key
	table.keyAllocate(&key_hdl);

	auto tableInfo = table.tableInfoGet();

	//prepare key spec and set
	{
		//retreive key field value from string name
		std::string key_name = "offload-id";
		uint64_t value  = 1;
		auto keyFieldInfo = tableInfo->keyFieldGet(key_name.c_str());

		EXPECT_TRUE(keyFieldInfo != nullptr);
		tdi_id_t field_id = keyFieldInfo->idGet();
		tdi::KeyFieldValueExact <const uint64_t> keyFieldValue(value);

		EXPECT_EQ(key_hdl->setValue(field_id, keyFieldValue), TDI_SUCCESS);
	}
	{
		//retreive key field value from string name
		std::string key_name = "direction";
		uint64_t value  = 1;
		auto keyFieldInfo = tableInfo->keyFieldGet(key_name.c_str());

		EXPECT_TRUE(keyFieldInfo != nullptr);
		tdi_id_t field_id = keyFieldInfo->idGet();
		tdi::KeyFieldValueExact <const uint64_t> keyFieldValue(value);
		EXPECT_EQ(key_hdl->setValue(field_id, keyFieldValue), TDI_SUCCESS);
	}

        EXPECT_GLOBAL_CALL(fixed_func_mgr_ent_del, fixed_func_mgr_ent_del(_,_,_,_))
                .Times(1).
                WillOnce(Return(BF_NO_SYS_RESOURCES));
	
	//delete an entry
	EXPECT_EQ(table.entryDel((*session), (*target), (*flags), (*key_hdl)), BF_NO_SYS_RESOURCES);

        EXPECT_GLOBAL_CALL(fixed_func_mgr_ent_del, fixed_func_mgr_ent_del(_,_,_,_))
                .Times(1).
                WillOnce(Return(BF_SUCCESS));

	//delete an entry
	EXPECT_EQ(table.entryDel((*session), (*target), (*flags), (*key_hdl)), TDI_SUCCESS);

	return TDI_SUCCESS;
}

/**
 * @brief Test table.defaultEntryGet()
 * Test whether default get entry is successful.
 */
tdi_status_t FixedFunctionConfigTableTest::table_entry_default_get(const tdi::Table &table)
{
	std::unique_ptr<tdi::TableData> data_hdl;

	auto table_type = static_cast<tdi_rt_table_type_e>(table.tableInfoGet()->tableTypeGet());

	EXPECT_EQ(table_type, TDI_RT_TABLE_TYPE_FIXED_FUNC);

	//allocate data
	table.dataAllocate(&data_hdl);

	auto data = reinterpret_cast<tdi::TableData *>(data_hdl.release());

        EXPECT_GLOBAL_CALL(fixed_func_mgr_get_default_entry, fixed_func_mgr_get_default_entry(_,_,_,_))
                .Times(1).
                WillOnce(Return(TDI_NOT_SUPPORTED));

	//get default entry
	EXPECT_EQ(table.defaultEntryGet((*session), (*target), (*flags), (data)), TDI_NOT_SUPPORTED);

        EXPECT_GLOBAL_CALL(fixed_func_mgr_get_default_entry, fixed_func_mgr_get_default_entry(_,_,_,_))
                .Times(1).
                WillOnce(Return(BF_SUCCESS));

	//get default entry
	EXPECT_EQ(table.defaultEntryGet((*session), (*target), (*flags), (data)), TDI_SUCCESS);
	//free up the memory
	delete (data);
	return TDI_SUCCESS;
}

tdi_status_t FixedFunctionConfigTableTest::table_attributes_set(const tdi::Table &table)
{
	auto table_type = static_cast<tdi_rt_table_type_e>(table.tableInfoGet()->tableTypeGet());

	EXPECT_EQ(table_type, TDI_RT_TABLE_TYPE_FIXED_FUNC);

	// tdi_attributes_allocate
	std::unique_ptr<tdi::TableAttributes> attributes_field;
	tdi::TableAttributes *attributes_object;
	auto bf_status = table.attributeAllocate((tdi_attributes_type_e) TDI_RT_ATTRIBUTES_TYPE_IPSEC_SADB_EXPIRE_NOTIF, 
			&attributes_field);
	EXPECT_EQ(bf_status, TDI_SUCCESS);

	attributes_object = attributes_field.get();
	// tdi_attributes_set_value
	const uint64_t value = 1;
	bf_status = attributes_object->setValue(
			(tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_ENABLE,
			value);
	EXPECT_EQ(bf_status, TDI_SUCCESS);
	bf_status = attributes_object->setValue(
			(tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_CALLBACK_C,
			(uint64_t)test_notif_cb);
	EXPECT_EQ(bf_status, TDI_SUCCESS);

	const uint64_t value0 = 3;
	bf_status = attributes_object->setValue(
			(tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_COOKIE,
			value0);
	EXPECT_EQ(bf_status, TDI_SUCCESS);

	// tdi_table_attributes_set
	bf_status = table.tableAttributesSet(*session,*target,*flags,*attributes_object);
	EXPECT_EQ(table.tableAttributesSet((*session), (*target), (*flags), (*attributes_object)), TDI_SUCCESS);
	
	return TDI_SUCCESS;
}

tdi_status_t FixedFunctionConfigTableTest::table_attributes_get(const tdi::Table &table)
{
	auto table_type = static_cast<tdi_rt_table_type_e>(table.tableInfoGet()->tableTypeGet());

	EXPECT_EQ(table_type, TDI_RT_TABLE_TYPE_FIXED_FUNC);

	std::unique_ptr<tdi::TableAttributes> attributes_field;
	tdi::TableAttributes *attributes_object;
	auto bf_status = table.attributeAllocate((tdi_attributes_type_e) TDI_RT_ATTRIBUTES_TYPE_IPSEC_SADB_EXPIRE_NOTIF, &attributes_field);
	EXPECT_EQ(bf_status, TDI_SUCCESS);

	// tdi_table_attributes_get
	attributes_object = attributes_field.get();

	table.tableAttributesGet(*session,*target,*flags,attributes_field.get());
	EXPECT_EQ(table.tableAttributesGet((*session), (*target), (*flags), (attributes_field.get())), TDI_SUCCESS);

	attributes_object = attributes_field.get();
	// tdi_attributes_set_value
	uint64_t value = 1;
	bf_status = attributes_object->getValue(
			(tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_ENABLE,
			&value);
	EXPECT_EQ(bf_status, TDI_SUCCESS);
	bf_status = attributes_object->getValue(
			(tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_CALLBACK_C,
			&value);
	EXPECT_EQ(bf_status, TDI_SUCCESS);

	uint64_t value0 = 3;
	bf_status = attributes_object->getValue(
			(tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_COOKIE,
			&value0);
	EXPECT_EQ(bf_status, TDI_SUCCESS);

	return TDI_SUCCESS;
}

TEST_P(FixedFunctionConfigTableTest, EntryAdd) {
	int a_res = 0;
	int b_res = 0;

	if (!strcmp(prog_name.c_str(), "fixed_port")) {
		std::string table_name = "port.PORT";
		const tdi::Table *ptable;

		ASSERT_EQ(tdiInfo->tableFromNameGet(table_name.c_str(), &ptable), TDI_SUCCESS);

		ASSERT_EQ(table_entry_add(*ptable), TDI_SUCCESS);
	} else {
	   EXPECT_TRUE(1);
	}
}

TEST_P(FixedFunctionConfigTableTest, EntryDelete) {
	int a_res = 0;
	int b_res = 0;

	if (!strcmp(prog_name.c_str(), "fixed_crypto")) {
		std::string table_name = "ipsec-offload.ipsec-offload.sad.sad-entry.ipsec-sa-config";
		const tdi::Table *ptable;

		ASSERT_EQ(tdiInfo->tableFromNameGet(table_name.c_str(), &ptable), TDI_SUCCESS);

		ASSERT_EQ(table_entry_delete(*ptable), TDI_SUCCESS);
	} else {
	   EXPECT_TRUE(1);
	}
}

TEST_P(FixedFunctionConfigTableTest, EntryDefaultGet) {
	int a_res = 0;
	int b_res = 0;

	if (!strcmp(prog_name.c_str(), "fixed_crypto")) {
		std::string table_name = "ipsec-offload.ipsec-offload.ipsec-spi";
		const tdi::Table *ptable;

		ASSERT_EQ(tdiInfo->tableFromNameGet(table_name.c_str(), &ptable), TDI_SUCCESS);

		ASSERT_EQ(table_entry_default_get(*ptable), TDI_SUCCESS);
	} else {
	   EXPECT_TRUE(1);
	}
}

TEST_P(FixedFunctionConfigTableTest, tableAttributesSet) {
	int a_res = 0;
	int b_res = 0;

	if (!strcmp(prog_name.c_str(), "fixed_crypto")) {
		std::string table_name = "ipsec-offload";
		const tdi::Table *ptable;

		ASSERT_EQ(tdiInfo->tableFromNameGet(table_name.c_str(), &ptable), TDI_SUCCESS);

		ASSERT_EQ(table_attributes_set(*ptable), TDI_SUCCESS);
	} else {
	   EXPECT_TRUE(1);
	}
}

TEST_P(FixedFunctionConfigTableTest, tableAttributesGet) {
	int a_res = 0;
	int b_res = 0;

	if (!strcmp(prog_name.c_str(), "fixed_crypto")) {
		std::string table_name = "ipsec-offload";
		const tdi::Table *ptable;

		ASSERT_EQ(tdiInfo->tableFromNameGet(table_name.c_str(), &ptable), TDI_SUCCESS);

		ASSERT_EQ(table_attributes_get(*ptable), TDI_SUCCESS);
	} else {
	   EXPECT_TRUE(1);
	}
}

TEST_P(FixedFunctionConfigTableTest, tableSizeGet) {
	int a_res = 0;
	int b_res = 0;

	if (!strcmp(prog_name.c_str(), "fixed_port")) {
		std::string table_name = "port.PORT";
		const tdi::Table *ptable;
		uint64_t count = 0;

		ASSERT_EQ(tdiInfo->tableFromNameGet(table_name.c_str(), &ptable), TDI_SUCCESS);

		ASSERT_EQ(ptable->sizeGet(*session,*target,*flags,&count), TDI_SUCCESS);
	} else {
	   EXPECT_TRUE(1);
	}
}

TEST_P(FixedFunctionConfigTableTest, EntryMod) {
	int a_res = 0;
	int b_res = 0;

	if (!strcmp(prog_name.c_str(), "fixed_port")) {
		std::string table_name = "port.PORT";
		const tdi::Table *ptable;
		std::unique_ptr<tdi::TableKey> key_hdl;
		std::unique_ptr<tdi::TableData> data_hdl;

		ASSERT_EQ(tdiInfo->tableFromNameGet(table_name.c_str(), &ptable), TDI_SUCCESS);
		//allocate key and data
		ptable->keyAllocate(&key_hdl);
		ptable->dataAllocate(&data_hdl);

		ASSERT_EQ(ptable->entryMod(*session,*target,*flags,(*key_hdl), (*data_hdl)), TDI_NOT_SUPPORTED);
	} else {
		EXPECT_TRUE(1);
	}
}

} //namespace rt
} //namespace pna
} //namespace tdi

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

#include "tdi_reg_ut.hpp"
#include "tdi_p4_table_data_impl.cpp"
#include "tdi_p4_table_key_impl.cpp"
#include "tdi_p4_table_impl.cpp"

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

tdi_status_t RegisterTableTest::table_entry_mod(const tdi::Table &table)
{
  std::unique_ptr<tdi::TableKey> key_hdl;
  std::unique_ptr<tdi::TableData> data_hdl;

  auto table_type = static_cast<tdi_rt_table_type_e>(table.tableInfoGet()->tableTypeGet());

  EXPECT_EQ(table_type, TDI_RT_TABLE_TYPE_REGISTER);

  //allocate key
  table.keyAllocate(&key_hdl);
  //allocate data
  table.dataAllocate(&data_hdl);

  auto tableInfo = table.tableInfoGet();
  //prepare key spec and set
  {
    //retrieve key field value from string name
    std::string key_name = "$REGISTER_INDEX";
    uint64_t value  = 1;
    auto keyFieldInfo = tableInfo->keyFieldGet(key_name.c_str());

    EXPECT_TRUE(keyFieldInfo != nullptr);
    tdi_id_t field_id = keyFieldInfo->idGet();
    tdi::KeyFieldValueExact <const uint64_t> keyFieldValue(value);

    EXPECT_EQ(key_hdl->setValue(field_id, keyFieldValue), TDI_SUCCESS);
  }
  //prepare and set data spec
  {
    std::string key_name = "$REGISTER_SPEC";
    uint64_t value    = 100;

    //retrieve key field info for a field name
    auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
    EXPECT_TRUE(dataFieldInfo != nullptr);

    tdi_id_t field_id = dataFieldInfo->idGet();

    EXPECT_EQ(data_hdl->setValue(field_id, value), TDI_SUCCESS);
  }
  EXPECT_GLOBAL_CALL(pipe_stful_ent_set, pipe_stful_ent_set(_,_,_,_,_,_,_))
    .Times(1).
    WillOnce(Return(BF_NO_SYS_RESOURCES));
  EXPECT_EQ(table.entryMod((*session), (*target), (*flags), (*key_hdl), (*data_hdl)), BF_NO_SYS_RESOURCES);

  EXPECT_GLOBAL_CALL(pipe_stful_ent_set, pipe_stful_ent_set(_,_,_,_,_,_,_))
    .Times(1).
    WillOnce(Return(BF_SUCCESS));
  EXPECT_EQ(table.entryMod((*session), (*target), (*flags), (*key_hdl), (*data_hdl)), TDI_SUCCESS);

  // test setValue
  //prepare key spec and set
  {
    //retrieve key field value from string name
    std::string key_name = "$REGISTER_INDEX";
    uint8_t *value;
    int size = 8;
    auto keyFieldInfo = tableInfo->keyFieldGet(key_name.c_str());

    EXPECT_TRUE(keyFieldInfo != nullptr);
    tdi_id_t field_id = keyFieldInfo->idGet();
    tdi::KeyFieldValueExact <const uint8_t *> keyFieldValue(value, size);

    EXPECT_EQ(key_hdl->setValue(field_id, keyFieldValue), TDI_INVALID_ARG);
  }
  {
    //retrieve key field value from string name
    std::string key_name = "$REGISTER_INDEX";
    uint8_t *value;
    int size = sizeof(uint32_t);
    auto keyFieldInfo = tableInfo->keyFieldGet(key_name.c_str());

    EXPECT_TRUE(keyFieldInfo != nullptr);
    tdi_id_t field_id = keyFieldInfo->idGet();
    tdi::KeyFieldValueExact <const uint8_t *> keyFieldValue(value, size);

    EXPECT_EQ(key_hdl->setValue(field_id, keyFieldValue), TDI_SUCCESS);
  }
  // Test data setvalue
  {
    std::string key_name = "$REGISTER_SPEC";
    uint8_t *value;
    int size = sizeof(uint32_t);

    //retrieve key field info for a field name
    auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
    EXPECT_TRUE(dataFieldInfo != nullptr);

    tdi_id_t field_id = dataFieldInfo->idGet();

    EXPECT_EQ(data_hdl->setValue(field_id, value, size), TDI_INVALID_ARG);
  }
  // test data getValue
  {
    std::string key_name = "$REGISTER_SPEC";
    uint8_t *value;
    int size = sizeof(uint32_t);

    //retrieve key field info for a field name
    auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
    EXPECT_TRUE(dataFieldInfo != nullptr);

    tdi_id_t field_id = dataFieldInfo->idGet();

    EXPECT_EQ(data_hdl->getValue(field_id, value, size), TDI_INVALID_ARG);
  }
  {
    std::string key_name = "$REGISTER_SPEC";
    std::vector<uint64_t> value;
    //retrieve key field info for a field name
    auto dataFieldInfo = tableInfo->dataFieldGet(key_name.c_str());
    EXPECT_TRUE(dataFieldInfo != nullptr);

    tdi_id_t field_id = dataFieldInfo->idGet();

    EXPECT_EQ(data_hdl->getValue(field_id, &value), TDI_SUCCESS);

  }
  {
    //retrieve key field value from string name
    std::string key_name = "$REGISTER_INDEX";
    uint64_t value;
    auto keyFieldInfo = tableInfo->keyFieldGet(key_name.c_str());

    EXPECT_TRUE(keyFieldInfo != nullptr);
    tdi_id_t field_id = keyFieldInfo->idGet();
    tdi::KeyFieldValueExact <const uint64_t> keyFieldValue(value);

    EXPECT_EQ(key_hdl->getValue(field_id, &keyFieldValue), TDI_SUCCESS);
  }
  {
    std::string key_name = "$REGISTER_INDEX";
    uint8_t *value;
    int size = 8;
    auto keyFieldInfo = tableInfo->keyFieldGet(key_name.c_str());
    EXPECT_TRUE(keyFieldInfo != nullptr);
    tdi_id_t field_id = keyFieldInfo->idGet();
    tdi::KeyFieldValueExact <uint8_t *> keyFieldValue(value, size);
    EXPECT_EQ(key_hdl->getValue(field_id, &keyFieldValue), TDI_INVALID_ARG);
  }
  return TDI_SUCCESS;
}

tdi_status_t RegisterTableTest::table_entry_get(const tdi::Table &table)
{
  std::unique_ptr<tdi::TableData> data_hdl;
  std::unique_ptr<tdi::TableKey> key_hdl;
  auto table_type = static_cast<tdi_rt_table_type_e>(table.tableInfoGet()->tableTypeGet());
  EXPECT_EQ(table_type, TDI_RT_TABLE_TYPE_REGISTER);
  table.keyAllocate(&key_hdl);
  table.dataAllocate(&data_hdl);

  tdi_handle_t entry_handle = 1;
  auto data = reinterpret_cast<tdi::TableData *>(data_hdl.release());
  EXPECT_GLOBAL_CALL(pipe_stful_ent_query, pipe_stful_ent_query(_,_,_,_,_,_,_))
    .Times(2).
    WillOnce(Return(TDI_NOT_SUPPORTED)).
    WillOnce(Return(TDI_NOT_SUPPORTED));

  auto tableInfo = table.tableInfoGet();
  // get call to table
  EXPECT_EQ(table.entryGet((*session), (*target), (*flags),(*key_hdl), (data)), TDI_SUCCESS);

  // handleGet
  EXPECT_EQ(table.entryHandleGet((*session), (*target), (*flags), (*key_hdl), (&entry_handle)), TDI_SUCCESS);
  auto key  = reinterpret_cast<tdi::TableKey *>(key_hdl.release());
  EXPECT_EQ(table.entryGetFirst((*session), (*target), (*flags), (key), (data)), TDI_SUCCESS);
  // clear the table
  EXPECT_EQ(table.clear((*session), (*target), (*flags)), TDI_SUCCESS);
  // key and date Reset
  EXPECT_EQ(table.keyReset(key), TDI_SUCCESS);

  EXPECT_EQ(table.dataReset(data), TDI_SUCCESS);
  return TDI_SUCCESS;
}
TEST_P(RegisterTableTest, EntryMod) {
  int a_res = 0;
  int b_res = 0;

  if (!strcmp(prog_name.c_str(), "register")) {
    std::string table_name = "ip.reg";
    const tdi::Table *ptable;

    ASSERT_EQ(tdiInfo->tableFromNameGet(table_name.c_str(), &ptable), TDI_SUCCESS);

    ASSERT_EQ(table_entry_mod(*ptable), TDI_SUCCESS);
  } else {
     EXPECT_TRUE(1);
  }
}

TEST_P(RegisterTableTest, EntryGet) {
  int a_res = 0;
  int b_res = 0;

  if (!strcmp(prog_name.c_str(), "register")) {
    std::string table_name = "ip.reg";
    const tdi::Table *ptable;
    std::unique_ptr<tdi::TableKey> key_hdl;
    std::unique_ptr<tdi::TableData> data_hdl;

    ASSERT_EQ(tdiInfo->tableFromNameGet(table_name.c_str(), &ptable), TDI_SUCCESS);

    ptable->keyAllocate(&key_hdl);
    ptable->dataAllocate(&data_hdl);

    ASSERT_EQ(table_entry_get(*ptable), TDI_SUCCESS);
  } else {
     EXPECT_TRUE(1);
  }
}

}//namespace rt
}//namespace pna
}//namespace tdi

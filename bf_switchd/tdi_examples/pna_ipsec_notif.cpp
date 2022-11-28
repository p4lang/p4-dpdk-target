extern "C" {
#include "common.h"
}

//#include <bf_rt/bf_rt.hpp>
#include "tdi/common/tdi_defs.h"
#include "tdi_rt/tdi_rt_defs.h"
#include "tdi/common/tdi_info.hpp"
#include "tdi/common/tdi_table.hpp"
#include "tdi/common/tdi_table_key.hpp"
#include "tdi/common/tdi_defs.h"
#include "tdi/common/tdi_init.hpp"
#include "tdi/common/tdi_json_parser/tdi_table_info.hpp"
#include <tdi/common/tdi_session.hpp>
#include <tdi/common/tdi_attributes.hpp>
#include <tdi_rt/tdi_rt_attributes.hpp>
//#include <tdi_rt/tdi_rt_table_data.hpp>
//#include <tdi_rt/tdi_rt_table_key.hpp>
//#include <tdi_rt/tdi_rt_table_operations.hpp>

#if 0
getQualifiedTableName
#endif
/***********************************************************************************
 * This sample cpp application code is based on the P4 program
 *pna_exact_match.p4
 * Please refer to the P4 program and the generated bf-rt.json for information
 *on
 * the tables contained in the P4 program, and the associated key and data
 *fields.
 **********************************************************************************/

namespace tdi {
namespace examples {
namespace pna_notif {

namespace {
// Key field ids, table data field ids, action ids, Table object required for
// interacting with the table
const tdi::TdiInfo *tdiInfo = nullptr;
const tdi::Table *notifTable = nullptr;
std::shared_ptr<tdi::Session> session;
const tdi::Device *devObj = nullptr;
//std::shared_ptr<tdi::Device> device;
std::shared_ptr<tdi::Target> target;
std::shared_ptr<tdi::Flags> flags;

std::unique_ptr<tdi::TableKey> tdiTableKey;
std::unique_ptr<tdi::TableData> tdiTableData;

#define ALL_PIPES 0xffff
//tdi_target_t dev_tgt;
}  // anonymous namespace

// This function does the initial setUp of getting tdiInfo object associated
// with the P4 program from which all other required objects are obtained
void setUp() {
#if 0
  dev_tgt.dev_id = 0;
  dev_tgt.pipe_id = ALL_PIPES;
  // Get devMgr singleton instance
#endif
  uint64_t dev_id = 0;
  auto &devMgr = tdi::DevMgr::getInstance();
  // Get tdiInfo object from dev_id and p4 program name
  // For the fixed table, we need to use the p4 program name or 
  // fixed function name
  char prog_name[256]="counter";
  std::string program_name(prog_name);
  // From the dev_id, to get devObj
  auto sts = devMgr.deviceGet(dev_id, &devObj);
  if (sts != TDI_SUCCESS) {
    printf("devMgr.deviceGet: failure!");
    return;
  }
  sts = devObj->tdiInfoGet(program_name, &tdiInfo);
  if (sts != TDI_SUCCESS) {
    printf("devObj->tdiInfoGet: failure!");
    return;
  }
#if 0
  auto bf_status =
      devMgr.tdiInfoGet(dev_id, "counter", &tdiInfo);
  // Check for status
  bf_sys_assert(bf_status == BF_SUCCESS);
#endif
  // Create a session object
  devObj->createSession(&session);
}

void test_notif_cb(uint32_t dev_id,
                   uint32_t ipsec_sa_api,
                   bool soft_lifetime_expire,
                   uint8_t ipsec_sa_protocol,
                   char *ipsec_sa_dest_address,
                   bool ipv4,
                   void *cookie) {
  printf("This is a test func:\n");
  printf("callback:: dev_id %d ipsec_sa_api %d bool = %d\nipsec_sa_protocol %d ipsec_sa_dest_address %s ipv4 = %d\ncookie %p\n",
         dev_id, ipsec_sa_api, soft_lifetime_expire, ipsec_sa_protocol, ipsec_sa_dest_address, ipv4, cookie);
}
// This function does the initial set up of getting key field-ids, action-ids
// and data field ids associated with the notif table. This is done once
// during init time.
void tableSetUp() {
  // Get table object from name
  auto bf_status =
      tdiInfo->tableFromNameGet("ipsec-offload", &notifTable);
  // tdi_attributes_allocate
  std::unique_ptr<tdi::TableAttributes> attributes_field;
  tdi::TableAttributes *attributes_object;
  bf_status = notifTable->attributeAllocate((tdi_attributes_type_e) TDI_RT_ATTRIBUTES_TYPE_IPSEC_SADB_EXPIRE_NOTIF, &attributes_field);
  bf_sys_assert(bf_status == BF_SUCCESS);

  attributes_object = attributes_field.get();
  // tdi_attributes_set_value
  const uint64_t value = 1;
  bf_status = attributes_object->setValue(
                 (tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_ENABLE,
                 value);
  bf_sys_assert(bf_status == BF_SUCCESS);
  bf_status = attributes_object->setValue(
                 (tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_CALLBACK_C,
                 (uint64_t)test_notif_cb);
  bf_sys_assert(bf_status == BF_SUCCESS);

  const uint64_t value0 = 3;
  bf_status = attributes_object->setValue(
                 (tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_COOKIE,
                 value0);
  bf_sys_assert(bf_status == BF_SUCCESS);

  // tdi_target_create
  std::unique_ptr<tdi::Target> target;
  devObj->createTarget(&target);
  const uint64_t flag_value = 0;
  // tdi_flags_create
  tdi::Flags *flags = new tdi::Flags(flag_value);
  // tdi_table_attributes_set
  notifTable->tableAttributesSet(*session,*target,*flags,*attributes_object);
}

void notifGet() {
  // Get table object from name
  auto bf_status =
      tdiInfo->tableFromNameGet("ipsec-offload", &notifTable);
  // tdi_attributes_allocate
  std::unique_ptr<tdi::TableAttributes> attributes_field;
  tdi::TableAttributes *attributes_object;
  bf_status = notifTable->attributeAllocate((tdi_attributes_type_e) TDI_RT_ATTRIBUTES_TYPE_IPSEC_SADB_EXPIRE_NOTIF, &attributes_field);
  // tdi_table_attributes_get
  attributes_object = attributes_field.get();
  std::unique_ptr<tdi::Target> target;
  devObj->createTarget(&target);
  const uint64_t flag_value = 0;
  // tdi_flags_create
  tdi::Flags *flags = new tdi::Flags(flag_value);
  notifTable->tableAttributesGet(*session,*target,*flags,attributes_field.get());
  bf_sys_assert(bf_status == BF_SUCCESS);

  attributes_object = attributes_field.get();
  // tdi_attributes_set_value
  uint64_t value = 1;
  bf_status = attributes_object->getValue(
                  (tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_ENABLE,
                  &value);
  printf("get notif result:\nenable = %ld\n", value);
  bf_sys_assert(bf_status == BF_SUCCESS);
  bf_status = attributes_object->getValue(
                  (tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_CALLBACK_C,
                  &value);
  printf("cb_c= %ld\n", value);
  bf_sys_assert(bf_status == BF_SUCCESS);

  uint64_t value0 = 3;
  bf_status = attributes_object->getValue(
                  (tdi_attributes_field_type_e) TDI_RT_ATTRIBUTES_IPSEC_SADB_EXPIRE_TABLE_FIELD_TYPE_COOKIE, 
                  &value0);
  printf("cookie= %ld\n", value0);
  bf_sys_assert(bf_status == BF_SUCCESS);
  return;
}

}  // namespace pna_notif
}  // namespace examples
}  // namespace tdi

int main(int argc, char **argv) {
  parse_opts_and_switchd_init(argc, argv);

  // Do initial set up
  tdi::examples::pna_notif::setUp();
  // Do table level set up
  tdi::examples::pna_notif::tableSetUp();

  // Do get notif
  tdi::examples::pna_notif::notifGet();
  run_cli_or_cleanup();
  return 0;
}

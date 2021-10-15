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
#ifndef _BF_RT_TABLE_OBJ_HPP
#define _BF_RT_TABLE_OBJ_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <mutex>

#include <bf_rt/bf_rt_session.hpp>
#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>
#include <bf_rt/bf_rt_table_attributes.hpp>
#include <bf_rt/bf_rt_table_operations.hpp>
#include "bf_rt_table_key_impl.hpp"
#include "bf_rt_table_data_impl.hpp"
#include "bf_rt_table_attributes_impl.hpp"
#include "bf_rt_utils.hpp"

namespace bfrt {

typedef struct key_size_ {
  size_t bytes;
  size_t bits;
} key_size_t;

/* Struct to keep info regarding a reference to a table */
typedef struct bf_rt_table_ref_info_ {
  std::string name;
  bf_rt_id_t id;
  pipe_tbl_hdl_t tbl_hdl;
  // A flag to indicate if the reference is indirect. TRUE when it is, FALSE
  // when the refernece is direct
  bool indirect_ref;
} bf_rt_table_ref_info_t;

/* Structure to keep action info */
typedef struct bf_rt_info_action_info_ {
  bf_rt_id_t action_id;
  std::string name;
  pipe_act_fn_hdl_t act_fn_hdl;
  // Map of table_data_fields
  std::map<bf_rt_id_t, std::unique_ptr<BfRtTableDataField>> data_fields;
  // Map of table_data_fields with names
  std::map<std::string, BfRtTableDataField *> data_fields_names;
  // Size of the action data in bytes
  size_t dataSz;
  // Size of the action data in bits (not including byte padding)
  size_t dataSzbits;
  std::set<Annotation> annotations{};
} bf_rt_info_action_info_t;

class BfRtTableObj : public BfRtTable {
 public:
  virtual ~BfRtTableObj() = default;

  BfRtTableObj(const std::string &program_name,
               const bf_rt_id_t &id,
               const std::string &name,
               const size_t &size,
               const TableType &type,
               const std::set<TableApi> table_apis,
               const pipe_tbl_hdl_t &pipe_hdl)
      : BfRtTableObj(program_name, id, name, size, type, table_apis) {
    this->pipe_tbl_hdl = pipe_hdl;
  };

  BfRtTableObj(const std::string &program_name,
               const bf_rt_id_t &id,
               const std::string &name,
               const size_t &size,
               const TableType &type,
               const std::set<TableApi> table_apis)
      : prog_name(program_name),
        actProfTbl(nullptr),
        selectorTbl(nullptr),
        object_id(id),
        key_size(),
        _table_size(size),
        act_prof_id(),
        selector_tbl_id(),
        object_name(name),
        object_type(type),
        table_apis_(table_apis) {}

  static std::unique_ptr<BfRtTableObj> make_table(
      const std::string &prog_name,
      const TableType &table_type,
      const bf_rt_id_t &id,
      const std::string &name,
      const size_t &size,
      const pipe_tbl_hdl_t &pipe_hdl);

  static std::unique_ptr<BfRtTableObj> make_table(const std::string &prog_name,
                                                  const TableType &table_type,
                                                  const bf_rt_id_t &id,
                                                  const std::string &name,
                                                  const size_t &size);

  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override ;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override ;

  bf_status_t tableEntryModInc(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtTableKey &key,
      const BfRtTableData &data,
      const BfRtTable::BfRtTableModIncFlag &flag) const override ;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const BfRtTableKey &key) const override ;

  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt) const override ;

  bf_status_t tableDefaultEntrySet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtTableData &data) const override ;

  bf_status_t tableDefaultEntryReset(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt) const override ;

  bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const BfRtTable::BfRtTableGetFlag &flag,
                                   BfRtTableData *data) const override ;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const BfRtTable::BfRtTableGetFlag &flag,
                            const bf_rt_handle_t &entry_handle,
                            BfRtTableKey *key,
                            BfRtTableData *data) const override ;

  bf_status_t tableEntryKeyGet(const BfRtSession &session,
                               const bf_rt_target_t &dev_tgt,
                               const bf_rt_handle_t &entry_handle,
                               bf_rt_target_t *entry_tgt,
                               BfRtTableKey *key) const override ;

  bf_status_t tableEntryHandleGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtTableKey &key,
      bf_rt_handle_t *entry_handle) const override ;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const BfRtTableKey &key,
                            const BfRtTable::BfRtTableGetFlag &flag,
                            BfRtTableData *data) const override ;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const BfRtTable::BfRtTableGetFlag &flag,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override ;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  const BfRtTable::BfRtTableGetFlag &flag,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override ;

  virtual bf_status_t tableEntryAdd(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    const uint64_t &flags,
                                    const BfRtTableKey &key,
                                    const BfRtTableData &data) const override;

  virtual bf_status_t tableEntryMod(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    const uint64_t &flags,
                                    const BfRtTableKey &key,
                                    const BfRtTableData &data) const override;

  virtual bf_status_t tableEntryModInc(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableKey &key,
      const BfRtTableData &data) const override;

  virtual bf_status_t tableEntryDel(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    const uint64_t &flags,
                                    const BfRtTableKey &key) const override;

  virtual bf_status_t tableClear(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags) const override;

  virtual bf_status_t tableDefaultEntrySet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableData &data) const override;

  virtual bf_status_t tableDefaultEntryReset(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags) const override;

  virtual bf_status_t tableDefaultEntryGet(const BfRtSession &session,
                                           const bf_rt_target_t &dev_tgt,
                                           const uint64_t &flags,
                                           BfRtTableData *data) const override;

  virtual bf_status_t tableEntryGet(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    const uint64_t &flags,
                                    const bf_rt_handle_t &entry_handle,
                                    BfRtTableKey *key,
                                    BfRtTableData *data) const override;

  virtual bf_status_t tableEntryKeyGet(const BfRtSession &session,
                                       const bf_rt_target_t &dev_tgt,
                                       const uint64_t &flags,
                                       const bf_rt_handle_t &entry_handle,
                                       bf_rt_target_t *entry_tgt,
                                       BfRtTableKey *key) const override;

  virtual bf_status_t tableEntryHandleGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableKey &key,
      bf_rt_handle_t *entry_handle) const override;

  virtual bf_status_t tableEntryGet(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    const uint64_t &flags,
                                    const BfRtTableKey &key,
                                    BfRtTableData *data) const override;

  virtual bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                         const bf_rt_target_t &dev_tgt,
                                         const uint64_t &flags,
                                         BfRtTableKey *key,
                                         BfRtTableData *data) const override;

  virtual bf_status_t tableEntryGetNext_n(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableKey &key,
      const uint32_t &n,
      keyDataPairs *key_data_pairs,
      uint32_t *num_returned) const override;

  virtual bf_status_t tableUsageGet(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    const uint64_t &flags,
                                    uint32_t *count) const override;

  virtual bf_status_t tableSizeGet(const BfRtSession &session,
                                   const bf_rt_target_t &dev_tgt,
                                   const uint64_t &flags,
                                   size_t *size) const override;

  bf_status_t tableNameGet(std::string *name) const override ;

  bf_status_t tableIdGet(bf_rt_id_t *id) const override ;

  bf_status_t tableSizeGet(const BfRtSession &session,
                           const bf_rt_target_t &dev_tgt,
                           size_t *size) const override ;

  bf_status_t tableTypeGet(TableType *type) const override ;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const BfRtTable::BfRtTableGetFlag &flag,
                            uint32_t *count) const override ;

  bf_status_t tableHasConstDefaultAction(
      bool *has_const_default_action) const override ;

  bf_status_t tableIsConst(bool *is_const) const override ;

  bf_status_t tableAnnotationsGet(
      AnnotationSet *annotations) const override ;
  bf_status_t tableApiSupportedGet(TableApiSet *tableApis) const override ;

  // Table sub-object allocate APIs
  virtual bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override;

  virtual bf_status_t keyReset(BfRtTableKey *key) const override;

  virtual bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  virtual bf_status_t dataAllocate(
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  virtual bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  virtual bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  virtual bf_status_t dataAllocateContainer(
      const bf_rt_id_t &container_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  virtual bf_status_t dataAllocateContainer(
      const bf_rt_id_t &container_id,
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  virtual bf_status_t dataAllocateContainer(
      const bf_rt_id_t &container_id,
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override;
  virtual bf_status_t dataAllocateContainer(
      const bf_rt_id_t &container_id,
      const std::vector<bf_rt_id_t> &fields,
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override;

  virtual bf_status_t dataReset(BfRtTableData *data) const override;

  virtual bf_status_t dataReset(const bf_rt_id_t &action_id,
                                BfRtTableData *data) const override;

  virtual bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                                BfRtTableData *data) const override;

  virtual bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                                const bf_rt_id_t &action_id,
                                BfRtTableData *data) const override;

  virtual bf_status_t attributeAllocate(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  virtual bf_status_t attributeAllocate(
      const TableAttributesType &type,
      const TableAttributesIdleTableMode &idle_table_type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  virtual bf_status_t attributeReset(
      const TableAttributesType &type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  virtual bf_status_t attributeReset(
      const TableAttributesType &type,
      const TableAttributesIdleTableMode &idle_table_type,
      std::unique_ptr<BfRtTableAttributes> *attr) const override;

  virtual bf_status_t operationsAllocate(
      const TableOperationsType &type,
      std::unique_ptr<BfRtTableOperations> *table_ops) const override;

  //// KeyField APIs
  bf_status_t keyFieldIdListGet(
      std::vector<bf_rt_id_t> *id_vec) const override ;

  bf_status_t keyFieldTypeGet(const bf_rt_id_t &field_id,
                              KeyFieldType *field_type) const override ;

  bf_status_t keyFieldDataTypeGet(const bf_rt_id_t &field_id,
                                  DataType *data_type) const override ;

  bf_status_t keyFieldIdGet(const std::string &name,
                            bf_rt_id_t *field_id) const override ;

  bf_status_t keyFieldSizeGet(const bf_rt_id_t &field_id,
                              size_t *size) const override ;

  bf_status_t keyFieldIsPtrGet(const bf_rt_id_t &field_id,
                               bool *is_ptr) const override ;

  bf_status_t keyFieldNameGet(const bf_rt_id_t &field_id,
                              std::string *name) const override ;

  bf_status_t keyFieldAllowedChoicesGet(
      const bf_rt_id_t &field_id,
      std::vector<std::reference_wrapper<const std::string>> *choices)
      const override ;

  //// DataField APIs
  bf_status_t dataFieldIdListGet(
      const bf_rt_id_t &action_id,
      std::vector<bf_rt_id_t> *id_vec) const override ;

  bf_status_t dataFieldIdListGet(
      std::vector<bf_rt_id_t> *id_vec) const override ;

  bf_status_t containerDataFieldIdListGet(
      const bf_rt_id_t &field_id,
      std::vector<bf_rt_id_t> *id_vec) const override ;

  bf_status_t dataFieldIdGet(const std::string &name,
                             bf_rt_id_t *field_id) const override ;

  bf_status_t dataFieldIdGet(const std::string &name,
                             const bf_rt_id_t &action_id,
                             bf_rt_id_t *field_id) const override ;

  bf_status_t dataFieldSizeGet(const bf_rt_id_t &field_id,
                               size_t *size) const override ;

  bf_status_t dataFieldSizeGet(const bf_rt_id_t &field_id,
                               const bf_rt_id_t &action_id,
                               size_t *size) const override ;

  bf_status_t dataFieldIsPtrGet(const bf_rt_id_t &field_id,
                                bool *is_ptr) const override ;

  bf_status_t dataFieldIsPtrGet(const bf_rt_id_t &field_id,
                                const bf_rt_id_t &action_id,
                                bool *is_ptr) const override ;

  bf_status_t dataFieldMandatoryGet(const bf_rt_id_t &field_id,
                                    bool *is_mandatory) const override ;

  bf_status_t dataFieldMandatoryGet(const bf_rt_id_t &field_id,
                                    const bf_rt_id_t &action_id,
                                    bool *is_mandatory) const override ;

  bf_status_t dataFieldReadOnlyGet(const bf_rt_id_t &field_id,
                                   bool *is_read_only) const override ;

  bf_status_t dataFieldReadOnlyGet(const bf_rt_id_t &field_id,
                                   const bf_rt_id_t &action_id,
                                   bool *is_read_only) const override ;

  bf_status_t dataFieldOneofSiblingsGet(
      const bf_rt_id_t &field_id,
      std::set<bf_rt_id_t> *oneof_siblings) const override ;

  bf_status_t dataFieldOneofSiblingsGet(
      const bf_rt_id_t &field_id,
      const bf_rt_id_t &action_id,
      std::set<bf_rt_id_t> *oneof_siblings) const override ;

  bf_status_t dataFieldNameGet(const bf_rt_id_t &field_id,
                               std::string *name) const override ;

  bf_status_t dataFieldNameGet(const bf_rt_id_t &field_id,
                               const bf_rt_id_t &action_id,
                               std::string *name) const override ;

  bf_status_t dataFieldDataTypeGet(const bf_rt_id_t &field_id,
                                   DataType *type) const override ;

  bf_status_t dataFieldDataTypeGet(const bf_rt_id_t &field_id,
                                   const bf_rt_id_t &action_id,
                                   DataType *type) const override ;

  bf_status_t dataFieldAllowedChoicesGet(
      const bf_rt_id_t &field_id,
      std::vector<std::reference_wrapper<const std::string>> *choices)
      const override ;

  bf_status_t dataFieldAllowedChoicesGet(
      const bf_rt_id_t &field_id,
      const bf_rt_id_t &action_id,
      std::vector<std::reference_wrapper<const std::string>> *choices)
      const override ;

  bf_status_t dataFieldAnnotationsGet(
      const bf_rt_id_t &field_id,
      AnnotationSet *annotations) const override ;
  bf_status_t dataFieldAnnotationsGet(
      const bf_rt_id_t &field_id,
      const bf_rt_id_t &action_id,
      AnnotationSet *annotations) const override ;

  bf_status_t defaultDataValueGet(const bf_rt_id_t &field_id,
                                  const bf_rt_id_t action_id,
                                  uint64_t *default_value) const;

  bf_status_t defaultDataValueGet(const bf_rt_id_t &field_id,
                                  const bf_rt_id_t action_id,
                                  std::string *default_value) const;

  bf_status_t defaultDataValueGet(const bf_rt_id_t &field_id,
                                  const bf_rt_id_t action_id,
                                  float *default_value) const;

  // Action ID APIs
  bf_status_t actionIdGet(const std::string &name,
                          bf_rt_id_t *action_id) const override ;
  bf_status_t actionNameGet(const bf_rt_id_t &action_id,
                            std::string *name) const override ;
  bf_status_t actionIdListGet(
      std::vector<bf_rt_id_t> *action_id) const override ;

  virtual bool actionIdApplicable() const override;

  bf_status_t actionAnnotationsGet(
      const bf_rt_id_t &action_id,
      AnnotationSet *annotations) const override ;

  // Table attributes APIs
  bf_status_t tableAttributesSet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const BfRtTableAttributes &tableAttributes) const override ;
  bf_status_t tableAttributesGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      BfRtTableAttributes *tableAttributes) const override ;
  virtual bf_status_t tableAttributesSet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      const BfRtTableAttributes &tableAttributes) const override;
  virtual bf_status_t tableAttributesGet(
      const BfRtSession &session,
      const bf_rt_target_t &dev_tgt,
      const uint64_t &flags,
      BfRtTableAttributes *tableAttributes) const override;
  bf_status_t tableAttributesSupported(
      std::set<TableAttributesType> *type_set) const override ;

  // Table operations APIs
  bf_status_t tableOperationsSupported(
      std::set<TableOperationsType> *type_set) const override ;
  bf_status_t tableOperationsExecute(
      const BfRtTableOperations &tableOperations) const override ;
  
  // Unexposed APIs
  // This API sets the pipe table handle of the internally generated table
  // by the compiler for a particular resource table. This allows us to
  // expose certain table operations on indrect resource tables which are
  // not explicitly attached to any match action table in the p4.

  virtual bf_status_t ghostTableHandleSet(const pipe_tbl_hdl_t &pipe_hdl);

  bf_status_t dataFieldTypeGet(const bf_rt_id_t &field_id,
                               std::set<DataFieldType> *ret_type) const;

  bf_status_t dataFieldTypeGet(const bf_rt_id_t &field_id,
                               const bf_rt_id_t &action_id,
                               std::set<DataFieldType> *ret_type) const;

  uint32_t keyFieldListSize() const { return key_fields.size(); }

  uint32_t dataFieldListSize() const;

  uint32_t dataFieldListSize(const bf_rt_id_t &action_id) const;

  bf_status_t getKeyField(const bf_rt_id_t &field_id,
                          const BfRtTableKeyField **field) const;

  bf_status_t getDataField(const bf_rt_id_t &field_id,
                           const BfRtTableDataField **field) const;
  bf_status_t getDataField(const bf_rt_id_t &field_id,
                           const bf_rt_id_t &action_id,
                           const BfRtTableDataField **field) const;

  // getDataField() takes in a vector
  // container field IDs instead of a single container field.
  // This is because since this function is on a table, we need to
  // be able to process recursive containers. The above helper func
  // also takes care of containers recursively but it assumes
  // that the first found container is good enough for metadata
  // which is usually the case.
  //
  // if containers are organized
  // {a: {a1: {a2: { a3}}},
  //  b: {a1: {b2: { a3}},
  //      b1: {b2: { a3}}},
  //  c: {a1: {a2: { c3}}}
  //  }
  //
  //  Then in order to get info of b.a1.b2.a3 , vec(b, a1, b2)
  //  should be given to identify it.
  //
  bf_status_t getDataField(const bf_rt_id_t &field_id,
                           const bf_rt_id_t &action_id,
                           const std::vector<bf_rt_id_t> &container_id_v,
                           const BfRtTableDataField **field) const;
  const BfRtTableDataField *getDataFieldHelper(
      const bf_rt_id_t &field_id,
      const std::vector<bf_rt_id_t> &container_id_v,
      const uint32_t depth,
      const std::map<bf_rt_id_t, std::unique_ptr<BfRtTableDataField>> &
          field_map) const;

  // Below 2 funcs are similar to the ones above but they work
  // on using names instead. They internally use name maps instead
  // of traversing through id_maps linearly and finding a name match
  bf_status_t getDataField(const std::string &field_name,
                           const bf_rt_id_t &action_id,
                           const std::vector<bf_rt_id_t> &container_id_v,
                           const BfRtTableDataField **field) const;
  const BfRtTableDataField *getDataFieldHelper(
      const std::string &field_name,
      const std::vector<bf_rt_id_t> &container_id_v,
      const uint32_t depth,
      const std::map<std::string, BfRtTableDataField *> &field_map) const;

  const key_size_t &getKeySize() const { return key_size; }

  size_t getMaxdataSz() const;
  size_t getMaxdataSzbits() const;

  size_t getdataSz(bf_rt_id_t act_id) const;
  size_t getdataSzbits(bf_rt_id_t act_id) const;

  pipe_tbl_hdl_t getResourceHdl(const DataFieldType &field_type) const;

  pipe_tbl_hdl_t getIndirectResourceHdl(const DataFieldType &field_type) const;

  pipe_act_fn_hdl_t getActFnHdl(const bf_rt_id_t &action_id) const;

  bf_rt_id_t getActProfId() const { return act_prof_id; }

  bf_rt_id_t getSelectorId() const { return selector_tbl_id; }

  bf_rt_id_t getActIdFromActFnHdl(const pipe_act_fn_hdl_t &act_fn_hdl) const;

  const pipe_tbl_hdl_t &tablePipeHandleGet() const { return pipe_tbl_hdl; }

  const std::string &programNameGet() const { return prog_name; }

  const std::string &table_name_get() const;
  bf_rt_id_t table_id_get() const;
  const uint64_t &hash_bit_width_get() const { return hash_bit_width; }
  const bool &hasConstDefaultAction() const {
    return has_const_default_action_;
  }

  const std::string &key_field_name_get(const bf_rt_id_t &field_id) const;
  const std::string &data_field_name_get(const bf_rt_id_t &field_id) const;
  const std::string &data_field_name_get(const bf_rt_id_t &field_id,
                                         const bf_rt_id_t &action_id) const;
  const std::string &action_name_get(const bf_rt_id_t &action_id) const;

  virtual bool idleTableEnabled() const { return false; }
  virtual bool idleTablePollMode() const { return false; }

  // Function to verify if the key object is associated with the table
  bool validateTable_from_keyObj(const BfRtTableKeyObj &key) const;
  // Function to verify if the data object is associated with the table
  bool validateTable_from_dataObj(const BfRtTableDataObj &data) const;

  const std::map<std::string, std::vector<bf_rt_table_ref_info_t>> &
  getTableRefMap() const {
    return table_ref_map;
  }
  uint32_t commonDataFieldListSizeGet() const {
    return common_data_fields.size();
  }

  virtual void setActionResources(const bf_dev_id_t & /*dev_id*/) { return; };
  void getActionResources(const bf_rt_id_t action_id,
                          bool *meter,
                          bool *reg,
                          bool *cntr) const;

  virtual void setIsTernaryTable(const bf_dev_id_t & /*dev_id*/) { return; };
  const bool &getIsTernaryTable() const { return is_ternary_table_; };

  const BfRtInfo *bfRtInfoGet() const { return bfrt_info_; };

 protected:
  // Name of the program to which this table obj belongs
  std::string prog_name;

  // Store information about direct resources applicable per action
  std::map<bf_rt_id_t, bool> act_uses_dir_meter;
  std::map<bf_rt_id_t, bool> act_uses_dir_cntr;
  std::map<bf_rt_id_t, bool> act_uses_dir_reg;

  // if this table is a ternary table
  bool is_ternary_table_{false};
  BfRtTableObj *actProfTbl;
  BfRtTableObj *selectorTbl;
  const bf_rt_id_t object_id;
  key_size_t key_size;
  pipe_tbl_hdl_t pipe_tbl_hdl{0};
  const size_t _table_size;
  // Action profile table ID associated with this table. Applicable for
  // MatchAction_Indirect, MatchAction_Indirect_Selector and Selector table
  // types
  bf_rt_id_t act_prof_id;
  // Selector table ID associated with this table. Applicable for
  // MatchAction_Indirect_Selector table
  bf_rt_id_t selector_tbl_id;
  // hash_bit_width of hash object. Only required for the hash tables
  uint64_t hash_bit_width = 0;
  // action_info map
  std::map<bf_rt_id_t, std::unique_ptr<bf_rt_info_action_info_t>>
      action_info_list;
  // action_info map but with names. Owner is still above map.
  // This one only contains the raw pointers
  std::map<std::string, bf_rt_info_action_info_t *> action_info_list_name;

  // A map from action fn handle to action id
  std::map<pipe_act_fn_hdl_t, bf_rt_id_t> act_fn_hdl_to_id;

  mutable std::mutex state_lock;
  bf_status_t action_id_get_helper(const std::string &name,
                                   bf_rt_id_t *action_id) const;
  // Whether this table has a const default action or not
  bool has_const_default_action_{false};
  // Whether this is a const table
  bool is_const_table_{false};
  // Table annotations
  std::set<Annotation> annotations_{};

  // Helper functions
  bf_status_t getKeyStringChoices(const std::string &key_str,
                                  std::vector<std::string> &choices) const;

  bf_status_t getDataStringChoices(const std::string &data_str,
                                   std::vector<std::string> &choices) const;

 private:
  std::vector<bf_rt_table_ref_info_t> tableGetRefNameVec(
      std::string ref) const {
    if (table_ref_map.find(ref) != table_ref_map.end()) {
      return table_ref_map.at(ref);
    }
    return {};
  }

  void addDataFieldType(const bf_rt_id_t &field_id, const DataFieldType &type);

  bf_status_t getResourceInternal(const DataFieldType &field_type,
                                  bf_rt_table_ref_info_t *tbl_ref) const;

  const std::string object_name;
  const TableType object_type;
  const std::set<TableApi> table_apis_{};
  // Map of reference_type -> vector of ref_info structs
  std::map<std::string, std::vector<bf_rt_table_ref_info_t>> table_ref_map;
  // key-fields map
  std::map<bf_rt_id_t, std::unique_ptr<BfRtTableKeyField>> key_fields;

  // Map of common data-fields like TTL/Counter etc
  std::map<bf_rt_id_t, std::unique_ptr<BfRtTableDataField>> common_data_fields;
  // Map of common data-fields like TTL/Counter etc with names
  std::map<std::string, BfRtTableDataField *> common_data_fields_names;

  std::set<TableOperationsType> operations_type_set;
  std::set<TableAttributesType> attributes_type_set;

  // Backpointer to BfRtInfo object
  const BfRtInfo *bfrt_info_;

  friend class BfRtInfoImpl;
};

}  // bfrt

#endif  //

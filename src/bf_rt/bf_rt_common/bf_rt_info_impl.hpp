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
#ifndef _BF_RT_INFO_IMPL_HPP
#define _BF_RT_INFO_IMPL_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <pipe_mgr/pipe_mgr_intf.h>
#include <bf_rt/bf_rt_common.h>
#ifdef __cplusplus
}
#endif

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>

namespace bfrt {

class Cjson;
class PipeMgrIntf;
class BfRtTableObj;
class BfRtTableDataField;
class BfRtTableKeyField;

/* Structure to retrieve info into from the generated program data */
typedef struct bf_rt_info_action_info_ bf_rt_info_action_info_t;

/**
 * @brief This class contains Program configuration information
 * like program name, BF-RT json path and information regarding
 * all the pipeline_profiles of this Program. It is created
 * at the time of device add and resides inside BfRtInfo. Since
 * the menber object in BfRtInfo is a const object, having this
 * class behave like a struct with public members is fine.
 */
class ProgramConfig {
 public:
  class P4Pipeline {
   public:
    P4Pipeline(const std::string &name,
               const std::string &context_json_path,
               const std::string &binary_path,
               const std::vector<bf_dev_pipe_t> pipe_scope)
        : name_(name),
          context_json_path_(context_json_path),
          binary_path_(binary_path),
          pipe_scope_(pipe_scope){};
    const std::string name_;
    const std::string context_json_path_;
    const std::string binary_path_;
    const std::vector<bf_dev_pipe_t> pipe_scope_;
  };
  ProgramConfig(const std::string &prog_name,
                const std::vector<std::string> &bfrtInfoFilePathVect,
                const std::vector<P4Pipeline> &p4_pipelines)
      : prog_name_(prog_name),
        bfrtInfoFilePathVect_(bfrtInfoFilePathVect),
        p4_pipelines_(p4_pipelines){};
  const std::string prog_name_;
  const std::vector<std::string> bfrtInfoFilePathVect_;
  const std::vector<P4Pipeline> p4_pipelines_;
};

class BfRtInfoImpl : public BfRtInfo {
 public:
  bf_status_t bfrtInfoGetTables(
      std::vector<const BfRtTable *> *table_vec_ret) const override;
  bf_status_t bfrtTableFromNameGet(const std::string &name,
                                   const BfRtTable **table_ret) const override;
  bf_status_t bfrtTableFromIdGet(bf_rt_id_t id,
                                 const BfRtTable **table_ret) const override;

  std::unique_ptr<const BfRtInfo> static makeBfRtInfo(
      const bf_dev_id_t &dev_id, const ProgramConfig &program_config);

  bf_status_t bfRtInfoFilePathGet(
      std::vector<std::reference_wrapper<const std::string>> *file_name_vec)
      const override;

  bf_status_t bfrtInfoTablesDependentOnThisTableGet(
      const bf_rt_id_t &tbl_id,
      std::vector<bf_rt_id_t> *table_vec_ret) const override;

  bf_status_t bfrtInfoTablesThisTableDependsOnGet(
      const bf_rt_id_t &tbl_id,
      std::vector<bf_rt_id_t> *table_vec_ret) const override;

  bf_status_t bfRtInfoPipelineInfoGet(
      PipelineProfInfoVec *pipe_info) const override;

  bf_status_t contextFilePathGet(
      std::vector<std::reference_wrapper<const std::string>> *file_name_vec)
      const override;

  bf_status_t binaryFilePathGet(
      std::vector<std::reference_wrapper<const std::string>> *file_name_vec)
      const override;

  // Unexposed
  bf_status_t bfrtInfoGetTables(std::vector<BfRtTable *> *table_vec_ret) const;

 private:
  void buildDependencyGraph();
  bf_status_t setGhostTableHandles(Cjson &table_context);
  BfRtInfoImpl(const bf_dev_id_t &dev_id, const ProgramConfig &program_config);

  std::unique_ptr<BfRtTableObj> parseTable(const bf_dev_id_t &dev_id,
                                           const std::string &prog_name,
                                           Cjson &table_bfrt,
                                           Cjson &table_context);

  std::unique_ptr<BfRtTableObj> parseFixedTable(const std::string &prog_name,
                                                Cjson &table_bfrt);

  std::unique_ptr<BfRtTableKeyField> parseKey(
      Cjson &key,
      const bf_rt_id_t &table_id,
      const size_t &field_offset,
      const size_t &start_bit,
      const size_t &field_full_byte_size,
      const bool &is_atcam_partition_index);
  std::unique_ptr<bf_rt_info_action_info_t> parseActionSpecsForPhase0(
      Cjson &common_data_cjson, Cjson &action_context, BfRtTableObj *bfrtTable);
  std::unique_ptr<bf_rt_info_action_info_t> parseActionSpecs(
      Cjson &action_bfrt, Cjson &action_context, const BfRtTableObj *bfrtTable);
  std::unique_ptr<BfRtTableDataField> parseData(
      Cjson data,
      Cjson action_indirect_res,
      const BfRtTableObj *bfrtTable,
      size_t &field_offset,
      size_t &bitsize,
      pipe_act_fn_hdl_t action_handle,
      bf_rt_id_t action_id,
      uint32_t oneof_index,
      bool *is_register_data_field = nullptr);
  std::unique_ptr<BfRtTableObj> parseSnapshotTable(const std::string &prog_name,
                                                   Cjson &table_bfrt);
  bf_status_t parseContainer(Cjson table_data_cjson,
                             const BfRtTableObj *bfrtTable,
                             BfRtTableDataField *data_field_obj);

  std::unique_ptr<BfRtTableDataField> parseContainerData(
      Cjson data, const BfRtTableObj *bfrtTable);

  void parseType(const Cjson &node,
                 std::string &type,
                 size_t &width,
                 DataType *dataType,
                 uint64_t *default_value,
                 float *default_fl_value,
                 std::string *default_str_value,
                 std::vector<std::string> *choices);

  bool isParamActionParam(const BfRtTableObj *bfrtTable,
                          bf_rt_id_t action_handle,
                          std::string parameter_name,
                          bool use_p4_params_node = false);

  bool isActionParam_matchTbl(const BfRtTableObj *bfrtTable,
                              bf_rt_id_t action_handle,
                              std::string parameter_name);

  bool isActionParam_actProf(const BfRtTableObj *bfrtTable,
                             bf_rt_id_t action_handle,
                             std::string parameter_name,
                             bool use_p4_params_node = false);

  bool isActionParam(Cjson action_table_cjson,
                     bf_rt_id_t action_handle,
                     std::string action_name,
                     std::string parameter_name,
                     bool use_p4_params_node = false);

  void set_direct_register_data_field_type(
      const std::vector<bf_rt_id_t> &data_fields, BfRtTableObj *tbl);

  void set_register_data_field_type(BfRtTableObj *tbl);

  pipe_tbl_hdl_t getPvsHdl(Cjson &parser_context_obj);
  void populateParserObj(const std::string &profile_name,
                         Cjson &parser_gress_ctxt);
  std::string getParentKeyName(const Cjson &key_field) const;

  size_t getStartBit(const Cjson *context_json_key_field,
                     const Cjson *bfrt_json_key_field,
                     const BfRtTableObj &bfrtTable) const;

  bool isFieldSlice(const Cjson &key_field,
                    const size_t &start_bit,
                    const size_t &key_width_byte_size,
                    const size_t &key_width_parent_byte_size) const;

  bf_status_t verifySupportedApi(const BfRtTableObj &bfrtTable,
                                 const Cjson &table_bfrt) const;

  void populate_depends_on_refs(BfRtTableObj *bfrtTable,
                                const Cjson &table_bfrt);
  void parseKeyHelper(
      const Cjson *context_json_table_keys,
      const Cjson *bfrt_json_table_keys,
      const BfRtTableObj &bfrtTable,
      std::map<std::string, size_t> *match_key_field_name_to_position_map,
      std::map<size_t, size_t> *match_key_field_position_to_offset_map,
      size_t *total_offset,
      size_t *num_valid_match_bits,
      std::map<std::string, size_t> *match_key_field_name_to_full_size_map);

  bf_status_t keyByteSizeAndOffsetGet(
      const std::string &table_name,
      const std::string &key_name,
      const std::map<std::string, size_t> &match_key_field_name_to_position_map,
      const std::map<std::string, size_t> &
          match_key_field_name_to_byte_size_map,
      const std::map<size_t, size_t> &match_key_field_position_to_offset_map,
      size_t *field_offset,
      size_t *field_byte_size);

  /* Main P4_info map. object_name --> Obf_rt_info_object_t */
  std::map<std::string, std::unique_ptr<BfRtTable>> tableMap;
  // This is the map which is to be queried when a name lookup for a table
  // happens. Multiple names can point to the same table because multiple
  // names can exist for a table. Example, switchingress.forward and forward
  // both are valid for a table if no conflicts with other table is present
  std::map<std::string, const BfRtTable *> fullTableMap;

  /* in case lookup from ID is needed*/
  std::map<uint32_t, std::string> idToNameMap;

  // Map from table handle to table ID
  std::map<pipe_tbl_hdl_t, bf_rt_id_t> handleToIdMap;

  // internal map of (table name -> pair<bfrtCjson, contextCjson>)
  std::map<std::string,
           std::pair<std::shared_ptr<Cjson>, std::shared_ptr<Cjson>>>
      table_cjson_map;

  // Set of optimized out resource table names
  std::set<std::string> invalid_table_names;

  // Program config
  const ProgramConfig program_config_;

  // Map to indicate what tables depend on a particular table
  std::map<bf_rt_id_t, std::vector<bf_rt_id_t>>
      tables_dependent_on_this_table_map;
  // Map to store pipeline_name <-> context_json_handle_mask
  // which is the mask to be applied (ORed) on all handles
  // parsed from ctx_json
  std::map<std::string, bf_rt_id_t> context_json_handle_mask_map;
  // Map to store table_name <-> pipeline_name
  std::map<std::string, std::string> table_pipeline_name;
  // Map to store learn <-> pipeline_name
  std::map<std::string, std::string> learn_pipeline_name;
};

}  // namespace bfrt

#endif  // ifdef _BF_RT_INFO_IMPL_HPP

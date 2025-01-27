/*******************************************************************************
 * Copyright (c) 2017-2018 Barefoot Networks, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * $Id: $
 *
 ******************************************************************************/
#ifndef _TDI_CONTEXT_INFO_HPP
#define _TDI_CONTEXT_INFO_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <pipe_mgr/pipe_mgr_intf.h>
//#include <tdi/common/tdi_common.h>
#ifdef __cplusplus
}
#endif

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <tdi/common/tdi_defs.h>
#include <tdi/common/tdi_info.hpp>
#include <tdi/common/tdi_learn.hpp>
#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_table_data.hpp>
#include <tdi/common/tdi_table_key.hpp>
#include <tdi/common/tdi_utils.hpp>

#include <tdi_rt/tdi_rt_defs.h>

#include <tdi/common/tdi_json_parser/tdi_cjson.hpp>

namespace tdi {
namespace pna{
namespace rt {


class PipeMgrIntf;

class RtTableContextInfo;
class RtKeyFieldContextInfo;
class RtActionContextInfo;
class RtDataFieldContextInfo;

typedef struct key_size_ {
  size_t bytes;
  size_t bits;
} key_size_t;

enum DataFieldType {
  INVALID,
  ACTION_PARAM,
  ACTION_PARAM_OPTIMIZED_OUT,
  COUNTER_INDEX,
  METER_INDEX,
  REGISTER_INDEX,
  LPF_INDEX,
  WRED_INDEX,
  COUNTER_SPEC_BYTES,
  COUNTER_SPEC_PACKETS,
  METER_SPEC_CIR_PPS,
  METER_SPEC_PIR_PPS,
  METER_SPEC_CBS_PKTS,
  METER_SPEC_PBS_PKTS,
  METER_SPEC_CIR_KBPS,
  METER_SPEC_PIR_KBPS,
  METER_SPEC_CBS_KBITS,
  METER_SPEC_PBS_KBITS,
  ACTION_MEMBER_ID,
  SELECTOR_GROUP_ID,
  SELECTOR_MEMBERS,
  ACTION_MEMBER_STATUS,
  MAX_GROUP_SIZE,
  TTL,
  ENTRY_HIT_STATE,
  LPF_SPEC_GAIN_TIME_CONSTANT,
  LPF_SPEC_DECAY_TIME_CONSTANT,
  LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR,
  LPF_SPEC_TYPE,
  WRED_SPEC_TIME_CONSTANT,
  WRED_SPEC_MIN_THRESHOLD,
  WRED_SPEC_MAX_THRESHOLD,
  WRED_SPEC_MAX_PROBABILITY,
  REGISTER_SPEC_HI,
  REGISTER_SPEC_LO,
  REGISTER_SPEC,
  SNAPSHOT_ENABLE,
  SNAPSHOT_TIMER_ENABLE,
  SNAPSHOT_TIMER_VALUE_USECS,
  SNAPSHOT_STAGE_ID,
  SNAPSHOT_PREV_STAGE_TRIGGER,
  SNAPSHOT_TIMER_TRIGGER,
  SNAPSHOT_LOCAL_STAGE_TRIGGER,
  SNAPSHOT_NEXT_TABLE_NAME,
  SNAPSHOT_ENABLED_NEXT_TABLES,
  SNAPSHOT_TABLE_ID,
  SNAPSHOT_TABLE_NAME,
  SNAPSHOT_MATCH_HIT_ADDRESS,
  SNAPSHOT_MATCH_HIT_HANDLE,
  SNAPSHOT_TABLE_HIT,
  SNAPSHOT_TABLE_INHIBITED,
  SNAPSHOT_TABLE_EXECUTED,
  SNAPSHOT_FIELD_INFO,
  SNAPSHOT_CONTROL_INFO,
  SNAPSHOT_METER_ALU_INFO,
  SNAPSHOT_METER_ALU_OPERATION_TYPE,
  SNAPSHOT_TABLE_INFO,
  SNAPSHOT_LIVENESS_VALID_STAGES,
  SNAPSHOT_GBL_EXECUTE_TABLES,
  SNAPSHOT_ENABLED_GBL_EXECUTE_TABLES,
  SNAPSHOT_LONG_BRANCH_TABLES,
  SNAPSHOT_ENABLED_LONG_BRANCH_TABLES,
  MULTICAST_NODE_ID,
  MULTICAST_NODE_L1_XID_VALID,
  MULTICAST_NODE_L1_XID,
  MULTICAST_ECMP_ID,
  MULTICAST_ECMP_L1_XID_VALID,
  MULTICAST_ECMP_L1_XID,
  MULTICAST_RID,
  MULTICAST_LAG_ID,
  MULTICAST_LAG_REMOTE_MSB_COUNT,
  MULTICAST_LAG_REMOTE_LSB_COUNT,
  DEV_PORT,
  DYN_HASH_CFG_START_BIT,
  DYN_HASH_CFG_LENGTH,
  DYN_HASH_CFG_ORDER,
};
enum class fieldDestination {
  ACTION_SPEC,
  DIRECT_COUNTER,
  DIRECT_METER,
  DIRECT_LPF,
  DIRECT_WRED,
  DIRECT_REGISTER,
  TTL,
  ENTRY_HIT_STATE,
  INVALID
};

/* Struct to keep info regarding a reference to a table */
typedef struct tdi_table_ref_info_ {
  std::string name;
  tdi_id_t id;
  pipe_tbl_hdl_t tbl_hdl;
  // A flag to indicate if the reference is indirect. TRUE when it is, FALSE
  // when the refernece is direct
  bool indirect_ref;
} tdi_table_ref_info_t;

// Map of reference_type -> vector of ref_info structs
using table_ref_map_t =
    std::map<std::string, std::vector<tdi_table_ref_info_t>>;

class ContextInfoParser {
 public:
  ContextInfoParser(const TdiInfo *tdi_info,
                    const ProgramConfig &program_config,
                    const tdi_dev_id_t &dev_id);
  std::unique_ptr<RtTableContextInfo> parseTableContext(
      Cjson &table_context,
      const TableInfo *tdi_table_Info,
      const tdi_dev_id_t &dev_id,
      const std::string &prog_name);
  std::unique_ptr<RtKeyFieldContextInfo> parseKeyFieldContext(
      const KeyFieldInfo *tdi_key_field_info,
      const size_t &field_offset,
      const size_t &start_bit,
      const size_t &parent_field_byte_size);

  std::unique_ptr<RtActionContextInfo> parseActionContext(
      Cjson &action_context,
      const ActionInfo *tdi_action_Info,
      const TableInfo *tdi_table_info,
      const RtTableContextInfo *table_context_info);
  std::unique_ptr<RtDataFieldContextInfo> parseDataFieldContext(
      Cjson action_indirect_res,
      const DataFieldInfo *tdi_data_field_info,
      const TableInfo *tdi_table_info,
      const RtTableContextInfo *table_context_info,
      size_t &field_offset,
      size_t &bitsize,
      pipe_act_fn_hdl_t action_handle);

  static tdi_status_t parseContextJson(const TdiInfo *tdi_info,
                                       const tdi_dev_id_t &dev_id,
                                       const ProgramConfig &program_config);

  tdi_status_t setGhostTableHandles(Cjson &table_context,
                                    const TdiInfo *tdi_info);

  // This function applies a mask (obtained from pipr mgr) on all the
  // handles parsed from tdi/context jsons. We need to do this as the
  // for multiprogram and multiprofile case, we need to ensure that the
  // handles are unique across all the programs on the device
  template <typename T>
  void applyMaskOnContextJsonHandle(T *handle,
                                    const std::string &name,
                                    const bool is_learn = false);

  size_t getStartBit(const Cjson *context_json_key_field,
                     const KeyFieldInfo *key_field_info,
                     const TableInfo *tdi_table_info) const;
#if 0
      std::string getParentKeyName(const Cjson &key_field) const;
#endif

  bool isFieldSlice(const KeyFieldInfo *tdi_key_field_info,
                    const size_t &start_bit,
                    const size_t &key_width_byte_size,
                    const size_t &key_width_parent_byte_size) const;
  void parseKeyHelper(
      const Cjson *context_json_table_keys,
      const std::map<tdi_id_t, std::unique_ptr<KeyFieldInfo>> &table_key_map,
      const TableInfo *tdi_table_info,
      std::map<std::string, size_t> *match_key_field_name_to_position_map,
      std::map<size_t, size_t> *match_key_field_position_to_offset_map,
      size_t *total_offset,
      size_t *num_valid_match_bits,
      std::map<std::string, size_t> *match_key_field_name_to_full_size_map);

  tdi_status_t keyByteSizeAndOffsetGet(
      const std::string &table_name,
      const std::string &key_name,
      const std::map<std::string, size_t> &match_key_field_name_to_position_map,
      const std::map<std::string, size_t>
          &match_key_field_name_to_byte_size_map,
      const std::map<size_t, size_t> &match_key_field_position_to_offset_map,
      size_t *field_offset,
      size_t *field_byte_size);

  std::unique_ptr<RtTableContextInfo> parseFixedTable(
      Cjson &table_context,
      const TableInfo *tdi_table_Info,
      const tdi_dev_id_t &dev_id,
      const std::string &prog_name);

  bool isParamActionParam(const TableInfo *tdi_table_info,
                          const RtTableContextInfo *table_context_info,
                          tdi_id_t action_handle,
                          std::string parameter_name,
                          bool use_p4_params_node = false);

  bool isActionParam_matchTbl(const TableInfo *tdi_table_info,
                              const RtTableContextInfo *table_context_info,
                              tdi_id_t action_handle,
                              std::string parameter_name);

  bool isActionParam_actProf(const TableInfo *tdi_table_info,
                             tdi_id_t action_handle,
                             std::string parameter_name,
                             bool use_p4_params_node = false);

  bool isActionParam(Cjson action_table_cjson,
                     tdi_id_t action_handle,
                     std::string action_name,
                     std::string parameter_name,
                     bool use_p4_params_node = false);

  // TODO: To be made private members
  // tableInfo has mapping from id to object.
  // context json parsing need name to object mapping between table name and
  // TableInfo object, table_context json pair
  std::map<std::string, std::pair<const TableInfo *, std::shared_ptr<Cjson>>>
      tableInfo_contextJson_map;

  // Map from table handle to table ID
  std::map<pipe_tbl_hdl_t, tdi_id_t> handleToIdMap;

  // Map to store pipeline_name <-> context_json_handle_mask
  // which is the mask to be applied (ORed) on all handles
  // parsed from ctx_json
  std::map<std::string, tdi_id_t> context_json_handle_mask_map;

  // Map to store table_name <-> pipeline_name
  std::map<std::string, std::string> table_pipeline_name;
  std::vector<tdi::Cjson> context_json_files;

  // class_end
};
class RtTableContextInfo : public TableContextInfo {
 public:
  std::string tableNameGet() const { return table_name_; };

  tdi_id_t tableIdGet() const { return table_id_; };

  tdi_rt_table_type_e tableTypeGet() const { return table_type_; };

  pipe_tbl_hdl_t tableHdlGet() const { return table_hdl_; };

  const table_ref_map_t &tableRefMapGet() const { return table_ref_map_; };

  bool isConstTable() const { return is_const_table_; };

  key_size_t keySizeGet() const { return key_size_; };

  size_t maxDataSzGet() const { return maxDataSz_; };
  size_t maxDataSzBitsGet() const { return maxDataSzbits_; };

  static std::unique_ptr<RtTableContextInfo> makeTableContextInfo(
      tdi_rt_table_type_e);

  virtual tdi_status_t ghostTableHandleSet(
      const pipe_tbl_hdl_t & /*hdl*/) const {
    LOG_ERROR("%s:%d API Not supported", __func__, __LINE__);
    return TDI_NOT_SUPPORTED;
  }

 protected:
  std::string table_name_;
  tdi_id_t table_id_;
  tdi_rt_table_type_e table_type_;
  pipe_tbl_hdl_t table_hdl_;
  // Map of reference_type -> vector of ref_info structs
  table_ref_map_t table_ref_map_;
  bool is_const_table_;
  // hash_bit_width of hash object. Only required for the hash tables
  uint64_t hash_bit_width_ = 0;
  key_size_t key_size_;

  mutable std::set<tdi_rt_operations_type_e> operations_type_set_;
  mutable std::set<tdi_rt_attributes_type_e> attributes_type_set_;

  size_t maxDataSz_{0};
  size_t maxDataSzbits_{0};

  std::vector<tdi_table_ref_info_t> tableGetRefNameVec(std::string ref) const {
    if (table_ref_map_.find(ref) != table_ref_map_.end()) {
      return table_ref_map_.at(ref);
    }
    return {};
  }

  friend class ContextInfoParser;
};

class MatchActionTableContextInfo : public RtTableContextInfo {
 public:
  MatchActionTableContextInfo() : RtTableContextInfo() ,
                                  actProfTbl_(nullptr),
                                  selectorTbl_(nullptr),
                                  act_prof_id_(),
                                  selector_tbl_id_(){};
  pipe_tbl_hdl_t resourceHdlGet(const DataFieldType &field_type) const {
    tdi_table_ref_info_t tbl_ref;
    bf_status_t status = resourceInternalGet(field_type, &tbl_ref);
    if (status != BF_SUCCESS) {
      return 0;
    }
    return tbl_ref.tbl_hdl;
  }

  pipe_tbl_hdl_t indirectResourceHdlGet(const DataFieldType &field_type) const {
    tdi_table_ref_info_t tbl_ref;
    bf_status_t status = resourceInternalGet(field_type, &tbl_ref);
    if (status != BF_SUCCESS) {
      return 0;
    }
    if (tbl_ref.indirect_ref) {
      return tbl_ref.tbl_hdl;
    }
    return 0;
  }

  void isTernaryTableSet(const tdi_dev_id_t &dev_id);
  const bool &isTernaryTableGet() const { return is_ternary_table_; };

  const std::map<pipe_act_fn_hdl_t, tdi_id_t> &actFnHdlToIdGet() const {
    return act_fn_hdl_to_id_;
  };

  void actionResourcesSet(const tdi_dev_id_t &dev_id);
  void actionResourcesGet(const tdi_id_t action_id,
                          bool *meter,
                          bool *reg,
                          bool *cntr) const {
    *cntr = *meter = *reg = false;
    // Will default to false if action_id does not exist.
    if (this->act_uses_dir_cntr_.find(action_id) !=
        this->act_uses_dir_cntr_.end()) {
      *cntr = this->act_uses_dir_cntr_.at(action_id);
    }
    if (this->act_uses_dir_reg_.find(action_id) !=
        this->act_uses_dir_reg_.end()) {
      *reg = this->act_uses_dir_reg_.at(action_id);
    }
    if (this->act_uses_dir_meter_.find(action_id) !=
        this->act_uses_dir_meter_.end()) {
      *meter = this->act_uses_dir_meter_.at(action_id);
    }
  }
  const tdi::Table *actProfGet() const { return actProfTbl_; };
  const tdi::Table *selectorGet() const { return selectorTbl_; };

 private:
  tdi_status_t resourceInternalGet(const DataFieldType &field_type,
                                   tdi_table_ref_info_t *tbl_ref) const;
  // Store information about direct resources applicable per action
  std::map<tdi_id_t, bool> act_uses_dir_meter_;
  std::map<tdi_id_t, bool> act_uses_dir_cntr_;
  std::map<tdi_id_t, bool> act_uses_dir_reg_;
  // if this table is a ternary table
  bool is_ternary_table_{false};

  // A map from action fn handle to action id
  std::map<pipe_act_fn_hdl_t, tdi_id_t> act_fn_hdl_to_id_;
  mutable tdi::Table *actProfTbl_;
  mutable tdi::Table *selectorTbl_;
  // Action profile table ID associated with this table. Applicable for
  // MatchAction_Indirect, MatchAction_Indirect_Selector and Selector table
  // types
  mutable tdi_id_t act_prof_id_;
  // Selector table ID associated with this table. Applicable for
  // MatchAction_Indirect_Selector table
  mutable tdi_id_t selector_tbl_id_;

  friend class ContextInfoParser;
};

class MatchActionDirectTableContextInfo : public MatchActionTableContextInfo {
 public:
  MatchActionDirectTableContextInfo() : MatchActionTableContextInfo(){};
};

class MatchActionIndirectTableContextInfo : public MatchActionTableContextInfo {
 public:
  MatchActionIndirectTableContextInfo() : MatchActionTableContextInfo(){};
};

class SelectorTableContextInfo : public RtTableContextInfo {
 public:
  SelectorTableContextInfo() : RtTableContextInfo(),
                               actProfTbl_(nullptr),
                               act_prof_id_(){};

 private:
  mutable tdi::Table *actProfTbl_;
  // Action profile table ID associated with this table. Applicable for
  // MatchAction_Indirect, MatchAction_Indirect_Selector and Selector table
  // types
  mutable tdi_id_t act_prof_id_;

  friend class ContextInfoParser;
  friend class Selector;
};

class ActionProfileContextInfo : public RtTableContextInfo {
 public:
  ActionProfileContextInfo() : RtTableContextInfo(){};

  const std::map<pipe_act_fn_hdl_t, tdi_id_t> &actFnHdlToIdGet() const {
    return act_fn_hdl_to_id_;
  };
  pipe_tbl_hdl_t resourceHdlGet(const DataFieldType &field_type) const {
    tdi_table_ref_info_t tbl_ref;
    bf_status_t status = resourceInternalGet(field_type, &tbl_ref);
    if (status != BF_SUCCESS) {
      return 0;
    }
    return tbl_ref.tbl_hdl;
  }

  pipe_tbl_hdl_t indirectResourceHdlGet(const DataFieldType &field_type) const {
    tdi_table_ref_info_t tbl_ref;
    bf_status_t status = resourceInternalGet(field_type, &tbl_ref);
    if (status != BF_SUCCESS) {
      return 0;
    }
    if (tbl_ref.indirect_ref) {
      return tbl_ref.tbl_hdl;
    }
    return 0;
  }

 private:
  tdi_status_t resourceInternalGet(const DataFieldType &field_type,
                                   tdi_table_ref_info_t *tbl_ref) const;

  // A map from action fn handle to action id
  std::map<pipe_act_fn_hdl_t, tdi_id_t> act_fn_hdl_to_id_;

  friend class ContextInfoParser;
};

class RtKeyFieldContextInfo : public KeyFieldContextInfo {
 public:
  std::string nameGet() const { return name_; };

  size_t startBitGet() const { return start_bit_; };

  size_t fieldOffsetGet() const { return field_offset_; };

  size_t parentFieldFullByteSizeGet() const {
    return parent_field_full_byte_size_;
  };

  bool isFieldSlice() const { return is_field_slice_; };
  bool isMatchPriority() const { return is_match_priority_; };

 private:
  std::string name_;
  size_t start_bit_{0};
  size_t field_offset_;
  // Flag to indicate if this is a field slice or not
  bool is_field_slice_{false};
  // This might vary from the 'size_bits' in case of field slices when the field
  // slice width does not equal the size of the entire key field
  size_t parent_field_full_byte_size_{0};
  // flag to indicate whether it is a priority index or not
  bool is_match_priority_{false};
  friend class ContextInfoParser;
};

class RtActionContextInfo : public ActionContextInfo {
 public:
  std::string nameGet() const { return name_; };

  // tdi_id_t actionIdGet() const { return action_id_; };

  pipe_act_fn_hdl_t actionFnHdlGet() const { return act_fn_hdl_; };

  size_t dataSzGet() const { return dataSz_; };

  size_t dataSzBitsGet() const { return dataSzbits_; };

 private:
  std::string name_;
  // tdi_id_t action_id_;
  pipe_act_fn_hdl_t act_fn_hdl_;
  size_t dataSz_;
  // Size of the action data in bits (not including byte padding)
  size_t dataSzbits_;

  friend class ContextInfoParser;
};

class RtDataFieldContextInfo : public DataFieldContextInfo {
 public:
  std::string nameGet() const { return name_; };
  const std::set<DataFieldType> &typesGet() const { return types_; }
  const size_t &offsetGet() const { return offset_; };
  static fieldDestination getDataFieldDestination(
      const std::set<DataFieldType> &fieldTypes);

 private:
  std::string name_;
  size_t offset_{0};
  std::set<DataFieldType> types_;  // resource_set
  friend class ContextInfoParser;
};

const std::vector<std::string> indirect_refname_list = {
    "action_data_table_refs",
    "selection_table_refs",
    "meter_table_refs",
    "statistics_table_refs",
    "stateful_table_refs"};

void prependPipePrefixToKeyName(const std::string &&key,
                                const std::string &prefix,
                                Cjson *object);

void changeTableName(const std::string &pipe_name,
                     std::shared_ptr<Cjson> *context_table_cjson);

void changeIndirectResourceName(const std::string &pipe_name,
                                std::shared_ptr<Cjson> *context_table_cjson);

void changeActionIndirectResourceName(
    const std::string &pipe_name, std::shared_ptr<Cjson> *context_table_cjson);

DataFieldType getDataFieldTypeFrmName(std::string data_name,
                                      tdi_rt_table_type_e table_type);

DataFieldType getDataFieldTypeFrmRes(tdi_rt_table_type_e table_type);

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  // ifdef _TDI_CONTEXT_INFO_HPP

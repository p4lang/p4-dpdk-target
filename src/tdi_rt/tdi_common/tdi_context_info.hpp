/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) 2017-2018 Barefoot Networks, Inc.

 * All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of
 * Barefoot Networks, Inc. and its suppliers, if any. The intellectual and
 * technical concepts contained herein are proprietary to Barefoot Networks,
 * Inc.
 * and its suppliers and may be covered by U.S. and Foreign Patents, patents in
 * process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material is
 * strictly forbidden unless prior written permission is obtained from
 * Barefoot Networks, Inc.
 *
 * No warranty, explicit or implicit is provided, unless granted under a
 * written agreement with Barefoot Networks, Inc.
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
  REGISTER_SPEC_HI,
  REGISTER_SPEC_LO,
  REGISTER_SPEC,
  DEV_PORT,
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

tdi_status_t parseContextJson(const TdiInfo *tdi_info,
                              const tdi_dev_id_t &dev_id,
                              const ProgramConfig &program_config);

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
      const size_t &parent_field_byte_size,
      const bool &is_atcam_partition_index);

  std::unique_ptr<RtActionContextInfo> parseActionContext(
      Cjson &action_context,
      const ActionInfo *tdi_action_Info,
      const TableInfo *tdi_table_info);
  std::unique_ptr<RtDataFieldContextInfo> parseDataFieldContext(
      Cjson action_indirect_res,
      const DataFieldInfo *tdi_data_field_info,
      const TableInfo *tdi_table_info,
      size_t &field_offset,
      size_t &bitsize,
      pipe_act_fn_hdl_t action_handle,
      tdi_id_t action_id,
      uint32_t oneof_index,
      bool *is_register_data_field);

  friend tdi_status_t parseContextJson(const TdiInfo *tdi_info,
                                       const tdi_dev_id_t &dev_id,
                                       const ProgramConfig &program_config);

  // This function applies a mask (obtained from pipr mgr) on all the
  // handles parsed from tdi/context jsons. We need to do this as the
  // for multiprogram and multiprofile case, we need to ensure that the
  // handles are unique across all the programs on the device
  template <typename T>
  void applyMaskOnContextJsonHandle(T *handle,
                                    const std::string &name);

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
      std::map<std::string, const KeyFieldInfo *> name_key_map,
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

  // TODO: To be made private members
  // tableInfo has mapping from id to object.
  // context json parsing need name to object mapping between table name and
  // TableInfo object, table_context json pair
  std::map<std::string, std::pair<const TableInfo *, std::shared_ptr<Cjson>>>
      tableInfo_contextJson_map;

  // TableInfo has id to object maps. contextJson parsing needs name to object
  // maps.
  // so defining name to id maps
  std::map<std::string, tdi_id_t> key_name_id_map;
  std::map<std::string, tdi_id_t> data_name_id_map;
  std::map<std::string, tdi_id_t> action_name_id_map;

  // Map to store pipeline_name <-> context_json_handle_mask
  // which is the mask to be applied (ORed) on all handles
  // parsed from ctx_json
  std::map<std::string, tdi_id_t> context_json_handle_mask_map;

  // Map to store table_name <-> pipeline_name
  std::map<std::string, std::string> table_pipeline_name;
  // class_end
};
class RtTableContextInfo : public TableContextInfo {
 public:
  std::string table_name;
  tdi_id_t table_id;
  tdi_rt_table_type_e table_type;
  pipe_tbl_hdl_t table_hdl;
  // Map of reference_type -> vector of ref_info structs
  std::map<std::string, std::vector<tdi_table_ref_info_t>> table_ref_map;
  bool is_const_table_;
  // hash_bit_width of hash object. Only required for the hash tables
  uint64_t hash_bit_width = 0;
  key_size_t key_size;
  // A map from action fn handle to action id
  std::map<pipe_act_fn_hdl_t, tdi_id_t> act_fn_hdl_to_id;

  friend class ContextInfoParser;
};
class RtKeyFieldContextInfo : public KeyFieldContextInfo {
 public:
  std::string name;
  size_t start_bit{0};
  size_t field_offset;
  // Flag to indicate if this is a field slice or not
  bool is_field_slice{false};
  // This might vary from the 'size_bits' in case of field slices when the field
  // slice width does not equal the size of the entire key field
  size_t parent_field_full_byte_size{0};
  // flag to indicate whether it is a priority index or not
  bool is_partition;
#if 0
    field_offset,
    start_bit,
    isFieldSlice(key, start_bit, (width + 7) / 8, parent_field_byte_size),
    parent_field_byte_size,
    is_atcam_partition_index
#endif
  friend class ContextInfoParser;
};

class RtActionContextInfo : public ActionContextInfo {
  std::string name_;
  tdi_id_t action_id_;
  pipe_act_fn_hdl_t act_fn_hdl_;
  size_t dataSz_;
  // Size of the action data in bits (not including byte padding)
  size_t dataSzbits_;

  friend class ContextInfoParser;
};

class RtDataFieldContextInfo : public DataFieldContextInfo {
  std::set<DataFieldType> types;  // resource_set
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

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

#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include <tdi/common/tdi_info.hpp>
#include <tdi/common/tdi_learn.hpp>
#include <tdi/common/tdi_operations.hpp>
#include <tdi/common/tdi_table.hpp>

#include <tdi/common/tdi_json_parser/tdi_cjson.hpp>
#include <tdi/common/tdi_utils.hpp>

#include "tdi_context_info.hpp"
#include "tdi_rt_info.hpp"
#include "tdi_pipe_mgr_intf.hpp"

namespace tdi {
namespace pna {
namespace rt {

static const std::string match_key_priority_field_name = "$MATCH_PRIORITY";

struct fileOpenFailException : public std::exception {
  const char *what() const throw() { return "File not found"; }
};

tdi_status_t parseContextJson(const TdiInfo *tdi_info,
                              const tdi_dev_id_t &dev_id,
                              const ProgramConfig &program_config) {
  // create contextInfoParser.
  auto contextInfoParser = ContextInfoParser(tdi_info, program_config, dev_id);

  // run through tables
  for (const auto &kv : contextInfoParser.tableInfo_contextJson_map) {
    if (kv.second.first && kv.second.second) {
      auto rt_table_context_info =
          contextInfoParser.parseTableContext(*(kv.second.second),
                                              kv.second.first,
                                              dev_id,
                                              program_config.prog_name_);

      kv.second.first->tableContextInfoSet(
          std::move(rt_table_context_info));
    }
    // TODO: tables present in context.json but not in tdi.json
    // parseGhostTables();
  }

  return TDI_SUCCESS;
}

ContextInfoParser::ContextInfoParser(const TdiInfo *tdi_info,
                                     const ProgramConfig &program_config,
                                     const tdi_dev_id_t &dev_id) {
  // (I): Run through tables in tdi_info and add them in
  // tableInfo_contextJson_map

  const TableInfo *tdi_table_info;
  for (const auto &kv : tdi_info->tableMap) {
    tdi_table_info = (kv.second)->tableInfoGet();
    tableInfo_contextJson_map[kv.first] =
        std::make_pair(tdi_table_info, nullptr);
  }

  // (II): Parse all the context.json for the non-fixed tdi table entries now
  std::vector<tdi::Cjson> context_json_files;
  for (const auto &p4_pipeline : program_config.p4_pipelines_) {
    std::ifstream file_context(p4_pipeline.context_path_);
    if (file_context.fail()) {
      LOG_CRIT("Unable to find Context Json File %s",
               p4_pipeline.context_path_.c_str());
      throw fileOpenFailException();
    }

    std::string content_context((std::istreambuf_iterator<char>(file_context)),
                                std::istreambuf_iterator<char>());
    tdi::Cjson root_cjson_context = Cjson::createCjsonFromFile(content_context);
    // Save for future use
    context_json_files.push_back(root_cjson_context);

    // Parse context.json, tables part
    Cjson tables_cjson_context = root_cjson_context["tables"];

    // Prepending pipe names to table/resource names in context.json.
    // tdi.json contains p4_pipeline names as prefixes but context.json
    // doesn't because a context.json is per pipe already.
    for (auto &table : tables_cjson_context.getCjsonChildVec()) {
      changeTableName(p4_pipeline.name_, &table);
      changeIndirectResourceName(p4_pipeline.name_, &table);
      changeActionIndirectResourceName(p4_pipeline.name_, &table);
    }

    // Add tables from Context.json to the tableInfo_ContextJson_map
    for (const auto &table : tables_cjson_context.getCjsonChildVec()) {
      std::string name = (*table)["name"];
      if (tableInfo_contextJson_map.find(name) !=
          tableInfo_contextJson_map.end()) {
        tableInfo_contextJson_map[name].second = table;
      } else {
        // else this name wasn't present in tdi.json
        // So it must be a compiler gen table. Lets store it in the map
        // anyway
        tableInfo_contextJson_map[name] = std::make_pair(nullptr, table);
      }

      table_pipeline_name[name] = p4_pipeline.name_;
    }

// TODO: Other than tables
    // Get the mask which needs to be applied on all the handles parsed from
    // the context json/s
    (void)dev_id;
  }
}

std::unique_ptr<RtTableContextInfo> ContextInfoParser::parseTableContext(
    Cjson &table_context,
    const TableInfo *tdi_table_info,
    const tdi_dev_id_t &dev_id,
    const std::string &prog_name) {
  // TODO: to avoid compilation errors
  (void)dev_id;
  (void)prog_name;

  auto table_context_info =
      std::unique_ptr<RtTableContextInfo>(new RtTableContextInfo());

  std::string table_name = tdi_table_info->nameGet();
  tdi_id_t table_id = tdi_table_info->idGet();
  // size_t table_size = tdi_table_info->sizeGet();

  // Get table type
  tdi_table_type_e table_type_s = tdi_table_info->tableTypeGet();
  tdi_rt_table_type_e table_type =
      static_cast<tdi_rt_table_type_e>(table_type_s);

  // Get table handle
  pipe_tbl_hdl_t table_hdl;
  table_hdl = Cjson(table_context, "handle");
  applyMaskOnContextJsonHandle(&table_hdl, table_name);

  // update table_context_info.
  table_context_info->table_name = table_name;
  table_context_info->table_type = (tdi_rt_table_type_e)table_type;
  table_context_info->table_id = table_id;
  table_context_info->table_hdl = table_hdl;

  // reference-parsing

  // Add refs of the table
  for (const auto &refname : indirect_refname_list) {
    // get json object *_table_refs from the table
    Cjson indirect_resource_context_cjson = table_context[refname.c_str()];
    for (const auto &indirect_entry :
         indirect_resource_context_cjson.getCjsonChildVec()) {
      // get all info including id and handle
      tdi_table_ref_info_t ref_info;
      ref_info.name = static_cast<std::string>((*indirect_entry)["name"]);
      ref_info.tbl_hdl =
          static_cast<pipe_tbl_hdl_t>((*indirect_entry)["handle"]);
      applyMaskOnContextJsonHandle(&ref_info.tbl_hdl, ref_info.name);
      if (tableInfo_contextJson_map.find(ref_info.name) ==
          tableInfo_contextJson_map.end()) {
        LOG_ERROR("%s:%d table %s was not found in map",
                  __func__,
                  __LINE__,
                  ref_info.name.c_str());
      } else {
        // if how referenced is 'direct', then we do not have an
        // "id" per se.
        if (static_cast<std::string>((*indirect_entry)["how_referenced"]) !=
            "direct") {
          auto res_tableInfo =
              tableInfo_contextJson_map.at(ref_info.name).first;
          if (res_tableInfo) {
            ref_info.id = res_tableInfo->idGet();
            ref_info.indirect_ref = true;
          } else {
            // This is an error since the indirect ref table blob should always
            // be present in the tdi json
            LOG_ERROR("%s:%d TDI json blob not present for table %s",
                      __func__,
                      __LINE__,
                      ref_info.name.c_str());
          }
        } else {
          ref_info.indirect_ref = false;
        }
      }

      LOG_DBG("%s:%d Adding \'%s\' as \'%s\' of \'%s\'",
              __func__,
              __LINE__,
              ref_info.name.c_str(),
              refname.c_str(),
              table_name.c_str());

      table_context_info->table_ref_map[refname].push_back(std::move(ref_info));
    }
  }

  // match_attributes parsing

  // A table with constant entries might have its 'match_attributes' with
  // conditions published as an attached gateway table (named with '-gateway'
  // suffix in context.json).
  // TODO Should be an attribute of TdiTableObj class.
  bool is_gateway_table_key = false;

  if (table_type == TDI_RT_TABLE_TYPE_MATCH_DIRECT ||
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT ||
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR) {
    // If static entries node is present then check the array size,
    // else check if it is an ALPM or ATCAM table and then check for static
    // entries accordingly
    if (table_context["static_entries"].exists()) {
      if (table_context["static_entries"].array_size() > 0) {
        table_context_info->is_const_table_ = true;
      }
    } else {
      std::string atcam = table_context["match_attributes"]["match_type"];
      bool is_atcam = (atcam == "algorithmic_tcam") ? true : false;
      bool is_alpm =
          table_context["match_attributes"]["pre_classifier"].exists();
      if (is_atcam) {
        for (const auto &unit :
             table_context["match_attributes"]["units"].getCjsonChildVec()) {
          if ((*unit)["static_entries"].exists() &&
              (*unit)["static_entries"].array_size() > 0) {
            table_context_info->is_const_table_ = true;
            break;
          }
        }
      } else if (is_alpm) {
        // There is no way to specify const entries for alpm tables as of now
        // due to language restrictions. The below will help it make work when
        // it becomes supported.
        if (table_context["match_attributes"]["pre_classifier"]
                         ["static_entries"]
                             .array_size() > 0) {
          table_context_info->is_const_table_ = true;
        }
      }
    }  // if table_context["static_entries"].exists()

    // Look for any stage table match attributes with gateway table attached.
    // TODO it should be easier when 'gateway_with_entries' stage table type
    // will be implemented.
    if (table_context["match_attributes"].exists() &&
        table_context["match_attributes"]["match_type"].exists() &&
        table_context["match_attributes"]["stage_tables"].exists()) {
      std::string match_type_ = table_context["match_attributes"]["match_type"];
      if (match_type_ == "match_with_no_key") {
        Cjson s_tbl_cjson_ = table_context["match_attributes"]["stage_tables"];
        for (const auto &s_tbl_ : s_tbl_cjson_.getCjsonChildVec()) {
          if ((*s_tbl_)["has_attached_gateway"].exists() &&
              static_cast<bool>((*s_tbl_)["has_attached_gateway"]) == true) {
            is_gateway_table_key = true;
            break;
          }
        }
      }
    }  // if table_context["match_attributes"] ..
  }

  if (table_type == TDI_RT_TABLE_TYPE_SELECTOR) {
    // For selector table add the reference of the action profile table
    tdi_table_ref_info_t ref_info;
    ref_info.tbl_hdl = static_cast<pipe_tbl_hdl_t>(
        table_context["bound_to_action_data_table_handle"]);
    ref_info.indirect_ref = true;
    // Search for the ref tbl handle in the tableInfo_contextjson_map in
    // context_json blobs
    ref_info.name = "";
    for (const auto &json_pair : tableInfo_contextJson_map) {
      if (json_pair.second.second != nullptr) {
        auto handle =
            static_cast<tdi_id_t>((*(json_pair.second.second))["handle"]);
        if (handle == ref_info.tbl_hdl) {
          ref_info.name =
              static_cast<std::string>((*(json_pair.second.second))["name"]);
          break;
        }
      }
    }
    // Apply the mask on the handle now so that it doesn't interfere with the
    // lookup in the above for loop
    applyMaskOnContextJsonHandle(&ref_info.tbl_hdl, ref_info.name);
    auto res_table_info = tableInfo_contextJson_map.at(ref_info.name).first;
    if (res_table_info) {
      ref_info.id = res_table_info->idGet();
    } else {
      // This is an error since the action table blob should always
      // be present in the tdi json
      LOG_ERROR("%s:%d TDI json blob not present for table %s",
                __func__,
                __LINE__,
                ref_info.name.c_str());
      TDI_ASSERT(0);
    }
    table_context_info->table_ref_map["action_data_table_refs"].push_back(
        std::move(ref_info));
  }

  // key parsing

  // getting key
  Cjson table_context_match_fields_cjson = table_context["match_key_fields"];
  auto &name_key_map = tdi_table_info->name_key_map_;

  std::map<std::string /*match_key_field_name*/, size_t /* position*/>
      match_key_field_name_to_position_map;
  std::map<std::string /*match_key_field_name*/,
           size_t /* size of parent field in bytes */>
      match_key_field_name_to_parent_field_byte_size_map;
  std::map<size_t /* position */, size_t /* offset */>
      match_key_field_position_to_offset_map;

  size_t cumulative_offset = 0;
  size_t cumulative_key_width_bits = 0;

  parseKeyHelper(&table_context_match_fields_cjson,
                 name_key_map,
                 tdi_table_info,
                 &match_key_field_name_to_position_map,
                 &match_key_field_position_to_offset_map,
                 &cumulative_offset,
                 &cumulative_key_width_bits,
                 &match_key_field_name_to_parent_field_byte_size_map);

  // parse each key field
  for (const auto &kv : name_key_map) {
    const std::string key_name = kv.first;
    const KeyFieldInfo *tdi_key_field_info = kv.second;
    std::string partition_name =
        table_context["match_attributes"]["partition_field_name"];
    bool is_atcam_partition_index = (partition_name == key_name) ? true : false;
    size_t start_bit = 0;
    // TODO Do not skip parse on is_gateway_table_key when the attached gateway
    // condition table will be correctly exposed by the compiler.
    if (!is_atcam_partition_index && !is_gateway_table_key) {
      start_bit = getStartBit(&table_context_match_fields_cjson,
                              tdi_key_field_info,
                              tdi_table_info);
    }
    size_t field_offset = 0;
    size_t parent_field_byte_size = 0;
    // TODO Implement gateway condition table support on is_gateway_table_key.
    if (key_name != match_key_priority_field_name && !is_gateway_table_key) {
      // Get the field offset and the size of the field in bytes
      if (keyByteSizeAndOffsetGet(
              table_name,
              key_name,
              match_key_field_name_to_position_map,
              match_key_field_name_to_parent_field_byte_size_map,
              match_key_field_position_to_offset_map,
              &field_offset,
              &parent_field_byte_size) != TDI_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in processing key field %s while trying to get "
            "field size and offset",
            __func__,
            __LINE__,
            table_name.c_str(),
            key_name.c_str());
        // TDI_DBGCHK(0);
        return nullptr;
      }
    } else {
      // We don't pack the match priority key field in the bytes array in the
      // match spec and hence this field is also not published in the context
      // json key node. So set the field offset to an invalid value
      field_offset = 0xffffffff;
      parent_field_byte_size = sizeof(uint32_t);
    }

    // Finally form the key field context object
    auto key_field_context_info =
        parseKeyFieldContext(tdi_key_field_info,
                             field_offset,
                             start_bit,
                             parent_field_byte_size,
                             is_atcam_partition_index);

    tdi_key_field_info->keyFieldContextInfoSet(
        std::move(key_field_context_info));
  }

  // Set the table key size in terms of bytes. The field_offset will be the
  // total size of the key
  table_context_info->key_size.bytes = cumulative_offset;

  // Set the table key in terms of exact bit widths.
  table_context_info->key_size.bits = cumulative_key_width_bits;

  ///////////////////////////////////////////////////
  ////// key_parsing end ///////////////////////////////
  /////////////////////////////////////////////////

  ///////////////////////////////////////////////////
  ////// action_parsing ///////////////////////////////
  /////////////////////////////////////////////////

  // getting action profile
  // Cjson table_action_spec_cjson = table_tdi["action_specs"];

  // get the actionInfo map
  // TODO: Need getter to get name_action_map
  auto &name_action_map = tdi_table_info->name_action_map_;
  // If the table type is action, let's find the first match table
  // which has action_data_table_refs as this action profile table.
  // If none was found, then continue with this table itself.
  Cjson table_action_spec_context_cjson = table_context["actions"];
  if (table_type == TDI_RT_TABLE_TYPE_ACTION_PROFILE) {
    for (const auto &kv : tableInfo_contextJson_map) {
      if (kv.second.first != nullptr && kv.second.second != nullptr) {
        const auto table_tdi_node = (kv.second.first);
        const auto &table_context_node = *(kv.second.second);
        tdi_table_type_e curr_table_type_s = table_tdi_node->tableTypeGet();
        tdi_rt_table_type_e curr_table_type =
            static_cast<tdi_rt_table_type_e>(curr_table_type_s);
        if (curr_table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT ||
            curr_table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR) {
          if (table_context_node["action_data_table_refs"].array_size() == 0) {
            continue;
          }

          if (static_cast<std::string>(
                  (*(table_context_node["action_data_table_refs"]
                         .getCjsonChildVec())[0])["name"]) == table_name) {
            table_action_spec_context_cjson = table_context_node["actions"];

            // Also need to push in the reference info for indirect resources
            // They always exist in the match table
            const std::vector<std::string> indirect_res_refs = {
                "stateful_table_refs",
                "statistics_table_refs",
                "meter_table_refs"};

            for (const auto &ref : indirect_res_refs) {
              if (table_context_node[ref.c_str()].array_size() == 0) {
                continue;
              }

              tdi_table_ref_info_t ref_info;
              ref_info.indirect_ref = true;
              ref_info.tbl_hdl = static_cast<pipe_tbl_hdl_t>(
                  (*(table_context_node[ref.c_str()]
                         .getCjsonChildVec())[0])["handle"]);
              ref_info.name = static_cast<std::string>(
                  (*(table_context_node[ref.c_str()]
                         .getCjsonChildVec())[0])["name"]);

              auto res_tdi_table =
                  tableInfo_contextJson_map.at(ref_info.name).first;
              if (res_tdi_table) {
                ref_info.id = res_tdi_table->idGet();
              }

              table_context_info->table_ref_map[ref.c_str()].push_back(
                  std::move(ref_info));
            }

            break;
          }
        }
      }
    }
  }

  // create a map of (action name -> pair<tdiActionCjson, contextActionCjson>)
  std::map<std::string, std::pair<const ActionInfo *, std::shared_ptr<Cjson>>>
      actionInfo_contextJson_map;

  for (const auto &kv : name_action_map) {
    std::string action_name = kv.first;
    actionInfo_contextJson_map[action_name] =
        std::make_pair(kv.second, nullptr);
  }
  for (const auto &action :
       table_action_spec_context_cjson.getCjsonChildVec()) {
    std::string action_name = (*action)["name"];
    if (actionInfo_contextJson_map.find(action_name) !=
        actionInfo_contextJson_map.end()) {
      actionInfo_contextJson_map[action_name].second = action;
    } else {
      if (table_type == TDI_RT_TABLE_TYPE_PORT_METADATA) {
        // This is an acceptable case for phase0 tables for the action spec is
        // not published in the tdi json but it is published in the context
        // json. Hence add it to the map
        actionInfo_contextJson_map[action_name] =
            std::make_pair(nullptr, action);
      } else {
        LOG_WARN(
            "%s:%d Action %s present in context json but not in tdi json \
                 for table %s",
            __func__,
            __LINE__,
            action_name.c_str(),
            table_name.c_str());
        continue;
      }
    }
  }

  // Now parseAction with help of both the Cjsons from the map
  for (const auto &kv : actionInfo_contextJson_map) {
    const ActionInfo *tdi_action_info = kv.second.first;
    std::unique_ptr<RtActionContextInfo> action_context_info;
    if (!kv.second.second) {
      if (table_type != TDI_RT_TABLE_TYPE_DYN_HASH_ALGO) {
        LOG_ERROR("%s:%d Action not present", __func__, __LINE__);
        continue;
      }
    }
    if (kv.second.first == nullptr) {
#if 0  // TODO
      // Phase0 table
      Cjson common_data_cjson = table_tdi["data"];
      action_info = parseActionSpecsForPhase0(
          common_data_cjson, *kv.second.second, tdiTable.get());
#endif
    } else {
      if (table_type == TDI_RT_TABLE_TYPE_DYN_HASH_ALGO) {
#if 0
        // Make dummy action for dyn algo table
        Cjson dummy;
        std::unique_ptr<ActionContextInfo> action_context_info =
            parseActionContext(*kv.second.first, dummy, tdi_action_info);
#endif
      } else {
        // All others tables
        action_context_info = parseActionContext(
            *kv.second.second, tdi_action_info, tdi_table_info);
      }
    }

    tdi_id_t a_id = tdi_action_info->idGet();
    table_context_info->act_fn_hdl_to_id[action_context_info->act_fn_hdl_] =
        a_id;

    // update ActionInfo
    tdi_action_info->actionContextInfoSet(std::move(action_context_info));
  }

  ///////////////////////////////////////////////////
  ////// common_data_parsing ///////////////////////////////
  /////////////////////////////////////////////////
  // nothing to do here
  ///////////////////////////////////////////////////
  ////// common_data_parsing  end///////////////////////////////
  /////////////////////////////////////////////////

  ///////////////////////////////////////////////////
  ////// depends_parsing ///////////////////////////////
  /////////////////////////////////////////////////

  // TODO: Depends parsing
  return table_context_info;
}

std::unique_ptr<RtKeyFieldContextInfo>
ContextInfoParser::parseKeyFieldContext(
    const KeyFieldInfo *tdi_key_field_info,
    const size_t &field_offset,
    const size_t &start_bit,
    const size_t &parent_field_byte_size, /* Field size in bytes */
    const bool &is_atcam_partition_index) {
  auto key_field_context_info = std::unique_ptr<RtKeyFieldContextInfo>(
      new RtKeyFieldContextInfo());

  key_field_context_info->name = tdi_key_field_info->nameGet();
  key_field_context_info->is_field_slice =
      isFieldSlice(tdi_key_field_info,
                   start_bit,
                   (tdi_key_field_info->sizeGet() + 7) / 8,
                   parent_field_byte_size);
  key_field_context_info->start_bit = start_bit;
  key_field_context_info->is_partition = is_atcam_partition_index;
  key_field_context_info->field_offset = field_offset;
  key_field_context_info->parent_field_full_byte_size = parent_field_byte_size;

  return key_field_context_info;
}

// This variant of the function is to parse the action specs for all the tables
// except phase0 table
std::unique_ptr<RtActionContextInfo> ContextInfoParser::parseActionContext(
    Cjson &action_context,
    const ActionInfo *tdi_action_info,
    const TableInfo *tdi_table_info) {
  // create action_context_info here;
  auto action_context_info =
      std::unique_ptr<RtActionContextInfo>(new RtActionContextInfo());

  Cjson action_indirect;  // dummy
  if (action_context.exists()) {
    action_context_info->act_fn_hdl_ =
        static_cast<pipe_act_fn_hdl_t>(action_context["handle"]);
    std::string table_name = tdi_table_info->nameGet();
    applyMaskOnContextJsonHandle(&action_context_info->act_fn_hdl_, table_name);
    action_indirect = action_context["indirect_resources"];
  } else {
    action_context_info->act_fn_hdl_ = 0;
  }
  // get action profile data
  //  Cjson actionDataInfo = action_tdi["data"];
  auto &data_field_map = tdi_action_info->data_fields_names_;
  size_t offset = 0;
  size_t bitsize = 0;

  tdi_id_t action_id = tdi_action_info->idGet();
  for (auto &kv : data_field_map) {
    bool is_register_data_field = false;
    auto data_field_context_info =
        parseDataFieldContext(action_indirect,
                              kv.second,
                              tdi_table_info,
                              offset,
                              bitsize,
                              action_context_info->act_fn_hdl_,
                              action_id,
                              0,
                              &is_register_data_field);
  }
  // Offset value will be the total size of the action spec byte array
  action_context_info->dataSz_ = offset;
  // Bitsize is needed to fill out some back-end info
  // which has both action size in bytes and bits
  action_context_info->dataSzbits_ = bitsize;
  return action_context_info;
}

std::unique_ptr<RtDataFieldContextInfo>
ContextInfoParser::parseDataFieldContext(
    Cjson action_indirect_res,
    const DataFieldInfo *tdi_data_field_info,
    const TableInfo *tdi_table_info,
    size_t &field_offset,
    size_t &bitsize,
    pipe_act_fn_hdl_t action_handle,
    tdi_id_t action_id,
    uint32_t oneof_index,
    bool *is_register_data_field) {
  // TODO: to avoid compilation errors
  (void)action_id;
  (void)action_handle;
  (void)oneof_index;
  (void)is_register_data_field;
  (void)field_offset;
  (void)bitsize;

  // create data_field_context_info here;
  auto data_field_context_info = std::unique_ptr<RtDataFieldContextInfo>(
      new RtDataFieldContextInfo());

  // get "type"
  // tdi_field_data_type_e type = tdi_data_field_info->dataTypeGet();

  std::string data_name = tdi_data_field_info->nameGet();
  std::set<DataFieldType> resource_set;
  tdi_rt_table_type_e this_table_type =
      static_cast<tdi_rt_table_type_e>(tdi_table_info->tableTypeGet());
  // Legacy internal names, if the name starts with '$', then infer its type
  if (data_name[0] == '$') {
    auto data_field_type =
        getDataFieldTypeFrmName(data_name.substr(1), this_table_type);
    resource_set.insert(data_field_type);
  }

  // figure out if this is an indirect_param
  // Doesn't work for action_member or selector member
  // for each parameter listed under indirect
  for (const auto &action_indirect : action_indirect_res.getCjsonChildVec()) {
    if (data_name ==
        static_cast<std::string>((*action_indirect)["parameter_name"])) {
      // get the resource name
      std::string resource_name = (*action_indirect)["resource_name"];
      auto resource = tableInfo_contextJson_map.at(resource_name).first;
      // get the resource type from the table type
      tdi_table_type_e table_type = resource->tableTypeGet();
      auto data_field_type =
          getDataFieldTypeFrmRes((tdi_rt_table_type_e)table_type);

      if ((data_field_type != DataFieldType::INVALID) &&
          (resource_set.find(data_field_type) == resource_set.end())) {
        resource_set.insert(data_field_type);
      }
    }
  }

  data_field_context_info->types = resource_set;
  return data_field_context_info;
}

// This function parses the table_key json node and extracts all the relevant
// information into the helper maps. It extracts the following things
// - Gets the position (the order in which they appear in p4) of every match
// field
// - Gets the full size of every match field in bytes
// - Gets the offset (in the match spec byte buf) of every match field
void ContextInfoParser::parseKeyHelper(
    const Cjson *context_json_table_keys,
    std::map<std::string, const tdi::KeyFieldInfo *> name_key_map,
    const TableInfo *tdi_table_info,
    std::map<std::string, size_t> *match_key_field_name_to_position_map,
    std::map<size_t, size_t> *match_key_field_position_to_offset_map,
    size_t *total_offset_bytes,
    size_t *num_valid_match_bits,
    std::map<std::string, size_t>
        *match_key_field_name_to_parent_field_byte_size_map) {
  bool is_context_json_node = false;
  std::map<std::string, size_t> match_key_field_name_to_bit_size_map;
  auto table_type =
      static_cast<tdi_rt_table_type_e>(tdi_table_info->tableTypeGet());
  if (table_type == TDI_RT_TABLE_TYPE_MATCH_DIRECT ||
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT ||
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR ||
      table_type == TDI_RT_TABLE_TYPE_PORT_METADATA) {
    // For these tables we have context json key nodes. Hence use the context
    // json key node to fill the helper maps.
    is_context_json_node = true;
  } else {
    // For all the other tables, we don't have a context json key node. Hence
    // use the key node in tdi json to extract all the information
    is_context_json_node = false;
  }

  if (is_context_json_node) {
    // This means use the passed in context json key node
    for (const auto &match_field :
         context_json_table_keys->getCjsonChildVec()) {
      if (match_key_field_name_to_position_map->find((*match_field)["name"]) !=
          match_key_field_name_to_position_map->end()) {
        continue;
      }
      (*match_key_field_name_to_position_map)[(*match_field)["name"]] =
          (*match_field)["position"];
      // Initialize the offset map so that we can later calculate the offsets
      // of all the fields in the match spec byte array
      (*match_key_field_position_to_offset_map)[(*match_field)["position"]] =
          (static_cast<size_t>((*match_field)["bit_width_full"]) + 7) / 8;
      (*match_key_field_name_to_parent_field_byte_size_map)[(
          *match_field)["name"]] =
          (static_cast<size_t>((*match_field)["bit_width_full"]) + 7) / 8;
      match_key_field_name_to_bit_size_map[(*match_field)["name"]] =
          static_cast<size_t>((*match_field)["bit_width_full"]);
    }
  } else {
    // This means use the tdi json key node
    size_t pos = 0;
    // Since we don't support field slices for fixed objects, we use the tdi
    // key json node to extract all the information. The position of the
    // fields will just be according to the order in which they are published
    // Hence simply increment the position as we iterate
    for (const auto &kv : name_key_map) {
      std::string key_name = kv.first;
      const KeyFieldInfo *tdi_key_field_info = kv.second;

      (*match_key_field_name_to_position_map)[key_name] = pos;
      (*match_key_field_position_to_offset_map)[pos] =
          (tdi_key_field_info->sizeGet() + 7) / 8;
      (*match_key_field_name_to_parent_field_byte_size_map)[key_name] =
          (tdi_key_field_info->sizeGet() + 7) / 8;
      match_key_field_name_to_bit_size_map[key_name] =
          tdi_key_field_info->sizeGet();
      pos++;
    }
  }
  // Now that we have the position of each field with it's offset, get the
  // cumulative offset of each field.
  size_t cumulative_offset = 0, prev_cumulative_offset = 0;
  for (auto &iter : (*match_key_field_position_to_offset_map)) {
    cumulative_offset += iter.second;
    iter.second = prev_cumulative_offset;
    prev_cumulative_offset = cumulative_offset;
  }

  // Set the total offset
  *total_offset_bytes = cumulative_offset;

  // Now compute the cumulative bit size
  *num_valid_match_bits = 0;
  for (const auto &iter : match_key_field_name_to_bit_size_map) {
    *num_valid_match_bits += iter.second;
  }
}

// This function returns if a key field is a field slice or not
bool ContextInfoParser::isFieldSlice(
    const KeyFieldInfo *tdi_key_field_info,
    const size_t &start_bit,
    const size_t &key_width_byte_size,
    const size_t &key_width_parent_byte_size) const {
  if (tdi_key_field_info->isFieldSlice()) return true;

  // The 2 invariants to determine if a key field is a slice or not are the
  // following
  // 1. the start bit published in the context json is not zero (soft
  //    invariant)
  // 2. the bit_witdth and bit_width_full published in the context json
  //    are not equal (hard invariant)
  // The #1 invariant is soft because there can be cases where-in the start
  // bit is zero but the field is still a field slice. This can happen when
  // the p4 is written such that we have a template for a table wherein the
  // the match key field is templatized and then we instantiate it using a
  // field slice. As a result, the front end compiler (tdi generator)
  // doesn't know that the key field is actually a field slice and doesn't
  // publish the "is_field_slice" annotation in tdi json. However, the
  // the back-end compiler (context json generator) knows it's a field slice
  // and hence makes the bit width and bit-width-full unequal

  // Test invariant #1
  // Now check if the start bit of the key field. If the key field is not a
  // field slice then the start bit has to be zero. If the start bit is not
  // zero. then that indicates that this key field is a name aliased field
  // slice. Return true in that case. We need to correctly distinguish if
  // a key field is a field slice (name aliased or not) or not, so that we
  // correctly and efficiently pack the field in the match spec byte buffer
  if (start_bit != 0) {
    return true;
  }

  // Test invariant #2
  if (key_width_byte_size != key_width_parent_byte_size) {
    return true;
  }

  return false;
}

// This function returns the start bit of the field
// If the field is a field slice then the start bit is derived from the name
// The format of a field slice name is as follows :
// <field_name>[<end_bit>:<start_bit>]
// If the field is a name annotated field slice or not a field slice at all,
// then the start bit is read from the context json node if present or set
// to zero if the context json node is absent
size_t ContextInfoParser::getStartBit(const Cjson *context_json_key_field,
                                      const KeyFieldInfo *tdi_key_field_info,
                                      const TableInfo *tdi_table_info) const {
  const std::string key_name = tdi_key_field_info->nameGet();
  const bool is_field_slice = tdi_key_field_info->isFieldSlice();
  if (is_field_slice) {
    size_t offset = key_name.find(":");
    if (offset == std::string::npos) {
      LOG_ERROR(
          "%s:%d ERROR %s Field is a field slice but the name does not "
          "contain a ':'",
          __func__,
          __LINE__,
          key_name.c_str());
      TDI_DBGCHK(0);
      return 0;
    }

    return std::stoi(key_name.substr(offset + 1), nullptr, 10);
  }

  bool is_context_json_node = false;

  tdi_table_type_e table_type_s = tdi_table_info->tableTypeGet();
  tdi_rt_table_type_e table_type =
      static_cast<tdi_rt_table_type_e>(table_type_s);
  if (table_type == TDI_RT_TABLE_TYPE_MATCH_DIRECT ||
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT ||
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR ||
      table_type == TDI_RT_TABLE_TYPE_PORT_METADATA) {
    // For these tables we have context json key nodes. Hence use the context
    // json key node to get the start bit
    is_context_json_node = true;
  }

  // Since we couldn't get the start bit from the name of the key field, get
  // it from the context json if the context json node is present
  if (is_context_json_node) {
    if (key_name == match_key_priority_field_name) {
      // $MATCH_PRIORITY key field is not published in context json. Just return
      // zero in this case
      return 0;
    }

    for (const auto &match_field : context_json_key_field->getCjsonChildVec()) {
      if (static_cast<const std::string>((*match_field)["name"]) != key_name) {
        continue;
      } else {
        return static_cast<size_t>((*match_field)["start_bit"]);
      }
    }
    // This indicates that we weren't able to find the context json node of the
    // key field
    LOG_ERROR(
        "%s:%d ERROR %s Context json node not found for the key field. Hence "
        "unable to determine the start bit",
        __func__,
        __LINE__,
        key_name.c_str());
    // TDI_DBGCHK(0);

    return 0;
  }

  // This indicates that the key field is not a field slice and it does not
  // even have a context json node associated with it (this is true for fixed
  // tables, resourse tables, etc.) Hence just return 0
  return 0;
}

// This function determines the offset and the size(in bytes) of the field
tdi_status_t ContextInfoParser::keyByteSizeAndOffsetGet(
    const std::string &table_name,
    const std::string &key_name,
    const std::map<std::string, size_t> &match_key_field_name_to_position_map,
    const std::map<std::string, size_t>
        &match_key_field_name_to_parent_field_byte_size_map,
    const std::map<size_t, size_t> &match_key_field_position_to_offset_map,
    size_t *field_offset,
    size_t *parent_field_byte_size) {
  const auto iter_1 = match_key_field_name_to_position_map.find(key_name);
  // Get the field offset and the size of the field in bytes
  if (iter_1 == match_key_field_name_to_position_map.end()) {
    LOG_ERROR(
        "%s:%d %s ERROR key field name %s not found in "
        "match_key_field_name_to_position_map ",
        __func__,
        __LINE__,
        table_name.c_str(),
        key_name.c_str());
    // TDI_DBGCHK(0);
    return TDI_OBJECT_NOT_FOUND;
  }
  size_t position = iter_1->second;
  const auto iter_2 = match_key_field_position_to_offset_map.find(position);
  if (iter_2 == match_key_field_position_to_offset_map.end()) {
    LOG_ERROR(
        "%s:%d %s ERROR Unable to find offset of key field %s with position "
        "%zu",
        __func__,
        __LINE__,
        table_name.c_str(),
        key_name.c_str(),
        position);
    TDI_DBGCHK(0);
    return TDI_OBJECT_NOT_FOUND;
  }
  *field_offset = iter_2->second;
  const auto iter_3 =
      match_key_field_name_to_parent_field_byte_size_map.find(key_name);
  if (iter_3 == match_key_field_name_to_parent_field_byte_size_map.end()) {
    LOG_ERROR(
        "%s:%d %s ERROR key field name %s not found in "
        "match_key_field_name_to_parent_field_byte_size_map ",
        __func__,
        __LINE__,
        table_name.c_str(),
        key_name.c_str());
    TDI_DBGCHK(0);
    return TDI_OBJECT_NOT_FOUND;
  }
  *parent_field_byte_size = iter_3->second;
  return TDI_SUCCESS;
}

// utility methods

void prependPipePrefixToKeyName(const std::string &&key,
                                const std::string &prefix,
                                Cjson *object) {
  std::string t_name = static_cast<std::string>((*object)[key.c_str()]);
  t_name = prefix + "." + t_name;
  object->updateChildNode(key, t_name);
}

void changeTableName(const std::string &pipe_name,
                     std::shared_ptr<Cjson> *context_table_cjson) {
  prependPipePrefixToKeyName("name", pipe_name, context_table_cjson->get());
}

void changeIndirectResourceName(const std::string &pipe_name,
                                std::shared_ptr<Cjson> *context_table_cjson) {
  for (const auto &refname : indirect_refname_list) {
    Cjson indirect_resource_context_cjson =
        (*context_table_cjson->get())[refname.c_str()];
    for (const auto &indirect_resource :
         indirect_resource_context_cjson.getCjsonChildVec()) {
      prependPipePrefixToKeyName("name", pipe_name, indirect_resource.get());
    }
  }
}
void changeActionIndirectResourceName(
    const std::string &pipe_name, std::shared_ptr<Cjson> *context_table_cjson) {
  Cjson actions = (*context_table_cjson->get())["actions"];
  for (const auto &action : actions.getCjsonChildVec()) {
    Cjson indirect_resource_context_cjson = (*action)["indirect_resources"];
    for (const auto &indirect_resource :
         indirect_resource_context_cjson.getCjsonChildVec()) {
      prependPipePrefixToKeyName(
          "resource_name", pipe_name, indirect_resource.get());
    }
  }
}

template <typename T>
void ContextInfoParser::applyMaskOnContextJsonHandle(T *handle,
                                                     const std::string &name) {
  try {
    // figure out the pipeline_name
    std::string pipeline_name = "";
    pipeline_name = table_pipeline_name.at(name);
    LOG_ERROR("%s:%d pipeline_name %s %s",
              __func__,
              __LINE__,
	      pipeline_name.c_str(),
              name.c_str());
    *handle |= context_json_handle_mask_map.at(pipeline_name);
  } catch (const std::exception &e) {
    LOG_ERROR("%s:%d Exception caught %s for %s",
              __func__,
              __LINE__,
              e.what(),
              name.c_str());
    //std::rethrow_exception(std::current_exception());
  }
}

DataFieldType getDataFieldTypeFrmName(std::string data_name,
                                      tdi_rt_table_type_e table_type) {
  if (data_name == "COUNTER_SPEC_BYTES") {
    return DataFieldType::COUNTER_SPEC_BYTES;
  } else if (data_name == "COUNTER_SPEC_PKTS") {
    return DataFieldType::COUNTER_SPEC_PACKETS;
  } else if (data_name == "REGISTER_INDEX") {
    return DataFieldType::REGISTER_INDEX;
  } else if (data_name == "METER_SPEC_CIR_PPS") {
    return DataFieldType::METER_SPEC_CIR_PPS;
  } else if (data_name == "METER_SPEC_PIR_PPS") {
    return DataFieldType::METER_SPEC_PIR_PPS;
  } else if (data_name == "METER_SPEC_CBS_PKTS") {
    return DataFieldType::METER_SPEC_CBS_PKTS;
  } else if (data_name == "METER_SPEC_PBS_PKTS") {
    return DataFieldType::METER_SPEC_PBS_PKTS;
  } else if (data_name == "METER_SPEC_CIR_KBPS") {
    return DataFieldType::METER_SPEC_CIR_KBPS;
  } else if (data_name == "METER_SPEC_PIR_KBPS") {
    return DataFieldType::METER_SPEC_PIR_KBPS;
  } else if (data_name == "METER_SPEC_CBS_KBITS") {
    return DataFieldType::METER_SPEC_CBS_KBITS;
  } else if (data_name == "METER_SPEC_PBS_KBITS") {
    return DataFieldType::METER_SPEC_PBS_KBITS;
  } else if (data_name == "ACTION_MEMBER_ID" &&
             (table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR ||
              table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT)) {
    // ACTION_MEMBER_ID exists  as a data field for
    // MatchActionIndirect Tables
    return DataFieldType::ACTION_MEMBER_ID;
  } else if (data_name == "ACTION_MEMBER_ID" &&
             table_type == TDI_RT_TABLE_TYPE_SELECTOR) {
    // members for a selector group in the selector table
    return DataFieldType::SELECTOR_MEMBERS;
  } else if (data_name == "SELECTOR_GROUP_ID") {
    return DataFieldType::SELECTOR_GROUP_ID;
  } else if (data_name == "ACTION_MEMBER_STATUS") {
    return DataFieldType::ACTION_MEMBER_STATUS;
  } else if (data_name == "MAX_GROUP_SIZE") {
    return DataFieldType::MAX_GROUP_SIZE;
  } else if (data_name == "DEFAULT_FIELD") {
    // This is for phase0 tables as the data that is published in the tdi json
    // is actually an action param for the backend pipe mgr
    return DataFieldType::ACTION_PARAM;
  } else if (data_name == "DEV_PORT") {
    return DataFieldType::DEV_PORT;
  } else {
    return DataFieldType::INVALID;
  }

  return DataFieldType::INVALID;
}

DataFieldType getDataFieldTypeFrmRes(tdi_rt_table_type_e table_type) {
  switch (table_type) {
    case TDI_RT_TABLE_TYPE_COUNTER:
      return DataFieldType::COUNTER_INDEX;
    case TDI_RT_TABLE_TYPE_METER:
      return DataFieldType::METER_INDEX;
    case TDI_RT_TABLE_TYPE_REGISTER:
      return DataFieldType::REGISTER_INDEX;
    default:
      break;
  }
  return DataFieldType::INVALID;
}

}  // namespace rt
}  // namespace pna
}  // namespace tdi

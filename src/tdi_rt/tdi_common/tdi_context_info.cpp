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

fieldDestination RtDataFieldContextInfo::getDataFieldDestination(
    const std::set<DataFieldType> &fieldTypes) {
  for (const auto &fieldType : fieldTypes) {
    switch (fieldType) {
      case DataFieldType::ACTION_PARAM:
      case DataFieldType::ACTION_PARAM_OPTIMIZED_OUT:
      case DataFieldType::ACTION_MEMBER_ID:
      case DataFieldType::SELECTOR_GROUP_ID:
      case DataFieldType::COUNTER_INDEX:
      case DataFieldType::REGISTER_INDEX:
      case DataFieldType::METER_INDEX:
        return fieldDestination::ACTION_SPEC;
        break;
      case DataFieldType::COUNTER_SPEC_BYTES:
      case DataFieldType::COUNTER_SPEC_PACKETS:
        return fieldDestination::DIRECT_COUNTER;
        break;
      case DataFieldType::TTL:
        return fieldDestination::TTL;
      case DataFieldType::ENTRY_HIT_STATE:
        return fieldDestination::ENTRY_HIT_STATE;
      case DataFieldType::METER_SPEC_CIR_PPS:
      case DataFieldType::METER_SPEC_PIR_PPS:
      case DataFieldType::METER_SPEC_CBS_PKTS:
      case DataFieldType::METER_SPEC_PBS_PKTS:
      case DataFieldType::METER_SPEC_CIR_KBPS:
      case DataFieldType::METER_SPEC_PIR_KBPS:
      case DataFieldType::METER_SPEC_CBS_KBITS:
      case DataFieldType::METER_SPEC_PBS_KBITS:
        return fieldDestination::DIRECT_METER;
      case DataFieldType::REGISTER_SPEC:
      case DataFieldType::REGISTER_SPEC_HI:
      case DataFieldType::REGISTER_SPEC_LO:
        return fieldDestination::DIRECT_REGISTER;
      default:
        return fieldDestination::INVALID;
        break;
    }
  }
  return fieldDestination::INVALID;
}

tdi_status_t ContextInfoParser::parseContextJson(
    const TdiInfo *tdi_info,
    const tdi_dev_id_t &dev_id,
    const ProgramConfig &program_config) {
  // create contextInfoParser.
  ContextInfoParser contextInfoParser(tdi_info, program_config, dev_id);

  // run through tables
  for (const auto &kv : contextInfoParser.tableInfo_contextJson_map) {
    if (kv.second.first && kv.second.second) {
      auto rt_table_context_info =
          contextInfoParser.parseTableContext(*(kv.second.second),
                                              kv.second.first,
                                              dev_id,
                                              program_config.prog_name_);

      tdi_id_t table_id = rt_table_context_info->tableIdGet();
      pipe_tbl_hdl_t tbl_hdl = rt_table_context_info->tableHdlGet();

      if (tbl_hdl != 0) {
        contextInfoParser.applyMaskOnContextJsonHandle(&tbl_hdl, kv.first);
        if (contextInfoParser.handleToIdMap.find(tbl_hdl) !=
            contextInfoParser.handleToIdMap.end()) {
            LOG_ERROR(
                "%s:%d %s Repeating table handle %d (%d) in context json. "
                "Cannot "
                "countinue parsing",
                __func__,
                __LINE__,
                kv.first.c_str(),
                tbl_hdl,
                table_id);
            continue;
        } else {
          // table wasn't found in this map. add it
          contextInfoParser.handleToIdMap[tbl_hdl] = table_id;
        }
      }

      kv.second.first->tableContextInfoSet(
          std::move(rt_table_context_info));
    }
  }

  for (const auto &kv : contextInfoParser.tableInfo_contextJson_map) {
    if (!kv.second.first && kv.second.second) {
      // Now set the ghost table handles if applicable
      // Since the table blob exists in the context json but not in bf-rt json
      // it indicates that the table was internally generated by the compiler.
      // Hence we need to set the pipe tbl handle of this internally generated
      // table in the resource table (for which it was generated).
      auto tdi_status =
          contextInfoParser.setGhostTableHandles(*(kv.second.second), tdi_info);
      if (tdi_status != TDI_SUCCESS) {
        LOG_ERROR("%s:%d Unable to set the ghost table handle for table %s",
                  __func__,
                  __LINE__,
                  kv.first.c_str());
      }
    }
  }

  // Now link MatchAction tables to action profile tables
  for (const auto &each : tdi_info->tableMapGet()) {
    auto table_name = each.first;
    const auto &tdi_table = each.second;
    if (tdi_table == nullptr) {
      LOG_ERROR("%s:%d Table object not found for \"%s\"",
                __func__,
                __LINE__,
                table_name.c_str());
      continue;
    }

    const auto tdi_table_info = tdi_table->tableInfoGet();
    const auto context_tbl = static_cast<const RtTableContextInfo *>(
        tdi_table_info->tableContextInfoGet());
    const auto mat_context_tbl =
        static_cast<const MatchActionTableContextInfo *>(
            tdi_table_info->tableContextInfoGet());
    const auto selector_context_tbl =
        static_cast<const SelectorTableContextInfo *>(
            tdi_table_info->tableContextInfoGet());
    if (context_tbl == nullptr) {
      LOG_ERROR("%s:%d context Table object not found for \"%s\"",
                __func__,
                __LINE__,
                table_name.c_str());
	    continue;
    }

    for (const auto &action_table_res_pair :
         context_tbl->tableGetRefNameVec("action_data_table_refs")) {
      auto elem = tdi_info->tableMapGet().find(action_table_res_pair.name);
      if (elem != tdi_info->tableMapGet().end()) {
        // Direct addressed tables will not be present in bf-rt.json and we
        // don't need direct to link match-action tables to direct addressed
        // action tables
        if (static_cast<tdi_rt_table_type_e>(
                tdi_table_info->tableTypeGet()) ==
            TDI_RT_TABLE_TYPE_SELECTOR)
          selector_context_tbl->actProfTbl_ = elem->second.get();
        else
          mat_context_tbl->actProfTbl_ = elem->second.get();
      }
    }

    // For selector tables, store the action profile table ID
    if (static_cast<tdi_rt_table_type_e>(tdi_table_info->tableTypeGet()) ==
        TDI_RT_TABLE_TYPE_SELECTOR) {
      auto ctx_json =
          *(contextInfoParser.tableInfo_contextJson_map[table_name].second);
      pipe_tbl_hdl_t act_prof_tbl_hdl =
          ctx_json["bound_to_action_data_table_handle"];
      contextInfoParser.applyMaskOnContextJsonHandle(&act_prof_tbl_hdl,
                                                     table_name);
      selector_context_tbl->act_prof_id_ =
          contextInfoParser.handleToIdMap[act_prof_tbl_hdl];
    }
    // For MatchActionIndirect_Selector tables, store the selector table id
    // in
    // the match table
    if (static_cast<tdi_rt_table_type_e>(tdi_table_info->tableTypeGet()) ==
        TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR) {
      auto ctx_json =
          *(contextInfoParser.tableInfo_contextJson_map[table_name].second);
      pipe_tbl_hdl_t sel_tbl_hdl =
          ctx_json["selection_table_refs"][0]["handle"];
      contextInfoParser.applyMaskOnContextJsonHandle(&sel_tbl_hdl, table_name);
      mat_context_tbl->selector_tbl_id_ =
          contextInfoParser.handleToIdMap[sel_tbl_hdl];
      auto elem = tdi_info->tableMapGet().find(static_cast<std::string>(
          ctx_json["selection_table_refs"][0]["name"]));
      if (elem != tdi_info->tableMapGet().end()) {
        mat_context_tbl->selectorTbl_ = elem->second.get();
      }
    }
  }

  return TDI_SUCCESS;
}

ContextInfoParser::ContextInfoParser(const TdiInfo *tdi_info,
                                     const ProgramConfig &program_config,
                                     const tdi_dev_id_t &dev_id) {
  // (I): Run through tables in tdi_info and add them in
  // tableInfo_contextJson_map

  const TableInfo *tdi_table_info;
  for (const auto &kv : tdi_info->tableMapGet()) {
    tdi_table_info = (kv.second)->tableInfoGet();
    tableInfo_contextJson_map[kv.first] =
        std::make_pair(tdi_table_info, nullptr);
  }

  // (II): Parse all the context.json for the non-fixed tdi table entries now

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

    // Get the mask which needs to be applied on all the handles parsed from
    // the context json/s
    tdi_id_t context_json_handle_mask = 0;
    IPipeMgrIntf *pipe_mgr_obj = PipeMgrIntf::getInstance();
    pipe_mgr_obj->pipeMgrTblHdlPipeMaskGet(dev_id,
                                           program_config.prog_name_,
                                           p4_pipeline.name_,
                                           &context_json_handle_mask);
    context_json_handle_mask_map[p4_pipeline.name_] = context_json_handle_mask;
  }
}

std::unique_ptr<RtTableContextInfo>
RtTableContextInfo::makeTableContextInfo(
    tdi_rt_table_type_e table_type) {
  switch (table_type) {
    case TDI_RT_TABLE_TYPE_MATCH_DIRECT:
      return std::unique_ptr<RtTableContextInfo>(
          new MatchActionDirectTableContextInfo());
    case TDI_RT_TABLE_TYPE_MATCH_INDIRECT:
    case TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR:
      return std::unique_ptr<RtTableContextInfo>(
          new MatchActionIndirectTableContextInfo());
    case TDI_RT_TABLE_TYPE_ACTION_PROFILE:
      return std::unique_ptr<RtTableContextInfo>(
          new ActionProfileContextInfo());
    case TDI_RT_TABLE_TYPE_SELECTOR:
      return std::unique_ptr<RtTableContextInfo>(
          new SelectorTableContextInfo());
    default:
      return std::unique_ptr<RtTableContextInfo>(
          new RtTableContextInfo());
  }
}

std::unique_ptr<RtTableContextInfo> ContextInfoParser::parseTableContext(
    Cjson &table_context,
    const TableInfo *tdi_table_info,
    const tdi_dev_id_t &dev_id,
    const std::string &prog_name) {
  // TODO: to avoid compilation errors
  (void)prog_name;

  std::string table_name = tdi_table_info->nameGet();
  tdi_id_t table_id = tdi_table_info->idGet();
  // size_t table_size = tdi_table_info->sizeGet();

  // Get table type
  tdi_rt_table_type_e table_type =
      static_cast<tdi_rt_table_type_e>(tdi_table_info->tableTypeGet());

  // Get table handle
  pipe_tbl_hdl_t table_hdl;
  table_hdl = Cjson(table_context, "handle");
  applyMaskOnContextJsonHandle(&table_hdl, table_name);

  auto table_context_info =
      RtTableContextInfo::makeTableContextInfo(table_type);

  // update table_context_info.
  table_context_info->table_name_ = table_name;
  table_context_info->table_type_ = table_type;
  table_context_info->table_id_ = table_id;
  table_context_info->table_hdl_ = table_hdl;

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

      table_context_info->table_ref_map_[refname].push_back(
          std::move(ref_info));
    }
  }

  // match_attributes parsing

  if (table_type == TDI_RT_TABLE_TYPE_MATCH_DIRECT ||
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT ||
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR) {
    auto mat_context_info =
        static_cast<MatchActionTableContextInfo *>(table_context_info.get());
    mat_context_info->isTernaryTableSet(dev_id);
    // If static entries node is present then check the array size,
    // else check if it is an ALPM or ATCAM table and then check for static
    // entries accordingly
    if (table_context["static_entries"].exists()) {
      if (table_context["static_entries"].array_size() > 0) {
        mat_context_info->is_const_table_ = true;
      }
    }  // if table_context["static_entries"].exists()

    if (table_context["match_attributes"].exists() &&
        table_context["match_attributes"]["match_type"].exists() &&
        table_context["match_attributes"]["stage_tables"].exists()) {
      std::string match_type_ = table_context["match_attributes"]["match_type"];
      if (match_type_ == "match_with_no_key") {
        Cjson s_tbl_cjson_ = table_context["match_attributes"]["stage_tables"];
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
    table_context_info->table_ref_map_["action_data_table_refs"].push_back(
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
    size_t start_bit = 0;
    size_t field_offset = 0;
    size_t parent_field_byte_size = 0;
    start_bit = getStartBit(&table_context_match_fields_cjson,
                              tdi_key_field_info,
                              tdi_table_info);

    if (key_name != match_key_priority_field_name) {
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
                             parent_field_byte_size);

    tdi_key_field_info->keyFieldContextInfoSet(
        std::move(key_field_context_info));
  }

  // Set the table key size in terms of bytes. The field_offset will be the
  // total size of the key
  table_context_info->key_size_.bytes = cumulative_offset;

  // Set the table key in terms of exact bit widths.
  table_context_info->key_size_.bits = cumulative_key_width_bits;

  // action_parsing

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

              table_context_info->table_ref_map_[ref.c_str()].push_back(
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
        LOG_WARN(
            "%s:%d Action %s present in context json but not in tdi json \
                 for table %s",
            __func__,
            __LINE__,
            action_name.c_str(),
            table_name.c_str());
    }
  }

  // Now parseAction with help of both the Cjsons from the map
  for (const auto &kv : actionInfo_contextJson_map) {
    const ActionInfo *tdi_action_info = kv.second.first;
    std::unique_ptr<RtActionContextInfo> action_context_info;
    if (!kv.second.second) {
      LOG_ERROR("%s:%d Action not present", __func__, __LINE__);
      continue;
    }
    if (tdi_action_info == nullptr) {
      // Phase0 table
      action_context_info = parseActionContext(*kv.second.second,
                                               tdi_action_info,
                                               tdi_table_info,
                                               table_context_info.get());
    } else {
        // All others tables
      action_context_info = parseActionContext(*kv.second.second,
                                               tdi_action_info,
                                               tdi_table_info,
                                               table_context_info.get());
    }

    // Check for maxDataSz
    if (table_context_info->maxDataSz_ < action_context_info->dataSz_) {
      table_context_info->maxDataSz_ = action_context_info->dataSz_;
    }
    if (table_context_info->maxDataSzbits_ < action_context_info->dataSzbits_) {
      table_context_info->maxDataSzbits_ = action_context_info->dataSzbits_;
    }

    // Phase0 tables doesn't have action_info in tdi.
    // so phase0_action_context_info is used as place holder.
    if (tdi_action_info != nullptr) {
      if (static_cast<tdi_rt_table_type_e>(
              tdi_table_info->tableTypeGet()) ==
          TDI_RT_TABLE_TYPE_ACTION_PROFILE) {
        auto action_profile_context_info =
            static_cast<ActionProfileContextInfo *>(table_context_info.get());
        action_profile_context_info
            ->act_fn_hdl_to_id_[action_context_info->act_fn_hdl_] =
            tdi_action_info->idGet();
      } else {
        auto mat_context_info = static_cast<MatchActionTableContextInfo *>(
            table_context_info.get());
        mat_context_info->act_fn_hdl_to_id_[action_context_info->act_fn_hdl_] =
            tdi_action_info->idGet();
      }

      tdi_action_info->actionContextInfoSet(std::move(action_context_info));
    }
  }

  // Update action function resource usage map.
  if (table_type == TDI_RT_TABLE_TYPE_MATCH_DIRECT ||
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT ||
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR) {
    auto mat_context_info =
        static_cast<MatchActionTableContextInfo *>(table_context_info.get());
    mat_context_info->actionResourcesSet(dev_id);
  }

  // parse common data

  // If the table is phase0, then we would have already parsed the common
  // data fields while parsing the action specs. Hence don't do it
  // again
  if (table_type != TDI_RT_TABLE_TYPE_PORT_METADATA) {
    // get common data
    // Cjson common_data_cjson = table_bfrt["data"];
    const auto &data_field_map = tdi_table_info->name_data_map_;
    for (auto &kv : data_field_map) {
      size_t offset = 0;
      // Bitsize is needed to fill out some back-end info
      // which has both action size in bytes and bits
      // This is not applicable for the common data, but only applicable
      // for per action data
      size_t bitsize = 0;
      // TODO correct the zeroes down
      // making a null temp object to pass as contextJson info since we dont
      // need that
      Cjson temp;
      const DataFieldInfo *common_data_field = kv.second;

      auto data_field_context_info =
          parseDataFieldContext(temp,
                                common_data_field,
                                tdi_table_info,
                                table_context_info.get(),
                                offset,
                                bitsize,
                                0);
      common_data_field->dataFieldContextInfoSet(
          std::move(data_field_context_info));
    }
  }

  // populate_depends_on_refs(bfrtTable.get(), table_bfrt);
  return table_context_info;
}

std::unique_ptr<RtKeyFieldContextInfo>
ContextInfoParser::parseKeyFieldContext(
    const KeyFieldInfo *tdi_key_field_info,
    const size_t &field_offset,
    const size_t &start_bit,
    const size_t &parent_field_byte_size /* Field size in bytes */) {
  auto key_field_context_info = std::unique_ptr<RtKeyFieldContextInfo>(
      new RtKeyFieldContextInfo());

  key_field_context_info->name_ = tdi_key_field_info->nameGet();
  key_field_context_info->is_field_slice_ =
      isFieldSlice(tdi_key_field_info,
                   start_bit,
                   (tdi_key_field_info->sizeGet() + 7) / 8,
                   parent_field_byte_size);
  key_field_context_info->start_bit_ = start_bit;
  key_field_context_info->is_match_priority_ =
      tdi_key_field_info->nameGet() == "$MATCH_PRIORITY";
  key_field_context_info->field_offset_ = field_offset;
  key_field_context_info->parent_field_full_byte_size_ = parent_field_byte_size;

  return key_field_context_info;
}

std::unique_ptr<RtActionContextInfo> ContextInfoParser::parseActionContext(
    Cjson &action_context,
    const ActionInfo *tdi_action_info,
    const TableInfo *tdi_table_info,
    const RtTableContextInfo *table_context_info) {
  // create action_context_info here;
  auto action_context_info =
      std::unique_ptr<RtActionContextInfo>(new RtActionContextInfo());

  Cjson action_indirect;  // dummy
  if (action_context.exists()) {
    action_context_info->name_ =
        static_cast<std::string>(action_context["name"]);
    action_context_info->act_fn_hdl_ =
        static_cast<pipe_act_fn_hdl_t>(action_context["handle"]);
    std::string table_name = tdi_table_info->nameGet();
    applyMaskOnContextJsonHandle(&action_context_info->act_fn_hdl_, table_name);
    action_indirect = action_context["indirect_resources"];
  } else {
    action_context_info->act_fn_hdl_ = 0;
  }

  size_t offset = 0;
  size_t bitsize = 0;

  // For phase0 tables, the common data that is published is actually the
  // action data for the backend pipe mgr. Thus, we need go through
  // all the common data fields so that we know the actual total size of
  // the action_info.
  if (tdi_action_info == nullptr) {
    const auto &data_field_map = tdi_table_info->name_data_map_;
    for (auto &kv : data_field_map) {
      const DataFieldInfo *tdi_data_field_info = kv.second;
      auto data_field_context_info =
          parseDataFieldContext(action_indirect,
                                tdi_data_field_info,
                                tdi_table_info,
                                table_context_info,
                                offset,
                                bitsize,
                                action_context_info->act_fn_hdl_);

      tdi_data_field_info->dataFieldContextInfoSet(
          std::move(data_field_context_info));
    }
  } else {
    const auto &data_field_map = tdi_action_info->data_fields_names_;
    for (auto &kv : data_field_map) {
      const DataFieldInfo *tdi_data_field_info = kv.second;
      auto data_field_context_info =
          parseDataFieldContext(action_indirect,
                                tdi_data_field_info,
                                tdi_table_info,
                                table_context_info,
                                offset,
                                bitsize,
                                action_context_info->act_fn_hdl_);
      tdi_data_field_info->dataFieldContextInfoSet(
          std::move(data_field_context_info));
    }
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
    const RtTableContextInfo *table_context_info,
    size_t &field_offset,
    size_t &bitsize,
    pipe_act_fn_hdl_t action_handle) {
  // create data_field_context_info here;
  auto data_field_context_info = std::unique_ptr<RtDataFieldContextInfo>(
      new RtDataFieldContextInfo());

  data_field_context_info->name_ = tdi_data_field_info->nameGet();
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

  // If it's a resource index, check if it's also an action param
  // using the p4_parameters node, as the compiler might optimize it out
  // from the pack format node.
  if (resource_set.find(DataFieldType::COUNTER_INDEX) != resource_set.end() ||
      resource_set.find(DataFieldType::METER_INDEX) != resource_set.end() ||
      resource_set.find(DataFieldType::REGISTER_INDEX) != resource_set.end()) {
    if (isParamActionParam(tdi_table_info,
                           table_context_info,
                           action_handle,
                           data_name,
                           true)) {
      resource_set.insert(DataFieldType::ACTION_PARAM);
    }
  } else if (isParamActionParam(tdi_table_info,
                                table_context_info,
                                action_handle,
                                data_name)) {
    // Check if it is an action param
    resource_set.insert(DataFieldType::ACTION_PARAM);
  }

  // TODO: register
  // Get Annotations if any
  // We need to use "annotations" field to detect if a particular data field
  // is a register data field as unlike other resource parameters register
  // fields have user defined names in tdi json

  data_field_context_info->types_ = resource_set;
  data_field_context_info->offset_ = field_offset;

  field_offset += ((tdi_data_field_info->sizeGet() + 7) / 8);
  bitsize += tdi_data_field_info->sizeGet();
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
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR) {
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

// Ghost tables are the match action tables that are internally generated
// by the compiler. This happens when the resource table is not associated
// with any match action table in the p4 explicitly but the output of the
// resource is used in a phv field. The information about
// these tables thus appears in context.json but not in bf-rt.json.
// Since certain table operations like setting attributes can only be done on
// match action tables we parse information of the ghost match action tables
// so that we can expose those operations on the resource tables. Thus the
// user will operate directly on the resource and we will internally call the
// ghost match action table
tdi_status_t ContextInfoParser::setGhostTableHandles(Cjson &table_context,
                                                     const TdiInfo *tdi_info) {
  const std::string table_name = table_context["name"];
  pipe_mat_tbl_hdl_t pipe_hdl = table_context["handle"];
  applyMaskOnContextJsonHandle(&pipe_hdl, table_name);

  for (const auto &refname : indirect_refname_list) {
    // get json object *_table_refs from the table
    Cjson indirect_resource_context_cjson = table_context[refname.c_str()];
    for (const auto &indirect_entry :
         indirect_resource_context_cjson.getCjsonChildVec()) {
      // get all info including id and handle
      tdi_table_ref_info_t ref_info;
      ref_info.name = static_cast<std::string>((*indirect_entry)["name"]);
      if (refname != "stateful_table_refs") {
        // TODO currently we support ghost table handling only for stateful
        // tables
        continue;
      }
      // Get hold of the resource table and set the ghost_pipe_hdl
      const auto &ele = tdi_info->tableMapGet().find(ref_info.name);
      if (ele == tdi_info->tableMapGet().end()) {
        // This indicates that the resource table for which the compiler
        // generated this ghost itself doesn't exist, which sounds really
        // wrong. Hence assert
        LOG_ERROR(
            "%s:%d table %s was not found in the tableMap for all the tables",
            __func__,
            __LINE__,
            ref_info.name.c_str());
        TDI_DBGCHK(0);
        return TDI_OBJECT_NOT_FOUND;
      }

      const auto res_table = ele->second.get();
      const auto res_table_context_info =
          static_cast<const RtTableContextInfo *>(
              res_table->tableInfoGet()->tableContextInfoGet());

      auto tdi_status = res_table_context_info->ghostTableHandleSet(pipe_hdl);
      if (tdi_status != TDI_SUCCESS) {
        LOG_ERROR("%s:%d Unable to set ghost table handle for table %s",
                  __func__,
                  __LINE__,
                  ref_info.name.c_str());
        return tdi_status;
      }
      // Currently we support only entry scope table attribute to be set on
      // resource tables
      // Set the entry scope supported flag
      res_table_context_info->attributes_type_set_.insert(ENTRY_SCOPE);
    }
  }
  return BF_SUCCESS;
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
      table_type == TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR) {
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

bool ContextInfoParser::isActionParam(Cjson action_table_cjson,
                                      tdi_id_t action_handle,
                                      std::string action_table_name,
                                      std::string parameter_name,
                                      bool use_p4_params_node) {
  /* FIXME : Cleanup this hairy logic. For the time being we are cheking the
             pack format to figure this out, whereas we should be using the
             p4_parameters node (commented out logic down below). If we
             indeed move to parsing the p4_parameters, we could get rid of
             the new type that we added "ACTION_PARAM_OPTIMIZED_OUT"
  */
  if (!use_p4_params_node) {
    for (const auto &pack_format :
         action_table_cjson["stage_tables"][0]["pack_format"]
             .getCjsonChildVec()) {
      auto json_action_handle =
          static_cast<unsigned int>((*pack_format)["action_handle"]);
      // We need to modify the raw handle parsed before comparing
      applyMaskOnContextJsonHandle(&json_action_handle, action_table_name);
      if (json_action_handle != action_handle) {
        continue;
      }
      for (const auto &field :
           (*pack_format)["entries"][0]["fields"].getCjsonChildVec()) {
        if (static_cast<std::string>((*field)["field_name"]) ==
            parameter_name) {
          return true;
        }
      }
    }
    return false;
  } else {
    // Using the p4_parameters node to figure out if its an action param instead
    // of using the pack format. This is because there can be instances where
    // the param might be optimized out by the compiler and hence not appear in
    // pack format. However, the param always appears in tdi json (optimized
    // out
    // or not). Hence if we use pack format to figure out if its an action param
    // there is an inconsistency (as the param exists in the tdi json but not
    // in pack format). So its better to use p4_parameters as the param is
    // guaranteed to be present there (optimized or not)
    for (const auto &action :
         action_table_cjson["actions"].getCjsonChildVec()) {
      auto json_action_handle = static_cast<unsigned int>((*action)["handle"]);
      // We need to modify the raw handle parsed before comparing
      applyMaskOnContextJsonHandle(&json_action_handle, action_table_name);
      if (json_action_handle != action_handle) {
        continue;
      }
      for (const auto &field : (*action)["p4_parameters"].getCjsonChildVec()) {
        if (static_cast<std::string>((*field)["name"]) == parameter_name) {
          return true;
        }
      }
    }
    return false;
  }
}

bool ContextInfoParser::isActionParam_matchTbl(
    const TableInfo *tdi_table_info,
    const RtTableContextInfo *table_context_info,
    tdi_id_t action_handle,
    std::string parameter_name) {
  // If action table exists then check the action table for the param name
  for (const auto &action_table_res_pair :
       table_context_info->tableGetRefNameVec("action_data_table_refs")) {
    if (tableInfo_contextJson_map.find(action_table_res_pair.name) ==
        tableInfo_contextJson_map.end()) {
      LOG_ERROR("%s:%d Ref Table %s not found",
                __func__,
                __LINE__,
                action_table_res_pair.name.c_str());
      continue;
    }
    Cjson action_table_cjson =
        *(tableInfo_contextJson_map.at(action_table_res_pair.name).second);
    auto action_table_name = action_table_res_pair.name;
    if (isActionParam(action_table_cjson,
                      action_handle,
                      action_table_name,
                      parameter_name)) {
      return true;
    }
  }

  // FIXME : we should probably move to parsing the p4 parameters. In that case
  // 	     we dont need to worry if the field is an immediate action or not
  //	     and we can get rid of the below logic

  // We are here because the parameter is not encoded in the action RAM. Next
  // check if its encoded as part of an immediate field

  Cjson match_table_cjson =
      *(tableInfo_contextJson_map.at(tdi_table_info->nameGet()).second);
  Cjson stage_table = match_table_cjson["match_attributes"]["stage_tables"];
  stage_table = match_table_cjson["match_attributes"]["stage_tables"][0];

  Cjson action_formats = stage_table["action_format"];

  if (stage_table["ternary_indirection_stage_table"].exists()) {
    action_formats =
        stage_table["ternary_indirection_stage_table"]["action_format"];
  }
  for (const auto &action_format : action_formats.getCjsonChildVec()) {
    auto json_action_handle =
        static_cast<unsigned int>((*action_format)["action_handle"]);
    // We need to modify the raw handle parsed before comparing
    applyMaskOnContextJsonHandle(&json_action_handle,
                                 tdi_table_info->nameGet());
    if (json_action_handle != action_handle) {
      continue;
    }
    for (const auto &imm_field :
         (*action_format)["immediate_fields"].getCjsonChildVec()) {
      if (static_cast<std::string>((*imm_field)["param_name"]) ==
          parameter_name) {
        return true;
      }
    }
  }
  return false;
}

// This funcion, given the parameter_name of a certain action figures out if the
// parameter is part of the action spec based on the action table entry
// formatting information
bool ContextInfoParser::isActionParam_actProf(const TableInfo *tdi_table_info,
                                              tdi_id_t action_handle,
                                              std::string parameter_name,
                                              bool use_p4_params_node) {
  auto action_table_name = tdi_table_info->nameGet();
  Cjson action_table_cjson =
      *(tableInfo_contextJson_map.at(action_table_name).second);

  return isActionParam(action_table_cjson,
                       action_handle,
                       action_table_name,
                       parameter_name,
                       use_p4_params_node);
}

bool ContextInfoParser::isParamActionParam(
    const TableInfo *tdi_table_info,
    const RtTableContextInfo *table_context_info,
    tdi_id_t action_handle,
    std::string parameter_name,
    bool use_p4_params_node) {
  // If the table is a Match-Action table call the matchTbl function to analyze
  // if this parameter belongs to the action spec or not
  tdi_rt_table_type_e table_type =
      static_cast<tdi_rt_table_type_e>(tdi_table_info->tableTypeGet());
  if (table_type == TDI_RT_TABLE_TYPE_MATCH_DIRECT) {
    return isActionParam_matchTbl(
        tdi_table_info, table_context_info, action_handle, parameter_name);
  } else if (table_type == TDI_RT_TABLE_TYPE_ACTION_PROFILE) {
    return isActionParam_actProf(
        tdi_table_info, action_handle, parameter_name, use_p4_params_node);
  }
  return false;
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
                                                     const std::string &name,
                                                     const bool is_learn) {
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
    std::rethrow_exception(std::current_exception());
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

tdi_status_t MatchActionTableContextInfo::resourceInternalGet(
    const DataFieldType &field_type, tdi_table_ref_info_t *tbl_ref) const {
  // right now we return the first ref info back only since
  // we do not have provision for multiple refs
  switch (field_type) {
    case (DataFieldType::COUNTER_INDEX):
    case (DataFieldType::COUNTER_SPEC_BYTES):
    case (DataFieldType::COUNTER_SPEC_PACKETS): {
      auto vec = tableGetRefNameVec("statistics_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return TDI_SUCCESS;
      }
      break;
    }

    case (DataFieldType::REGISTER_SPEC):
    case (DataFieldType::REGISTER_SPEC_HI):
    case (DataFieldType::REGISTER_SPEC_LO):
    case (DataFieldType::REGISTER_INDEX): {
      auto vec = tableGetRefNameVec("stateful_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return TDI_SUCCESS;
      }
      break;
    }

    case (DataFieldType::METER_INDEX):
    case (DataFieldType::METER_SPEC_CIR_PPS):
    case (DataFieldType::METER_SPEC_PIR_PPS):
    case (DataFieldType::METER_SPEC_CBS_PKTS):
    case (DataFieldType::METER_SPEC_PBS_PKTS):
    case (DataFieldType::METER_SPEC_CIR_KBPS):
    case (DataFieldType::METER_SPEC_PIR_KBPS):
    case (DataFieldType::METER_SPEC_CBS_KBITS):
    case (DataFieldType::METER_SPEC_PBS_KBITS): {
      auto vec = tableGetRefNameVec("meter_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return TDI_SUCCESS;
      }
      break;
    }
    default:
      LOG_ERROR("%s:%d %s ERROR: Wrong dataFieldType provided",
                __func__,
                __LINE__,
                tableNameGet().c_str());
      return TDI_OBJECT_NOT_FOUND;
  }
  return TDI_OBJECT_NOT_FOUND;
}

// Derived class methods implementation
void MatchActionTableContextInfo::isTernaryTableSet(
    const tdi_dev_id_t &dev_id) {
  auto status = PipeMgrIntf::getInstance()->pipeMgrTblIsTern(
      dev_id, this->tableHdlGet(), &this->is_ternary_table_);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s Unable to find whether table is ternary or not",
              __func__,
              __LINE__,
              tableNameGet().c_str());
  }
}

void MatchActionTableContextInfo::actionResourcesSet(
    const tdi_dev_id_t &dev_id) {
  auto *pipeMgr = PipeMgrIntf::getInstance();
  for (auto const &i : this->act_fn_hdl_to_id_) {
    bool has_cntr, has_meter, has_lpf, has_wred, has_reg;
    has_cntr = has_meter = has_lpf = has_wred = has_reg = false;
    tdi_status_t sts =
        pipeMgr->pipeMgrGetActionDirectResUsage(dev_id,
                                                this->tableHdlGet(),
                                                i.first,
                                                &has_cntr,
                                                &has_meter,
                                                &has_lpf,
                                                &has_wred,
                                                &has_reg);
    if (sts) {
      LOG_ERROR("%s:%d %s Cannot fetch action resource information, %d",
                __func__,
                __LINE__,
                this->tableNameGet().c_str(),
                sts);
      continue;
    }
    this->act_uses_dir_cntr_[i.second] = has_cntr;
    this->act_uses_dir_meter_[i.second] = has_meter || has_lpf || has_wred;
    this->act_uses_dir_reg_[i.second] = has_reg;
  }
}

tdi_status_t ActionProfileContextInfo::resourceInternalGet(
    const DataFieldType &field_type, tdi_table_ref_info_t *tbl_ref) const {
  // right now we return the first ref info back only since
  // we do not have provision for multiple refs
  switch (field_type) {
    case (DataFieldType::COUNTER_INDEX):
    case (DataFieldType::COUNTER_SPEC_BYTES):
    case (DataFieldType::COUNTER_SPEC_PACKETS): {
      auto vec = tableGetRefNameVec("statistics_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return TDI_SUCCESS;
      }
      break;
    }

    case (DataFieldType::REGISTER_SPEC):
    case (DataFieldType::REGISTER_SPEC_HI):
    case (DataFieldType::REGISTER_SPEC_LO):
    case (DataFieldType::REGISTER_INDEX): {
      auto vec = tableGetRefNameVec("stateful_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return TDI_SUCCESS;
      }
      break;
    }

    case (DataFieldType::LPF_INDEX):
    case (DataFieldType::LPF_SPEC_TYPE):
    case (DataFieldType::LPF_SPEC_GAIN_TIME_CONSTANT):
    case (DataFieldType::LPF_SPEC_DECAY_TIME_CONSTANT):
    case (DataFieldType::LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR):
    case (DataFieldType::WRED_INDEX):
    case (DataFieldType::WRED_SPEC_TIME_CONSTANT):
    case (DataFieldType::WRED_SPEC_MIN_THRESHOLD):
    case (DataFieldType::WRED_SPEC_MAX_THRESHOLD):
    case (DataFieldType::WRED_SPEC_MAX_PROBABILITY):
    case (DataFieldType::METER_INDEX):
    case (DataFieldType::METER_SPEC_CIR_PPS):
    case (DataFieldType::METER_SPEC_PIR_PPS):
    case (DataFieldType::METER_SPEC_CBS_PKTS):
    case (DataFieldType::METER_SPEC_PBS_PKTS):
    case (DataFieldType::METER_SPEC_CIR_KBPS):
    case (DataFieldType::METER_SPEC_PIR_KBPS):
    case (DataFieldType::METER_SPEC_CBS_KBITS):
    case (DataFieldType::METER_SPEC_PBS_KBITS): {
      auto vec = tableGetRefNameVec("meter_table_refs");
      if (!vec.empty()) {
        *tbl_ref = vec.front();
        return TDI_SUCCESS;
      }
      break;
    }
    default:
      LOG_ERROR("%s:%d %s ERROR: Wrong dataFieldType provided",
                __func__,
                __LINE__,
                tableNameGet().c_str());
      return TDI_OBJECT_NOT_FOUND;
  }
  return TDI_OBJECT_NOT_FOUND;
}

}  // namespace rt
}  // namespace pna
}  // namespace tdi

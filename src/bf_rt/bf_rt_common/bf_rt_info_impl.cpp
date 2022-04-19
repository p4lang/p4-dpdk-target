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

#include <memory>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <vector>
#include <exception>
#include <regex>

#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_operations.hpp>
#include "bf_rt_info_impl.hpp"
#include "bf_rt_table_impl.hpp"
#include "bf_rt_cjson.hpp"
#include "bf_rt_pipe_mgr_intf.hpp"
#include "bf_rt_table_data_impl.hpp"
#include "bf_rt_utils.hpp"
#include "bf_rt_table_operations_impl.hpp"

namespace bfrt {

#define STAGE_TABLE_STR "stage_tables"

// The following handles parsed from bf-rt/context jsons are modified with the
// mask got from pipe mgr so as to ensure that all the handles are unique
// across all the programs on the device
// 1. Action Handle
// 2. Action function Handle
// 3. Pipe table Handle
// 4. Indirect resource Handle
// 5. Learn filter Handle
// 6. Action Profile Table Handle
// 7. Selector Table Handle

namespace {

static const std::string match_key_priority_field_name = "$MATCH_PRIORITY";

struct fileOpenFailException : public std::exception {
  const char *what() const throw() { return "File not found"; }
};

KeyFieldType getKeyFieldTypeEnum(std::string match_type) {
  if (match_type == "Exact" || match_type == "ATCAM") {
    return KeyFieldType::EXACT;
  } else if (match_type == "Ternary") {
    return KeyFieldType::TERNARY;
  } else if (match_type == "Range") {
    return KeyFieldType::RANGE;
  } else if (match_type == "LPM") {
    return KeyFieldType::LPM;
  } else if (match_type == "Optional") {
    return KeyFieldType::OPTIONAL;
  } else {
    return KeyFieldType::INVALID;
  }
}

DataFieldType getDataFieldTypeFrmName(std::string data_name,
                                      BfRtTable::TableType object_type) {
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
             (object_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR ||
              object_type == BfRtTable::TableType::MATCH_INDIRECT)) {
    // ACTION_MEMBER_ID exists  as a data field for
    // MatchActionIndirect Tables
    return DataFieldType::ACTION_MEMBER_ID;
  } else if (data_name == "ACTION_MEMBER_ID" &&
             object_type == BfRtTable::TableType::SELECTOR) {
    // members for a selector group in the selector table
    return DataFieldType::SELECTOR_MEMBERS;
  } else if (data_name == "SELECTOR_GROUP_ID") {
    return DataFieldType::SELECTOR_GROUP_ID;
  } else if (data_name == "ACTION_MEMBER_STATUS") {
    return DataFieldType::ACTION_MEMBER_STATUS;
  } else if (data_name == "MAX_GROUP_SIZE") {
    return DataFieldType::MAX_GROUP_SIZE;
  } else if (data_name == "ENTRY_TTL") {
    return DataFieldType::TTL;
  } else if (data_name == "ENTRY_HIT_STATE") {
    return DataFieldType::ENTRY_HIT_STATE;
  } else if (data_name == "DEFAULT_FIELD") {
    // This is for phase0 tables as the data that is published in the bfrt json
    // is actually an action param for the backend pipe mgr
    return DataFieldType::ACTION_PARAM;
  } else if (data_name == "DEV_PORT") {
    return DataFieldType::DEV_PORT;
  } else {
    return DataFieldType::INVALID;
  }
}

DataFieldType getDataFieldTypeFrmRes(BfRtTable::TableType table_obj) {
  switch (table_obj) {
    case BfRtTable::TableType::COUNTER:
      return DataFieldType::COUNTER_INDEX;
    case BfRtTable::TableType::METER:
      return DataFieldType::METER_INDEX;
    case BfRtTable::TableType::REGISTER:
      return DataFieldType::REGISTER_INDEX;
    default:
      break;
  }
  return DataFieldType::INVALID;
}

std::set<Annotation> parseAnnotations(Cjson annotation_cjson) {
  std::set<Annotation> annotations;
  for (const auto &annotation : annotation_cjson.getCjsonChildVec()) {
    std::string annotation_name = (*annotation)["name"];
    std::string annotation_value = (*annotation)["value"];
    annotations.emplace(annotation_name, annotation_value);
  }
  return annotations;
}

const std::vector<std::string> indirect_refname_list = {
    "action_data_table_refs",
    "selection_table_refs",
    "meter_table_refs",
    "statistics_table_refs",
    "stateful_table_refs"};

const std::map<std::string, BfRtTable::TableApi> bfrt_api_map = {
    {"tableEntryAdd", BfRtTable::TableApi::ADD},
    {"tableEntryMod", BfRtTable::TableApi::MODIFY},
    {"tableEntryModInc", BfRtTable::TableApi::MODIFY_INC},
    {"tableEntryDel", BfRtTable::TableApi::DELETE},
    {"tableClear", BfRtTable::TableApi::CLEAR},
    {"tableDefaultEntrySet", BfRtTable::TableApi::DEFAULT_ENTRY_SET},
    {"tableDefaultEntryReset", BfRtTable::TableApi::DEFAULT_ENTRY_RESET},
    {"tableDefaultEntryGet", BfRtTable::TableApi::DEFAULT_ENTRY_GET},
    {"tableEntryGet", BfRtTable::TableApi::GET},
    {"tableEntryGetFirst", BfRtTable::TableApi::GET_FIRST},
    {"tableEntryGetNext_n", BfRtTable::TableApi::GET_NEXT_N},
    {"tableUsageGet", BfRtTable::TableApi::USAGE_GET}};

BfRtTable::TableApi getTableApi(std::string api_name) {
  if (bfrt_api_map.find(api_name) != bfrt_api_map.end()) {
    return bfrt_api_map.at(api_name);
  } else {
    return BfRtTable::TableApi::INVALID_API;
  }
}

const std::map<std::string, BfRtTable::TableType> obj_name_map = {
    {"MatchAction_Direct", BfRtTable::TableType::MATCH_DIRECT},
    {"MatchAction_Indirect", BfRtTable::TableType::MATCH_INDIRECT},
    {"MatchAction_Indirect_Selector",
     BfRtTable::TableType::MATCH_INDIRECT_SELECTOR},
    {"Action", BfRtTable::TableType::ACTION_PROFILE},
    {"Selector", BfRtTable::TableType::SELECTOR},
    {"SelectorGetMember", BfRtTable::TableType::SELECTOR_GET_MEMBER},
    {"Meter", BfRtTable::TableType::METER},
    {"Counter", BfRtTable::TableType::COUNTER},
    {"Register", BfRtTable::TableType::REGISTER},
    {"PortMetadata", BfRtTable::TableType::PORT_METADATA},
    {"PortConfigure", BfRtTable::TableType::PORT_CFG},
    {"PortStat", BfRtTable::TableType::PORT_STAT},
    {"PortHdlInfo", BfRtTable::TableType::PORT_HDL_INFO},
    {"PortFpIdxInfo", BfRtTable::TableType::PORT_FRONT_PANEL_IDX_INFO},
    {"PortStrInfo", BfRtTable::TableType::PORT_STR_INFO},
    {"MirrorCfg", BfRtTable::TableType::MIRROR_CFG},
    {"RegisterParam", BfRtTable::TableType::REG_PARAM},
    {"TblDbgCnt", BfRtTable::TableType::DBG_CNT},
    {"LogDbgCnt", BfRtTable::TableType::LOG_DBG_CNT}};

BfRtTable::TableType getTableType(std::string table_type) {
  if (obj_name_map.find(table_type) != obj_name_map.end()) {
    return obj_name_map.at(table_type);
  } else {
    return BfRtTable::TableType::INVALID;
  }
}

void get_register_field_hi_and_low_values(
    const std::vector<bf_rt_id_t> &data_fields,
    bf_rt_id_t *hi_value_id,
    bf_rt_id_t *lo_value_id) {
  // The determination of which field id is 'hi' and 'lo' is done based
  // on the fields ids published in the bf-rt.json. The value with lower
  // field id is 'lo' and the one with the higher field id is 'hi'
  if (data_fields[0] < data_fields[1]) {
    *hi_value_id = data_fields[1];
    *lo_value_id = data_fields[0];
  } else {
    *hi_value_id = data_fields[0];
    *lo_value_id = data_fields[1];
  }
}

std::vector<std::string> splitString(const std::string &s,
                                     const std::string &delimiter) {
  size_t pos = 0;
  size_t start_pos = 0;
  std::vector<std::string> token_list;
  while ((pos = s.find(delimiter, start_pos)) != std::string::npos) {
    token_list.push_back(s.substr(start_pos, pos - start_pos));
    start_pos = pos + delimiter.length();
  }
  token_list.push_back(s.substr(start_pos, pos - start_pos));
  return token_list;
}

/*
 * @brief Generate unique names
 * Split the name using . as delimiter. Then create partially qualified
 * names.
 * tokens(pipe0.SwitchIngress.forward) = set(pipe0, SwitchIngress, forward)
 * full_name_list = [pipe0.SwitchIngress.forward, SwitchIngress.forward,
 * forward]
 */
std::set<std::string> generateUniqueNames(const std::string &obj_name) {
  auto tokens = splitString(obj_name, ".");
  std::string last_token = "";
  std::set<std::string> full_name_list;
  for (auto it = tokens.rbegin(); it != tokens.rend(); ++it) {
    auto token = *it;
    if (last_token == "") {
      full_name_list.insert(token);
      last_token = token;
    } else {
      full_name_list.insert(token + "." + last_token);
      last_token = token + "." + last_token;
    }
  }
  return full_name_list;
}

/* @brief This function converts a nameMap to a fullNameMap. A fullNameMap is a
 *mapping of
 * all possible names of a table entity to the table object's raw pointer.
 *
 * pipe0.SI.forward = <forward_table_1>
 * SI.forward       = <forward_table_1>
 *
 * pipe0.SE.forward = <forward_table_2>
 * SE.forward       = <forward_table_2>
 *
 * pipe0.SI.fib = <fib_table>
 * SI.fib       = <fib_table>
 * fib          = <fib_table>
 */
template <typename T>
void populateFullNameMap(
    const std::map<std::string, std::unique_ptr<T>> &nameMap,
    std::map<std::string, const T *> *fullNameMap) {
  std::set<std::string> names_to_remove;
  // We need to trim the possible names down since all are not possible.
  // Loop over all the tables
  for (const auto &name_pair : nameMap) {
    // Generate all possible names for all the tables. The below map will
    // contain
    // mapping of
    // a table name to all the possible table names.
    const auto possible_name_list = generateUniqueNames(name_pair.first);
    // Loop over all the possible names. If they are not present in the map,
    // then add them to the map. Else, just mark this name in a set kept to
    // remove these later
    for (const auto &prospective_name : possible_name_list) {
      if ((*fullNameMap).find(prospective_name) != (*fullNameMap).end()) {
        names_to_remove.insert(prospective_name);
      } else {
        (*fullNameMap)[prospective_name] = name_pair.second.get();
      }
    }
  }

  // Remove the marked names from the map as well.
  for (const auto &name : names_to_remove) {
    (*fullNameMap).erase(name);
  }
}

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

std::string getAlgoTableName(const std::string &cfg_table_name) {
  // This function constructs the algorithm table
  // name from the configure table. Both these bfrt tables need
  // to be associated with the same hash context json "table" hdl
  std::regex re("(.*)(configure)");
  std::smatch match;
  std::string ret = "";
  if (regex_search(cfg_table_name, match, re) == true) {
    LOG_DBG("%s:%d Found %s as dyn hash table ",
            __func__,
            __LINE__,
            match.str(1).c_str());
    ret = match.str(1) + "algorithm";
  } else {
    LOG_ERROR("%s:%d No match found while trying to search for %s",
              __func__,
              __LINE__,
              cfg_table_name.c_str());
  }
  return ret;
}

}  // anonymous namespace

std::unique_ptr<const BfRtInfo> BfRtInfoImpl::makeBfRtInfo(
    const bf_dev_id_t &dev_id, const ProgramConfig &program_config) {
  try {
    std::unique_ptr<const BfRtInfo> bfRtInfo(
        new BfRtInfoImpl(dev_id, program_config));
    return bfRtInfo;
  } catch (...) {
    LOG_ERROR("%s:%d Failed to create BfRtInfo for dev_id  %d",
              __func__,
              __LINE__,
              dev_id);
    return nullptr;
  }
}

void BfRtInfoImpl::parseType(const Cjson &node,
                             std::string &type,
                             size_t &width,
                             DataType *dataType,
                             uint64_t *default_value,
                             float *default_fl_value,
                             std::string *default_str_value,
                             std::vector<std::string> *choices) {
  //  Cjson node_type = node["type"];
  type = std::string(node["type"]["type"]);
  if (type == "bytes") {
    width = static_cast<unsigned int>(node["type"]["width"]);
    *dataType = DataType::BYTE_STREAM;
  } else if (type == "uint64") {
    width = 64;
    *dataType = DataType::UINT64;
  } else if (type == "uint32") {
    width = 32;
    *dataType = DataType::UINT64;
  } else if (type == "uint16") {
    width = 16;
    *dataType = DataType::UINT64;
  } else if (type == "uint8") {
    width = 8;
    *dataType = DataType::UINT64;
  } else if (type == "bool") {
    width = 1;
    *dataType = DataType::BOOL;
  } else if (type == "float") {
    width = 0;
    *dataType = DataType::FLOAT;
    // If default value is listed, populate it, else its zero
    if (node["type"]["default_value"].exists()) {
      *default_fl_value = static_cast<float>(node["type"]["default_value"]);
    } else {
      *default_fl_value = 0.0;
    }
  } else if (type == "string") {
    width = 0;
    *dataType = DataType::STRING;
    for (const auto &choice : node["type"]["choices"].getCjsonChildVec()) {
      choices->push_back(static_cast<std::string>(*choice));
    }
    // If string default value is listed populate it, else its empty
    if (node["type"]["default_value"].exists()) {
      *default_str_value =
          static_cast<std::string>(node["type"]["default_value"]);
    } else {
      *default_str_value = std::string("INVALID");
    }
    if (node["repeated"].exists()) {
      bool repeated = node["repeated"];
      if (repeated) *dataType = DataType::STRING_ARR;
    }
  } else if (node["container"].exists()) {
    width = 0;
    *dataType = DataType::CONTAINER;
  } else {
    width = 0;
  }

  // If default value is listed, populate it, else its zero
  if (node["type"]["default_value"].exists()) {
    *default_value = static_cast<uint64_t>(node["type"]["default_value"]);
  } else {
    *default_value = 0;
  }

  if (node["repeated"].exists()) {
    bool repeated = node["repeated"];
    if (repeated) {
      if (width == 32 || width == 16 || width == 8) {
        *dataType = DataType::INT_ARR;
      } else if (width == 1) {
        *dataType = DataType::BOOL_ARR;
      }
    }
  }
}

std::unique_ptr<BfRtTableDataField> BfRtInfoImpl::parseData(
    Cjson data,
    Cjson action_indirect_res,
    const BfRtTableObj *bfrtTable,
    size_t &field_offset,
    size_t &bitsize,
    pipe_act_fn_hdl_t action_handle,
    bf_rt_id_t action_id,
    uint32_t oneof_index,
    bool *is_register_data_field) {
  // first get mandatory and read_only
  // because it exists outside oneof and singleton
  auto mandatory = bool(data["mandatory"]);
  auto read_only = bool(data["read_only"]);
  // if "singleton" field exists, then
  // lets point to it for further parsing
  if (data["singleton"].exists()) {
    data = data["singleton"];
  }
  std::set<bf_rt_id_t> oneof_siblings;
  if (data["oneof"].exists()) {
    // Create a set of all the oneof members IDs
    for (const auto &oneof_data : data["oneof"].getCjsonChildVec()) {
      oneof_siblings.insert(static_cast<bf_rt_id_t>((*oneof_data)["id"]));
    }
    data = data["oneof"][oneof_index];
    // remove this field's ID from the siblings. One
    // cannot be their own sibling now, can they?
    oneof_siblings.erase(static_cast<bf_rt_id_t>(data["id"]));
  }
  // get "type"
  std::string key_type_str;
  size_t width;
  DataType type;
  std::vector<std::string> choices;
  // Default value of the field. We currently only support listing upto 64 bits
  // of default value
  uint64_t default_value;
  float default_fl_value;
  std::string default_str_value;
  parseType(data,
            key_type_str,
            width,
            &type,
            &default_value,
            &default_fl_value,
            &default_str_value,
            &choices);
  bool repeated = data["repeated"];

  std::string data_name = static_cast<std::string>(data["name"]);
  std::set<DataFieldType> resource_set;
  BfRtTable::TableType this_table_type;
  bfrtTable->tableTypeGet(&this_table_type);
  // Legacy internal names, if the name starts with '$', then infer its type
  if (data_name[0] == '$') {
    auto data_field_type =
        getDataFieldTypeFrmName(data_name.substr(1), bfrtTable->object_type);
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
      auto resource = table_cjson_map.at(resource_name).first;
      // get the resource type from the table type
      auto table_type = getTableType((*resource)["table_type"]);
      auto data_field_type = getDataFieldTypeFrmRes(table_type);

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
    if (isParamActionParam(bfrtTable, action_handle, data_name, true)) {
      resource_set.insert(DataFieldType::ACTION_PARAM);
    }
  } else if (isParamActionParam(bfrtTable, action_handle, data_name)) {
    // Check if it is an action param
    resource_set.insert(DataFieldType::ACTION_PARAM);
  }

  // Get Annotations if any
  // We need to use "annotations" field to detect if a particular data field
  // is a register data field as unlike other resource parameters register
  // fields have user defined names in bfrt json
  bool data_field_is_reg = false;
  std::set<Annotation> annotations = parseAnnotations(data["annotations"]);
  if (annotations.find(Annotation("$bfrt_field_class", "register_data")) !=
      annotations.end()) {
    data_field_is_reg = true;
    *is_register_data_field = data_field_is_reg;
  }

  BfRtTable::TableType table_type;
  bfrtTable->tableTypeGet(&table_type);
  if (table_type == BfRtTable::TableType::MATCH_DIRECT ||
      table_type == BfRtTable::TableType::MATCH_INDIRECT ||
      table_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR ||
      table_type == BfRtTable::TableType::ACTION_PROFILE ||
      table_type == BfRtTable::TableType::PORT_METADATA) {
    if ((resource_set.size() == 0) && (data_field_is_reg == false)) {
      // This indicates that this field is an action param which has been
      // optmized out by the compiler from the context json. So set the type
      // accordingly
      resource_set.insert(DataFieldType::ACTION_PARAM_OPTIMIZED_OUT);
    }
  }

  bf_rt_id_t table_id = bfrtTable->table_id_get();
  bf_rt_id_t data_id = static_cast<bf_rt_id_t>(data["id"]);
  // Parse the container if it exists
  bool container_valid = false;
  if (data["container"].exists()) {
    container_valid = true;
  }
  std::unique_ptr<BfRtTableDataField> data_field(
      new BfRtTableDataField(table_id,
                             data_id,
                             data_name,
                             action_id,
                             width,
                             type,
                             std::move(choices),
                             default_value,
                             default_fl_value,
                             default_str_value,
                             field_offset,
                             repeated,
                             resource_set,
                             mandatory,
                             read_only,
                             container_valid,
                             annotations,
                             oneof_siblings));

  // Byte offset. Incremented only after DataField construction
  field_offset += ((width + 7) / 8);
  bitsize += width;

  // Parse the container if it exists
  if (container_valid) {
    Cjson c_table_data = data["container"];
    parseContainer(c_table_data, bfrtTable, data_field.get());
  }
  return data_field;
}

std::unique_ptr<BfRtTableKeyField> BfRtInfoImpl::parseKey(
    Cjson &key,
    const bf_rt_id_t &table_id,
    const size_t &field_offset,
    const size_t &start_bit,
    const size_t &parent_field_byte_size,
    const bool &is_atcam_partition_index) {
  // get "type"
  std::string key_type_str;
  size_t width;
  DataType type;
  uint64_t default_value;
  float default_fl_value;
  std::string default_str_value;
  std::vector<std::string> choices;
  parseType(key,
            key_type_str,
            width,
            &type,
            &default_value,
            &default_fl_value,
            &default_str_value,
            &choices);
  std::string key_name = key["name"];

  // create key_field structure and fill it
  std::unique_ptr<BfRtTableKeyField> key_field(new BfRtTableKeyField(
      key["id"],
      key_name,
      width,
      getKeyFieldTypeEnum(key["match_type"]),
      type,
      std::move(choices),
      field_offset,
      start_bit,
      isFieldSlice(key, start_bit, (width + 7) / 8, parent_field_byte_size),
      parent_field_byte_size,
      is_atcam_partition_index));

  if (key_field == nullptr) {
    LOG_ERROR("%s:%d Error forming key_field %d for table_id %d",
              __func__,
              __LINE__,
              static_cast<uint32_t>(key["id"]),
              table_id);
  }
  return key_field;
}

bool BfRtInfoImpl::isActionParam(Cjson action_table_cjson,
                                 bf_rt_id_t action_handle,
                                 std::string action_table_name,
                                 std::string parameter_name,
                                 bool use_p4_params_node) {
  /* FIXME : Cleanup this hairy logic. For the time being we are cheking the
             pack format to figure this out, whereas we should be using the
             p4_parameters node (commented out logic down below). If we
             indeed move to parsing the p4_parameters, we could get rid of
             the new type that we added "ACTION_PARAM_OPTIMIZED_OUT"
  */
  /* FIXME: For the DPDK and MEV target we don't have pack format,
   * so always use p4_parameters node */
  use_p4_params_node = 1;
  if (!use_p4_params_node) {
    for (const auto &pack_format :
         action_table_cjson[STAGE_TABLE_STR][0]["pack_format"]
             .getCjsonChildVec()) {
      auto json_action_handle =
          static_cast<unsigned int>((*pack_format)["action_handle"]);
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
    // pack format. However, the param always appears in bfrt json (optimized
    // out
    // or not). Hence if we use pack format to figure out if its an action param
    // there is an inconsistency (as the param exists in the bfrt json but not
    // in pack format). So its better to use p4_parameters as the param is
    // guaranteed to be present there (optimized or not)
    for (const auto &action :
         action_table_cjson["actions"].getCjsonChildVec()) {
      auto json_action_handle = static_cast<unsigned int>((*action)["handle"]);
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

bool BfRtInfoImpl::isActionParam_matchTbl(const BfRtTableObj *bfrtTable,
                                          bf_rt_id_t action_handle,
                                          std::string parameter_name) {
  // If action table exists then check the action table for the param name
  for (const auto &action_table_res_pair :
       bfrtTable->tableGetRefNameVec("action_data_table_refs")) {
    if (table_cjson_map.find(action_table_res_pair.name) ==
        table_cjson_map.end()) {
      LOG_ERROR("%s:%d Ref Table %s not found",
                __func__,
                __LINE__,
                action_table_res_pair.name.c_str());
      continue;
    }
    Cjson action_table_cjson =
        *(table_cjson_map.at(action_table_res_pair.name).second);
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
      *(table_cjson_map.at(bfrtTable->table_name_get()).second);
  // Find if it's an ATCAM table
  std::string atcam = match_table_cjson["match_attributes"]["match_type"];
  bool is_atcam = (atcam == "algorithmic_tcam") ? true : false;
  // Find if it's an ALPM table
  bool is_alpm =
      match_table_cjson["match_attributes"]["pre_classifier"].exists();
  // For ALPM, table data fields reside in the algorithmic tcam part of the
  // table
  // ATCAM table will be present in the "atcam_table" node of the match table
  // json
  // There will be as many "units" as the number of ATCAM logical tables for the
  // ALPM table. We just process the first one in order to find out about action
  // tables
  Cjson stage_table = match_table_cjson["match_attributes"][STAGE_TABLE_STR];
  if (is_alpm) {
    stage_table =
        match_table_cjson["match_attributes"]["atcam_table"]["match_attributes"]
                         ["units"][0]["match_attributes"][STAGE_TABLE_STR][0];
  } else if (is_atcam) {
    stage_table = match_table_cjson["match_attributes"]["units"][0]
                                   ["match_attributes"][STAGE_TABLE_STR][0];
  } else {
    stage_table = match_table_cjson["match_attributes"][STAGE_TABLE_STR][0];
  }

  Cjson action_formats = stage_table["action_format"];

  if (stage_table["ternary_indirection_stage_table"].exists()) {
    action_formats =
        stage_table["ternary_indirection_stage_table"]["action_format"];
  }
  for (const auto &action_format : action_formats.getCjsonChildVec()) {
    auto json_action_handle =
        static_cast<unsigned int>((*action_format)["action_handle"]);
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
bool BfRtInfoImpl::isActionParam_actProf(const BfRtTableObj *bfrtTable,
                                         bf_rt_id_t action_handle,
                                         std::string parameter_name,
                                         bool use_p4_params_node) {
  Cjson action_table_cjson =
      *(table_cjson_map.at(bfrtTable->object_name).second);

  auto action_table_name = bfrtTable->table_name_get();

  return isActionParam(action_table_cjson,
                       action_handle,
                       action_table_name,
                       parameter_name,
                       use_p4_params_node);
}

bool BfRtInfoImpl::isParamActionParam(const BfRtTableObj *bfrtTable,
                                      bf_rt_id_t action_handle,
                                      std::string parameter_name,
                                      bool use_p4_params_node) {
  // If the table is a Match-Action table call the matchTbl function to analyze
  // if this parameter belongs to the action spec or not
  if (bfrtTable->object_type == BfRtTable::TableType::MATCH_DIRECT) {
    return isActionParam_matchTbl(bfrtTable, action_handle, parameter_name);
  } else if (bfrtTable->object_type == BfRtTable::TableType::ACTION_PROFILE) {
    return isActionParam_actProf(
        bfrtTable, action_handle, parameter_name, use_p4_params_node);
  } else if (bfrtTable->object_type == BfRtTable::TableType::PORT_METADATA) {
    // Phase0 can have only action params as data
    return true;
  }
  return false;
}

// This variant of the function is to parse the action spec for phase0 tables
std::unique_ptr<bf_rt_info_action_info_t>
BfRtInfoImpl::parseActionSpecsForPhase0(Cjson &common_data_cjson,
                                        Cjson &action_context,
                                        BfRtTableObj *bfrtTable) {
  std::unique_ptr<bf_rt_info_action_info_t> action_info(
      new bf_rt_info_action_info_t());

  action_info->name = static_cast<std::string>(action_context["name"]);
  action_info->act_fn_hdl =
      static_cast<pipe_act_fn_hdl_t>(action_context["handle"]);
  action_info->action_id = action_info->act_fn_hdl;

  // For phase0 tables, the common data that is published is actually the
  // action data for the backend pipe mgr. Thus, we need go through
  // all the common data fields so that we know the actual total size of
  // the action_info.
  size_t offset = 0;
  size_t bitsize = 0;
  for (const auto &common_data : common_data_cjson.getCjsonChildVec()) {
    if ((*common_data)["oneof"].exists()) {
      LOG_ERROR(
          "%s:%d Invalid data field format published for port_metadata table "
          "in bf-rt.json",
          __func__,
          __LINE__);
      BF_RT_ASSERT(0);
    }
    Cjson temp;
    bool dummy;
    auto data_field = parseData(
        *common_data, temp, bfrtTable, offset, bitsize, 0, 0, 0, &dummy);
    if (bfrtTable->common_data_fields.find(data_field->getId()) !=
        bfrtTable->common_data_fields.end()) {
      LOG_ERROR(
          "%s:%d Id \"%u\" Exists for common data of port_metadata table %s",
          __func__,
          __LINE__,
          data_field->getId(),
          bfrtTable->table_name_get().c_str());
      continue;
    }
    // insert data_field in table
    bfrtTable->common_data_fields_names[data_field->getName()] =
        data_field.get();
    bfrtTable->common_data_fields[data_field->getId()] = std::move(data_field);
  }
  action_info->dataSzbits = bitsize;
  action_info->dataSz = offset;

  return action_info;
}

// This variant of the function is to parse the action specs for all the tables
// except phase0 table
std::unique_ptr<bf_rt_info_action_info_t> BfRtInfoImpl::parseActionSpecs(
    Cjson &action_bfrt, Cjson &action_context, const BfRtTableObj *bfrtTable) {
  std::unique_ptr<bf_rt_info_action_info_t> action_info(
      new bf_rt_info_action_info_t());

  action_info->name = static_cast<std::string>(action_bfrt["name"]);
  action_info->action_id = static_cast<bf_rt_id_t>(action_bfrt["id"]);
  action_info->annotations = parseAnnotations(action_bfrt["annotations"]);
  Cjson action_indirect;  // dummy
  if (action_context.exists()) {
    action_info->act_fn_hdl =
        static_cast<pipe_act_fn_hdl_t>(action_context["handle"]);
    action_indirect = action_context["indirect_resources"];
  } else {
    action_info->act_fn_hdl = 0;
  }
  // get action profile data
  Cjson action_data_cjson = action_bfrt["data"];
  size_t offset = 0;
  size_t bitsize = 0;
  for (const auto &action_data : action_data_cjson.getCjsonChildVec()) {
    auto data_field = parseData(*action_data,
                                action_indirect,
                                bfrtTable,
                                offset,
                                bitsize,
                                action_info->act_fn_hdl,
                                action_info->action_id,
                                0);
    if (action_info->data_fields.find(data_field->getId()) !=
        action_info->data_fields.end()) {
      LOG_ERROR("%s:%d Key \"%u\" Exists for data ",
                __func__,
                __LINE__,
                data_field->getId());
      continue;
    }
    action_info->data_fields_names[data_field->getName()] = data_field.get();
    action_info->data_fields[data_field->getId()] = std::move(data_field);
  }
  // Offset value will be the total size of the action spec byte array
  action_info->dataSz = offset;
  // Bitsize is needed to fill out some back-end info
  // which has both action size in bytes and bits
  action_info->dataSzbits = bitsize;
  return action_info;
}

void BfRtInfoImpl::set_direct_register_data_field_type(
    const std::vector<bf_rt_id_t> &data_fields, BfRtTableObj *tbl) {
  bf_rt_id_t hi_value_id;
  bf_rt_id_t lo_value_id;
  auto dual_width = false;
  if (data_fields.size() == 2) {
    dual_width = true;
    get_register_field_hi_and_low_values(
        data_fields, &hi_value_id, &lo_value_id);
  }
  if (dual_width) {
    // First check if the data field type for this field is not set already.
    // If it is set, then we need to assert here as this function is supposed
    // to set the data field type of registers
    const BfRtTableDataField *data_field;
    tbl->getDataField(hi_value_id, &data_field);
    auto data_field_type = data_field->getTypes();
    if (data_field_type.size()) {
      LOG_ERROR(
          "%s:%d Unexpected field type found to be set for register field",
          __func__,
          __LINE__);
      BF_RT_ASSERT(0);
    }
    tbl->getDataField(lo_value_id, &data_field);
    data_field_type = data_field->getTypes();
    if (data_field_type.size()) {
      LOG_ERROR(
          "%s:%d Unexpected field type found to be set for register field",
          __func__,
          __LINE__);
      BF_RT_ASSERT(0);
    }
    tbl->addDataFieldType(hi_value_id, DataFieldType::REGISTER_SPEC_HI);
    tbl->addDataFieldType(lo_value_id, DataFieldType::REGISTER_SPEC_LO);
  } else {
    // First check if the data field type for this field is not set already.
    // If it is set, then we need to assert here as this function is supposed
    // to set the data field type of registers
    const BfRtTableDataField *data_field;
    tbl->getDataField(data_fields[0], &data_field);
    auto data_field_type = data_field->getTypes();
    if (data_field_type.size()) {
      LOG_ERROR(
          "%s:%d Unexpected field type found to be set for register field",
          __func__,
          __LINE__);
      BF_RT_ASSERT(0);
    }
    tbl->addDataFieldType(data_fields[0], DataFieldType::REGISTER_SPEC);
  }
  return;
}

void BfRtInfoImpl::set_register_data_field_type(BfRtTableObj *tbl) {
  std::vector<bf_rt_id_t> data_fields;
  bf_rt_id_t hi_value_id;
  bf_rt_id_t lo_value_id;
  bf_status_t status = tbl->dataFieldIdListGet(&data_fields);
  BF_RT_ASSERT(status == BF_SUCCESS);
  bool dual_width = false;
  if (data_fields.size() == 2) {
    dual_width = true;
    get_register_field_hi_and_low_values(
        data_fields, &hi_value_id, &lo_value_id);
  }
  if (dual_width) {
    tbl->addDataFieldType(hi_value_id, DataFieldType::REGISTER_SPEC_HI);
    tbl->addDataFieldType(lo_value_id, DataFieldType::REGISTER_SPEC_LO);
  } else {
    tbl->addDataFieldType(data_fields[0], DataFieldType::REGISTER_SPEC);
  }
  return;
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
bf_status_t BfRtInfoImpl::setGhostTableHandles(Cjson &table_context) {
  const std::string table_name = table_context["name"];
  pipe_mat_tbl_hdl_t pipe_hdl = table_context["handle"];

  for (const auto &refname : indirect_refname_list) {
    // get json object *_table_refs from the table
    Cjson indirect_resource_context_cjson = table_context[refname.c_str()];
    for (const auto &indirect_entry :
         indirect_resource_context_cjson.getCjsonChildVec()) {
      // get all info including id and handle
      bf_rt_table_ref_info_t ref_info;
      ref_info.name = static_cast<std::string>((*indirect_entry)["name"]);
      if (refname != "stateful_table_refs") {
        // TODO currently we support ghost table handling only for stateful
        // tables
        continue;
      }
      // Get hold of the resource table and set the ghost_pipe_hdl
      const auto &ele = tableMap.find(ref_info.name);
      if (ele == tableMap.end()) {
        // This indicates that the resource table for which the compiler
        // generated this ghost itself doesn't exist, which sounds really
        // wrong. Hence assert
        LOG_ERROR(
            "%s:%d table %s was not found in the tableMap for all the tables",
            __func__,
            __LINE__,
            ref_info.name.c_str());
        BF_RT_DBGCHK(0);
        return BF_OBJECT_NOT_FOUND;
      }
      auto res_table = static_cast<BfRtTableObj *>(ele->second.get());
      auto bf_status = res_table->ghostTableHandleSet(pipe_hdl);
      if (bf_status != BF_SUCCESS) {
        LOG_ERROR("%s:%d Unable to set ghost table handle for table %s",
                  __func__,
                  __LINE__,
                  ref_info.name.c_str());
        return bf_status;
      }
      // Currently we support only entry scope table attribute to be set on
      // resource tables
      // Set the entry scope supported flag
      res_table->attributes_type_set.insert(TableAttributesType::ENTRY_SCOPE);
    }
  }
  return BF_SUCCESS;
}

pipe_tbl_hdl_t BfRtInfoImpl::getPvsHdl(Cjson &parser_context_obj) {
  if (parser_context_obj["pvs_handle"].exists()) {
    return Cjson(parser_context_obj, "pvs_handle");
  }
  return 0;
}

// This function determines the offset and the size(in bytes) of the field
bf_status_t BfRtInfoImpl::keyByteSizeAndOffsetGet(
    const std::string &table_name,
    const std::string &key_name,
    const std::map<std::string, size_t> &match_key_field_name_to_position_map,
    const std::map<std::string, size_t> &
        match_key_field_name_to_parent_field_byte_size_map,
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
    // BF_RT_DBGCHK(0);
    return BF_OBJECT_NOT_FOUND;
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
    BF_RT_DBGCHK(0);
    return BF_OBJECT_NOT_FOUND;
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
    BF_RT_DBGCHK(0);
    return BF_OBJECT_NOT_FOUND;
  }
  *parent_field_byte_size = iter_3->second;
  return BF_SUCCESS;
}

bf_status_t BfRtInfoImpl::verifySupportedApi(const BfRtTableObj &bfrtTable,
                                             const Cjson &table_bfrt) const {
  std::string table_name = table_bfrt["name"];
  bool fmt_error = false;
  int f_cnt = 0;

  // Verify fixed function tables' API implemented vs. declared by the json.
  if (table_bfrt["supported_apis"].exists()) {
    Cjson table_apis = table_bfrt["supported_apis"];

    BfRtTable::TableApiSet v_apis;
    auto bf_status = bfrtTable.tableApiSupportedGet(&v_apis);
    if (bf_status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s unable to get implemented APIs",
                __func__,
                __LINE__,
                table_name.c_str());
      BF_RT_DBGCHK(0);
      return BF_UNEXPECTED;
    }

    BfRtTable::TableApi api_id = BfRtTable::TableApi::INVALID_API;
    std::string a_key("");
    for (const auto &t_api : table_apis.getCjsonChildVec()) {
      a_key = t_api->getCjsonKey();
      if (a_key.empty()) {
        fmt_error = true;
        break;
      }
      api_id = getTableApi(a_key);
      if (api_id == BfRtTable::TableApi::INVALID_API) {
        fmt_error = true;
        break;
      }
      f_cnt++;
      if (v_apis.find(api_id) != v_apis.end()) {
        v_apis.erase(api_id);
      } else {
        LOG_WARN("%s:%d %s API declaration %s is not implemented",
                 __func__,
                 __LINE__,
                 table_name.c_str(),
                 a_key.c_str());
        continue;
      }
    }

    if (fmt_error) {
      LOG_WARN("%s:%d %s invalid 'supported_apis' format",
               __func__,
               __LINE__,
               table_name.c_str());
    }
    if (v_apis.size()) {
      LOG_WARN("%s:%d %s has %zu implemented APIs without json declarations",
               __func__,
               __LINE__,
               table_name.c_str(),
               v_apis.size());
    }
    LOG_DBG("%s:%d %s has %d supported APIs",
            __func__,
            __LINE__,
            table_name.c_str(),
            f_cnt);
  } else {
    LOG_DBG("%s:%d %s has no supported API declarations",
            __func__,
            __LINE__,
            table_name.c_str());
  }

  return BF_SUCCESS;
}

std::unique_ptr<BfRtTableObj> BfRtInfoImpl::parseFixedTable(
    const std::string &prog_name, Cjson &table_bfrt) {
  std::string table_name = table_bfrt["name"];
  std::string table_type = table_bfrt["table_type"];
  bf_rt_id_t table_id = table_bfrt["id"];
  size_t table_size = static_cast<unsigned int>(table_bfrt["size"]);

  LOG_DBG("Table : %s :: Type :: %s", table_name.c_str(), table_type.c_str());
  std::unique_ptr<BfRtTableObj> bfrtTable = BfRtTableObj::make_table(
      prog_name, getTableType(table_type), table_id, table_name, table_size);
  if (bfrtTable == nullptr) {
    return nullptr;
  }

  if (BF_SUCCESS != verifySupportedApi(*bfrtTable, table_bfrt)) {
    return nullptr;
  }

  // getting key
  Cjson table_key_cjson = table_bfrt["key"];
  std::map<std::string /*match_key_field_name*/, size_t /* position*/>
      match_key_field_name_to_position_map;
  std::map<std::string /*match_key_field_name*/,
           size_t /* size of parent field in bytes */>
      match_key_field_name_to_parent_field_byte_size_map;
  std::map<size_t /* position */, size_t /* offset */>
      match_key_field_position_to_offset_map;

  size_t cumulative_offset = 0;
  size_t cumulative_key_width_bits = 0;

  // Extract all the relevant information into the helper maps
  parseKeyHelper(nullptr /* context json node */,
                 &table_key_cjson,
                 *bfrtTable,
                 &match_key_field_name_to_position_map,
                 &match_key_field_position_to_offset_map,
                 &cumulative_offset,
                 &cumulative_key_width_bits,
                 &match_key_field_name_to_parent_field_byte_size_map);

  for (const auto &key : table_key_cjson.getCjsonChildVec()) {
    const std::string key_name = getParentKeyName(*key);
    const size_t start_bit =
        getStartBit(nullptr /* context json node */, key.get(), *bfrtTable);
    size_t field_offset = 0, parent_field_byte_size = 0;
    // Get the field offset and the size of the field in bytes
    if (keyByteSizeAndOffsetGet(
            table_name,
            key_name,
            match_key_field_name_to_position_map,
            match_key_field_name_to_parent_field_byte_size_map,
            match_key_field_position_to_offset_map,
            &field_offset,
            &parent_field_byte_size) != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR in processing key field %s while trying to get field "
          "size and offset",
          __func__,
          __LINE__,
          table_name.c_str(),
          key_name.c_str());
      BF_RT_DBGCHK(0);
      return nullptr;
    }
    // Finally form the key field object
    std::unique_ptr<BfRtTableKeyField> key_field = parseKey(
        *key, table_id, field_offset, start_bit, parent_field_byte_size, false);
    if (key_field == nullptr) {
      continue;
    }
    auto elem = bfrtTable->key_fields.find(key_field->getId());
    if (elem == bfrtTable->key_fields.end()) {
      bfrtTable->key_fields[key_field->getId()] = (std::move(key_field));
    } else {
      // Field id is repeating, log an error message
      LOG_ERROR("%s:%d Field ID %d is repeating",
                __func__,
                __LINE__,
                key_field->getId());
      return nullptr;
    }
  }

  // Set the table key size in terms of bytes. The field_offset will be the
  // total size of the key
  bfrtTable->key_size.bytes = cumulative_offset;

  // Set the table key in terms of exact bit widths.
  bfrtTable->key_size.bits = cumulative_key_width_bits;

  // getting Attributes
  std::vector<std::string> attributes_v =
      table_bfrt["attributes"].getCjsonChildStringVec();
  for (auto const &item : attributes_v) {
    if (item == "port_status_notif_cb") {
      bfrtTable->attributes_type_set.insert(
          TableAttributesType::PORT_STATUS_NOTIF);
    } else if (item == "poll_intvl_ms") {
      bfrtTable->attributes_type_set.insert(
          TableAttributesType::PORT_STAT_POLL_INTVL_MS);
    }
  }
  // getting action
  Cjson table_action_spec_cjson = table_bfrt["action_specs"];

  for (const auto &action : table_action_spec_cjson.getCjsonChildVec()) {
    std::unique_ptr<bf_rt_info_action_info_t> action_info;
    Cjson dummy;
    action_info = parseActionSpecs(*action, dummy, bfrtTable.get());
    auto elem = bfrtTable->action_info_list.find(action_info->action_id);
    if (elem == bfrtTable->action_info_list.end()) {
      auto act_id = action_info->action_id;
      auto act_name = action_info->name;
      bfrtTable->action_info_list[action_info->action_id] =
          (std::move(action_info));
      bfrtTable->action_info_list_name[act_name] =
          bfrtTable->action_info_list[act_id].get();
    } else {
      // Action id is repeating. Log an error message
      LOG_ERROR("%s:%d Action ID %d is repeating",
                __func__,
                __LINE__,
                action_info->action_id);
      return nullptr;
    }
  }

  // getting data
  Cjson common_data_cjson = table_bfrt["data"];
  for (const auto &common_data : common_data_cjson.getCjsonChildVec()) {
    std::string data_name;
    size_t offset = 0;
    // Bitsize is needed to fill out some back-end info
    // which has both action size in bytes and bits
    // This is not applicable for the common data, but only applicable
    // for per action data
    size_t bitsize = 0;
    int oneof_size = 1;
    if ((*common_data)["oneof"].exists()) {
      oneof_size = (*common_data)["oneof"].array_size();
    }
    Cjson temp;
    for (int oneof_loop = 0; oneof_loop < oneof_size; oneof_loop++) {
      bool is_register_data_field = false;
      auto data_field = parseData(*common_data,
                                  temp,
                                  bfrtTable.get(),
                                  offset,
                                  bitsize,
                                  0,
                                  0,
                                  oneof_loop,
                                  &is_register_data_field);
      bf_rt_id_t data_field_id = data_field->getId();
      if (is_register_data_field) {
        // something is not right
        LOG_ERROR(
            "%s:%d fixed table %s should not have register data field id %d",
            __func__,
            __LINE__,
            bfrtTable->table_name_get().c_str(),
            data_field_id);
        continue;
      }
      if (bfrtTable->common_data_fields.find(data_field_id) !=
          bfrtTable->common_data_fields.end()) {
        LOG_ERROR("%s:%d Id \"%u\" Exists for common data of table %s",
                  __func__,
                  __LINE__,
                  data_field_id,
                  bfrtTable->table_name_get().c_str());
        continue;
      }
      // insert data_field in table
      bfrtTable->common_data_fields_names[data_field->getName()] =
          data_field.get();
      bfrtTable->common_data_fields[data_field_id] = std::move(data_field);
    }
  }
  populate_depends_on_refs(bfrtTable.get(), table_bfrt);
  return bfrtTable;
}

std::unique_ptr<BfRtTableObj> BfRtInfoImpl::parseTable(
    const bf_dev_id_t &dev_id,
    const std::string &prog_name,
    Cjson &table_bfrt,
    Cjson &table_context) {
  std::string table_name = table_bfrt["name"];
  std::string table_type = table_bfrt["table_type"];
  bf_rt_id_t table_id = table_bfrt["id"];
  size_t table_size = static_cast<unsigned int>(table_bfrt["size"]);

  LOG_DBG("Table : %s :: Type :: %s", table_name.c_str(), table_type.c_str());

  // Create BfRtTableObj object
  pipe_tbl_hdl_t table_hdl;
  if (table_type == "ParserValueSet") {
    table_hdl = getPvsHdl(table_context);
  } else {
    table_hdl = Cjson(table_context, "handle");
  }
  std::unique_ptr<BfRtTableObj> bfrtTable =
      BfRtTableObj::make_table(prog_name,
                               getTableType(table_type),
                               table_id,
                               table_name,
                               table_size,
                               table_hdl);
  bfrtTable->setIsTernaryTable(dev_id);

  if (bfrtTable == nullptr) {
    return nullptr;
  }
  // Add refs of the table
  for (const auto &refname : indirect_refname_list) {
    // get json object *_table_refs from the table
    Cjson indirect_resource_context_cjson = table_context[refname.c_str()];
    for (const auto &indirect_entry :
         indirect_resource_context_cjson.getCjsonChildVec()) {
      // get all info including id and handle
      bf_rt_table_ref_info_t ref_info;
      ref_info.name = static_cast<std::string>((*indirect_entry)["name"]);
      ref_info.tbl_hdl =
          static_cast<pipe_tbl_hdl_t>((*indirect_entry)["handle"]);
      if (table_cjson_map.find(ref_info.name) == table_cjson_map.end()) {
        LOG_ERROR("%s:%d table %s was not found in map",
                  __func__,
                  __LINE__,
                  ref_info.name.c_str());
      } else {
        // if how referenced is 'direct', then we do not have an
        // "id" per se.
        if (static_cast<std::string>((*indirect_entry)["how_referenced"]) !=
            "direct") {
          auto res_bfrt_cjson = table_cjson_map.at(ref_info.name).first;
          if (res_bfrt_cjson) {
            ref_info.id = static_cast<bf_rt_id_t>((*res_bfrt_cjson)["id"]);
            ref_info.indirect_ref = true;
          } else {
            // This is an error since the indirect ref table blob should always
            // be present in the bf-rt json
            LOG_ERROR("%s:%d BF_RT json blob not present for table %s",
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
              bfrtTable->table_name_get().c_str());
      bfrtTable->table_ref_map[refname].push_back(std::move(ref_info));
    }
  }

  // A table with constant entries might have its 'match_attributes' with
  // conditions published as an attached gateway table (named with '-gateway'
  // suffix in context.json).
  // TODO Should be an attribute of BfRtTableObj class.
  bool is_gateway_table_key = false;

  if (bfrtTable->object_type == BfRtTableObj::TableType::MATCH_DIRECT ||
      bfrtTable->object_type == BfRtTableObj::TableType::MATCH_INDIRECT ||
      bfrtTable->object_type ==
          BfRtTableObj::TableType::MATCH_INDIRECT_SELECTOR) {
    // If static entries node is present then check the array size,
    // else check if it is an ALPM or ATCAM table and then check for static
    // entries accordingly
    if (table_context["static_entries"].exists()) {
      if (table_context["static_entries"].array_size() > 0) {
        bfrtTable->is_const_table_ = true;
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
            bfrtTable->is_const_table_ = true;
            break;
          }
        }
      } else if (is_alpm) {
        // There is no way to specify const entries for alpm tables as of now
        // due to language restrictions. The below will help it make work when
        // it becomes supported.
        if (table_context["match_attributes"]["pre_classifier"]
                         ["static_entries"].array_size() > 0) {
          bfrtTable->is_const_table_ = true;
        }
      }
    }  // if table_context["static_entries"].exists()

    // Look for any stage table match attributes with gateway table attached.
    // TODO it should be easier when 'gateway_with_entries' stage table type
    // will be implemented.
    if (table_context["match_attributes"].exists() &&
        table_context["match_attributes"]["match_type"].exists() &&
        table_context["match_attributes"][STAGE_TABLE_STR].exists()) {
      std::string match_type_ = table_context["match_attributes"]["match_type"];
      if (match_type_ == "match_with_no_key") {
        Cjson s_tbl_cjson_ = table_context["match_attributes"][STAGE_TABLE_STR];
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

  if (bfrtTable->object_type == BfRtTableObj::TableType::SELECTOR) {
    // For selector table add the reference of the action profile table
    bf_rt_table_ref_info_t ref_info;
    ref_info.tbl_hdl = static_cast<pipe_tbl_hdl_t>(
        table_context["bound_to_action_data_table_handle"]);
    ref_info.indirect_ref = true;
    // Search for the ref tbl handle in the table_cjson_map in context_json
    // blobs
    ref_info.name = "";
    for (const auto &json_pair : table_cjson_map) {
      if (json_pair.second.second != nullptr) {
        auto handle =
            static_cast<bf_rt_id_t>((*(json_pair.second.second))["handle"]);
        if (handle == ref_info.tbl_hdl) {
          ref_info.name =
              static_cast<std::string>((*(json_pair.second.second))["name"]);
          break;
        }
      }
    }
    // Apply the mask on the handle now so that it doesn't interfere with the
    // lookup in the above for loop
    auto res_bfrt_cjson = table_cjson_map.at(ref_info.name).first;
    if (res_bfrt_cjson) {
      ref_info.id = static_cast<bf_rt_id_t>((*res_bfrt_cjson)["id"]);
    } else {
      // This is an error since the action table blob should always
      // be present in the bf-rt json
      LOG_ERROR("%s:%d BF_RT json blob not present for table %s",
                __func__,
                __LINE__,
                ref_info.name.c_str());
      BF_RT_ASSERT(0);
    }
    bfrtTable->table_ref_map["action_data_table_refs"].push_back(
        std::move(ref_info));
  }

  // getting key
  Cjson table_context_match_fields_cjson = table_context["match_key_fields"];
  Cjson table_key_cjson = table_bfrt["key"];

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
                 &table_key_cjson,
                 *bfrtTable,
                 &match_key_field_name_to_position_map,
                 &match_key_field_position_to_offset_map,
                 &cumulative_offset,
                 &cumulative_key_width_bits,
                 &match_key_field_name_to_parent_field_byte_size_map);

  for (const auto &key : table_key_cjson.getCjsonChildVec()) {
    const std::string key_name = getParentKeyName(*key);
    std::string partition_name =
        table_context["match_attributes"]["partition_field_name"];
    bool is_atcam_partition_index = (partition_name == key_name) ? true : false;
    size_t start_bit = 0;
    // TODO Do not skip parse on is_gateway_table_key when the attached gateway
    // condition table will be correctly exposed by the compiler.
    if (!is_atcam_partition_index && !is_gateway_table_key) {
      start_bit =
          getStartBit(&table_context_match_fields_cjson, key.get(), *bfrtTable);
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
              &parent_field_byte_size) != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s ERROR in processing key field %s while trying to get "
            "field size and offset",
            __func__,
            __LINE__,
            table_name.c_str(),
            key_name.c_str());
        // BF_RT_DBGCHK(0);
        return nullptr;
      }
    } else {
      // We don't pack the match priority key field in the bytes array in the
      // match spec and hence this field is also not published in the context
      // json key node. So set the field offset to an invalid value
      field_offset = 0xffffffff;
      parent_field_byte_size = sizeof(uint32_t);
    }

    // Finally form the key field object
    std::unique_ptr<BfRtTableKeyField> key_field =
        parseKey(*key,
                 table_id,
                 field_offset,
                 start_bit,
                 parent_field_byte_size,
                 is_atcam_partition_index);
    if (key_field == nullptr) {
      continue;
    }
    auto elem = bfrtTable->key_fields.find(key_field->getId());
    if (elem == bfrtTable->key_fields.end()) {
      bfrtTable->key_fields[key_field->getId()] = (std::move(key_field));
    } else {
      // Field id is repeating, log an error message
      LOG_ERROR("%s:%d Field ID %d is repeating",
                __func__,
                __LINE__,
                key_field->getId());
      return nullptr;
    }
  }

  // Set the table key size in terms of bytes. The field_offset will be the
  // total size of the key
  bfrtTable->key_size.bytes = cumulative_offset;

  // Set the table key in terms of exact bit widths.
  bfrtTable->key_size.bits = cumulative_key_width_bits;

  // Parse operations
  std::vector<std::string> supported_operations_v =
      table_bfrt["supported_operations"].getCjsonChildStringVec();
  for (const auto &operation_str : supported_operations_v) {
    BfRtTable::TableType tableType;
    bfrtTable->tableTypeGet(&tableType);
    bfrtTable->operations_type_set.insert(
        BfRtTableOperationsImpl::getType(operation_str, tableType));
  }
  // get Attributes
  std::vector<std::string> attributes_v =
      table_bfrt["attributes"].getCjsonChildStringVec();
  for (auto const &item : attributes_v) {
    if (item == "IdleTimeout") {
      bfrtTable->attributes_type_set.insert(
          TableAttributesType::IDLE_TABLE_RUNTIME);
    } else if (item == "EntryScope") {
      bfrtTable->attributes_type_set.insert(TableAttributesType::ENTRY_SCOPE);
    } else if (item == "MeterByteCountAdjust") {
      bfrtTable->attributes_type_set.insert(
          TableAttributesType::METER_BYTE_COUNT_ADJ);
    } else if (item == "SelectorUpdateCb") {
      bfrtTable->attributes_type_set.insert(
          TableAttributesType::SELECTOR_UPDATE_CALLBACK);
    }
  }

  // get annotations
  bfrtTable->annotations_ = parseAnnotations(table_bfrt["annotations"]);

  // getting action profile
  Cjson table_action_spec_cjson = table_bfrt["action_specs"];

  // If the table type is action, let's find the first match table
  // which has action_data_table_refs as this action profile table.
  // If none was found, then continue with this table itself.
  Cjson table_action_spec_context_cjson = table_context["actions"];
  if (getTableType(table_type) == BfRtTable::TableType::ACTION_PROFILE) {
    for (const auto &kv : table_cjson_map) {
      if (kv.second.first != nullptr && kv.second.second != nullptr) {
        const auto &table_bfrt_node = *(kv.second.first);
        const auto &table_context_node = *(kv.second.second);
        const auto &curr_table_type =
            getTableType(table_bfrt_node["table_type"]);
        if (curr_table_type == BfRtTable::TableType::MATCH_INDIRECT ||
            curr_table_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR) {
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

              bf_rt_table_ref_info_t ref_info;
              ref_info.indirect_ref = true;
              ref_info.tbl_hdl = static_cast<pipe_tbl_hdl_t>(
                  (*(table_context_node[ref.c_str()]
                         .getCjsonChildVec())[0])["handle"]);
              ref_info.name = static_cast<std::string>(
                  (*(table_context_node[ref.c_str()]
                         .getCjsonChildVec())[0])["name"]);

              auto res_bfrt_cjson = table_cjson_map.at(ref_info.name).first;
              if (res_bfrt_cjson) {
                ref_info.id = static_cast<bf_rt_id_t>((*res_bfrt_cjson)["id"]);
              }

              bfrtTable->table_ref_map[ref.c_str()].push_back(
                  std::move(ref_info));
            }

            break;
          }
        }
      }
    }
  }

  // create a map of (action name -> pair<bfrtActionCjson, contextActionCjson>)
  std::map<std::string,
           std::pair<std::shared_ptr<Cjson>, std::shared_ptr<Cjson>>>
      action_cjson_map;

  for (const auto &action : table_action_spec_cjson.getCjsonChildVec()) {
    std::string action_name = (*action)["name"];
    action_cjson_map[action_name] = std::make_pair(action, nullptr);
  }
  for (const auto &action :
       table_action_spec_context_cjson.getCjsonChildVec()) {
    std::string action_name = (*action)["name"];
    if (action_cjson_map.find(action_name) != action_cjson_map.end()) {
      action_cjson_map[action_name].second = action;
    } else {
      if (bfrtTable->object_type == BfRtTable::TableType::PORT_METADATA) {
        // This is an acceptable case for phase0 tables for the action spec is
        // not published in the bf-rt json but it is published in the context
        // json. Hence add it to the map
        action_cjson_map[action_name] = std::make_pair(nullptr, action);
      } else {
        LOG_WARN(
            "%s:%d Action %s present in context json but not in bfrt json \
                 for table %s",
            __func__,
            __LINE__,
            action_name.c_str(),
            bfrtTable->table_name_get().c_str());
        continue;
      }
    }
  }
  BfRtTable::TableType table_type_bfrt;
  bfrtTable->tableTypeGet(&table_type_bfrt);
  // Check if the table has const default action
  bfrtTable->has_const_default_action_ = table_bfrt["has_const_default_action"];
  // Now parseAction with help of both the Cjsons from the map
  for (const auto &kv : action_cjson_map) {
    std::unique_ptr<bf_rt_info_action_info_t> action_info;
    if (kv.second.first == nullptr) {
      // Phase0 table
      Cjson common_data_cjson = table_bfrt["data"];
      action_info = parseActionSpecsForPhase0(
          common_data_cjson, *kv.second.second, bfrtTable.get());
    } else {
        // All others tables
        action_info = parseActionSpecs(
            *kv.second.first, *kv.second.second, bfrtTable.get());
    }
    auto elem = bfrtTable->action_info_list.find(action_info->action_id);
    if (elem == bfrtTable->action_info_list.end()) {
      // Maintain a mapping from action function handle to action id, which is
      // required for entry reads
      bf_rt_id_t a_id = action_info->action_id;
      auto act_name = action_info->name;
      bfrtTable->act_fn_hdl_to_id[action_info->act_fn_hdl] = a_id;
      bfrtTable->action_info_list[action_info->action_id] =
          (std::move(action_info));
      bfrtTable->action_info_list_name[act_name] =
          bfrtTable->action_info_list[a_id].get();

    } else {
      // Action id is repeating. Log an error message
      LOG_ERROR("%s:%d Action ID %d is repeating",
                __func__,
                __LINE__,
                action_info->action_id);
      return nullptr;
    }
  }

  // Update action function resource usage map.
  bfrtTable->setActionResources(dev_id);

  std::vector<bf_rt_id_t> register_data_field_id_v;
  // If the table is phase0, then we would have already parsed the common
  // data fields while parsing the action specs. Hence don't do it
  // again
  if (bfrtTable->object_type != BfRtTable::TableType::PORT_METADATA) {
    // get common data
    Cjson common_data_cjson = table_bfrt["data"];
    for (const auto &common_data : common_data_cjson.getCjsonChildVec()) {
      std::string data_name;
      size_t offset = 0;
      // Bitsize is needed to fill out some back-end info
      // which has both action size in bytes and bits
      // This is not applicable for the common data, but only applicable
      // for per action data
      size_t bitsize = 0;
      // TODO correct the zeroes down
      // making a null temp object to pass as contextJson info since we dont
      // need
      // that
      Cjson temp;
      //
      int oneof_size = 1;
      if ((*common_data)["oneof"].exists()) {
        oneof_size = (*common_data)["oneof"].array_size();
      }
      for (int oneof_loop = 0; oneof_loop < oneof_size; oneof_loop++) {
        bool is_register_data_field = false;
        auto data_field = parseData(*common_data,
                                    temp,
                                    bfrtTable.get(),
                                    offset,
                                    bitsize,
                                    0,
                                    0,
                                    oneof_loop,
                                    &is_register_data_field);
        bf_rt_id_t data_field_id = data_field->getId();
        if (is_register_data_field) {
          // If this data field is a register, store the field id for further
          // processing
          register_data_field_id_v.push_back(data_field_id);
        }
        if (bfrtTable->common_data_fields.find(data_field_id) !=
            bfrtTable->common_data_fields.end()) {
          LOG_ERROR("%s:%d Id \"%u\" Exists for common data of table %s",
                    __func__,
                    __LINE__,
                    data_field_id,
                    bfrtTable->table_name_get().c_str());
          continue;
        }
        // insert data_field in table
        bfrtTable->common_data_fields_names[data_field->getName()] =
            data_field.get();
        bfrtTable->common_data_fields[data_field_id] = std::move(data_field);
      }
    }
  }

  // For register tables set data field type
  if (bfrtTable->object_type == BfRtTable::TableType::REGISTER) {
    set_register_data_field_type(bfrtTable.get());
  } else if (register_data_field_id_v.size()) {
    // Since the table type is not REGISTER, but we have a non empty register
    // field id vec, it indicates that this is a direct register table.
    set_direct_register_data_field_type(register_data_field_id_v,
                                        bfrtTable.get());
  }
  populate_depends_on_refs(bfrtTable.get(), table_bfrt);
  return bfrtTable;
}

void BfRtInfoImpl::populateParserObj(const std::string &profile_name,
                                     Cjson &parser_gress_ctxt) {
	return;
}

void BfRtInfoImpl::populate_depends_on_refs(BfRtTableObj *bfrtTable,
                                            const Cjson &table_bfrt) {
  // getting depends_on
  Cjson depends_on_cjson = table_bfrt["depends_on"];
  std::string ref_name = "other";
  for (const auto &tbl_id : depends_on_cjson.getCjsonChildVec()) {
    bf_rt_table_ref_info_t ref_info;
    ref_info.id = static_cast<bf_rt_id_t>(*tbl_id);
    ref_info.name = "";
    for (auto &tbl_bfrt_cjson : table_cjson_map) {
      // Check if table present in bf-rt.json if so then verify if id match
      if (tbl_bfrt_cjson.second.first != nullptr &&
          static_cast<bf_rt_id_t>((*tbl_bfrt_cjson.second.first)["id"]) ==
              ref_info.id) {
        ref_info.name = tbl_bfrt_cjson.first;
      }
    }
    if (ref_info.name == "") {
      LOG_TRACE("%s:%d Cannot find proper reference table info for id %u",
                __func__,
                __LINE__,
                ref_info.id);
      break;
    }
    auto context_cjson = table_cjson_map.at(ref_info.name).second;
    if (!context_cjson || !context_cjson->exists()) continue;

    ref_info.tbl_hdl = static_cast<bf_rt_id_t>((*context_cjson)["handle"]);
    ref_info.indirect_ref = true;

    LOG_DBG("%s:%d Adding \'%s\' as \'%s\' of \'%s\'",
            __func__,
            __LINE__,
            ref_info.name.c_str(),
            ref_name.c_str(),
            bfrtTable->table_name_get().c_str());
    bfrtTable->table_ref_map[ref_name].push_back(std::move(ref_info));
  }
}

///// Public APIs
BfRtInfoImpl::BfRtInfoImpl(const bf_dev_id_t &dev_id,
                           const ProgramConfig &program_config)
    : program_config_(program_config) {
  if (program_config_.bfrtInfoFilePathVect_.empty()) {
    LOG_CRIT("Unable to find any BfRt Json File");
    throw fileOpenFailException();
  }
  // I: Parse bf_rt.json
  for (auto const &bfrtInfoFilePath : program_config_.bfrtInfoFilePathVect_) {
    std::ifstream file(bfrtInfoFilePath);
    if (file.fail()) {
      LOG_CRIT("Unable to find BfRt Json File %s", bfrtInfoFilePath.c_str());
      throw fileOpenFailException();
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    Cjson root_cjson = Cjson::createCjsonFromFile(content);
    Cjson tables_cjson = root_cjson["tables"];
    for (const auto &table : tables_cjson.getCjsonChildVec()) {
      std::string name = (*table)["name"];
      table_cjson_map[name] = std::make_pair(table, nullptr);
    }
  }
  std::ifstream file_last(program_config_.bfrtInfoFilePathVect_.back());
  std::string content_last((std::istreambuf_iterator<char>(file_last)),
                           std::istreambuf_iterator<char>());
  Cjson root_cjson_last = Cjson::createCjsonFromFile(content_last);
  IPipeMgrIntf *pipe_mgr_obj = PipeMgrIntf::getInstance();
  if (program_config_.prog_name_ == "$SHARED") {
    /*
      not related to p4_program
    */
    // (II): Parse Table for fixed feature without help of context json.
    for (const auto &kv : table_cjson_map) {
      if (!kv.second.first) {
        LOG_DBG("%s:%d Table '%s' Not present in bfrt json",
                __func__,
                __LINE__,
                kv.first.c_str());
        continue;
      }
      std::unique_ptr<BfRtTableObj> bfrtTable =
          parseFixedTable(program_config_.prog_name_, *(kv.second.first));
      // Insert table object in the 2 maps
      if (!bfrtTable) {
        continue;
      }
      bf_rt_id_t table_id = bfrtTable->table_id_get();
      idToNameMap[table_id] = bfrtTable->table_name_get();
      tableMap[bfrtTable->table_name_get()] = std::move(bfrtTable);
    }
    populateFullNameMap<BfRtTable>(this->tableMap, &(this->fullTableMap));
    // For fixed info obj, finish here.
    return;
  }

  std::vector<Cjson> context_json_files;
  // (II): Parse all the context.json for the non-fixed bf_rt table entries now
  for (const auto &p4_pipeline : program_config_.p4_pipelines_) {
    std::ifstream file_context(p4_pipeline.context_json_path_);
    if (file_context.fail()) {
      LOG_CRIT("Unable to find Context Json File %s",
               p4_pipeline.context_json_path_.c_str());
      throw fileOpenFailException();
    }

    std::string content_context((std::istreambuf_iterator<char>(file_context)),
                                std::istreambuf_iterator<char>());
    Cjson root_cjson_context = Cjson::createCjsonFromFile(content_context);
    // Save for future use
    context_json_files.push_back(root_cjson_context);
    // II: Parse context.json, tables part
    Cjson tables_cjson_context = root_cjson_context["tables"];

    // Prepending pipe names to table/resource names in context.json.
    // bf-rt.json contains p4_pipeline names as prefixes but context.json
    // doesn't because a context.json is per pipe already.
    for (auto &table : tables_cjson_context.getCjsonChildVec()) {
      changeTableName(p4_pipeline.name_, &table);
      changeIndirectResourceName(p4_pipeline.name_, &table);
      changeActionIndirectResourceName(p4_pipeline.name_, &table);
    }
    for (const auto &table : tables_cjson_context.getCjsonChildVec()) {
      std::string name = (*table)["name"];
      if (table_cjson_map.find(name) != table_cjson_map.end()) {
        table_cjson_map[name].second = table;
      } else {
        // else this name wasn't present in bfrt.json
        // So it must be a compiler gen table. Lets store it in the map
        // anyway
        table_cjson_map[name] = std::make_pair(nullptr, table);
      }
      table_pipeline_name[name] = p4_pipeline.name_;
    }
    // II: Parse context.json, parser part
    Cjson parser_context = root_cjson_context["parser"];
    if (parser_context) {
      Cjson parser_gress_ctxt = parser_context["ingress"];
      if (parser_gress_ctxt) {
        populateParserObj(p4_pipeline.name_, parser_gress_ctxt);
      }
      parser_gress_ctxt = parser_context["egress"];
      if (parser_gress_ctxt) {
        populateParserObj(p4_pipeline.name_, parser_gress_ctxt);
      }
    }
    Cjson parsers_context = root_cjson_context["parsers"];
    if (parsers_context) {
      Cjson parser_gress_ctxt = parsers_context["ingress"];
      if (parser_gress_ctxt) {
        for (const auto &parser : parser_gress_ctxt.getCjsonChildVec()) {
          Cjson parser_stats_ctxt = (*parser)["states"];
          populateParserObj(p4_pipeline.name_, parser_stats_ctxt);
        }
      }
      parser_gress_ctxt = parsers_context["egress"];
      if (parser_gress_ctxt) {
        for (const auto &parser : parser_gress_ctxt.getCjsonChildVec()) {
          Cjson parser_stats_ctxt = (*parser)["states"];
          populateParserObj(p4_pipeline.name_, parser_stats_ctxt);
        }
      }
    }
    // II: Parse context.json, dyn hash part
    Cjson dynhash_cjson_context =
        root_cjson_context["dynamic_hash_calculations"];
    for (const auto &dynhash : dynhash_cjson_context.getCjsonChildVec()) {
      std::string name = (*dynhash)["name"];
      if (table_cjson_map.find(name) != table_cjson_map.end()) {
        table_cjson_map[name].second = dynhash;
        auto algo_table_name = getAlgoTableName(name);
        table_cjson_map[algo_table_name].second = dynhash;
        table_pipeline_name[name] = p4_pipeline.name_;
        table_pipeline_name[algo_table_name] = p4_pipeline.name_;
      }
    }
    // Get the mask which needs to be applied on all the handles parsed from
    // the context json/s
    bf_rt_id_t context_json_handle_mask;
    pipe_mgr_obj->pipeMgrTblHdlPipeMaskGet(dev_id,
                                           program_config_.prog_name_,
                                           p4_pipeline.name_,
                                           &context_json_handle_mask);
    context_json_handle_mask_map[p4_pipeline.name_] = context_json_handle_mask;
  }

  // III: Now parseTable with help of both the Cjsons from the map
  for (const auto &kv : table_cjson_map) {
    /*
      related to p4_program:
      first = bf-rt.json
      second = context.json
      !first &&  second: check ghost table, do later.
       first && !second: fixed tables, sp:snapshot tables
       first &&  second: p4 tables
      !first && !second: invalid
    */
    std::unique_ptr<BfRtTableObj> bfrtTable;
    // Parse Snapshot and Snapshot-Liveness table
    if (!kv.second.second && kv.second.first) {
      switch (getTableType((*kv.second.first)["table_type"])) {
        case BfRtTable::TableType::PORT_CFG:
        case BfRtTable::TableType::PORT_STAT:
        case BfRtTable::TableType::PORT_HDL_INFO:
        case BfRtTable::TableType::PORT_FRONT_PANEL_IDX_INFO:
        case BfRtTable::TableType::PORT_STR_INFO:
        case BfRtTable::TableType::MIRROR_CFG:
        case BfRtTable::TableType::DEV_CFG:
        case BfRtTable::TableType::REG_PARAM:
        case BfRtTable::TableType::SELECTOR_GET_MEMBER:
        case BfRtTable::TableType::DBG_CNT:
        case BfRtTable::TableType::LOG_DBG_CNT: {
          // fixed tables related to p4 program
          // fixed tables are not published in context json, but this
          // is legal as this is by design, So don't add the table to
          // the invalid_table_names set
          bfrtTable =
              parseFixedTable(program_config_.prog_name_, *(kv.second.first));
          break;
        }
        case BfRtTable::TableType::INVALID: {
          LOG_TRACE("%s:%d %s INVALID Table Type found for table",
                    __func__,
                    __LINE__,
                    kv.first.c_str());
          break;
        }
        default: {
          // Indicates that the table was present in bfrt.json but absent
          // in context as it may have been optimized out
          LOG_DBG(
              "%s:%d %s table optimized out of the context json by the "
              "compiler",
              __func__,
              __LINE__,
              kv.first.c_str());
          invalid_table_names.insert(kv.first);
          break;
        }
      }
    }
    if (kv.second.first && kv.second.second) {
      // p4 dependent tables
      bfrtTable = parseTable(dev_id,
                             program_config_.prog_name_,
                             *(kv.second.first),
                             *(kv.second.second));
    }

    // Insert table object in the 2 maps
    if (!bfrtTable) {
      continue;
    }
    bfrtTable->bfrt_info_ = this;

    bf_rt_id_t table_id = bfrtTable->table_id_get();
    idToNameMap[table_id] = bfrtTable->table_name_get();
    pipe_tbl_hdl_t tbl_hdl = bfrtTable->tablePipeHandleGet();
    BfRtTable::TableType table_type;
    bfrtTable->tableTypeGet(&table_type);
    if (tbl_hdl != 0) {
      if (handleToIdMap.find(tbl_hdl) == handleToIdMap.end()) {
        // table wasn't found in this map. add it
        handleToIdMap[tbl_hdl] = table_id;
      }
    }
    tableMap[bfrtTable->table_name_get()] = std::move(bfrtTable);
  }

  for (const auto &kv : table_cjson_map) {
    if (!kv.second.first && kv.second.second) {
      // Now set the ghost table handles if applicable
      // Since the table blob exists in the context json but not in bf-rt json
      // it indicates that the table was internally generated by the compiler.
      // Hence we need to set the pipe tbl handle of this internally generated
      // table in the resource table (for which it was generated).
      auto bf_status = setGhostTableHandles(*(kv.second.second));
      if (bf_status != BF_SUCCESS) {
        LOG_ERROR("%s:%d Unable to set the ghost table handle for table %s",
                  __func__,
                  __LINE__,
                  kv.first.c_str());
      }
    }
  }

  // Now link MatchAction tables to action profile tables
  for (const auto &each : tableMap) {
    auto table_name = each.first;
    if (!each.second) {
      LOG_ERROR("%s:%d Table object not found for \"%s\"",
                __func__,
                __LINE__,
                table_name.c_str());
      continue;
    }
    BfRtTableObj *tbl = static_cast<BfRtTableObj *>(each.second.get());

    for (const auto &action_table_res_pair :
         tbl->tableGetRefNameVec("action_data_table_refs")) {
      auto elem = tableMap.find(action_table_res_pair.name);
      if (elem != tableMap.end()) {
        // Direct addressed tables will not be present in bf-rt.json and we
        // don't need direct to link match-action tables to direct addressed
        // action tables
        tbl->actProfTbl = static_cast<BfRtTableObj *>(elem->second.get());
      }
    }

    // For selector tables, store the action profile table ID
    if (tbl->object_type == BfRtTable::TableType::SELECTOR) {
      auto ctx_json = *(table_cjson_map[tbl->object_name].second);
      pipe_tbl_hdl_t act_prof_tbl_hdl =
          ctx_json["bound_to_action_data_table_handle"];
      tbl->act_prof_id = handleToIdMap[act_prof_tbl_hdl];
    }
    // For MatchActionIndirect_Selector tables, store the selector table id
    // in
    // the match table
    if (tbl->object_type == BfRtTable::TableType::MATCH_INDIRECT_SELECTOR) {
      auto ctx_json = *(table_cjson_map[tbl->object_name].second);
      pipe_tbl_hdl_t sel_tbl_hdl =
          ctx_json["selection_table_refs"][0]["handle"];
      tbl->selector_tbl_id = handleToIdMap[sel_tbl_hdl];
      auto elem = tableMap.find(static_cast<std::string>(
          ctx_json["selection_table_refs"][0]["name"]));
      if (elem != tableMap.end()) {
        tbl->selectorTbl = static_cast<BfRtTableObj *>(elem->second.get());
      }
    }
  }

  // Build the table graph at the end
  buildDependencyGraph();
  populateFullNameMap<BfRtTable>(this->tableMap, &(this->fullTableMap));
  // Clear the table cjson map since we don't need it any more
  table_cjson_map.clear();
}

// This function will go over all the tables and build a graph to enumerate
// all the tables that depend on that table
void BfRtInfoImpl::buildDependencyGraph() {
  // Initialize all the table with an empty vector
  for (const auto &item : tableMap) {
    const auto &table = static_cast<const BfRtTableObj &>(*item.second);
    tables_dependent_on_this_table_map[table.object_id] = {};
  }

  for (const auto &item : tableMap) {
    const auto &table = static_cast<const BfRtTableObj &>(*item.second);
    for (const auto &kv : table.table_ref_map) {
      if (kv.first != "action_data_table_refs" &&
          kv.first != "selection_table_refs") {
        continue;
      }
      const auto &ref_list = kv.second;
      for (const auto &ref : ref_list) {
        // Only expose the tables which indirectly depend on this table
        if (ref.indirect_ref == false) {
          continue;
        }
        tables_dependent_on_this_table_map[ref.id].push_back(table.object_id);
      }
    }
  }
}

bf_status_t BfRtInfoImpl::bfrtInfoTablesDependentOnThisTableGet(
    const bf_rt_id_t &tbl_id, std::vector<bf_rt_id_t> *table_vec_ret) const {
  if (table_vec_ret == nullptr) {
    LOG_ERROR("%s:%d nullptr arg passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  if (tables_dependent_on_this_table_map.find(tbl_id) ==
      tables_dependent_on_this_table_map.end()) {
    LOG_ERROR("%s:%d Info requested for the given table with id %d not found",
              __func__,
              __LINE__,
              tbl_id);
    return BF_OBJECT_NOT_FOUND;
  }
  *table_vec_ret = tables_dependent_on_this_table_map.at(tbl_id);
  return BF_SUCCESS;
}

bf_status_t BfRtInfoImpl::bfrtInfoTablesThisTableDependsOnGet(
    const bf_rt_id_t &tbl_id, std::vector<bf_rt_id_t> *table_vec_ret) const {
  if (table_vec_ret == nullptr) {
    LOG_ERROR("%s:%d nullptr arg passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  const BfRtTable *table;
  auto sts = bfrtTableFromIdGet(tbl_id, &table);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d Unable to get table list that the table with id %d depends on",
        __func__,
        __LINE__,
        tbl_id);
    return sts;
  }
  const auto *table_obj = static_cast<const BfRtTableObj *>(table);
  for (const auto &kv : table_obj->table_ref_map) {
    if (kv.first != "action_data_table_refs" &&
        kv.first != "selection_table_refs" && kv.first != "other") {
      continue;
    }
    const auto &ref_list = kv.second;
    for (const auto &ref : ref_list) {
      // Only expose the tables which this table indirectly depends on
      if (ref.indirect_ref == false) {
        continue;
      }
      table_vec_ret->push_back(ref.id);
    }
  }
  return BF_SUCCESS;
}

bf_status_t BfRtInfoImpl::bfRtInfoPipelineInfoGet(
    PipelineProfInfoVec *pipe_info) const {
  if (pipe_info == nullptr) {
    LOG_ERROR("%s:%d nullptr arg passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  for (const auto &pipeline : program_config_.p4_pipelines_) {
    (*pipe_info)
        .emplace_back(std::cref(pipeline.name_),
                      std::cref(pipeline.pipe_scope_));
  }
  return BF_SUCCESS;
}
bf_status_t BfRtInfoImpl::contextFilePathGet(
    std::vector<std::reference_wrapper<const std::string>> *file_name_vec)
    const {
  if (file_name_vec == nullptr) {
    LOG_ERROR("%s:%d nullptr arg passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  for (const auto &pipeline : program_config_.p4_pipelines_) {
    (*file_name_vec).push_back(std::cref(pipeline.context_json_path_));
  }
  return BF_SUCCESS;
}

bf_status_t BfRtInfoImpl::binaryFilePathGet(
    std::vector<std::reference_wrapper<const std::string>> *file_name_vec)
    const {
  if (file_name_vec == nullptr) {
    LOG_ERROR("%s:%d nullptr arg passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  for (const auto &pipeline : program_config_.p4_pipelines_) {
    (*file_name_vec).push_back(std::cref(pipeline.binary_path_));
  }
  return BF_SUCCESS;
}

bf_status_t BfRtInfoImpl::bfrtInfoGetTables(
    std::vector<const BfRtTable *> *table_vec_ret) const {
  if (table_vec_ret == nullptr) {
    LOG_ERROR("%s:%d nullptr arg passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  for (auto const &item : tableMap) {
    table_vec_ret->push_back(item.second.get());
  }
  return BF_SUCCESS;
}

bf_status_t BfRtInfoImpl::bfrtInfoGetTables(
    std::vector<BfRtTable *> *table_vec_ret) const {
  if (table_vec_ret == nullptr) {
    LOG_ERROR("%s:%d nullptr arg passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  for (auto const &item : tableMap) {
    table_vec_ret->push_back(item.second.get());
  }
  return BF_SUCCESS;
}

bf_status_t BfRtInfoImpl::bfrtTableFromNameGet(
    const std::string &name, const BfRtTable **table_ret) const {
  if (invalid_table_names.find(name) != invalid_table_names.end()) {
    LOG_ERROR("%s:%d Table \"%s\" was optimized out by compiler",
              __func__,
              __LINE__,
              name.c_str());
    return BF_INVALID_ARG;
  }
  auto it = this->fullTableMap.find(name);
  if (it == this->fullTableMap.end()) {
    LOG_ERROR("%s:%d Table \"%s\" not found", __func__, __LINE__, name.c_str());
    return BF_OBJECT_NOT_FOUND;
  } else {
    *table_ret = it->second;
    return BF_SUCCESS;
  }
}

bf_status_t BfRtInfoImpl::bfrtTableFromIdGet(
    bf_rt_id_t id, const BfRtTable **table_ret) const {
  auto it = idToNameMap.find(id);
  if (it == idToNameMap.end()) {
    LOG_ERROR("%s:%d Table_id \"%d\" not found", __func__, __LINE__, id);
    return BF_OBJECT_NOT_FOUND;
  } else {
    return bfrtTableFromNameGet(it->second, table_ret);
  }
}

bf_status_t BfRtInfoImpl::bfRtInfoFilePathGet(
    std::vector<std::reference_wrapper<const std::string>> *file_name_vec)
    const {
  if (file_name_vec == nullptr) {
    LOG_ERROR("%s:%d nullptr arg passed", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  for (const auto &bfrt_info_file_path :
       program_config_.bfrtInfoFilePathVect_) {
    (*file_name_vec).push_back(std::cref(bfrt_info_file_path));
  }
  return BF_SUCCESS;
}

// This function returns if a key field is a field slice or not
bool BfRtInfoImpl::isFieldSlice(
    const Cjson &key_field,
    const size_t &start_bit,
    const size_t &key_width_byte_size,
    const size_t &key_width_parent_byte_size) const {
  Cjson key_annotations = key_field["annotations"];
  for (const auto &annotation : key_annotations.getCjsonChildVec()) {
    std::string annotation_name = (*annotation)["name"];
    std::string annotation_value = (*annotation)["value"];
    if ((annotation_name == "isFieldSlice") && (annotation_value == "true")) {
      return true;
    }
  }

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
  // field slice. As a result, the front end compiler (bf-rt generator)
  // doesn't know that the key field is actually a field slice and doesn't
  // publish the "is_field_slice" annotation in bf-rt json. However, the
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
size_t BfRtInfoImpl::getStartBit(const Cjson *context_json_key_field,
                                 const Cjson *bfrt_json_key_field,
                                 const BfRtTableObj &bfrtTable) const {
  const std::string key_name = (*bfrt_json_key_field)["name"];
  Cjson key_annotations = (*bfrt_json_key_field)["annotations"];
  for (const auto &annotation : key_annotations.getCjsonChildVec()) {
    std::string annotation_name = (*annotation)["name"];
    std::string annotation_value = (*annotation)["value"];
    if ((annotation_name == "isFieldSlice") && (annotation_value == "true")) {
      size_t offset = key_name.find(":");
      if (offset == std::string::npos) {
        LOG_ERROR(
            "%s:%d ERROR %s Field is a field slice but the name does not "
            "contain a ':'",
            __func__,
            __LINE__,
            key_name.c_str());
        BF_RT_DBGCHK(0);
        return 0;
      }
      return std::stoi(key_name.substr(offset + 1), nullptr, 10);
    }
  }
  bool is_context_json_node = false;
  if (bfrtTable.object_type == BfRtTableObj::TableType::MATCH_DIRECT ||
      bfrtTable.object_type == BfRtTableObj::TableType::MATCH_INDIRECT ||
      bfrtTable.object_type ==
          BfRtTableObj::TableType::MATCH_INDIRECT_SELECTOR ||
      bfrtTable.object_type == BfRtTableObj::TableType::PORT_METADATA) {
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
    // BF_RT_DBGCHK(0);
    return 0;
  }

  // This indicates that the key field is not a field slice and it does not
  // even have a context json node associated with it (this is true for fixed
  // tables, resourse tables, etc.) Hence just return 0
  return 0;
}

// This function is only applicable when the key field is a field slice
// This function strips out '[]' if present in the name of the key_field.
// This will be true when the key is a field slice. The compiler will
// publish the name of the key with '[]' when the user does not use '@name'
// p4 annotation for the field slice. If the p4 annotation is uses, then
// the new name will be published in bfrt json and this field that will be
// treated as an independent field and not as a field slice.
// Refer P4C-1293 for more details on how the compiler will publish
// different fields
std::string BfRtInfoImpl::getParentKeyName(const Cjson &key_field) const {
  const std::string key_name = key_field["name"];
  Cjson key_annotations = key_field["annotations"];
  for (const auto &annotation : key_annotations.getCjsonChildVec()) {
    std::string annotation_name = (*annotation)["name"];
    std::string annotation_value = (*annotation)["value"];
    if ((annotation_name == "isFieldSlice") && (annotation_value == "true")) {
      size_t offset = key_name.find_last_of("[");
      if (offset == std::string::npos) {
        LOG_ERROR(
            "%s:%d ERROR %s Field is a field slice but the name does not "
            "contain a '['",
            __func__,
            __LINE__,
            key_name.c_str());
        BF_RT_DBGCHK(0);
        return key_name;
      }
      return std::string(key_name.begin(), key_name.begin() + offset);
    }
  }
  return key_name;
}

// This function parses the table_key json node and extracts all the relevant
// information into the helper maps. It extracts the following things
// - Gets the position (the order in which they appear in p4) of every match
// field
// - Gets the full size of every match field in bytes
// - Gets the offset (in the match spec byte buf) of every match field
void BfRtInfoImpl::parseKeyHelper(
    const Cjson *context_json_table_keys,
    const Cjson *bfrt_json_table_keys,
    const BfRtTableObj &bfrtTable,
    std::map<std::string, size_t> *match_key_field_name_to_position_map,
    std::map<size_t, size_t> *match_key_field_position_to_offset_map,
    size_t *total_offset_bytes,
    size_t *num_valid_match_bits,
    std::map<std::string, size_t> *
        match_key_field_name_to_parent_field_byte_size_map) {
  bool is_context_json_node = false;
  std::map<std::string, size_t> match_key_field_name_to_bit_size_map;
  if (bfrtTable.object_type == BfRtTableObj::TableType::MATCH_DIRECT ||
      bfrtTable.object_type == BfRtTableObj::TableType::MATCH_INDIRECT ||
      bfrtTable.object_type ==
          BfRtTableObj::TableType::MATCH_INDIRECT_SELECTOR ||
      bfrtTable.object_type == BfRtTableObj::TableType::PORT_METADATA) {
    // For these tables we have context json key nodes. Hence use the context
    // json key node to fill the helper maps.
    is_context_json_node = true;
  } else {
    // For all the other tables, we don't have a context json key node. Hence
    // use the key node in bfrt json to extract all the information
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
    // This means use the bfrt json key node
    size_t pos = 0;
    // Since we don't support field slices for fixed objects, we use the bfrt
    // key json node to extract all the information. The position of the
    // fields will just be according to the order in which they are published
    // Hence simply increment the position as we iterate
    for (const auto &key : bfrt_json_table_keys->getCjsonChildVec()) {
      std::string key_name = (*key)["name"];
      std::string key_type_str;
      size_t width;
      DataType type;
      uint64_t default_value;
      float default_fl_value;
      std::string default_str_value;
      std::vector<std::string> choices;
      parseType(*key,
                key_type_str,
                width,
                &type,
                &default_value,
                &default_fl_value,
                &default_str_value,
                &choices);
      (*match_key_field_name_to_position_map)[key_name] = pos;
      (*match_key_field_position_to_offset_map)[pos] = (width + 7) / 8;
      (*match_key_field_name_to_parent_field_byte_size_map)[key_name] =
          (width + 7) / 8;
      match_key_field_name_to_bit_size_map[key_name] = width;
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

std::unique_ptr<BfRtTableObj> BfRtInfoImpl::parseSnapshotTable(
    const std::string &prog_name, Cjson &table_bfrt) {
  std::string table_name = table_bfrt["name"];
  std::string table_type = table_bfrt["table_type"];
  bf_rt_id_t table_id = table_bfrt["id"];
  size_t table_size = static_cast<unsigned int>(table_bfrt["size"]);

  LOG_DBG("Snapshot Table : %s :: Type :: %s",
          table_name.c_str(),
          table_type.c_str());

  // Create BfRtTableObj object
  std::unique_ptr<BfRtTableObj> bfrtTable = BfRtTableObj::make_table(
      prog_name, getTableType(table_type), table_id, table_name, table_size, 0);

  if (bfrtTable == nullptr) {
    LOG_ERROR(
        "%s:%d ERROR : Failed to create bfrtTable object", __func__, __LINE__);
    return nullptr;
  }

  // getting key
  Cjson table_key_cjson = table_bfrt["key"];
  std::map<std::string /*match_key_field_name*/, size_t /* position*/>
      match_key_field_name_to_position_map;
  std::map<std::string /*match_key_field_name*/,
           size_t /* size of parent field in bytes */>
      match_key_field_name_to_parent_field_byte_size_map;
  std::map<size_t /* position */, size_t /* offset */>
      match_key_field_position_to_offset_map;

  size_t cumulative_offset = 0;
  size_t cumulative_key_width_bits = 0;

  // Extract all the relevant information into the helper maps
  parseKeyHelper(nullptr /* context json node */,
                 &table_key_cjson,
                 *bfrtTable,
                 &match_key_field_name_to_position_map,
                 &match_key_field_position_to_offset_map,
                 &cumulative_offset,
                 &cumulative_key_width_bits,
                 &match_key_field_name_to_parent_field_byte_size_map);

  for (const auto &key : table_key_cjson.getCjsonChildVec()) {
    const std::string key_name = getParentKeyName(*key);
    const size_t start_bit =
        getStartBit(nullptr /* context json node */, key.get(), *bfrtTable);
    size_t field_offset = 0, parent_field_byte_size = 0;
    // Get the field offset and the size of the field in bytes
    if (keyByteSizeAndOffsetGet(
            table_name,
            key_name,
            match_key_field_name_to_position_map,
            match_key_field_name_to_parent_field_byte_size_map,
            match_key_field_position_to_offset_map,
            &field_offset,
            &parent_field_byte_size) != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s ERROR in processing key field %s while trying to get field "
          "size and offset",
          __func__,
          __LINE__,
          table_name.c_str(),
          key_name.c_str());
      BF_RT_DBGCHK(0);
      return nullptr;
    }
    // Finally form the key field object
    std::unique_ptr<BfRtTableKeyField> key_field = parseKey(
        *key, table_id, field_offset, start_bit, parent_field_byte_size, false);
    if (key_field == nullptr) {
      continue;
    }
    bf_rt_id_t key_field_id = key_field->getId();
    if (bfrtTable->key_fields.find(key_field_id) !=
        bfrtTable->key_fields.end()) {
      // Field id is repeating, log an error message
      LOG_ERROR("%s:%d Field ID %d is repeating for table %s",
                __func__,
                __LINE__,
                key_field->getId(),
                table_name.c_str());
      return nullptr;
    } else {
      bfrtTable->key_fields[key_field_id] = (std::move(key_field));
    }
  }

  // Set the table key size in terms of bytes. The field_offset will be the
  // total size of the key
  bfrtTable->key_size.bytes = cumulative_offset;

  // Set the table key in terms of exact bit widths.
  bfrtTable->key_size.bits = cumulative_key_width_bits;

  // getting data
  Cjson table_data_cjson = table_bfrt["data"];
  Cjson temp;
  size_t offset = 0;
  size_t bitsize = 0;
  for (const auto &table_data : table_data_cjson.getCjsonChildVec()) {
    bool is_register_data_field = false;
    std::unique_ptr<BfRtTableDataField> data_field =
        parseData(*table_data,
                  temp,
                  bfrtTable.get(),
                  offset,
                  bitsize,
                  0,
                  0,
                  0,
                  &is_register_data_field);
    if (data_field == nullptr) {
      continue;
    }
    bf_rt_id_t data_field_id = data_field->getId();
    if (bfrtTable->common_data_fields.find(data_field_id) !=
        bfrtTable->common_data_fields.end()) {
      LOG_ERROR("%s:%d Id \"%u\" Exists for common data of table %s",
                __func__,
                __LINE__,
                data_field_id,
                bfrtTable->table_name_get().c_str());
      // Field-id is repeated
      return nullptr;
    }
    bfrtTable->common_data_fields_names[data_field->getName()] =
        data_field.get();
    bfrtTable->common_data_fields[data_field_id] = std::move(data_field);
  }

  return bfrtTable;
}

// The container field has a list of fields in it. Containers can also
// contain more containers. Parse each container field and store in
// the container map within the object.
// This function parses a repeated-container
bf_status_t BfRtInfoImpl::parseContainer(Cjson table_data_cjson,
                                         const BfRtTableObj *bfrtTable,
                                         BfRtTableDataField *data_field_obj) {
  std::set<DataFieldType> resource_set;

  // Go over the child nodes in this container
  for (const auto &table_data : table_data_cjson.getCjsonChildVec()) {
    std::unique_ptr<BfRtTableDataField> container_field =
        parseContainerData(*table_data, bfrtTable);
    if (container_field == nullptr) {
      LOG_ERROR("%s:%d ERROR : Failed to parse a container field for table %s",
                __func__,
                __LINE__,
                bfrtTable->table_name_get().c_str());
      continue;
    }
    auto bf_status =
        data_field_obj->dataFieldInsertContainer(std::move(container_field));
    if (bf_status != BF_SUCCESS) {
      return bf_status;
    }
  }
  return BF_SUCCESS;
}

// This functions parses a container field
std::unique_ptr<BfRtTableDataField> BfRtInfoImpl::parseContainerData(
    Cjson data, const BfRtTableObj *bfrtTable) {
  // first get mandatory and read_only
  auto mandatory = bool(data["mandatory"]);
  auto read_only = bool(data["read_only"]);
  // if "singleton" field exists, then
  // lets point to it for further parsing
  if (data["singleton"].exists()) {
    data = data["singleton"];
  }
  // get "type"
  std::string key_type_str;
  size_t width;
  DataType type;
  uint64_t default_value;
  float default_fl_value;
  std::string default_str_value;
  std::vector<std::string> choices;
  parseType(data,
            key_type_str,
            width,
            &type,
            &default_value,
            &default_fl_value,
            &default_str_value,
            &choices);
  bool repeated = data["repeated"];

  std::string data_name = static_cast<std::string>(data["name"]);
  std::set<DataFieldType> resource_set;
  BfRtTable::TableType this_table_type;
  bfrtTable->tableTypeGet(&this_table_type);
  // Legacy internal names, if the name starts with '$', then infer its type
  if (data_name[0] == '$') {
    auto data_field_type =
        getDataFieldTypeFrmName(data_name.substr(1), bfrtTable->object_type);
    resource_set.insert(data_field_type);
  }

  bf_rt_id_t data_id = static_cast<bf_rt_id_t>(data["id"]);
  // This container field could in turn contain a container object.
  // Container
  // objects have container_valid set to true
  bool container_valid = false;
  if (data["container"].exists()) {
    container_valid = true;
  }
  std::set<Annotation> annotations = parseAnnotations(data["annotations"]);
  // Parse the field
  std::unique_ptr<BfRtTableDataField> data_field(
      new BfRtTableDataField(bfrtTable->table_id_get(),
                             data_id,
                             data_name,
                             0,  // action_id
                             width,
                             type,
                             std::move(choices),
                             default_value,    // default_value
                             0.0,              // default_fl_value
                             std::string(""),  // default_str_value
                             0,                // offset
                             repeated,
                             resource_set,
                             mandatory,
                             read_only,
                             container_valid,
                             annotations,
                             {}));

  if (data["container"].exists()) {
    Cjson c_table_data = data["container"];
    parseContainer(c_table_data, bfrtTable, data_field.get());
  }

  // Return the parsed data field
  return data_field;
}

}  // namespace bfrt

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
#include <bf_rt_common/bf_rt_init_impl.hpp>
#include <bf_rt_common/bf_rt_table_attributes_impl.hpp>
#include <bf_rt_port/bf_rt_port_table_attributes_state.hpp>

#include "bf_rt_port_table_impl.hpp"
#include <bf_rt_port/dpdk/bf_rt_port_table_data_impl.hpp>
#include <bf_rt_port/bf_rt_port_table_key_impl.hpp>
#include <bf_rt_port/bf_rt_port_mgr_intf.hpp>

namespace bfrt {
// Port Configuration Table
enum PortCfgDataFieldId {
  PORT_NAME_ID = 1,
  PORT_TYPE_ID = 2,
  PORT_DIR_ID = 3,
  MTU_ID = 4,
  PIPE_IN_ID = 5,
  PIPE_OUT_ID = 6,
  PORT_IN_ID = 7,
  PORT_OUT_ID = 8,
  MEMPOOL_ID = 9,
  PCIE_BDF_ID = 10,
  FILE_NAME_ID = 11,
  DEV_ARGS_ID = 12,
  DEV_HOTPLUG_ENABLED_ID = 13,
  SIZE_ID = 14,
};

void BfRtPortCfgTable::mapInit() {
  portDirMap["PM_PORT_DIR_DEFAULT"] = PM_PORT_DIR_DEFAULT;
  portDirMap["PM_PORT_DIR_TX_ONLY"] = PM_PORT_DIR_TX_ONLY;
  portDirMap["PM_PORT_DIR_RX_ONLY"] = PM_PORT_DIR_RX_ONLY;

  portTypeMap["BF_DPDK_TAP"] = BF_DPDK_TAP;
  portTypeMap["BF_DPDK_LINK"] = BF_DPDK_LINK;
  portTypeMap["BF_DPDK_SOURCE"] = BF_DPDK_SOURCE;
  portTypeMap["BF_DPDK_SINK"] = BF_DPDK_SINK;
  portTypeMap["BF_DPDK_RING"] = BF_DPDK_RING;
}

bf_status_t BfRtPortCfgTable::tableEntryAdd(const BfRtSession & /*session*/,
                                            const bf_rt_target_t &dev_tgt,
                                            const BfRtTableKey &key,
                                            const BfRtTableData &data) const {
  const BfRtPortCfgTableKey &port_key =
      static_cast<const BfRtPortCfgTableKey &>(key);
  const uint32_t dev_port = port_key.getId();
  const BfRtPortCfgTableData &port_data =
      static_cast<const BfRtPortCfgTableData &>(data);
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  const std::unordered_map<bf_rt_id_t, uint32_t> &u32Data =
      port_data.getU32FieldDataMap();
  const std::unordered_map<bf_rt_id_t, std::string> &strData =
      port_data.getStrFieldDataMap();

  struct port_attributes_t port_attrib;

  memset(&port_attrib, 0, sizeof(port_attrib));

  if ((strData.empty()) ||
      (u32Data.empty()) ||
      (strData.find(PORT_NAME_ID) == strData.end()) ||
      (strData.find(PORT_TYPE_ID) == strData.end()) ||
      (strData.find(PORT_DIR_ID) == strData.end())) {
       LOG_ERROR(
          "%s:%d %s ERROR : Port Cfg table entry add requires port name, port type"
          " and port dir",
          __func__,
          __LINE__,
          table_name_get().c_str());
       return BF_INVALID_ARG;
  }

  if (portTypeMap.find(strData.at(PORT_TYPE_ID)) == portTypeMap.end()) {
      LOG_ERROR("%s:%d %s ERROR : invalid port type %s",
                   __func__,
                   __LINE__,
                   table_name_get().c_str(),
                   strData.at(PORT_TYPE_ID).c_str());
      return BF_INVALID_ARG;
  }

  if (portDirMap.find(strData.at(PORT_DIR_ID)) == portDirMap.end()) {
      LOG_ERROR("%s:%d %s ERROR : invalid port direction %s",
                   __func__,
                   __LINE__,
                   table_name_get().c_str(),
                   strData.at(PORT_DIR_ID).c_str());
      return BF_INVALID_ARG;
  }

  port_attrib.port_type = portTypeMap.at(strData.at(PORT_TYPE_ID));
  port_attrib.port_dir = portDirMap.at(strData.at(PORT_DIR_ID));
  strncpy(port_attrib.port_name, strData.at(PORT_NAME_ID).c_str(),
          PORT_NAME_LEN - 1);
  port_attrib.port_name[PORT_NAME_LEN - 1] = '\0';

  if (strData.find(MEMPOOL_ID) != strData.end()) {
      strncpy(port_attrib.mempool_name, strData.at(MEMPOOL_ID).c_str(),
              MEMPOOL_NAME_LEN - 1);
      port_attrib.mempool_name[MEMPOOL_NAME_LEN - 1] = '\0';
  } else {
      if (portTypeMap.at(strData.at(PORT_TYPE_ID)) != BF_DPDK_SINK) {
          LOG_ERROR("%s:%d %s ERROR : Mempool name is needed",
                    __func__,
                    __LINE__,
                    table_name_get().c_str());
          return BF_INVALID_ARG;
      }
  }

  if ((portDirMap.at(strData.at(PORT_DIR_ID)) == PM_PORT_DIR_DEFAULT) ||
      (portDirMap.at(strData.at(PORT_DIR_ID)) == PM_PORT_DIR_RX_ONLY)) {
      if (u32Data.find(PORT_IN_ID) != u32Data.end()) {
          port_attrib.port_in_id =  u32Data.at(PORT_IN_ID);
      } else {
          LOG_ERROR("%s:%d %s ERROR : Port In ID needed",
                     __func__,
                     __LINE__,
                     table_name_get().c_str());
          return BF_INVALID_ARG;
      }

      if (strData.find(PIPE_IN_ID) != strData.end()) {
          strncpy(port_attrib.pipe_in, strData.at(PIPE_IN_ID).c_str(),
                  PIPE_NAME_LEN - 1);
          port_attrib.pipe_in[PIPE_NAME_LEN - 1] = '\0';

      } else {
          LOG_ERROR("%s:%d %s ERROR : Pipe In needed",
                     __func__,
                     __LINE__,
                     table_name_get().c_str());
          return BF_INVALID_ARG;
      }
  }

  if ((portDirMap.at(strData.at(PORT_DIR_ID)) == PM_PORT_DIR_DEFAULT) ||
      (portDirMap.at(strData.at(PORT_DIR_ID)) == PM_PORT_DIR_TX_ONLY)) {
      if (u32Data.find(PORT_OUT_ID) != u32Data.end()) {
          port_attrib.port_out_id =  u32Data.at(PORT_OUT_ID);
      } else {
          LOG_ERROR("%s:%d %s ERROR :  Port Out ID needed",
                     __func__,
                     __LINE__,
                     table_name_get().c_str());
          return BF_INVALID_ARG;
      }

      if (strData.find(PIPE_OUT_ID) != strData.end()) {
          strncpy(port_attrib.pipe_out, strData.at(PIPE_OUT_ID).c_str(),
                  PIPE_NAME_LEN - 1);
          port_attrib.pipe_out[PIPE_NAME_LEN - 1] = '\0';

      } else {
          LOG_ERROR("%s:%d %s ERROR : Pipe Out needed",
                     __func__,
                     __LINE__,
                     table_name_get().c_str());
          return BF_INVALID_ARG;
      }
  }

  switch (portTypeMap.at(strData.at(PORT_TYPE_ID))) {
    case BF_DPDK_TAP:
      if (u32Data.find(MTU_ID) != u32Data.end()) {
          port_attrib.tap.mtu =  u32Data.at(MTU_ID);
      } else {
          LOG_ERROR("%s:%d %s ERROR : TAP Port needs mtu",
                     __func__,
                     __LINE__,
                     table_name_get().c_str());
          return BF_INVALID_ARG;
      }
      break;
    case BF_DPDK_LINK:
      if (strData.find(PCIE_BDF_ID) != strData.end()) {
          strncpy(port_attrib.link.pcie_domain_bdf,
                  strData.at(PCIE_BDF_ID).c_str(),
                  PCIE_BDF_LEN - 1);
	  port_attrib.link.pcie_domain_bdf[PCIE_BDF_LEN - 1] = '\0';
      } else {
          LOG_ERROR("%s:%d %s ERROR : Link Port needs PCIE BDF",
                     __func__,
                     __LINE__,
                     table_name_get().c_str());
          return BF_INVALID_ARG;
      }
      if (strData.find(DEV_ARGS_ID) != strData.end()) {
          strncpy(port_attrib.link.dev_args,
                  strData.at(DEV_ARGS_ID).c_str(),
                  DEV_ARGS_LEN- 1);
	  port_attrib.link.dev_args[DEV_ARGS_LEN- 1] = '\0';
      }
      if (u32Data.find(DEV_HOTPLUG_ENABLED_ID) != u32Data.end()) {
          port_attrib.link.dev_hotplug_enabled =
                            u32Data.at(DEV_HOTPLUG_ENABLED_ID);
      }
      break;
    case BF_DPDK_SOURCE:
      if (strData.find(FILE_NAME_ID) != strData.end()) {
          strncpy(port_attrib.source.file_name,
                  strData.at(FILE_NAME_ID).c_str(),
                  PCAP_FILE_NAME_LEN - 1);
	  port_attrib.source.file_name[PCAP_FILE_NAME_LEN - 1] = '\0';
      } else {
          LOG_ERROR("%s:%d %s ERROR : Source Port needs file name",
                     __func__,
                     __LINE__,
                     table_name_get().c_str());
          return BF_INVALID_ARG;
      }
      break;
    case BF_DPDK_SINK:
      if (strData.find(FILE_NAME_ID) != strData.end()) {
          strncpy(port_attrib.sink.file_name,
                  strData.at(FILE_NAME_ID).c_str(),
                  PCAP_FILE_NAME_LEN - 1);
	  port_attrib.sink.file_name[PCAP_FILE_NAME_LEN - 1] = '\0';
      } else {
          LOG_ERROR("%s:%d %s ERROR : Sink Port needs file name",
                     __func__,
                     __LINE__,
                     table_name_get().c_str());
          return BF_INVALID_ARG;
      }
      break;
    case BF_DPDK_RING:
      if (u32Data.find(SIZE_ID) != u32Data.end()) {
          port_attrib.ring.size =  u32Data.at(SIZE_ID);
      } else {
          LOG_ERROR("%s:%d %s ERROR : Ring Port needs size",
                     __func__,
                     __LINE__,
                     table_name_get().c_str());
          return BF_INVALID_ARG;
      }
      break;
    default:
      LOG_ERROR("%s:%d ERROR : Incorrect Port Type",
                 __func__,
                 __LINE__);
      return BF_INVALID_ARG;
  }

  status = portMgr->portMgrPortAdd(dev_tgt.dev_id,
                                   dev_port,
                                   &port_attrib);

  if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in adding an port",
              __func__,
              __LINE__,
              table_name_get().c_str());
  }
  return status;
}

bf_status_t BfRtPortCfgTable::tableEntryDel(const BfRtSession & /*session*/,
                                            const bf_rt_target_t &dev_tgt,
                                            const BfRtTableKey &key) const {
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_ERROR("%s:%d %s Error : Failed to reset key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortCfgTableKey *>(key))->reset();
}

bf_status_t BfRtPortCfgTable::dataReset(BfRtTableData *data) const {
  std::vector<bf_rt_id_t> emptyFields;
  return this->dataReset(emptyFields, data);
}

bf_status_t BfRtPortCfgTable::dataReset(const std::vector<bf_rt_id_t> &fields,
                                        BfRtTableData *data) const {
  if (data == nullptr) {
    LOG_ERROR("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortCfgTableData *>(data))->reset(fields);
}

bf_status_t BfRtPortCfgTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPortCfgTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPortCfgTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPortCfgTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_ERROR("%s:%d Attribute %d is not supported",
              __func__,
              __LINE__,
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }
  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}

bf_status_t BfRtPortCfgTable::attributeReset(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  auto &tbl_attr_impl = static_cast<BfRtTableAttributesImpl &>(*(attr->get()));
  switch (type) {
    case TableAttributesType::PORT_STATUS_NOTIF:
      break;
    default:
      LOG_ERROR("%s:%d Trying to set Invalid Attribute Type %d",
                __func__,
                __LINE__,
                static_cast<int>(type));
      return BF_INVALID_ARG;
  }
  return tbl_attr_impl.resetAttributeType(type);
}

bf_status_t BfRtPortCfgTable::tableAttributesSet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const BfRtTableAttributes &tableAttributes) const {
  auto tbl_attr_impl =
      static_cast<const BfRtTableAttributesImpl *>(&tableAttributes);
  const auto attr_type = tbl_attr_impl->getAttributeType();
  std::set<TableAttributesType> attribute_type_set;
  auto bf_status = tableAttributesSupported(&attribute_type_set);
  if (bf_status != BF_SUCCESS ||
      (attribute_type_set.find(attr_type) == attribute_type_set.end())) {
    LOG_ERROR("%s:%d Attribute %d is not supported",
              __func__,
              __LINE__,
              static_cast<int>(attr_type));
    return BF_NOT_SUPPORTED;
  }
  switch (attr_type) {
    default:
      LOG_ERROR(
          "%s:%d Invalid Attribute type (%d) encountered while trying to set "
          "attributes",
          __func__,
          __LINE__,
          static_cast<int>(attr_type));
      return BF_NOT_SUPPORTED;
  }
  return BF_SUCCESS;
}

// Port Stat Table
bf_status_t BfRtPortStatTable::tableEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const BfRtTableKey &key,
    const BfRtTable::BfRtTableGetFlag &flag,
    BfRtTableData *data) const {
  const BfRtPortStatTableKey &port_key =
      static_cast<const BfRtPortStatTableKey &>(key);
  uint32_t dev_port = port_key.getId();
  BfRtPortStatTableData *port_data = static_cast<BfRtPortStatTableData *>(data);
  return this->tableEntryGet_internal(dev_tgt, dev_port, flag, port_data);
}

bf_status_t BfRtPortStatTable::tableEntryGet_internal(
    const bf_rt_target_t &dev_tgt,
    const uint32_t &dev_port,
    const BfRtTable::BfRtTableGetFlag &flag,
    BfRtPortStatTableData *port_data) const {
  const std::vector<bf_rt_id_t> activeDataFields =
      port_data->getActiveDataFields();
  auto *portMgr = PortMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;

  // all fields
  uint64_t stats[BF_PORT_NUM_COUNTERS];

  memset(stats, 0, BF_PORT_NUM_COUNTERS * sizeof(uint64_t));

  status = portMgr->portMgrPortAllStatsGet(dev_tgt.dev_id, dev_port, stats);
  if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s: Error getting all stat for dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);

      return status;
  }

  port_data->setAllValues(stats);
  return status;
}

bf_status_t BfRtPortStatTable::dataReset(const std::vector<bf_rt_id_t> &fields,
                                         BfRtTableData *data) const {
  if (data == nullptr) {
    LOG_ERROR("%s:%d %s Error : Failed to reset data",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return (static_cast<BfRtPortStatTableData *>(data))->reset(fields);
}

bf_status_t BfRtPortStatTable::dataReset(BfRtTableData *data) const {
  std::vector<bf_rt_id_t> emptyFields;
  return (static_cast<BfRtPortStatTableData *>(data))->reset(emptyFields);
}

bf_status_t BfRtPortStatTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtPortStatTableKey(this));
  if (*key_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTable::dataAllocate(
    std::unique_ptr<BfRtTableData> *data_ret) const {
  std::vector<bf_rt_id_t> fields;
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPortStatTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTable::dataAllocate(
    const std::vector<bf_rt_id_t> &fields,
    std::unique_ptr<BfRtTableData> *data_ret) const {
  *data_ret =
      std::unique_ptr<BfRtTableData>(new BfRtPortStatTableData(this, fields));
  if (*data_ret == nullptr) {
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtPortStatTable::attributeAllocate(
    const TableAttributesType &type,
    std::unique_ptr<BfRtTableAttributes> *attr) const {
  std::set<TableAttributesType> attribute_type_set;
  auto status = tableAttributesSupported(&attribute_type_set);
  if (status != BF_SUCCESS ||
      (attribute_type_set.find(type) == attribute_type_set.end())) {
    LOG_ERROR("%s:%d Attribute %d is not supported",
              __func__,
              __LINE__,
              static_cast<int>(type));
    return BF_NOT_SUPPORTED;
  }

  *attr = std::unique_ptr<BfRtTableAttributes>(
      new BfRtTableAttributesImpl(this, type));
  return BF_SUCCESS;
}
}  // bfrt

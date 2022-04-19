/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) 2022-present Barefoot Networks, Inc.

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

// tdi includes
#include <tdi/common/tdi_utils.hpp>


#include "tdi_session_impl.hpp"
#include "tdi_rt_info.hpp"
#include "tdi_rt_target.hpp"
#include "tdi_context_info.hpp"
#include "tdi_pipe_mgr_intf.hpp"

namespace tdi {
namespace pna {
namespace rt {

tdi_status_t Target::setValue(const tdi_target_e &target_field,
                              const uint32_t &value) {
    tdi_status_t status = TDI_SUCCESS;
    switch(static_cast<pna_target_e>(target_field)) {
      case PNA_TARGET_PIPE_ID:
      	this->pipe_id_ = value;
      break;
      case PNA_TARGET_DEV_ID:
      	this->dev_id_ = value;
      break;
      case PNA_TARGET_DIRECTION:
        this->direction_ = static_cast<pna_direction_e>(value);
      break;
      default:
        status = TDI_INVALID_ARG;
      break;
    }
    return status;
}

tdi_status_t Target::getValue(const tdi_target_e &target_field,
                              uint32_t *value) const {
    tdi_status_t status = TDI_SUCCESS;
    switch(static_cast<pna_target_e>(target_field)) {
      case PNA_TARGET_PIPE_ID:
      	*value = this->pipe_id_;
      break;
      case PNA_TARGET_DEV_ID:
      	*value = this->dev_id_;
      break;
      case PNA_TARGET_DIRECTION:
        *value = this->direction_;
      break;
      default:
        status = TDI_INVALID_ARG;
      break;
    }
    return status;
}

void Target::getTargetVals(bf_dev_target_t *dev_tgt,
                           pna_direction_e *direction) const {
  if (dev_tgt) {
    dev_tgt->device_id = this->dev_id_;
    dev_tgt->dev_pipe_id = this->pipe_id_;
  }
  if (direction)
    *direction = static_cast<pna_direction_e>(this->direction_);
}

Device::Device(const tdi_dev_id_t &device_id,
               const tdi_arch_type_e &arch_type,
               const std::vector<tdi::ProgramConfig> &device_config,
               const std::vector<tdi_mgr_type_e> mgr_type_list,
               void *cookie)
    : tdi::pna::Device(
          device_id, arch_type, device_config, mgr_type_list, cookie) {
  // Parse tdi json for every program
  for (const auto &program_config : device_config) {
    LOG_ERROR("%s:%d parsing %s",
              __func__,
              __LINE__,
              program_config.prog_name_.c_str());
    auto tdi_info_mapper = std::unique_ptr<tdi::TdiInfoMapper>(
        new tdi::pna::rt::TdiInfoMapper());
    auto table_factory = std::unique_ptr<tdi::TableFactory>(
        new tdi::pna::rt::TableFactory());

    auto tdi_info_parser = std::unique_ptr<TdiInfoParser>(
        new TdiInfoParser(std::move(tdi_info_mapper)));
    tdi_info_parser->parseTdiInfo(program_config.tdi_info_file_paths_);
    auto tdi_info = tdi::TdiInfo::makeTdiInfo(program_config.prog_name_,
                                              std::move(tdi_info_parser),
                                              table_factory.get());
    // Parse context json
    auto status = ContextInfoParser::parseContextJson(
        tdi_info.get(), device_id, program_config);
    if (status) {
      LOG_ERROR("%s:%d Failed to parse context.json", __func__, __LINE__);
    }

    tdi_info_map_.insert({program_config.prog_name_, std::move(tdi_info)});
  }
}


tdi_status_t Device::createSession(std::shared_ptr<tdi::Session>* session) const {
  auto session_t = std::make_shared<TdiSessionImpl>();
  tdi_status_t status = session_t->create();

  *session = session_t;
  if (status != TDI_SUCCESS)
    *session = nullptr;
  return status;
}

tdi_status_t Device::createTarget(std::unique_ptr<tdi::Target> * target) const {
/*TODOSW: check why second argument is 0 instead of PNA_DEV_PIPE_ALL*/
  *target = std::unique_ptr<tdi::Target>(new tdi::pna::rt::Target(this->device_id_,
                                                              0,
                                                              PNA_DIRECTION_ALL));
  return TDI_SUCCESS;
}

}  // namespace rt
}  // namespace pna
}  // namespace tdi

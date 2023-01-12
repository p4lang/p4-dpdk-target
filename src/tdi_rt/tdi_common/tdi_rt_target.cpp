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
                              const uint64_t &value) {
    // Since PNA can handle all the target_fields already,
    // Just let the tdi::pna::Target to handle this
    return tdi::pna::Target::setValue(target_field, value);
}

tdi_status_t Target::getValue(const tdi_target_e &target_field,
                              uint64_t *value) const {
    return tdi::pna::Target::getValue(target_field, value);
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
	       void *target_options,
               void *cookie)
    : tdi::pna::Device(
          device_id, arch_type, device_config, cookie) {
  auto mgr_list = static_cast<std::vector<tdi_mgr_type_e> *>(target_options);
  std::copy(
      mgr_list->begin(), mgr_list->end(), std::back_inserter(mgr_type_list_));

  // Parse tdi json for every program
  for (const auto &program_config : device_config) {
    LOG_DBG("%s:%d parsing %s",
              __func__,
              __LINE__,
              program_config.prog_name_.c_str());
    auto tdi_info_mapper = std::unique_ptr<tdi::TdiInfoMapper>(
        new tdi::pna::rt::TdiInfoMapper());
    auto table_factory = std::unique_ptr<tdi::TableFactory>(
        new tdi::pna::rt::TableFactory());

    auto tdi_info_parser = std::unique_ptr<TdiInfoParser>(
        new TdiInfoParser(std::move(tdi_info_mapper)));

    if (!program_config.tdi_info_file_paths.empty())
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

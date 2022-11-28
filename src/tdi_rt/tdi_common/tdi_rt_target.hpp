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
/** @file tdi_target.hpp
 *
 *  @brief Contains common cpp defines like Target, Flags
 */
#ifndef _TDI_RT_TARGET_HPP_
#define _TDI_RT_TARGET_HPP_

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <bf_types/bf_types.h>
// tdi includes
#include <tdi/common/tdi_defs.h>
#include <tdi/common/tdi_target.hpp>

// pna includes
#include <tdi/arch/pna//pna_target.hpp>

// tdi rt includes
#include <tdi_rt/tdi_rt_defs.h>

namespace tdi {
namespace pna {
namespace rt {

/**
 * @brief Class which encapsulates static info of a device eg.
 * Arch type,
 * Mgrs it was started with, State information, TdiInfo of all
 * programs, Pipeline profile information.
 *
 * Static info means that none of this can be changed after
 * device add happens.
 */
class Device : public tdi::pna::Device {
 public:
  Device(const tdi_dev_id_t &device_id,
         const tdi_arch_type_e &arch_type,
         const std::vector<tdi::ProgramConfig> &device_config,
         void *target_options,
         void *cookie);

  tdi_status_t createSession(
      std::shared_ptr<tdi::Session> * session) const override final;
  tdi_status_t createTarget(
      std::unique_ptr<tdi::Target> * target) const override final;
  virtual tdi_status_t createFlags(
      const uint64_t & /*flags_val*/,
      std::unique_ptr<tdi::Flags> * /*flags*/) const override final {
    return TDI_SUCCESS;
  }
  private:
  std::vector<tdi_mgr_type_e> mgr_type_list_;
};

/**
 * @brief Can be constructed by \ref tdi::Device::createTarget()
 */
class Target : public tdi::pna::Target {
 public:
  Target(tdi_dev_id_t dev_id,
         pna_pipe_id_t pipe_id,
         pna_direction_e direction)
      : tdi::pna::Target(dev_id, pipe_id, direction){};
  tdi_status_t setValue(const tdi_target_e &target,
                                const uint64_t &value) override;
  tdi_status_t getValue(const tdi_target_e &target,
                                uint64_t *value) const override;

  void getTargetVals(bf_dev_target_t *dev_tgt,
                     pna_direction_e *direction) const;
};

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  // _TDI_RT_TARGET_HPP_

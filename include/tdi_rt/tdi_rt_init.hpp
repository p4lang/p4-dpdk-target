/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) 2017-2021 Barefoot Networks, Inc.

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
/** @file tdi_rt_init.hpp
 *
 *  @brief Contains TDI Dev Manager APIs. These APIs help manage TdiInfo
 *  \n objects with respect to the devices and programs in the ecosystem.
 *
 *  \n Contains DevMgr, Device, Target, Flags
 */
#ifndef _TDI_RT_INIT_HPP_
#define _TDI_RT_INIT_HPP_

#include <functional>
#include <memory>
#include <mutex>

// tdi includes
//#include <tdi/common/tdi_target.hpp>
//#include <tdi/common/tdi_info.hpp>
//#include <tdi/common/tdi_session.hpp>

// pna includes
#include <tdi/arch/pna/pna_init.hpp>

// local includes

#ifdef __cplusplus
extern "C" {
#endif
#include <tdi/common/tdi_defs.h>
#include <dvm/bf_drv_intf.h>

#ifdef __cplusplus
}
#endif

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
         const std::vector<tdi_mgr_type_e> mgr_type_list,
         void *cookie);

  virtual tdi_status_t createSession(
      std::shared_ptr<tdi::Session>* session) const override final;
  virtual tdi_status_t createTarget(
      std::unique_ptr<tdi::Target> * target) const override final {
    *target=std::unique_ptr<tdi::Target>(new tdi::pna::Target(this->device_id_, 0, PNA_DIRECTION_ALL));
    return TDI_SUCCESS;
  }
  virtual tdi_status_t createFlags(
      const uint64_t & /*flags_val*/,
      std::unique_ptr<tdi::Flags> * /*flags*/) const override final {
    return TDI_SUCCESS;
  }

 private:
  // This is where the real state map wil go
  // std::map<std::string, std::shared_ptr<DeviceState>> tdi_dev_state_map_;
};

/**
 * @brief Class to manage initialization of TDI <br>
 * <B>Creation: </B> Cannot be created
 */
class Init : public tdi::Init {
 public:
  /**
   * @brief Bf Rt Module Init API. This function needs to be called to
   * initialize TDI. Some specific managers can be specified to be skipped
   * TDI initialization. This allows TDI session layer to not know about these
   * managers. By default, no mgr initialization is skipped if empty vector is
   * passed
   *
   * @param[in] mgr_type_list vector of mgrs to skip initializing. If
   * empty, don't skip anything
   * @return Status of the API call
   */
  static tdi_status_t tdiModuleInit(
      const std::vector<tdi_mgr_type_e> mgr_type_list);
  static std::string tdi_module_name;
  static bf_drv_client_handle_t tdi_drv_hdl;
};  // Init

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  

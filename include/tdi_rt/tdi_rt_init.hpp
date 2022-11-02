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
 * @brief Class to manage initialization of TDI <br>
 * <B>Creation: </B> Cannot be created
 */
class Init : public tdi::Init {
 public:
  /**
   * @brief Bf Rt Module Init API. This function needs to be called to
   * initialize TDI. Target specific options can be provided
   *
   * @param[in] target_options
   * @return Status of the API call
   */
  static tdi_status_t tdiModuleInit(void *target_options);
  static std::string tdi_module_name;
  static bf_drv_client_handle_t tdi_drv_hdl;
};  // Init

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  

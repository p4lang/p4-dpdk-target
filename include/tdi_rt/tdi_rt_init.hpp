/*******************************************************************************
 * Copyright (c) 2017-2021 Barefoot Networks, Inc.
 * SPDX-License-Identifier: Apache-2.0
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

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
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#ifdef __cplusplus
}
#endif

// tdi includes
//#include <tdi/common/tdi_info.hpp>

// target include
#include <tdi_rt/tdi_rt_init.hpp>
#include <tdi_rt/c_frontend/tdi_rt_init.h>

tdi_status_t tdi_module_init(const tdi_mgr_type_e* arr, const size_t arr_size) {
  std::vector<tdi_mgr_type_e> mgr_type_vec(arr, arr + arr_size);
  return tdi::pna::rt::Init::tdiModuleInit(mgr_type_vec);
}

/*******************************************************************************
 * Copyright (c) 2017-2018 Barefoot Networks, Inc.
 * SPDX-License-Identifier: Apache-2.0
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

tdi_status_t tdi_module_init(void *target_options) {
  return tdi::pna::rt::Init::tdiModuleInit(target_options);
}

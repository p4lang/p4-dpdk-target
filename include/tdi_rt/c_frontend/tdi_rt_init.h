/*******************************************************************************
 * Copyright (c) 2017-2021 Barefoot Networks, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * $Id: $
 *
 ******************************************************************************/
/** @file tdi_rt_init.h
 *
 *  @brief C frontend for rt specific init
 */
#ifndef _TDI_RT_INIT_H_
#define _TDI_RT_INIT_H_

// tdi includes
#include <tdi/common/tdi_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

tdi_status_t tdi_module_init(void *target_options);

#ifdef __cplusplus
}
#endif

#endif  // _TDI_RT_INIT_H_

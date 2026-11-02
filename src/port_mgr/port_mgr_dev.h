/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef port_mgr_dev_h_included
#define port_mgr_dev_h_included

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>

bf_status_t port_mgr_dev_add(bf_dev_id_t dev_id,
			     bf_dev_family_t dev_family,
			     bf_device_profile_t *profile,
			     bf_dev_init_mode_t warm_init_mode);
bf_status_t port_mgr_dev_remove(bf_dev_id_t dev_id);

#ifdef __cplusplus
}
#endif /* C++ */

#endif

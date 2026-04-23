/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>  //for memset
#include <dvm/bf_drv_intf.h>
#include <lld/lld_err.h>
#include "lld.h"
#include "lld_dev.h"

// global LLD context
struct lld_context_t g_ctx;
struct lld_context_t *lld_ctx = &g_ctx;
bool skip_dev_init = false;
/** \brief Initialize LLD module
 *
 * \param is_master, LLD instance is "master" and will perform
 * core-reset and interrupt initialization
 * \param wr_fn, user-defined register write callback fn
 * \param rd_fn, user-defined register read callback fn
 * called during bf_device_add
 */
void lld_init(void)
{
	/* Register for notificatons */
	bf_drv_client_handle_t bf_drv_hdl;
	bf_status_t r;
	bf_drv_client_callbacks_t callbacks;

	// init and clear LLD context structure
	memset(lld_ctx, 0, sizeof(*lld_ctx));

	// register with dvm for important callbacks
	r = bf_drv_register("lld", &bf_drv_hdl);
	bf_sys_assert(r == BF_SUCCESS);

	memset((void *)&callbacks, 0, sizeof(bf_drv_client_callbacks_t));

	callbacks.device_add = lld_dev_add;
	callbacks.device_del = lld_dev_remove;
	bf_drv_client_register_callbacks(bf_drv_hdl, &callbacks, BF_CLIENT_PRIO_5);

	// initialize debug environment
	lld_debug_init();
}

/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "bf_switchd_lib_init.h"

extern int bf_switchd_lib_init_local(void *ctx);

/** \brief initialize the bf_switchd
 *
 * \param ctx: Per device context
 *
 * \return: BF_SUCCESS (0)
 * \return: -ive integer
 */

int bf_switchd_lib_init(bf_switchd_context_t *ctx)
{
	return bf_switchd_lib_init_local((void *)ctx);
}

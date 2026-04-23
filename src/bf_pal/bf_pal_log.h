/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BF_PAL_LOG_H__
#define __BF_PAL_LOG_H__

#include "osdep/p4_sde_osdep.h"

#define BF_PAL_ERROR(...) \
bf_sys_log_and_trace(BF_MOD_PAL, BF_LOG_ERR, __VA_ARGS__)

#define BF_PAL_DEBUG(...) \
bf_sys_log_and_trace(BF_MOD_PAL, BF_LOG_DBG, __VA_ARGS__)

#endif /* __BF_PAL_LOG_H__ */

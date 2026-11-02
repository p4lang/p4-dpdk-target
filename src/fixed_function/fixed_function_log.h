/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __FIXED_FUNCTION_LOG_H__
#define __FIXED_FUNCTION_LOG_H__

#include <osdep/p4_sde_osdep.h>

#define LOG_ERROR(...) \
        P4_SDE_LOG(BF_MOD_PAL, BF_LOG_ERR, __VA_ARGS__)
#define LOG_WARN(...) \
        P4_SDE_LOG(BF_MOD_PAL, BF_LOG_WARN, __VA_ARGS__)
#define LOG_TRACE(...) \
        P4_SDE_LOG(BF_MOD_PAL, BF_LOG_INFO, __VA_ARGS__)
#define LOG_DBG(...) P4_SDE_LOG(BF_MOD_PAL, BF_LOG_DBG, __VA_ARGS__)

#endif /* __FIXED_FUNCTION_LOG_H__ */


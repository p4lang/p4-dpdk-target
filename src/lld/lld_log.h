/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LLD_LOG_INCLUDED
#define LLD_LOG_INCLUDED

#include <osdep/p4_sde_osdep.h>

#define lld_log_critical(...) \
bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_CRIT, __VA_ARGS__)

#define lld_log_error(...) \
bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_ERR, __VA_ARGS__)

#define lld_log_warn(...) \
bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_WARN, __VA_ARGS__)

#define lld_log_trace(...) \
bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_INFO, __VA_ARGS__)

#define lld_log_debug(...) \
bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_DBG, __VA_ARGS__)

#define lld_log lld_log_debug

enum lld_log_type_e {
	LOG_TYP_GLBL = 0,
	LOG_TYP_CHIP,
};

int lld_log_worthy(enum lld_log_type_e typ, int p1, int p2, int p3);
int lld_log_set(enum lld_log_type_e typ, int p1, int p2, int p3);
void lld_log_settings(void);
void lld_log_internal(const char *fmt, ...);

#endif  // LLD_LOG_INCUDED

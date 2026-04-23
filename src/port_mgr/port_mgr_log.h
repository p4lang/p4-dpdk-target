/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORT_MGR_LOG_INCLUDED
#define PORT_MGR_LOG_INCLUDED

#include <osdep/p4_sde_osdep.h>

#define port_mgr_log_critical(...) \
bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_CRIT, __VA_ARGS__)

#define port_mgr_log_error(...) \
bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_ERR, __VA_ARGS__)

#define port_mgr_log_warn(...) \
bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_WARN, __VA_ARGS__)

#define port_mgr_log_trace(...) \
bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_INFO, __VA_ARGS__)

#define port_mgr_log_debug(...) \
bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_DBG, __VA_ARGS__)

#define port_mgr_log port_mgr_log_debug

void port_mgr_log_internal(const char *fmt, ...);

#endif  // PORT_MGR_LOG_INCUDED

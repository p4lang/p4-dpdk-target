/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __DVM_LOG_H__
#define __DVM_LOG_H__

#include <osdep/p4_sde_osdep.h>

#define LOG_CRIT(...) bf_sys_log_and_trace(BF_MOD_DVM, BF_LOG_CRIT, __VA_ARGS__)
#define LOG_ERROR(...) bf_sys_log_and_trace(BF_MOD_DVM, BF_LOG_ERR, __VA_ARGS__)
#define LOG_WARN(...) bf_sys_log_and_trace(BF_MOD_DVM, BF_LOG_WARN, __VA_ARGS__)
#define LOG_TRACE(...) \
  bf_sys_log_and_trace(BF_MOD_DVM, BF_LOG_INFO, __VA_ARGS__)
#define LOG_DBG(...) bf_sys_log_and_trace(BF_MOD_DVM, BF_LOG_DBG, __VA_ARGS__)

#endif /* __DVM_LOG_H__ */

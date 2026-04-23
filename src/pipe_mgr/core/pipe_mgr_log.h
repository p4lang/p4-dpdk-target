/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __PIPE_MGR_LOG_H__
#define __PIPE_MGR_LOG_H__

#include <osdep/p4_sde_osdep.h>

#define LOG_ERROR(...) \
	P4_SDE_LOG(BF_MOD_PIPE, BF_LOG_ERR, __VA_ARGS__)
#define LOG_WARN(...) \
	P4_SDE_LOG(BF_MOD_PIPE, BF_LOG_WARN, __VA_ARGS__)
#define LOG_TRACE(...) \
	P4_SDE_LOG(BF_MOD_PIPE, BF_LOG_INFO, __VA_ARGS__)
#define LOG_DBG(...) P4_SDE_LOG(BF_MOD_PIPE, BF_LOG_DBG, __VA_ARGS__)

#endif /* __PIPE_MGR_LOG_H__ */

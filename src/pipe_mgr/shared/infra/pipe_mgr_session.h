/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*!
 * @file pipe_mgr_session.h
 * @date
 *
 * Sessions Management.
 */
#ifndef __PIPE_MGR_SESSION_H__
#define __PIPE_MGR_SESSION_H__

int pipe_mgr_api_enter(u32 sess_hdl);
void pipe_mgr_api_exit(u32 sess_hdl);
int pipe_mgr_api_exclusive_enter(u32 sess_hdl);
void pipe_mgr_api_exclusive_exit(u32 sess_hdl);
int pipe_mgr_session_create(u32 *sess_hdl);
int pipe_mgr_session_destroy(u32 sess_hdl);
bool pipe_mgr_session_valid(u32 sess_hdl);

#endif

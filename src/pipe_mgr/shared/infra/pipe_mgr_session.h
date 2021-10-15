/*
 * Copyright(c) 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

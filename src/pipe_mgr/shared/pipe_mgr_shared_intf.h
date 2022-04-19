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
 * @file pipe_mgr_shared_intf.h
 * @date
 *
 * PipeMgr shared code interfaces to be used by core code.
 */
#ifndef __PIPE_MGR_SHARED_INTF_H__
#define __PIPE_MGR_SHARED_INTF_H__

#include <dvm/bf_drv_profile.h>
#include "infra/pipe_mgr_int.h"
#include <dvm/bf_drv_intf.h>

int pipe_mgr_shared_init(void);
void pipe_mgr_shared_cleanup(void);
int pipe_mgr_shared_add_device(int dev_id,
			       enum bf_dev_family_t dev_family,
			       struct bf_device_profile *prof,
			       enum bf_dev_init_mode_s warm_init_mode);
int pipe_mgr_shared_remove_device(int dev_id);
int pipe_mgr_get_int_sess_hdl(int *sess_hdl);
int pipe_mgr_get_num_profiles(int dev_id, uint32_t *num_profiles);
int pipe_mgr_is_pipe_valid(int dev_id, uint32_t dev_pipe_id);
int pipe_mgr_set_profile(int dev_id,
			 int profile_id,
			 struct bf_p4_program *p4_program,
			 struct bf_p4_pipeline *p4_pipeline,
			 struct pipe_mgr_p4_pipeline *parsed_pipe_ctx);
int pipe_mgr_init_dev(int dev_id,
		      struct bf_device_profile *prof);
int pipe_mgr_get_profile_ctx(struct bf_dev_target_t dev_tgt,
			     struct pipe_mgr_p4_pipeline **parsed_pipe_ctx);
int pipe_mgr_get_profile(int dev_id,
			 int profile_id,
			 struct pipe_mgr_profile **profile);
bf_status_t pipe_mgr_shared_enable_pipeline(bf_dev_id_t dev_id,
		int profile_id,
		void *spec_file,
		enum bf_dev_init_mode_s warm_init_mode);
void pipe_mgr_free_pipe_ctx(struct pipe_mgr_p4_pipeline *pipe_ctx);
void pipe_mgr_free_mat_state(struct pipe_mgr_mat_state *mat_state);
bool pipe_mgr_mat_store_entries(struct pipe_mgr_mat_ctx *mat_ctx);
#endif

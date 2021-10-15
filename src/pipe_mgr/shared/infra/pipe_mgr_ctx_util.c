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
#include <stdio.h>

#include <osdep/p4_sde_osdep.h>
#include <bf_types/bf_types.h>
#include "pipe_mgr_ctx_util.h"

#include "../../core/pipe_mgr_log.h"
#include "../pipe_mgr_shared_intf.h"

/* TODO: Update this function to return object of all
 * table type as opaque void * pointer.
 */
int pipe_mgr_ctx_get_tbl(struct bf_dev_target_t dev_tgt,
			 u32 tbl_hdl,
			 struct pipe_mgr_mat **tbl)
{
	struct pipe_mgr_mat *cur_tbl;
	struct pipe_mgr_p4_pipeline *ctx_obj;
	int status;

	status = pipe_mgr_get_profile_ctx(dev_tgt, &ctx_obj);
	if (status)
		return status;

	cur_tbl = ctx_obj->mat_tables;
	while (cur_tbl) {
		if (cur_tbl->ctx.handle == tbl_hdl) {
			*tbl = cur_tbl;
			return BF_SUCCESS;
		}
		cur_tbl = cur_tbl->next;
	}
	return BF_OBJECT_NOT_FOUND;
}

int pipe_mgr_ctx_get_action(struct pipe_mgr_mat_ctx *tbl_ctx,
			    u32 act_fn_hdl,
			    struct pipe_mgr_actions_list **action)
{
	struct pipe_mgr_actions_list *cur_act;

	if (!tbl_ctx || !action)
		return BF_INVALID_ARG;

	cur_act = tbl_ctx->actions;
	while (cur_act) {
		if (cur_act->handle == act_fn_hdl) {
			*action = cur_act;
			return BF_SUCCESS;
		}
		cur_act = cur_act->next;
	}
	return BF_OBJECT_NOT_FOUND;
}

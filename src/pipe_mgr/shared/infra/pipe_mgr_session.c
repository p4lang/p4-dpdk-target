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

/* P4 SDE Headers */
#include <osdep/p4_sde_osdep.h>
#include <pipe_mgr/shared/pipe_mgr_infra.h>
#include <dvm/bf_drv_intf.h>

/* Local module headers */
#include "../../core/pipe_mgr_log.h"
#include "pipe_mgr_int.h"
#include "../pipe_mgr_shared_intf.h"
#include "pipe_mgr_session.h"

extern p4_sde_rwlock pipe_mgr_lock;

static int api_enter(u32 sess_hdl, bool exclusive_lock)
{
	struct pipe_mgr_ctx *ctx;
	int status;

	if (exclusive_lock)
		status = P4_SDE_RWLOCK_WRLOCK(&pipe_mgr_lock);
	else
		status = P4_SDE_RWLOCK_RDLOCK(&pipe_mgr_lock);

	if (status)
		return BF_UNEXPECTED;

	if (!pipe_mgr_session_valid(sess_hdl)) {
		status = BF_INVALID_ARG;
		goto rel_pipe_mgr_lock;
	}

	ctx = get_pipe_mgr_ctx();
	if (!ctx) {
		status = BF_NOT_READY;
		goto rel_pipe_mgr_lock;
	}

	status = P4_SDE_MUTEX_LOCK(&ctx->sessions[sess_hdl].lock);
	if (status) {
		status = BF_UNEXPECTED;
		goto rel_pipe_mgr_lock;
	}

	return status;

rel_pipe_mgr_lock:
	if (P4_SDE_RWLOCK_UNLOCK(&pipe_mgr_lock))
		LOG_ERROR("Releasing pipe_mgr_lock failed");
	return status;
}

/*
 * Provides serialization of operations in a session. Other sessions
 * can run in parallel to the session provided here since PipeMgr Read
 * Lock is acquired here.
 *
 * @param  sess_hdl	Session handle
 * @return		Status of the API call
 */
int pipe_mgr_api_enter(u32 sess_hdl)
{
	return api_enter(sess_hdl, false);
}

/*
 * Releases locks acquired in 'pipe_mgr_api_enter'.
 *
 * @param  sess_hdl	Session handle
 * @return		None
 */
void pipe_mgr_api_exit(u32 sess_hdl)
{
	struct pipe_mgr_ctx *ctx;

	ctx = get_pipe_mgr_ctx();
	if (!ctx) {
		LOG_ERROR("Pipe Mgr not ready");
		return;
	}

	if (pipe_mgr_session_valid(sess_hdl)) {
		if (P4_SDE_MUTEX_UNLOCK(&ctx->sessions[sess_hdl].lock)) {
			LOG_ERROR("Unlocking session's %u lock failed",
				  sess_hdl);
		}
	} else {
		LOG_ERROR("Session %u is not found", sess_hdl);
	}

	if (P4_SDE_RWLOCK_UNLOCK(&pipe_mgr_lock))
		LOG_ERROR("Unlocking pipe_mgr_lock failed");
}

/*
 * Provides exclusive access into the PipeMgr API. No other parallel
 * PipeMgr API can be executed when this function succeeds and till
 * 'pipe_mgr_api_exclusive_exit' is called to release the locks.
 *
 * @param  sess_hdl	Session handle
 * @return		Status of the API call
 */
int pipe_mgr_api_exclusive_enter(u32 sess_hdl)
{
	return api_enter(sess_hdl, true);
}

/*
 * Releases locks acquired in 'pipe_mgr_exclusive_api_enter'.
 *
 * @param  sess_hdl	Session handle
 * @return		None
 */
void pipe_mgr_api_exclusive_exit(u32 sess_hdl)
{
	pipe_mgr_api_exit(sess_hdl);
}

/*
 * Create session.
 *
 * @param  sess_hdl	Session handle is returned on success.
 * @return		Status of the API call
 */
int pipe_mgr_session_create(u32 *sess_hdl)
{
	struct pipe_mgr_ctx *ctx;
	int status;
	u32 i;

	if (!sess_hdl)
		return BF_INVALID_ARG;

	ctx = get_pipe_mgr_ctx();
	if (!ctx)
		return BF_NOT_READY;

	for (i = 0; i < P4_SDE_MAX_SESSIONS; i++) {
		if (!ctx->sessions[i].in_use)
			break;
	}

	if (i == P4_SDE_MAX_SESSIONS) {
		LOG_ERROR("No free sessions available.");
		return BF_MAX_SESSIONS_EXCEEDED;
	}

	status = P4_SDE_MUTEX_INIT(&ctx->sessions[i].lock);
	if (status) {
		LOG_ERROR("Initializing session lock failed.");
		return BF_UNEXPECTED;
	}

	ctx->sessions[i].in_use = true;
	*sess_hdl = i;

	return BF_SUCCESS;
}

/*
 * Destroy session.
 *
 * @param  sess_hdl	Session handle.
 * @return		Status of the API call
 */
int pipe_mgr_session_destroy(u32 sess_hdl)
{
	struct pipe_mgr_ctx *ctx;

	if (!pipe_mgr_session_valid(sess_hdl))
		return BF_SESSION_NOT_FOUND;

	ctx = get_pipe_mgr_ctx();
	if (!ctx)
		return BF_NOT_READY;

	ctx->sessions[sess_hdl].in_use = false;
	P4_SDE_MUTEX_DESTROY(&ctx->sessions[sess_hdl].lock);

	return BF_SUCCESS;
}

/*
 * Verifies if the session is valid.
 *
 * @param  sess_hdl	Session handle.
 * @return		True/False based on the session's validity.
 */
bool pipe_mgr_session_valid(u32 sess_hdl)
{
	struct pipe_mgr_ctx *ctx;

	if (sess_hdl >= P4_SDE_MAX_SESSIONS) {
		LOG_ERROR("Session %u is invalid", sess_hdl);
		return false;
	}

	ctx = get_pipe_mgr_ctx();
	if (!ctx) {
		LOG_ERROR("Device is not ready");
		return false;
	}

	return ctx->sessions[sess_hdl].in_use;
}

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
#include "../dal/dal_init.h"
#include "../pipe_mgr_shared_intf.h"
#include "pipe_mgr_session.h"
#include "../dal/dal_mat.h"

/* Pointer to global pipe_mgr context */
static struct pipe_mgr_ctx *pipe_mgr_ctx_obj;
/* RW Lock to protect pipe_mgr_ctx_obj global object */
p4_sde_rwlock pipe_mgr_lock;
static u32 pipe_mgr_int_sess_hndl;
struct pipe_mgr_ctx *get_pipe_mgr_ctx()
{
	return pipe_mgr_ctx_obj;
}

struct pipe_mgr_dev *pipe_mgr_get_dev(int dev_id)
{
	struct pipe_mgr_dev *d;
	struct pipe_mgr_ctx *c;
	p4_sde_map_sts s;
	bf_map_t *m;

	c = get_pipe_mgr_ctx();
	if (!c)
		return NULL;
	m = &c->dev_map;
	d = NULL;
	s = P4_SDE_MAP_GET(m, dev_id, (void **)&d);
	return s == BF_MAP_OK ? d : NULL;
}

static void free_dev(struct pipe_mgr_dev *dev)
{
	struct pipe_mgr_p4_pipeline *pipe_ctx;
	uint32_t i;

	if (dev->profiles) {
		for (i = 0; i < dev->num_pipeline_profiles; i++) {
			pipe_ctx = &dev->profiles[i].pipe_ctx;
			pipe_mgr_free_pipe_ctx(pipe_ctx);
			P4_SDE_RWLOCK_DESTROY(&dev->profiles[i].lock);
		}
		P4_SDE_FREE(dev->profiles);
	}
	P4_SDE_FREE(dev);
}

int pipe_mgr_set_dev(struct pipe_mgr_dev **dev,
		     int dev_id,
		     struct bf_device_profile *profile)
{
	struct pipe_mgr_dev *dev_info;
	p4_sde_map_sts map_sts;
	uint32_t i;
	int p;

	dev_info = P4_SDE_MALLOC(sizeof(*dev_info));
	if (!dev_info)
		return BF_NO_SYS_RESOURCES;

	P4_SDE_MEMSET(dev_info, 0, sizeof(*dev_info));

	/* Initialize device ID and type */
	dev_info->dev_id = dev_id;
	/* TODO: Set dev_type and dev_family using lld functions */

	/* Save the number of pipeline profiles and allocate them. */
	if (!profile) {
		dev_info->num_pipeline_profiles = 1;
	} else {
		dev_info->num_pipeline_profiles = 0;
		for (p = 0; p < profile->num_p4_programs; p++) {
			dev_info->num_pipeline_profiles +=
				profile->p4_programs[p].num_p4_pipelines;
		}
	}
	dev_info->profiles = P4_SDE_CALLOC(dev_info->num_pipeline_profiles,
					   sizeof(*dev_info->profiles));
	if (!dev_info->profiles) {
		free_dev(dev_info);
		return BF_NO_SYS_RESOURCES;
	}

	for (i = 0; i < dev_info->num_pipeline_profiles; i++) {
		if (P4_SDE_RWLOCK_INIT(&dev_info->profiles[i].lock, NULL)) {
			free_dev(dev_info);
			return BF_UNEXPECTED;
		}
		dev_info->profiles[i].profile_id = i;
	}

	map_sts = P4_SDE_MAP_ADD(&pipe_mgr_ctx_obj->dev_map, dev_id, dev_info);
	if (map_sts != BF_MAP_OK) {
		free_dev(dev_info);
		LOG_ERROR("Failed to save device info, dev %d sts %d",
			  dev_id, map_sts);
		return BF_UNEXPECTED;
	}
	*dev = dev_info;

	return BF_SUCCESS;
}

int pipe_mgr_set_profile(int dev_id,
			 int profile_id,
			 struct bf_p4_program *p4_program,
			 struct bf_p4_pipeline *p4_pipeline,
			 struct pipe_mgr_p4_pipeline *parsed_pipe_ctx)
{
	struct pipe_mgr_profile *profile;
	struct pipe_mgr_dev *dev;

	dev = pipe_mgr_get_dev(dev_id);
	if (!dev)
		return BF_NOT_READY;

	profile = &dev->profiles[profile_id];
	if (p4_program) {
		strncpy(profile->prog_name,
			p4_program->prog_name,
			P4_SDE_PROG_NAME_LEN - 1);
	}

	if (p4_pipeline) {
		strncpy(profile->pipeline_name,
			p4_pipeline->p4_pipeline_name,
			P4_SDE_PROG_NAME_LEN - 1);
		strncpy(profile->cfg_file,
			p4_pipeline->cfg_file,
			PIPE_MGR_CFG_FILE_LEN - 1);
		profile->core_id = p4_pipeline->core_id;
	}
	if (parsed_pipe_ctx)
		profile->pipe_ctx = *parsed_pipe_ctx;
	return BF_SUCCESS;
}

int pipe_mgr_get_num_profiles(int dev_id, uint32_t *num_profiles)
{
	struct pipe_mgr_dev *dev;

	dev = pipe_mgr_get_dev(dev_id);
	if (!dev)
		return BF_NOT_READY;

	*num_profiles = dev->num_pipeline_profiles;
	return BF_SUCCESS;
}

int pipe_mgr_is_pipe_valid(int dev_id, uint32_t dev_pipe_id)
{
	struct pipe_mgr_dev *dev;

        dev = pipe_mgr_get_dev(dev_id);
        if (!dev)
                return BF_NOT_READY;

	if (dev_pipe_id >= dev->num_pipeline_profiles) {
		LOG_ERROR("%s : Invalid Pipe ID : %d", __func__, dev_pipe_id);
		return BF_INVALID_ARG;
	}

	return BF_SUCCESS;
}

int pipe_mgr_init_dev(int dev_id,
		      struct bf_device_profile *prof)
{
	struct pipe_mgr_dev *dev;
	int status;

	LOG_TRACE("Entering %s device %u", __func__, dev_id);

	dev = pipe_mgr_get_dev(dev_id);
	if (dev) {
		LOG_TRACE("Exiting %s", __func__);
		return BF_ALREADY_EXISTS;
	}

	status = pipe_mgr_set_dev(&dev, dev_id, prof);
	if (status != BF_SUCCESS) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	LOG_TRACE("Exiting %s", __func__);
	return BF_SUCCESS;
}

/* API to instantiate a new device */
int pipe_mgr_shared_add_device(int dev_id,
			       enum bf_dev_family_t dev_family,
			       struct bf_device_profile *prof,
			       enum bf_dev_init_mode_s warm_init_mode)
{
	int status;

	LOG_TRACE("Entering %s device %u", __func__, dev_id);

	status = dal_add_device(dev_id, dev_family, prof,
				warm_init_mode);
	if (status != BF_SUCCESS) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	LOG_TRACE("Exiting %s", __func__);
	return BF_SUCCESS;
}

/* API to de-instantiate a device */
int pipe_mgr_shared_remove_device(int dev_id)
{
	struct pipe_mgr_dev *dev;
	int status;

	LOG_TRACE("Entering %s device %u", __func__, dev_id);

	if (!pipe_mgr_ctx_obj) {
		LOG_TRACE("Exiting %s", __func__);
		return BF_OBJECT_NOT_FOUND;
	}

	dev = pipe_mgr_get_dev(dev_id);
	if (!dev) {
		LOG_TRACE("Exiting %s", __func__);
		return BF_OBJECT_NOT_FOUND;
	}

	status = dal_remove_device(dev_id);
	if (status != BF_SUCCESS) {
		LOG_ERROR("%s: failed to de-instantiate device %d",
			  __func__, dev_id);
	}

	free_dev(dev);
	if (P4_SDE_MAP_RMV(&pipe_mgr_ctx_obj->dev_map, dev_id)) {
		LOG_ERROR("%s: failed to remove object from map", __func__);
		status = (status != BF_SUCCESS) ? status : BF_OBJECT_NOT_FOUND;
	}

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

bf_status_t pipe_mgr_shared_enable_pipeline(bf_dev_id_t dev_id,
		int profile_id,
		void *spec_file,
		enum bf_dev_init_mode_s warm_init_mode)
{
	bf_status_t status = BF_SUCCESS;

	LOG_TRACE("Entering %s device %u", __func__, dev_id);
	status = dal_enable_pipeline(dev_id, profile_id, spec_file,
				     warm_init_mode);
	LOG_TRACE("Exiting %s", __func__);
	return status;
}

int pipe_mgr_shared_init(void)
{
	struct pipe_mgr_ctx *ctx;
	int status;

	LOG_TRACE("Entering %s", __func__);

	/* If pipe_mgr is already initialized ignore */
	if (pipe_mgr_ctx_obj) {
		LOG_TRACE("Exiting %s", __func__);
		return BF_SUCCESS;
	}

	/* Initialize context for service */
	ctx = P4_SDE_CALLOC(1, sizeof(struct pipe_mgr_ctx));
	if (!ctx) {
		LOG_ERROR("%s: failed to allocate memory for context",
			  __func__);
		status = BF_NO_SYS_RESOURCES;
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = P4_SDE_RWLOCK_INIT(&pipe_mgr_lock, NULL);
	if (status) {
		LOG_ERROR("Initializing RW lock failed");
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	status = P4_SDE_RWLOCK_WRLOCK(&pipe_mgr_lock);
	if (status) {
		LOG_ERROR("Acquiring WR lock on pipemgr failed");
		status = BF_UNEXPECTED;
		goto cleanup;
	}

	status = P4_SDE_MAP_INIT(&ctx->dev_map);
	if (status) {
		LOG_ERROR("Initializing device objects container failed");
		status = BF_UNEXPECTED;
		goto cleanup_pipe_mgr_lock;
	}

	/* Store the context pointer */
	pipe_mgr_ctx_obj = ctx;
	/* Internal session handle should not be updated after init and
	 * hence pipe_mgr_int_sess_hndl is not protected by any lock.
	 */
	status = pipe_mgr_session_create(&pipe_mgr_int_sess_hndl);
	if (status) {
		LOG_ERROR("Creating internal session failed");
		goto cleanup_dev_map;
	}

	P4_SDE_RWLOCK_UNLOCK(&pipe_mgr_lock);
	LOG_TRACE("Exiting %s", __func__);
	return BF_SUCCESS;

cleanup_dev_map:
	P4_SDE_MAP_DESTROY(&ctx->dev_map);

cleanup_pipe_mgr_lock:
	P4_SDE_RWLOCK_UNLOCK(&pipe_mgr_lock);
	P4_SDE_RWLOCK_DESTROY(&pipe_mgr_lock);

cleanup:
	P4_SDE_FREE(ctx);
	LOG_TRACE("Exiting %s", __func__);
	return status;
}

void pipe_mgr_shared_cleanup(void)
{
	LOG_TRACE("Entering %s", __func__);

	if (P4_SDE_RWLOCK_WRLOCK(&pipe_mgr_lock))
		LOG_ERROR("Acquiring pipe_mgr_lock failed");

	if (!pipe_mgr_ctx_obj) {
		LOG_TRACE("Exiting %s", __func__);
		P4_SDE_RWLOCK_UNLOCK(&pipe_mgr_lock);
		return;
	}

	P4_SDE_MAP_DESTROY(&pipe_mgr_ctx_obj->dev_map);
	if (pipe_mgr_session_destroy(pipe_mgr_int_sess_hndl))
		LOG_ERROR("Destroying internal session %d failed",
			  pipe_mgr_int_sess_hndl);

	P4_SDE_FREE(pipe_mgr_ctx_obj);
	pipe_mgr_ctx_obj = NULL;

	if (P4_SDE_RWLOCK_UNLOCK(&pipe_mgr_lock))
		LOG_ERROR("Releasing pipe_mgr_lock failed.");

	if (P4_SDE_RWLOCK_DESTROY(&pipe_mgr_lock))
		LOG_ERROR("Destroy of pipe_mgr_lock failed.");

	LOG_TRACE("Exiting %s", __func__);
}

int pipe_mgr_get_int_sess_hdl(int *sess_hdl)
{
	if (!pipe_mgr_ctx_obj)
		return BF_NOT_READY;
	*sess_hdl = pipe_mgr_int_sess_hndl;
	return BF_SUCCESS;
}

int pipe_mgr_get_profile(int dev_id,
			 int profile_id,
			 struct pipe_mgr_profile **profile)
{
	struct pipe_mgr_dev *dev;

	dev = pipe_mgr_get_dev(dev_id);
	if (!dev)
		return BF_NOT_READY;

	*profile = &dev->profiles[profile_id];
	return BF_SUCCESS;
}

/* Return context json object associated with dev_tgt (dev_id, pipe_id)
 */
int pipe_mgr_get_profile_ctx(struct bf_dev_target_t dev_tgt,
			     struct pipe_mgr_p4_pipeline **parsed_pipe_ctx)
{
	struct pipe_mgr_profile *profile;
	int status;

	status = pipe_mgr_get_profile(dev_tgt.device_id,
				      dev_tgt.dev_pipe_id, &profile);
	if (status)
		return status;
	*parsed_pipe_ctx = &profile->pipe_ctx;
	return BF_SUCCESS;
}

/*
 * Acquires locks needed for PipeMgr API. This function should be used
 * by all the public PipeMgr API for handling P4 entities (shared/features/
 * directory).
 *
 * @param  sess_hdl	Session handle
 * @param  dev_tgt	Device target (device id, pipe id)
 * @return		Status of the API call
 */
int pipe_mgr_api_prologue(u32 sess_hdl, struct bf_dev_target_t dev_tgt)
{
	struct pipe_mgr_ctx *ctx;
	struct pipe_mgr_dev *dev;
	int status;

	status = pipe_mgr_api_enter(sess_hdl);
	if (status)
		return status;
	ctx = get_pipe_mgr_ctx();
	if (!ctx) {
		status = BF_NOT_READY;
		goto rel_lock;
	}

	dev = pipe_mgr_get_dev(dev_tgt.device_id);
	if (!dev) {
		status = BF_NOT_READY;
		goto rel_lock;
	}

	/* TODO: Validate dev_tgt.pipe_id and use it to index profiles array. */
	status = P4_SDE_RWLOCK_RDLOCK
			(&dev->profiles[dev_tgt.dev_pipe_id].lock);
	if (status) {
		status = BF_UNEXPECTED;
		goto rel_lock;
	}

	return BF_SUCCESS;

rel_lock:
	pipe_mgr_api_exit(sess_hdl);
	return status;
}

/*
 * Release locks which are acquired by 'pipe_mgr_api_prologue'.
 * This function should be used by all the public PipeMgr API for
 * handling P4 entities (shared/features/ directory).
 *
 * @param  sess_hdl	Session handle
 * @param  dev_tgt	Device target (device id, pipe id)
 * @return		None
 */
void pipe_mgr_api_epilogue(u32 sess_hdl, struct bf_dev_target_t dev_tgt)
{
	struct pipe_mgr_ctx *ctx;
	struct pipe_mgr_dev *dev;
	int status;

	ctx = get_pipe_mgr_ctx();
	if (!ctx) {
		LOG_ERROR("Pipe Mgr not ready");
		return;
	}

	dev = pipe_mgr_get_dev(dev_tgt.device_id);
	if (!dev) {
		LOG_ERROR("Pipe Mgr not ready");
		return;
	}

	/* TODO: Validate dev_tgt.pipe_id and use it to index profiles array. */
	status = P4_SDE_RWLOCK_UNLOCK
			(&dev->profiles[dev_tgt.dev_pipe_id].lock);
	if (status)
		LOG_ERROR("Unlocking Pipleline %d failed",
			  dev_tgt.dev_pipe_id);

	pipe_mgr_api_exit(sess_hdl);
	return;
}

void pipe_mgr_free_mat_state(struct pipe_mgr_mat_state *mat_state)
{
	int i;

	if (!mat_state)
		return;

	if (mat_state->entry_handle_array)
		P4_SDE_ID_DESTROY(mat_state->entry_handle_array);

	P4_SDE_MAP_DESTROY(&mat_state->entry_info_htbl);
	P4_SDE_MUTEX_DESTROY(&mat_state->lock);

	for (i = 0; i < mat_state->num_htbls; i++)
		bf_hashtbl_delete(mat_state->key_htbl[i]);

	P4_SDE_FREE(mat_state->key_htbl);
	P4_SDE_FREE(mat_state);
}

void pipe_mgr_free_pipe_ctx(struct pipe_mgr_p4_pipeline *pipe_ctx)
{
	struct pipe_mgr_mat *mat;

	mat = pipe_ctx->mat_tables;
	while (mat) {
		pipe_mgr_free_mat_state(mat->state);
		mat = mat->next;
	}
	PIPE_MGR_FREE_LIST(pipe_ctx->mat_tables);
	pipe_ctx->mat_tables = NULL;
}

int pipe_mgr_client_init(u32 *sess_hdl)
{
	int status;

	LOG_TRACE("Entering %s", __func__);

	if (P4_SDE_RWLOCK_WRLOCK(&pipe_mgr_lock)) {
		status = BF_UNEXPECTED;
		LOG_TRACE("Exiting %s with status %d", __func__, status);
		return status;
	}

	status = pipe_mgr_session_create(sess_hdl);

	if (P4_SDE_RWLOCK_UNLOCK(&pipe_mgr_lock)) {
		LOG_ERROR("Releasing pipe_mgr_lock failed.");
		status = status ? status : BF_UNEXPECTED;
	}

	LOG_TRACE("Exiting %s with status %d", __func__, status);
	return status;
}

int pipe_mgr_client_cleanup(u32 sess_hdl)
{
	int status;

	LOG_TRACE("Entering %s", __func__);

	if (P4_SDE_RWLOCK_WRLOCK(&pipe_mgr_lock)) {
		status = BF_UNEXPECTED;
		LOG_TRACE("Exiting %s with status %d", __func__, status);
		return status;
	}

	status = pipe_mgr_session_destroy(sess_hdl);

	if (P4_SDE_RWLOCK_UNLOCK(&pipe_mgr_lock)) {
		LOG_ERROR("Releasing pipe_mgr_lock failed.");
		status = status ? status : BF_UNEXPECTED;
	}

	LOG_TRACE("Exiting %s with status %d", __func__, status);
	return status;
}

bool pipe_mgr_mat_store_entries(struct pipe_mgr_mat_ctx *mat_ctx)
{
	return dal_mat_store_entries(mat_ctx);
}

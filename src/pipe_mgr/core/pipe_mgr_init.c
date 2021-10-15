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
#include <dvm/bf_drv_intf.h>
#include <pipe_mgr/shared/pipe_mgr_infra.h>

/* Local Headers */
#include "pipe_mgr_log.h"
#include "pipe_mgr_ctx_json.h"
#include "../shared/pipe_mgr_shared_intf.h"
#include "../shared/infra/pipe_mgr_session.h"
#include "../shared/infra/pipe_mgr_int.h"

static int pipe_mgr_add_device(int dev_id,
			       enum bf_dev_family_t dev_family,
			       struct bf_device_profile *prof,
			       enum bf_dev_init_mode_s warm_init_mode)
{
	int sess_hdl;
	int status;

	LOG_TRACE("Entering %s device %u", __func__, dev_id);

	status = pipe_mgr_get_int_sess_hdl(&sess_hdl);
	if (status != BF_SUCCESS) {
		LOG_TRACE("Exiting %s", __func__);
		return BF_SESSION_NOT_FOUND;
	}

	status = pipe_mgr_api_exclusive_enter(sess_hdl);
	if (status != BF_SUCCESS) {
		LOG_TRACE("Exiting %s", __func__);
		return BF_SESSION_NOT_FOUND;
	}

	status = pipe_mgr_init_dev(dev_id, prof);
	if (status != BF_SUCCESS) {
		LOG_ERROR("Initializing dev %d failed with err %d", dev_id,
			  status);
		goto api_exit;
	}

	status = pipe_mgr_ctx_import(dev_id, prof, warm_init_mode);
	if (status != BF_SUCCESS) {
		LOG_ERROR("Importing context json of dev %d"
				" failed with err %d",
				dev_id, status);
		goto api_exit;
	}

	status = pipe_mgr_shared_add_device(dev_id, dev_family,
			prof, warm_init_mode);
	if (status != BF_SUCCESS) {
		LOG_ERROR("pipe_mgr_shared_add_device() failed with err %d",
			  status);
		goto api_exit;
	}

	/* As part of stratum SetForwardingPipelineConfig API call
	FAST_RECFG mode trigger the add_device. Let's Build the
	pipemgr with passed SPEC file. */
	if (warm_init_mode == BF_DEV_WARM_INIT_FAST_RECFG) {
		FILE *spec = NULL;
		struct pipe_mgr_profile *profile;
		status = pipe_mgr_get_profile(dev_id, &profile);
		if (status) {
			LOG_ERROR("not able find profile with device_id  %d",
				  dev_id);
			status = BF_OBJECT_NOT_FOUND;
			goto api_exit;
		}
		spec = fopen(profile->cfg_file, "r");
		if (!spec) {
			LOG_ERROR("Cannot open file %s.\n", profile->cfg_file);
			status = BF_OBJECT_NOT_FOUND;
			goto api_exit;
		}
		LOG_TRACE("%s cfg_file %s \n", __func__, profile->cfg_file);
		status = pipe_mgr_shared_enable_pipeline(dev_id, spec,
							 warm_init_mode);

		/* closes the file pointed by spec */
		fclose(spec);
	}

api_exit:
	pipe_mgr_api_exclusive_exit(sess_hdl);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/* API to de-instantiate a device */
int pipe_mgr_remove_device(int dev_id)
{
	int sess_hdl;
	int status;

	LOG_TRACE("Entering %s device %u", __func__, dev_id);

	status = pipe_mgr_get_int_sess_hdl(&sess_hdl);
	if (status != BF_SUCCESS) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_api_exclusive_enter(sess_hdl);
	if (status != BF_SUCCESS) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_shared_remove_device(dev_id);
	pipe_mgr_api_exclusive_exit(sess_hdl);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

// TODO Remove this function and related APIs
bf_status_t pipe_mgr_add_port(bf_dev_id_t dev_id,
		bf_dev_port_t port,
		port_attributes_t *port_attrib)
{
	bf_status_t status = BF_SUCCESS;
	struct pipe_mgr_profile *profile;
	FILE *spec = NULL;

	LOG_TRACE("Entering %s device %u", __func__, dev_id);

	status = pipe_mgr_get_profile(dev_id, &profile);
	if (status) {
		LOG_ERROR("not able find profile with device_id  %d",
				dev_id);
		return BF_OBJECT_NOT_FOUND;
	}

	spec = fopen(profile->cfg_file, "r");
	if (!spec) {
		LOG_ERROR("Cannot open file %s.\n",profile->cfg_file);
		return BF_OBJECT_NOT_FOUND;
	}
	status = pipe_mgr_shared_enable_pipeline(dev_id, spec,
			BF_DEV_INIT_COLD);
	fclose(spec);
	LOG_TRACE("Exiting %s", __func__);
	return status;

}

int pipe_mgr_init(void)
{
	struct bf_drv_client_callbacks_s callbacks = {0};
	int bf_drv_hdl;
	int status;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_shared_init();
	if (status != BF_SUCCESS) {
		LOG_TRACE("Exiting %s", __func__);
		return BF_SUCCESS;
	}

	/* Register for notificatons */
	status = bf_drv_register("pipe-mgr", &bf_drv_hdl);
	if (status != BF_SUCCESS) {
		LOG_ERROR("%s: Registration failed, sts %s",
			  __func__, bf_err_str(status));
		LOG_TRACE("Exiting %s", __func__);
		return BF_NOT_READY;
	}
	callbacks.device_add = pipe_mgr_add_device;
	callbacks.device_del = pipe_mgr_remove_device;
	callbacks.port_add = pipe_mgr_add_port;
	bf_drv_client_register_callbacks(bf_drv_hdl, &callbacks,
					 BF_CLIENT_PRIO_3);
	LOG_TRACE("Exiting %s", __func__);
	return BF_SUCCESS;
}

void pipe_mgr_cleanup(void)
{
	LOG_TRACE("Entering %s", __func__);
	pipe_mgr_shared_cleanup();
	LOG_TRACE("Exiting %s", __func__);
}

/**
 * This function is used to build the pipeline
 *
 * @param  dev_id              Device ID
 * @return                     Status of the API call
 */
int pipe_mgr_enable_pipeline(bf_dev_id_t dev_id)
{
	struct pipe_mgr_profile *profile;
	int status = BF_SUCCESS;
	FILE *spec = NULL;
	int sess_hdl;

	LOG_TRACE("Entering %s device %u", __func__, dev_id);
	status = pipe_mgr_get_int_sess_hdl(&sess_hdl);
	if (status != BF_SUCCESS) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_api_exclusive_enter(sess_hdl);
	if (status != BF_SUCCESS) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_get_profile(dev_id, &profile);
	if (status) {
		LOG_ERROR("not able find profile with device_id  %d",
			  dev_id);
		goto api_exit;
	}

	spec = fopen(profile->cfg_file, "r");
	if (!spec) {
		LOG_ERROR("Cannot open file %s.\n", profile->cfg_file);
		status = BF_OBJECT_NOT_FOUND;
		goto api_exit;
	}
	status = pipe_mgr_shared_enable_pipeline(dev_id, spec,
						 BF_DEV_INIT_COLD);
	LOG_TRACE("%s cfg_file %s\n", __func__, profile->cfg_file);

	/* closes the file pointed by spec */
	fclose(spec);

api_exit:
	pipe_mgr_api_exclusive_exit(sess_hdl);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

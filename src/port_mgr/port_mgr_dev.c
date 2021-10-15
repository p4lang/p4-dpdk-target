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
 * @file port_mgr_dev.c
 *
 *
 * Functions for Port Manager Device actions
 */

#include "port_mgr/port_mgr_dev.h"
#include "port_mgr/port_mgr_config_json.h"
#include "port_mgr/port_mgr_log.h"
#include "port_mgr/port_mgr_intf.h"

bf_status_t port_mgr_dev_add(bf_dev_id_t dev_id,
			     bf_dev_family_t dev_family,
			     bf_device_profile_t *profile,
			     bf_dev_init_mode_t warm_init_mode)
{
	int status = BF_SUCCESS;

	port_mgr_log_trace("Entering %s", __func__);

	status = port_mgr_config_import(dev_id, profile);
	if (status != BF_SUCCESS) {
		port_mgr_log_trace("Exiting %s", __func__);
		return status;
	}

	return status;
}

bf_status_t port_mgr_dev_remove(bf_dev_id_t dev_id)
{
	/*
	 * TODO: Check if this is right function
	 *       for Port Mgr Cleanup
	 */
	port_mgr_log_trace("Entering %s", __func__);
	port_mgr_cleanup();
	port_mgr_log_trace("Exiting %s", __func__);
	return BF_SUCCESS;
}

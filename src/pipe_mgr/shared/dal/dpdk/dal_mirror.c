/*
 * Copyright(c) 2022 Intel Corporation.
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
 * @file dal_mirror.c
 *
 * @description Utilities for Mirroring (DPDK)
 */

#include "../dal_mirror.h"
#include "../../../core/pipe_mgr_log.h"
#include "../../infra/pipe_mgr_int.h"

#include <rte_swx_ctl.h>

/*!
 * Set the config params for mirror session.
 *
 * @param id session id for which config to be done.
 * @param params params for the mirror session.
 * @param p pointer to pipeline info.
 * @return Status of the API call
 */
bf_status_t
dal_mirror_session_set(uint32_t id,
		       void *params,
		       void *p)
{
	struct rte_swx_pipeline_mirroring_session_params mir_params;
	struct pipe_mgr_mir_prof *pipe_mir_params;
	int rc = BF_SUCCESS;

	pipe_mir_params = params;

	mir_params.port_id = pipe_mir_params->port_id;
	mir_params.fast_clone = pipe_mir_params->fast_clone;
	mir_params.truncation_length = pipe_mir_params->truncate_length;

	if (rte_swx_ctl_pipeline_mirroring_session_set(p,
						       id,
						       &mir_params)) {
		LOG_ERROR("Could not configure the mirror session params.");
		rc = BF_UNEXPECTED;
	}

	return rc;
}

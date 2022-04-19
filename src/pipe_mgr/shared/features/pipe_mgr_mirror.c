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
 * @file pipe_mgr_mirror.c
 *
 * @description Utilities for mirror profiles
 */

#include "pipe_mgr_mirror.h"
#include "../dal/dal_mirror.h"

/*!
 * Set the config params for mirror profile.
 *
 * @param id session id for which config to be done.
 * @param params params for the mirror session.
 * @param p pointer to pipeline info.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_mirror_session_set(uint32_t id,
			    void *params,
			    void *p)
{
	return dal_mirror_session_set(id, params, p);
}


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

/*
 * @file dal_mirror.h
 *
 * @description Utilities for Mirroring
 */

#ifndef __DAL_MIRROR_H__
#define __DAL_MIRROR_H__

#include <bf_types/bf_types.h>

/*
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
		       void *p);

/*
 * Clear the config params for mirror session.
 *
 * @param id session id for which config to be done.
 * @param p pointer to pipeline info.
 * @return Status of the API call
 */
bf_status_t
dal_mirror_session_clear(uint32_t id,
			 void *p);

/*
 * Set the config params for mirror profile.
 *
 * @param id profile id for which config to be done.
 * @param params params for the mirror profile.
 * @param dev_id device id
 * @return Status of the API call
 */
bf_status_t
dal_mirror_profile_set(uint32_t id,
		       void *params,
		       int dev_id);

/*
 * Clear the config params for mirror profile.
 *
 * @param id profile id for which config to be done.
 * @param dev_id device id
 * @return Status of the API call
 */
bf_status_t
dal_mirror_profile_clear(uint32_t id,
			 int dev_id);

#endif /* __DAL_MIRROR_H__ */

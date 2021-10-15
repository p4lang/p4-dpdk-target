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

#include <bf_pal/dev_intf.h>

bf_pal_dev_callbacks_t dev_cbs;

bf_status_t bf_pal_device_warm_init_begin(bf_dev_id_t dev_id,
					  bf_dev_init_mode_t warm_init_mode,
					  bool upgrade_agents)
{
	if (!dev_cbs.warm_init_begin)
		return BF_NOT_IMPLEMENTED;

	return dev_cbs.warm_init_begin(dev_id, warm_init_mode, upgrade_agents);
}

bf_status_t bf_pal_device_add(bf_dev_id_t dev_id,
			      bf_device_profile_t *device_profile)
{
	if (!dev_cbs.device_add)
		return BF_NOT_IMPLEMENTED;

	return dev_cbs.device_add(dev_id, device_profile);
}

bf_status_t bf_pal_device_warm_init_end(bf_dev_id_t dev_id)
{
	if (!dev_cbs.warm_init_end)
		return BF_NOT_IMPLEMENTED;

	return dev_cbs.warm_init_end(dev_id);
}

bf_status_t bf_pal_device_callbacks_register(bf_pal_dev_callbacks_t *callbacks)
{
	if (!callbacks)
		return BF_INVALID_ARG;

	memcpy(&dev_cbs, callbacks, sizeof(bf_pal_dev_callbacks_t));
	return BF_SUCCESS;
}

bf_status_t bf_pal_pltfm_type_get(bf_dev_id_t dev_id, bool *is_sw_model)
{
	if (!dev_cbs.pltfm_type_get)
		return BF_NOT_IMPLEMENTED;

	return dev_cbs.pltfm_type_get(dev_id, is_sw_model);
}

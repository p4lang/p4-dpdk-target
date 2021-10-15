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
#include <dvm/bf_drv_intf.h>
#include <bf_types/bf_types.h>

#include "../lld_dev.h"
#include "../lldlib_log.h"
#include "lld_dpdk_lib.h"

/** \brief Add a device to the system
 *
 * \param dev_id        : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param dev_family    : DPDK, ...
 * \param profile       : unused (common param for other modules)
 * \param dma_info      : DMA DR configuration (used, size)
 * \param warm_init_mode: driver start-up mode (cold, warm, fast-recfg)
 *
 * \return: BF_SUCCESS     : dev_id added successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 */
int lld_dev_add(bf_dev_id_t dev_id,
		bf_dev_family_t dev_family,
		bf_device_profile_t *profile,
		bf_dev_init_mode_t warm_init_mode)
{
	LOG_TRACE("Entering :%s\n", __func__);

	/* DPDK init. */
	if (warm_init_mode == BF_DEV_INIT_COLD)
		lld_dpdk_init(profile);

	LOG_TRACE("Exiting :%s\n", __func__);
	return BF_SUCCESS;
}

/** \brief Remove a device from the system
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : dev_id removed successfully
 * \return: BF_INVALID_ARG : dev_id invalid
 */
int lld_dev_remove(bf_dev_id_t dev_id)
{
	LOG_TRACE("Entering dummy STUB:%s\n", __func__);
	LOG_TRACE("Exiting dummy STUB:%s\n", __func__);
	return BF_SUCCESS;
}


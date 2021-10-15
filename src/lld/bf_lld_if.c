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
#include <lld/bf_dev_if.h>
#include "lld.h"
#include "lld_log.h"

/**
 * @file bf_lld_if.c
 * \brief Details LLD module-level APIs.
 *
 */

/**
 * @addtogroup lld-api
 * @{
 * This is a description of some APIs.
 */

/** \brief Initializa the LLD submodule of a process
 *
 * \param is_master    : whether this LLD instance is the "master" LLD instance
 * \param wr_fn        : Function used to write 32b chip registers
 * \param rd_fn        : Function used to read  32b chip registers
 *
 * \return: BF_SUCCESS : LLD initialized successfully
 */
bf_status_t bf_lld_init(void)
{
	lld_init();
	return BF_SUCCESS;
}

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

#include <bf_types/bf_types.h>
#include <port_mgr/port_mgr_intf.h>
/**
 * @file bf_port_if.c
 * \brief Details Port-level APIs.
 *
 */

/*****************************************************************************
 * bf_port_mgr_init
 *****************************************************************************/
bf_status_t bf_port_mgr_init(void)
{
	port_mgr_init();
	return BF_SUCCESS;
}

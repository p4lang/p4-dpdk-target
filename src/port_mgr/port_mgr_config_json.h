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
 * @file port_mgr_config_json.h
 *
 *
 * Functions for DPDK Port Manager Config Parsing
 */

#ifndef __PORT_MGR_CONFIG_JSON_H__
#define __PORT_MGR_CONFIG_JSON_H__
#include <dvm/bf_drv_profile.h>

int port_mgr_config_import(int dev_id,
			   struct bf_device_profile *dev_profile);
#endif /*__PORT_MGR_CONFIG_JSON_H__*/


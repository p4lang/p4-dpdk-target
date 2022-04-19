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

#ifndef PORT_MGR_PORT_H_INCLUDED
#define PORT_MGR_PORT_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <bf_types/bf_types.h>
#include <osdep/p4_sde_osdep.h>
#include <port_mgr/bf_port_if.h>

/**
 * Add a New Port
 * @param dev_id The Device ID
 * @param dev_port The Port ID
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @return Status of the API call.
 */
bf_status_t port_mgr_port_add(bf_dev_id_t dev_id,
			      bf_dev_port_t dev_port,
			      struct port_attributes_t *port_attrib);

/**
 * Get all statistics for a port
 * @param dev_id The Device ID
 * @param dev_port The Port ID
 * @param stats Array to hold all the stats read from hardware
 * @return Status of the API call.
 */
bf_status_t port_mgr_port_all_stats_get(bf_dev_id_t dev_id,
					bf_dev_port_t dev_port,
					u64 *stats);

/**
 * Get Port ID from MAC address
 * @param dev_id The Device ID
 * @param mac MAC Address String
 * @param port_id Port ID
 * @return Status of the API call.
 */
bf_status_t port_mgr_get_port_id_from_mac(bf_dev_id_t dev_id, char *mac,
					  u32 *port_id);

/**
 * Get Port ID from Port Name
 * @param dev_id The Device ID
 * @param port_name Port Name
 * @param port_id Port ID
 * @return Status of the API call.
 */
bf_status_t port_mgr_get_port_id_from_name(bf_dev_id_t dev_id, char *port_name,
					      u32 *port_id);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // PORT_MGR_PORT_H_INCLUDED

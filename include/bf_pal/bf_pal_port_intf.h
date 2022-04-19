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
#ifndef _BF_PAL_PORT_INTF_H
#define _BF_PAL_PORT_INTF_H

#include <bf_types/bf_types.h>
#include <port_mgr/bf_port_if.h>
#include <osdep/p4_sde_osdep.h>

/**
 * @brief Port add function
 * @param dev_id Device id
 * @param dev_port Device port number
 * @param speed Enum type describing the speed of the port
 * @param fec_type Enum type describing the FEC type of the port
 * @return Status of the API call
 */
bf_status_t bf_pal_port_add(bf_dev_id_t dev_id,
			    bf_dev_port_t dev_port,
			    struct port_attributes_t *port_attrib);

/**
 * @brief Port delete function
 * @param dev_id Device id
 * @param dev_port Device port number
 * @return Status of the API call
 */
bf_status_t bf_pal_port_del(bf_dev_id_t dev_id, bf_dev_port_t dev_port);

/**
 * @brief Get all the stats of a port (User must ensure that that sufficient
 * space fot stats array has been allocated
 * @param dev_id Device id
 * @param dev_port Device port number
 * @param stats Array to hold all the stats read from hardware
 * @return Status of the API call
 */
bf_status_t bf_pal_port_all_stats_get(bf_dev_id_t dev_id,
				      bf_dev_port_t dev_port,
				      u64 *stats);

/**
 * @brief Get Port ID from MAC
 * @param dev_id Device id
 * @param mac MAC Address
 * @param port_id Port ID
 * @return Status of the API call
 */
bf_status_t bf_pal_get_port_id_from_mac(bf_dev_id_t dev_id, char *mac,
					u32 *port_id);

/**
 * @brief Get Port ID from Port Name
 * @param dev_id Device id
 * @param port_name Port Name
 * @param port_id Port ID
 * @return Status of the API call
 */
bf_status_t bf_pal_get_port_id_from_name(bf_dev_id_t dev_id, char *port_name,
					 u32 *port_id);

/**
 * @brief Get the dev port number
 * @param dev_id Device id
 * @param port_str Port str, length max:MAX_PORT_HDL_STRING_LEN
 * @param dev_port Corresponding dev port
 * @return Status of the API call
 */
bf_status_t bf_pal_port_str_to_dev_port_map(bf_dev_id_t dev_id,
					    char *port_str,
					    bf_dev_port_t *dev_port);

/**
 * @brief Get the dev port number
 * @param dev_id Device id
 * @param dev_port Corresponding dev port
 * @param port_str Port str, length required:MAX_PORT_HDL_STRING_LEN
 * @return Status of the API call
 */
bf_status_t bf_pal_dev_port_to_port_str_map(bf_dev_id_t dev_id,
					    bf_dev_port_t dev_port,
					    char *port_str);
/**
 * @brief Create port/chnl-port mapping during bf-switchd init
 * @param dev_id Device id
 * @return Status of the API call
 */
bf_status_t bf_pal_create_port_info(bf_dev_id_t dev_id);

/**
 * @brief Get Port Info from Port ID
 * @param dev_id Device id
 * @param dev_port Port id
 * @param port_info Port Information
 * @return Status of the API call
 */
bf_status_t bf_pal_port_info_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port,
				 struct port_info_t **port_info);
#endif

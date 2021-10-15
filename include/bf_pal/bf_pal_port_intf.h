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
			    port_attributes_t *port_attrib);

/**
 * @brief Port delete function
 * @param dev_id Device id
 * @param dev_port Device port number
 * @return Status of the API call
 */
bf_status_t bf_pal_port_del(bf_dev_id_t dev_id, bf_dev_port_t dev_port);

/**
 * @brief Get a particular stat of a port
 * @param dev_id Device id
 * @param dev_port Device port number
 * @param ctr_type Enum type to hold the id of the stats counter
 * @param stat_val Counter value
 * @return Status of the API call
 */
bf_status_t bf_pal_port_this_stat_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint64_t *stat_val);

/**
 * @brief Clear a particular stats counter for a particular port
 * @param dev_id Device id
 * @param dev_port Device port number
 * @param ctr_type Enum type to hold the id of the stats counter
 * @return Status of the API call
 */
bf_status_t bf_pal_port_this_stat_clear(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port);
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
#endif

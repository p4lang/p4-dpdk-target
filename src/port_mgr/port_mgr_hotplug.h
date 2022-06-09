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

#ifndef PORT_MGR_HOTPLUG_H_INCLUDED
#define PORT_MGR_HOTPLUG_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <bf_types/bf_types.h>
#include <port_mgr/bf_port_if.h>

/**
 * @brief Hotplug add function
 * @param dev_id Device id
 * @param dev_port Device port number
 * @param hotplug_attrib Hotplug attributes
 * @return Status of the API call
 */
bf_status_t port_mgr_hotplug_add(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 struct hotplug_attributes_t *hotplug_attrib);

/**
 * @brief Hotplug del function
 * @param dev_id Device id
 * @param dev_port Device port number
 * @param hotplug_attrib Hotplug attributes
 * @return Status of the API call
 */
bf_status_t port_mgr_hotplug_del(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 struct hotplug_attributes_t *hotplug_attrib);
#ifdef __cplusplus
}
#endif /* C++ */

#endif

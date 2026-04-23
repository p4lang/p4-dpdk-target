/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORT_MGR_HOTPLUG_H_INCLUDED
#define PORT_MGR_HOTPLUG_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <bf_types/bf_types.h>
#include <bf_pal/bf_pal_port_intf.h>

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

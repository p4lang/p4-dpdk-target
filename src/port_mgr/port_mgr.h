/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef port_mgr_h_included
#define port_mgr_h_included

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/* P4 SDE Headers */
#include <osdep/p4_sde_osdep_utils.h>
#include <osdep/p4_sde_osdep.h>

// top-level logical abstraction(s)
#include <port_mgr/bf_port_if.h>
#include "port_mgr/port_mgr_log.h"
#include "port_mgr/port_mgr_dev.h"
#include "port_mgr/port_mgr_intf.h"

/**
 * Global context for port_mgr service
 */
struct port_mgr_ctx_t {
	/* Maps a bf_dev_port_t to a struct port_info_t.
	 * port_info_t contains port information for the
	 * target device
	 */
	p4_sde_map port_info_map;         /*!< Port Info Hash Map */
        /* Maps port name string hash
	 * to port id 
         */
	p4_sde_map port_name_map;         /*!< Port Name Hash to ID Map */
};

/**
 * Platform specific port initialization
 *
 * @param void
 * @return void
 */
void port_mgr_platform_init(void);

/**
 * Platform specific port cleanup
 *
 * @param void
 * @return void
 */
void port_mgr_platform_cleanup(void);

/**
 * Get the Global Port Manager Context
 * @param void
 * @return port_mgr_ctx_t Port Manager Context
 */
struct port_mgr_ctx_t *get_port_mgr_ctx(void);

/**
 * Cleanup the members of Global Port Manager Context
 * @param ctx Port Manager Context
 * @return void
 */
void port_mgr_ctx_cleanup(struct port_mgr_ctx_t *ctx);

/**
 * Add entry to Port Info Hash Map
 * @param dev_port Port ID
 * @param port_attrib Port Attributes
 * @return Status of the API Call
 */
int port_mgr_set_port_info(bf_dev_port_t dev_port,
			   struct port_attributes_t *port_attrib);

/**
 * Remove entry from Port Info Hash Map
 * @param dev_port Port ID
 * @return Status of the API Call
 */
int port_mgr_remove_port_info(bf_dev_port_t dev_port);

/**
 * Get an entry from Port Info Hash Map
 * @param dev_port Port ID
 * @return port_info_t Port Info Structure
 */
struct port_info_t *port_mgr_get_port_info(bf_dev_port_t dev_port);

/**
 * Get an entry from Port Name Hash Map
 * @param port_name Port Name
 * @return Port ID
 */
u32 *port_mgr_get_port_from_name(char *port_name);

/**
 * Add entry to Port Name Hash Map
 * @param port_name Port Name
 * @param dev_port Port ID
 * @return Status of the API Call
 */
int port_mgr_add_name(char *port_name, u32 dev_port);

/**
 * Remove entry from Port Name Hash Map
 * @param port_name Port Name
 * @return Status of the API Call
 */
int port_mgr_remove_name(char *port_name);

#ifdef __cplusplus
}
#endif /* C++ */

#endif

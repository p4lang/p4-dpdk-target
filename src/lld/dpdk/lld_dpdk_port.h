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
 * @file lld_dpdk_port.h
 *
 *
 * Details DPDK Port-level structures and APIs
 */

#ifndef _LLD_DPDK_PORT_H
#define _LLD_DPDK_PORT_H

#include <osdep/p4_sde_osdep.h>
#include "port_mgr/bf_port_if.h"
#include <rte_swx_port_fd.h>
#include <rte_swx_port_ethdev.h>
#include <rte_swx_port_source_sink.h>
#include <rte_swx_port_ring.h>
#include "infra/dpdk_infra.h"
#include "infra/dpdk_cli.h"

#define BUFF_SIZE 1024

/**
 * Create a DPDK Tap Port
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @return Status of the API call
 */

int lld_dpdk_tap_port_create(struct port_attributes_t *port_attrib);

/**
 * Add a Tap Port to DPDK Pipeline in Both Directions
 * @param dev_port The Port ID
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @param pipe DPDK Pipeline
 * @param mp DPDK Mempool
 * @return Status of the API call
 */

int lld_dpdk_pipeline_tap_port_add(bf_dev_port_t dev_port,
				   struct port_attributes_t *port_attrib,
				   struct pipeline *pipe_in,
				   struct pipeline *pipe_out,
				   struct mempool *mp);

/**
 * Add a New DPDK Tap Port
 * @param dev_port The Port ID
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @param pipe DPDK Pipeline
 * @param mp DPDK Mempool
 * @return Status of the API call
 */

int lld_dpdk_tap_port_add(bf_dev_port_t dev_port,
			  struct port_attributes_t *port_attrib,
			  struct pipeline *pipe_in,
			  struct pipeline *pipe_out,
			  struct mempool *mp);

/**
 * Create a DPDK Link Port
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @return Status of the API call
 */

int lld_dpdk_link_port_create(struct port_attributes_t *port_attrib);

/**
 * Add a Link Port to DPDK Pipeline in Both Directions
 * @param dev_port The Port ID
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @param pipe DPDK Pipeline
 * @return Status of the API call
 */

int lld_dpdk_pipeline_link_port_add(bf_dev_port_t dev_port,
				    struct port_attributes_t *port_attrib,
				    struct pipeline *pipe_in,
				    struct pipeline *pipe_out);

/**
 * Add a New DPDK Link Port
 * @param dev_port The Port ID
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @param pipe DPDK Pipeline
 * @return Status of the API call
 */

int lld_dpdk_link_port_add(bf_dev_port_t dev_port,
			   struct port_attributes_t *port_attrib,
			   struct pipeline *pipe_in,
			   struct pipeline *pipe_out);

/**
 * Add a New DPDK Source Port
 * @param dev_port The Port ID
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @param pipe DPDK Pipeline
 * @param mp DPDK Mempool
 * @return Status of the API call
 */

int lld_dpdk_source_port_add(bf_dev_port_t dev_port,
			     struct port_attributes_t *port_attrib,
			     struct pipeline *pipe_in,
			     struct mempool *mp);

/**
 * Add a New DPDK Sink Port
 * @param dev_port The Port ID
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @param pipe DPDK Pipeline
 * @return Status of the API call
 */

int lld_dpdk_sink_port_add(bf_dev_port_t dev_port,
			   struct port_attributes_t *port_attrib,
			   struct pipeline *pipe_out);

/**
 * Create a DPDK Ring Port
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @return Status of the API call
 */

int lld_dpdk_ring_port_create(struct port_attributes_t *port_attrib);

/**
 * Add a Ring Port to DPDK Pipeline in Both Directions
 * @param dev_port The Port ID
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @param pipe DPDK Pipeline
 * @return Status of the API call
 */

int lld_dpdk_pipeline_ring_port_add(bf_dev_port_t dev_port,
                                    struct port_attributes_t *port_attrib,
                                    struct pipeline *pipe_in,
                                    struct pipeline *pipe_out);

/**
 * Add a New DPDK Ring Port
 * @param dev_port The Port ID
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @param pipe DPDK Pipeline
 * @return Status of the API call
 */

int lld_dpdk_ring_port_add(bf_dev_port_t dev_port,
			   struct port_attributes_t *port_attrib,
			   struct pipeline *pipe_in,
			   struct pipeline *pipe_out);

/**
 * Get All Port Statistics for DPDK Port
 * @param dev_port The Port ID
 * @param port_attrib The Port Attributes
 * @param stats Array to hold all the stats read from DPDK Target
 * @return Status of the API call
 */

int lld_dpdk_port_stats_get(bf_dev_port_t dev_port,
			    struct port_attributes_t *port_attrib,
			    u64 *stats);

/**
 * Add a New DPDK Port
 * @param dev_port The Port ID
 * @param port_attrib The Port Attributes Eg. Port Name, Port Type etc
 * @return Status of the API call
 */

int lld_dpdk_port_add(bf_dev_port_t dev_port,
		      struct port_attributes_t *port_attrib);

#endif

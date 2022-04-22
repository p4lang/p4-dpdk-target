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

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#ifdef RTE_EXEC_ENV_LINUX
#include <linux/if.h>
#include <linux/if_tun.h>
#endif
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_swx_pipeline.h>
#include <rte_swx_ctl.h>

#include "dpdk_infra.h"

/*
 * mempool
 */
TAILQ_HEAD(mempool_list, mempool);

/*
 * link
 */
TAILQ_HEAD(link_list, link);

/*
 * ring
 */
TAILQ_HEAD(ring_list, ring);

/*
 * tap
 */
TAILQ_HEAD(tap_list, tap);

/*
 * pipeline
 */
TAILQ_HEAD(pipeline_list, pipeline);

/*
 * obj
 */
struct obj {
	struct mempool_list mempool_list;
	struct link_list link_list;
	struct ring_list ring_list;
	struct pipeline_list pipeline_list;
	struct tap_list tap_list;
};

struct obj *obj;
/*
 * mempool
 */
#define BUFFER_SIZE_MIN (sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

struct mempool *
mempool_create(const char *name, struct mempool_params *params)
{
	struct mempool *mempool;
	struct rte_mempool *m;

	/* Check input params */
	if ((name == NULL) ||
		mempool_find(name) ||
		(params == NULL) ||
		(params->buffer_size < BUFFER_SIZE_MIN) ||
		(params->pool_size == 0))
		return NULL;

	/* Resource create */
	m = rte_pktmbuf_pool_create(
		name,
		params->pool_size,
		params->cache_size,
		0,
		params->buffer_size - sizeof(struct rte_mbuf),
		params->cpu_id);

	if (m == NULL)
		return NULL;

	/* Node allocation */
	mempool = calloc(1, sizeof(struct mempool));
	if (mempool == NULL) {
		rte_mempool_free(m);
		return NULL;
	}

	/* Node fill in */
	strlcpy(mempool->name, name, sizeof(mempool->name));
	mempool->m = m;
	mempool->buffer_size = params->buffer_size;

	/* Node add to list */
	TAILQ_INSERT_TAIL(&obj->mempool_list, mempool, node);

	return mempool;
}

struct mempool *
mempool_find(const char *name)
{
	struct mempool *mempool;

	if (!name)
		return NULL;

	TAILQ_FOREACH(mempool, &obj->mempool_list, node)
		if (strcmp(mempool->name, name) == 0)
			return mempool;

	return NULL;
}

/*
 * link
 */
static struct rte_eth_conf port_conf_default = {
	.link_speeds = 0,
	.rxmode = {
		.mq_mode = RTE_ETH_MQ_RX_NONE,
		.mtu = 9000 - (RTE_ETHER_HDR_LEN + RTE_ETHER_CRC_LEN), /* Jumbo frame MTU */
		.split_hdr_size = 0, /* Header split buffer size */
	},
	.rx_adv_conf = {
		.rss_conf = {
			.rss_key = NULL,
			.rss_key_len = 40,
			.rss_hf = 0,
		},
	},
	.txmode = {
		.mq_mode = RTE_ETH_MQ_TX_NONE,
	},
	.lpbk_mode = 0,
};

#define RETA_CONF_SIZE     (RTE_ETH_RSS_RETA_SIZE_512 / RTE_ETH_RETA_GROUP_SIZE)

static int
rss_setup(uint16_t port_id,
	uint16_t reta_size,
	struct link_params_rss *rss)
{
	struct rte_eth_rss_reta_entry64 reta_conf[RETA_CONF_SIZE];
	uint32_t i;
	int status;

	/* RETA setting */
	memset(reta_conf, 0, sizeof(reta_conf));

	for (i = 0; i < reta_size; i++)
		reta_conf[i / RTE_ETH_RETA_GROUP_SIZE].mask = UINT64_MAX;

	for (i = 0; i < reta_size; i++) {
		uint32_t reta_id = i / RTE_ETH_RETA_GROUP_SIZE;
		uint32_t reta_pos = i % RTE_ETH_RETA_GROUP_SIZE;
		uint32_t rss_qs_pos = i % rss->n_queues;

		reta_conf[reta_id].reta[reta_pos] =
			(uint16_t) rss->queue_id[rss_qs_pos];
	}

	/* RETA update */
	status = rte_eth_dev_rss_reta_update(port_id,
		reta_conf,
		reta_size);

	return status;
}

struct link *
link_create(const char *name, struct link_params *params)
{
	struct rte_eth_dev_info port_info;
	struct rte_eth_conf port_conf;
	struct link *link;
	struct link_params_rss *rss;
	struct mempool *mempool;
	uint32_t cpu_id, i;
	int status;
	uint16_t port_id;

	/* Check input params */
	if ((name == NULL) ||
		link_find(name) ||
		(params == NULL) ||
		(params->rx.n_queues == 0) ||
		(params->rx.queue_size == 0) ||
		(params->tx.n_queues == 0) ||
		(params->tx.queue_size == 0))
		return NULL;

	printf("LINK CREATE: Dev:%s Args:%s dev_hotplug_enabled: %d\n",
		params->dev_name, params->dev_args, params->dev_hotplug_enabled);

	/* Performing Device Hotplug and valid for only VDEVs */
	if (params->dev_hotplug_enabled) {
		if (rte_eal_hotplug_add("vdev", params->dev_name,
					 params->dev_args)) {
			printf("LINK CREATE: Dev:%s probing failed\n",
				params->dev_name);
			return NULL;
		}
		printf("LINK CREATE: Dev:%s probing successful\n",
			params->dev_name);
	}

	port_id = params->port_id;
	if (params->dev_name) {
		status = rte_eth_dev_get_port_by_name(params->dev_name,
			&port_id);

		if (status)
			return NULL;
	} else
		if (!rte_eth_dev_is_valid_port(port_id))
			return NULL;

	if (rte_eth_dev_info_get(port_id, &port_info) != 0)
		return NULL;

	mempool = mempool_find(params->rx.mempool_name);
	if (mempool == NULL)
		return NULL;

	rss = params->rx.rss;
	if (rss) {
		if ((port_info.reta_size == 0) ||
			(port_info.reta_size > RTE_ETH_RSS_RETA_SIZE_512))
			return NULL;

		if ((rss->n_queues == 0) ||
			(rss->n_queues >= LINK_RXQ_RSS_MAX))
			return NULL;

		for (i = 0; i < rss->n_queues; i++)
			if (rss->queue_id[i] >= port_info.max_rx_queues)
				return NULL;
	}

	/**
	 * Resource create
	 */
	/* Port */
	memcpy(&port_conf, &port_conf_default, sizeof(port_conf));
	if (rss) {
		port_conf.rxmode.mq_mode = RTE_ETH_MQ_RX_RSS;
		port_conf.rx_adv_conf.rss_conf.rss_hf =
			(RTE_ETH_RSS_IP | RTE_ETH_RSS_TCP | RTE_ETH_RSS_UDP) &
			port_info.flow_type_rss_offloads;
	}

	cpu_id = (uint32_t) rte_eth_dev_socket_id(port_id);
	if (cpu_id == (uint32_t) SOCKET_ID_ANY)
		cpu_id = 0;

	status = rte_eth_dev_configure(
		port_id,
		params->rx.n_queues,
		params->tx.n_queues,
		&port_conf);

	if (status < 0)
		return NULL;

	if (params->promiscuous) {
		status = rte_eth_promiscuous_enable(port_id);
		if (status != 0)
			return NULL;
	}

	/* Port RX */
	for (i = 0; i < params->rx.n_queues; i++) {
		status = rte_eth_rx_queue_setup(
			port_id,
			i,
			params->rx.queue_size,
			cpu_id,
			NULL,
			mempool->m);

		if (status < 0)
			return NULL;
	}

	/* Port TX */
	for (i = 0; i < params->tx.n_queues; i++) {
		status = rte_eth_tx_queue_setup(
			port_id,
			i,
			params->tx.queue_size,
			cpu_id,
			NULL);

		if (status < 0)
			return NULL;
	}

	/* Port start */
	status = rte_eth_dev_start(port_id);
	if (status < 0)
		return NULL;

	if (rss) {
		status = rss_setup(port_id, port_info.reta_size, rss);

		if (status) {
			rte_eth_dev_stop(port_id);
			return NULL;
		}
	}

	/* Port link up */
	status = rte_eth_dev_set_link_up(port_id);
	if ((status < 0) && (status != -ENOTSUP)) {
		rte_eth_dev_stop(port_id);
		return NULL;
	}

	/* Node allocation */
	link = calloc(1, sizeof(struct link));
	if (link == NULL) {
		rte_eth_dev_stop(port_id);
		return NULL;
	}

	/* Node fill in */
	strlcpy(link->name, name, sizeof(link->name));
	link->port_id = port_id;
	rte_eth_dev_get_name_by_port(port_id, link->dev_name);
	link->n_rxq = params->rx.n_queues;
	link->n_txq = params->tx.n_queues;

	/* Node add to list */
	TAILQ_INSERT_TAIL(&obj->link_list, link, node);

	return link;
}

int
link_is_up(const char *name)
{
	struct rte_eth_link link_params;
	struct link *link;

	/* Check input params */
	if (!name)
		return 0;

	link = link_find(name);
	if (link == NULL)
		return 0;

	/* Resource */
	if (rte_eth_link_get(link->port_id, &link_params) < 0)
		return 0;

	return (link_params.link_status == RTE_ETH_LINK_DOWN) ? 0 : 1;
}

struct link *
link_find(const char *name)
{
	struct link *link;

	if (!name)
		return NULL;

	TAILQ_FOREACH(link, &obj->link_list, node)
		if (strcmp(link->name, name) == 0)
			return link;

	return NULL;
}

struct link *
link_next(struct link *link)
{
	return (link == NULL) ?
		TAILQ_FIRST(&obj->link_list) : TAILQ_NEXT(link, node);
}

/*
 * ring
 */
struct ring *
ring_create(const char *name, struct ring_params *params)
{
	struct ring *ring;
	struct rte_ring *r;
	unsigned int flags = RING_F_SP_ENQ | RING_F_SC_DEQ;

	/* Check input params */
	if (!name || ring_find(name) || !params || !params->size)
		return NULL;

	/**
	 * Resource create
	 */
	r = rte_ring_create(
		name,
		params->size,
		params->numa_node,
		flags);
	if (!r)
		return NULL;

	/* Node allocation */
	ring = calloc(1, sizeof(struct ring));
	if (!ring) {
		rte_ring_free(r);
		return NULL;
	}

	/* Node fill in */
	strlcpy(ring->name, name, sizeof(ring->name));

	/* Node add to list */
	TAILQ_INSERT_TAIL(&obj->ring_list, ring, node);

	return ring;
}

struct ring *
ring_find(const char *name)
{
	struct ring *ring;

	if (!name)
		return NULL;

	TAILQ_FOREACH(ring, &obj->ring_list, node)
		if (strcmp(ring->name, name) == 0)
			return ring;

	return NULL;
}

/*
 * tap
 */
#define TAP_DEV		"/dev/net/tun"

struct tap *
tap_find(const char *name)
{
	struct tap *tap;

	if (!name)
		return NULL;

	TAILQ_FOREACH(tap, &obj->tap_list, node)
		if (strcmp(tap->name, name) == 0)
			return tap;

	return NULL;
}

struct tap *
tap_next(struct tap *tap)
{
	return (tap == NULL) ?
		TAILQ_FIRST(&obj->tap_list) : TAILQ_NEXT(tap, node);
}

#ifndef RTE_EXEC_ENV_LINUX

struct tap *
tap_create(struct obj *obj __rte_unused, const char *name __rte_unused)
{
	return NULL;
}

#else

struct tap *
tap_create(const char *name)
{
	struct tap *tap;
	struct ifreq ifr;
	int fd, status;

	/* Check input params */
	if ((name == NULL) ||
		tap_find(name))
		return NULL;

	/* Resource create */
	fd = open(TAP_DEV, O_RDWR | O_NONBLOCK);
	if (fd < 0)
		return NULL;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI; /* No packet information */
	strlcpy(ifr.ifr_name, name, IFNAMSIZ);

	status = ioctl(fd, TUNSETIFF, (void *) &ifr);
	if (status < 0) {
		close(fd);
		return NULL;
	}

	/* Node allocation */
	tap = calloc(1, sizeof(struct tap));
	if (tap == NULL) {
		close(fd);
		return NULL;
	}
	/* Node fill in */
	strlcpy(tap->name, name, sizeof(tap->name));
	tap->fd = fd;

	/* Node add to list */
	TAILQ_INSERT_TAIL(&obj->tap_list, tap, node);

	return tap;
}

#endif

/*
 * pipeline
 */
#ifndef PIPELINE_MSGQ_SIZE
#define PIPELINE_MSGQ_SIZE                                 64
#endif

struct pipeline *
pipeline_create(const char *name, int numa_node)
{
	struct pipeline *pipeline;
	struct rte_swx_pipeline *p = NULL;
	int status;

	/* Check input params */
	if ((name == NULL) ||
		pipeline_find(name))
		return NULL;

	/* Resource create */
	status = rte_swx_pipeline_config(&p, numa_node);
	if (status)
		goto error;

	/* Node allocation */
	pipeline = calloc(1, sizeof(struct pipeline));
	if (pipeline == NULL)
		goto error;

	/* Node fill in */
	strlcpy(pipeline->name, name, sizeof(pipeline->name));
	pipeline->p = p;
	pipeline->timer_period_ms = 10;

	/* Node add to list */
	TAILQ_INSERT_TAIL(&obj->pipeline_list, pipeline, node);

	return pipeline;

error:
	rte_swx_pipeline_free(p);
	return NULL;
}

struct pipeline *
pipeline_find(const char *name)
{
	struct pipeline *pipeline;

	if (!name)
		return NULL;

	TAILQ_FOREACH(pipeline, &obj->pipeline_list, node)
		if (strcmp(name, pipeline->name) == 0)
			return pipeline;

	return NULL;
}

/**
 * Validate the number of ports added to the
 * pipeline in input and output directions
 */
int
pipeline_port_is_valid(struct pipeline *pipe)
{
	struct rte_swx_ctl_pipeline_info pipe_info = {0};

	if (rte_swx_ctl_pipeline_info_get(pipe->p, &pipe_info) < 0) {
		printf("%s failed at %d for pipeinfo \n",__func__, __LINE__);
        return 0;
    }

	if (!pipe_info.n_ports_in || !(rte_is_power_of_2(pipe_info.n_ports_in)))
		return 0;

	if (!pipe_info.n_ports_out)
		return 0;

	return 1;
}

/*
 * obj
 */
int
obj_init(void)
{
	obj = calloc(1, sizeof(struct obj));
	if (!obj)
		return -1;

	TAILQ_INIT(&obj->mempool_list);
	TAILQ_INIT(&obj->link_list);
	TAILQ_INIT(&obj->ring_list);
	TAILQ_INIT(&obj->pipeline_list);
	TAILQ_INIT(&obj->tap_list);

	return 0;
}

void table_entry_free(struct rte_swx_table_entry *entry)
{
	if (!entry)
		return;

	free(entry->key);
	free(entry->key_mask);
	free(entry->action_data);
	free(entry);
}

uint64_t
get_action_id(struct pipeline *pipe, const char *action_name)
{
	uint64_t i;
	int ret;
	struct rte_swx_ctl_action_info action;
	struct rte_swx_ctl_pipeline_info pipe_info = {0};

	if (action_name == NULL || pipe == NULL || pipe->p == NULL) {
		printf("%s failed at %d\n",__func__, __LINE__);
		goto action_error;
	}
	ret = rte_swx_ctl_pipeline_info_get(pipe->p, &pipe_info);
	if (ret < 0) {
		printf("%s failed at %d for pipeinfo \n",__func__, __LINE__);
		goto action_error;
	}
	for (i = 0; i < pipe_info.n_actions; i++) {
		memset(&action, 0, sizeof(action));
		ret = rte_swx_ctl_action_info_get (pipe->p, i, &action);
		if (ret < 0) {
			printf("%s failed at %d for actioninfo\n",
				__func__, __LINE__);
			break;
		}
		if (!strncmp(action_name, action.name, RTE_SWX_CTL_NAME_SIZE))
			return i;
	}
action_error:
	printf("%s failed at %d end\n",__func__, __LINE__);
	return UINT64_MAX;
}

uint32_t
get_table_id(struct pipeline *pipe, const char *table_name)
{
	uint32_t i;
	int ret;
	struct rte_swx_ctl_table_info table;
	struct rte_swx_ctl_pipeline_info pipe_info = {0};

	if (table_name == NULL || pipe == NULL || pipe->p == NULL) {
		printf("%s failed at %d\n",__func__, __LINE__);
		goto table_error;
	}

	ret = rte_swx_ctl_pipeline_info_get(pipe->p, &pipe_info);
	if (ret < 0) {
		printf("%s failed at %d for pipeinfo\n",__func__, __LINE__);
		goto table_error;
	}
	for (i = 0; i < pipe_info.n_tables; i++) {
		memset(&table, 0, sizeof(table));
		ret = rte_swx_ctl_table_info_get (pipe->p, i, &table);
		if (ret < 0) {
			printf("%s failed at %d for tableinfo\n",
				__func__, __LINE__);
			break;
		}
		if (!strncmp(table_name, table.name, RTE_SWX_CTL_NAME_SIZE))
			return i;
	}
table_error:
	printf("%s failed at %d end\n",__func__, __LINE__);
	return UINT32_MAX;
}


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

#ifndef _INCLUDE_DPDK_INFRA_H_
#define _INCLUDE_DPDK_INFRA_H_

#include <stdint.h>
#include <sys/queue.h>
#include <endian.h>

#include <rte_mempool.h>
#include <rte_swx_pipeline.h>
#include <rte_byteorder.h>
#include <rte_swx_ctl.h>

#ifndef NAME_SIZE
#define NAME_SIZE 64
#endif

/*
 * obj
 */

#define ntoh64(x) rte_be_to_cpu_64(x)
#define hton64(x) rte_cpu_to_be_64(x)

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define dpdk_field_ntoh(val, n_bits) (ntoh64((val) << (64 - n_bits)))
#define dpdk_field_hton(val, n_bits) (hton64((val) << (64 - n_bits)))
#else
#define dpdk_field_ntoh(val, n_bits) (val)
#define dpdk_field_hton(val, n_bits) (val)
#endif


struct obj;

int
obj_init(void);

extern struct obj *obj;

/*
 * mempool
 */
struct mempool_params {
	uint32_t buffer_size;
	uint32_t pool_size;
	uint32_t cache_size;
	uint32_t cpu_id;
};

struct mempool {
	TAILQ_ENTRY(mempool) node;
	char name[NAME_SIZE];
	struct rte_mempool *m;
	uint32_t buffer_size;
};

struct mempool *
mempool_create(const char *name,
	       struct mempool_params *params);

struct mempool *
mempool_find(const char *name);

/*
 * link
 */
#ifndef LINK_RXQ_RSS_MAX
#define LINK_RXQ_RSS_MAX                                   16
#endif

struct link_params_rss {
	uint32_t queue_id[LINK_RXQ_RSS_MAX];
	uint32_t n_queues;
};

struct link_params {
	const char *dev_name;
	const char *dev_args;
	uint16_t port_id; /**< Valid only when *dev_name* is NULL. */
	uint16_t dev_hotplug_enabled;

	struct {
		uint32_t n_queues;
		uint32_t queue_size;
		const char *mempool_name;
		struct link_params_rss *rss;
	} rx;

	struct {
		uint32_t n_queues;
		uint32_t queue_size;
	} tx;

	int promiscuous;
};

struct link {
	TAILQ_ENTRY(link) node;
	char name[NAME_SIZE];
	char dev_name[NAME_SIZE];
	uint16_t port_id;
	uint32_t n_rxq;
	uint32_t n_txq;
};

struct link *
link_create(const char *name,
	    struct link_params *params);

int
link_is_up(const char *name);

struct link *
link_find(const char *name);

struct link *
link_next(struct link *link);

/*
 * ring
 */
struct ring_params {
	uint32_t size;
	uint32_t numa_node;
};

struct ring {
	TAILQ_ENTRY(ring) node;
	char name[NAME_SIZE];
};

struct ring *
ring_create(const char *name,
	   struct ring_params *params);

struct ring *
ring_find(const char *name);

/*
 * tap
 */
struct tap {
	TAILQ_ENTRY(tap) node;
	char name[NAME_SIZE];
	int fd;
};

struct tap *
tap_find(const char *name);

struct tap *
tap_next(struct tap *tap);

struct tap *
tap_create(const char *name);

/*
 * pipeline
 */
struct pipeline {
	TAILQ_ENTRY(pipeline) node;
	char name[NAME_SIZE];

	struct rte_swx_pipeline *p;
	struct rte_swx_ctl_pipeline *ctl;

	uint32_t timer_period_ms;
	int enabled;
	uint32_t thread_id;
	uint32_t cpu_id;
};

struct pipeline *
pipeline_create(const char *name,
		int numa_node);

struct pipeline *
pipeline_find(const char *name);

/* Infra related functions */
int
dpdk_infra_init(int count, char **arr);


/* Thread related functions */
int
thread_pipeline_enable(uint32_t thread_id,
	const char *pipeline_name);

int
thread_pipeline_disable(uint32_t thread_id,
	const char *pipeline_name);

int
thread_init(void);

int
thread_main(void *arg);

void table_entry_free(struct rte_swx_table_entry *entry);
uint64_t
get_action_id(struct pipeline *pipe, const char *action_name);
uint32_t
get_table_id(struct pipeline *pipe, const char *table_name);
#endif /* _INCLUDE_DPDK_INFRA_H_ */

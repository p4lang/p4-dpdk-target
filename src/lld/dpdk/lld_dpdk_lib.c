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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lld_dpdk_lib.h"
#include "../lldlib_log.h"
#include <dvm/bf_drv_profile.h>

#include <rte_swx_pipeline.h>
#include <rte_swx_port_fd.h>
#include <rte_swx_port_ethdev.h>

#include "infra/dpdk_infra.h"
#include "infra/dpdk_cli.h"

#define MAX_EAL_ARGS 64
#define DEFAULT_NUMA_NODE 0

static char *dpdk_args[] = {"dummy", "-n", "4", "-c", "3"};
static char *dpdk_arr[MAX_EAL_ARGS];

static size_t parse(char *arr)
{
	int idx = 0;
	char *ptr, *ptr_tok;
	char eal_param[MAX_EAL_LEN] = {0};

	strncpy(eal_param, arr, MAX_EAL_LEN - 1);
	LOG_TRACE("%s EAL args are %s", __func__, eal_param);
	ptr = eal_param;
	ptr_tok = strtok_r(ptr, " ", &ptr);
	while (ptr_tok && idx < MAX_EAL_ARGS) {
		free(dpdk_arr[idx]);
		dpdk_arr[idx] = strdup(ptr_tok);
		if (!dpdk_arr[idx]) {
			LOG_TRACE("%s Memory alloc failed", __func__);
			while (idx) {
				free(dpdk_arr[idx]);
				dpdk_arr[idx] = NULL;
				idx--;
			}
			break;
		}
		ptr_tok = strtok_r(ptr, " ", &ptr);
		idx++;
	}
	return idx;
}

static int
lld_dpdk_pipeline_mirror_config(struct rte_swx_pipeline *p, void *mir_cfg)
{
	int rc = BF_SUCCESS;
	struct rte_swx_pipeline_mirroring_params mir_params;

	mir_params.n_slots = ((struct rte_swx_pipeline_mirroring_params *)mir_cfg)->n_slots;
	mir_params.n_sessions = ((struct rte_swx_pipeline_mirroring_params *)mir_cfg)->n_sessions;

	if (rte_swx_pipeline_mirroring_config(p, &mir_params)) {
		LOG_ERROR("Could not configure the pipeline mirror params.");
		rc = BF_UNEXPECTED;
	}

	return rc;
}

int lld_dpdk_init(bf_device_profile_t *profile)
{
	size_t arr_size = sizeof(dpdk_args) / sizeof(*dpdk_args);
	size_t parse_size = 0;
	char *pipeline_name = NULL;
	char *eal_args = NULL;
	struct mempool *mempool = NULL;
	struct mempool_params mempool_p;
	bf_p4_program_t *p4_program = NULL;
	bf_p4_pipeline_t *p4_pipeline = NULL;
	struct bf_mempool_obj_s *mempool_obj = NULL;
	struct pipeline *pipe = NULL;
	int i, j;

	LOG_TRACE("%s Start...", __func__);

	eal_args = profile->eal_args;

	if (!eal_args) {
		LOG_ERROR(" eal_args is null");
		return BF_UNEXPECTED;
	}

	if (strlen(eal_args)) {
		LOG_TRACE("%s:%d EALargs present", __func__, __LINE__);
		parse_size = parse(eal_args);
		if (parse_size)
			dpdk_infra_init(parse_size, dpdk_arr,
							profile->debug_cli_enable);
	}

	if (parse_size == 0) {
		LOG_TRACE("%s Init with default args", __func__);
		dpdk_infra_init(arr_size, dpdk_args,
						profile->debug_cli_enable);
	}

	for (i = 0; i < profile->num_mempool_objs; i++) {
		mempool_obj = &profile->mempool_objs[i];
		mempool_p.buffer_size = mempool_obj->buffer_size;
		mempool_p.pool_size   = mempool_obj->pool_size;
		mempool_p.cache_size  = mempool_obj->cache_size;
		mempool_p.cpu_id      = mempool_obj->numa_node;
		LOG_TRACE("%s:%d Creating Mempool %s",
			  __func__, __LINE__, mempool_obj->name);

		mempool = mempool_create(mempool_obj->name, &mempool_p);
		if (!mempool) {
			LOG_ERROR("Error in Creating Mempool %s",
				  mempool_obj->name);
			return BF_UNEXPECTED;
		}
	}

	for (i = 0; i < profile->num_p4_programs; i++) {
		p4_program = &(profile->p4_programs[i]);
		for (j = 0; j < p4_program->num_p4_pipelines; j++) {
                        p4_pipeline = &(p4_program->p4_pipelines[j]);
                        pipeline_name = p4_pipeline->p4_pipeline_name;
                        if (!pipeline_name) {
                                LOG_ERROR("Pipeline name is null");
                                return BF_UNEXPECTED;
                        }
                        LOG_TRACE("%s:%d Creating Pipeline %s",
				  __func__, __LINE__, pipeline_name);

			pipe = pipeline_create(pipeline_name,
					p4_pipeline->numa_node);
			if (!pipe) {
				LOG_ERROR("Error in Creating Pipeline %s",
					  pipeline_name);
				return BF_UNEXPECTED;
			}

			if (lld_dpdk_pipeline_mirror_config(pipe->p, &p4_pipeline->mir_cfg)) {
				LOG_ERROR("Error in Setting Mirror Config for pipeline %s",
					  pipeline_name);
				return BF_UNEXPECTED;
			}
		}
	}

	LOG_TRACE("%s End...\n", __func__);
	return 0;
}

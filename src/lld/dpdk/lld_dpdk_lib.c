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
#include <rte_swx_port_fd.h>
#include <rte_swx_port_ethdev.h>

#include "infra/dpdk_infra.h"
#include "infra/dpdk_cli.h"
#define MAX_EAL_ARGS 64
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

int lld_dpdk_init(bf_device_profile_t *profile)
{
	char send_buff[BUFF_SIZE] = {0};
	char recv_buff[BUFF_SIZE] = {0};
	size_t arr_size = sizeof(dpdk_args) / sizeof(*dpdk_args);
	size_t parse_size = 0;
	char *pipeline_name = NULL;
	char *eal_args = NULL;

	LOG_TRACE("%s Start...", __func__);

	/* FIXME: now only one pipeline is created.
	 * revisit when we have mutiple pipeline */
	pipeline_name = profile->p4_programs[0].
		p4_pipelines[0].p4_pipeline_name;

	if (!pipeline_name) {
		LOG_ERROR("pipeline name is null");
		return BF_UNEXPECTED;
	}

	eal_args = profile->eal_args;

	if (!eal_args) {
		LOG_ERROR(" eal_args is null");
		return BF_UNEXPECTED;
	}

	if (strlen(eal_args)) {
		LOG_TRACE("%s:%d EALargs present", __func__, __LINE__);
		parse_size = parse(eal_args);
		if (parse_size)
			dpdk_infra_init(parse_size, dpdk_arr);
	}

	if (parse_size == 0) {
		LOG_TRACE("%s Init with default args", __func__);
		dpdk_infra_init(arr_size, dpdk_args);
	}

	snprintf(send_buff, BUFF_SIZE, "mempool MEMPOOL0 buffer 2304 pool 1K "
		 "cache 256 cpu 0");
	cli_process(send_buff, recv_buff, BUFF_SIZE);
	LOG_TRACE("%s:%d recv_buff is %s", __func__, __LINE__, recv_buff);
	memset(recv_buff, 0, sizeof(recv_buff));
	snprintf(send_buff, BUFF_SIZE, "pipeline %s create 0", pipeline_name);
	cli_process(send_buff, recv_buff, BUFF_SIZE);
	LOG_TRACE("%s:%d recv_buff is %s", __func__, __LINE__, recv_buff);
	memset(recv_buff, 0, sizeof(recv_buff));
	LOG_TRACE("%s End...\n", __func__);
	return 0;
}

void lld_dpdk_table_rule_add(char *pipeline, char *table,
			     enum mat_type action_type,
			     void *request, void *reply)
{
	char send_buff[BUFF_SIZE] = {0};
	char recv_buff[BUFF_SIZE] = {0};

	LOG_TRACE("%s Start...\n", __func__);
	switch (action_type) {
	case VXLAN_ENCAP:
	{
		// Not implemented yet.
		break;
	}
	case SIMPLE_L3:
	{
		struct simple_l3_mav *match_action_fields =
					(struct simple_l3_mav *)request;

		u32 match_key = match_action_fields->match_key;
		u32 port_num = match_action_fields->fwd_port;

		if (port_num != UINT32_MAX_VAL) {
		    // Add the Entry with send to fwd_port action.
		    snprintf(send_buff, BUFF_SIZE,
				"pipeline %s table %s rule add "
				"match 0x%x action send "
				"fwd_port H(%d)\n",
				pipeline, table, match_key, port_num);
		} else {
		    // Add the Entry with drop action.
		    snprintf(send_buff, BUFF_SIZE,
				"pipeline %s table %s rule add "
				"match 0x%x action drop_1\n",
				pipeline, table, match_key);
		}
		break;
	}
	default:
		break;
	}
	cli_process(send_buff, recv_buff, BUFF_SIZE);
	memcpy(reply, recv_buff, BUFF_SIZE);
	LOG_TRACE("%s Exit...\n", __func__);
}

void lld_dpdk_table_rule_delete(char *pipeline, char *table,
				enum mat_type action_type,
				void *request, void *reply)
{
	char send_buff[BUFF_SIZE] = {0};
	char recv_buff[BUFF_SIZE] = {0};

	LOG_TRACE("%s Start...\n", __func__);
	switch (action_type) {
	case VXLAN_ENCAP:
	{
		// Not implemented yet.
		break;
	}
	case SIMPLE_L3:
	{
		struct simple_l3_mav *match_action_fields =
					(struct simple_l3_mav *)request;

		u32 match_key = match_action_fields->match_key;

		// Delete the table entry.
		snprintf(send_buff, BUFF_SIZE,
			    "pipeline %s table %s rule delete "
			    "match 0x%x\n", pipeline, table, match_key);
		break;
	}
	default:
		break;
	}
	cli_process(send_buff, recv_buff, BUFF_SIZE);
	memcpy(reply, recv_buff, BUFF_SIZE);
	LOG_TRACE("%s Exit...\n", __func__);
}

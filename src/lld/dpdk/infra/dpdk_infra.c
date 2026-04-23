/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include <rte_launch.h>
#include <rte_eal.h>

#include "dpdk_cli.h"
#include "dpdk_conn.h"
#include "dpdk_infra.h"

static pthread_t cli_thread_id;

struct conn_params conn_param = {
	.welcome = "\nWelcome!\n\n",
	.prompt = "pipeline> ",
	.addr = "0.0.0.0",
	.port = 8086,
	.buf_size = 1024 * 1024,
	.msg_in_len_max = 1024,
	.msg_out_len_max = 1024 * 1024,
	.msg_handle = cli_process,
	.msg_handle_arg = NULL, /* set later. */
};

void *
dpdk_cli_thread(void *arg)
{
	struct conn *conn;

	/* Connectivity */
	conn = conn_init(&conn_param);
	if (!conn) {
		printf("Error: Connectivity initialization failed\n");
		return NULL;
	};

    /* Dispatch loop */
	for ( ; ; ) {
		conn_poll_for_conn(conn);

		conn_poll_for_msg(conn);
	}

	return NULL;
}

int
dpdk_infra_init(int count, char **arr, bool debug_cli_enable)
{
	int status = 0;

	if (count == 0 || !arr)
		return -1;

	/* EAL */
	status = rte_eal_init(count, arr);
	if (status < 0) {
		printf("Error: EAL initialization failed (%d)\n", status);
		return status;
	};

	/* Obj */
	status = obj_init();
	if (status < 0) {
		printf("Error: Obj initialization failed (%d)\n", status);
		return status;
	}

	/* Thread */
	status = thread_init();
	if (status) {
		printf("Error: Thread initialization failed (%d)\n", status);
		return status;
	}

	rte_eal_mp_remote_launch(
		thread_main,
		NULL,
		SKIP_MAIN);

	if (debug_cli_enable) {
		/* DPDK CLI Thread */
		status = pthread_create(&cli_thread_id, NULL, dpdk_cli_thread,
								NULL);
		if (status) {
			printf("Error: DPDK CLI thread creation failed (%d)\n",
					status);
			return status;
		}

		status = pthread_setname_np(cli_thread_id, "dpdk_cli_thread");
		if (status) {
			printf("Error: Failed to set name for DPDK CLI Thread (%d)\n",
					status);
			return status;
		}
	}
	return status;
}

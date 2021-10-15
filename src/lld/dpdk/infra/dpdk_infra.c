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

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include <rte_launch.h>
#include <rte_eal.h>

#include "dpdk_infra.h"

int
dpdk_infra_init(int count, char **arr)
{
	int status;

	if (count == 0 || arr == NULL)
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
	return 0;
}

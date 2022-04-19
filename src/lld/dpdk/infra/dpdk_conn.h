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

#ifndef __INCLUDE_DPDK_CONN_H__
#define __INCLUDE_DPDK_CONN_H__

#include <stdint.h>

struct conn;

#ifndef CONN_WELCOME_LEN_MAX
#define CONN_WELCOME_LEN_MAX                               1024
#endif

#ifndef CONN_PROMPT_LEN_MAX
#define CONN_PROMPT_LEN_MAX                                16
#endif

typedef void
(*conn_msg_handle_t)(char *msg_in,
		     char *msg_out,
		     size_t msg_out_len_max);

struct conn_params {
	const char *welcome;
	const char *prompt;
	const char *addr;
	uint16_t port;
	size_t buf_size;
	size_t msg_in_len_max;
	size_t msg_out_len_max;
	conn_msg_handle_t msg_handle;
	void *msg_handle_arg;
};

struct conn *
conn_init(struct conn_params *p);

void
conn_free(struct conn *conn);

int
conn_poll_for_conn(struct conn *conn);

int
conn_poll_for_msg(struct conn *conn);

#endif

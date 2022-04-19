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

#ifndef __INCLUDE_DPDK_CLI_H__
#define __INCLUDE_DPDK_CLI_H__

#include <stddef.h>

void
cli_process(char *in, char *out, size_t out_size);

int
cli_script_process(const char *file_name,
	size_t msg_in_len_max,
	size_t msg_out_len_max);

#endif

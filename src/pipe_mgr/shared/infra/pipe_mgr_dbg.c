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
#include "pipe_mgr_dbg.h"

void pipe_mgr_print_match_spec(struct pipe_tbl_match_spec *match_spec)
{
	int i;

	printf("MatchSpec:\n");
	printf("priority = %d\n", (int) match_spec->priority);
	printf("num_valid_match_bits:%d\n", match_spec->num_valid_match_bits);
	printf("num_match_bytes:%d\n", match_spec->num_match_bytes);
	printf("match_value_bits: 0x");
	for (i = 0; i < match_spec->num_match_bytes; i++)
		printf("%02x", match_spec->match_value_bits[i]);

	printf("\nmatch_mask_bits: 0x");
	for (i = 0; i < match_spec->num_match_bytes; i++)
		printf("%02x", match_spec->match_mask_bits[i]);
	printf("\n");
}

void pipe_mgr_print_action_spec(struct pipe_action_spec *action_spec)
{
	int i;

	printf("ActionSpec:\n");
	printf("action_bmap:%02x\n", action_spec->pipe_action_datatype_bmap);
	printf("num_valid_action_data_bits:%d\n",
	       action_spec->act_data.num_valid_action_data_bits);
	printf("num_action_data_bytes:%d\n",
	       action_spec->act_data.num_action_data_bytes);
	printf("action_data: 0x");
	for (i = 0; i < action_spec->act_data.num_action_data_bytes; i++)
		printf("%02x", action_spec->act_data.action_data_bits[i]);
	printf("\n");
}

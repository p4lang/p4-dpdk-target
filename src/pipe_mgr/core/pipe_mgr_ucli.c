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
 * @file pipe_mgr_ucli.c
 *
 * @description ucli utilities for pipe mgr
 */

#include <bf_types/bf_types.h>
#include <osdep/p4_sde_osdep.h>

#include <target-utils/target_utils.h>
#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_node.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>


/* Add new pipe mgr ucli methods here. */
static ucli_command_handler_f pipe_mgr_ucli_handlers__[] = {
	NULL
};

static ucli_module_t pipe_mgr_ucli_mod = {"pipe-mgr-ucli", NULL, pipe_mgr_ucli_handlers__};

ucli_node_t *pipe_mgr_ucli_node_create(void)
{
	ucli_node_t *node;

	ucli_module_init(&pipe_mgr_ucli_mod);
	node = ucli_node_create("pipe-mgr", NULL, &pipe_mgr_ucli_mod);
	ucli_node_subnode_add(node, ucli_module_log_node_create("pipe-mgr"));
	return node;
}

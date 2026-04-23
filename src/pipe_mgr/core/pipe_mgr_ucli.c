/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
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

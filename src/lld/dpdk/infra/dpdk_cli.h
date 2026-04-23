/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INCLUDE_DPDK_CLI_H__
#define __INCLUDE_DPDK_CLI_H__

#include <stddef.h>

void
cli_process(char *in, char *out, size_t out_size);
#endif

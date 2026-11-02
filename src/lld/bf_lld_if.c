/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>
#include "lld.h"
#include "lld_log.h"

/**
 * @file bf_lld_if.c
 * \brief Details LLD module-level APIs.
 *
 */

/**
 * @addtogroup lld-api
 * @{
 * This is a description of some APIs.
 */

/** \brief Initializa the LLD submodule of a process
 *
 * \param is_master    : whether this LLD instance is the "master" LLD instance
 * \param wr_fn        : Function used to write 32b chip registers
 * \param rd_fn        : Function used to read  32b chip registers
 *
 * \return: BF_SUCCESS : LLD initialized successfully
 */
bf_status_t bf_lld_init(void)
{
	lld_init();
	return BF_SUCCESS;
}

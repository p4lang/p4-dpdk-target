/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*!
 * @file pipe_mgr_mirror.c
 *
 * @description Utilities for mirror profiles
 */

#include "pipe_mgr_mirror.h"
#include "../dal/dal_mirror.h"

/*!
 * Set the config params for mirror profile.
 *
 * @param id session id for which config to be done.
 * @param params params for the mirror session.
 * @param p pointer to pipeline info.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_mirror_session_set(uint32_t id,
			    void *params,
			    void *p)
{
	return dal_mirror_session_set(id, params, p);
}

/*!
 * Set the config params for mirror profile.
 *
 * @param id profile id for which config to be done.
 * @param params params for the mirror session.
 * @param dev_id device id
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_mirror_profile_set(uint32_t id,
			    void *params,
			    int dev_id)
{
	return dal_mirror_profile_set(id, params, dev_id);
}

/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*!
 * @file pipe_mgr_mirror.h
 *
 * @description Utilities for mirror profiles
 */

#ifndef __PIPE_MGR_MIRROR_H__
#define __PIPE_MGR_MIRROR_H__

#include <bf_types/bf_types.h>

/* TODO we will have to revisit some code once, MatchValueLookupTable
 * is implemented.
 */

/*!
 * Set the config params for mirror session.
 *
 * @param id session id for which config to be done.
 * @param params params for the mirror session.
 * @param p pointer to pipeline info.
 * @return Status of the API call
 */
bf_status_t
pipe_mgr_mirror_session_set(uint32_t id,
			    void *params,
			    void *p);

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
			    int dev_id);

#endif /* __PIPE_MGR_MIRROR_H__ */

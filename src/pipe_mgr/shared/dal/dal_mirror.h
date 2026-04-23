/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file dal_mirror.h
 *
 * @description Utilities for Mirroring
 */

#ifndef __DAL_MIRROR_H__
#define __DAL_MIRROR_H__

#include <bf_types/bf_types.h>

/*
 * Set the config params for mirror session.
 *
 * @param id session id for which config to be done.
 * @param params params for the mirror session.
 * @param p pointer to pipeline info.
 * @return Status of the API call
 */
bf_status_t
dal_mirror_session_set(uint32_t id,
		       void *params,
		       void *p);

/*
 * Clear the config params for mirror session.
 *
 * @param id session id for which config to be done.
 * @param p pointer to pipeline info.
 * @return Status of the API call
 */
bf_status_t
dal_mirror_session_clear(uint32_t id,
			 void *p);

/*
 * Set the config params for mirror profile.
 *
 * @param id profile id for which config to be done.
 * @param params params for the mirror profile.
 * @param dev_id device id
 * @return Status of the API call
 */
bf_status_t
dal_mirror_profile_set(uint32_t id,
		       void *params,
		       int dev_id);

/*
 * Clear the config params for mirror profile.
 *
 * @param id profile id for which config to be done.
 * @param dev_id device id
 * @return Status of the API call
 */
bf_status_t
dal_mirror_profile_clear(uint32_t id,
			 int dev_id);

#endif /* __DAL_MIRROR_H__ */

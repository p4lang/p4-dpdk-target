/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <dvm/bf_drv_intf.h>
#include "pipe_mgr/shared/pipe_mgr_infra.h"

int dal_add_device(int dev_id,
		enum bf_dev_family_t dev_family,
		struct bf_device_profile *prof,
		enum bf_dev_init_mode_s warm_init_mode);

int dal_remove_device(int dev_id);

int dal_enable_pipeline(bf_dev_id_t dev_id,
			int profile_id,
			void *spec_file,
			enum bf_dev_init_mode_s warm_init_mode);

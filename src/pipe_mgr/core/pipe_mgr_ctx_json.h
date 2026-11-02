/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __PIPE_MGR_CTX_JSON_H__
#define __PIPE_MGR_CTX_JSON_H__
#include <dvm/bf_drv_profile.h>

int pipe_mgr_ctx_import(int dev_id,
			struct bf_device_profile *inp_profile,
			enum bf_dev_init_mode_s warm_init_mode);
char *trim_classifier_str(char *str);
#endif /*__PIPE_MGR_CTX_JSON_H__*/

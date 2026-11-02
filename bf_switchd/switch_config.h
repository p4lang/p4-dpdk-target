/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __SWITCH_CONFIG_H__
#define __SWITCH_CONFIG_H__
#include "bf_switchd.h"

void switch_p4_pipeline_config_each_program_update(
    p4_devices_t *p4_device,
    bf_device_profile_t *device_profile,
    const char *install_dir,
    bool absolute_paths);
void switch_p4_pipeline_config_each_profile_update(
    p4_programs_t *p4_program,
    bf_p4_program_t *bf_p4_program,
    const char *install_dir,
    bool absolute_paths);
int switch_dev_config_init(const char *install_dir,
                           const char *config_filename,
                           bf_switchd_context_t *self);
#endif /* __SWITCH_CONFIG_H__ */

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

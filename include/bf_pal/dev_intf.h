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
#ifndef _DEV_INTF_H
#define _DEV_INTF_H

#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>
#include <dvm/bf_drv_profile.h>

bf_status_t bf_pal_device_warm_init_begin(
    bf_dev_id_t dev_id,
    bf_dev_init_mode_t warm_init_mode,
    bool upgrade_agents);

bf_status_t bf_pal_device_add(bf_dev_id_t dev_id,
                              bf_device_profile_t *device_profile);

bf_status_t bf_pal_device_warm_init_end(bf_dev_id_t dev_id);

typedef bf_status_t (*bf_pal_device_warm_init_begin_fn)(
    bf_dev_id_t dev_id,
    bf_dev_init_mode_t warm_init_mode,
    bool upgrade_agents);

typedef bf_status_t (*bf_pal_device_add_fn)(
    bf_dev_id_t dev_id, bf_device_profile_t *device_profile);

typedef bf_status_t (*bf_pal_device_warm_init_end_fn)(bf_dev_id_t dev_id);
typedef bf_status_t (*bf_pal_device_pltfm_type_get_fn)(bf_dev_id_t dev_id,
                                                       bool *is_sw_model);

typedef struct bf_pal_dev_callbacks_s {
  bf_pal_device_warm_init_begin_fn warm_init_begin;
  bf_pal_device_add_fn device_add;
  bf_pal_device_warm_init_end_fn warm_init_end;
  bf_pal_device_pltfm_type_get_fn pltfm_type_get;
} bf_pal_dev_callbacks_t;

bf_status_t bf_pal_device_callbacks_register(bf_pal_dev_callbacks_t *callbacks);
bf_status_t bf_pal_pltfm_type_get(bf_dev_id_t dev_id, bool *is_sw_model);
#endif

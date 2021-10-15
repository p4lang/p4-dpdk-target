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

#ifndef DVM_H_INCLUDED
#define DVM_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dvm/bf_drv_intf.h>

#define BF_DRV_MAX_CLIENTS 16
#define BF_DRV_CLIENT_NAME_LEN 50

typedef struct dvm_port_ {
  bf_dev_port_t port;
  int added;
  int enabled;
} dvm_port_t;

typedef struct dvm_asic_ {
  bf_dev_id_t dev_id;
  bf_dev_family_t dev_family;
  int added;
  dvm_port_t port[BF_PIPE_COUNT][BF_PIPE_PORT_COUNT];
  // Logical pipe count.
  uint32_t num_pipes;
  bf_dev_init_mode_t warm_init_mode;
  // Flag to indicate whether any warm init (fast reconfig,
  // fast reconfig quick and hitless HA) is in progress.
  bool warm_init_in_progress;
  bool dev_add_done;
  bool is_sw_model;
} dvm_asic_t;

typedef struct bf_drv_hdl_info_ {
  bool allocated;
  char client_name[BF_DRV_CLIENT_NAME_LEN];
} bf_drv_hdl_info_t;

/* Internal driver modules registration DB */
typedef struct bf_drv_client_ {
  bool valid;
  bf_drv_client_handle_t client_handle;
  bf_drv_client_prio_t priority;
  char client_name[BF_DRV_CLIENT_NAME_LEN];
  bf_drv_client_callbacks_t callbacks;
} bf_drv_client_t;

typedef enum bf_fast_reconfig_step_e {
  BF_RECONFIG_LOCK,
  BF_RECONFIG_UNLOCK,
  BF_RECONFIG_UNLOCK_QUICK,
} bf_fast_reconfig_step_t;

bf_status_t bf_drv_notify_clients_dev_add(bf_dev_id_t dev_id,
                                          bf_dev_family_t dev_family,
                                          bf_device_profile_t *profile,
                                          bf_dev_init_mode_t warm_init_mode);
bf_status_t bf_drv_notify_clients_dev_del(bf_dev_id_t dev_id, bool log_err);
bf_status_t bf_drv_notify_clients_dev_log(bf_dev_id_t dev_id,
                                          const char *filepath);
bf_status_t bf_drv_notify_clients_dev_restore(bf_dev_id_t dev_id,
                                              const char *filepath);
bf_status_t bf_drv_notify_clients_port_add(bf_dev_id_t dev_id,
					   bf_dev_port_t port_id);
bf_status_t bf_drv_notify_clients_port_del(bf_dev_id_t dev_id,
                                           bf_dev_port_t port_id,
                                           bool log_err);
#endif  // DVM_H_INCLUDED

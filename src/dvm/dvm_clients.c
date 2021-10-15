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
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <osdep/p4_sde_osdep.h>
#include <dvm/bf_drv_intf.h>
#include "dvm.h"
#include <dvm/dvm_log.h>
#include <pipe_mgr/pipe_mgr_intf.h>

static bool bf_driver_client_db_init_done = false;
static int bf_drv_num_clients = 0;
bf_drv_hdl_info_t bf_drv_hdl_info[BF_DRV_MAX_CLIENTS];

bf_drv_client_t bf_driver_client_db[BF_DRV_MAX_CLIENTS];

/* Init the DB */
void bf_driver_client_db_init() {
  memset(&bf_drv_hdl_info, 0, sizeof(bf_drv_hdl_info));
  memset(&bf_driver_client_db, 0, sizeof(bf_driver_client_db));
}

/* allocate an id for the client */
bf_status_t bf_drv_register(const char *client_name,
                            bf_drv_client_handle_t *client_handle) {
  int id = 0;

  if (bf_driver_client_db_init_done == false) {
    bf_driver_client_db_init();
    bf_driver_client_db_init_done = true;
  }

  /* Allocate id from 1 */
  for (id = 1; id < BF_DRV_MAX_CLIENTS; id++) {
    if (bf_drv_hdl_info[id].allocated == true) {
      continue;
    }

    bf_drv_hdl_info[id].allocated = true;
    strncpy(bf_drv_hdl_info[id].client_name,
            client_name,
            BF_DRV_CLIENT_NAME_LEN - 1);
    bf_drv_hdl_info[id].client_name[BF_DRV_CLIENT_NAME_LEN - 1] = '\0';
    break;
  }

  if (id >= BF_DRV_MAX_CLIENTS) {
    *client_handle = -1;
    return BF_MAX_SESSIONS_EXCEEDED;
  }

  bf_drv_num_clients++;
  *client_handle = id;

  return BF_SUCCESS;
}

/* De-register the client */
bf_status_t bf_drv_deregister(bf_drv_client_handle_t client_handle) {
  int id = 0, i = 0;
  bf_drv_client_t *db_ptr;

  bf_sys_assert(client_handle != -1);
  bf_sys_assert(client_handle < BF_DRV_MAX_CLIENTS);

  memset(&bf_drv_hdl_info[client_handle],
         0,
         sizeof(bf_drv_hdl_info[client_handle]));

  bf_drv_num_clients--;

  for (id = 0; id < BF_DRV_MAX_CLIENTS; id++) {
    db_ptr = &bf_driver_client_db[id];
    if (db_ptr->valid == false) {
      continue;
    }
    if (db_ptr->client_handle == client_handle) {
      /* Move all entries up */
      for (i = id; i < (BF_DRV_MAX_CLIENTS - 1); i++) {
        memcpy(&bf_driver_client_db[i],
               &bf_driver_client_db[i + 1],
               sizeof(bf_drv_client_t));
      }
      /* Zero out last entry */
      db_ptr = &bf_driver_client_db[BF_DRV_MAX_CLIENTS - 1];
      memset(db_ptr, 0, sizeof(bf_drv_client_t));
      break;
    }
  }

  return BF_SUCCESS;
}

/* Master function to register callbacks */
bf_status_t bf_drv_client_register_callbacks(
    bf_drv_client_handle_t client_handle,
    bf_drv_client_callbacks_t *callbacks,
    bf_drv_client_prio_t add_priority) {
  bf_status_t status = 0;
  int id = 0, i = 0;
  bool shift = false;

  if (!callbacks) return BF_INVALID_ARG;

  bf_sys_assert(client_handle != -1);
  bf_sys_assert(client_handle < BF_DRV_MAX_CLIENTS);
  bf_sys_assert(bf_drv_hdl_info[client_handle].allocated == true);

  for (id = 0; id < BF_DRV_MAX_CLIENTS; id++) {
    if (bf_driver_client_db[id].valid == false) {
      break;
    }
    if (bf_driver_client_db[id].client_handle == client_handle) {
      return status;
    }
    if (bf_driver_client_db[id].priority < add_priority) {
      shift = true;
      break;
    }
  }

  /* Make sure an empty space was found */
  if (id > (BF_DRV_MAX_CLIENTS - 1 - 1)) {
    bf_sys_assert(0);
    return BF_NO_SYS_RESOURCES;
  }

  if (shift == true) {
    for (i = BF_DRV_MAX_CLIENTS - 1 - 1; i >= id; i--) {
      memcpy(&bf_driver_client_db[i + 1],
             &bf_driver_client_db[i],
             sizeof(bf_drv_client_t));
    }
  }

  bf_driver_client_db[id] = (bf_drv_client_t){
      .valid = true,
      .client_handle = client_handle,
      .priority = add_priority,
      .callbacks = *callbacks,
  };

  strncpy(bf_driver_client_db[id].client_name,
          bf_drv_hdl_info[client_handle].client_name,
          BF_DRV_CLIENT_NAME_LEN - 1);
  bf_driver_client_db[id].client_name[BF_DRV_CLIENT_NAME_LEN - 1] = '\0';

  return status;
}

bf_status_t bf_drv_notify_clients_dev_add(bf_dev_id_t dev_id,
                                          bf_dev_family_t dev_family,
                                          bf_device_profile_t *profile,
                                          bf_dev_init_mode_t warm_init_mode) {
  int id = 0;
  bf_drv_client_t *db_ptr = NULL;
  bf_status_t status = BF_SUCCESS;

  for (id = 0; id < BF_DRV_MAX_CLIENTS; id++) {
    db_ptr = &bf_driver_client_db[id];
    if (db_ptr->valid == false) {
      continue;
    }
    if (db_ptr->callbacks.device_add) {
      status = db_ptr->callbacks.device_add(
          dev_id, dev_family, profile, warm_init_mode);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "Device add handling failed for dev %d, sts %s (%d),"
            " Client %s ",
            dev_id,
            bf_err_str(status),
            status,
            db_ptr->client_name);
        return status;
      }
    }
  }

  return status;
}


bf_status_t bf_drv_notify_clients_dev_del(bf_dev_id_t dev_id, bool log_err) {
  int id = 0;
  bf_drv_client_t *db_ptr = NULL;
  bf_status_t status = BF_SUCCESS;

  for (id = BF_DRV_MAX_CLIENTS - 1; id >= 0; id--) {
    db_ptr = &bf_driver_client_db[id];
    if (db_ptr->valid == false) {
      continue;
    }
    if (db_ptr->callbacks.device_del) {
      bf_status_t client_status = db_ptr->callbacks.device_del(dev_id);
      if (client_status != BF_SUCCESS) {
        /* Don't log error if this is a cleanup being performed due
           to an earlier add failure
        */
        if (log_err) {
          LOG_ERROR(
              "Device del handling failed for dev %d,"
              "sts %s (%d), Client %s ",
              dev_id,
              bf_err_str(client_status),
              client_status,
              db_ptr->client_name);
        }
        if (status == BF_SUCCESS) {
          status = client_status;
        }
      }
    }
  }

  return status;
}

bf_status_t bf_drv_notify_clients_dev_log(bf_dev_id_t dev_id,
                                          const char *filepath) {
  int id = 0;
  bf_drv_client_t *db_ptr = NULL;
  bf_status_t status = BF_SUCCESS;
  cJSON *dev, *clients, *client;
  FILE *logfile;
  char *output_str;

  dev = cJSON_CreateObject();
  cJSON_AddItemToObject(dev, "clients", clients = cJSON_CreateArray());

  for (id = 0; id < BF_DRV_MAX_CLIENTS; id++) {
    db_ptr = &bf_driver_client_db[id];
    if (db_ptr->valid == false) {
      continue;
    }
    if (db_ptr->callbacks.device_log) {
      cJSON_AddItemToArray(clients, client = cJSON_CreateObject());
      cJSON_AddNumberToObject(client, "client_id", id);
      cJSON_AddStringToObject(client, "name", db_ptr->client_name);
      bf_status_t client_status = db_ptr->callbacks.device_log(dev_id, client);
      if (client_status != BF_SUCCESS) {
        LOG_ERROR(
            "Device log handling failed for dev %d,"
            "sts %s (%d), Client %s ",
            dev_id,
            bf_err_str(client_status),
            client_status,
            db_ptr->client_name);
        if (status == BF_SUCCESS) {
          status = client_status;
        }
      }
    }
  }

  if (status == BF_SUCCESS) {
    logfile = fopen(filepath, "w");
    if (!logfile) {
      status = BF_OBJECT_NOT_FOUND;
    } else {
      output_str = cJSON_Print(dev);
      if (output_str) {
          fputs(output_str, logfile);
          bf_sys_free(output_str);
      }
      fclose(logfile);
    }
  }
  cJSON_Delete(dev);
  return status;
}

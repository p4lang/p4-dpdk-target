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

#include "dvm_log.h"
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_err.h>
#include <osdep/p4_sde_osdep.h>
#include "dvm.h"
#include "bf_drv_ver.h"

static bool bf_drv_init_done = false;

dvm_asic_t *dev_id_set[BF_MAX_DEV_COUNT];

/****************************************************************
* bf_drv_init
****************************************************************/
bf_status_t bf_drv_init() {
  if (bf_drv_init_done) {
    return BF_SUCCESS;
  }
  memset((char *)dev_id_set, 0, sizeof(dev_id_set));

  // log SDE version
  LOG_DBG("BF-SDE vesion: %s", BF_DRV_REL_VER);

  bf_drv_init_done = true;
  return BF_SUCCESS;
}

/****************************************************************
* bf_device_add
****************************************************************/
bf_status_t bf_device_add(bf_dev_family_t dev_family,
                          bf_dev_id_t dev_id,
                          bf_device_profile_t *profile,
                          bf_dev_flags_t flags) {
  bf_status_t status = BF_SUCCESS;
  uint32_t num_pipes = 0;
  bf_dev_init_mode_t warm_init_mode;
  bool is_sw_model = BF_DEV_IS_SW_MODEL_GET(flags);

  LOG_TRACE("bf_device_add dev id %d, is_sw_model %d", dev_id, is_sw_model);

  if (dev_id_set[dev_id] && dev_id_set[dev_id]->dev_add_done) {
    LOG_ERROR("Device id %u already exists in device add", dev_id);
    return BF_ALREADY_EXISTS;
  }

  if (dev_id_set[dev_id]) {
    warm_init_mode = dev_id_set[dev_id]->warm_init_mode;
  } else {
    warm_init_mode = BF_DEV_INIT_COLD;
  }

  if (!dev_id_set[dev_id]) {
    dev_id_set[dev_id] = bf_sys_calloc(1, sizeof(dvm_asic_t));
    if (!dev_id_set[dev_id]) {
      return BF_NO_SYS_RESOURCES;
    }
  }

  /* Update dev_id and device type before notifying clients */
  dev_id_set[dev_id]->dev_id = dev_id;
  dev_id_set[dev_id]->dev_family = dev_family;
  dev_id_set[dev_id]->is_sw_model = is_sw_model;

  // update our db
  // Cache the number of logical pipes
  dev_id_set[dev_id]->num_pipes = 1;

  /* Sanitize the profile.  Check that invalid pipes are not requested. */
  bf_device_profile_t local_profile;
  if (profile) {
    local_profile = *profile;
    bool bad_pipe = false;
    for (unsigned i = 0; i < local_profile.num_p4_programs; ++i) {
      bf_p4_program_t *prog = &local_profile.p4_programs[i];
      for (unsigned j = 0; j < prog->num_p4_pipelines; ++j) {
        bf_p4_pipeline_t *pipeline = &prog->p4_pipelines[j];

        /* Sort the pipe_scope array. */
        for (int J = 0; J < pipeline->num_pipes_in_scope; ++J)
          for (int O = J + 1; O < pipeline->num_pipes_in_scope; ++O)
            if (pipeline->pipe_scope[O] < pipeline->pipe_scope[J]) {
              int x = pipeline->pipe_scope[J];
              pipeline->pipe_scope[J] = pipeline->pipe_scope[O];
              pipeline->pipe_scope[O] = x;
            }

        /* Limit the number of pipes a P4 Pipeline is assigned to based on the
         * number of pipes on the chip. */
        if ((uint32_t)pipeline->num_pipes_in_scope > num_pipes) {
          pipeline->num_pipes_in_scope = num_pipes;
        }
        /* Check that the requested pipes exist on this device. */
        for (int k = 0; k < pipeline->num_pipes_in_scope; ++k) {
          if ((uint32_t)pipeline->pipe_scope[k] >= num_pipes) {
            bad_pipe = true;
            LOG_ERROR(
                "Program \"%s\" Pipeline \"%s\" cannot be assigned to device "
                "%d pipe %d, only %d pipe(s) available",
                prog->prog_name,
                pipeline->p4_pipeline_name,
                dev_id,
                pipeline->pipe_scope[k],
                num_pipes);
          }
        }
      }
    }
    if (bad_pipe) return BF_INVALID_ARG;
  }

  // notify clients
  status = bf_drv_notify_clients_dev_add(dev_id,
                                         dev_family,
                                         profile ? &local_profile : NULL,
                                         warm_init_mode);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Device add failed for dev %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
    bf_drv_notify_clients_dev_del(dev_id, false);

    bf_sys_free(dev_id_set[dev_id]);
    dev_id_set[dev_id] = NULL;
    return status;
  }

  dev_id_set[dev_id]->dev_add_done = true;

  return status;
}

/****************************************************************
* bf_device_remove
****************************************************************/
bf_status_t bf_device_remove(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;

  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }

  // notify clients
  status = bf_drv_notify_clients_dev_del(dev_id, true);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Device remove failed for dev %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
    /* Do not return here as we cannot recover from this, update DB */
  }

  // update our db
  bf_sys_free(dev_id_set[dev_id]);
  dev_id_set[dev_id] = NULL;
  return status;
}

/****************************************************************
* bf_device_log
****************************************************************/
bf_status_t bf_device_log(bf_dev_id_t dev_id, const char *filepath) {
  bf_status_t status = BF_SUCCESS;

  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }

  // notify clients
  status = bf_drv_notify_clients_dev_log(dev_id, filepath);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Device log failed for dev %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
  }
  return status;
}

const char *bf_drv_get_version(void) { return BF_DRV_VER; }

const char *bf_drv_get_internal_version(void) { return BF_DRV_INTERNAL_VER; }

/**
 * Initiate a warm init process for a device
 * @param dev_id The device id
 * @param warm_init_mode The warm init mode to use for this device
 * @param serdes_upgrade_mode The mode to use for updating SerDes
 * @return Status of the API call.
 */
bf_status_t bf_device_warm_init_begin(
    bf_dev_id_t dev_id,
    bf_dev_init_mode_t warm_init_mode) {
  LOG_TRACE(
      "%s:%d Entering WARM_INIT_BEGIN for device %d with warm_init_mode %d ",
      __func__,
      __LINE__,
      dev_id,
      warm_init_mode);
  bf_status_t status = BF_SUCCESS;

  if (warm_init_mode != BF_DEV_WARM_INIT_FAST_RECFG_QUICK) {
    if (dev_id_set[dev_id] && dev_id_set[dev_id]->dev_add_done) {
      /* If the device already exists, lock the device and remove it */
      status = bf_device_remove(dev_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d Device id %u cannot be removed error %d",
                  __func__,
                  __LINE__,
                  dev_id,
                  status);
        return status;
      }
    }

    dev_id_set[dev_id] = bf_sys_calloc(1, sizeof(dvm_asic_t));
    if (!dev_id_set[dev_id]) {
      return BF_NO_SYS_RESOURCES;
    }

    dev_id_set[dev_id]->dev_id = dev_id;
    dev_id_set[dev_id]->dev_add_done = false;
  } else {
    if (!dev_id_set[dev_id]) {
      return BF_INVALID_ARG;
    }
  }
  dev_id_set[dev_id]->warm_init_mode = warm_init_mode;
  // Set warm init in progress flag
  dev_id_set[dev_id]->warm_init_in_progress = true;

  LOG_TRACE("%s:%d Exiting WARM_INIT_BEGIN for device %d",
            __func__,
            __LINE__,
            dev_id);
  return status;
}

/**
 * End the warm init sequence for the device and resume normal operation
 * @param dev_id The device id
 * @return Status of the API call.
 */
bf_status_t bf_device_warm_init_end(bf_dev_id_t dev_id) {
  LOG_TRACE(
      "%s:%d Entering WARM_INIT_END for device %d", __func__, __LINE__, dev_id);
  bf_status_t status = BF_SUCCESS;
  bf_dev_init_mode_t warm_init_mode;

  if (!dev_id_set[dev_id] || !dev_id_set[dev_id]->dev_add_done) {
    LOG_ERROR("%s:%d Device id %u not found in warm init end",
              __func__,
              __LINE__,
              dev_id);
    return BF_OBJECT_NOT_FOUND;
  }

  warm_init_mode = dev_id_set[dev_id]->warm_init_mode;

  switch (warm_init_mode) {
    case BF_DEV_INIT_COLD:
      /* Do nothing */
      return status;
    case BF_DEV_WARM_INIT_FAST_RECFG:
      LOG_TRACE("%s:%d Apply BF_RECONFIG_LOCK step to device %d",
                __func__,
                __LINE__,
                dev_id);
      break;
    default:
      LOG_TRACE("warm_init mode %d not supported", warm_init_mode);
      break;
  }

  // Reset warm init in progress flag
  dev_id_set[dev_id]->warm_init_in_progress = false;

  LOG_TRACE(
      "%s:%d Exiting WARM_INIT_END for device %d", __func__, __LINE__, dev_id);
  return status;
}

/**
 * Get the init mode of a device
 * @param dev_id The device id
 * @param warm_init_mode The warm init mode to use for this device
 * @return Status of the API call
 */
bf_status_t bf_device_init_mode_get(bf_dev_id_t dev_id,
                                    bf_dev_init_mode_t *warm_init_mode) {
  if (!dev_id_set[dev_id] || !dev_id_set[dev_id]->dev_add_done) {
    LOG_ERROR("Device id %u not found in warm init end", dev_id);
    return BF_OBJECT_NOT_FOUND;
  }

  *warm_init_mode = dev_id_set[dev_id]->warm_init_mode;

  return BF_SUCCESS;
}

/**
 * Get the device type (model or asic)
 * @param dev_id The device id
 * @param is_sw_model Pointer to bool flag to return true for model and
 *                    false for asic devices
 * @return Status of the API call.
 */
bf_status_t bf_drv_device_type_get(bf_dev_id_t dev_id, bool *is_sw_model) {
  *is_sw_model = false;

  if (dev_id < 0 || dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }

  *is_sw_model = dev_id_set[dev_id]->is_sw_model;

  return BF_SUCCESS;
}


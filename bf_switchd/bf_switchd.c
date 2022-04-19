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
/*
    bf_switchd.c
*/
/* Standard includes */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sched.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <errno.h>
#include <dirent.h>
#include <sys/ioctl.h>

/* <bf_syslib> includes */
#include <osdep/p4_sde_osdep.h>

/* <clish> includes  */
#include <target-utils/clish/thread.h>

/* <bf_driver> includes */
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_err.h>
#include <port_mgr/bf_port_if.h>
#include <bf_rt/bf_rt_init.h>
#include <bf_pal/dev_intf.h>
#include <bf_pal/bf_pal_port_intf.h>

/* Required for python shell */
#include <lld/python_shell_mutex.h>

#ifdef P4RT_ENABLED
#include "p4_rt/server/server.h"
#endif

#ifdef GRPC_ENABLED
#include <bf_rt/proto/bf_rt_server.h>
#endif

/* Local includes */
#include "switch_config.h"

/******************************************************************************
*******************************************************************************
                          BF_SWITCHD OPERATIONAL MODE SETTINGS
*******************************************************************************
******************************************************************************/
#include "bf_switchd.h"

/* Global defines */
bool is_skip_p4 = false;

/* Local defines */
typedef void *(*start_routine)(void *);
typedef void *pvoid_dl_t __attribute__((__may_alias__));
static bf_switchd_context_t *switchd_ctx = NULL;

/* Initialize switch.p4 SWITCHAPI library */
static void bf_switchd_switchapi_lib_init(bf_dev_id_t dev_id) {
  int j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);
  bf_dev_init_mode_t init_mode = switchd_ctx->init_mode;
  bool warm_boot = (init_mode != BF_DEV_INIT_COLD) ? true : false;
  (void)warm_boot;

#ifdef STATIC_LINK_LIB
  if (!state) {
    return;
  }

  /* Currently the switch libraries need to be initialized only once */
  if (dev_id != 0) {
    return;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    p4_programs_t *p4_programs = &(p4_device->p4_programs[j]);

    /* Initialize the switch API lib */
    if (switchd_ctx->switch_api_init_fn) {
      if (p4_programs->use_eth_cpu_port) {
        switchd_ctx->switch_api_init_fn(dev_id,
                                        0,
                                        p4_programs->eth_cpu_port_name,
                                        p4_programs->add_ports_to_switchapi,
                                        switchd_ctx->rpc_server_cookie);
      } else {
        switchd_ctx->switch_api_init_fn(dev_id,
                                        0,
                                        NULL,
                                        p4_programs->add_ports_to_switchapi,
                                        switchd_ctx->rpc_server_cookie);
      }
      printf("bf_switchd: switchapi initialized\n");

      /* Load switchapi only once */
      return;
    }
  }
#else   // STATIC_LINK_LIB
  int (*switchapi_init_fn)(int, int, char *, bool);
  int (*switchapi_start_srv_fn)(void);
  int (*switchapi_start_packet_driver_fn)();
  int (*bf_switch_init_fn)(uint16_t, char *, char *, bool, char *);

  if (!state) {
    return;
  }

  /* Currently the switch libraries need to be initialized only once */
  if (dev_id != 0) {
    return;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    p4_programs_t *p4_programs = &(p4_device->p4_programs[j]);
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);

    if (p4_program_state->switchapi_lib_handle == NULL) {
      continue;
    }

    /* Initialize the switch API lib */
    *(pvoid_dl_t *)(&bf_switch_init_fn) =
        dlsym(p4_program_state->switchapi_lib_handle, "bf_switch_init");
    if (bf_switch_init_fn) {
      if (p4_programs->use_eth_cpu_port) {
        bf_switch_init_fn(dev_id,
                          p4_programs->eth_cpu_port_name,
                          p4_programs->model_json_path,
                          warm_boot,
                          "/tmp/db.txt");
      } else {
        bf_switch_init_fn(dev_id,
                          NULL,
                          p4_programs->model_json_path,
                          warm_boot,
                          "/tmp/db.txt");
      }
    }

    *(pvoid_dl_t *)(&switchapi_init_fn) =
        dlsym(p4_program_state->switchapi_lib_handle, "switch_api_init");
    if (switchapi_init_fn) {
      if (p4_programs->use_eth_cpu_port) {
        switchapi_init_fn(dev_id,
                          0,
                          p4_programs->eth_cpu_port_name,
                          p4_programs->add_ports_to_switchapi);
      } else {
        switchapi_init_fn(dev_id, 0, NULL, p4_programs->add_ports_to_switchapi);
      }

      *(pvoid_dl_t *)(&switchapi_start_srv_fn) =
          dlsym(p4_program_state->switchapi_lib_handle,
                "start_switch_api_rpc_server");
      if (switchapi_start_srv_fn) {
        switchapi_start_srv_fn();
      }
    } else {
      *(pvoid_dl_t *)(&switchapi_start_srv_fn) =
          dlsym(p4_program_state->switchapi_lib_handle,
                "start_bf_switcht_api_rpc_server");
      if (switchapi_start_srv_fn) {
        switchapi_start_srv_fn();
      }
    }
    printf("bf_switchd: switchapi initialized\n");

    *(pvoid_dl_t *)(&switchapi_start_packet_driver_fn) =
        dlsym(p4_program_state->switchapi_lib_handle,
              "start_switch_api_packet_driver");
    if (switchapi_start_packet_driver_fn) {
      switchapi_start_packet_driver_fn();
    }
    /* Load switchapi only once */
    return;
  }
#endif  // STATIC_LINK_LIB
}

/* Initialize switch.p4 SWITCHSAI library */
static void bf_switchd_switchsai_lib_init(bf_dev_id_t dev_id) {
  int j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);

  printf("bf_switchd_switchsai_lib_init");
#ifdef STATIC_LINK_LIB
  if (!state) {
    return;
  }

  /* Currently the switch libraries need to be initialized only once */
  if (dev_id != 0) {
    return;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    p4_programs_t *p4_programs = &(p4_device->p4_programs[j]);

    if (p4_programs->add_ports_to_switchapi) {
      if (switchd_ctx->switch_sai_init_fn) {
        switchd_ctx->switch_sai_init_fn();

        /* Load switchsai only once */
        return;
      }
    }
  }

#else   // STATIC_LINK_LIB
  char fn_name[250];
  int (*switchsai_start_srv_fn)(char *);
  void (*switchsai_init_fn)();
  void (*smi_sai_init_fn)(uint16_t, char *, char *);

  if (!state) {
    return;
  }

  /* Currently the switch libraries need to be initialized only once */
  if (dev_id != 0) {
    return;
  }

  printf("\n starting SAI \n");
  for (j = 0; j < p4_device->num_p4_programs; j++) {
    p4_programs_t *p4_programs = &(p4_device->p4_programs[j]);
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);

    if (p4_program_state->switchsai_lib_handle == NULL) {
      continue;
    }

    printf("switch_sai_init \n");
    if (p4_programs->add_ports_to_switchapi) {
      sprintf(fn_name, "switch_sai_init");
      *(pvoid_dl_t *)(&switchsai_init_fn) =
          dlsym(p4_program_state->switchsai_lib_handle, fn_name);
      if (switchsai_init_fn) {
        switchsai_init_fn();
      }
    }

    printf("smi_sai_init \n");
    if (p4_programs->add_ports_to_switchapi) {
      sprintf(fn_name, "smi_sai_init");
      *(pvoid_dl_t *)(&smi_sai_init_fn) =
          dlsym(p4_program_state->switchsai_lib_handle, fn_name);
      if (smi_sai_init_fn) {
        smi_sai_init_fn(dev_id,
                        p4_programs->eth_cpu_port_name,
                        p4_programs->model_json_path);
      }
    }

    /* Initialize the switch SAI lib */
    if (p4_programs->add_ports_to_switchapi) {
      sprintf(fn_name, "start_p4_sai_thrift_rpc_server");
      *(pvoid_dl_t *)(&switchsai_start_srv_fn) =
          dlsym(p4_program_state->switchsai_lib_handle, fn_name);
      if (switchsai_start_srv_fn) {
        switchsai_start_srv_fn("9093");
      }
    }

    printf("bf_switchd: switchsai initialized\n");
    /* Load switchsai only once */
    return;
  }
#endif  // STATIC_LINK_LIB
}

#ifndef STATIC_LINK_LIB
/* Initialize switch.p4 SWITCHLINK library */
static void bf_switchd_switchlink_lib_init(bf_dev_id_t dev_id) {
  int j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);
  char fn_name[250];
  int (*switchlink_start_fn)();

  if (!state) {
    return;
  }

  /* Currently the switch libraries need to be initialized only once */
  if (dev_id != 0) {
    return;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);

    if (p4_program_state->switchlink_lib_handle == NULL) {
      continue;
    }

    /* Initialize the switch LINK lib */
    sprintf(fn_name, "switchlink_init");
    *(pvoid_dl_t *)(&switchlink_start_fn) =
        dlsym(p4_program_state->switchlink_lib_handle, fn_name);
    if (switchlink_start_fn) {
      switchlink_start_fn();
    }

    printf("bf_switchd: switchlink initialized\n");
    /* init switchsai only once */
    return;
  }
}
#endif  // STATIC_LINK_LIB

static void bf_switchd_ports_del(bf_dev_id_t dev_id) {
  unsigned int pipe = 0, port = 0;
      if (!switchd_ctx->state[dev_id].device_locked)
        bf_port_remove(dev_id, MAKE_DEV_PORT(pipe, port));
}

/* Delete all ports of a device */
static void bf_switchd_ports_delete(int dev_id) {
  if (switchd_ctx->asic[dev_id].is_virtual) return;
    bf_switchd_ports_del(dev_id);
}

/* Helper routine to initialize_profile structure needed by the driver */
static bf_status_t bf_switchd_init_device_profile(
    bf_dev_id_t dev_id, bool skip_p4, bf_device_profile_t *dev_profile_p) {
  if (!switchd_ctx) return BF_NOT_READY;

  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  int i = 0, j = 0, s = 0;

  if (switchd_ctx->asic[dev_id].chip_family == BF_DEV_FAMILY_DPDK) {
	  char *eal_args = dev_profile_p->eal_args;
	  memset(eal_args, 0, MAX_EAL_LEN);
	  strncpy(eal_args, p4_device->eal_args, MAX_EAL_LEN - 1);
	  // Flag for enabling debug cli
	  dev_profile_p->debug_cli_enable = p4_device->debug_cli_enable;
	  // initialize the number of mempool objs
	  dev_profile_p->num_mempool_objs = p4_device->num_mempool_objs;
	  for (j = 0; j < p4_device->num_mempool_objs; j++) {
		  struct mempool_obj_s *mempool_obj = &(p4_device->mempool_objs[j]);
		  struct bf_mempool_obj_s *dev_profile_mempool = &(dev_profile_p->mempool_objs[j]);

		  snprintf(dev_profile_mempool->name, sizeof(dev_profile_mempool->name),
				  "%s", mempool_obj->name);
		  dev_profile_mempool->buffer_size = mempool_obj->buffer_size;
		  dev_profile_mempool->pool_size   = mempool_obj->pool_size;
		  dev_profile_mempool->cache_size  = mempool_obj->cache_size;
		  dev_profile_mempool->numa_node   = mempool_obj->numa_node;
	  }
  }

#ifdef BFRT_ENABLED
  if (switchd_ctx->install_dir != NULL) {
    // set bfrt non p4 json path
    snprintf(switchd_ctx->asic[dev_id].bfrt_non_p4_json_dir_path,
             sizeof(switchd_ctx->asic[dev_id].bfrt_non_p4_json_dir_path),
             "%s/%s",
             switchd_ctx->install_dir,
             "share/bf_rt_shared/");
    dev_profile_p->bfrt_non_p4_json_dir_path =
        strdup(switchd_ctx->asic[dev_id].bfrt_non_p4_json_dir_path);
  }
#endif

  // initialize the number of p4 programs for the device
  dev_profile_p->num_p4_programs = p4_device->num_p4_programs;
  for (j = 0; j < p4_device->num_p4_programs; j++) {
    p4_programs_t *p4_programs = &(p4_device->p4_programs[j]);
    bf_p4_program_t *dev_profile_program = &(dev_profile_p->p4_programs[j]);

    /* Copy the program name */
    snprintf(dev_profile_program->prog_name,
             sizeof(dev_profile_program->prog_name),
             "%s",
             p4_programs->program_name);
    if (strlen(p4_programs->bfrt_config)) {
      dev_profile_program->bfrt_json_file = strdup(p4_programs->bfrt_config);
    } else {
      dev_profile_program->bfrt_json_file = NULL;
    }

    /* Port Configuration Json File */
    if (strnlen(p4_programs->port_config, BF_SWITCHD_MAX_FILE_NAME)) {
      dev_profile_program->port_config_json = strdup(p4_programs->port_config);
    } else {
      dev_profile_program->port_config_json = NULL;
    }

    // initialize p4 profile(s) for the device
    dev_profile_program->num_p4_pipelines = p4_programs->num_p4_pipelines;
    for (i = 0; i < p4_programs->num_p4_pipelines; i++) {
      p4_pipeline_config_t *p4_pipeline = &(p4_programs->p4_pipelines[i]);
      bf_p4_pipeline_t *dev_profile_pipeline =
          &(dev_profile_program->p4_pipelines[i]);

      /* Copy the pipeline name */
      snprintf(
          dev_profile_pipeline->p4_pipeline_name,
          sizeof(dev_profile_pipeline->p4_pipeline_name),
          "%s",
          p4_pipeline->p4_pipeline_name ? p4_pipeline->p4_pipeline_name : "");
      if (switchd_ctx->asic[dev_id].chip_family == BF_DEV_FAMILY_DPDK) {
         dev_profile_pipeline->core_id   = p4_pipeline->core_id;
         dev_profile_pipeline->numa_node = p4_pipeline->numa_node;
	 memcpy(&dev_profile_pipeline->mir_cfg, &p4_pipeline->mir_cfg,
		sizeof(struct mirror_config_s));
      }

      // configuration file
      dev_profile_pipeline->cfg_file = strdup(p4_pipeline->cfg_file);
      // Json file
      dev_profile_pipeline->runtime_context_file = p4_pipeline->table_config[0]?
	      strdup(p4_pipeline->table_config) : NULL;
      // PI configuration
      dev_profile_pipeline->pi_config_file =
          strdup(p4_pipeline->pi_native_config_path);
      // Pipes in Scope
      dev_profile_pipeline->num_pipes_in_scope =
          p4_pipeline->num_pipes_in_scope;
      for (s = 0; s < p4_pipeline->num_pipes_in_scope; s++) {
        dev_profile_pipeline->pipe_scope[s] = p4_pipeline->pipe_scope[s];
      }
    }
  }

  return BF_SUCCESS;
}

static void bf_switchd_free_device_profile(bf_device_profile_t *dev_profile) {
  if (!dev_profile) return;

  if (dev_profile->bfrt_non_p4_json_dir_path)
    free(dev_profile->bfrt_non_p4_json_dir_path);
  for (unsigned i = 0; i < dev_profile->num_p4_programs; ++i) {
    bf_p4_program_t *p = dev_profile->p4_programs + i;
    if (p->bfrt_json_file) free(p->bfrt_json_file);
    for (unsigned j = 0; j < p->num_p4_pipelines; ++j) {
      if (p->p4_pipelines[j].cfg_file) free(p->p4_pipelines[j].cfg_file);
      if (p->p4_pipelines[j].runtime_context_file)
        free(p->p4_pipelines[j].runtime_context_file);
      if (p->p4_pipelines[j].pi_config_file)
        free(p->p4_pipelines[j].pi_config_file);
    }
  }
}

/* Routine to instantiate and initialize a device with the driver */
bf_status_t bf_switchd_device_add(bf_dev_id_t dev_id, bool setup_dma) {
  bf_status_t status = BF_SUCCESS;
  bf_device_profile_t dev_profile;
  bool is_sw_model;
  bf_dev_flags_t flags = 0;

  if (dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  /* Initialize P4 profile for the device */
  memset(&dev_profile, 0, sizeof dev_profile);

  if (switchd_ctx->asic[dev_id].configured) {
    status = bf_switchd_init_device_profile(
        dev_id, switchd_ctx->skip_p4, &dev_profile);
    if (status != BF_SUCCESS) {
      printf("ERROR: bf_switchd_init_device_profile failed(%d) for dev_id %d\n",
             status,
             dev_id);
      return status;
    }
  }

  is_skip_p4 = switchd_ctx->skip_p4;
  if (switchd_ctx->skip_p4) {
    printf("Skipping P4 program load for dev_id %d\n", dev_id);
  }
  {
    is_sw_model = switchd_ctx->asic[dev_id].is_sw_model;
    /* Set the is_sw_model value in drv flags to pass it to bf-drivers */
    BF_DEV_IS_SW_MODEL_SET(flags, is_sw_model);
    /* device add only if  using PCIe access */
    /* *** might want to disable bf_device_add for i2c_only_image */
    /* Invoke the driver API to add the device */
    status = bf_device_add(switchd_ctx->asic[dev_id].chip_family,
                           dev_id,
                           &dev_profile,
                           flags);
    if (status != BF_SUCCESS) {
      printf("ERROR: bf_device_add failed(%d) for dev_id %d\n", status, dev_id);
      bf_switchd_free_device_profile(&dev_profile);
      return status;
    }
  }
  bf_switchd_free_device_profile(&dev_profile);

  switchd_ctx->state[dev_id].device_ready = true;

  return status;
}

#ifndef STATIC_LINK_LIB
/* Helper routine to load a library */
static void *bf_switchd_lib_load(char *lib) {
  void *handle = NULL;
  char *error;

  printf("bf_switchd_lib_load \n");
  if (strlen(lib) == 0) {
    return NULL;
  }

  handle = dlopen(lib, RTLD_LAZY | RTLD_GLOBAL);
  if ((error = dlerror()) != NULL) {
    printf("ERROR:%s:%d: dlopen failed for %s, err=%s\n",
           __func__,
           __LINE__,
           lib,
           error);
    return NULL;
  }
  printf("bf_switchd: library %s loaded\n", lib);
  return handle;
}

/* Load mav diag lib */
static int bf_switchd_accton_diag_lib_load(bf_dev_id_t dev_id) {
  int j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);

  if (!p4_device || !state || switchd_ctx->skip_p4) {
    return 0;
  }
  for (j = 0; j < p4_device->num_p4_programs; j++) {
    p4_programs_t *p4_programs = &(p4_device->p4_programs[j]);
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);

    if (strlen(p4_programs->accton_diag)) {
      if (access(p4_programs->accton_diag, F_OK) == -1) {
        return -1;
      }
      p4_program_state->accton_diag_lib_handle =
          bf_switchd_lib_load(p4_programs->accton_diag);
      if (p4_program_state->accton_diag_lib_handle == NULL) {
        printf("ERROR: accton_diag lib loading failure \n");
        return -1;
      }

      /* Load accton_diag library only once per device */
      return 0;
    }
  }
  return 0;
}

/* Unload mav diag lib */
static void bf_switchd_accton_diag_lib_unload(bf_dev_id_t dev_id) {
  int j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);
  int ret = 0;

  if (!p4_device || !state) {
    return;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    p4_programs_t *p4_programs = &(p4_device->p4_programs[j]);
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);

    if (p4_program_state->accton_diag_lib_handle) {
      /* Cancel the thread before unloading lib */
      if (switchd_ctx->accton_diag_t_id) {
        ret = pthread_cancel(switchd_ctx->accton_diag_t_id);
        if (ret != 0) {
          printf("ERROR: thread cancel failed for accton_diag : %d\n", ret);
        } else {
          pthread_join(switchd_ctx->accton_diag_t_id, NULL);
        }
        switchd_ctx->accton_diag_t_id = 0;
      }
      dlclose(p4_program_state->accton_diag_lib_handle);
      p4_program_state->accton_diag_lib_handle = NULL;
      printf("bf_switchd: Mav diag library %s unloaded for dev_id %d\n",
             p4_programs->accton_diag,
             dev_id);
    }
  }
}

/* De-Initialize diag library */
static void bf_switchd_diag_lib_deinit(bf_dev_id_t dev_id) {
  int j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);
  char *fn_name = "bf_diag_deinit";
  int (*diag_deinit_fn)();

  if (switchd_ctx->skip_p4) {
    printf("Skip diag lib deinit\n");
    return;
  }

  if (!state) {
    return;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);

    if (p4_program_state->diag_lib_handle == NULL) {
      continue;
    }

    /* Deinitialize the diag library */
    *(pvoid_dl_t *)(&diag_deinit_fn) =
        dlsym(p4_program_state->diag_lib_handle, fn_name);
    if (diag_deinit_fn) {
      diag_deinit_fn();
    }
    /* Currently the diag libraries are initialized only once */
    return;
  }
}

/* Unload switch.p4 specific libraries */
static void bf_switchd_p4_switch_lib_unload(bf_dev_id_t dev_id) {
  int i = 0, j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);
  bf_dev_init_mode_t init_mode = switchd_ctx->init_mode;
  bool warm_boot = (init_mode != BF_DEV_INIT_COLD) ? true : false;
  int (*switchapi_stop_srv_fn)(void) = NULL;
  int (*switchapi_stop_packet_driver_fn)() = NULL;
  int (*switchlink_stop_fn)() = NULL;
  int (*switchsai_stop_srv_fn)(void) = NULL;
  int (*switchapi_deinit_fn)(int, bool, char *) = NULL;
  if (!p4_device || !state) {
    return;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    p4_programs_t *p4_programs = &(p4_device->p4_programs[j]);
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);
    /* Stop the switch api rpc server */
    if (p4_program_state->switchapi_lib_handle) {
      *(pvoid_dl_t *)(&switchapi_stop_srv_fn) = dlsym(
          p4_program_state->switchapi_lib_handle, "stop_switch_api_rpc_server");
      if (switchapi_stop_srv_fn) {
        switchapi_stop_srv_fn();
      }

      /* Stop the switch api packet driver */
      *(pvoid_dl_t *)(&switchapi_stop_packet_driver_fn) =
          dlsym(p4_program_state->switchapi_lib_handle,
                "stop_switch_api_packet_driver");
      if (switchapi_stop_packet_driver_fn) {
        switchapi_stop_packet_driver_fn();
      }
    }
    /* Stop the switch link thread */
    if (p4_program_state->switchlink_lib_handle) {
      *(pvoid_dl_t *)(&switchlink_stop_fn) =
          dlsym(p4_program_state->switchlink_lib_handle, "switchlink_stop");
      if (switchlink_stop_fn) {
        switchlink_stop_fn();
      }
    }
    /* Stop the switch SAI thrift server */
    if (p4_program_state->switchsai_lib_handle) {
      *(pvoid_dl_t *)(&switchsai_stop_srv_fn) =
          dlsym(p4_program_state->switchsai_lib_handle,
                "stop_p4_sai_thrift_rpc_server");
      if (switchsai_stop_srv_fn) {
        switchsai_stop_srv_fn();
      }
    }
    /* Deinit Switch APi */
    if (p4_program_state->switchapi_lib_handle) {
      *(pvoid_dl_t *)(&switchapi_deinit_fn) =
          dlsym(p4_program_state->switchapi_lib_handle, "switch_api_free");
    }

    if (switchapi_deinit_fn) {
      switchapi_deinit_fn(dev_id, false, NULL);
    } else {
      if (p4_program_state->switchapi_lib_handle) {
        *(pvoid_dl_t *)(&switchapi_deinit_fn) =
            dlsym(p4_program_state->switchapi_lib_handle, "bf_switch_clean");
      }

      if (switchapi_deinit_fn) {
        switchapi_deinit_fn(dev_id, warm_boot, "/tmp/db.txt");
      }
    }

    /* Unload the switch API lib */
    if (p4_program_state->switchapi_lib_handle) {
      i = dlclose(p4_program_state->switchapi_lib_handle);
      char *err = dlerror();
      if (i != 0)
        printf("bf_switchd: library %s unload error %d/%s\n",
               p4_programs->switchapi,
               i,
               (err) ? err : "NONE");
      p4_program_state->switchapi_lib_handle = NULL;
      printf("bf_switchd: library %s unloaded for dev_id %d\n",
             p4_programs->switchapi,
             dev_id);
    }

    /* Unload the switch SAI lib */
    if (p4_program_state->switchsai_lib_handle) {
      dlclose(p4_program_state->switchsai_lib_handle);
      p4_program_state->switchsai_lib_handle = NULL;
      printf("bf_switchd: library %s unloaded for dev_id %d\n",
             p4_programs->switchsai,
             dev_id);
    }

    /* Unload the switch LINK lib */
    if (p4_program_state->switchlink_lib_handle) {
      dlclose(p4_program_state->switchlink_lib_handle);
      p4_program_state->switchlink_lib_handle = NULL;
      printf("bf_switchd: library %s unloaded for dev_id %d\n",
             p4_programs->switchlink,
             dev_id);
    }
  }
}

/* Load switch.p4 specific libraries */
static int bf_switchd_p4_switch_lib_load(bf_dev_id_t dev_id) {
  int j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);

  printf("bf_switchd: bf_switchd_p4_switch_lib_load \n");
  if (!p4_device || !state || switchd_ctx->skip_p4) {
    return 0;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    p4_programs_t *p4_programs = &(p4_device->p4_programs[j]);
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);

    /* Load the switch API lib */
    printf("bf_switchd: bf_switchd_p4_switch_lib_load \n");
    if (p4_programs && strlen(p4_programs->switchapi)) {
      p4_program_state->switchapi_lib_handle =
          bf_switchd_lib_load(p4_programs->switchapi);
      if (p4_program_state->switchapi_lib_handle == NULL) {
        return -1;
      }
    }

    printf("bf_switchd: loading the SAI lib \n");
    /* Load the switch SAI lib */
    if (p4_programs && strlen(p4_programs->switchsai)) {
      p4_program_state->switchsai_lib_handle =
          bf_switchd_lib_load(p4_programs->switchsai);
    }

    /* Load the switch LINK lib */
    if (p4_programs && strlen(p4_programs->switchlink)) {
      p4_program_state->switchlink_lib_handle =
          bf_switchd_lib_load(p4_programs->switchlink);
    }
  }

  return 0;
}

/* Initialize diag library */
static void bf_switchd_diag_lib_init(bf_dev_id_t dev_id) {
  int j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);
  char *fn_name = "bf_diag_init";
  int (*diag_init_fn)();

  if (switchd_ctx->skip_p4) {
    printf("Skip diag lib init\n");
    return;
  }

  if (!state) {
    return;
  }
  if (dev_id != 0) {
    return;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);
    p4_programs_t *p4_programs = &(p4_device->p4_programs[j]);

    if (p4_program_state->diag_lib_handle == NULL) {
      continue;
    }

    /* Initialize the diag library */
    *(pvoid_dl_t *)(&diag_init_fn) =
        dlsym(p4_program_state->diag_lib_handle, fn_name);
    if (diag_init_fn) {
      const char *cpu_port = NULL;

      if (p4_programs->use_eth_cpu_port) {
        cpu_port = p4_programs->eth_cpu_port_name;
      }
      diag_init_fn(cpu_port);
      /* Currently the diag libraries need to be initialized only once */
      return;
    }
  }
}

/* Initialize accton_diag library */
static void bf_switchd_accton_diag_lib_init(bf_dev_id_t dev_id) {
  int j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);
  char fn_name[250];
  agent_init_fn_t agent_init_fn;
  int ret = 0;

  if (switchd_ctx->skip_p4) {
    printf("Skip mav diag lib init\n");
    return;
  }

  if (!state) {
    return;
  }
  if (dev_id != 0) {
    return;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);

    if (p4_program_state->accton_diag_lib_handle == NULL) {
      continue;
    }

    /* Initialize the mav diag library */
    snprintf(fn_name, 250, "bf_switchd_agent_init");
    *(pvoid_dl_t *)(&agent_init_fn) =
        dlsym(p4_program_state->accton_diag_lib_handle, fn_name);
    if (agent_init_fn) {
      /* Start the agent thread */
      pthread_attr_t agent_t_attr;
      pthread_attr_init(&agent_t_attr);
      ret = pthread_create(
          &(switchd_ctx->accton_diag_t_id), &agent_t_attr, agent_init_fn, NULL);
      pthread_attr_destroy(&agent_t_attr);
      if (ret != 0) {
        printf("ERROR: thread creation failed for accton_diag service: %d\n",
               ret);
        return;
      }
      pthread_setname_np(switchd_ctx->accton_diag_t_id, "bf_mdiag_srv");
      printf("bf_switchd: accton_diag initialized\n");

      /* Currently the mav diag libraries need to be initialized only once */
      return;
    } else {
      printf("ERROR: bf_switchd: accton_diag init function not found \n");
    }
  }
}
#endif  // STATIC_LINK_LIB

/* Initialize P4 specific libraries */
static void bf_switchd_p4_lib_init(bf_dev_id_t dev_id) {
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);

  if (!state) {
    return;
  }
  if (switchd_ctx->skip_p4) {
    printf("Skip p4 lib init\n");
    return;
  }

#ifdef STATIC_LINK_LIB
  /* Initialize the switch API lib (only expected for switch.p4) */
  bf_switchd_switchapi_lib_init(dev_id);

  /* Initialize the switch SAI lib (only expected for switch.p4) */
  bf_switchd_switchsai_lib_init(dev_id);
#else  // STATIC_LINK_LIB

  /* Initialize the switch API lib (only expected for switch.p4) */
  bf_switchd_switchapi_lib_init(dev_id);

  /* Initialize the switch SAI lib (only expected for switch.p4) */
  bf_switchd_switchsai_lib_init(dev_id);
  /* Initialize the switch LINK lib (only expected for switch.p4) */
  bf_switchd_switchlink_lib_init(dev_id);
  /* Initialize the diag lib */
  bf_switchd_diag_lib_init(dev_id);
#endif  // STATIC_LINK_LIB
}

#ifndef STATIC_LINK_LIB
/* Load diag specific libraries */
static int bf_switchd_diag_lib_load(bf_dev_id_t dev_id) {
  int j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);

  if (!p4_device || !state) {
    return 0;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    p4_programs_t *p4_program = &(p4_device->p4_programs[j]);
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);

    if (p4_program && strlen(p4_program->diag)) {
      p4_program_state->diag_lib_handle = bf_switchd_lib_load(p4_program->diag);
      if (p4_program_state->diag_lib_handle == NULL) {
        return -1;
      }

      /* Load diag library only once per device */
      return 0;
    }
  }
  return 0;
}

/* Unload diag specific libraries */
static void bf_switchd_diag_lib_unload(bf_dev_id_t dev_id) {
  int j = 0;
  p4_devices_t *p4_device = &(switchd_ctx->p4_devices[dev_id]);
  switchd_state_t *state = &(switchd_ctx->state[dev_id]);

  bf_switchd_diag_lib_deinit(dev_id);

  if (!p4_device || !state) {
    return;
  }

  for (j = 0; j < p4_device->num_p4_programs; j++) {
    p4_programs_t *p4_programs = &(p4_device->p4_programs[j]);
    switchd_p4_program_state_t *p4_program_state =
        &(state->p4_programs_state[j]);

    if (p4_program_state->diag_lib_handle) {
      dlclose(p4_program_state->diag_lib_handle);
      p4_program_state->diag_lib_handle = NULL;
      printf("bf_switchd: library %s unloaded for dev_id %d\n",
             p4_programs->diag,
             dev_id);
    }
  }
}
#endif  // STATIC_LINK_LIB

static bf_status_t bf_switchd_warm_init_begin(
    bf_dev_id_t dev_id,
    bf_dev_init_mode_t warm_init_mode,
    bool upgrade_agents) {
  if (dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (upgrade_agents && (warm_init_mode == BF_DEV_WARM_INIT_FAST_RECFG_QUICK)) {
    upgrade_agents = false;
  }

  /* Only if device is ready do fast-reconfig */
  if (switchd_ctx->state[dev_id].device_ready == false) {
    return BF_NOT_READY;
  }

  /* Set the switchd_ctx->init_mode to the appropriate mode. So that the
   * agent libraries know of the init mode via the passed in switchd_ctx
   */
  switchd_ctx->init_mode = warm_init_mode;

  printf(
      "bf_switchd: starting warm init%s for dev_id %d mode %d"
      "\n",
      (warm_init_mode == BF_DEV_WARM_INIT_FAST_RECFG_QUICK) ? " quick" : "",
      dev_id,
      warm_init_mode);

  bf_sys_mutex_lock(&switchd_ctx->asic[dev_id].switch_mutex);
  switchd_ctx->state[dev_id].device_locked = true;
  bf_sys_mutex_unlock(&switchd_ctx->asic[dev_id].switch_mutex);

  #ifndef STATIC_LINK_LIB
  /* Unload P4 Libs */
  /* TODO: STATIC_LINK_LIB_TODO - Fix this for STATIC_LINK_LIB case */
  if (warm_init_mode != BF_DEV_WARM_INIT_FAST_RECFG_QUICK) {
    bf_switchd_p4_switch_lib_unload(dev_id);
    bf_switchd_diag_lib_unload(dev_id);

    /* Unload the mav diag library. */
    bf_switchd_accton_diag_lib_unload(dev_id);
  }
#endif  // STATIC_LINK_LIB

  bf_device_warm_init_begin(dev_id, warm_init_mode);

  if (warm_init_mode != BF_DEV_WARM_INIT_FAST_RECFG_QUICK) {
  }

  return BF_SUCCESS;
}

static bf_status_t bf_switchd_device_add_with_p4(
    bf_dev_id_t dev_id, bf_device_profile_t *device_profile) {
  if (dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (device_profile) {
    switchd_ctx->skip_p4 = (device_profile->num_p4_programs == 0);

    /* Update the switchd context with the information from the new profile
     * since bf_switchd_device_add will construct a new bf_device_profile_t from
     * the switchd context. */
    bf_p4_pipeline_t *pipeline_profile =
        &(device_profile->p4_programs[0].p4_pipelines[0]);
    struct stat stat_buf;
    bool absolute_paths = (stat(pipeline_profile->cfg_file, &stat_buf) == 0);
    switch_p4_pipeline_config_each_program_update(
        &switchd_ctx->p4_devices[dev_id],
        device_profile,
        switchd_ctx->install_dir,
        absolute_paths);
  }

#ifndef STATIC_LINK_LIB
  int ret = 0;

    /* Load Diag */
    ret = bf_switchd_diag_lib_load(dev_id);
    if (ret != 0) {
      return ret;
    }
    /* Load the mav diag libs */
    ret = bf_switchd_accton_diag_lib_load(dev_id);
    if (ret != 0) {
      return ret;
    }
    /* Load the switch libs */
    ret = bf_switchd_p4_switch_lib_load(dev_id);
    if (ret != 0) {
      return ret;
    }
#endif  // STATIC_LINK_LIB

  bf_status_t status = BF_SUCCESS;
  /* Add the device */
  status = bf_switchd_device_add(dev_id, false);
  if (status != BF_SUCCESS) {
    printf("ERROR: bf_switchd_device_add failed(%d) for dev_id %d\n",
           status,
           dev_id);
    return status;
  }


  /* Initialize P4 specific libraries for the device
  */
  if (switchd_ctx->p4_devices[dev_id].configured) {
    bf_switchd_p4_lib_init(dev_id);
#ifndef STATIC_LINK_LIB
    bf_switchd_accton_diag_lib_init(dev_id);
#endif  // STATIC_LINK_LIB
  }

  return status;
}

static bf_status_t bf_switchd_warm_init_port_bring_up(bf_dev_id_t dev_id) {
  bf_status_t status  = BF_SUCCESS;
#if 0
  unsigned int pipe = 0, port;
  bf_dev_port_t dev_port;
  bool link_down;
    for (port = 0; port < BF_PIPE_PORT_COUNT; port += 1) {
      dev_port = MAKE_DEV_PORT(pipe, port);
      // Assuming no-delta before and after replay of the config -
      // Link will be down, if port was not enabled before warm-init
      // or enabled but was in down-state. Move fsm to init state.
      // If link is up. move the fsm to link-down monitoring state.
      status = bf_device_warm_init_port_bring_up(dev_id, dev_port, &link_down);
  }
#endif
  return status;
}

bf_status_t bf_switchd_warm_init_end(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;

  if (dev_id >= BF_MAX_DEV_COUNT) {
    return status;
  }

  if (switchd_ctx->state[dev_id].device_locked) {
    status = bf_device_warm_init_end(dev_id);
    if (status != BF_SUCCESS) {
      printf("Error encountered: Force cold-reset for dev_id %d \n", dev_id);
    }
    if (!switchd_ctx->asic[dev_id].is_virtual &&
        !switchd_ctx->asic[dev_id].is_sw_model) {
      status = bf_switchd_warm_init_port_bring_up(dev_id);
      if (status != BF_SUCCESS) {
        printf(
            "Error encountered while bringing up ports in warm init end: Force "
            "cold-reset for dev_id %d\n",
            dev_id);
      }
    }
    switchd_ctx->state[dev_id].device_locked = false;
  }

  /* Warm init done.  Reset init_mode to INIT_COLD
   */
  if (switchd_ctx->init_mode == BF_DEV_WARM_INIT_FAST_RECFG ||
      switchd_ctx->init_mode == BF_DEV_WARM_INIT_FAST_RECFG_QUICK ||
      switchd_ctx->init_mode == BF_DEV_WARM_INIT_HITLESS) {
    switchd_ctx->init_mode = BF_DEV_INIT_COLD;
  }

  return status;
}

/**
 * Initialize the driver CLI shell and the switchd uCLI node. This does not
 * actually start the shell; call bf_switchd_drv_shell_start() afterwards
 * to begin processing interactive input.
 */
static int bf_switchd_drv_shell_init() {
  int ret = 0;

  /* Initialize uCLI and the driver shell */
  ret = bf_drv_shell_init();
  if (ret != 0) {
    printf("ERROR: bf-driver shell initialization failed : %d\n", ret);
    return ret;
  }

  /* Register switchd uCLI node */
  switchd_register_ucli_node();

  return 0;
}

/**
 * Function that creates copies of the p4 program names and stores
 * them in a char* array. The caller of this function is responsible
 * for freeing the memory associated with the strings and the array.
 * The array of char* is itself null-ptr terminated.
 */
static char **bf_switchd_cpy_p4_names(p4_devices_t *p4_devices) {
  bf_dev_id_t dev_id;
  int num_cfg = 0;
  for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
    if (p4_devices[dev_id].configured == 0) continue;
    num_cfg++;
  }

  char **p4_names = malloc((num_cfg + 2) * sizeof(char *));
  if (!p4_names) {
    printf("bf_switchd: Failed to allocate memory for p4_names\n");
    return NULL;
  }

  // Null terminate, put extra placeholder for switchapi
  p4_names[num_cfg + 1] = NULL;
  p4_names[num_cfg] = NULL;
  int cur_dev = 0;
  for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
    if (p4_devices[dev_id].configured == 0) continue;
    p4_names[cur_dev] =
        bf_sys_strdup(p4_devices[dev_id].p4_programs[0].program_name);
    // We add a value for switchapi only if its library has been loaded
    /* We must be careful when free-ing this array, as this value is not
       stored in the heap. */
    if (strcmp(p4_devices[dev_id].p4_programs[0].model_json_path, "") != 0) {
      /* this indicates bf_switchd running p4_16 stack */
      p4_names[num_cfg] = "bf_switchapi~";
    } else if (strcmp("switch",
                      p4_devices[dev_id].p4_programs[0].program_name) == 0 &&
               strcmp(p4_devices[dev_id].p4_programs[0].switchapi, "") != 0) {
      p4_names[num_cfg] = "switchapi~";  // ~ to reduce collision chance
    }
    cur_dev++;
  }
  return p4_names;
}

/**
 * Function to start the driver CLI shell. You must call
 * bf_switchd_drv_shell_init() before calling this function.
 */
static int bf_switchd_drv_shell_start(char *install_dir,
                                      bool run_background,
                                      bool run_ucli,
                                      bool local_only,
                                      p4_devices_t *p4_devices) {
  int ret = 0;

  /* Note that the CLI thread is responsible for free-ing this data */
  char **p4_names = bf_switchd_cpy_p4_names(p4_devices);
  char *install_dir_path = calloc(1024, sizeof(char));
  if (!install_dir_path) {
    printf("bf_switchd: Failed to allocate memory for install_dir_path\n");
    if (p4_names)
        free(p4_names);
    return -1;
  }

  /* Start the CLI server thread */
  printf("bf_switchd: spawning cli server thread\n");
  snprintf(install_dir_path, 1023, "%s/", install_dir);
  cli_thread_main(install_dir_path, p4_names, local_only);

  if (!run_background) {
    printf("bf_switchd: spawning driver shell\n");
    /* Start driver shell */
    if (run_ucli) {
      ret = bf_drv_shell_start();
      if (ret != 0) {
        printf("ERROR: uCLI instantiation failed : %d\n", ret);
	if (p4_names)
	    free(p4_names);
        return ret;
      }
    } else {
      cli_run_bfshell();
    }
  } else {
    printf("bf_switchd: running in background; driver shell is disabled\n");
  }
  printf("bf_switchd: server started - listening on port 9999\n");

  return 0;
}

static void bf_switchd_default_device_type_set(void) {
  bf_dev_id_t dev_id;

  /* Set the default device type to MODEL */
  switchd_ctx->is_sw_model = true;
  switchd_ctx->is_asic = false;


  /* Set the device type to MODEL for all devices */
  for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
    if (switchd_ctx->p4_devices[dev_id].configured == 0) {
      continue;
    }

    switchd_ctx->asic[dev_id].is_sw_model = true;
  }

  return;
}

int bf_switchd_device_type_update(bf_dev_id_t dev_id, bool is_sw_model) {
  /*
   * Update the global context flags if this is the first device. For
   * subsequent devices, check if mixed set of devices exist.
   */
  if (switchd_ctx->is_sw_model == false && switchd_ctx->is_asic == false) {
    switchd_ctx->is_sw_model = is_sw_model;
    switchd_ctx->is_asic = (!is_sw_model);
    printf("Operational mode set to %s\n", is_sw_model ? "DPDK MODEL" : "ASIC");
  } else if (is_sw_model != switchd_ctx->is_sw_model) {
    /* Return error as mixed set of devices is not supported */
    printf("ERROR: Mixed set of devices found (both model & asic)\n");
    return -1;
  }

  /* Update the type of device in per asic struct */
  switchd_ctx->asic[dev_id].is_sw_model = is_sw_model;

  return 0;
}

static int bf_switchd_pal_device_type_get(void) {
  bf_dev_id_t dev_id;
  int ret;
  bool is_sw_model = true;
  /*
   * Check for PAL handler registration to get device type and
   * if registered, update the device type for all devices.
   */
  for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
    if (switchd_ctx->p4_devices[dev_id].configured == 0) {
      continue;
    }
    /* Update the device type. If mixed set of devices found, return error */
    ret = bf_switchd_device_type_update(dev_id, is_sw_model);
    if (ret != 0) {
      return -1;
    }
  }

  return 0;
}

/*
 * Set device type to sw_model for dpdk
 * For ASIC it needs to be obtained from platfm/pal functions
 */
static int bf_switchd_device_type_get(void) {
  int ret = 0;

  /* Check the PAL handler registration to get device type */
  ret = bf_switchd_pal_device_type_get();
  if (ret != 0) {
    return -1;
  }

  /* Return if device types are identified in PAL handler */
  if (switchd_ctx->is_sw_model == true || switchd_ctx->is_asic == true) {
    printf("Initialized the device types using PAL handler registration\n");
    return 0;
  }

  /* Return if device types are indentified using pltfm infra agent lib */
  if (switchd_ctx->is_sw_model == true || switchd_ctx->is_asic == true) {
    printf("Initialized the device types using platforms infra API\n");
    return 0;
  }

 /* If device types are not identified still, set it to model by default */
  bf_switchd_default_device_type_set();

  return 0;
}

/* Initialize bf-driver libraries */
static int bf_switchd_driver_init(bool kernel_pkt_proc) {
  int ret = 0;

  /* Initialize the base driver */
  ret = bf_drv_init();
  if (ret != 0) {
    printf("ERROR: bf_drv_init failed : %d\n", ret);
    return ret;
  }

  /* Initialize the Low Level Driver (LLD) */
  /* "master_mode" bf_lld_init is done by kernel packet processing module
   * if enabled. A command line option to switchd indicates that switchd must
   * only do "non-master-mode" bf_lld_init in that case.
   */
  ret =
      bf_lld_init();
  if (ret != 0) {
    printf("ERROR: bf_lld_init failed : %d\n", ret);
    return ret;
  }

  /*
   * All the mgrs can be skipped using --skip-hld. No need
   * to stub out
   */
  /* Initialize the Port Mgmt Driver (port_mgr) */
  if (!switchd_ctx->skip_hld.port_mgr) {
    ret = bf_port_mgr_init();
    if (ret != 0) {
      printf("ERROR: bf_port_mgr_init failed : %d\n", ret);
      return ret;
    }
  } else {
    printf("Skipped port-mgr init \n");
  }

  /* Initialize the P4 driver (PD) and pipe_mgr */
  if (!switchd_ctx->skip_hld.pipe_mgr) {
    ret = pipe_mgr_init();
    if (ret != 0) {
      printf("ERROR: pipe_mgr_init() failed : %d\n", ret);
      return ret;
    }
  } else {
    printf("Skipped pipe-mgr init \n");
  }
#ifdef BFRT_ENABLED
  /* Initialize the BF_RT module */
  if (!switchd_ctx->skip_hld.pipe_mgr) {
    uint32_t i = 0, dev_id = 0;
    bool init_done = false;
    // Initialize BF-RT if skip-p4 is on
    if (switchd_ctx->skip_p4) {
      ret = bf_rt_module_init(switchd_ctx->skip_hld.port_mgr);
      if (ret != 0) {
        printf("ERROR: bf_rt_init failed : %d\n", ret);
        return ret;
      }
      init_done = true;
    }
    for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
      p4_devices_t *p4_devices = &(switchd_ctx->p4_devices[dev_id]);
      if (!p4_devices->configured) {
        continue;
      }
      for (i = 0; i < p4_devices->num_p4_programs; i++) {
        p4_programs_t *p4_programs = &(p4_devices->p4_programs[i]);
        if (strlen(p4_programs->bfrt_config) && !init_done) {
          // Initialize bfrt only if bfrt json file is mentioned in the conf
          // file
          ret = bf_rt_module_init(switchd_ctx->skip_hld.port_mgr);
          if (ret != 0) {
            printf("ERROR: bf_rt_init failed : %d\n", ret);
            return ret;
          }
          init_done = true;
          break;
        }
      }
      if (init_done) {
        break;
      }
    }
  }
#endif

  return 0;
}

/* Process bf_switchd conf-file */
static int bf_switchd_load_switch_conf_file() {
  if (!switchd_ctx->conf_file) {
    printf("ERROR: invalid switchd configuration file\n");
    return -1;
  }

  printf("bf_switchd: loading conf_file %s...\n", switchd_ctx->conf_file);

  return switch_dev_config_init(
      switchd_ctx->install_dir, switchd_ctx->conf_file, switchd_ctx);
}

static void *bf_sys_timer_init_wrapper(void *arg) {
  (void)arg;
  bf_sys_timer_init();
  return NULL;
}
/* Initialize system services expected by drivers */
static int bf_switchd_sys_init(void) {
  int ret = 0;

  /* Initialize system timer service */
  static pthread_attr_t tmr_t_attr;
  pthread_attr_init(&tmr_t_attr);
  if ((ret = pthread_create(&switchd_ctx->tmr_t_id,
                            &tmr_t_attr,
                            bf_sys_timer_init_wrapper,
                            NULL)) != 0) {
    printf("ERROR: thread creation failed for system timer service: %d\n", ret);
    return ret;
  }
  pthread_setname_np(switchd_ctx->tmr_t_id, "bf_timer_src");

  char bf_sys_log_cfg_file[180] = {0};
  sprintf(
      bf_sys_log_cfg_file, "%s/share/target_sys/zlog-cfg", switchd_ctx->install_dir);
  printf("Install dir: %s (%p)\n",
         switchd_ctx->install_dir,
         (void *)switchd_ctx->install_dir);
  /* Initialize the logging service */
  if ((ret = bf_sys_log_init(
           bf_sys_log_cfg_file, (void *)BF_LOG_DBG, (void *)(32 * 1024))) !=
      0) {
    printf("ERROR: failed to initialize logging service!\n");
    return ret;
  }

  /* For performance set defaut log level to error for pipe and pkt modules */
  bf_sys_log_level_set(BF_MOD_PIPE, BF_LOG_DEST_FILE, BF_LOG_ERR);
  bf_sys_trace_level_set(BF_MOD_PIPE, BF_LOG_ERR);

#ifdef BFRT_ENABLED
  bf_sys_log_level_set(BF_MOD_BFRT, BF_LOG_DEST_FILE, BF_LOG_ERR);
  bf_sys_trace_level_set(BF_MOD_BFRT, BF_LOG_ERR);
#endif

  return 0;
}

bf_status_t bf_switchd_pltfm_type_get(bf_dev_id_t dev_id, bool *is_sw_model) {
  if (dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  *is_sw_model = switchd_ctx->asic[dev_id].is_sw_model;
  return BF_SUCCESS;
}

static int bf_switchd_pal_handler_reg() {
  bf_pal_dev_callbacks_t dev_callbacks;
  dev_callbacks.warm_init_begin = bf_switchd_warm_init_begin;
  dev_callbacks.device_add = bf_switchd_device_add_with_p4;
  dev_callbacks.warm_init_end = bf_switchd_warm_init_end;
  dev_callbacks.pltfm_type_get = bf_switchd_pltfm_type_get;
  bf_pal_device_callbacks_register(&dev_callbacks);
  return 0;
}

#ifdef GRPC_ENABLED

static void *bf_switchd_device_status_srv(void *arg) {
  bf_switchd_context_t *ctx = (bf_switchd_context_t *)arg;
  int optval = 1;
  int sock_fd;
  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr,
	    "Error: socket creation failed for switchd dev status %s\n",
	    strerror(errno));
    return NULL;
  }

  struct sockaddr_in switchd_addr;
  memset(&switchd_addr, 0, sizeof(switchd_addr));
  switchd_addr.sin_family = AF_INET;
  switchd_addr.sin_addr.s_addr = ctx->server_listen_local_only
                                     ? htonl(INADDR_LOOPBACK)
                                     : htonl(INADDR_ANY);
  switchd_addr.sin_port = htons(ctx->dev_sts_port);
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
      (const void *)&optval,
      sizeof(int)) < 0) {
    fprintf(stderr,
	    "Error: unable to set socket opt for switchd dev status %s\n",
	    strerror(errno));
    return NULL;
  }

  if (bind(sock_fd, (struct sockaddr *)&switchd_addr,
      sizeof(switchd_addr)) < 0 ) {
    fprintf(stderr,
	    "Error: bind socket failed for switchd dev status %s\n",
	    strerror(errno));
    return NULL;
  }

  if (listen(sock_fd, 1) < 0) {
    fprintf(stderr,
	    "Error: listen failed for switchd dev status %s\n",
	    strerror(errno));
    return NULL;
  }

  while (1) {
    int fd;
    if ((fd = accept(sock_fd, (struct sockaddr *)NULL, NULL)) < 0) {
      fprintf(stderr,
	      "Error: accept Failed for switchd dev status %s\n",
	      strerror(errno));
      return NULL;
    }
    char x[2] = {0};
    ssize_t rs = read(fd, x, 1);
    if (-1 == rs) {
      fprintf(stderr,
              "Read failure for device status request: %s\n",
              strerror(errno));
    }
    int dev_id = atoi(x);
    if (dev_id >= BF_MAX_DEV_COUNT) {
      fprintf(stderr, "Invalid device id: %d\n", dev_id);
      x[0] = '0';
    }
    else {
      x[0] = switchd_ctx->init_done && switchd_ctx->state[dev_id].device_ready
                 ? '1'
                 : '0';
    }
    ssize_t ws = write(fd, x, 1);
    if (-1 == ws) {
      fprintf(stderr,
              "Write failure for device status request: %s\n",
              strerror(errno));
    }
    close(fd);
  }
  return NULL;
}

#endif //GRPC_Enabled


/* bf_switchd main */
int bf_switchd_lib_init_local(void *ctx_local) {
  bf_switchd_context_t *ctx = (bf_switchd_context_t*)ctx_local;
  int ret = 0;
  bf_dev_id_t dev_id = 0;
  int num_active_devices = 0;

  if (ctx) {
    if ((switchd_ctx = malloc(sizeof(bf_switchd_context_t))) == NULL) {
      printf("ERROR: Failed to allocate memory for switchd context\n");
      return -1;
    }
    memcpy(switchd_ctx, ctx, sizeof(bf_switchd_context_t));
  } else {
    return -1;
  }

  #ifdef GRPC_ENABLED
  /* Status port thread creation */
  if (switchd_ctx->dev_sts_thread) {
    pthread_attr_t a;
    pthread_attr_init(&a);

    ret = pthread_create(&switchd_ctx->dev_sts_t_id,
		         &a,
                         bf_switchd_device_status_srv,
                         switchd_ctx);
    if (ret != 0) {
      printf("ERROR: thread creation failed for switchd dev status service %d\n",
	     ret);
      return ret;
    }

    pthread_attr_destroy(&a);
    pthread_setname_np(switchd_ctx->dev_sts_t_id, "bf_device_sts");
  }
  #endif

  /* Initialize system services */
  ret = bf_switchd_sys_init();
  if (ret != 0) {
    printf("ERROR: system services initialization failed : %d\n", ret);
    if (switchd_ctx) free(switchd_ctx);
    return ret;
  } else {
    printf("bf_switchd: system services initialized\n");
  }

  /* Load switchd configuration file */
  ret = bf_switchd_load_switch_conf_file();
  if (ret != 0) {
    printf("ERROR: loading conf_file failed : %d\n", ret);
    if (switchd_ctx) free(switchd_ctx);
    return ret;
  }

  /* Load P4 specific libraries associated with each device */
  for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
    if (switchd_ctx->p4_devices[dev_id].configured == 0) continue;
    if (!switchd_ctx->asic[dev_id].is_virtual) {
      /* Increment the count of the number of active devices */
      num_active_devices++;
    }

    ret = bf_sys_mutex_init(&switchd_ctx->asic[dev_id].switch_mutex);
    if (ret != BF_SUCCESS) {
      printf("ERROR: bf_sys_mutex_init failed (%d), switch_mutex, dev_id %d\n",
             ret,
             dev_id);
    }
  }

  /* Initialize the driver shell. We need to do this even if we're running in
   * the background, because the shell is accessible via telnet using clish. */
  ret = bf_switchd_drv_shell_init();
  if (ret != 0) {
    printf("ERROR: bf-driver shell initialization failed : %d\n", ret);
    if (switchd_ctx) free(switchd_ctx);
    return ret;
  }
  ret = bf_switchd_device_type_get();
  if (ret != 0) {
    printf("ERROR: Getting device type (model or asic) failed: %d\n", ret);
    return ret;
  }

  /* derive Asic pci sysfs string by going thru sysfs files in run time
   * instead of using the configured name in case of single device
   * program
   * is_asic is never set to TRUE for DPDK. This needs to be changed for Asic.
   * For now, it doesn't get executed
   */
  if (switchd_ctx->is_asic) {
    bool single_device = false;
    for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
      if ((switchd_ctx->asic[dev_id].configured == 0) ||
          (switchd_ctx->asic[dev_id].is_virtual)) {
        continue;
      } else {
        if (!single_device) {
          single_device = true;
        } else {
          single_device = false;
          break;
        }
      }
    }
  }

  /* Initialize driver libraries */
  ret = bf_switchd_driver_init(0);
  if (ret != 0) {
    printf("ERROR: bf-driver initialization failed : %d\n", ret);
    if (switchd_ctx) free(switchd_ctx);
    return ret;
  } else {
    printf("bf_switchd: drivers initialized\n");
  }

  if (switchd_ctx->shell_before_dev_add) {
    /* Start the driver shell before devices are added. */
    ret = bf_switchd_drv_shell_start(switchd_ctx->install_dir,
                                     switchd_ctx->running_in_background,
                                     switchd_ctx->shell_set_ucli,
                                     switchd_ctx->server_listen_local_only,
                                     switchd_ctx->p4_devices);
    if (ret != 0) {
      return ret;
    }
  }

  bf_status_t status = BF_SUCCESS;
  /* Initialize all devices and their ports */
  for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
    if (switchd_ctx->asic[dev_id].configured == 0) continue;

    /* Issue a warm-init-begin if switchd is starting in "HA mode". */
    if (switchd_ctx->init_mode == BF_DEV_WARM_INIT_FAST_RECFG ||
        switchd_ctx->init_mode == BF_DEV_WARM_INIT_HITLESS) {
      switchd_ctx->state[dev_id].device_locked = true;
      status = bf_device_warm_init_begin(
          dev_id, switchd_ctx->init_mode);
      if (BF_SUCCESS != status) {
        printf("ERROR: bf_device_warm_init_begin returned %d for dev %d\n",
               status,
               dev_id);
      }
    }
    bf_pal_create_port_info(dev_id);
    /* Add device */
    status = bf_switchd_device_add(dev_id, false);
    if (status != BF_SUCCESS) {
      printf("ERROR: bf_switchd_device_add failed(%d) for dev_id %d\n",
             status,
             dev_id);
      continue;
    }
    switchd_ctx->no_of_devices++;

    printf("\nbf_switchd: dev_id %d initialized\n", dev_id);
  }

  if (switchd_ctx->init_mode == BF_DEV_WARM_INIT_FAST_RECFG ||
      switchd_ctx->init_mode == BF_DEV_WARM_INIT_HITLESS) {
    printf(
        "Resetting init mode to cold-boot since all devices have completed "
        "warm-init\n");
    switchd_ctx->init_mode = BF_DEV_INIT_COLD;
  }
  printf("\nbf_switchd: initialized %d devices\n", switchd_ctx->no_of_devices);

  // Register the functions against the interfaces in bf_pal layer
  if (bf_switchd_pal_handler_reg() != 0) {
    printf("Unable to register the functions to be used by switchapi\n");
  }

  /* Initialize P4 specific libraries for all devices */
  for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
    if (switchd_ctx->p4_devices[dev_id].configured == 0) continue;
    bf_switchd_p4_lib_init(dev_id);
#ifndef STATIC_LINK_LIB
    bf_switchd_accton_diag_lib_init(dev_id);
#endif  // STATIC_LINK_LIB

  }

  if (!switchd_ctx->shell_before_dev_add) {
    /* Start the driver shell now that devices are added. */
    ret = bf_switchd_drv_shell_start(switchd_ctx->install_dir,
                                     switchd_ctx->running_in_background,
                                     switchd_ctx->shell_set_ucli,
                                     switchd_ctx->server_listen_local_only,
                                     switchd_ctx->p4_devices);
    if (ret != 0) {
      return ret;
    }
  }

  /* Flag that our init is now complete. */
  __asm__ __volatile__("" ::: "memory");  // Don't let this be moved earlier
  switchd_ctx->init_done = true;

  ctx->non_default_port_ppgs = switchd_ctx->non_default_port_ppgs;
  ctx->sai_default_init = switchd_ctx->sai_default_init;
  /* Join with the created threads */
  ctx->tmr_t_id = switchd_ctx->tmr_t_id;
  ctx->accton_diag_t_id = switchd_ctx->accton_diag_t_id;
  for (int device = 0; device < BF_MAX_DEV_COUNT; device++) {
    if (!switchd_ctx->p4_devices[device].configured) continue;
    for (int program = 0;
         program < switchd_ctx->p4_devices[device].num_p4_programs;
         program++) {
      ctx->p4_devices[device].p4_programs[program].use_eth_cpu_port =
          switchd_ctx->p4_devices[device].p4_programs[program].use_eth_cpu_port;
      if (ctx->p4_devices[device].p4_programs[program].use_eth_cpu_port) {
        strcpy(ctx->p4_devices[device].p4_programs[program].eth_cpu_port_name,
               switchd_ctx->p4_devices[device]
                   .p4_programs[program]
                   .eth_cpu_port_name);
      }
    }
  }

// Starting GRPC servers only after device add is done
#ifdef GRPC_ENABLED
#ifdef BFRT_ENABLED
  bf_rt_grpc_server_run(switchd_ctx->p4_devices[0].p4_programs[0].program_name,
                        switchd_ctx->server_listen_local_only);
#endif
#endif

#ifdef P4RT_ENABLED
  if (switchd_ctx->p4rt_server) {
    BfP4RtServerRunAddr(switchd_ctx->p4rt_server);
    printf("P4Runtime GRPC server started on %s\n", switchd_ctx->p4rt_server);
  }
#endif

  if (0) bf_switchd_ports_delete(0);  // Avoid the compiler warning

  return ret;
}

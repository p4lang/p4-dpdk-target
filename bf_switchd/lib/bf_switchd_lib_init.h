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

#ifndef __BF_SWITCHD_LIB_INIT_H__
#define __BF_SWITCHD_LIB_INIT_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
/* OSAL includs */
#include <bf_types/bf_types.h>
#include <osdep/p4_sde_osdep.h>
#include <dvm/bf_drv_profile.h>
#include <dvm/bf_drv_intf.h>

#define BF_SWITCHD_MAX_FILE_NAME 512

typedef struct switch_mac_s {
  int configured;  // set from switch config json parsing
} switch_mac_t;

typedef struct switchd_pcie_cfg_s {
  char bdf[PCIE_DOMAIN_BDF_LEN];
} switchd_pcie_cfg_t;

typedef struct switch_asic_s {
  int pci_dev_id;
  int configured;
  bf_dev_family_t chip_family;
  bool is_virtual;
  bool is_virtual_dev_slave;
  bool is_sw_model;
  int iommu_grp_num;
  int host_id; 
  int pf_num;
  switchd_pcie_cfg_t pcie_cfg;
  char bfrt_non_p4_json_dir_path[BF_SWITCHD_MAX_FILE_NAME];
  bf_sys_mutex_t switch_mutex;
} switch_asic_t;

#define BF_SWITCHD_MAX_PIPES 4
#define BF_SWITCHD_MAX_MEMPOOL_OBJS 2
#define BF_SWITCHD_MAX_P4_PROGRAMS BF_SWITCHD_MAX_PIPES
#define BF_SWITCHD_MAX_P4_PIPELINES BF_SWITCHD_MAX_PIPES
typedef struct p4_pipeline_config_s {
  char *p4_pipeline_name;
  char table_config[BF_SWITCHD_MAX_FILE_NAME];
  char cfg_file[BF_SWITCHD_MAX_FILE_NAME];
  char pi_native_config_path[BF_SWITCHD_MAX_FILE_NAME];
  int core_id;
  int numa_node;
  int num_pipes_in_scope;
  int pipe_scope[BF_SWITCHD_MAX_PIPES];
} p4_pipeline_config_t;

typedef struct p4_programs_s {
  char *program_name;
  char diag[BF_SWITCHD_MAX_FILE_NAME];
  char accton_diag[BF_SWITCHD_MAX_FILE_NAME];
  char switchapi[BF_SWITCHD_MAX_FILE_NAME];
  char switchsai[BF_SWITCHD_MAX_FILE_NAME];
  char switchlink[BF_SWITCHD_MAX_FILE_NAME];
  bool add_ports_to_switchapi;
  char model_json_path[BF_SWITCHD_MAX_FILE_NAME];
  bool use_eth_cpu_port;
  char eth_cpu_port_name[BF_SWITCHD_MAX_FILE_NAME];
  char bfrt_config[BF_SWITCHD_MAX_FILE_NAME];
  char port_config[BF_SWITCHD_MAX_FILE_NAME];
  int num_p4_pipelines;
  p4_pipeline_config_t p4_pipelines[BF_SWITCHD_MAX_P4_PIPELINES];
} p4_programs_t;

struct mempool_obj_s {
	char *name;
	int buffer_size;
	int pool_size;
	int cache_size;
	int numa_node;
};

typedef struct p4_devices_s {
  bool configured;
  bool debug_cli_enable;
  uint8_t num_mempool_objs;
  struct mempool_obj_s mempool_objs[BF_SWITCHD_MAX_MEMPOOL_OBJS];
  uint8_t num_p4_programs;
  p4_programs_t p4_programs[BF_SWITCHD_MAX_P4_PROGRAMS];
  char eal_args[MAX_EAL_LEN];
} p4_devices_t;

typedef struct switchd_p4_program_state_s {
  void *switchapi_lib_handle;
  void *switchsai_lib_handle;
  void *switchlink_lib_handle;
  void *diag_lib_handle;
  void *accton_diag_lib_handle;
} switchd_p4_program_state_t;

typedef struct switchd_state_s {
  switchd_p4_program_state_t p4_programs_state[BF_SWITCHD_MAX_P4_PROGRAMS];
  bool device_locked;
  bool device_ready;
  bool device_pktmgr_ready;
  bool device_pci_err;
} switchd_state_t;

typedef struct switchd_skip_hld_s {
  bool pipe_mgr;
  bool port_mgr;
} switchd_skip_hld_t;

#ifdef STATIC_LINK_LIB
typedef int32_t (*switch_api_init_fn_t)(uint16_t device_id,
                                        uint32_t num_ports,
                                        char *cpu_port,
                                        bool add_ports,
                                        void *rpc_server_cookie);
typedef int32_t (*switch_api_deinit_fn_t)(uint16_t device_id,
                                          void *rpc_server_cookie);

typedef void (*switch_sai_init_fn_t)();

typedef int (*bf_pltfm_device_type_get_fn_t)(bf_dev_id_t dev_id,
                                             bool *is_sw_model);
#endif  // STATIC_LINK_LIB

typedef struct bf_switchd_context_s {
  /* Per device context */
  switch_asic_t asic[BF_MAX_DEV_COUNT];
  p4_devices_t p4_devices[BF_MAX_DEV_COUNT];
  switchd_state_t state[BF_MAX_DEV_COUNT];
  /* Global context */
  bf_dev_init_mode_t init_mode;
  int no_of_devices;
  char *install_dir;   // install directory for BF-SDE build
  char *conf_file;     // switchd conf_file
  bool skip_p4;        // Skip P4 config load
  bool skip_port_add;  // Skip port-add for all bf-driver-mgrs including lld
  switchd_skip_hld_t skip_hld;  // Skip some high level mgr
  bool is_sw_model;             // Flag to identify operating mode (for model)
  bool is_virtual_dev_slave;    // Flag to identify if a device is a virtual
                                // device slave
  bool is_asic;                 // Flag to identify operating mode (for asic)
  bool dev_sts_thread;
  uint16_t dev_sts_port;
  bool running_in_background;  // Do we expect our process to be backgrounded?
  bool shell_set_ucli;         // Load uCLI as default shell instead of BFShell
  bool bfshell_local_only;     // Only allow bfshell connections on loopback.
  bool
      status_server_local_only;  // Only allow status server listen on loopback.
  bool regular_channel_server_local_only;  // Only allow regular channel server
                                           // listen on loopback.
  bool bf_rt_server_local_only;         // Only allow bfrt grpc server listen on
                                        // loopback.
  bool server_listen_local_only;        // Only allow
  // bfrt-grpc/status/dma/regular/fpga server
  // listen on loopback.
  bool shell_before_dev_add;  // Start shell before device-add is issued.
  bool init_done;
  pthread_t dev_sts_t_id;
  pthread_t tmr_t_id;
  pthread_t accton_diag_t_id;
  void *rpc_server_cookie;
  uint32_t num_active_devices;
  char *p4rt_server;  // Socket (addr:port) on which to run the P4RT server
  uint32_t non_default_port_ppgs;
  char board_port_map_conf_file[BF_SWITCHD_MAX_FILE_NAME];
  bool sai_default_init;  // Flag to create default ports, initialize port
                          // speed, vlan and buffer configs.

#ifdef STATIC_LINK_LIB
  switch_api_init_fn_t switch_api_init_fn;
  switch_api_deinit_fn_t switch_api_deinit_fn;
  switch_sai_init_fn_t switch_sai_init_fn;
  bf_pltfm_device_type_get_fn_t bf_pltfm_device_type_get_fn;
#endif  // STATIC_LINK_LIB
} bf_switchd_context_t;

int bf_switchd_lib_init(bf_switchd_context_t *ctx);

#endif /* __BF_SWITCHD_LIB_INIT_H__ */

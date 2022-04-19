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
#ifndef BF_DRV_PROFILE_H_INCLUDED
#define BF_DRV_PROFILE_H_INCLUDED

#include <bf_types/bf_types.h>

#define DEF_PROFILE_INDEX 0
#define DEF_PROGRAM_INDEX 0
#define DEF_PROFILE_ID 0
#define PROF_FUNC_NAME_LEN 200
#define PIPE_MGR_CFG_FILE_LEN 250
#define PIPE_MGR_DYN_LIB_STR_LENGTH 250

/**
 * @addtogroup dvm-device-mgmt
 * @{
 */
#define MAX_MEMPOOL_OBJS 2
#define MAX_P4_PIPELINES 4
#define PROG_NAME_LEN 256
#define MAX_PROGRAMS_PER_DEVICE MAX_P4_PIPELINES
#define PCIE_DOMAIN_BDF_LEN 16
#define IOMMU_GRP_NUM_LEN 6
#define MAX_EAL_LEN 512

struct mirror_config_s {
	/* Number of packet mirroring slots. */
	uint32_t n_slots;
	/* Maximum number of packet mirroring sessions. */
	uint32_t n_sessions;
	/* Flag to decide fast or slow mirroring.
	 * 0 (when zero, i.e. false) for slow mirroring,
	 * 1 (non-zero, i.e. true) for fast mirroring.
	 */
	int fast_clone;
};

typedef struct bf_p4_pipeline {
  char p4_pipeline_name[PROG_NAME_LEN];
  char *cfg_file;
  char *runtime_context_file;        // json context
  char *pi_config_file;              // json PI config
  int core_id;                 // core on which pipeline run
  int numa_node;         // pipeline uses mempool created this numa node
  int num_pipes_in_scope;            // num pipes in scope
  int pipe_scope[MAX_P4_PIPELINES];  // logical pipe list
  struct mirror_config_s mir_cfg;   // dpdk mirror profile cfg.
} bf_p4_pipeline_t;

typedef struct asic_fw_profile {
  char pcie_domain_bdf[PCIE_DOMAIN_BDF_LEN];
  char iommu_grp_num[IOMMU_GRP_NUM_LEN];
  int host_id; 
  int pf_num;
} asic_fw_profile_t;

typedef struct bf_p4_program {
  char prog_name[PROG_NAME_LEN];
  char *bfrt_json_file;  // bf-rt info json file
  char *port_config_json;  // port config json file
  uint8_t num_p4_pipelines;
  bf_p4_pipeline_t p4_pipelines[MAX_P4_PIPELINES];
} bf_p4_program_t;

struct bf_mempool_obj_s {
	char name[PROG_NAME_LEN];     // name of the mempool
	int buffer_size;   // buffer size in mempool
	int pool_size;     // number of buffers in mempool
	int cache_size;    // these many mempool buffers are available in cache
	int numa_node;     // mempool created on this numa socket
};

typedef struct bf_device_profile {
  bool debug_cli_enable;
  uint8_t num_p4_programs;
  bf_p4_program_t p4_programs[MAX_PROGRAMS_PER_DEVICE];
  uint8_t num_mempool_objs;
  struct bf_mempool_obj_s mempool_objs[MAX_MEMPOOL_OBJS];
  asic_fw_profile_t asic_prof;
  char *bfrt_non_p4_json_dir_path;  // bfrt fixed feature info json files path
  char eal_args[MAX_EAL_LEN]; // eal-args required for dpdk model
} bf_device_profile_t;

/* @} */

#endif  // BF_DRV_PROFILE_H_INCLUDED

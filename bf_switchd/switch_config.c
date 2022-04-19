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
#include "switch_config.h"
#include <cjson/cJSON.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <regex.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

int get_int(cJSON *item, const char *key) {
  cJSON *value = cJSON_GetObjectItem(item, key);
  assert(NULL != value);
  assert(cJSON_Number == value->type);
  return value->valueint;
}

const char *get_string(cJSON *item, const char *key) {
  cJSON *value = cJSON_GetObjectItem(item, key);
  assert(NULL != value);
  assert(cJSON_String == value->type);
  return value->valuestring;
}

uint32_t check_and_get_int(cJSON *item, const char *key, int default_value) {
  cJSON *value = cJSON_GetObjectItem(item, key);
  if (NULL != value)
    return get_int(item, key);
  else
    return default_value;
}

const char *check_and_get_string(cJSON *item, const char *key) {
  cJSON *value = cJSON_GetObjectItem(item, key);
  if (NULL != value)
    return get_string(item, key);
  else
    return "";
}

bool check_and_get_bool(cJSON *item, const char *key, bool default_value) {
  cJSON *value = cJSON_GetObjectItem(item, key);
  if (!value) return default_value;
  assert(value->type == cJSON_False || value->type == cJSON_True);
  return (value->type == cJSON_True);
}

static bool
check_number_is_power_of_two(int num)
{
	/* First num in the below expression is for the case when num is 0 */
	return num && (!(num & (num - 1)));
}

/**
 * @brief  This function copies details of each program in a bf_device_profile_t
 *  to a p4_devices_t.
 *
 * @param[out] p4_device
 * @param[in] device_profile
 * @param[in] install_dir base path in case of non-absolute path
 * @param[in] absolute_paths
 */
void switch_p4_pipeline_config_each_program_update(
    p4_devices_t *p4_device,
    bf_device_profile_t *device_profile,
    const char *install_dir,
    bool absolute_paths) {
  if (p4_device == NULL) {
    return;
  }
  p4_device->num_p4_programs = device_profile->num_p4_programs;
  for (uint8_t i = 0; i < device_profile->num_p4_programs; i++) {
    p4_programs_t *p4_program = &(p4_device->p4_programs[i]);
    bf_p4_program_t *bf_p4_program = &(device_profile->p4_programs[i]);

    // Reset
    if (p4_program->program_name) {
      free(p4_program->program_name);
    }
    p4_program->program_name = strndup(bf_p4_program->prog_name, PROG_NAME_LEN);

    if (bf_p4_program->bfrt_json_file) {
      if (absolute_paths) {
        snprintf(p4_program->bfrt_config,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s",
                 bf_p4_program->bfrt_json_file);
      } else {
        snprintf(p4_program->bfrt_config,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s/%s",
                 install_dir,
                 bf_p4_program->bfrt_json_file);
      }
    } else {
      // src is NULL. Make dst NULL too.
      p4_program->bfrt_config[0] = 0;
    }
    p4_program->num_p4_pipelines = bf_p4_program->num_p4_pipelines;
    switch_p4_pipeline_config_each_profile_update(
        p4_program, bf_p4_program, install_dir, absolute_paths);
  }
  return;
}

/**
 * @brief Copies into a bf_p4_program struct from a p4_programs
 * struct
 *
 * @param[out] p4_program
 * @param[in] bf_p4_program
 * @param[in] install_dir base path in case of non-absolute path
 * @param[in] absolute_paths
 */
void switch_p4_pipeline_config_each_profile_update(
    p4_programs_t *p4_program,
    bf_p4_program_t *bf_p4_program,
    const char *install_dir,
    bool absolute_paths) {
  for (int j = 0; j < p4_program->num_p4_pipelines; j++) {
    p4_pipeline_config_t *p4_pipeline = &(p4_program->p4_pipelines[j]);
    bf_p4_pipeline_t *bf_p4_pipeline = &(bf_p4_program->p4_pipelines[j]);
    // Reset
    if (p4_pipeline->p4_pipeline_name) {
      free(p4_pipeline->p4_pipeline_name);
      p4_pipeline->p4_pipeline_name = NULL;
    }
    p4_pipeline->cfg_file[0] = 0;
    p4_pipeline->table_config[0] = 0;
    p4_pipeline->pi_native_config_path[0] = 0;

    // Set
    if (bf_p4_pipeline->p4_pipeline_name[0] != '\0') {
      p4_pipeline->p4_pipeline_name =
          strndup(bf_p4_pipeline->p4_pipeline_name, PROG_NAME_LEN);
    }
    if (bf_p4_pipeline->cfg_file) {
      if (absolute_paths) {
        snprintf(p4_pipeline->cfg_file,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s",
                 bf_p4_pipeline->cfg_file);
      } else {
        snprintf(p4_pipeline->cfg_file,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s/%s",
                 install_dir,
                 bf_p4_pipeline->cfg_file);
      }
    }
    if (bf_p4_pipeline->runtime_context_file) {
      if (absolute_paths) {
        snprintf(p4_pipeline->table_config,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s",
                 bf_p4_pipeline->runtime_context_file);
      } else {
        snprintf(p4_pipeline->table_config,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s/%s",
                 install_dir,
                 bf_p4_pipeline->runtime_context_file);
      }
    }
    if (bf_p4_pipeline->pi_config_file) {
      if (absolute_paths) {
        snprintf(p4_pipeline->pi_native_config_path,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s",
                 bf_p4_pipeline->pi_config_file);
      } else {
        snprintf(p4_pipeline->pi_native_config_path,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s/%s",
                 install_dir,
                 bf_p4_pipeline->pi_config_file);
      }
    }
    p4_pipeline->num_pipes_in_scope = bf_p4_pipeline->num_pipes_in_scope;
    for (uint8_t k = 0; k < p4_pipeline->num_pipes_in_scope; k++) {
      p4_pipeline->pipe_scope[k] = bf_p4_pipeline->pipe_scope[k];
    }
  }
}

/* Check if the required path is absolute, and if not
 * prepend the install directory */
static void to_abs_path(char *dest,
                        cJSON *p4_program_obj,
                        const char *key,
                        const char *install_dir) {
  if (strlen(check_and_get_string(p4_program_obj, key))) {
    const char *path = check_and_get_string(p4_program_obj, key);
    if (path[0] != '/')  // absolute path -- don't prepend install_dir
      sprintf(dest, "%s/%s", install_dir, path);
    else
      sprintf(dest, "%s", path);
  }
}

static void switch_p4_pipeline_config_init(const char *install_dir,
                                           cJSON *switch_config_obj,
                                           bf_switchd_context_t *self) {
  printf("bf_switchd: processing P4 configuration...\n");


  /* Use new format of json file if possible */
  cJSON *p4_device_arr = cJSON_GetObjectItem(switch_config_obj, "p4_devices");
  if (p4_device_arr != NULL) {
    int j = 0, k = 0, s = 0;
    /* New Format */
    /* Go over all devices */
    for (int i = 0; i < cJSON_GetArraySize(p4_device_arr); ++i) {
      cJSON *p4_device_obj = cJSON_GetArrayItem(p4_device_arr, i);

      const bf_dev_id_t device = get_int(p4_device_obj, "device-id");
      p4_devices_t *p4_device = &(self->p4_devices[device]);
      char *eal_args = p4_device->eal_args;

      p4_device->configured = true;
      memset(eal_args, 0, MAX_EAL_LEN);
      strncpy(eal_args, check_and_get_string(p4_device_obj, "eal-args"),
		MAX_EAL_LEN - 1);

	if (self->asic[0].chip_family == BF_DEV_FAMILY_DPDK) {
		/* debug_cli_enable flag set to false by default */
		p4_device->debug_cli_enable = false;
		if (!strcmp(check_and_get_string(p4_device_obj, "debug-cli"), "enable")) {
			p4_device->debug_cli_enable = true;
		}

		cJSON *mempool_obj_arr =
				cJSON_GetObjectItem(p4_device_obj, "mempools");

		assert(mempool_obj_arr != NULL);
		assert(cJSON_Array == mempool_obj_arr->type);
		p4_device->num_mempool_objs =
				cJSON_GetArraySize(mempool_obj_arr);
		/* Go over all mempool objs */
		for (j = 0; j < cJSON_GetArraySize(mempool_obj_arr); ++j) {
			cJSON *mempool_obj =
				cJSON_GetArrayItem(mempool_obj_arr, j);
			struct mempool_obj_s *mempool =
				&p4_device->mempool_objs[j];

			mempool->name = strdup(get_string(mempool_obj, "name"));
			mempool->buffer_size =
					get_int(mempool_obj, "buffer_size");
			mempool->pool_size = get_int(mempool_obj, "pool_size");
			mempool->cache_size =
					get_int(mempool_obj, "cache_size");
			mempool->numa_node = get_int(mempool_obj, "numa_node");
		}
	}

      cJSON *p4_program_arr = cJSON_GetObjectItem(p4_device_obj, "p4_programs");
      assert(p4_program_arr != NULL);
      assert(cJSON_Array == p4_program_arr->type);

      p4_device->num_p4_programs = cJSON_GetArraySize(p4_program_arr);
      /* Go over all programs */
      for (j = 0; j < cJSON_GetArraySize(p4_program_arr); ++j) {
        cJSON *p4_program_obj = cJSON_GetArrayItem(p4_program_arr, j);
        p4_programs_t *p4_program = &(p4_device->p4_programs[j]);
        p4_program->program_name =
            strdup(get_string(p4_program_obj, "program-name"));
        self->non_default_port_ppgs =
            check_and_get_int(p4_program_obj, "non_default_port_ppgs", 0);
        self->sai_default_init =
            check_and_get_bool(p4_program_obj, "sai_default_init", true);

        to_abs_path(
            p4_program->switchapi, p4_program_obj, "switchapi", install_dir);
        to_abs_path(p4_program->switchsai, p4_program_obj, "sai", install_dir);
        to_abs_path(
            p4_program->switchlink, p4_program_obj, "switchlink", install_dir);
        to_abs_path(p4_program->diag, p4_program_obj, "diag", install_dir);
        to_abs_path(p4_program->accton_diag,
                    p4_program_obj,
                    "accton_diag",
                    install_dir);
        p4_program->add_ports_to_switchapi =
            check_and_get_bool(p4_program_obj, "switchapi_port_add", true);
        // CPU port name (if using ethernet CPU port)
        if (strlen(check_and_get_string(p4_program_obj, "cpu_port"))) {
          p4_program->use_eth_cpu_port = true;
          sprintf(p4_program->eth_cpu_port_name,
                  "%s",
                  check_and_get_string(p4_program_obj, "cpu_port"));
        } else {
          p4_program->use_eth_cpu_port = false;
        }
        // BFRT config
        to_abs_path(p4_program->bfrt_config,
                    p4_program_obj,
                    "bfrt-config",
                    install_dir);

	// Port Config
        to_abs_path(p4_program->port_config,
                    p4_program_obj,
                    "port-config",
                    install_dir);

	// BF-SMI model json
        to_abs_path(p4_program->model_json_path,
                    p4_program_obj,
                    "model_json_path",
                    install_dir);

        to_abs_path(self->board_port_map_conf_file,
                    p4_program_obj,
                    "board-port-map",
                    install_dir);

        cJSON *p4_pipeline_arr =
            cJSON_GetObjectItem(p4_program_obj, "p4_pipelines");
        assert(p4_pipeline_arr != NULL);

        p4_program->num_p4_pipelines = cJSON_GetArraySize(p4_pipeline_arr);
        if (p4_program->num_p4_pipelines > BF_SWITCHD_MAX_P4_PIPELINES) {
          printf("Too many P4 control flows %d ", p4_program->num_p4_pipelines);
          assert(0);
        }
        /* Go over all p4_pipelines */
        for (k = 0; k < cJSON_GetArraySize(p4_pipeline_arr); ++k) {
          cJSON *p4_pipeline_obj = cJSON_GetArrayItem(p4_pipeline_arr, k);
          p4_pipeline_config_t *p4_pipeline = &(p4_program->p4_pipelines[k]);

          p4_pipeline->p4_pipeline_name =
              strdup(get_string(p4_pipeline_obj, "p4_pipeline_name"));
	if (self->asic[0].chip_family == BF_DEV_FAMILY_DPDK) {
		p4_pipeline->core_id = get_int(p4_pipeline_obj, "core_id");
		p4_pipeline->numa_node = get_int(p4_pipeline_obj, "numa_node");
		cJSON *mir_cfg = cJSON_GetObjectItem(p4_pipeline_obj, "mirror_config");
		/* Provide default params for mirror config, if not provided.
		 */
		if (!mir_cfg) {
			p4_pipeline->mir_cfg.n_slots = DEFAULT_MIRROR_N_SLOTS;
			p4_pipeline->mir_cfg.n_sessions = DEFAULT_MIRROR_N_SESSIONS;
			p4_pipeline->mir_cfg.fast_clone = DEFAULT_MIRROR_FAST_CLONE;
		} else {
			p4_pipeline->mir_cfg.n_slots = check_and_get_int(mir_cfg,
									 "n_slots",
									 DEFAULT_MIRROR_N_SLOTS);
			p4_pipeline->mir_cfg.n_sessions = check_and_get_int(mir_cfg,
									    "n_sessions",
									    DEFAULT_MIRROR_N_SESSIONS);
			p4_pipeline->mir_cfg.fast_clone = check_and_get_int(mir_cfg,
									    "fast_clone",
									    DEFAULT_MIRROR_FAST_CLONE);
			assert(check_number_is_power_of_two(p4_pipeline->mir_cfg.n_slots));
			assert(check_number_is_power_of_two(p4_pipeline->mir_cfg.n_sessions));
			if (!(p4_pipeline->mir_cfg.fast_clone == 0 ||
			      p4_pipeline->mir_cfg.fast_clone == 1)) {
				printf("fast clone can be 0 or 1 only, invalid value provided.");
				assert(0);
			}
		}
	}
          to_abs_path(p4_pipeline->table_config,
                      p4_pipeline_obj,
                      "context",
                      install_dir);
          to_abs_path(
              p4_pipeline->cfg_file, p4_pipeline_obj, "config", install_dir);

          /* Get the pipe scope */
          cJSON *pipe_scope_arr =
              cJSON_GetObjectItem(p4_pipeline_obj, "pipe_scope");
          /*
           * pipe_scope_arr == Null is only valid for 1-Program, 1-Pipeline
           * case where we can go and set everything to default. Assert
           * if that's not true
           */
          if (pipe_scope_arr == NULL) {
            assert(cJSON_GetArraySize(p4_program_arr) <= 1);
            assert(cJSON_GetArraySize(p4_pipeline_arr) <= 1);
            /* Set num_pipes to 0 so that it can be populated later
             * with the correct values from the info queried from lld
             */
            p4_pipeline->num_pipes_in_scope = 0;
          } else {
            p4_pipeline->num_pipes_in_scope =
                cJSON_GetArraySize(pipe_scope_arr);
          }
          if (p4_pipeline->num_pipes_in_scope > BF_SWITCHD_MAX_PIPES) {
            p4_pipeline->num_pipes_in_scope = BF_SWITCHD_MAX_PIPES;
          }
          for (s = 0; s < p4_pipeline->num_pipes_in_scope; ++s) {
            cJSON *pipe_scope_obj = cJSON_GetArrayItem(pipe_scope_arr, s);
            p4_pipeline->pipe_scope[s] = pipe_scope_obj->valueint;
          }
        }
      }
      /* Print config */
      printf("P4 profile for dev_id %d\n", device);
      printf("P4 EAL args %s\n", p4_device->eal_args);
	  printf("Debug CLI %s\n",
			(p4_device->debug_cli_enable == true) ? "enable" : "disable");
	printf("num mempool objs %d\n", p4_device->num_mempool_objs);
	for (j = 0; j < p4_device->num_mempool_objs;  ++j) {
		struct mempool_obj_s *mempool = &p4_device->mempool_objs[j];

		printf("  mempool_name: %s\n", mempool->name);
		printf("  buffer_size:  %d\n", mempool->buffer_size);
		printf("  pool_size:    %d\n", mempool->pool_size);
		printf("  cache_size:   %d\n", mempool->cache_size);
		printf("  numa_node:    %d\n", mempool->numa_node);
	}
      printf("num P4 programs %d\n", p4_device->num_p4_programs);
      for (j = 0; j < p4_device->num_p4_programs; ++j) {
        p4_programs_t *p4_program = &(p4_device->p4_programs[j]);
        printf("  p4_name: %s\n", p4_program->program_name);
	printf("  bfrt_config: %s\n", p4_program->bfrt_config);
	printf("  port_config: %s\n", p4_program->port_config);
        for (k = 0; k < p4_program->num_p4_pipelines; k++) {
          p4_pipeline_config_t *p4_pipeline = &(p4_program->p4_pipelines[k]);
	  printf("  p4_pipeline_name: %s\n", p4_pipeline->p4_pipeline_name);
	  printf("  core_id: %d\n", p4_pipeline->core_id);
	  printf("  numa_node: %d\n", p4_pipeline->numa_node);
          printf("    context: %s\n", p4_pipeline->table_config);
          printf("    config: %s\n", p4_pipeline->cfg_file);
          if (p4_pipeline->num_pipes_in_scope > 0) {
            printf("  Pipes in scope [");
            for (s = 0; s < p4_pipeline->num_pipes_in_scope; ++s) {
              printf("%d ", p4_pipeline->pipe_scope[s]);
            }
            printf("]\n");
          }
	  printf("  Mirror Config\n");
	  printf("    n_slots: %u \n", p4_pipeline->mir_cfg.n_slots);
	  printf("    n_sessions: %u \n", p4_pipeline->mir_cfg.n_sessions);
	  printf("    fast_clone: %d \n", p4_pipeline->mir_cfg.fast_clone);
          printf("  diag: %s\n", p4_program->diag);
          printf("  accton diag: %s\n", p4_program->accton_diag);
          if (strlen(self->board_port_map_conf_file)) {
            printf("  board-port-map: %s\n", self->board_port_map_conf_file);
          }
        }
      }
    }
    printf("  non_default_port_ppgs: %d\n", self->non_default_port_ppgs);
    printf("  SAI default initialize: %d \n", self->sai_default_init);
  }
}


static int switch_parse_chip_list(cJSON *root,
                                  bf_switchd_context_t *ctx,
                                  const char *cfg_install_dir) {
  /* Get the chip_list node */
  cJSON *chip_list = cJSON_GetObjectItem(root, "chip_list");
  if (!chip_list || chip_list->type == cJSON_NULL) {
    printf("Cannot find chip_list in conf file\n");
    return -1;
  }

  /* For each chip in the chip_list... */
  for (cJSON *chip = chip_list->child; chip; chip = chip->next) {
    /* Get the asic id. */
    int chip_id = get_int(chip, "instance");

    /* Get the virtual and slave flags. */
    ctx->asic[chip_id].is_virtual = check_and_get_int(chip, "virtual", 0);
    ctx->asic[chip_id].is_virtual_dev_slave =
        check_and_get_int(chip, "virtual_dev_slave", 0);

    /* Get chip family */
    const char *fam = check_and_get_string(chip, "chip_family");
    if (!strlen(fam)) {
      /* The family wasn't specified, default to DPDK. */
      ctx->asic[chip_id].chip_family = BF_DEV_FAMILY_DPDK;
      fam = "dpdk";
    } else if (!strcasecmp(fam, "dpdk")) {
      ctx->asic[chip_id].chip_family = BF_DEV_FAMILY_DPDK;
    }
    else {
      printf("Unexpected chip family \"%s\" for instance %d\n", fam, chip_id);
      return -1;
    }

    ctx->asic[chip_id].configured = true;

    /* Display the parsed data. */
    printf("Configuration for dev_id %d\n", chip_id);
    printf("  Family        : %s\n", fam);
    if (ctx->asic[chip_id].is_virtual)
      printf("  Virtual   : %d\n", ctx->asic[chip_id].is_virtual);
  }
  return 0;
}

/* Count the number of occurences of a "character" in the "string_to_parse" */
static int count_chars(const char *string_to_parse, char character) {
  int count = 0;

  for (int i = 0; string_to_parse[i]; i++) {
    if (string_to_parse[i] == character) count++;
  }

  return count;
}

/* Get the certain "line_no" line in a multiline "string_to_parse" */
static char *get_line(char *string_to_parse, int line_no) {
  char *save, *ret;
  int count = 0;

  do {
    ret = strtok_r(string_to_parse, (const char *)"\n", &save);
    if (ret) {
      count++;
      if (count == line_no) return ret;
      string_to_parse = NULL;
    }
  } while (ret);

  return NULL;
}

int switch_dev_config_init(const char *install_dir,
                           const char *config_filename,
                           bf_switchd_context_t *self) {
  int ret = 0;
  FILE *file;

  if (!config_filename) {
    return -EINVAL;
  }

  file = fopen(config_filename, "r");
  if (file) {
    int fd = fileno(file);
    struct stat stat_b;
    fstat(fd, &stat_b);
    size_t to_alloc = stat_b.st_size + 1;
    char *config_file = malloc(to_alloc);
    if (config_file) {
      config_file[to_alloc - 1] = 0;
      printf("bf_switchd: processing device configuration...\n");
      int i = fread(config_file, stat_b.st_size, 1, file);
      if (i != 1) {
        printf("Error reading conf file %s\n", config_file);
        ret = -EINVAL;
      } else {
        const char *ptr = NULL;

        /* Parse the config file, print wrong line if it fails */
        cJSON *swch = cJSON_ParseWithOpts(config_file, &ptr, true);
        if (swch) {
          switch_parse_chip_list(swch, self, install_dir);
          switch_p4_pipeline_config_init(install_dir, swch, self);
        } else {
          int lines_in_cfg = count_chars(config_file, '\n');
          int lines_after_err = count_chars(ptr, '\n');
          int at_line = lines_in_cfg - lines_after_err + 1;
          char *line = get_line(config_file, at_line);

          printf("Error in config file at line %d:\n[%s]\n",
                 at_line,
                 (line) ? line : "NONE");
          ret = -EINVAL;
        }
        cJSON_Delete(swch);
      }
      free(config_file);
    } else {
      printf("Could not allocate %zd bytes to load conf file\n", to_alloc);
      ret = -ENOMEM;
    }
    fclose(file);
  } else {
    printf("Could not open conf file %s\n", config_filename);
    ret = -errno;
  }

  return ret;
}

int switch_pci_sysfs_str_get(char *name, size_t name_size) {
	snprintf(name, name_size, "/sys/class/bf/bf0/device");
	return 0;
}

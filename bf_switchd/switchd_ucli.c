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
#include <inttypes.h>
#include <stdlib.h>
#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#include <osdep/p4_sde_osdep.h>

#include <dvm/bf_drv_intf.h>
#include <port_mgr/bf_port_if.h>
#include "bf_switchd.h"

static ucli_status_t switchd_ucli_ucli__terminate__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(
      uc, "terminate", 1, "Exit switchd with status: terminate <exit status>");
  int status = atoi(uc->pargs->args[0]);

  for (int i = 0; i < BF_MAX_DEV_COUNT; ++i) bf_device_remove(i);

  exit(status);
  return 0;
}

static ucli_status_t switchd_ucli_ucli__warm_init_end__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc,
                    "warm-init-end",
                    1,
                    "End warm init for a device: warm-init-end <device>");
  bf_dev_id_t dev_id = atoi(uc->pargs->args[0]);

  bf_status_t x = bf_switchd_warm_init_end(dev_id);
  aim_printf(
      &uc->pvs, "Warm init end for device %d returned status %d\n", dev_id, x);
  return 0;
}

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */
static ucli_command_handler_f switchd_ucli_ucli_handlers__[] = {
    switchd_ucli_ucli__terminate__,
    switchd_ucli_ucli__warm_init_end__,
    NULL};

static ucli_module_t switchd_ucli_module__ = {
    "switchd_ucli", NULL, switchd_ucli_ucli_handlers__, NULL, NULL,
};

static ucli_node_t *switchd_ucli_node_create(void) {
  ucli_node_t *n;
  ucli_module_init(&switchd_ucli_module__);
  n = ucli_node_create("switchd", NULL, &switchd_ucli_module__);
  ucli_node_subnode_add(n, ucli_module_log_node_create("switchd"));
  return n;
}

void switchd_register_ucli_node() {
  ucli_node_t *ucli_node = switchd_ucli_node_create();
  bf_drv_shell_register_ucli(ucli_node);
}

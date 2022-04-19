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
#ifndef __BF_SWITCHD_H__
#define __BF_SWITCHD_H__
/*-------------------- bf_switchd.h -----------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>
#include <lib/bf_switchd_lib_init.h>

#define BF_SWITCHD_MSIX_NONE 0
#define BF_SWITCHD_MSIX_HOSTIF 1
#define BF_SWITCHD_MSIX_TBUS 2
#define BF_SWITCHD_MSIX_CBUS 3
#define BF_SWITCHD_MSIX_MBUS 4
#define BF_SWITCHD_MSIX_PBUS 5

typedef void *(*agent_init_fn_t)(void *);
typedef void *(*agent_init_done_fn_t)(void *);


void switchd_register_ucli_node(void);

int bf_switchd_lib_init_local(void *ctx);

bool bf_switchd_is_kernel_pkt_proc(bf_dev_id_t dev_id);

void bf_switchd_exit_sighandler(int signum);

int bf_switchd_device_type_update(bf_dev_id_t dev_id, bool is_sw_model);

bf_status_t bf_switchd_warm_init_end(bf_dev_id_t dev_id);

/******************************************************************************
*******************************************************************************
                          BF_SWITCHD OPERATIONAL MODE SETTINGS
*******************************************************************************
******************************************************************************/
/*
 * Defines to be set based on the target device type for EMULATOR,
 * IPOSER_FPGA and SERDES_EVAL. Define only ONE for any of the above
 * three targets!!!
 * Note that for SW_MODEL and ASIC targets, nothing has to be defined and
 * device type will be identified at runtime without the need
 * for compile time flags.
 */
//#define DEVICE_IS_EMULATOR
//#define DEVICE_IS_SERDES_EVAL
//#define DEVICE_IS_IPOSER_FPGA

#if defined(DEVICE_IS_SERDES_EVAL)
// TODO - What needs to be defined here?

#endif

/* Define for exercising FASTRECFG test */
#define FASTRECFG_TEST

/* Define for enabling register access log */
#define REG_ACCESS_LOG
/******************************************************************************
*******************************************************************************
                          BF_SWITCHD OPERATIONAL MODE SETTINGS
*******************************************************************************
******************************************************************************/
#endif /* __BF_SWITCHD_H__ */

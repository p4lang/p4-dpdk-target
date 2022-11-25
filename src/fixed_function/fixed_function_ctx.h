/*
 * Copyright(c) 2022 Intel Corporation.
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

/*!
 * @file fixed_function_ctx.h
 *
 *
 * Header File for Fixed Function Context Parsing
 */

#ifndef __FIXED_FUNCTION_CTX_H__
#define __FIXED_FUNCTION_CTX_H__

#include <bf_types/bf_types.h>
#include <osdep/p4_sde_osdep_utils.h>
#include <osdep/p4_sde_osdep.h>
#include "fixed_function/fixed_function_int.h"
#include "dvm/bf_drv_profile.h"

#define CTX_JSON_FIXED_FUNC_KEY_FIELDS "key_fields"
#define CTX_JSON_FIXED_FUNC_DATA_FIELDS "data_fields"
#define CTX_JSON_FIXED_FUNC_KEY_NAME "name"
#define CTX_JSON_FIXED_FUNC_KEY_START_BIT "start_bit"
#define CTX_JSON_FIXED_FUNC_KEY_BIT_WIDTH "bit_width"
#define CTX_JSON_FIXED_FUNC_DATA_NAME "name"
#define CTX_JSON_FIXED_FUNC_DATA_START_BIT "start_bit"
#define CTX_JSON_FIXED_FUNC_DATA_BIT_WIDTH "bit_width"

/**
 * Import Fixed Function Context File
 * @param dev_id The Device ID
 * @param dev_profile Device Profile
 * @return Status of the API call
 */
bf_status_t fixed_function_ctx_import(int dev_id,
                           struct bf_device_profile *dev_profile);

/**
 * Add Fixed Function Context to Map
 * @param mgr_ctx Fixed Function Manager Context
 * @return Status of the API call
 */
bf_status_t fixed_function_ctx_map_add(struct fixed_function_mgr_ctx *mgr_ctx);

#endif /* __FIXED_FUNCTION_CTX_H__ */

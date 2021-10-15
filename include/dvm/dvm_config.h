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
/**************************************************************************/ /**
  *
  * @file
  * @brief dvm Configuration Header
  *
  * @addtogroup dvm-config
  * @{
  *
  *****************************************************************************/
#ifndef __DVM_CONFIG_H__
#define __DVM_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef DVM_INCLUDE_CUSTOM_CONFIG
#include <dvm_custom_config.h>
#endif

/**
 * DVM_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */

#ifndef DVM_CONFIG_PORTING_STDLIB
#define DVM_CONFIG_PORTING_STDLIB 1
#endif

/**
 * DVM_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */

#ifndef DVM_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define DVM_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS DVM_CONFIG_PORTING_STDLIB
#endif

/**
 * DVM_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */

#ifndef DVM_CONFIG_INCLUDE_UCLI
#define DVM_CONFIG_INCLUDE_UCLI 0
#endif

#endif /* __DVM_CONFIG_H__ */
/* @} */

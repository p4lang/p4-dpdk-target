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
  * @brief lld Configuration Header
  *
  * @addtogroup lld-config
  * @{
  *
  *****************************************************************************/
#ifndef __LLDLIB_CONFIG_H__
#define __LLDLIB_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef LLDLIB_INCLUDE_CUSTOM_CONFIG
#include <lld_custom_config.h>
#endif

/**
 * LLDLIB_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */

#ifndef LLDLIB_CONFIG_PORTING_STDLIB
#define LLDLIB_CONFIG_PORTING_STDLIB 1
#endif

/**
 * LLDLIB_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */

#ifndef LLDLIB_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define LLDLIB_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS \
  LLDLIB_CONFIG_PORTING_STDLIB
#endif

/**
 * LLDLIB_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */

#ifndef LLDLIB_CONFIG_INCLUDE_UCLI
#define LLDLIB_CONFIG_INCLUDE_UCLI 0
#endif

#endif /* __LLDLIB_CONFIG_H__ */
/* @} */

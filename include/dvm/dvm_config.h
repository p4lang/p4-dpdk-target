/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
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

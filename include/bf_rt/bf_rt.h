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
/** @file bf_rt.h
 *
 *  @brief One-stop C header file for applications to include for
 *  using BFRT C-frontend
 */
#ifndef _BF_RT_H
#define _BF_RT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bf_rt/bf_rt_common.h>
#include <bf_rt/bf_rt_info.h>
#include <bf_rt/bf_rt_init.h>
#include <bf_rt/bf_rt_learn.h>
#include <bf_rt/bf_rt_session.h>
#include <bf_rt/bf_rt_table.h>
#include <bf_rt/bf_rt_table_attributes.h>
#include <bf_rt/bf_rt_table_data.h>
#include <bf_rt/bf_rt_table_key.h>
#include <bf_rt/bf_rt_table_operations.h>

#ifdef __cplusplus
}
#endif

#endif  //_BF_RT_H

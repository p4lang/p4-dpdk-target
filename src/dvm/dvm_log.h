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
#ifndef __DVM_LOG_H__
#define __DVM_LOG_H__

#include <osdep/p4_sde_osdep.h>

#define LOG_CRIT(...) bf_sys_log_and_trace(BF_MOD_DVM, BF_LOG_CRIT, __VA_ARGS__)
#define LOG_ERROR(...) bf_sys_log_and_trace(BF_MOD_DVM, BF_LOG_ERR, __VA_ARGS__)
#define LOG_WARN(...) bf_sys_log_and_trace(BF_MOD_DVM, BF_LOG_WARN, __VA_ARGS__)
#define LOG_TRACE(...) \
  bf_sys_log_and_trace(BF_MOD_DVM, BF_LOG_INFO, __VA_ARGS__)
#define LOG_DBG(...) bf_sys_log_and_trace(BF_MOD_DVM, BF_LOG_DBG, __VA_ARGS__)

#endif /* __DVM_LOG_H__ */

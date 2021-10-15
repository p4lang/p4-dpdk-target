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

#ifndef PORT_MGR_LOG_INCLUDED
#define PORT_MGR_LOG_INCLUDED

#include <osdep/p4_sde_osdep.h>

#define port_mgr_log_critical(...) \
bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_CRIT, __VA_ARGS__)

#define port_mgr_log_error(...) \
bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_ERR, __VA_ARGS__)

#define port_mgr_log_warn(...) \
bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_WARN, __VA_ARGS__)

#define port_mgr_log_trace(...) \
bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_INFO, __VA_ARGS__)

#define port_mgr_log_debug(...) \
bf_sys_log_and_trace(BF_MOD_PORT, BF_LOG_DBG, __VA_ARGS__)

#define port_mgr_log port_mgr_log_debug

void port_mgr_log_internal(const char *fmt, ...);

#endif  // PORT_MGR_LOG_INCUDED

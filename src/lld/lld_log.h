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

#ifndef LLD_LOG_INCLUDED
#define LLD_LOG_INCLUDED

#include <osdep/p4_sde_osdep.h>

#define lld_log_critical(...) \
bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_CRIT, __VA_ARGS__)

#define lld_log_error(...) \
bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_ERR, __VA_ARGS__)

#define lld_log_warn(...) \
bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_WARN, __VA_ARGS__)

#define lld_log_trace(...) \
bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_INFO, __VA_ARGS__)

#define lld_log_debug(...) \
bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_DBG, __VA_ARGS__)

#define lld_log lld_log_debug

enum lld_log_type_e {
	LOG_TYP_GLBL = 0,
	LOG_TYP_CHIP,
};

int lld_log_worthy(enum lld_log_type_e typ, int p1, int p2, int p3);
int lld_log_set(enum lld_log_type_e typ, int p1, int p2, int p3);
void lld_log_settings(void);
void lld_log_internal(const char *fmt, ...);

#endif  // LLD_LOG_INCUDED

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
#ifndef __PIPE_MGR_LOG_H__
#define __PIPE_MGR_LOG_H__

#include <osdep/p4_sde_osdep.h>

#define LOG_CRIT(...) \
	P4_SDE_LOG(BF_MOD_PIPE, BF_LOG_CRIT, __VA_ARGS__)
#define LOG_ERROR(...) \
	P4_SDE_LOG(BF_MOD_PIPE, BF_LOG_ERR, __VA_ARGS__)
#define LOG_WARN(...) \
	P4_SDE_LOG(BF_MOD_PIPE, BF_LOG_WARN, __VA_ARGS__)
#define LOG_TRACE(...) \
	P4_SDE_LOG(BF_MOD_PIPE, BF_LOG_INFO, __VA_ARGS__)
#define LOG_DBG(...) P4_SDE_LOG(BF_MOD_PIPE, BF_LOG_DBG, __VA_ARGS__)

#endif /* __PIPE_MGR_LOG_H__ */

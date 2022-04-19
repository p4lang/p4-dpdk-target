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
#include <memory>
// local includes
#include "bf_rt_utils.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <bf_types/bf_types.h>
#include <target-sys/bf_sal/bf_sys_mem.h>

void bf_rt_err_str(bf_status_t sts, const char **err_str) {
  *err_str = bf_err_str(sts);
}

#ifdef __cplusplus
}
#endif

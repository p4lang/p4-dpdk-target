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

#ifndef lld_h_included
#define lld_h_included

#include <bf_types/bf_types.h>

#include <lld/lld_err.h>
#include <lld/bf_dev_if.h>

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/** Structure lld_dev_t:
 *
 */
struct lld_dev_t {
	// cache line 1:
	bf_dev_family_t dev_family;  //  ..
	int assigned;                // 1=in-use, 0=available for assignment
	int ready;  // 1=chip ready to use, 0=lld_chip_add not finished
	bf_dev_pipe_t pipe_log2phy[BF_PIPE_COUNT];
};

/** Structure lld_context_t:
 *
 */
struct lld_context_t {
	struct lld_dev_t asic[BF_MAX_DEV_COUNT];
};

extern struct lld_context_t *lld_ctx;
void lld_init(void);
void lld_debug_init(void);

#ifdef __cplusplus
}
#endif /* C++ */

#endif

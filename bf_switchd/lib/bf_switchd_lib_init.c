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
#include "bf_switchd_lib_init.h"

extern int bf_switchd_lib_init_local(void *ctx);

/** \brief initialize the bf_switchd
 *
 * \param ctx: Per device context
 *
 * \return: BF_SUCCESS (0)
 * \return: -ive integer
 */

int bf_switchd_lib_init(bf_switchd_context_t *ctx)
{
	return bf_switchd_lib_init_local((void *)ctx);
}

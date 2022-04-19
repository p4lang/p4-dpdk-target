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

#ifndef LLD_DEV_H_INCLUDED
#define LLD_DEV_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif
extern bool is_skip_p4;
bf_status_t lld_dev_add(bf_dev_id_t dev_id,
			bf_dev_family_t dev_family,
			bf_device_profile_t *profile,
			bf_dev_init_mode_t warm_init_mode);
bf_status_t lld_dev_remove(bf_dev_id_t dev_id);
bf_status_t lld_reset_core(bf_dev_id_t dev_id);
bool lld_dev_ready(bf_dev_id_t dev_id);
bf_status_t lld_dev_lock(bf_dev_id_t dev_id);
bf_status_t lld_dev_unlock(bf_dev_id_t dev_id);
bool lld_dev_is_locked(bf_dev_id_t dev_id);
bf_status_t lld_warm_init_quick(bf_dev_id_t dev_id);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // LLD_DEV_H_INCLUDED

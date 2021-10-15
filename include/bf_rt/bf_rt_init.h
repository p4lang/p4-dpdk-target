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
#ifndef _BF_RT_INIT_H
#define _BF_RT_INIT_H

#include <bf_rt/bf_rt_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the BfRtInfo object corresponding to the (device_id,
 * program name)
 *
 * @param[in] dev_id Device ID
 * @param[in] prog_name Name of the P4 program
 * @param[out] bfrt_info BfRtInfo Obj associated with the Device
 *    and the program name.
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_info_get(const bf_dev_id_t dev_id,
                           const char *prog_name,
                           const bf_rt_info_hdl **info_hdl_ret);

/**
 * @brief Get size of list of all device IDs currently added
 *
 * @param[out] num Size of list of device IDs
 * IDs that are present.
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_num_device_id_list_get(uint32_t *num);
/**
 * @brief Get a list of all device IDs currently added
 *
 * @param[out] device_id_list Set Contains list of device
 * IDs that are present.
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_device_id_list_get(bf_dev_id_t *device_id_list);

/**
 * @brief Get size of list of loaded p4 program names on a particular device
 *
 * @param[in] dev_id Device ID
 * @param[out] num_names Size of the list
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_num_p4_names_get(const bf_dev_id_t dev_id, int *num_names);

/**
 * @brief Get a list of loaded p4 program names on a particular device
 *
 * @param[in] dev_id Device ID
 * @param[out] p4_names Array of char*. User needs to allocate the array of ptrs
 * to pass as input to this function.
 *
 * @return Status of the API call
 */
bf_status_t bf_rt_p4_names_get(const bf_dev_id_t dev_id, const char **p4_names);

/**
 * @brief Bf Rt Module Init API. This function needs to be called to
 * initialize BF-RT. Some specific managers can be specified to be skipped
 * BFRT initialization. This allows BFRT session layer to not know about these
 * managers.
 * Recommendation is not to skip any unless user knows exactly
 * what they are doing.
 *
 * @param[in] port_mgr_skip Skip Port mgr
 * @return Status of the API call
 */
bf_status_t bf_rt_module_init(bool port_mgr_skip);

bf_status_t bf_rt_enable_pipeline(const bf_dev_id_t dev_id);

#ifdef __cplusplus
}
#endif

#endif  // _BF_RT_INIT_H

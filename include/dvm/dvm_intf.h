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

#ifndef DVM_INTF_H_INCLUDED
#define DVM_INTF_H_INCLUDED

/**
 * @file dvm_intf.h
 *
 * @brief Device manager interface APIs
 *
 */

/**
 * @brief Check whether the device is a virtual device or not.
 *
 * @param[in] dev_id Device identifier.
 *
 * @return true - if the device is a virtual device, false -otherwise.
 */
bool bf_drv_is_device_virtual(bf_dev_id_t dev_id);

#endif  // DVM_INTF_H

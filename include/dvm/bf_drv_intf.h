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

#ifndef BF_DRV_INTF_H_INCLUDED
#define BF_DRV_INTF_H_INCLUDED

/**
 * @file bf_drv_intf.h
 *
 * @brief BF drivers interface APIs
 *
 */

#ifndef PORT_MGR_HA_UNIT_TESTING
#define PORT_MGR_HA_UNIT_TESTING
#endif

#include <bf_types/bf_types.h>
#include <dvm/bf_drv_profile.h>
#include <port_mgr/bf_port_if.h>
#include <osdep/p4_sde_osdep.h>
#include <cjson/cJSON.h>
#include <target-utils/uCli/ucli.h>

/**
 * @addtogroup dvm-device-mgmt
 * @{
 */

typedef enum bf_client_prio_e {
  BF_CLIENT_PRIO_0 = 0, /* Lowest priority */
  BF_CLIENT_PRIO_1,
  BF_CLIENT_PRIO_2,
  BF_CLIENT_PRIO_3,
  BF_CLIENT_PRIO_4,
  BF_CLIENT_PRIO_5, /* Highest priority */
} bf_drv_client_prio_t;

typedef enum bf_dev_init_mode_s {
  BF_DEV_INIT_COLD,
  BF_DEV_WARM_INIT_FAST_RECFG,
  BF_DEV_WARM_INIT_HITLESS,
  BF_DEV_WARM_INIT_FAST_RECFG_QUICK
} bf_dev_init_mode_t;

/**
 * @brief Initialize device mgr
 *
 * @return Status of the API call.
 */
bf_status_t bf_drv_init(void);

/**
 * @brief Add a new device.
 *
 * @param[in] dev_family The type of device, e.g. BF_DEV_FAMILY_DPDK
 * @param[in] dev_id.
 * @param[in] profile Profile-details, program name, cfg file path.
 * @param[in] flags Device related flags passed by application to drivers
 *
 * @return Status of the API call.
 */
bf_status_t bf_device_add(bf_dev_family_t dev_family,
                          bf_dev_id_t dev_id,
                          bf_device_profile_t *profile,
                          bf_dev_flags_t flags);

/**
 * @brief Deletes an existing device.
 *
 * @param[in] dev_id.
 *
 * @return Status of the API call.
 */
bf_status_t bf_device_remove(bf_dev_id_t dev_id);

/**
 * @brief Logs an existing device.
 *
 * @param[in] dev_id
 * @param[in] filepath The logfile path/name.
 *
 * @return Status of the API call.
 */
bf_status_t bf_device_log(bf_dev_id_t dev_id, const char *filepath);

/**
 * @brief Adds a port to a device.
 *
 * @param[in] dev_id.
 * @param[in] dev_port.
 * @return Status of the API call.
 */
bf_status_t bf_port_add(bf_dev_id_t dev_id,
                        bf_dev_port_t port);

/**
 * @brief Remove a port from a device.
 *
 * @param[in] dev_id.
 * @param[in] dev_port.
 *
 * @return Status of the API call.
 */
bf_status_t bf_port_remove(bf_dev_id_t dev_id, bf_dev_port_t dev_port);

/* @} */

/* Non-doxy */
typedef enum bf_port_cb_direction_ {
  BF_PORT_CB_DIRECTION_INGRESS = 0,
  BF_PORT_CB_DIRECTION_EGRESS
} bf_port_cb_direction_t;

typedef bf_status_t (*bf_drv_device_add_cb)(bf_dev_id_t dev_id,
                                            bf_dev_family_t dev_family,
                                            bf_device_profile_t *profile,
                                            bf_dev_init_mode_t warm_init_mode);

typedef bf_status_t (*bf_drv_device_del_cb)(bf_dev_id_t dev_id);
typedef bf_status_t (*bf_drv_device_log_cb)(bf_dev_id_t dev_id, cJSON *node);
typedef bf_status_t (*bf_drv_port_add_cb)(bf_dev_id_t dev_id,
                                          bf_dev_port_t port,
                                          struct port_attributes_t *port_attrib);
typedef bf_status_t (*bf_drv_port_del_cb)(bf_dev_id_t dev_id,
                                          bf_dev_port_t port,
                                          bf_port_cb_direction_t direction);
typedef struct bf_drv_client_callbacks_s {
  bf_drv_device_add_cb device_add;
  bf_drv_device_del_cb device_del;
  bf_drv_device_log_cb device_log;
  bf_drv_port_add_cb port_add;
  bf_drv_port_del_cb port_del;
} bf_drv_client_callbacks_t;

typedef int bf_drv_client_handle_t;

/**
 * @brief Register as device-manager-client.
 *
 * @param[in] client_name Client Name.
 * @param[out] client_handle Pointer to return the allocated client handle.
 *
 * @return Status of the API call.
 */
bf_status_t bf_drv_register(const char *client_name,
                            bf_drv_client_handle_t *client_handle);

/**
 * @brief De-Register as device-manager-client.
 *
 * @param[in] client_handle The client handle allocated.
 *
 * @return Status of the API call.
 */
bf_status_t bf_drv_deregister(bf_drv_client_handle_t client_handle);

/**
 * @brief Register all callbacks.
 *
 * @param[in] client_handle The client handle allocated.
 * @param[in] callbacks Pointer to the callback.
 * @param[in] add_priority Priority of the client for add callback
 *
 * @return Status of the API call.
 */
bf_status_t bf_drv_client_register_callbacks(
    bf_drv_client_handle_t client_handle,
    bf_drv_client_callbacks_t *callbacks,
    bf_drv_client_prio_t add_priority);

/**
 * @brief Get the drivers version
 *
 * @return Pointer to return the version string
 */
const char *bf_drv_get_version(void);

/**
 * @brief Get the drivers internal version
 *
 * @return Pointer to return the version string
 */
const char *bf_drv_get_internal_version(void);

/* Driver shell related API */
/**
 * @brief Initialize the debug shell
 *
 * @return Status of the API call
 */
bf_status_t bf_drv_shell_init(void);

/**
 * @brief Start the debug shell
 *
 * @return Status of the API call
 */
bf_status_t bf_drv_shell_start(void);

/**
 * @brief Stop the debug shell
 *
 * @return Status of the API call
 */
bf_status_t bf_drv_shell_stop(void);

/**
 * @brief Register the ucli node with bf-sde shell
 *
 * @param[in] ucli_node UCLI node pointer
 *
 * @return Status of the API call
 */
bf_status_t bf_drv_shell_register_ucli(ucli_node_t *ucli_node);

/**
 * @brief Unregister uthe cli node with bf-sde shell
 *
 * @param[in] ucli_node UCLI node pointer
 *
 * @return Status of the API call
 */
bf_status_t bf_drv_shell_unregister_ucli(ucli_node_t *ucli_node);

/**
 * @brief Start the UCLI
 *
 * @param[in] fin Input stream file pointer
 * @param[in] fout Output stream file pointer
 *
 * @return None
 */
void bf_drv_ucli_run(FILE *fin, FILE *fout);

/**
 * @brief Initiate a warm init process for a device
 *
 * @param[in] dev_id The device id
 * @param[in] warm_init_mode The warm init mode to use for this device
 * @param[in] serdes_upgrade_mode The mode to use for updating SerDes
 *
 * @return Status of the API call.
 */
bf_status_t bf_device_warm_init_begin(
    bf_dev_id_t dev_id,
    bf_dev_init_mode_t warm_init_mode);

/**
 * @brief End the warm init sequence for the device and resume normal operation
 *
 * @param[in] dev_id The device id
 *
 * @return Status of the API call.
 */
bf_status_t bf_device_warm_init_end(bf_dev_id_t dev_id);

/**
 * @brief Get the init mode of a device
 *
 * @param[in] dev_id The device id
 * @param[out] warm_init_mode The warm init mode to use for this device
 *
 * @return Status of the API call
 */
bf_status_t bf_device_init_mode_get(bf_dev_id_t dev_id,
                                    bf_dev_init_mode_t *warm_init_mode);
/**
 * @brief Get the device type (model or asic)
 *
 * @param[in] dev_id The device id
 * @param[out] is_sw_model Pointer to bool flag to return true for model and
 * false for asic devices
 *
 * @return Status of the API call.
 */
bf_status_t bf_drv_device_type_get(bf_dev_id_t dev_id, bool *is_sw_model);

#endif  // BF_DRV_INTF_H_INCLUDED

/*******************************************************************************
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

/** @file tdi_rt_attributes.h
 *
 *  @brief Contains TDI Attributes related public headers for tdi_rt C
 */
#ifndef _TDI_RT_ATTRIBUTES_H
#define _TDI_RT_ATTRIBUTES_H

#include <tdi/common/tdi_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IdleTimeout Callback
 * @param[in] dev_tgt Device target
 * @param[in] key Table Key
 * @param[in] cookie User provided cookie during cb registration
 */
typedef void (*tdi_idle_tmo_expiry_cb)(tdi_target_hdl *dev_tgt,
                                       tdi_table_key_hdl *key,
                                       void *cookie);

#if 0
/**
 * @brief tdi_ipsec_sadb_expire_cb Callback
 * @param[in] dev_tgt Device target
 * @param[in] key ipsec_sadb_expire Table Key hdl
 * @param[in] ipsec_sa_api
 * @param[in] soft_lifetime_expire_ is expire.
 * @param[in] ipsec_sa_protocol
 * @param[in] ipsec_sa_dest_address
 * @param[in] cookie User provided cookie during cb registration
 */
typedef void (*tdi_ipsec_sadb_expire_cb)(tdi_target_hdl *dev_tgt,
                                         tdi_table_key_hdl *key,
                                         uint32_t ipsec_sa_api,
                                         bool soft_lifetime_expire,
                                         uint8_t ipsec_sa_protocol,
                                         char *ipsec_sa_dest_address,
                                         void *cookie);
#endif
// This callback for demo purpose
/**
 * @brief tdi_ipsec_sadb_expire_cb Callback
 * @param[in] dev_tgt Device target
 * @param[in] ipsec_sa_api
 * @param[in] soft_lifetime_expire_ is expire.
 * @param[in] ipsec_sa_protocol
 * @param[in] ipsec_sa_dest_address
 * @param[in] ipv4
 * @param[in] cookie User provided cookie during cb registration
 */
typedef void (*tdi_ipsec_sadb_expire_cb)(uint32_t dev_id,
                                         uint32_t ipsec_sa_api,
                                         bool soft_lifetime_expire,
                                         uint8_t ipsec_sa_protocol,
                                         char *ipsec_sa_dest_address,
                                         bool ipv4,
                                         void *cookie);

/**
 * @brief PortStatusChange Callback
 * @param[in] dev_tgt Device target
 * @param[in] key Port Table Key hdl
 * @param[in] port_up If port is up
 * @param[in] cookie User provided cookie during cb registration
 */
typedef void (*tdi_port_status_chg_cb)(tdi_target_hdl *dev_tgt,
                                       tdi_table_key_hdl *key,
                                       bool port_up,
                                       void *cookie);

#ifdef __cplusplus
}
#endif

#endif  // _TDI_TABLE_ATTRIBUTES_H

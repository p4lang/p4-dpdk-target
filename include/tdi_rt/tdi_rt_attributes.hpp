/*******************************************************************************
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

/** @file tdi_rt_attributes.hpp
 *
 *  @brief Contains TDI Attributes related public headers for rt C++
 */

#ifndef _TDI_RT_ATTRIBUTES_HPP
#define _TDI_RT_ATTRIBUTES_HPP

#include <bitset>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <tdi/common/tdi_defs.h>

#include <tdi/common/tdi_target.hpp>
#include <tdi/common/tdi_table_key.hpp>
#include <tdi/common/tdi_session.hpp>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief IdleTable Mode
 */
enum tdi_rt_attributes_idle_table_mode_e {
  /** Idle poll mode. When set, entry_hit_state on MAT entries can be
     queried to check idle time */
  TDI_RT_ATTRIBUTES_IDLE_TABLE_POLL_MODE = 0,
  /** Idle notify mode. Can be used to set CB for idletimeout on a MAT */
  TDI_RT_ATTRIBUTES_IDLE_TABLE_NOTIFY_MODE = 1,
  TDI_RT_ATTRIBUTES_IDLE_TABLE_INVALID_MODE = 2
};

/**
 * @brief Pipe Entry scope
 */
enum tdi_rt_attributes_entry_scope_e {
  /** Set scope to all pipelines of current profile for this table. Turns
     table to symmetric. Default mode of tables */
  TDI_RT_ATTRIBUTES_ENTRY_SCOPE_ALL_PIPELINES = 0,
  /** Set scope to a single logical pipe in this profile for this table.
      Turns table to assymmetric */
  TDI_RT_ATTRIBUTES_ENTRY_SCOPE_SINGLE_PIPELINE = 1,
  /** Set scope to user defined scope in this profile for this table.
      Turns table to assymmetric but can be used to group some pipes
      together and hence can be used differently from single scope */
  TDI_RT_ATTRIBUTES_ENTRY_SCOPE_USER_DEFINED = 2
};

/**
 * @brief Gress Scope. Similar to pipe scope but for gress
 */
enum tdi_rt_attributes_gress_scope_e {
  /** Both ingress and egress in scope */
  TDI_RT_ATTRIBUTES_GRESS_SCOPE_ALL_GRESS,
  /** Either Ingress or Egress in scope */
  TDI_RT_ATTRIBUTES_GRESS_SCOPE_SINGLE_GRESS
};

/**
 * @brief Parser Scope. Similar to pipe_scope bit for parser
 */
enum tdi_rt_attributes_parser_scope_e {
  /** All parsers in scope*/
  TDI_RT_ATTRIBUTES_PARSER_SCOPE_ALL_PARSERS_IN_PIPE,
  /** Single parser scope*/
  TDI_RT_ATTRIBUTES_PARSER_SCOPE_SINGLE_PARSER
};

/**
 * @brief Gress Target. Similar to Pipe ID but for gress
 */
enum tdi_rt_attributes_gress_target_e {
  /** Ingress */
  TDI_RT_ATTRIBUTES_GRESS_TARGET_INGRESS,
  /** Egress */
  TDI_RT_ATTRIBUTES_GRESS_TARGET_EGRESS,
  /** All gress */
  TDI_RT_ATTRIBUTES_GRESS_TARGET_ALL = 0xFF
};

#ifdef __cplusplus
}
#endif

namespace tdi {
namespace pna {
namespace rt {

/**
 * @brief IdleTimeout Callback
 * @param[in] dev_tgt Device target
 * @param[in] key Table Key
 * @param[in] cookie User provided cookie during cb registration
 */
typedef void (*IdleTmoExpiryCb)(const Target &dev_tgt,
                                const TableKey *key,
                                void *cookie);

/**
 * @brief IpsecSadbExpire Callback
 * @param[in] dev_id Device ID
 * @param[in] key IpsecSadbExpire Table Key
 * @param[in] 
 * @param[in] cookie User provided cookie during cb registration
 */
typedef void (*IpsecSadbExpireNotifCb)(
    const uint32_t dev_id,
    const uint32_t ipsec_sa_api,
    const bool soft_lifetime_expire,
    const uint8_t ipsec_sa_protocol,
    const char &ipsec_sa_dest_address,
    const bool ipv4,
    void *cookie);

/**
 * @brief PortStatusChange Callback
 * @param[in] dev_id Device ID
 * @param[in] key Port Table Key
 * @param[in] port_up If port is up
 * @param[in] cookie User provided cookie during cb registration
 */
typedef std::function<void(
    const tdi_dev_id_t &dev_id,
    const TableKey *key,
    const bool &port_up,
    void *cookie)>
    PortStatusNotifCb;

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  // __TDI_ATTRIBUTES_HPP

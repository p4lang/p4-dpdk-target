/*
 * Copyright(c) 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this software except as stipulated in the License.
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
/** @file tdi_rt_defs.h
 *
 *  @brief Contains Common data types used in TDI
 */
#ifndef _TDI_RT_DEFS_H
#define _TDI_RT_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
   * @brief Table types. Users are discouraged from using this especially when
   * creating table-agnostic generic applications like a CLI or RPC server
   */
  enum tdi_rt_table_type_e {
    /** Match action table*/
    TDI_RT_TABLE_TYPE_MATCH_DIRECT = TDI_TABLE_TYPE_DEVICE,
    /** Match action table with actions of the table implemented using an
    "ActionProfile" */
    TDI_RT_TABLE_TYPE_MATCH_INDIRECT,
    /** Match action table with actions of the table implemented using an
    "ActionSelector"*/
    TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR,
    /** Action Profile table*/
    TDI_RT_TABLE_TYPE_ACTION_PROFILE,
    /** Action Selector table*/
    TDI_RT_TABLE_TYPE_SELECTOR,
    /** Counter table*/
    TDI_RT_TABLE_TYPE_COUNTER,
    /** Meter table*/
    TDI_RT_TABLE_TYPE_METER,
    /** Register table*/
    TDI_RT_TABLE_TYPE_REGISTER,
    /** LPF table*/
    TDI_RT_TABLE_TYPE_LPF,
    /** WRED table*/
    TDI_RT_TABLE_TYPE_WRED,
    /** PVS*/
    TDI_RT_TABLE_TYPE_PVS,
    /** Port Metadata table.*/
    TDI_RT_TABLE_TYPE_PORT_METADATA,
    /** Dynamic Hashing configuration table*/
    TDI_RT_TABLE_TYPE_DYN_HASH_CFG,
    /** SNAPSHOT_CFG*/
    TDI_RT_TABLE_SNAPSHOT_CFG,
    /** Port Configuration */
    TDI_RT_TABLE_TYPE_PORT_CFG,
    /** Port Stats */
    TDI_RT_TABLE_TYPE_PORT_STAT,
    /** Port Hdl to Dev_port Conversion table */
    TDI_RT_TABLE_TYPE_PORT_HDL_INFO,
    /** Front panel Idx to Dev_port Conversion table */
    TDI_RT_TABLE_TYPE_PORT_FRONT_PANEL_IDX_INFO,
    /** Port Str to Dev_port Conversion table */
    TDI_RT_TABLE_TYPE_PORT_STR_INFO,
    /** Mirror configuration table */
    TDI_RT_TABLE_TYPE_MIRROR_CFG,
    /** Dynamic Hashing algorithm table*/
    TDI_RT_TABLE_TYPE_DYN_HASH_ALGO,
    /** Device Config Table */
    TDI_RT_TABLE_TYPE_DEV_CFG,
    /** Debug Counters table */
    TDI_RT_TABLE_TYPE_DBG_CNT,
    /** Logical table debug debug counters table */
    TDI_RT_TABLE_TYPE_LOG_DBG_CNT,
    /** Register param table */
    TDI_RT_TABLE_TYPE_REG_PARAM,
    /** Dynamic hash compute table */
    TDI_RT_TABLE_TYPE_DYN_HASH_COMPUTE,
    /** Action Selector Get Member table */
    TDI_RT_TABLE_TYPE_SELECTOR_GET_MEMBER,
    TDI_RT_TABLE_TYPE_INVALID_TYPE
  };


#ifdef __cplusplus
}
#endif

#endif

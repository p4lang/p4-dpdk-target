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
    /** Snapshot config table. */
    TDI_RT_TABLE_TYPE_SNAPSHOT_CFG,
    /** Snapshot field Liveness */
    TDI_RT_TABLE_TYPE_SNAPSHOT_LIVENESS,
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
    /** Pktgen Port Configuration table */
    TDI_RT_TABLE_TYPE_PKTGEN_PORT_CFG,
    /** Pktgen Application Configuration table */
    TDI_RT_TABLE_TYPE_PKTGEN_APP_CFG,
    /** Pktgen Packet Buffer Configuration table */
    TDI_RT_TABLE_TYPE_PKTGEN_PKT_BUFF_CFG,
    /** Pktgen Port Mask Configuration table */
    TDI_RT_TABLE_TYPE_PKTGEN_PORT_MASK_CFG,
    /** Pktgen Port Down Replay Configuration table*/
    TDI_RT_TABLE_TYPE_PKTGEN_PORT_DOWN_REPLAY_CFG,
    /** PRE MGID table */
    TDI_RT_TABLE_TYPE_PRE_MGID,
    /** PRE Node table */
    TDI_RT_TABLE_TYPE_PRE_NODE,
    /** PRE ECMP table */
    TDI_RT_TABLE_TYPE_PRE_ECMP,
    /** PRE LAG table */
    TDI_RT_TABLE_TYPE_PRE_LAG,
    /** PRE Prune (L2 XID) table */
    TDI_RT_TABLE_TYPE_PRE_PRUNE,
    /** Mirror configuration table */
    TDI_RT_TABLE_TYPE_MIRROR_CFG,
    /** Traffic Mgr PPG Table - retired */
    TDI_RT_TABLE_TYPE_TM_PPG_OBSOLETE,
    /** PRE Port table */
    TDI_RT_TABLE_TYPE_PRE_PORT,
    /** Dynamic Hashing algorithm table*/
    TDI_RT_TABLE_TYPE_DYN_HASH_ALGO,
    /** TM Pool Config Table **/
    TDI_RT_TABLE_TYPE_TM_POOL_CFG,
    /** TM Skid Pool Table **/
    TDI_RT_TABLE_TYPE_TM_POOL_SKID,
    /** Device Config Table */
    TDI_RT_TABLE_TYPE_DEV_CFG,
    /** TM App Pool Table **/
    TDI_RT_TABLE_TYPE_TM_POOL_APP,
    /** TM Egress Queue general configuration table */
    TDI_RT_TABLE_TYPE_TM_QUEUE_CFG,
    /** TM Egress Queue mappings read-only table */
    TDI_RT_TABLE_TYPE_TM_QUEUE_MAP,
    /** TM Egress Queue color limit settings table */
    TDI_RT_TABLE_TYPE_TM_QUEUE_COLOR,
    /** TM Egress Queue buffer and pool settings table */
    TDI_RT_TABLE_TYPE_TM_QUEUE_BUFFER,
    /** TM Port Group general config parameters table */
    TDI_RT_TABLE_TYPE_TM_PORT_GROUP_CFG,
    /** TM Port Group table */
    TDI_RT_TABLE_TYPE_TM_PORT_GROUP,
    /** TM Color table */
    TDI_RT_TABLE_TYPE_TM_POOL_COLOR,
    /** Snapshot PHV table */
    TDI_RT_TABLE_TYPE_SNAPSHOT_PHV,
    /** Snapshot Trigger Table */
    TDI_RT_TABLE_TYPE_SNAPSHOT_TRIG,
    /** Snapshot Data Table */
    TDI_RT_TABLE_TYPE_SNAPSHOT_DATA,
    /** TM Pool App PFC table */
    TDI_RT_TABLE_TYPE_TM_POOL_APP_PFC,
    /** TM Ingress Port Counters table */
    TDI_RT_TABLE_TYPE_TM_COUNTER_IG_PORT,
    /** TM Egress Port Counters table */
    TDI_RT_TABLE_TYPE_TM_COUNTER_EG_PORT,
    /** TM Queue Counters table */
    TDI_RT_TABLE_TYPE_TM_COUNTER_QUEUE,
    /** TM Pool Counters table */
    TDI_RT_TABLE_TYPE_TM_COUNTER_POOL,
    /** TM Port general config parameters table */
    TDI_RT_TABLE_TYPE_TM_PORT_CFG,
    /** TM Port Buffer table */
    TDI_RT_TABLE_TYPE_TM_PORT_BUFFER,
    /** TM Port Flow Control table */
    TDI_RT_TABLE_TYPE_TM_PORT_FLOWCONTROL,
    /** TM Pipe Counters table */
    TDI_RT_TABLE_TYPE_TM_COUNTER_PIPE,
    /** Debug Counters table */
    TDI_RT_TABLE_TYPE_DBG_CNT,
    /** Logical table debug debug counters table */
    TDI_RT_TABLE_TYPE_LOG_DBG_CNT,
    /** TM Cfg table */
    TDI_RT_TABLE_TYPE_TM_CFG,
    /** TM Pipe Multicast fifo table */
    TDI_RT_TABLE_TYPE_TM_PIPE_MULTICAST_FIFO,
    /** TM Mirror port DPG table */
    TDI_RT_TABLE_TYPE_TM_MIRROR_DPG,
    /** TM Port DPG table */
    TDI_RT_TABLE_TYPE_TM_PORT_DPG,
    /** TM PPG configuration table */
    TDI_RT_TABLE_TYPE_TM_PPG_CFG,
    /** Register param table */
    TDI_RT_TABLE_TYPE_REG_PARAM,
    /** TM Port DPG Counters table */
    TDI_RT_TABLE_TYPE_TM_COUNTER_PORT_DPG,
    /** TM Mirror Port DPG Counters table */
    TDI_RT_TABLE_TYPE_TM_COUNTER_MIRROR_PORT_DPG,
    /** TM PPG Counters table */
    TDI_RT_TABLE_TYPE_TM_COUNTER_PPG,
    /** Dynamic hash compute table */
    TDI_RT_TABLE_TYPE_DYN_HASH_COMPUTE,
    /** Action Selector Get Member table */
    TDI_RT_TABLE_TYPE_SELECTOR_GET_MEMBER,
    /** TM Egress Port Queue Scheduler parameters table */
    TDI_RT_TABLE_TYPE_TM_QUEUE_SCHED_CFG,
    /** TM Egress Port Queue Scheduler shaping table */
    TDI_RT_TABLE_TYPE_TM_QUEUE_SCHED_SHAPING,
    /** TM Egress Port Scheduler parameters table */
    TDI_RT_TABLE_TYPE_TM_PORT_SCHED_CFG,
    /** TM Egress Port Scheduler shaping table */
    TDI_RT_TABLE_TYPE_TM_PORT_SCHED_SHAPING,
    /** TM Pipe general config parameters table */
    TDI_RT_TABLE_TYPE_TM_PIPE_CFG,
    /** TM Pipe Scheduler parameters table */
    TDI_RT_TABLE_TYPE_TM_PIPE_SCHED_CFG,
    /** TM L1 Node Scheduler parameters table */
    TDI_RT_TABLE_TYPE_TM_L1_NODE_SCHED_CFG,
    /** TM L1 Node Scheduler shaping table */
    TDI_RT_TABLE_TYPE_TM_L1_NODE_SCHED_SHAPING,
    TDI_RT_TABLE_TYPE_INVALID_TYPE
  };
  
enum tdi_rt_flags_e {
  TDI_RT_FLAGS_FROM_HW = TDI_FLAGS_DEVICE,
  TDI_RT_FLAGS_MOD_INC,
};
enum tdi_rt_mgr_type_e {
  TDI_RT_MGR_TYPE_PIPE_MGR = TDI_MGR_TYPE_BEGIN,
};

enum tdi_rt_target_e {
  // Note: currently TDI RT don't have this field type.
  TDI_RT_TARGET_PARSER_ID = TDI_TARGET_DEVICE+1,
};

enum tdi_rt_attributes_type_e {
  /** Pipe scope of all entries. Applicable to all Match Action
       Tables(MAT) */
  ENTRY_SCOPE = TDI_ATTRIBUTES_FIELD_BEGIN,
  /** Dynamic key mask on MATs if applicable*/
  DYNAMIC_KEY_MASK,
  /** Idle table on MATs if applicable*/
  IDLE_TABLE_RUNTIME,
  /** Dynamic hash table attribute to set seed. To be deprecated soon */
  DYNAMIC_HASH_ALG_SEED,
  /** Meter byte count asjust. Applicable to meter
                               tables and MATs if they have direct meters*/
  METER_BYTE_COUNT_ADJ,
  /** Port status change cb set attribute. Applicable to Port table */
  PORT_STATUS_NOTIF,
  /** Port stat poll interval set. Applicable to port stats table*/
  PORT_STAT_POLL_INTVL_MS,
  /** PRE device config. Applicable to PRE tables*/
  PRE_DEVICE_CONFIG,
  /** Selector update CB*/
  SELECTOR_UPDATE_CALLBACK
};

enum class tdi_rt_operations_type_e {
  /** Update sw value of all counters with the value in hw.
     Applicable on Counters or MATs with direct counters */
  COUNTER_SYNC = TDI_OPERATIONS_TYPE_DEVICE,
  /** Update sw value of all registers with the value in hw.
     Applicable on Registers or MATs with direct registers */
  REGISTER_SYNC = 1,
  /** Update sw value of all hit state o entries with the actual
     hw status. Applicable MATs with idletimeout POLL mode*/
  HIT_STATUS_UPDATE = 2,
  INVALID = 3
};

#ifdef __cplusplus
}
#endif

#endif

#
# Copyright(c) 2021 Intel Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
from tdiDefs import *

"""
    These maps are based on the enums defined in tdi_rt_defs.h
    If the enums there are changed, these maps must also be changed.
"""

# tdi_rt_target_e
class TargetTypeRt(TargetType):
    class TargetEnumRt(Enum):
        TDI_TARGET_PIPE_ID = TargetEnum.TDI_TARGET_ARCH.value
        TDI_TARGET_DIRECTION = auto()

    target_type_dict = {
        TargetEnumRt.TDI_TARGET_PIPE_ID.value   : "pipe_id",
        TargetEnumRt.TDI_TARGET_DIRECTION.value : "direction"
    }
    target_type_rev_dict = {
        "pipe_id"   :   TargetEnumRt.TDI_TARGET_PIPE_ID.value,
        "direction" :   TargetEnumRt.TDI_TARGET_DIRECTION.value
    }

    # merge with core map
    target_type_rev_dict = {**TargetType.target_type_rev_dict, **target_type_rev_dict}

class FlagsTypeRt(FlagsType):
    class FlagsEnumRt(Enum):
        TDI_RT_FLAGS_FROM_HW = FlagsEnum.TDI_FLAGS_DEVICE.value
        TDI_RT_FLAGS_MOD_INC = auto()
        TDI_RT_FLAGS_SKIP_TTL_RESET = auto()

    flags_dict = {
        FlagsEnumRt.TDI_RT_FLAGS_FROM_HW.value : "from_hw"
        # TODO: strings for other enums
    }
    flags_rev_dict = {
        "from_hw" : FlagsEnumRt.TDI_RT_FLAGS_FROM_HW.value
    }

    flags_dict = {**FlagsType.flags_dict, **flags_dict}
    flags_rev_dict = {**FlagsType.flags_rev_dict, **flags_rev_dict}

# tdi_rt_attributes_type_e in tdi_rt_defs.h and tdi_rt_info.hpp
class AttributesTypeRt(AttributesType):
    class AttributesEnumRt(Enum):
        # temperate to keep as starting from CORE now. Need tdi parser to change to populate 
        # starting with AttributesEnum.TDI_ATTRIBUTES_TYPE_DEVICE.value
        #TDI_RT_ATTRIBUTES_TYPE_ENTRY_SCOPE = AttributesEnum.TDI_ATTRIBUTES_TYPE_DEVICE.value
        # Notes: need to finalize support of attributes type and update both here and
        # include/tdi_rt/tdi_rt_defs.h 
        TDI_RT_ATTRIBUTES_TYPE_ENTRY_SCOPE = AttributesEnum.TDI_ATTRIBUTES_TYPE_CORE.value
        TDI_RT_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK = auto()
        TDI_RT_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME = auto()
        TDI_RT_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ = auto()
        TDI_RT_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF = auto()
        TDI_RT_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS = auto()
        TDI_RT_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG = auto()
        TDI_RT_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK = auto()

    attributes_dict = {
        AttributesEnumRt.TDI_RT_ATTRIBUTES_TYPE_ENTRY_SCOPE.value: ["symmetric_mode_set", "symmetric_mode_get"],
        AttributesEnumRt.TDI_RT_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK.value: ["dyn_key_mask_get", "dyn_key_mask_set"],
        AttributesEnumRt.TDI_RT_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME.value: ["idle_table_set_poll", "idle_table_set_notify", "idle_table_get"],
        AttributesEnumRt.TDI_RT_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ.value: ["meter_byte_count_adjust_set", "meter_byte_count_adjust_get"],
        AttributesEnumRt.TDI_RT_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF.value: ["port_status_notif_cb_set"],
        AttributesEnumRt.TDI_RT_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS.value: ["port_stats_poll_intv_set", "port_stats_poll_intv_get"],
        AttributesEnumRt.TDI_RT_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG.value: ["pre_device_config_set", "pre_device_config_get"],
        AttributesEnumRt.TDI_RT_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK.value: ["selector_table_update_cb_set"]}

# tdi_rt_operations_type_e
class OperationsTypeRt(OperationsType):
    class OperationsEnumRt(Enum):
        TDI_RT_OPERATIONS_TYPE_COUNTER_SYNC = OperationsEnum.TDI_OPERATIONS_TYPE_DEVICE.value
        TDI_RT_OPERATIONS_TYPE_REGISTER_SYNC = auto()
        TDI_RT_OPERATIONS_TYPE_HIT_STATUS_UPDATE = auto()
        TDI_RT_OPERATIONS_TYPE_SYNC = auto()

    operations_dict = {
        OperationsEnumRt.TDI_RT_OPERATIONS_TYPE_COUNTER_SYNC.value        :   "operation_counter_sync",
        OperationsEnumRt.TDI_RT_OPERATIONS_TYPE_REGISTER_SYNC.value       :   "operation_register_sync",
        OperationsEnumRt.TDI_RT_OPERATIONS_TYPE_HIT_STATUS_UPDATE.value   :   "operation_hit_state_update"}

# tdi_rt_match_type_e -- currently dpdk/mev don't have target specific match type.
class KeyMatchTypeRt(KeyMatchType):
    '''
    class KeyMatchEnumRt(Enum):
        TDI_MATCH_TYPE_ATCAM = KeyMatchEnum.TDI_MATCH_TYPE_DEVICE.value

    key_match_type_dict = {
        KeyMatchEnumRt.TDI_MATCH_TYPE_ATCAM.value: "ATCAM"}
    # merge with core map
    key_match_type_dict = {**KeyMatchType.key_match_type_dict, **key_match_type_dict}
    '''
    key_match_type_dict = {**KeyMatchType.key_match_type_dict}

# tdi_rt_table_type_e
# NOTES: need to update both here and include/tdi_rt/tdi_rt_defs.h, only we have finalized the
# table type supported on DPDK/MEV.
class TableTypeRt(TableType):
    class TableTypeEnumRt(Enum):
        TDI_RT_TABLE_TYPE_MATCH_DIRECT = TableTypeEnum.TDI_TABLE_TYPE_DEVICE.value
        TDI_RT_TABLE_TYPE_MATCH_INDIRECT = auto()
        TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR = auto()
        TDI_RT_TABLE_TYPE_ACTION_PROFILE = auto()
        TDI_RT_TABLE_TYPE_SELECTOR = auto()
        TDI_RT_TABLE_TYPE_COUNTER = auto()
        TDI_RT_TABLE_TYPE_METER = auto()
        TDI_RT_TABLE_TYPE_REGISTER = auto()
        TDI_RT_TABLE_TYPE_LPF = auto()
        TDI_RT_TABLE_TYPE_WRED = auto()
        TDI_RT_TABLE_TYPE_PVS = auto()
        TDI_RT_TABLE_TYPE_PORT_METADATA = auto()
        TDI_RT_TABLE_TYPE_DYN_HASH_CFG = auto()
        TDI_RT_TABLE_TYPE_SNAPSHOT_CFG = auto()
        TDI_RT_TABLE_TYPE_SNAPSHOT_LIVENESS = auto()
        TDI_RT_TABLE_TYPE_PORT_CFG = auto()
        TDI_RT_TABLE_TYPE_PORT_STAT = auto()
        TDI_RT_TABLE_TYPE_PORT_HDL_INFO = auto()
        TDI_RT_TABLE_TYPE_PORT_FRONT_PANEL_IDX_INFO = auto()
        TDI_RT_TABLE_TYPE_PORT_STR_INFO = auto()
        TDI_RT_TABLE_TYPE_PKTGEN_PORT_CFG = auto()
        TDI_RT_TABLE_TYPE_PKTGEN_APP_CFG = auto()
        TDI_RT_TABLE_TYPE_PKTGEN_PKT_BUFF_CFG = auto()
        TDI_RT_TABLE_TYPE_PKTGEN_PORT_MASK_CFG = auto()
        TDI_RT_TABLE_TYPE_PKTGEN_PORT_DOWN_REPLAY_CFG = auto()
        TDI_RT_TABLE_TYPE_PRE_MGID = auto()
        TDI_RT_TABLE_TYPE_PRE_NODE = auto()
        TDI_RT_TABLE_TYPE_PRE_ECMP = auto()
        TDI_RT_TABLE_TYPE_PRE_LAG = auto()
        TDI_RT_TABLE_TYPE_PRE_PRUNE = auto()
        TDI_RT_TABLE_TYPE_MIRROR_CFG = auto()
        TDI_RT_TABLE_TYPE_TM_PPG_OBSOLETE = auto()
        TDI_RT_TABLE_TYPE_PRE_PORT = auto()
        TDI_RT_TABLE_TYPE_DYN_HASH_ALGO = auto()
        TDI_RT_TABLE_TYPE_TM_POOL_CFG = auto()
        TDI_RT_TABLE_TYPE_TM_POOL_SKID = auto()
        TDI_RT_TABLE_TYPE_DEV_CFG = auto()
        TDI_RT_TABLE_TYPE_TM_POOL_APP = auto()
        TDI_RT_TABLE_TYPE_TM_QUEUE_CFG = auto()
        TDI_RT_TABLE_TYPE_TM_QUEUE_MAP = auto()
        TDI_RT_TABLE_TYPE_TM_QUEUE_COLOR = auto()
        TDI_RT_TABLE_TYPE_TM_QUEUE_BUFFER = auto()
        TDI_RT_TABLE_TYPE_TM_PORT_GROUP_CFG = auto()
        TDI_RT_TABLE_TYPE_TM_PORT_GROUP = auto()
        TDI_RT_TABLE_TYPE_TM_POOL_COLOR = auto()
        TDI_RT_TABLE_TYPE_SNAPSHOT_PHV = auto()
        TDI_RT_TABLE_TYPE_SNAPSHOT_TRIG = auto()
        TDI_RT_TABLE_TYPE_SNAPSHOT_DATA = auto()
        TDI_RT_TABLE_TYPE_TM_POOL_APP_PFC = auto()
        TDI_RT_TABLE_TYPE_TM_COUNTER_IG_PORT = auto()
        TDI_RT_TABLE_TYPE_TM_COUNTER_EG_PORT = auto()
        TDI_RT_TABLE_TYPE_TM_COUNTER_QUEUE = auto()
        TDI_RT_TABLE_TYPE_TM_COUNTER_POOL = auto()
        TDI_RT_TABLE_TYPE_TM_PORT_CFG = auto()
        TDI_RT_TABLE_TYPE_TM_PORT_BUFFER = auto()
        TDI_RT_TABLE_TYPE_TM_PORT_FLOWCONTROL = auto()
        TDI_RT_TABLE_TYPE_TM_COUNTER_PIPE = auto()
        TDI_RT_TABLE_TYPE_DBG_CNT = auto()
        TDI_RT_TABLE_TYPE_LOG_DBG_CNT = auto()
        TDI_RT_TABLE_TYPE_TM_CFG = auto()
        TDI_RT_TABLE_TYPE_TM_PIPE_MULTICAST_FIFO = auto()
        TDI_RT_TABLE_TYPE_TM_MIRROR_DPG = auto()
        TDI_RT_TABLE_TYPE_TM_PORT_DPG = auto()
        TDI_RT_TABLE_TYPE_TM_PPG_CFG = auto()
        TDI_RT_TABLE_TYPE_REG_PARAM = auto()
        TDI_RT_TABLE_TYPE_TM_COUNTER_PORT_DPG = auto()
        TDI_RT_TABLE_TYPE_TM_COUNTER_MIRROR_PORT_DPG = auto()
        TDI_RT_TABLE_TYPE_TM_COUNTER_PPG = auto()
        TDI_RT_TABLE_TYPE_DYN_HASH_COMPUTE = auto()
        TDI_RT_TABLE_TYPE_SELECTOR_GET_MEMBER = auto()
        TDI_RT_TABLE_TYPE_TM_QUEUE_SCHED_CFG = auto()
        TDI_RT_TABLE_TYPE_TM_QUEUE_SCHED_SHAPING = auto()
        TDI_RT_TABLE_TYPE_TM_PORT_SCHED_CFG = auto()
        TDI_RT_TABLE_TYPE_TM_PORT_SCHED_SHAPING = auto()
        TDI_RT_TABLE_TYPE_TM_PIPE_CFG = auto()
        TDI_RT_TABLE_TYPE_TM_PIPE_SCHED_CFG = auto()
        TDI_RT_TABLE_TYPE_VALUE_LOOKUP = auto()
        TDI_RT_TABLE_TYPE_INVALID_TYPE = auto()

    table_type_dict = {
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_MATCH_DIRECT.value				:  "MATCH_DIRECT",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_MATCH_INDIRECT.value				:  "MATCH_INDIRECT",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_MATCH_INDIRECT_SELECTOR.value		:  "MATCH_INDIRECT_SELECTOR",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_ACTION_PROFILE.value				:  "ACTION_PROFILE",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_SELECTOR.value					:  "SELECTOR",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_COUNTER.value						:  "COUNTER",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_METER.value						:  "METER",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_REGISTER.value					:  "REGISTER",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_LPF.value							:  "LPF",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_WRED.value						:  "WRED",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PVS.value							:  "PVS",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PORT_METADATA.value				:  "PORT_METADATA",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_DYN_HASH_CFG.value				:  "DYN_HASH_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_SNAPSHOT_CFG.value				:  "SNAPSHOT_CFG",          # /** Snapshot. */
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_SNAPSHOT_LIVENESS.value			:  "SNAPSHOT_LIVENESS",          # /** Snapshot. */
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PORT_CFG.value					:  "PORT_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PORT_STAT.value					:  "PORT_STAT",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PORT_HDL_INFO.value				:  "PORT_HDL_INFO",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PORT_FRONT_PANEL_IDX_INFO.value	:  "PORT_FRONT_PANEL_IDX_INFO",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PORT_STR_INFO.value				:  "PORT_STR_INFO",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PKTGEN_PORT_CFG.value				:  "PKTGEN_PORT_CFG",     # /** Pktgen Port Configuration table */
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PKTGEN_APP_CFG.value				:  "PKTGEN_APP_CFG",      # /** Pktgen Application Configuration table */
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PKTGEN_PKT_BUFF_CFG.value			:  "PKTGEN_PKT_BUFF_CFG", # /** Pktgen Packet Buffer Configuration table */
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PKTGEN_PORT_MASK_CFG.value		:  "PKTGEN_PORT_MASK_CFG", # /** Pktgen Port Mask Configuration table */
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PKTGEN_PORT_DOWN_REPLAY_CFG.value	:  "PKTGEN_PORT_DOWN_REPLAY_CFG", # /** Pktgen Port Down Replay Configuration table*/
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PRE_MGID.value					:  "PRE_MGID",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PRE_NODE.value					:  "PRE_NODE",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PRE_ECMP.value					:  "PRE_ECMP",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PRE_LAG.value						:  "PRE_LAG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PRE_PRUNE.value					:  "PRE_PRUNE",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_MIRROR_CFG.value					:  "MIRROR_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PPG_OBSOLETE.value				:  "TM_PPG_OBSOLETE", # retired
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_PRE_PORT.value					:  "PRE_PORT",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_DYN_HASH_ALGO.value				:  "DYN_HASH_ALGO",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_POOL_CFG.value					:  "TM_POOL_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_POOL_SKID.value				:  "TM_POOL_SKID",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_DEV_CFG.value						:  "DEV_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_POOL_APP.value					:  "TM_POOL_APP",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_QUEUE_CFG.value				:  "TM_QUEUE_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_QUEUE_MAP.value				:  "TM_QUEUE_MAP",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_QUEUE_COLOR.value				:  "TM_QUEUE_COLOR",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_QUEUE_BUFFER.value				:  "TM_QUEUE_BUFFER",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PORT_GROUP_CFG.value			:  "TM_PORT_GROUP_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PORT_GROUP.value				:  "TM_PORT_GROUP",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_POOL_COLOR.value				:  "TM_POOL_COLOR",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_SNAPSHOT_PHV.value				:  "SNAPSHOT_PHV",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_SNAPSHOT_TRIG.value				:  "SNAPSHOT_TRIG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_SNAPSHOT_DATA.value				:  "SNAPSHOT_DATA",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_POOL_APP_PFC.value				:  "TM_POOL_APP_PFC",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_COUNTER_IG_PORT.value			:  "TM_COUNTER_IG_PORT",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_COUNTER_EG_PORT.value			:  "TM_COUNTER_EG_PORT",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_COUNTER_QUEUE.value			:  "TM_COUNTER_QUEUE",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_COUNTER_POOL.value				:  "TM_COUNTER_POOL",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PORT_CFG.value					:  "TM_PORT_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PORT_BUFFER.value				:  "TM_PORT_BUFFER",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PORT_FLOWCONTROL.value			:  "TM_PORT_FLOWCONTROL",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_COUNTER_PIPE.value				:  "TM_COUNTER_PIPE",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_DBG_CNT.value						:  "DBG_CNT",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_LOG_DBG_CNT.value					:  "LOG_DBG_CNT",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_CFG.value						:  "TM_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PIPE_MULTICAST_FIFO.value		:  "TM_PIPE_MULTICAST_FIFO",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_MIRROR_DPG.value				:  "TM_MIRROR_DPG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PORT_DPG.value					:  "TM_PORT_DPG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PPG_CFG.value					:  "TM_PPG_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_REG_PARAM.value					:  "REG_PARAM",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_COUNTER_PORT_DPG.value			:  "TM_COUNTER_PORT_DPG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_COUNTER_MIRROR_PORT_DPG.value	:  "TM_COUNTER_MIRROR_PORT_DPG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_COUNTER_PPG.value				:  "TM_COUNTER_PPG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_DYN_HASH_COMPUTE.value			:  "DYN_HASH_COMPUTE",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_SELECTOR_GET_MEMBER.value			:  "SELECTOR_GET_MEMBER",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_QUEUE_SCHED_CFG.value			:  "TM_QUEUE_SCHED_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_QUEUE_SCHED_SHAPING.value		:  "TM_QUEUE_SCHED_SHAPING",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PORT_SCHED_CFG.value			:  "TM_PORT_SCHED_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PORT_SCHED_SHAPING.value		:  "TM_PORT_SCHED_SHAPING",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PIPE_CFG.value					:  "TM_PIPE_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_TM_PIPE_SCHED_CFG.value			:  "TM_PIPE_SCHED_CFG",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_VALUE_LOOKUP.value            	:  "VALUE_LOOKUP",
        TableTypeEnumRt.TDI_RT_TABLE_TYPE_INVALID_TYPE.value				:  "INVLD"
    }

    # merge with core map
    table_type_dict = {**TableType.table_type_dict, **table_type_dict}

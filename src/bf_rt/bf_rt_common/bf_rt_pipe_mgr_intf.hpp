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

#ifndef _BF_RT_PIPE_MGR_INTERFACE_HPP
#define _BF_RT_PIPE_MGR_INTERFACE_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_types/bf_types.h>
#include <target-sys/bf_sal/bf_sys_mem.h>

#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#ifdef __cplusplus
}
#endif

#include <map>
#include <iostream>
#include <mutex>
#include <memory>

/* bf_rt_includes */

namespace bfrt {

class IPipeMgrIntf {
 public:
  virtual ~IPipeMgrIntf() = default;
  // Library init API
  virtual pipe_status_t pipeMgrInit(void) = 0;
  virtual void pipeMgrCleanup(void) = 0;

  // Client API
  virtual pipe_status_t pipeMgrClientInit(pipe_sess_hdl_t *sess_hdl) = 0;
  virtual pipe_status_t pipeMgrClientCleanup(pipe_sess_hdl_t def_sess_hdl) = 0;
  virtual pipe_status_t pipeMgrCompleteOperations(pipe_sess_hdl_t shdl) = 0;

  // Transaction related API

  virtual pipe_status_t pipeMgrBeginTxn(pipe_sess_hdl_t shdl,
                                        bool isAtomic) = 0;
  virtual pipe_status_t pipeMgrVerifyTxn(pipe_sess_hdl_t shdl) = 0;
  virtual pipe_status_t pipeMgrAbortTxn(pipe_sess_hdl_t shdl) = 0;
  virtual pipe_status_t pipeMgrCommitTxn(pipe_sess_hdl_t shdl,
                                         bool hwSynchronous) = 0;

  // Batch related API
  virtual pipe_status_t pipeMgrBeginBatch(pipe_sess_hdl_t shdl) = 0;
  virtual pipe_status_t pipeMgrFlushBatch(pipe_sess_hdl_t shdl) = 0;
  virtual pipe_status_t pipeMgrEndBatch(pipe_sess_hdl_t shdl,
                                        bool hwSynchronous) = 0;

  virtual bf_dev_pipe_t pipeGetHdlPipe(pipe_mat_ent_hdl_t entry_hdl) = 0;

  virtual pipe_status_t pipeMgrMatchSpecFree(
      pipe_tbl_match_spec_t *match_spec) = 0;

  // Match action table manipulation APIs
  virtual pipe_status_t pipeMgrGetActionDirectResUsage(
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_act_fn_hdl_t act_fn_hdl,
      bool *has_dir_stats,
      bool *has_dir_meter,
      bool *has_dir_lpf,
      bool *has_dir_wred,
      bool *has_dir_stful) = 0;

  virtual pipe_status_t pipeMgrMatchSpecToEntHdl(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_tbl_match_spec_t *match_spec,
      pipe_mat_ent_hdl_t *mat_ent_hdl) = 0;

  virtual pipe_status_t pipeMgrEntHdlToMatchSpec(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      bf_dev_pipe_t *ent_pipe_id,
      const pipe_tbl_match_spec_t **match_spec) = 0;

  virtual pipe_status_t pipeMgrMatchKeyMaskSpecSet(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_tbl_match_spec_t *match_spec) = 0;

  virtual pipe_status_t pipeMgrMatchKeyMaskSpecReset(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl) = 0;

  virtual pipe_status_t pipeMgrMatchKeyMaskSpecGet(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_tbl_match_spec_t *match_spec) = 0;

  virtual pipe_status_t pipeMgrMatEntAdd(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_tbl_match_spec_t *match_spec,
      pipe_act_fn_hdl_t act_fn_hdl,
      const pipe_action_spec_t *act_data_spec,
      uint32_t ttl, /*< TTL value in msecs, 0 for disable */
      uint32_t pipe_api_flags,
      pipe_mat_ent_hdl_t *ent_hdl_p) = 0;

  virtual pipe_status_t pipeMgrMatDefaultEntrySet(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_act_fn_hdl_t act_fn_hdl,
      const pipe_action_spec_t *act_spec,
      uint32_t pipe_api_flags,
      pipe_mat_ent_hdl_t *ent_hdl_p) = 0;

  virtual pipe_status_t pipeMgrTableGetDefaultEntry(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_action_spec_t *pipe_action_spec,
      pipe_act_fn_hdl_t *act_fn_hdl,
      bool from_hw,
      uint32_t res_get_flags,
      pipe_res_get_data_t *res_data) = 0;

  virtual pipe_status_t pipeMgrMatTblDefaultEntryReset(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrTableGetDefaultEntryHandle(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t *ent_hdl_p) = 0;

  virtual pipe_status_t pipeMgrMatTblClear(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrMatEntDel(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t device_id,
                                         pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                         pipe_mat_ent_hdl_t mat_ent_hdl,
                                         uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrMatEntDelByMatchSpec(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_tbl_match_spec_t *match_spec,
      uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrMatEntSetAction(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t device_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               pipe_mat_ent_hdl_t mat_ent_hdl,
                                               pipe_act_fn_hdl_t act_fn_hdl,
                                               pipe_action_spec_t *act_spec,
                                               uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrMatEntSetActionByMatchSpec(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_tbl_match_spec_t *match_spec,
      pipe_act_fn_hdl_t act_fn_hdl,
      const pipe_action_spec_t *act_spec,
      uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrMatEntSetResource(pipe_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t device_id,
                                                 pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                 pipe_mat_ent_hdl_t mat_ent_hdl,
                                                 pipe_res_spec_t *resources,
                                                 int resource_count,
                                                 uint32_t pipe_api_flags) = 0;
  // API for Action Data Table Manipulation
  virtual pipe_status_t pipeMgrAdtEntAdd(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                         pipe_act_fn_hdl_t act_fn_hdl,
                                         const pipe_action_spec_t *action_spec,
                                         pipe_adt_ent_hdl_t *adt_ent_hdl_p,
                                         uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrAdtEntDel(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                         pipe_adt_ent_hdl_t adt_ent_hdl,
                                         uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrAdtEntSet(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t device_id,
                                         pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                         pipe_adt_ent_hdl_t adt_ent_hdl,
                                         pipe_act_fn_hdl_t act_fn_hdl,
                                         const pipe_action_spec_t *action_spec,
                                         uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrTblIsTern(bf_dev_id_t dev_id,
                                         pipe_tbl_hdl_t tbl_hdl,
                                         bool *is_tern) = 0;

  // API for Selector Table Manipulation
  virtual pipe_status_t pipeMgrSelTblRegisterCb(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_sel_tbl_hdl_t sel_tbl_hdl,
      pipe_mgr_sel_tbl_update_callback cb,
      void *cb_cookie) = 0;

  virtual pipe_status_t pipeMgrSelTblProfileSet(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_sel_tbl_hdl_t sel_tbl_hdl,
      pipe_sel_tbl_profile_t *sel_tbl_profile) = 0;

  virtual pipe_status_t pipeMgrSelGrpAdd(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                         uint32_t max_grp_size,
                                         pipe_sel_grp_hdl_t *sel_grp_hdl_p,
                                         uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpDel(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                         pipe_sel_grp_hdl_t sel_grp_hdl,
                                         uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrAdd(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_sel_grp_hdl_t sel_grp_hdl,
                                            pipe_act_fn_hdl_t act_fn_hdl,
                                            pipe_adt_ent_hdl_t adt_ent_hdl,
                                            uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrDel(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_sel_grp_hdl_t sel_grp_hdl,
                                            pipe_adt_ent_hdl_t adt_ent_hdl,
                                            uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrsSet(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                             pipe_sel_grp_hdl_t sel_grp_hdl,
                                             uint32_t num_mbrs,
                                             pipe_adt_ent_hdl_t *mbrs,
                                             bool *enable,
                                             uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrsGet(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                             pipe_sel_grp_hdl_t sel_grp_hdl,
                                             uint32_t mbrs_size,
                                             pipe_adt_ent_hdl_t *mbrs,
                                             bool *enable,
                                             uint32_t *mbrs_populated) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrDisable(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                pipe_sel_grp_hdl_t sel_grp_hdl,
                                                pipe_adt_ent_hdl_t adt_ent_hdl,
                                                uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrEnable(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t device_id,
                                               pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                               pipe_sel_grp_hdl_t sel_grp_hdl,
                                               pipe_adt_ent_hdl_t adt_ent_hdl,
                                               uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrStateGet(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_sel_tbl_hdl_t sel_tbl_hdl,
      pipe_sel_grp_hdl_t sel_grp_hdl,
      pipe_adt_ent_hdl_t adt_ent_hdl,
      enum pipe_mgr_grp_mbr_state_e *mbr_state_p) = 0;

  virtual pipe_status_t pipeMgrSelFallbackMbrSet(pipe_sess_hdl_t sess_hdl,
                                                 dev_target_t dev_tgt,
                                                 pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                 pipe_adt_ent_hdl_t adt_ent_hdl,
                                                 uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelFallbackMbrReset(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_sel_tbl_hdl_t sel_tbl_hdl,
      uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrGetFromHash(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t dev_id,
      pipe_sel_tbl_hdl_t sel_tbl_hdl,
      pipe_sel_grp_hdl_t grp_hdl,
      uint8_t *hash,
      uint32_t hash_len,
      pipe_adt_ent_hdl_t *adt_ent_hdl_p) = 0;

  // API for Flow Learning notifications
  virtual pipe_status_t pipeMgrLrnDigestNotificationRegister(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
      pipe_flow_lrn_notify_cb callback_fn,
      void *callback_fn_cookie) = 0;

  virtual pipe_status_t pipeMgrLrnDigestNotificationDeregister(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl) = 0;

  virtual pipe_status_t pipeMgrFlowLrnNotifyAck(
      pipe_sess_hdl_t sess_hdl,
      pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
      pipe_flow_lrn_msg_t *pipe_flow_lrn_msg) = 0;

  virtual pipe_status_t pipeMgrFlowLrnTimeoutSet(pipe_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t device_id,
                                                 uint32_t usecs) = 0;

  virtual pipe_status_t pipeMgrFlowLrnTimeoutGet(bf_dev_id_t device_id,
                                                 uint32_t *usecs) = 0;

  virtual pipe_status_t pipeMgrFlowLrnIntrModeSet(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_id_t device_id,
                                                  bool en) = 0;

  virtual pipe_status_t pipeMgrFlowLrnIntrModeGet(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_id_t device_id,
                                                  bool *en) = 0;

  virtual pipe_status_t pipeMgrFlowLrnSetNetworkOrderDigest(
      bf_dev_id_t device_id, bool network_order) = 0;

  // API for Statistics Table Manipulation

  virtual pipe_status_t pipeMgrMatEntDirectStatQuery(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      pipe_stat_data_t *stat_data) = 0;

  virtual pipe_status_t pipeMgrMatEntDirectStatSet(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      pipe_stat_data_t *stat_data) = 0;

  virtual pipe_status_t pipeMgrMatEntDirectStatLoad(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      pipe_stat_data_t *stat_data) = 0;

  virtual pipe_status_t pipeMgrStatEntQuery(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_target,
                                            pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                            pipe_stat_ent_idx_t stat_ent_idx,
                                            pipe_stat_data_t *stat_data) = 0;

  virtual pipe_status_t pipeMgrStatTableReset(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                              pipe_stat_data_t *stat_data) = 0;

  virtual pipe_status_t pipeMgrStatEntSet(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_tgt,
                                          pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                          pipe_stat_ent_idx_t stat_ent_idx,
                                          pipe_stat_data_t *stat_data) = 0;

  virtual pipe_status_t pipeMgrStatEntLoad(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                           pipe_stat_ent_idx_t stat_idx,
                                           pipe_stat_data_t *stat_data) = 0;

  virtual pipe_status_t pipeMgrStatDatabaseSync(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_stat_tbl_hdl_t stat_tbl_hdl,
      pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
      void *cookie) = 0;

  virtual pipe_status_t pipeMgrDirectStatDatabaseSync(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
      void *cookie) = 0;

  virtual pipe_status_t pipeMgrStatEntDatabaseSync(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_stat_tbl_hdl_t stat_tbl_hdl,
      pipe_stat_ent_idx_t stat_ent_idx) = 0;

  virtual pipe_status_t pipeMgrDirectStatEntDatabaseSync(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl) = 0;

  // API for Meter Table Manipulation

  virtual pipe_status_t pipeMgrMeterEntSet(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                           pipe_meter_idx_t meter_idx,
                                           pipe_meter_spec_t *meter_spec,
                                           uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrMeterReset(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_tgt,
                                          pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                          uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrMeterReadEntry(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      pipe_meter_spec_t *meter_spec) = 0;

  virtual pipe_status_t pipeMgrMeterReadEntryIdx(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_meter_tbl_hdl_t meter_tbl_hdl,
      pipe_meter_idx_t index,
      pipe_meter_spec_t *meter_spec) = 0;

  virtual pipe_status_t pipeMgrMeterByteCountSet(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_meter_tbl_hdl_t meter_tbl_hdl,
      int byte_count) = 0;

  virtual pipe_status_t pipeMgrMeterByteCountGet(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_meter_tbl_hdl_t meter_tbl_hdl,
      int *byte_count) = 0;


  virtual pipe_status_t pipeMgrExmEntryActivate(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl) = 0;

  virtual pipe_status_t pipeMgrExmEntryDeactivate(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl) = 0;

  virtual pipe_status_t pipeMgrMatEntSetIdleTtl(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      uint32_t ttl, /*< TTL value in msecs */
      uint32_t pipe_api_flags,
      bool reset) = 0;

  virtual pipe_status_t pipeMgrMatEntResetIdleTtl(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl) = 0;
  // API for Idle-Time Management

  virtual pipe_status_t pipeMgrIdleTmoEnableSet(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                bool enable) = 0;

  virtual pipe_status_t pipeMgrIdleRegisterTmoCb(pipe_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t device_id,
                                                 pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                 pipe_idle_tmo_expiry_cb cb,
                                                 void *client_data) = 0;

  virtual pipe_status_t pipeMgrIdleRegisterTmoCbWithMatchSpecCopy(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_idle_tmo_expiry_cb_with_match_spec_copy cb,
      void *client_data) = 0;

  virtual pipe_status_t pipeMgrIdleTimeGetHitState(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      pipe_idle_time_hit_state_e *idle_time_data) = 0;

  virtual pipe_status_t pipeMgrIdleTimeSetHitState(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      pipe_idle_time_hit_state_e idle_time_data) = 0;

  virtual pipe_status_t pipeMgrIdleTimeUpdateHitState(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_idle_tmo_update_complete_cb callback_fn,
      void *cb_data) = 0;

  virtual pipe_status_t pipeMgrMatEntGetIdleTtl(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                pipe_mat_ent_hdl_t mat_ent_hdl,
                                                uint32_t *ttl) = 0;

  virtual pipe_status_t pipeMgrIdleParamsGet(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_idle_time_params_t *params) = 0;

  virtual pipe_status_t pipeMgrIdleParamsSet(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_idle_time_params_t params) = 0;

  // API for Stateful Memory Table Manipulation
  virtual pipe_status_t pipeStfulEntSet(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_target,
                                        pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                        pipe_stful_mem_idx_t stful_ent_idx,
                                        pipe_stful_mem_spec_t *stful_spec,
                                        uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeStfulDatabaseSync(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_stful_tbl_hdl_t stful_tbl_hdl,
      pipe_stful_tbl_sync_cback_fn cback_fn,
      void *cookie) = 0;

  virtual pipe_status_t pipeStfulDirectDatabaseSync(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_stful_tbl_sync_cback_fn cback_fn,
      void *cookie) = 0;

  virtual pipe_status_t pipeStfulQueryGetSizes(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_stful_tbl_hdl_t stful_tbl_hdl,
      int *num_pipes) = 0;

  virtual pipe_status_t pipeStfulDirectQueryGetSizes(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      int *num_pipes) = 0;

  virtual pipe_status_t pipeStfulEntQuery(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_tgt,
                                          pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                          pipe_stful_mem_idx_t stful_ent_idx,
                                          pipe_stful_mem_query_t *stful_query,
                                          uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeStfulDirectEntQuery(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      pipe_stful_mem_query_t *stful_query,
      uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeStfulTableReset(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_stful_tbl_hdl_t stful_tbl_hdl,
      pipe_stful_mem_spec_t *stful_spec) = 0;

  virtual pipe_status_t pipeStfulTableResetRange(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_stful_tbl_hdl_t stful_tbl_hdl,
      pipe_stful_mem_idx_t stful_ent_idx,
      uint32_t num_indices,
      pipe_stful_mem_spec_t *stful_spec) = 0;

  virtual pipe_status_t pipeStfulParamSet(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_tgt,
                                          pipe_tbl_hdl_t tbl_hdl,
                                          pipe_reg_param_hdl_t rp_hdl,
                                          int64_t value) = 0;

  virtual pipe_status_t pipeStfulParamGet(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_tgt,
                                          pipe_tbl_hdl_t tbl_hdl,
                                          pipe_reg_param_hdl_t rp_hdl,
                                          int64_t *value) = 0;

  virtual pipe_status_t pipeStfulParamReset(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_tbl_hdl_t tbl_hdl,
                                            pipe_reg_param_hdl_t rp_hdl) = 0;

  virtual pipe_status_t pipeStfulParamGetHdl(bf_dev_id_t dev,
                                             const char *name,
                                             pipe_reg_param_hdl_t *rp_hdl) = 0;

  virtual bf_dev_pipe_t devPortToPipeId(uint16_t dev_port_id) = 0;

  virtual pipe_status_t pipeMgrStoreEntries(pipe_sess_hdl_t sess_hdl,
                                            pipe_mat_tbl_hdl_t tbl_hdl,
                                            dev_target_t dev_tgt,
				            bool *store_entries) = 0;

  virtual pipe_status_t pipeMgrGetFirstEntry(
			pipe_sess_hdl_t sess_hdl,
			pipe_mat_tbl_hdl_t tbl_hdl,
			dev_target_t dev_tgt,
			pipe_tbl_match_spec_t *match_spec,
			pipe_action_spec_t *action_spec,
			pipe_act_fn_hdl_t *pipe_act_fn_hdl) = 0;

  virtual pipe_status_t pipeMgrGetNextNByKey(
				pipe_sess_hdl_t sess_hdl,
				pipe_mat_tbl_hdl_t tbl_hdl,
				dev_target_t dev_tgt,
				pipe_tbl_match_spec_t *cur_match_spec,
				int n,
				pipe_tbl_match_spec_t *match_specs,
				pipe_action_spec_t **action_specs,
				pipe_act_fn_hdl_t *pipe_act_fn_hdls,
				uint32_t *num) = 0;

  virtual pipe_status_t pipeMgrGetFirstEntryHandle(pipe_sess_hdl_t sess_hdl,
                                                   pipe_mat_tbl_hdl_t tbl_hdl,
                                                   dev_target_t dev_tgt,
                                                   uint32_t *entry_handle) = 0;

  virtual pipe_status_t pipeMgrGetNextEntryHandles(
      pipe_sess_hdl_t sess_hdl,
      pipe_mat_tbl_hdl_t tbl_hdl,
      dev_target_t dev_tgt,
      pipe_mat_ent_hdl_t entry_handle,
      int n,
      uint32_t *next_entry_handles) = 0;

  virtual pipe_status_t pipeMgrGetFirstGroupMember(
      pipe_sess_hdl_t sess_hdl,
      pipe_tbl_hdl_t tbl_hdl,
      dev_target_t dev_tgt,
      pipe_sel_grp_hdl_t sel_grp_hdl,
      pipe_adt_ent_hdl_t *mbr_hdl) = 0;

  virtual pipe_status_t pipeMgrGetNextGroupMembers(
      pipe_sess_hdl_t sess_hdl,
      pipe_tbl_hdl_t tbl_hdl,
      bf_dev_id_t dev_id,
      pipe_sel_grp_hdl_t sel_grp_hdl,
      pipe_adt_ent_hdl_t mbr_hdl,
      int n,
      pipe_adt_ent_hdl_t *next_mbr_hdls) = 0;

  virtual pipe_status_t pipeMgrGetSelGrpMbrCount(pipe_sess_hdl_t sess_hdl,
                                                 dev_target_t dev_tgt,
                                                 pipe_sel_tbl_hdl_t tbl_hdl,
                                                 pipe_sel_grp_hdl_t sel_grp_hdl,
                                                 uint32_t *count) = 0;

  virtual pipe_status_t pipeMgrGetReservedEntryCount(pipe_sess_hdl_t sess_hdl,
                                                     dev_target_t dev_tgt,
                                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                                     size_t *count) = 0;

  virtual pipe_status_t pipeMgrGetEntryCount(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_mat_tbl_hdl_t tbl_hdl,
                                             bool read_from_hw,
                                             uint32_t *count) = 0;

  virtual pipe_status_t pipeMgrGetEntry(pipe_sess_hdl_t sess_hdl,
                                        pipe_mat_tbl_hdl_t tbl_hdl,
                                        struct bf_dev_target_t dev_tgt,
                                        pipe_mat_ent_hdl_t entry_hdl,
                                        pipe_tbl_match_spec_t *pipe_match_spec,
                                        pipe_action_spec_t *pipe_action_spec,
                                        pipe_act_fn_hdl_t *act_fn_hdl,
                                        bool from_hw,
                                        uint32_t res_get_flags,
                                        pipe_res_get_data_t *res_data) = 0;

  virtual pipe_status_t pipeMgrGetActionDataEntry(
      pipe_sess_hdl_t sess_hdl,
      pipe_adt_tbl_hdl_t tbl_hdl,
      struct bf_dev_target_t dev_tgt,
      pipe_adt_ent_hdl_t entry_hdl,
      pipe_action_data_spec_t *pipe_action_data_spec,
      pipe_act_fn_hdl_t *act_fn_hdl,
      bool from_hw) = 0;

  virtual pipe_status_t pipeMgrTblSetProperty(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t dev_id,
      pipe_mat_tbl_hdl_t tbl_hdl,
      pipe_mgr_tbl_prop_type_t property,
      pipe_mgr_tbl_prop_value_t value,
      pipe_mgr_tbl_prop_args_t args) = 0;

  virtual pipe_status_t pipeMgrTblGetProperty(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t dev_id,
      pipe_mat_tbl_hdl_t tbl_hdl,
      pipe_mgr_tbl_prop_type_t property,
      pipe_mgr_tbl_prop_value_t *value,
      pipe_mgr_tbl_prop_args_t *args) = 0;

  /************ Table Debug Counters APIs **************/
  virtual bf_status_t bfDbgCounterGet(bf_dev_target_t dev_tgt,
                                      char *tbl_name,
                                      bf_tbl_dbg_counter_type_t *type,
                                      uint32_t *value) = 0;
  virtual bf_status_t bfDbgCounterGet(bf_dev_target_t dev_tgt,
                                      uint32_t stage,
                                      uint32_t log_tbl,
                                      bf_tbl_dbg_counter_type_t *type,
                                      uint32_t *value) = 0;

  virtual bf_status_t bfDbgCounterSet(bf_dev_target_t dev_tgt,
                                      char *tbl_name,
                                      bf_tbl_dbg_counter_type_t type) = 0;

  virtual bf_status_t bfDbgCounterSet(bf_dev_target_t dev_tgt,
                                      uint32_t stage,
                                      uint32_t log_tbl,
                                      bf_tbl_dbg_counter_type_t type) = 0;

  virtual bf_status_t bfDbgCounterClear(bf_dev_target_t dev_tgt,
                                        char *tbl_name) = 0;

  virtual bf_status_t bfDbgCounterClear(bf_dev_target_t dev_tgt,
                                        uint32_t stage,
                                        uint32_t log_tbl) = 0;

  virtual bf_status_t bfDbgCounterTableListGet(bf_dev_target_t dev_tgt,
                                               char **tbl_list,
                                               int *num_tbls) = 0;

  /************ Virtual dev CB register functions APIs ************/

  virtual bf_status_t pipeMgrTblHdlPipeMaskGet(bf_dev_id_t dev_id,
                                               const std::string &prog_name,
                                               const std::string &pipeline_name,
                                               uint32_t *pipe_mask) const = 0;

  virtual pipe_status_t pipeMgrGetNumPipelines(bf_dev_id_t dev_id,
                                               uint32_t *num_pipes) const = 0;

  virtual bf_status_t pipeMgrEnablePipeline(bf_dev_id_t dev_id) const = 0;

  /*************** DVM APIs ***************/

 protected:
  static std::unique_ptr<IPipeMgrIntf> instance;
  static std::mutex pipe_mgr_intf_mtx;
};

class PipeMgrIntf : public IPipeMgrIntf {
 public:
  virtual ~PipeMgrIntf() {
    if (instance) {
      instance.release();
    }
  };
  PipeMgrIntf() = default;
  // Library init API
  static IPipeMgrIntf *getInstance() {
    if (instance.get() == nullptr) {
      pipe_mgr_intf_mtx.lock();
      if (instance.get() == nullptr) {
        instance.reset(new PipeMgrIntf());
      }
      pipe_mgr_intf_mtx.unlock();
    }
    return IPipeMgrIntf::instance.get();
  }
  pipe_status_t pipeMgrInit(void) override;
  void pipeMgrCleanup(void);

  // Client API
  pipe_status_t pipeMgrClientInit(pipe_sess_hdl_t *sess_hdl);
  pipe_status_t pipeMgrClientCleanup(pipe_sess_hdl_t def_sess_hdl);
  pipe_status_t pipeMgrCompleteOperations(pipe_sess_hdl_t shdl);

  // Transaction related API

  pipe_status_t pipeMgrBeginTxn(pipe_sess_hdl_t shdl, bool isAtomic);
  pipe_status_t pipeMgrVerifyTxn(pipe_sess_hdl_t shdl);
  pipe_status_t pipeMgrAbortTxn(pipe_sess_hdl_t shdl);
  pipe_status_t pipeMgrCommitTxn(pipe_sess_hdl_t shdl, bool hwSynchronous);

  // Batch related API
  pipe_status_t pipeMgrBeginBatch(pipe_sess_hdl_t shdl);
  pipe_status_t pipeMgrFlushBatch(pipe_sess_hdl_t shdl);
  pipe_status_t pipeMgrEndBatch(pipe_sess_hdl_t shdl, bool hwSynchronous);

  // Match_Spec Related APIs
  pipe_status_t pipeMgrMatchSpecFree(pipe_tbl_match_spec_t *match_spec);

  // Match action table manipulation APIs
  pipe_status_t pipeMgrGetActionDirectResUsage(bf_dev_id_t device_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               pipe_act_fn_hdl_t act_fn_hdl,
                                               bool *has_dir_stats,
                                               bool *has_dir_meter,
                                               bool *has_dir_lpf,
                                               bool *has_dir_wred,
                                               bool *has_dir_stful);

  pipe_status_t pipeMgrMatchSpecToEntHdl(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                         pipe_tbl_match_spec_t *match_spec,
                                         pipe_mat_ent_hdl_t *mat_ent_hdl);

  pipe_status_t pipeMgrEntHdlToMatchSpec(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      bf_dev_pipe_t *ent_pipe_id,
      const pipe_tbl_match_spec_t **match_spec);

  pipe_status_t pipeMgrMatchKeyMaskSpecSet(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_tbl_match_spec_t *match_spec);

  pipe_status_t pipeMgrMatchKeyMaskSpecReset(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t device_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl);

  pipe_status_t pipeMgrMatchKeyMaskSpecGet(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_tbl_match_spec_t *match_spec);

  pipe_status_t pipeMgrMatEntAdd(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_tbl_match_spec_t *match_spec,
      pipe_act_fn_hdl_t act_fn_hdl,
      const pipe_action_spec_t *act_data_spec,
      uint32_t ttl, /*< TTL value in msecs, 0 for disable */
      uint32_t pipe_api_flags,
      pipe_mat_ent_hdl_t *ent_hdl_p);

  pipe_status_t pipeMgrMatDefaultEntrySet(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_tgt,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_act_fn_hdl_t act_fn_hdl,
                                          const pipe_action_spec_t *act_spec,
                                          uint32_t pipe_api_flags,
                                          pipe_mat_ent_hdl_t *ent_hdl_p);

  pipe_status_t pipeMgrTableGetDefaultEntry(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_action_spec_t *pipe_action_spec,
      pipe_act_fn_hdl_t *act_fn_hdl,
      bool from_hw,
      uint32_t res_get_flags,
      pipe_res_get_data_t *res_data);

  pipe_status_t pipeMgrTableGetDefaultEntryHandle(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t *ent_hdl_p);

  pipe_status_t pipeMgrMatTblClear(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                   uint32_t pipe_api_flags);

  pipe_status_t pipeMgrMatEntDel(pipe_sess_hdl_t sess_hdl,
                                 bf_dev_id_t device_id,
                                 pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                 pipe_mat_ent_hdl_t mat_ent_hdl,
                                 uint32_t pipe_api_flags);

  pipe_status_t pipeMgrMatEntDelByMatchSpec(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_tbl_match_spec_t *match_spec,
                                            uint32_t pipe_api_flags);

  pipe_status_t pipeMgrMatTblDefaultEntryReset(pipe_sess_hdl_t sess_hdl,
                                               dev_target_t dev_tgt,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               uint32_t pipe_api_flags);

  pipe_status_t pipeMgrMatEntSetAction(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                       pipe_mat_ent_hdl_t mat_ent_hdl,
                                       pipe_act_fn_hdl_t act_fn_hdl,
                                       pipe_action_spec_t *act_spec,
                                       uint32_t pipe_api_flags);

  pipe_status_t pipeMgrMatEntSetActionByMatchSpec(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_tbl_match_spec_t *match_spec,
      pipe_act_fn_hdl_t act_fn_hdl,
      const pipe_action_spec_t *act_spec,
      uint32_t pipe_api_flags);

  pipe_status_t pipeMgrMatEntSetResource(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t device_id,
                                         pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                         pipe_mat_ent_hdl_t mat_ent_hdl,
                                         pipe_res_spec_t *resources,
                                         int resource_count,
                                         uint32_t pipe_api_flags);
  // API for Action Data Table Manipulation
  pipe_status_t pipeMgrAdtEntAdd(pipe_sess_hdl_t sess_hdl,
                                 dev_target_t dev_tgt,
                                 pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                 pipe_act_fn_hdl_t act_fn_hdl,
                                 const pipe_action_spec_t *action_spec,
                                 pipe_adt_ent_hdl_t *adt_ent_hdl_p,
                                 uint32_t pipe_api_flags);

  pipe_status_t pipeMgrAdtEntDel(pipe_sess_hdl_t sess_hdl,
                                 dev_target_t dev_tgt,
                                 pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                 pipe_adt_ent_hdl_t adt_ent_hdl,
                                 uint32_t pipe_api_flags);

  pipe_status_t pipeMgrAdtEntSet(pipe_sess_hdl_t sess_hdl,
                                 bf_dev_id_t device_id,
                                 pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                 pipe_adt_ent_hdl_t adt_ent_hdl,
                                 pipe_act_fn_hdl_t act_fn_hdl,
                                 const pipe_action_spec_t *action_spec,
                                 uint32_t pipe_api_flags);
  pipe_status_t pipeMgrTblIsTern(bf_dev_id_t dev_id,
                                 pipe_tbl_hdl_t tbl_hdl,
                                 bool *is_tern);

  // API for Selector Table Manipulation
  pipe_status_t pipeMgrSelTblRegisterCb(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                        pipe_mgr_sel_tbl_update_callback cb,
                                        void *cb_cookie);

  pipe_status_t pipeMgrSelTblProfileSet(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_sel_tbl_hdl_t sel_tbl_hdl,
      pipe_sel_tbl_profile_t *sel_tbl_profile);

  pipe_status_t pipeMgrSelGrpAdd(pipe_sess_hdl_t sess_hdl,
                                 dev_target_t dev_tgt,
                                 pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                 uint32_t max_grp_size,
                                 pipe_sel_grp_hdl_t *sel_grp_hdl_p,
                                 uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpDel(pipe_sess_hdl_t sess_hdl,
                                 dev_target_t dev_tgt,
                                 pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                 pipe_sel_grp_hdl_t sel_grp_hdl,
                                 uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpMbrAdd(pipe_sess_hdl_t sess_hdl,
                                    bf_dev_id_t device_id,
                                    pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                    pipe_sel_grp_hdl_t sel_grp_hdl,
                                    pipe_act_fn_hdl_t act_fn_hdl,
                                    pipe_adt_ent_hdl_t adt_ent_hdl,
                                    uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpMbrDel(pipe_sess_hdl_t sess_hdl,
                                    bf_dev_id_t device_id,
                                    pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                    pipe_sel_grp_hdl_t sel_grp_hdl,
                                    pipe_adt_ent_hdl_t adt_ent_hdl,
                                    uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpMbrsSet(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                     pipe_sel_grp_hdl_t sel_grp_hdl,
                                     uint32_t num_mbrs,
                                     pipe_adt_ent_hdl_t *mbrs,
                                     bool *enable,
                                     uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpMbrsGet(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                     pipe_sel_grp_hdl_t sel_grp_hdl,
                                     uint32_t mbrs_size,
                                     pipe_adt_ent_hdl_t *mbrs,
                                     bool *enable,
                                     uint32_t *mbrs_populated);

  pipe_status_t pipeMgrSelGrpMbrDisable(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                        pipe_sel_grp_hdl_t sel_grp_hdl,
                                        pipe_adt_ent_hdl_t adt_ent_hdl,
                                        uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpMbrEnable(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                       pipe_sel_grp_hdl_t sel_grp_hdl,
                                       pipe_adt_ent_hdl_t adt_ent_hdl,
                                       uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpMbrStateGet(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_sel_tbl_hdl_t sel_tbl_hdl,
      pipe_sel_grp_hdl_t sel_grp_hdl,
      pipe_adt_ent_hdl_t adt_ent_hdl,
      enum pipe_mgr_grp_mbr_state_e *mbr_state_p);

  pipe_status_t pipeMgrSelFallbackMbrSet(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                         pipe_adt_ent_hdl_t adt_ent_hdl,
                                         uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelFallbackMbrReset(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                           uint32_t pipe_api_flags);
  pipe_status_t pipeMgrSelGrpMbrGetFromHash(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_sel_grp_hdl_t grp_hdl,
                                            uint8_t *hash,
                                            uint32_t hash_len,
                                            pipe_adt_ent_hdl_t *adt_ent_hdl_p);

  // API for Flow Learning notifications
  pipe_status_t pipeMgrLrnDigestNotificationRegister(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
      pipe_flow_lrn_notify_cb callback_fn,
      void *callback_fn_cookie);

  pipe_status_t pipeMgrLrnDigestNotificationDeregister(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl);

  pipe_status_t pipeMgrFlowLrnNotifyAck(pipe_sess_hdl_t sess_hdl,
                                        pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
                                        pipe_flow_lrn_msg_t *pipe_flow_lrn_msg);

  pipe_status_t pipeMgrFlowLrnTimeoutSet(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t device_id,
                                         uint32_t usecs);
  pipe_status_t pipeMgrFlowLrnTimeoutGet(bf_dev_id_t device_id,
                                         uint32_t *usecs);
  pipe_status_t pipeMgrFlowLrnIntrModeSet(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          bool en);
  pipe_status_t pipeMgrFlowLrnIntrModeGet(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          bool *en);
  pipe_status_t pipeMgrFlowLrnSetNetworkOrderDigest(bf_dev_id_t device_id,
                                                    bool network_order);

  // API for Statistics Table Manipulation

  pipe_status_t pipeMgrMatEntDirectStatQuery(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t device_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mat_ent_hdl_t mat_ent_hdl,
                                             pipe_stat_data_t *stat_data);

  pipe_status_t pipeMgrMatEntDirectStatSet(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mat_ent_hdl_t mat_ent_hdl,
                                           pipe_stat_data_t *stat_data);

  pipe_status_t pipeMgrMatEntDirectStatLoad(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_mat_ent_hdl_t mat_ent_hdl,
                                            pipe_stat_data_t *stat_data);

  pipe_status_t pipeMgrStatEntQuery(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_target,
                                    pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                    pipe_stat_ent_idx_t stat_ent_idx,
                                    pipe_stat_data_t *stat_data);

  pipe_status_t pipeMgrStatTableReset(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                      pipe_stat_data_t *stat_data);

  pipe_status_t pipeMgrStatEntSet(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_tgt,
                                  pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                  pipe_stat_ent_idx_t stat_ent_idx,
                                  pipe_stat_data_t *stat_data);

  pipe_status_t pipeMgrStatEntLoad(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                   pipe_stat_ent_idx_t stat_idx,
                                   pipe_stat_data_t *stat_data);

  pipe_status_t pipeMgrStatDatabaseSync(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_stat_tbl_hdl_t stat_tbl_hdl,
      pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
      void *cookie);

  pipe_status_t pipeMgrDirectStatDatabaseSync(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
      void *cookie);

  pipe_status_t pipeMgrStatEntDatabaseSync(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                           pipe_stat_ent_idx_t stat_ent_idx);

  pipe_status_t pipeMgrDirectStatEntDatabaseSync(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl);

  // API for Meter Table Manipulation

  pipe_status_t pipeMgrMeterEntSet(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                   pipe_meter_idx_t meter_idx,
                                   pipe_meter_spec_t *meter_spec,
                                   uint32_t pipe_api_flags);

  pipe_status_t pipeMgrModelTimeAdvance(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        uint64_t tick_time);

  pipe_status_t pipeMgrMeterReset(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_tgt,
                                  pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                  uint32_t pipe_api_flags);

  pipe_status_t pipeMgrMeterReadEntry(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      pipe_mat_ent_hdl_t mat_ent_hdl,
                                      pipe_meter_spec_t *meter_spec);

  pipe_status_t pipeMgrMeterReadEntryIdx(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                         pipe_meter_idx_t index,
                                         pipe_meter_spec_t *meter_spec);

  pipe_status_t pipeMgrMeterByteCountSet(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                         int byte_count);

  pipe_status_t pipeMgrMeterByteCountGet(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                         int *byte_count);

  pipe_status_t pipeMgrExmEntryActivate(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_mat_ent_hdl_t mat_ent_hdl);

  pipe_status_t pipeMgrExmEntryDeactivate(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl);

  pipe_status_t pipeMgrMatEntSetIdleTtl(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_mat_ent_hdl_t mat_ent_hdl,
                                        uint32_t ttl, /*< TTL value in msecs */
                                        uint32_t pipe_api_flags,
                                        bool reset);

  pipe_status_t pipeMgrMatEntResetIdleTtl(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl);
  // API for Idle-Time Management

  pipe_status_t pipeMgrIdleTmoEnableSet(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        bool enable);

  pipe_status_t pipeMgrIdleRegisterTmoCbWithMatchSpecCopy(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_idle_tmo_expiry_cb_with_match_spec_copy cb,
      void *client_data);

  pipe_status_t pipeMgrIdleRegisterTmoCb(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t device_id,
                                         pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                         pipe_idle_tmo_expiry_cb cb,
                                         void *client_data);

  pipe_status_t pipeMgrIdleTimeGetHitState(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      pipe_idle_time_hit_state_e *idle_time_data);

  pipe_status_t pipeMgrIdleTimeSetHitState(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      pipe_idle_time_hit_state_e idle_time_data);

  pipe_status_t pipeMgrIdleTimeUpdateHitState(
      pipe_sess_hdl_t sess_hdl,
      bf_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_idle_tmo_update_complete_cb callback_fn,
      void *cb_data);

  pipe_status_t pipeMgrMatEntGetIdleTtl(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_mat_ent_hdl_t mat_ent_hdl,
                                        uint32_t *ttl);

  pipe_status_t pipeMgrIdleParamsGet(pipe_sess_hdl_t sess_hdl,
                                     bf_dev_id_t device_id,
                                     pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                     pipe_idle_time_params_t *params);

  pipe_status_t pipeMgrIdleParamsSet(pipe_sess_hdl_t sess_hdl,
                                     bf_dev_id_t device_id,
                                     pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                     pipe_idle_time_params_t params);

  // API for Stateful Memory Table Manipulation
  pipe_status_t pipeStfulEntSet(pipe_sess_hdl_t sess_hdl,
                                dev_target_t dev_target,
                                pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                pipe_stful_mem_idx_t stful_ent_idx,
                                pipe_stful_mem_spec_t *stful_spec,
                                uint32_t pipe_api_flags);

  pipe_status_t pipeStfulDatabaseSync(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                      pipe_stful_tbl_sync_cback_fn cback_fn,
                                      void *cookie);

  pipe_status_t pipeStfulDirectDatabaseSync(
      pipe_sess_hdl_t sess_hdl,
      dev_target_t dev_tgt,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_stful_tbl_sync_cback_fn cback_fn,
      void *cookie);

  pipe_status_t pipeStfulQueryGetSizes(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                       int *num_pipes);

  pipe_status_t pipeStfulDirectQueryGetSizes(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t device_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             int *num_pipes);

  pipe_status_t pipeStfulEntQuery(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_tgt,
                                  pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                  pipe_stful_mem_idx_t stful_ent_idx,
                                  pipe_stful_mem_query_t *stful_query,
                                  uint32_t pipe_api_flags);

  pipe_status_t pipeStfulDirectEntQuery(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_mat_ent_hdl_t mat_ent_hdl,
                                        pipe_stful_mem_query_t *stful_query,
                                        uint32_t pipe_api_flags);

  pipe_status_t pipeStfulTableReset(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_tgt,
                                    pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                    pipe_stful_mem_spec_t *stful_spec);

  pipe_status_t pipeStfulTableResetRange(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                         pipe_stful_mem_idx_t stful_ent_idx,
                                         uint32_t num_indices,
                                         pipe_stful_mem_spec_t *stful_spec);

  pipe_status_t pipeStfulParamSet(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_tgt,
                                  pipe_tbl_hdl_t tbl_hdl,
                                  pipe_reg_param_hdl_t rp_hdl,
                                  int64_t value);

  pipe_status_t pipeStfulParamGet(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_tgt,
                                  pipe_tbl_hdl_t tbl_hdl,
                                  pipe_reg_param_hdl_t rp_hdl,
                                  int64_t *value);

  pipe_status_t pipeStfulParamReset(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_tgt,
                                    pipe_tbl_hdl_t tbl_hdl,
                                    pipe_reg_param_hdl_t rp_hdl);

  pipe_status_t pipeStfulParamGetHdl(bf_dev_id_t dev,
                                     const char *name,
                                     pipe_reg_param_hdl_t *rp_hdl);

  bf_dev_pipe_t devPortToPipeId(uint16_t dev_port_id);


  pipe_status_t pipeMgrStoreEntries(pipe_sess_hdl_t sess_hdl,
                                    pipe_mat_tbl_hdl_t tbl_hdl,
                                    dev_target_t dev_tgt,
				    bool *store_entries);

  pipe_status_t pipeMgrGetFirstEntryHandle(pipe_sess_hdl_t sess_hdl,
                                           pipe_mat_tbl_hdl_t tbl_hdl,
                                           dev_target_t dev_tgt,
                                           uint32_t *entry_handle);

  pipe_status_t pipeMgrGetFirstEntry(pipe_sess_hdl_t sess_hdl,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_tbl_match_spec_t *match_spec,
                                     pipe_action_spec_t *action_spec,
				     pipe_act_fn_hdl_t *pipe_act_fn_hdl);

  pipe_status_t pipeMgrGetNextNByKey(pipe_sess_hdl_t sess_hdl,
				     pipe_mat_tbl_hdl_t tbl_hdl,
				     dev_target_t dev_tgt,
				     pipe_tbl_match_spec_t *cur_match_spec,
				     int n,
				     pipe_tbl_match_spec_t *match_specs,
				     pipe_action_spec_t **action_specs,
				     pipe_act_fn_hdl_t *pipe_act_fn_hdls,
				     uint32_t *num);

  pipe_status_t pipeMgrGetNextEntryHandles(pipe_sess_hdl_t sess_hdl,
                                           pipe_mat_tbl_hdl_t tbl_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_mat_ent_hdl_t entry_handle,
                                           int n,
                                           uint32_t *next_entry_handles);

  pipe_status_t pipeMgrGetFirstGroupMember(pipe_sess_hdl_t sess_hdl,
                                           pipe_tbl_hdl_t tbl_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_sel_grp_hdl_t sel_grp_hdl,
                                           pipe_adt_ent_hdl_t *mbr_hdl);

  pipe_status_t pipeMgrGetNextGroupMembers(pipe_sess_hdl_t sess_hdl,
                                           pipe_tbl_hdl_t tbl_hdl,
                                           bf_dev_id_t dev_id,
                                           pipe_sel_grp_hdl_t sel_grp_hdl,
                                           pipe_adt_ent_hdl_t mbr_hdl,
                                           int n,
                                           pipe_adt_ent_hdl_t *next_mbr_hdls);

  pipe_status_t pipeMgrGetSelGrpMbrCount(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_sel_tbl_hdl_t tbl_hdl,
                                         pipe_sel_grp_hdl_t sel_grp_hdl,
                                         uint32_t *count);

  pipe_status_t pipeMgrGetReservedEntryCount(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_mat_tbl_hdl_t tbl_hdl,
                                             size_t *count);

  pipe_status_t pipeMgrGetEntryCount(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     bool read_from_hw,
                                     uint32_t *count);

  pipe_status_t pipeMgrGetEntry(pipe_sess_hdl_t sess_hdl,
                                pipe_mat_tbl_hdl_t tbl_hdl,
                                struct bf_dev_target_t dev_tgt,
                                pipe_mat_ent_hdl_t entry_hdl,
                                pipe_tbl_match_spec_t *pipe_match_spec,
                                pipe_action_spec_t *pipe_action_spec,
                                pipe_act_fn_hdl_t *act_fn_hdl,
                                bool from_hw,
                                uint32_t res_get_flags,
                                pipe_res_get_data_t *res_data);

  pipe_status_t pipeMgrGetActionDataEntry(
      pipe_sess_hdl_t sess_hdl,
      pipe_adt_tbl_hdl_t tbl_hdl,
      struct bf_dev_target_t dev_tgt,
      pipe_adt_ent_hdl_t entry_hdl,
      pipe_action_data_spec_t *pipe_action_data_spec,
      pipe_act_fn_hdl_t *act_fn_hdl,
      bool from_hw);

  pipe_status_t pipeMgrTblSetProperty(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t dev_id,
                                      pipe_mat_tbl_hdl_t tbl_hdl,
                                      pipe_mgr_tbl_prop_type_t property,
                                      pipe_mgr_tbl_prop_value_t value,
                                      pipe_mgr_tbl_prop_args_t args);

  pipe_status_t pipeMgrTblGetProperty(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t dev_id,
                                      pipe_mat_tbl_hdl_t tbl_hdl,
                                      pipe_mgr_tbl_prop_type_t property,
                                      pipe_mgr_tbl_prop_value_t *value,
                                      pipe_mgr_tbl_prop_args_t *args);

  bf_dev_pipe_t pipeGetHdlPipe(pipe_mat_ent_hdl_t entry_hdl);

  /************ Table Debug Counters APIs **************/
  bf_status_t bfDbgCounterGet(bf_dev_target_t dev_tgt,
                              char *tbl_name,
                              bf_tbl_dbg_counter_type_t *type,
                              uint32_t *value);

  bf_status_t bfDbgCounterGet(bf_dev_target_t dev_tgt,
                              uint32_t stage,
                              uint32_t log_tbl,
                              bf_tbl_dbg_counter_type_t *type,
                              uint32_t *value);

  bf_status_t bfDbgCounterSet(bf_dev_target_t dev_tgt,
                              uint32_t stage,
                              uint32_t log_tbl,
                              bf_tbl_dbg_counter_type_t type);

  bf_status_t bfDbgCounterSet(bf_dev_target_t dev_tgt,
                              char *tbl_name,
                              bf_tbl_dbg_counter_type_t type);

  bf_status_t bfDbgCounterClear(bf_dev_target_t dev_tgt, char *tbl_name);

  bf_status_t bfDbgCounterClear(bf_dev_target_t dev_tgt,
                                uint32_t stage,
                                uint32_t log_tbl);

  bf_status_t bfDbgCounterTableListGet(bf_dev_target_t dev_tgt,
                                       char **tbl_list,
                                       int *num_tbls);

  /************ Hitless HA State restore APIs **************/

  /************ Virtual dev CB register functions APIs ************/
  bf_status_t pipeRegisterMatUpdateCb(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t device_id,
                                      pipe_mat_tbl_hdl_t tbl_hdl,
                                      pipe_mat_update_cb cb,
                                      void *cb_cookie) const;

  bf_status_t pipeMgrTblHdlPipeMaskGet(bf_dev_id_t dev_id,
                                       const std::string &prog_name,
                                       const std::string &pipeline_name,
                                       uint32_t *pipe_mask) const;

  pipe_status_t pipeMgrGetNumPipelines(bf_dev_id_t dev_id,
                                       uint32_t *num_pipes) const override;

  bf_status_t pipeMgrEnablePipeline(bf_dev_id_t dev_id) const;

  /*************** DVM APIs ***************/

 private:
  PipeMgrIntf(const PipeMgrIntf &src) = delete;
  PipeMgrIntf &operator=(const PipeMgrIntf &rhs) = delete;
};

}  // bfrt

#endif  // _BF_RT_PIPE_MGR_INTERFACE_HPP

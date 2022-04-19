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

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#ifdef __cplusplus
}
#endif

/* bf_rt_includes */
#include <bf_rt/bf_rt_info.hpp>
#include "bf_rt_pipe_mgr_intf.hpp"

namespace bfrt {

std::unique_ptr<IPipeMgrIntf> IPipeMgrIntf::instance = nullptr;
std::mutex IPipeMgrIntf::pipe_mgr_intf_mtx;

pipe_status_t PipeMgrIntf::pipeMgrInit() { return pipe_mgr_init(); }

void PipeMgrIntf::pipeMgrCleanup() { return pipe_mgr_cleanup(); }

pipe_status_t PipeMgrIntf::pipeMgrClientInit(pipe_sess_hdl_t *sess_hdl) {
  return pipe_mgr_client_init(sess_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrClientCleanup(pipe_sess_hdl_t def_sess_hdl) {
  return pipe_mgr_client_cleanup(def_sess_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrCompleteOperations(
    pipe_sess_hdl_t def_sess_hdl) {
  return pipe_mgr_complete_operations(def_sess_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrBeginTxn(pipe_sess_hdl_t shdl,
                                           bool isAtomic) {
  return pipe_mgr_begin_txn(shdl, isAtomic);
}

pipe_status_t PipeMgrIntf::pipeMgrVerifyTxn(pipe_sess_hdl_t shdl) {
  return pipe_mgr_verify_txn(shdl);
}

pipe_status_t PipeMgrIntf::pipeMgrAbortTxn(pipe_sess_hdl_t shdl) {
  return pipe_mgr_abort_txn(shdl);
}

pipe_status_t PipeMgrIntf::pipeMgrCommitTxn(pipe_sess_hdl_t shdl,
                                            bool hwSynchronous) {
  return pipe_mgr_commit_txn(shdl, hwSynchronous);
}

pipe_status_t PipeMgrIntf::pipeMgrBeginBatch(pipe_sess_hdl_t shdl) {
  return pipe_mgr_begin_batch(shdl);
}

pipe_status_t PipeMgrIntf::pipeMgrFlushBatch(pipe_sess_hdl_t shdl) {
  return pipe_mgr_flush_batch(shdl);
}

pipe_status_t PipeMgrIntf::pipeMgrEndBatch(pipe_sess_hdl_t shdl,
                                           bool hwSynchronous) {
  return pipe_mgr_end_batch(shdl, hwSynchronous);
}

pipe_status_t PipeMgrIntf::pipeMgrMatchSpecFree(
    pipe_tbl_match_spec_t *match_spec) {
  return pipe_mgr_match_spec_free(match_spec);
}

pipe_status_t PipeMgrIntf::pipeMgrGetActionDirectResUsage(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    bool *has_dir_stats,
    bool *has_dir_meter,
    bool *has_dir_lpf,
    bool *has_dir_wred,
    bool *has_dir_stful) {
  return pipe_mgr_get_action_dir_res_usage(device_id,
                                           mat_tbl_hdl,
                                           act_fn_hdl,
                                           has_dir_stats,
                                           has_dir_meter,
                                           has_dir_lpf,
                                           has_dir_wred,
                                           has_dir_stful);
}

pipe_status_t PipeMgrIntf::pipeMgrMatchSpecToEntHdl(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_mat_ent_hdl_t *mat_ent_hdl) {
  return pipe_mgr_match_spec_to_ent_hdl(
      sess_hdl, dev_tgt, mat_tbl_hdl, match_spec, mat_ent_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrEntHdlToMatchSpec(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    bf_dev_pipe_t *entry_pipe,
    const pipe_tbl_match_spec_t **match_spec) {
  return pipe_mgr_ent_hdl_to_match_spec(
      sess_hdl, dev_tgt, mat_tbl_hdl, mat_ent_hdl, entry_pipe, match_spec);
}

pipe_status_t PipeMgrIntf::pipeMgrMatchKeyMaskSpecSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec) {
  return pipe_mgr_match_key_mask_spec_set(
      sess_hdl, device_id, mat_tbl_hdl, match_spec);
}

pipe_status_t PipeMgrIntf::pipeMgrMatchKeyMaskSpecReset(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  return pipe_mgr_match_key_mask_spec_reset(sess_hdl, device_id, mat_tbl_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrMatchKeyMaskSpecGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec) {
  return pipe_mgr_match_key_mask_spec_get(
      sess_hdl, device_id, mat_tbl_hdl, match_spec);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntAdd(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    const pipe_action_spec_t *act_data_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t *ent_hdl_p) {
  return pipe_mgr_mat_ent_add(sess_hdl,
                              dev_tgt,
                              mat_tbl_hdl,
                              match_spec,
                              act_fn_hdl,
                              (pipe_action_spec_t *)act_data_spec,
                              ttl,
                              pipe_api_flags,
                              ent_hdl_p);
}

pipe_status_t PipeMgrIntf::pipeMgrMatDefaultEntrySet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    const pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t *ent_hdl_p) {
  return pipe_mgr_mat_default_entry_set(sess_hdl,
                                        dev_tgt,
                                        mat_tbl_hdl,
                                        act_fn_hdl,
                                        (pipe_action_spec_t *)act_spec,
                                        pipe_api_flags,
                                        ent_hdl_p);
}

pipe_status_t PipeMgrIntf::pipeMgrTableGetDefaultEntry(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw,
    uint32_t res_get_flags,
    pipe_res_get_data_t *res_data) {
  return pipe_mgr_table_get_default_entry(sess_hdl,
                                          dev_tgt,
                                          mat_tbl_hdl,
                                          pipe_action_spec,
                                          act_fn_hdl,
                                          from_hw,
                                          res_get_flags,
                                          res_data);
}

pipe_status_t PipeMgrIntf::pipeMgrTableGetDefaultEntryHandle(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t *ent_hdl_p) {
  return pipe_mgr_table_get_default_entry_handle(
      sess_hdl, dev_tgt, mat_tbl_hdl, ent_hdl_p);
}

pipe_status_t PipeMgrIntf::pipeMgrMatTblClear(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              uint32_t pipe_api_flags) {
  return pipe_mgr_mat_tbl_clear(sess_hdl, dev_tgt, mat_tbl_hdl, pipe_api_flags);
}
pipe_status_t PipeMgrIntf::pipeMgrMatEntDel(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_mat_ent_hdl_t mat_ent_hdl,
                                            uint32_t pipe_api_flags) {
  return pipe_mgr_mat_ent_del(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntDelByMatchSpec(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    uint32_t pipe_api_flags) {
  return pipe_mgr_mat_ent_del_by_match_spec(
      sess_hdl, dev_tgt, mat_tbl_hdl, match_spec, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMatTblDefaultEntryReset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t pipe_api_flags) {
  return pipe_mgr_mat_tbl_default_entry_reset(
      sess_hdl, dev_tgt, mat_tbl_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntSetAction(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags) {
  return pipe_mgr_mat_ent_set_action(sess_hdl,
                                     device_id,
                                     mat_tbl_hdl,
                                     mat_ent_hdl,
                                     act_fn_hdl,
                                     act_spec,
                                     pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntSetActionByMatchSpec(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    const pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags) {
  return pipe_mgr_mat_ent_set_action_by_match_spec(
      sess_hdl,
      dev_tgt,
      mat_tbl_hdl,
      match_spec,
      act_fn_hdl,
      (pipe_action_spec_t *)act_spec,
      pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntSetResource(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_res_spec_t *resources,
    int resource_count,
    uint32_t pipe_api_flags) {
  return pipe_mgr_mat_ent_set_resource(sess_hdl,
                                       device_id,
                                       mat_tbl_hdl,
                                       mat_ent_hdl,
                                       resources,
                                       resource_count,
                                       pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrAdtEntAdd(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    const pipe_action_spec_t *action_spec,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p,
    uint32_t pipe_api_flags) {
  return pipe_mgr_adt_ent_add(sess_hdl,
                              dev_tgt,
                              adt_tbl_hdl,
                              act_fn_hdl,
                              (pipe_action_spec_t *)action_spec,
                              adt_ent_hdl_p,
                              pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrAdtEntDel(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                            pipe_adt_ent_hdl_t adt_ent_hdl,
                                            uint32_t pipe_api_flags) {
  return pipe_mgr_adt_ent_del(
      sess_hdl, dev_tgt, adt_tbl_hdl, adt_ent_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrAdtEntSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    const pipe_action_spec_t *action_spec,
    uint32_t pipe_api_flags) {
  return pipe_mgr_adt_ent_set(sess_hdl,
                              device_id,
                              adt_tbl_hdl,
                              adt_ent_hdl,
                              act_fn_hdl,
                              (pipe_action_spec_t *)action_spec,
                              pipe_api_flags);
}
pipe_status_t PipeMgrIntf::pipeMgrTblIsTern(bf_dev_id_t dev_id,
                                            pipe_tbl_hdl_t tbl_hdl,
                                            bool *is_tern) {
  return pipe_mgr_tbl_is_tern(dev_id, tbl_hdl, is_tern);
}

pipe_status_t PipeMgrIntf::pipeMgrSelTblRegisterCb(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_mgr_sel_tbl_update_callback cb,
    void *cb_cookie) {
  return pipe_mgr_sel_tbl_register_cb(
      sess_hdl, device_id, sel_tbl_hdl, cb, cb_cookie);
}

pipe_status_t PipeMgrIntf::pipeMgrSelTblProfileSet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_tbl_profile_t *sel_tbl_profile) {
  return pipe_mgr_sel_tbl_profile_set(
      sess_hdl, dev_tgt, sel_tbl_hdl, sel_tbl_profile);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpAdd(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            uint32_t max_grp_size,
                                            pipe_sel_grp_hdl_t *sel_grp_hdl_p,
                                            uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_add(sess_hdl,
                              dev_tgt,
                              sel_tbl_hdl,
                              max_grp_size,
                              sel_grp_hdl_p,
                              pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpDel(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_sel_grp_hdl_t sel_grp_hdl,
                                            uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_del(
      sess_hdl, dev_tgt, sel_tbl_hdl, sel_grp_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrAdd(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t device_id,
                                               pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                               pipe_sel_grp_hdl_t sel_grp_hdl,
                                               pipe_act_fn_hdl_t act_fn_hdl,
                                               pipe_adt_ent_hdl_t adt_ent_hdl,
                                               uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_mbr_add(sess_hdl,
                                  device_id,
                                  sel_tbl_hdl,
                                  sel_grp_hdl,
                                  act_fn_hdl,
                                  adt_ent_hdl,
                                  pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrDel(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t device_id,
                                               pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                               pipe_sel_grp_hdl_t sel_grp_hdl,
                                               pipe_adt_ent_hdl_t adt_ent_hdl,
                                               uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_mbr_del(sess_hdl,
                                  device_id,
                                  sel_tbl_hdl,
                                  sel_grp_hdl,
                                  adt_ent_hdl,
                                  pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrsSet(pipe_sess_hdl_t sess_hdl,
                                                dev_target_t dev_tgt,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                pipe_sel_grp_hdl_t sel_grp_hdl,
                                                uint32_t num_mbrs,
                                                pipe_adt_ent_hdl_t *mbrs,
                                                bool *enable,
                                                uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_mbrs_set(sess_hdl,
                                   dev_tgt,
                                   sel_tbl_hdl,
                                   sel_grp_hdl,
                                   num_mbrs,
                                   mbrs,
                                   enable,
                                   pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrsGet(pipe_sess_hdl_t sess_hdl,
                                                dev_target_t dev_tgt,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                pipe_sel_grp_hdl_t sel_grp_hdl,
                                                uint32_t mbrs_size,
                                                pipe_adt_ent_hdl_t *mbrs,
                                                bool *enable,
                                                uint32_t *mbrs_populated) {
  return pipe_mgr_sel_grp_mbrs_get(sess_hdl,
                                   dev_tgt,
                                   sel_tbl_hdl,
                                   sel_grp_hdl,
                                   mbrs_size,
                                   mbrs,
                                   enable,
                                   mbrs_populated);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrDisable(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_mbr_disable(sess_hdl,
                                      device_id,
                                      sel_tbl_hdl,
                                      sel_grp_hdl,
                                      adt_ent_hdl,
                                      pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrEnable(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_mbr_enable(sess_hdl,
                                     device_id,
                                     sel_tbl_hdl,
                                     sel_grp_hdl,
                                     adt_ent_hdl,
                                     pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrStateGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    enum pipe_mgr_grp_mbr_state_e *mbr_state_p) {
  return pipe_mgr_sel_grp_mbr_state_get(
      sess_hdl, device_id, sel_tbl_hdl, sel_grp_hdl, adt_ent_hdl, mbr_state_p);
}

pipe_status_t PipeMgrIntf::pipeMgrSelFallbackMbrSet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags) {
  return pipe_mgr_sel_fallback_mbr_set(
      sess_hdl, dev_tgt, sel_tbl_hdl, adt_ent_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelFallbackMbrReset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    uint32_t pipe_api_flags) {
  return pipe_mgr_sel_fallback_mbr_reset(
      sess_hdl, dev_tgt, sel_tbl_hdl, pipe_api_flags);
}
pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrGetFromHash(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t grp_hdl,
    uint8_t *hash,
    uint32_t hash_len,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p) {
  return pipe_mgr_sel_grp_mbr_get_from_hash(
      sess_hdl, dev_id, sel_tbl_hdl, grp_hdl, hash, hash_len, adt_ent_hdl_p);
}

pipe_status_t PipeMgrIntf::pipeMgrLrnDigestNotificationRegister(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
    pipe_flow_lrn_notify_cb callback_fn,
    void *callback_fn_cookie) {
  return pipe_mgr_lrn_digest_notification_register(sess_hdl,
                                                   device_id,
                                                   flow_lrn_fld_lst_hdl,
                                                   callback_fn,
                                                   callback_fn_cookie);
}

pipe_status_t PipeMgrIntf::pipeMgrLrnDigestNotificationDeregister(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl) {
  return pipe_mgr_lrn_digest_notification_deregister(
      sess_hdl, device_id, flow_lrn_fld_lst_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnNotifyAck(
    pipe_sess_hdl_t sess_hdl,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
    pipe_flow_lrn_msg_t *pipe_flow_lrn_msg) {
  return pipe_mgr_flow_lrn_notify_ack(
      sess_hdl, flow_lrn_fld_lst_hdl, pipe_flow_lrn_msg);
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnTimeoutSet(pipe_sess_hdl_t sess_hdl,
                                                    bf_dev_id_t device_id,
                                                    uint32_t usecs) {
  return pipe_mgr_flow_lrn_set_timeout(sess_hdl, device_id, usecs);
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnTimeoutGet(bf_dev_id_t device_id,
                                                    uint32_t *usecs) {
	return BF_SUCCESS;
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnIntrModeSet(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t device_id,
                                                     bool en) {
	return BF_SUCCESS;
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnIntrModeGet(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t device_id,
                                                     bool *en) {
	return BF_SUCCESS;
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnSetNetworkOrderDigest(
    bf_dev_id_t device_id, bool network_order) {
  return pipe_mgr_flow_lrn_set_network_order_digest(device_id, network_order);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntDirectStatQuery(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_data_t *stat_data) {
  return pipe_mgr_mat_ent_direct_stat_query(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntDirectStatSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_data_t *stat_data) {
  return pipe_mgr_mat_ent_direct_stat_set(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntDirectStatLoad(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_data_t *stat_data) {
  return pipe_mgr_mat_ent_direct_stat_load(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrStatEntQuery(pipe_sess_hdl_t sess_hdl,
                                               dev_target_t dev_target,
                                               pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                               pipe_stat_ent_idx_t stat_ent_idx,
                                               pipe_stat_data_t *stat_data) {
  return pipe_mgr_stat_ent_query(
      sess_hdl, dev_target, stat_tbl_hdl, stat_ent_idx, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrStatTableReset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_stat_data_t *stat_data) {
  return pipe_mgr_stat_table_reset(sess_hdl, dev_tgt, stat_tbl_hdl, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrStatEntSet(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                             pipe_stat_ent_idx_t stat_ent_idx,
                                             pipe_stat_data_t *stat_data) {
  return pipe_mgr_stat_ent_set(
      sess_hdl, dev_tgt, stat_tbl_hdl, stat_ent_idx, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrStatEntLoad(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                              pipe_stat_ent_idx_t stat_idx,
                                              pipe_stat_data_t *stat_data) {
  return pipe_mgr_stat_ent_load(
      sess_hdl, dev_tgt, stat_tbl_hdl, stat_idx, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrStatDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  return pipe_mgr_stat_database_sync(
      sess_hdl, dev_tgt, stat_tbl_hdl, cback_fn, cookie);
}

pipe_status_t PipeMgrIntf::pipeMgrDirectStatDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  return pipe_mgr_direct_stat_database_sync(
      sess_hdl, dev_tgt, mat_tbl_hdl, cback_fn, cookie);
}

pipe_status_t PipeMgrIntf::pipeMgrStatEntDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_stat_ent_idx_t stat_ent_idx) {
  return pipe_mgr_stat_ent_database_sync(
      sess_hdl, dev_tgt, stat_tbl_hdl, stat_ent_idx);
}

pipe_status_t PipeMgrIntf::pipeMgrDirectStatEntDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  return pipe_mgr_direct_stat_ent_database_sync(
      sess_hdl, dev_tgt, mat_tbl_hdl, mat_ent_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterEntSet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_idx_t meter_idx,
    pipe_meter_spec_t *meter_spec,
    uint32_t pipe_api_flags) {
  return pipe_mgr_meter_ent_set(
      sess_hdl, dev_tgt, meter_tbl_hdl, meter_idx, meter_spec, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterByteCountSet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    int byte_count) {
  return pipe_mgr_meter_set_bytecount_adjust(
      sess_hdl, dev_tgt, meter_tbl_hdl, byte_count);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterByteCountGet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    int *byte_count) {
  return pipe_mgr_meter_get_bytecount_adjust(
      sess_hdl, dev_tgt, meter_tbl_hdl, byte_count);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterReset(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                             uint32_t pipe_api_flags) {
  return pipe_mgr_meter_reset(sess_hdl, dev_tgt, meter_tbl_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterReadEntry(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_meter_spec_t *meter_spec) {
  return pipe_mgr_meter_read_entry(
      sess_hdl, dev_tgt, mat_tbl_hdl, mat_ent_hdl, meter_spec);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterReadEntryIdx(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_idx_t index,
    pipe_meter_spec_t *meter_spec) {
  return pipe_mgr_meter_read_entry_idx(
      sess_hdl, dev_tgt, meter_tbl_hdl, index, meter_spec);
}


pipe_status_t PipeMgrIntf::pipeMgrExmEntryActivate(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  return pipe_mgr_exm_entry_activate(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrExmEntryDeactivate(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  return pipe_mgr_exm_entry_deactivate(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntSetIdleTtl(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    bool reset) {
  return pipe_mgr_mat_ent_set_idle_ttl(sess_hdl,
                                       device_id,
                                       mat_tbl_hdl,
                                       mat_ent_hdl,
                                       ttl,
                                       pipe_api_flags,
                                       reset);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntResetIdleTtl(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  return pipe_mgr_mat_ent_reset_idle_ttl(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleTmoEnableSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    bool enable) {
	return BF_SUCCESS;
}

pipe_status_t PipeMgrIntf::pipeMgrIdleRegisterTmoCb(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_tmo_expiry_cb cb,
    void *client_data) {
  return pipe_mgr_idle_register_tmo_cb(
      sess_hdl, device_id, mat_tbl_hdl, cb, client_data);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleRegisterTmoCbWithMatchSpecCopy(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_tmo_expiry_cb_with_match_spec_copy cb,
    void *client_data) {
  return pipe_mgr_idle_register_tmo_cb_with_match_spec_copy(
      sess_hdl, device_id, mat_tbl_hdl, cb, client_data);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleTimeGetHitState(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_idle_time_hit_state_e *idle_time_data) {
  return pipe_mgr_idle_time_get_hit_state(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, idle_time_data);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleTimeSetHitState(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_idle_time_hit_state_e idle_time_data) {
	return BF_SUCCESS;
}

pipe_status_t PipeMgrIntf::pipeMgrIdleTimeUpdateHitState(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_tmo_update_complete_cb callback_fn,
    void *cb_data) {
  return pipe_mgr_idle_time_update_hit_state(
      sess_hdl, device_id, mat_tbl_hdl, callback_fn, cb_data);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntGetIdleTtl(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t *ttl) {
  return pipe_mgr_mat_ent_get_idle_ttl(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, ttl);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleParamsGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_time_params_t *params) {
	return BF_SUCCESS;
}

pipe_status_t PipeMgrIntf::pipeMgrIdleParamsSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_time_params_t params) {
	return BF_SUCCESS;
}

pipe_status_t PipeMgrIntf::pipeStfulEntSet(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_target,
                                           pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                           pipe_stful_mem_idx_t stful_ent_idx,
                                           pipe_stful_mem_spec_t *stful_spec,
                                           uint32_t pipe_api_flags) {
  return pipe_stful_ent_set(sess_hdl,
                            dev_target,
                            stful_tbl_hdl,
                            stful_ent_idx,
                            stful_spec,
                            pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeStfulDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    pipe_stful_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  return pipe_stful_database_sync(
      sess_hdl, dev_tgt, stful_tbl_hdl, cback_fn, cookie);
}

pipe_status_t PipeMgrIntf::pipeStfulDirectDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_stful_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  return pipe_stful_direct_database_sync(
      sess_hdl, dev_tgt, mat_tbl_hdl, cback_fn, cookie);
}

pipe_status_t PipeMgrIntf::pipeStfulQueryGetSizes(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    int *num_pipes) {
  return pipe_stful_query_get_sizes(
      sess_hdl, device_id, stful_tbl_hdl, num_pipes);
}

pipe_status_t PipeMgrIntf::pipeStfulDirectQueryGetSizes(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    int *num_pipes) {
  return pipe_stful_direct_query_get_sizes(
      sess_hdl, device_id, mat_tbl_hdl, num_pipes);
}

pipe_status_t PipeMgrIntf::pipeStfulEntQuery(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    pipe_stful_mem_idx_t stful_ent_idx,
    pipe_stful_mem_query_t *stful_query,
    uint32_t pipe_api_flags) {
  return pipe_stful_ent_query(sess_hdl,
                              dev_tgt,
                              stful_tbl_hdl,
                              stful_ent_idx,
                              stful_query,
                              pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeStfulDirectEntQuery(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stful_mem_query_t *stful_query,
    uint32_t pipe_api_flags) {
  return pipe_stful_direct_ent_query(sess_hdl,
                                     device_id,
                                     mat_tbl_hdl,
                                     mat_ent_hdl,
                                     stful_query,
                                     pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeStfulTableReset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    pipe_stful_mem_spec_t *stful_spec) {
  return pipe_stful_table_reset(sess_hdl, dev_tgt, stful_tbl_hdl, stful_spec);
}

pipe_status_t PipeMgrIntf::pipeStfulTableResetRange(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    pipe_stful_mem_idx_t stful_ent_idx,
    uint32_t num_indices,
    pipe_stful_mem_spec_t *stful_spec) {
  return pipe_stful_table_reset_range(
      sess_hdl, dev_tgt, stful_tbl_hdl, stful_ent_idx, num_indices, stful_spec);
}

pipe_status_t PipeMgrIntf::pipeStfulParamSet(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_tbl_hdl_t tbl_hdl,
                                             pipe_reg_param_hdl_t rp_hdl,
                                             int64_t value) {
	return BF_SUCCESS;
}

pipe_status_t PipeMgrIntf::pipeStfulParamGet(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_tbl_hdl_t tbl_hdl,
                                             pipe_reg_param_hdl_t rp_hdl,
                                             int64_t *value) {
	return BF_SUCCESS;
}

pipe_status_t PipeMgrIntf::pipeStfulParamReset(pipe_sess_hdl_t sess_hdl,
                                               dev_target_t dev_tgt,
                                               pipe_tbl_hdl_t tbl_hdl,
                                               pipe_reg_param_hdl_t rp_hdl) {
	return BF_SUCCESS;
}

pipe_status_t PipeMgrIntf::pipeStfulParamGetHdl(bf_dev_id_t dev,
                                                const char *name,
                                                pipe_reg_param_hdl_t *rp_hdl) {
	return BF_SUCCESS;
}

bf_dev_pipe_t PipeMgrIntf::devPortToPipeId(uint16_t dev_port_id) {
  return dev_port_to_pipe_id(dev_port_id);
}

pipe_status_t PipeMgrIntf::pipeMgrStoreEntries(pipe_sess_hdl_t sess_hdl,
                                      pipe_mat_tbl_hdl_t tbl_hdl,
                                      dev_target_t dev_tgt,
				      bool *store_entries) {
	return pipe_mgr_store_entries(sess_hdl, tbl_hdl, dev_tgt,
				      store_entries);
}

pipe_status_t PipeMgrIntf::pipeMgrGetFirstEntry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t *pipe_act_fn_hdl) {
	return pipe_mgr_get_first_entry(sess_hdl, tbl_hdl, dev_tgt, match_spec,
					action_spec, pipe_act_fn_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrGetNextNByKey(
    pipe_sess_hdl_t sess_hdl,
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_tbl_match_spec_t *cur_match_spec,
    int n,
    pipe_tbl_match_spec_t *match_specs,
    pipe_action_spec_t **action_specs,
    pipe_act_fn_hdl_t *pipe_act_fn_hdls,
    uint32_t *num) {
	return pipe_mgr_get_next_n_by_key(sess_hdl, tbl_hdl, dev_tgt,
					  cur_match_spec, n, match_specs,
					  action_specs, pipe_act_fn_hdls, num);
}


pipe_status_t PipeMgrIntf::pipeMgrGetFirstEntryHandle(
    pipe_sess_hdl_t sess_hdl,
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    uint32_t *entry_handle) {
  return pipe_mgr_get_first_entry_handle(
      sess_hdl, tbl_hdl, dev_tgt, entry_handle);
}

pipe_status_t PipeMgrIntf::pipeMgrGetNextEntryHandles(
    pipe_sess_hdl_t sess_hdl,
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_handle,
    int n,
    uint32_t *next_entry_handles) {
  return pipe_mgr_get_next_entry_handles(
      sess_hdl, tbl_hdl, dev_tgt, entry_handle, n, next_entry_handles);
}

pipe_status_t PipeMgrIntf::pipeMgrGetFirstGroupMember(
    pipe_sess_hdl_t sess_hdl,
    pipe_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t *mbr_hdl) {
  return pipe_mgr_get_first_group_member(
      sess_hdl, tbl_hdl, dev_tgt, sel_grp_hdl, mbr_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrGetNextGroupMembers(
    pipe_sess_hdl_t sess_hdl,
    pipe_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t mbr_hdl,
    int n,
    pipe_adt_ent_hdl_t *next_mbr_hdls) {
  return pipe_mgr_get_next_group_members(
      sess_hdl, tbl_hdl, dev_id, sel_grp_hdl, mbr_hdl, n, next_mbr_hdls);
}

pipe_status_t PipeMgrIntf::pipeMgrGetSelGrpMbrCount(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    uint32_t *count) {
  return pipe_mgr_get_sel_grp_mbr_count(
      sess_hdl, dev_tgt, tbl_hdl, sel_grp_hdl, count);
}

pipe_status_t PipeMgrIntf::pipeMgrGetReservedEntryCount(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t tbl_hdl,
    size_t *count) {
	return BF_SUCCESS;
}

pipe_status_t PipeMgrIntf::pipeMgrGetEntryCount(pipe_sess_hdl_t sess_hdl,
                                                dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                bool read_from_hw,
                                                uint32_t *count) {
  return pipe_mgr_get_entry_count(
      sess_hdl, dev_tgt, tbl_hdl, read_from_hw, count);
}

pipe_status_t PipeMgrIntf::pipeMgrGetEntry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mat_tbl_hdl_t tbl_hdl,
    struct bf_dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw,
    uint32_t res_get_flags,
    pipe_res_get_data_t *res_data) {
  return pipe_mgr_get_entry(sess_hdl,
                            tbl_hdl,
                            dev_tgt,
                            entry_hdl,
                            pipe_match_spec,
                            pipe_action_spec,
                            act_fn_hdl,
                            from_hw,
                            res_get_flags,
                            res_data);
}

pipe_status_t PipeMgrIntf::pipeMgrGetActionDataEntry(
    pipe_sess_hdl_t sess_hdl,
    pipe_adt_tbl_hdl_t tbl_hdl,
    struct bf_dev_target_t dev_tgt,
    pipe_adt_ent_hdl_t entry_hdl,
    pipe_action_data_spec_t *data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw) {
  return pipe_mgr_get_action_data_entry(
      sess_hdl, tbl_hdl, dev_tgt, entry_hdl, data_spec, act_fn_hdl, from_hw);
}

pipe_status_t PipeMgrIntf::pipeMgrTblSetProperty(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mgr_tbl_prop_type_t property,
    pipe_mgr_tbl_prop_value_t value,
    pipe_mgr_tbl_prop_args_t args) {
  return pipe_mgr_tbl_set_property(
      sess_hdl, dev_id, tbl_hdl, property, value, args);
}

pipe_status_t PipeMgrIntf::pipeMgrTblGetProperty(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mgr_tbl_prop_type_t property,
    pipe_mgr_tbl_prop_value_t *value,
    pipe_mgr_tbl_prop_args_t *args) {
  return pipe_mgr_tbl_get_property(
      sess_hdl, dev_id, tbl_hdl, property, value, args);
}

bf_dev_pipe_t PipeMgrIntf::pipeGetHdlPipe(pipe_mat_ent_hdl_t entry_hdl) {
  return PIPE_GET_HDL_PIPE(entry_hdl);
}

/************ Table Debug Counters APIs **************/
bf_status_t PipeMgrIntf::bfDbgCounterGet(bf_dev_target_t dev_tgt,
                                         char *tbl_name,
                                         bf_tbl_dbg_counter_type_t *type,
                                         uint32_t *value) {
	return BF_SUCCESS;
}

bf_status_t PipeMgrIntf::bfDbgCounterGet(bf_dev_target_t dev_tgt,
                                         uint32_t stage,
                                         uint32_t log_tbl,
                                         bf_tbl_dbg_counter_type_t *type,
                                         uint32_t *value) {
	return BF_SUCCESS;
}

bf_status_t PipeMgrIntf::bfDbgCounterSet(bf_dev_target_t dev_tgt,
                                         char *tbl_name,
                                         bf_tbl_dbg_counter_type_t type) {
  return bf_tbl_dbg_counter_type_set(dev_tgt, tbl_name, type);
}

bf_status_t PipeMgrIntf::bfDbgCounterSet(bf_dev_target_t dev_tgt,
                                         uint32_t stage,
                                         uint32_t log_tbl,
                                         bf_tbl_dbg_counter_type_t type) {
	return BF_SUCCESS;
}

bf_status_t PipeMgrIntf::bfDbgCounterClear(bf_dev_target_t dev_tgt,
                                           char *tbl_name) {
	return BF_SUCCESS;
}

bf_status_t PipeMgrIntf::bfDbgCounterClear(bf_dev_target_t dev_tgt,
                                           uint32_t stage,
                                           uint32_t log_tbl) {
	return BF_SUCCESS;
}

bf_status_t PipeMgrIntf::bfDbgCounterTableListGet(bf_dev_target_t dev_tgt,
                                                  char **tbl_list,
                                                  int *num_tbls) {
	return BF_SUCCESS;
}

bf_status_t PipeMgrIntf::pipeMgrTblHdlPipeMaskGet(
    bf_dev_id_t dev_id,
    const std::string &prog_name,
    const std::string &pipeline_name,
    uint32_t *pipe_mask) const {
  return pipe_mgr_tbl_hdl_pipe_mask_get(
      dev_id, prog_name.c_str(), pipeline_name.c_str(), pipe_mask);
}

pipe_status_t PipeMgrIntf::pipeMgrGetNumPipelines(bf_dev_id_t dev_id,
                                                  uint32_t *num_pipes) const {
  return pipe_mgr_get_num_pipelines(dev_id, num_pipes);
}

bf_status_t PipeMgrIntf::pipeMgrEnablePipeline(bf_dev_id_t dev_id) const {
   return pipe_mgr_enable_pipeline(dev_id);
}

}  // namespace bfrt

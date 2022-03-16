/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) 2017-2018 Barefoot Networks, Inc.

 * All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of
 * Barefoot Networks, Inc. and its suppliers, if any. The intellectual and
 * technical concepts contained herein are proprietary to Barefoot Networks,
 * Inc.
 * and its suppliers and may be covered by U.S. and Foreign Patents, patents in
 * process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material is
 * strictly forbidden unless prior written permission is obtained from
 * Barefoot Networks, Inc.
 *
 * No warranty, explicit or implicit is provided, unless granted under a
 * written agreement with Barefoot Networks, Inc.
 *
 * $Id: $
 *
 ******************************************************************************/

#ifndef _TDI_PIPE_MGR_INTERFACE_HPP
#define _TDI_PIPE_MGR_INTERFACE_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_types/bf_types.h>
//#include <bfsys/bf_sal/bf_sys_mem.h>
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

/* tdi_includes */

namespace tdi {

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
      tdi_dev_id_t device_id,
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
      tdi_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_tbl_match_spec_t *match_spec) = 0;

  virtual pipe_status_t pipeMgrMatchKeyMaskSpecReset(
      pipe_sess_hdl_t sess_hdl,
      tdi_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl) = 0;

  virtual pipe_status_t pipeMgrMatchKeyMaskSpecGet(
      pipe_sess_hdl_t sess_hdl,
      tdi_dev_id_t device_id,
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
                                         tdi_dev_id_t device_id,
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
                                               tdi_dev_id_t device_id,
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
                                                 tdi_dev_id_t device_id,
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
                                         tdi_dev_id_t device_id,
                                         pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                         pipe_adt_ent_hdl_t adt_ent_hdl,
                                         uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrAdtEntSet(pipe_sess_hdl_t sess_hdl,
                                         tdi_dev_id_t device_id,
                                         pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                         pipe_adt_ent_hdl_t adt_ent_hdl,
                                         pipe_act_fn_hdl_t act_fn_hdl,
                                         const pipe_action_spec_t *action_spec,
                                         uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrTblIsTern(tdi_dev_id_t dev_id,
                                         pipe_tbl_hdl_t tbl_hdl,
                                         bool *is_tern) = 0;

  // API for Selector Table Manipulation
  virtual pipe_status_t pipeMgrSelTblRegisterCb(
      pipe_sess_hdl_t sess_hdl,
      tdi_dev_id_t device_id,
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
                                         tdi_dev_id_t device_id,
                                         pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                         pipe_sel_grp_hdl_t sel_grp_hdl,
                                         uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpSizeSet(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                             pipe_sel_grp_hdl_t sel_grp_hdl,
                                             uint32_t max_grp_size) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrAdd(pipe_sess_hdl_t sess_hdl,
                                            tdi_dev_id_t device_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_sel_grp_hdl_t sel_grp_hdl,
                                            pipe_act_fn_hdl_t act_fn_hdl,
                                            pipe_adt_ent_hdl_t adt_ent_hdl,
                                            uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrDel(pipe_sess_hdl_t sess_hdl,
                                            tdi_dev_id_t device_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_sel_grp_hdl_t sel_grp_hdl,
                                            pipe_adt_ent_hdl_t adt_ent_hdl,
                                            uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrsSet(pipe_sess_hdl_t sess_hdl,
                                             tdi_dev_id_t device_id,
                                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                             pipe_sel_grp_hdl_t sel_grp_hdl,
                                             uint32_t num_mbrs,
                                             pipe_adt_ent_hdl_t *mbrs,
                                             bool *enable,
                                             uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrsGet(pipe_sess_hdl_t sess_hdl,
                                             tdi_dev_id_t device_id,
                                             pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                             pipe_sel_grp_hdl_t sel_grp_hdl,
                                             uint32_t mbrs_size,
                                             pipe_adt_ent_hdl_t *mbrs,
                                             bool *enable,
                                             uint32_t *mbrs_populated) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrDisable(pipe_sess_hdl_t sess_hdl,
                                                tdi_dev_id_t device_id,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                pipe_sel_grp_hdl_t sel_grp_hdl,
                                                pipe_adt_ent_hdl_t adt_ent_hdl,
                                                uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrEnable(pipe_sess_hdl_t sess_hdl,
                                               tdi_dev_id_t device_id,
                                               pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                               pipe_sel_grp_hdl_t sel_grp_hdl,
                                               pipe_adt_ent_hdl_t adt_ent_hdl,
                                               uint32_t pipe_api_flags) = 0;

  virtual pipe_status_t pipeMgrSelGrpMbrStateGet(
      pipe_sess_hdl_t sess_hdl,
      tdi_dev_id_t device_id,
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
      tdi_dev_id_t dev_id,
      pipe_sel_tbl_hdl_t sel_tbl_hdl,
      pipe_sel_grp_hdl_t grp_hdl,
      uint8_t *hash,
      uint32_t hash_len,
      pipe_adt_ent_hdl_t *adt_ent_hdl_p) = 0;

  // API for Statistics Table Manipulation

  virtual pipe_status_t pipeMgrMatEntDirectStatQuery(
      pipe_sess_hdl_t sess_hdl,
      tdi_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      pipe_stat_data_t *stat_data) = 0;

  virtual pipe_status_t pipeMgrMatEntDirectStatSet(
      pipe_sess_hdl_t sess_hdl,
      tdi_dev_id_t device_id,
      pipe_mat_tbl_hdl_t mat_tbl_hdl,
      pipe_mat_ent_hdl_t mat_ent_hdl,
      pipe_stat_data_t *stat_data) = 0;

  virtual pipe_status_t pipeMgrMatEntDirectStatLoad(
      pipe_sess_hdl_t sess_hdl,
      tdi_dev_id_t device_id,
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

  virtual tdi_status_t pipeMgrTblHdlPipeMaskGet(tdi_dev_id_t dev_id,
                                               const std::string &prog_name,
                                               const std::string &pipeline_name,
                                               uint32_t *pipe_mask) const = 0;

  virtual pipe_status_t pipeMgrGetNumPipelines(tdi_dev_id_t dev_id,
                                               uint32_t *num_pipes) const = 0;

  virtual bf_status_t pipeMgrEnablePipeline(tdi_dev_id_t dev_id) const = 0;
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
  pipe_status_t pipeMgrGetActionDirectResUsage(tdi_dev_id_t device_id,
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
                                           tdi_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_tbl_match_spec_t *match_spec);

  pipe_status_t pipeMgrMatchKeyMaskSpecReset(pipe_sess_hdl_t sess_hdl,
                                             tdi_dev_id_t device_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl);

  pipe_status_t pipeMgrMatchKeyMaskSpecGet(pipe_sess_hdl_t sess_hdl,
                                           tdi_dev_id_t device_id,
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
                                 tdi_dev_id_t device_id,
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
                                       tdi_dev_id_t device_id,
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
                                         tdi_dev_id_t device_id,
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
                                 tdi_dev_id_t device_id,
                                 pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                 pipe_adt_ent_hdl_t adt_ent_hdl,
                                 uint32_t pipe_api_flags);

  pipe_status_t pipeMgrAdtEntSet(pipe_sess_hdl_t sess_hdl,
                                 tdi_dev_id_t device_id,
                                 pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                 pipe_adt_ent_hdl_t adt_ent_hdl,
                                 pipe_act_fn_hdl_t act_fn_hdl,
                                 const pipe_action_spec_t *action_spec,
                                 uint32_t pipe_api_flags);
  pipe_status_t pipeMgrTblIsTern(tdi_dev_id_t dev_id,
                                 pipe_tbl_hdl_t tbl_hdl,
                                 bool *is_tern);

  // API for Selector Table Manipulation
  pipe_status_t pipeMgrSelTblRegisterCb(pipe_sess_hdl_t sess_hdl,
                                        tdi_dev_id_t device_id,
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
                                 tdi_dev_id_t device_id,
                                 pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                 pipe_sel_grp_hdl_t sel_grp_hdl,
                                 uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpSizeSet(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                     pipe_sel_grp_hdl_t sel_grp_hdl,
                                     uint32_t max_grp_size);

  pipe_status_t pipeMgrSelGrpMbrAdd(pipe_sess_hdl_t sess_hdl,
                                    tdi_dev_id_t device_id,
                                    pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                    pipe_sel_grp_hdl_t sel_grp_hdl,
                                    pipe_act_fn_hdl_t act_fn_hdl,
                                    pipe_adt_ent_hdl_t adt_ent_hdl,
                                    uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpMbrDel(pipe_sess_hdl_t sess_hdl,
                                    tdi_dev_id_t device_id,
                                    pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                    pipe_sel_grp_hdl_t sel_grp_hdl,
                                    pipe_adt_ent_hdl_t adt_ent_hdl,
                                    uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpMbrsSet(pipe_sess_hdl_t sess_hdl,
                                     tdi_dev_id_t device_id,
                                     pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                     pipe_sel_grp_hdl_t sel_grp_hdl,
                                     uint32_t num_mbrs,
                                     pipe_adt_ent_hdl_t *mbrs,
                                     bool *enable,
                                     uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpMbrsGet(pipe_sess_hdl_t sess_hdl,
                                     tdi_dev_id_t device_id,
                                     pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                     pipe_sel_grp_hdl_t sel_grp_hdl,
                                     uint32_t mbrs_size,
                                     pipe_adt_ent_hdl_t *mbrs,
                                     bool *enable,
                                     uint32_t *mbrs_populated);

  pipe_status_t pipeMgrSelGrpMbrDisable(pipe_sess_hdl_t sess_hdl,
                                        tdi_dev_id_t device_id,
                                        pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                        pipe_sel_grp_hdl_t sel_grp_hdl,
                                        pipe_adt_ent_hdl_t adt_ent_hdl,
                                        uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpMbrEnable(pipe_sess_hdl_t sess_hdl,
                                       tdi_dev_id_t device_id,
                                       pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                       pipe_sel_grp_hdl_t sel_grp_hdl,
                                       pipe_adt_ent_hdl_t adt_ent_hdl,
                                       uint32_t pipe_api_flags);

  pipe_status_t pipeMgrSelGrpMbrStateGet(
      pipe_sess_hdl_t sess_hdl,
      tdi_dev_id_t device_id,
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
                                            tdi_dev_id_t dev_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_sel_grp_hdl_t grp_hdl,
                                            uint8_t *hash,
                                            uint32_t hash_len,
                                            pipe_adt_ent_hdl_t *adt_ent_hdl_p);

  // API for Statistics Table Manipulation

  pipe_status_t pipeMgrMatEntDirectStatQuery(pipe_sess_hdl_t sess_hdl,
                                             tdi_dev_id_t device_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_mat_ent_hdl_t mat_ent_hdl,
                                             pipe_stat_data_t *stat_data);

  pipe_status_t pipeMgrMatEntDirectStatSet(pipe_sess_hdl_t sess_hdl,
                                           tdi_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mat_ent_hdl_t mat_ent_hdl,
                                           pipe_stat_data_t *stat_data);

  pipe_status_t pipeMgrMatEntDirectStatLoad(pipe_sess_hdl_t sess_hdl,
                                            tdi_dev_id_t device_id,
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
                                        tdi_dev_id_t device_id,
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

  bf_dev_pipe_t pipeGetHdlPipe(pipe_mat_ent_hdl_t entry_hdl);

  
  tdi_status_t pipeMgrTblHdlPipeMaskGet(tdi_dev_id_t dev_id,
                                       const std::string &prog_name,
                                       const std::string &pipeline_name,
                                       uint32_t *pipe_mask) const;

  pipe_status_t pipeMgrGetNumPipelines(tdi_dev_id_t dev_id,
                                       uint32_t *num_pipes) const override;

  bf_status_t pipeMgrEnablePipeline(tdi_dev_id_t dev_id) const;
 private:
  PipeMgrIntf(const PipeMgrIntf &src) = delete;
  PipeMgrIntf &operator=(const PipeMgrIntf &rhs) = delete;
};

}  // tdi

#endif  // _TDI_PIPE_MGR_INTERFACE_HPP

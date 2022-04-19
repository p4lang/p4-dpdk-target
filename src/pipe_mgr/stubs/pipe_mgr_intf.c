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
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local module headers */
#include "../core/pipe_mgr_log.h"

bf_status_t pipe_register_mat_update_cb(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t tbl_hdl, pipe_mat_update_cb cb, void* cb_cookie)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return BF_SUCCESS;
}

bf_status_t pipe_register_adt_update_cb(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_adt_tbl_hdl_t tbl_hdl, pipe_adt_update_cb cb, void* cb_cookie)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return BF_SUCCESS;
}

bf_status_t pipe_register_sel_update_cb(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_sel_tbl_hdl_t tbl_hdl, pipe_sel_update_cb cb, void* cb_cookie)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return BF_SUCCESS;
}

pipe_status_t pipe_mgr_begin_txn(pipe_sess_hdl_t shdl, bool isAtomic)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_verify_txn(pipe_sess_hdl_t shdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_abort_txn(pipe_sess_hdl_t shdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_commit_txn(pipe_sess_hdl_t shdl, bool hwSynchronous)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_begin_batch(pipe_sess_hdl_t shdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_flush_batch(pipe_sess_hdl_t shdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_end_batch(pipe_sess_hdl_t shdl, bool hwSynchronous)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_complete_operations(pipe_sess_hdl_t shdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tbl_is_tern(bf_dev_id_t dev_id, pipe_tbl_hdl_t tbl_hdl, bool* is_tern)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_ent_hdl_to_match_spec(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, bf_dev_pipe_t* ent_pipe_id, pipe_tbl_match_spec_t const** match_spec)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_match_spec_free(pipe_tbl_match_spec_t* match_spec)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_match_spec_duplicate(pipe_tbl_match_spec_t** match_spec_dest, pipe_tbl_match_spec_t const* match_spec_src)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_match_key_mask_spec_set(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_tbl_match_spec_t* match_spec)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_match_key_mask_spec_reset(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_match_key_mask_spec_get(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_tbl_match_spec_t* match_spec)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_default_entry_set(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_act_fn_hdl_t act_fn_hdl, pipe_action_spec_t* act_spec, uint32_t pipe_api_flags, pipe_mat_ent_hdl_t* ent_hdl_p)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_table_get_default_entry_handle(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t* ent_hdl_p)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_table_get_default_entry(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_action_spec_t* pipe_action_spec, pipe_act_fn_hdl_t* act_fn_hdl, bool from_hw, uint32_t res_get_flags, pipe_res_get_data_t* res_data)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_action_dir_res_usage(bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_act_fn_hdl_t act_fn_hdl, bool* has_dir_stats, bool* has_dir_meter, bool* has_dir_lpf, bool* has_dir_wred, bool* has_dir_stful)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_tbl_clear(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ent_del(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_tbl_default_entry_reset(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ent_set_action(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, pipe_act_fn_hdl_t act_fn_hdl, pipe_action_spec_t* act_spec, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ent_set_action_by_match_spec(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_tbl_match_spec_t* match_spec, pipe_act_fn_hdl_t act_fn_hdl, pipe_action_spec_t* act_spec, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ent_set_resource(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, pipe_res_spec_t* resources, int resource_count, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_ent_set(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_adt_tbl_hdl_t adt_tbl_hdl, pipe_adt_ent_hdl_t adt_ent_hdl, pipe_act_fn_hdl_t act_fn_hdl, pipe_action_spec_t* action_spec, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_tbl_register_cb(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_sel_tbl_hdl_t sel_tbl_hdl, pipe_mgr_sel_tbl_update_callback cb, void* cb_cookie)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_tbl_profile_set(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_sel_tbl_hdl_t sel_tbl_hdl, pipe_sel_tbl_profile_t* sel_tbl_profile)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_mbr_add(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_sel_tbl_hdl_t sel_tbl_hdl, pipe_sel_grp_hdl_t sel_grp_hdl, pipe_act_fn_hdl_t act_fn_hdl, pipe_adt_ent_hdl_t adt_ent_hdl, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_mbr_del(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_sel_tbl_hdl_t sel_tbl_hdl, pipe_sel_grp_hdl_t sel_grp_hdl, pipe_adt_ent_hdl_t adt_ent_hdl, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_mbr_disable(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_sel_tbl_hdl_t sel_tbl_hdl, pipe_sel_grp_hdl_t sel_grp_hdl, pipe_adt_ent_hdl_t adt_ent_hdl, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_mbr_enable(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_sel_tbl_hdl_t sel_tbl_hdl, pipe_sel_grp_hdl_t sel_grp_hdl, pipe_adt_ent_hdl_t adt_ent_hdl, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_mbr_get_from_hash(pipe_sess_hdl_t sess_hdl, bf_dev_id_t dev_id, pipe_sel_tbl_hdl_t sel_tbl_hdl, pipe_sel_grp_hdl_t grp_hdl, uint8_t* hash, uint32_t hash_len, pipe_adt_ent_hdl_t* adt_ent_hdl_p)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_fallback_mbr_set(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_sel_tbl_hdl_t sel_tbl_hdl, pipe_adt_ent_hdl_t adt_ent_hdl, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_fallback_mbr_reset(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_sel_tbl_hdl_t sel_tbl_hdl, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_lrn_digest_notification_register(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl, pipe_flow_lrn_notify_cb callback_fn, void* callback_fn_cookie)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_lrn_digest_notification_deregister(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_flow_lrn_notify_ack(pipe_sess_hdl_t sess_hdl, pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl, pipe_flow_lrn_msg_t* pipe_flow_lrn_msg)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_flow_lrn_set_timeout(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, uint32_t usecs)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_flow_lrn_set_network_order_digest(bf_dev_id_t device_id, bool network_order)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ent_direct_stat_query(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, pipe_stat_data_t* stat_data)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ent_direct_stat_set(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, pipe_stat_data_t* stat_data)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ent_direct_stat_load(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, pipe_stat_data_t* stat_data)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_stat_table_reset(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stat_tbl_hdl_t stat_tbl_hdl, pipe_stat_data_t* stat_data)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_stat_ent_set(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stat_tbl_hdl_t stat_tbl_hdl, pipe_stat_ent_idx_t stat_ent_idx, pipe_stat_data_t* stat_data)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_stat_ent_load(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stat_tbl_hdl_t stat_tbl_hdl, pipe_stat_ent_idx_t stat_idx, pipe_stat_data_t* stat_data)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_stat_database_sync(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stat_tbl_hdl_t stat_tbl_hdl, pipe_mgr_stat_tbl_sync_cback_fn cback_fn, void* cookie)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_direct_stat_database_sync(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mgr_stat_tbl_sync_cback_fn cback_fn, void* cookie)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_stat_ent_database_sync(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stat_tbl_hdl_t stat_tbl_hdl, pipe_stat_ent_idx_t stat_ent_idx)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_direct_stat_ent_database_sync(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_reset(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_meter_tbl_hdl_t meter_tbl_hdl, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_ent_set(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_meter_tbl_hdl_t meter_tbl_hdl, pipe_meter_idx_t meter_idx, pipe_meter_spec_t* meter_spec, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_set_bytecount_adjust(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_meter_tbl_hdl_t meter_tbl_hdl, int bytecount)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_get_bytecount_adjust(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_meter_tbl_hdl_t meter_tbl_hdl, int* bytecount)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_read_entry(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, pipe_meter_spec_t* meter_spec)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_read_entry_idx(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_meter_tbl_hdl_t meter_tbl_hdl, pipe_meter_idx_t index, pipe_meter_spec_t* meter_spec)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}


pipe_status_t pipe_mgr_exm_entry_activate(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_entry_deactivate(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ent_set_idle_ttl(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, uint32_t ttl, uint32_t pipe_api_flags, bool reset)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ent_reset_idle_ttl(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_params_get(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_idle_time_params_t* params)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_tmo_enable(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_idle_time_params_t params)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_register_tmo_cb(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_idle_tmo_expiry_cb cb, void* client_data)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_register_tmo_cb_with_match_spec_copy(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_idle_tmo_expiry_cb_with_match_spec_copy cb, void* client_data)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_tmo_disable(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_time_get_hit_state(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, pipe_idle_time_hit_state_e* idle_time_data)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_time_update_hit_state(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_idle_tmo_update_complete_cb callback_fn, void* cb_data)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ent_get_idle_ttl(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, uint32_t* ttl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_ent_set(pipe_sess_hdl_t sess_hdl, dev_target_t dev_target, pipe_stful_tbl_hdl_t stful_tbl_hdl, pipe_stful_mem_idx_t stful_ent_idx, pipe_stful_mem_spec_t* stful_spec, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_database_sync(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stful_tbl_hdl_t stful_tbl_hdl, pipe_stful_tbl_sync_cback_fn cback_fn, void* cookie)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_direct_database_sync(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_stful_tbl_sync_cback_fn cback_fn, void* cookie)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_query_get_sizes(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_stful_tbl_hdl_t stful_tbl_hdl, int* num_pipes)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_direct_query_get_sizes(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, int* num_pipes)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_ent_query(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stful_tbl_hdl_t stful_tbl_hdl, pipe_stful_mem_idx_t stful_ent_idx, pipe_stful_mem_query_t* stful_query, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_direct_ent_query(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_mat_ent_hdl_t mat_ent_hdl, pipe_stful_mem_query_t* stful_query, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_table_reset(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stful_tbl_hdl_t stful_tbl_hdl, pipe_stful_mem_spec_t* stful_spec)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_table_reset_range(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stful_tbl_hdl_t stful_tbl_hdl, pipe_stful_mem_idx_t stful_ent_idx, uint32_t num_indices, pipe_stful_mem_spec_t* stful_spec)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_fifo_occupancy(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stful_tbl_hdl_t stful_tbl_hdl, int* occupancy)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_fifo_reset(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stful_tbl_hdl_t stful_tbl_hdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_fifo_dequeue(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stful_tbl_hdl_t stful_tbl_hdl, int num_to_dequeue, pipe_stful_mem_spec_t* values, int* num_dequeued)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_fifo_enqueue(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stful_tbl_hdl_t stful_tbl_hdl, int num_to_enqueue, pipe_stful_mem_spec_t* values)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_stful_ent_query_range(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_stful_tbl_hdl_t stful_tbl_hdl, pipe_stful_mem_idx_t stful_ent_idx, uint32_t num_indices_to_read, pipe_stful_mem_query_t* stful_query, uint32_t* num_indices_read, uint32_t pipe_api_flags)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

bf_dev_pipe_t dev_port_to_pipe_id(uint16_t dev_port_id)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return 0;
}

pipe_status_t pipe_mgr_get_next_group_members(pipe_sess_hdl_t sess_hdl, pipe_tbl_hdl_t tbl_hdl, bf_dev_id_t dev_id, pipe_sel_grp_hdl_t sel_grp_hdl, pipe_adt_ent_hdl_t mbr_hdl, int n, pipe_adt_ent_hdl_t* next_mbr_hdls)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_word_llp_active_member_count(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_sel_tbl_hdl_t tbl_hdl, uint32_t word_index, uint32_t* count)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_word_llp_active_members(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_sel_tbl_hdl_t tbl_hdl, uint32_t word_index, uint32_t count, pipe_adt_ent_hdl_t* mbr_hdls)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_entry_count(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t tbl_hdl, bool read_from_hw, uint32_t* count)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tbl_set_property(pipe_sess_hdl_t sess_hdl, bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t tbl_hdl, pipe_mgr_tbl_prop_type_t property, pipe_mgr_tbl_prop_value_t value, pipe_mgr_tbl_prop_args_t args)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tbl_get_property(pipe_sess_hdl_t sess_hdl, bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t tbl_hdl, pipe_mgr_tbl_prop_type_t property, pipe_mgr_tbl_prop_value_t* value, pipe_mgr_tbl_prop_args_t* args)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_sel_grp_mbr_state_get(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, pipe_sel_tbl_hdl_t sel_tbl_hdl, pipe_sel_grp_hdl_t sel_grp_hdl, pipe_adt_ent_hdl_t adt_ent_hdl, enum pipe_mgr_grp_mbr_state_e* mbr_state_p)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_set_adt_ent_hdl_in_mat_data(void* data, pipe_adt_ent_hdl_t adt_ent_hdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_set_sel_grp_hdl_in_mat_data(void* data, pipe_adt_ent_hdl_t sel_grp_hdl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_set_ttl_in_mat_data(void* data, uint32_t ttl)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tcam_scrub_timer_set(bf_dev_id_t dev, uint32_t msec_timer)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

uint32_t pipe_mgr_tcam_scrub_timer_get(bf_dev_id_t dev)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return 0;
}

pipe_status_t pipe_mgr_pipe_id_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, bf_dev_pipe_t* pipe_id)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_enable_callbacks_for_hitless_ha(pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_mat_ha_reconciliation_report_get(pipe_sess_hdl_t sess_hdl, dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, pipe_tbl_ha_reconc_report_t* ha_report)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return PIPE_SUCCESS;
}

bf_status_t pipe_mgr_tbl_hdl_pipe_mask_get(bf_dev_id_t dev_id, const char* prog_name, const char *pipeline_name, uint32_t *pipe_mask)
{
    LOG_TRACE("STUB:%s\n",__func__);
    return BF_SUCCESS;
}

pipe_status_t pipe_mgr_get_num_pipelines(bf_dev_id_t dev_id, uint32_t* num_pipes)
{
    LOG_TRACE("STUB:%s Setting number of pipelines to 1\n",__func__);
    *num_pipes = 1;
    return PIPE_SUCCESS;
}

int pipe_mgr_tbl_hdl_set_pipe(bf_dev_id_t dev_id,
		profile_id_t profile_id,
		pipe_tbl_hdl_t handle,
		pipe_tbl_hdl_t *ret_handle) {
	/* TODO: Check if this function is needed. */
	*ret_handle = handle;
	return 0;
}

pipe_status_t bf_tbl_dbg_counter_type_set(dev_target_t dev_tgt, char* tbl_name, bf_tbl_dbg_counter_type_t type)
{
	LOG_TRACE("STUB:%s\n",__func__);
	return PIPE_SUCCESS;
}


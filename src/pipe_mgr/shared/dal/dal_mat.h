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
#include "pipe_mgr/shared/pipe_mgr_infra.h"
#include "pipe_mgr/shared/pipe_mgr_mat.h"
#include "../infra/pipe_mgr_int.h"
#include "../infra/pipe_mgr_ctx_util.h"
#include "../pipe_mgr_shared_intf.h"

/* Threshold of table size above which SDE doesn't store entries */
#define PIPE_MGR_STORE_ENTRIES_THRESHOLD 10000

/**
 * Match-action table entry add DAL layer API.
 *
 * @param  sess_hdl              Session handle.
 * @param  dev_tgt               Target device.
 * @param  mat_tbl_hdl 		 Table handle.
 * @param  match_spec		 Pointer to match spec.
 * @param  act_fn_hdl		 Action function handle.
 * @param  act_data_spec	 pointer to action data spec.
 * @param  mat_ctx	 	 Pointer to table context information.
 * @param  tbl_ent_hdl	 	 Table entry handle.
 * @param  dal_data 	 	 Pointer to Dal layer data.
 * @return                       Status of the API call
 */
int dal_table_ent_add(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 mat_tbl_hdl,
		struct pipe_tbl_match_spec *match_spec,
		u32 act_fn_hdl,
		struct pipe_action_spec *act_data_spec,
		u32 ttl,
		u32 pipe_api_flags,
		struct pipe_mgr_mat_ctx *mat_ctx,
		void **dal_data);

/**
 * Match-action table entry delete DAL layer API.
 *
 * @param  sess_hdl              Session handle.
 * @param  dev_tgt               Target device.
 * @param  mat_tbl_hdl 		 Table handle.
 * @param  match_spec		 Pointer to match spec.
 * @param  mat_ctx	 	 Pointer to table context information.
 * @param  tbl_ent_hdl	 	 Table entry handle.
 * @param  dal_data 	 	 Pointer to Dal layer data.
 * @return                       Status of the API call
 */
int dal_table_ent_del_by_match_spec(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 mat_tbl_hdl,
		struct pipe_tbl_match_spec *match_spec,
		u32 pipe_api_flags,
		struct pipe_mgr_mat_ctx *mat_ctx,
		void *dal_data);

/**
 * DAL data unpack API.
 *
 * @param  dal_data              Pointer to Dal layer data.
 * @return                       Status of the API call
 */
int dal_unpack_dal_data(void *dal_data);

/**
 * DAL data delete API.
 *
 * @param  dal_data              Pointer to Dal layer data.
 */
void dal_delete_table_entry_data(void *dal_data);

/**
 * Action table entry ADD DAL layer API.
 *
 * @param  sess_hdl              Session handle.
 * @param  dev_tgt               Target device.
 * @param  adt_tbl_hdl 		 Action Table handle.
 * @param  act_fn_hdl 		 Action function handle.
 * @param  mat_ctx	 	 Pointer to table context information.
 * @param  tbl_ent_hdl	 	 Table entry handle.
 * @param  dal_data 	 	 Pointer to Dal layer data.
 * @return                       Status of the API call
 */
int dal_table_adt_ent_add(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 adt_tbl_hdl,
		u32 act_fn_hdl,
		struct pipe_action_spec *action_spec,
		u32 pipe_api_flags,
		struct pipe_mgr_mat_ctx *mat_ctx,
		u32 tbl_ent_hdl,
		void **dal_data);

/**
 * Action table entry DELETE DAL layer API.
 *
 * @param  sess_hdl              Session handle.
 * @param  dev_tgt               Target device.
 * @param  adt_tbl_hdl 		 Action Table handle.
 * @param  mat_ctx	 	 Pointer to table context information.
 * @param  tbl_ent_hdl	 	 Table entry handle.
 * @param  dal_data 	 	 Pointer to Dal layer data.
 * @return                       Status of the API call
 */
int dal_table_adt_ent_del(u32 sess_hdl,
	 struct bf_dev_target_t dev_tgt,
	 u32 adt_tbl_hdl,
	 u32 pipe_api_flags,
	 struct pipe_mgr_mat_ctx *mat_ctx,
	 u32 tbl_ent_hdl,
	 void **dal_data);

/**
 * DAL layer action table data delete.
 *
 * @param  dal_adt_data         Pointer to Dal layer data.
 */
void dal_delete_adt_table_entry_data(void *dal_adt_data);

/**
 * Selector table group entry ADD/Delete DAL layer API.
 *
 * @param  sess_hdl              Session handle.
 * @param  dev_tgt               Target device.
 * @param  sel_tbl_hdl 		 Selector Table handle.
 * @param  mat_ctx	 	 Pointer to table context information.
 * @param  tbl_ent_hdl	 	 Table entry handle.
 * @param  del_grp 	 	 delete group bool flag used to delete entry.
 * @return                       Status of the API call
 */
int dal_table_sel_ent_add_del
	(u32 sess_hdl,
	 struct bf_dev_target_t dev_tgt,
	 u32 sel_tbl_hdl,
	 u32 pipe_api_flags,
	 struct pipe_mgr_mat_ctx *mat_ctx,
	 u32 *tbl_ent_hdl,
	 bool del_grp);

/**
 * Selector table member add/delete to group DAL layer API.
 *
 * @param  sess_hdl              Session handle.
 * @param  dev_tgt               Target device.
 * @param  sel_tbl_hdl 		 selector table handle.
 * @param  sel_grp_hdl 		 selector group handle.
 * @param  num_mbrs 		 Number of members.
 * @param  mbrs 		 Poiter to array of member handles.
 * @param  mat_ctx	 	 Pointer to table context information.
 * @param  delete_member 	 delete member bool flag used to delete entry.
 * @return                       Status of the API call
 */
int dal_table_sel_member_add_del
	(u32 sess_hdl,
	 struct bf_dev_target_t dev_tgt,
	 u32 sel_tbl_hdl,
	 u32 sel_grp_hdl,
	 uint32_t num_mbrs,
	 u32 *mbrs,
	 u32 pipe_api_flags,
	 struct pipe_mgr_mat_ctx *mat_ctx,
	 bool delete_member);

/**
 * Specifies if rule entries should be stored for a MatchAction table.
 *
 * @return	true/false to specify storing entries.
 */
bool dal_mat_store_entries(struct pipe_mgr_mat_ctx *mat_ctx);

/**
 * Get entry from the target for MatchAction table.
 *
 * @param  sess_hdl              Session handle.
 * @param  dev_tgt               Target device.
 * @param  mat_tbl_hdl 		 Table handle.
 * @param  match_spec		 Pointer to match spec to be returned.
 * @param  act_data_spec	 Pointer to action data spec to be returned.
 * @param  act_fn_hdl		 Pointer to action function handle to be returned.
 * @param  mat_ctx	 	 Pointer to table context information.
 * @return                       Status of the API call
 */
int dal_mat_get_first_entry(u32 sess_hdl,
			    struct bf_dev_target_t dev_tgt,
			    u32 mat_tbl_hdl,
			    struct pipe_tbl_match_spec *match_spec,
			    struct pipe_action_spec *act_data_spec,
			    u32 *act_fn_hdl,
		            struct pipe_mgr_mat_ctx *mat_ctx);


/**
 * Get Next n entries from the target for MatchAction table.
 *
 * @param  sess_hdl              Session handle.
 * @param  dev_tgt               Target device.
 * @param  mat_tbl_hdl 		 Table handle.
 * @param  cur_match_spec	 Match Spec to start retrieving next entries.
 * @param  n			 Number of entries expected.
 * @param  match_specs		 Pointer to match specs array to be returned.
 * @param  act_specs		 Pointer to action data spec array of pointers.
 *				 Action specs are to be returned.
 * @param  act_fn_hdl		 Pointer to action function handles array to be returned.
 * @param  num			 Number of entried returned.
 * @param  mat_ctx	 	 Pointer to table context information.
 * @return                       Status of the API call
 */
int dal_mat_get_next_n_by_key(u32 sess_hdl,
			       struct bf_dev_target_t dev_tgt,
			       u32 mat_tbl_hdl,
			       struct pipe_tbl_match_spec *cur_match_spec,
			       int n,
			       struct pipe_tbl_match_spec *match_specs,
			       struct pipe_action_spec **act_specs,
			       u32 *act_fn_hdls,
			       u32 *num,
		               struct pipe_mgr_mat_ctx *mat_ctx);

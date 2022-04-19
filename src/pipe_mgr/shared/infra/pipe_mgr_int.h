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

/*!
 * @file pipe_mgr_int.h
 * @date
 *
 * Internal definitions for pipe_mgr.
 */
#ifndef __PIPE_MGR_INT_H__
#define __PIPE_MGR_INT_H__

#include <osdep/p4_sde_osdep_utils.h>
#include <bf_types/bf_types.h>
#include <dvm/bf_drv_profile.h>
#include <port_mgr/bf_port_if.h>

/* TODO: Remove this #define and use pipe_id provided in the respective
 * function arguments.
 */
#define PIPE_MGR_DEFAULT_PIPE_ID 0

/* TODO: Revisit Linked list usage.
 * Should we use macro for P4 OSDEP instead?
 */
#define PIPE_MGR_FREE_LIST(head) \
	{ \
		void *temp;\
		while (head) { \
			temp = head->next; \
			P4_SDE_FREE(head); \
			head = temp; \
		} \
	} \

/* further match kind should go here */
enum pipe_mgr_match_type {
	PIPE_MGR_MATCH_TYPE_EXACT = 0,
	PIPE_MGR_MATCH_TYPE_TERNARY,
	PIPE_MGR_MATCH_TYPE_LPM,
	PIPE_MGR_MATCH_TYPE_SELECTOR,
	PIPE_MGR_MATCH_TYPE_INVALID
};

struct pipe_mgr_sess_ctx {
	/* To serialize operations within a session and
	 * protect this structure.
	 */
	p4_sde_mutex lock;

	bool in_use;

	/* TODO: Add transactions/batching related members. */

};

/* Global context for pipe_mgr service. It is protected by
 * global pipe_mgr_lock.
 */
struct pipe_mgr_ctx {
	/* Sessions in Pipe Mgr. */
	struct pipe_mgr_sess_ctx sessions[P4_SDE_MAX_SESSIONS];

	/* Maps a device id to a struct pipe_mgr_dev. pipe_mgr_dev
	 * contains information about the device.
	 */
	p4_sde_map dev_map;
};

struct pipe_mgr_p4_parameters {
	char name[P4_SDE_NAME_LEN];
	uint32_t start_bit;
	uint32_t position;
	uint32_t bit_width;
	struct pipe_mgr_p4_parameters *next;
};

struct pipe_mgr_actions_list {
	char name[P4_SDE_NAME_LEN];
	uint32_t handle;
	bool allowed_as_hit_action;
	bool allowed_as_default_action;
	bool is_compiler_added_action;
	bool constant_default_action;
	uint32_t p4_parameters_count;
	struct pipe_mgr_p4_parameters *p4_parameters_list;
	struct pipe_mgr_actions_list *next;
};

struct pipe_mgr_match_key_fields {
	char name[P4_SDE_NAME_LEN];
	uint32_t start_bit;
	uint32_t bit_width;
	int bit_width_full;
	uint32_t position;
	enum pipe_mgr_match_type match_type;
	bool is_valid;
	char instance_name[P4_SDE_NAME_LEN];
	char field_name[P4_SDE_NAME_LEN];
	struct pipe_mgr_match_key_fields *next;
};

struct pipe_mgr_match_attribute {

	int stage_table_count;
	void *stage_table;
	enum pipe_mgr_match_type match_type;
	/*  TODO: Applicability of this has to be revisited. */
};

struct pipe_mgr_mat_entry_info {
	/* Entry handle */
	u32 mat_ent_hdl;
	u32 act_fn_hdl;
	struct pipe_tbl_match_spec *match_spec;
	struct pipe_action_spec *act_data_spec;
	void *dal_data;
};

struct pipe_mgr_mat_key_htbl_node {
	u32 mat_ent_hdl;
	struct pipe_tbl_match_spec *match_spec;
};

struct action_data_table_refs {
	uint32_t handle;
	char name[P4_SDE_TABLE_NAME_LEN];
	char how_referenced[P4_SDE_TABLE_NAME_LEN];
	struct action_data_table_refs *next;
};

struct selection_table_refs {
	uint32_t handle;
	char name[P4_SDE_TABLE_NAME_LEN];
	char how_referenced[P4_SDE_TABLE_NAME_LEN];
	struct selection_table_refs *next;
};


struct pipe_mgr_adt_entry_info {
	/* Entry handle */
	u32 adt_ent_hdl;
	u32 act_fn_hdl;
	struct pipe_action_spec *act_data_spec;
	void *dal_data;
};

struct pipe_mgr_sel_entry_info {
	u32 sel_tbl_hdl;
	u32 sel_grp_hdl;
	u32 num_mbrs;
	u32 *mbrs;
};

struct pipe_mgr_mat_ctx {
	char direction[P4_SDE_NAME_LEN];
	uint32_t handle;
	char name[P4_SDE_TABLE_NAME_LEN];
	int size;
	int default_action_handle;
	/*  TODO: Applicability of this has to be revisited. */
	bool is_resource_controllable;
	/*  TODO: Applicability of this has to be revisited. */
	char uses_range;
	int mat_key_fields_count;
	struct pipe_mgr_match_key_fields *mat_key_fields;
	int action_count;
	struct pipe_mgr_actions_list *actions;
	struct pipe_mgr_match_attribute match_attr;
	/* Specifies if P4 table entries state should be stored. */
	bool store_entries;
	bool duplicate_entry_check;
	int adt_count;
	struct action_data_table_refs *adt;
	int sel_tbl_count;
	struct selection_table_refs *sel_tbl;
	int stage_table_count;
	void *stage_table;
};

struct pipe_mgr_mat_state {
	/* Mutex to protect all members of this structure. */
	p4_sde_mutex lock;

	/* entry handle array */
#define ENTRY_HANDLE_ARRAY_SIZE 0xFFFFFFFF
	p4_sde_id *entry_handle_array;
	p4_sde_map entry_info_htbl;

	/* hashtable keyed by match-spec, used for duplicate entry
	 * detection and access by match spec.
	 */
	bf_hashtable_t **key_htbl;
	/* Number of hash tables. This is same as number of pipelines.
	 * For each pipeline, a separate hash table is maintained.
	 */
	int num_htbls;
};

struct pipe_mgr_mat {
	/* Context json information of the table. */
	struct pipe_mgr_mat_ctx ctx;

	/* Run-time table state. State is maintained if
	 * ctx.store_state is true in the context json.
	 */
	struct pipe_mgr_mat_state *state;

	struct pipe_mgr_mat *next;
};

/* Contains pipeline global configs and hook for target specific
 * global configs.
 */

struct pipe_mgr_global_config {
	/* hook for target specific configs */
	void *dal_global_config;
};


/* Contains information about P4 pipeline. Stores information from
 * the context json and run-time rule entries (if rule entry storage
 * is enabled).
 */
struct pipe_mgr_p4_pipeline {
	/* Number of tables as per context json. */
	int num_mat_tables;

	/* Array containing Match-Action Tables information for this P4
	 * pipeline.
	 */
	struct pipe_mgr_mat *mat_tables;


};

/* P4 Program Pipeline Profile  */
struct pipe_mgr_profile {
	/* Read-write lock to protect below members of this structure. */
	p4_sde_rwlock lock;

	int profile_id;
	int core_id;

	char prog_name[P4_SDE_PROG_NAME_LEN];
	char pipeline_name[P4_SDE_PROG_NAME_LEN];
	char cfg_file[PIPE_MGR_CFG_FILE_LEN];

	/* Stores P4 processing pipeline static and run-time information. */
	struct pipe_mgr_p4_pipeline pipe_ctx;

	int compiler_version[P4_SDE_VERSION_LEN];

	/* Schema version [major, minor, maintenance] */
	int schema_version[P4_SDE_VERSION_LEN];

	/* Currently mod_addr action has 24 bit to specify */
};

struct pipe_mgr_dev {
	/* Unique identifier. */
	int dev_id;

	/* Device family. */
	enum bf_dev_family_t dev_family;

	/* No of profiles. */
	uint32_t num_pipeline_profiles;

	/* Static and Run-time information of P4 pipeline profile. */
	struct pipe_mgr_profile *profiles;
	/* store global device configs */
	struct pipe_mgr_global_config global_cfg;
};

int pipe_mgr_set_dev(struct pipe_mgr_dev **dev,
		     int dev_id,
		     struct bf_device_profile *profile);
struct pipe_mgr_dev *pipe_mgr_get_dev(int dev_id);
int pipe_mgr_api_prologue(u32 sess_hdl, struct bf_dev_target_t dev_tgt);
void pipe_mgr_api_epilogue(u32 sess_hdl, struct bf_dev_target_t dev_tgt);
void pipe_mgr_delete_act_data_spec(struct pipe_action_spec *ads);
int pipe_mgr_mat_pack_act_spec(
		struct pipe_action_spec **act_data_spec_out,
		struct pipe_action_spec *ads);
int pipe_mgr_mat_unpack_act_spec(
		struct pipe_action_spec *ads,
		struct pipe_action_spec *act_data_spec);
struct pipe_mgr_ctx *get_pipe_mgr_ctx();

#endif /* __PIPE_MGR_INT_H__ */

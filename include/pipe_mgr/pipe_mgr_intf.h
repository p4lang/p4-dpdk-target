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
 * @file pipe_mgr_intf.h
 * @date
 *
 * Definitions for interfaces to pipeline manager
 */

#ifndef _PIPE_MGR_INTF_H
#define _PIPE_MGR_INTF_H

/* Standard includes */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Module header files */
#include <bf_types/bf_types.h>
#include <pipe_mgr/pipe_mgr_err.h>
#include <pipe_mgr/shared/pipe_mgr_mat.h>

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum pipe_hdl_type {
  /* API session handles */
  PIPE_HDL_TYPE_SESSION = 0x00,

  /* Handles for logical table objects */
  PIPE_HDL_TYPE_MAT_TBL = 0x01,   /* Match action table */
  PIPE_HDL_TYPE_ADT_TBL = 0x02,   /* Action data table */
  PIPE_HDL_TYPE_SEL_TBL = 0x03,   /* Selection table */
  PIPE_HDL_TYPE_STAT_TBL = 0x04,  /* Statistics table */
  PIPE_HDL_TYPE_METER_TBL = 0x05, /* Meter, LPF and WRED table */
  PIPE_HDL_TYPE_STFUL_TBL = 0x06, /* Stateful table */
  PIPE_HDL_TYPE_COND_TBL = 0x07,  /* Gateway table */

  /* Handles for logical table entry objects */
  PIPE_HDL_TYPE_MAT_ENT = 0x10,  /* Match action table entry */
  PIPE_HDL_TYPE_ADT_ENT = 0x11,  /* Action data table entry */
  PIPE_HDL_TYPE_SEL_GRP = 0x12,  /* Selection table group entry */
  PIPE_HDL_TYPE_STAT_ENT = 0x13, /* Statistics table entry */
  PIPE_HDL_TYPE_MET_ENT = 0x14,  /* Meter table entry */
  PIPE_HDL_TYPE_SFUL_ENT = 0x15, /* Stateful memory table entry */

  /* Handles for other P4 level objects */
  PIPE_HDL_TYPE_ACTION_FN = 0x20, /* P4 action function */
  PIPE_HDL_TYPE_FIELD_LST = 0x21, /* P4 field list */
  PIPE_HDL_TYPE_CALC_ALGO = 0x22, /* P4 calculation algorithm */

  /* Handle for Poor-mans selection table */
  PIPE_HDL_TYPE_POOR_MANS_SEL_TBL = 0x23,

  /* Reserve one type for invalid type */
  PIPE_HDL_TYPE_INVALID = 0x3F /* Reserved to indicate invalid handle */
} pipe_hdl_type_t;

#define PIPE_SET_HDL_TYPE(hdl, type) (((type) << 24) | (hdl))
#define PIPE_GET_HDL_TYPE(hdl) (((hdl) >> 24) & 0x3F)
#define PIPE_SET_HDL_PIPE(hdl, pipe) (((pipe) << 30) | (hdl))
#define PIPE_GET_HDL_PIPE(hdl) (((hdl) >> 30) & 0x3)
#define PIPE_GET_HDL_VAL(hdl) ((hdl)&0x00FFFFFF)

typedef enum pipe_mgr_tbl_prop_type_ {
  PIPE_MGR_TABLE_PROP_NONE = 0,
  PIPE_MGR_TABLE_ENTRY_SCOPE,
  PIPE_MGR_DUPLICATE_ENTRY_CHECK,
  PIPE_MGR_IDLETIME_REPEATED_NOTIFICATION,
} pipe_mgr_tbl_prop_type_t;

typedef enum pipe_mgr_tbl_prop_scope_value {
  PIPE_MGR_ENTRY_SCOPE_ALL_PIPELINES = 0,
  PIPE_MGR_ENTRY_SCOPE_SINGLE_PIPELINE = 1,
  PIPE_MGR_ENTRY_SCOPE_USER_DEFINED = 2,
} pipe_mgr_tbl_prop_scope_value_t;

typedef enum pipe_mgr_tbl_prop_duplicate_entry_check_value {
  PIPE_MGR_DUPLICATE_ENTRY_CHECK_DISABLE = 0,
  PIPE_MGR_DUPLICATE_ENTRY_CHECK_ENABLE = 1,
} pipe_mgr_tbl_prop_duplicate_entry_check_value_t;

typedef enum pipe_mgr_tbl_prop_idletime_repeated_notification_enable_value {
  PIPE_MGR_IDLETIME_REPEATED_NOTIFICATION_DISABLE = 0,
  PIPE_MGR_IDLETIME_REPEATED_NOTIFICATION_ENABLE = 1,
} pipe_mgr_tbl_prop_idletime_repeated_notification_enable_value_t;

typedef union pipe_mgr_tbl_prop_value {
  uint32_t value;
  pipe_mgr_tbl_prop_scope_value_t scope;
  pipe_mgr_tbl_prop_duplicate_entry_check_value_t duplicate_check;
  pipe_mgr_tbl_prop_idletime_repeated_notification_enable_value_t
      repeated_notify;
} pipe_mgr_tbl_prop_value_t;

#define PIPE_MGR_MAX_USER_DEFINED_SCOPES 4
typedef uint8_t scope_pipes_t;
typedef union pipe_mgr_tbl_prop_args {
  uint32_t value;
  scope_pipes_t user_defined_entry_scope[PIPE_MGR_MAX_USER_DEFINED_SCOPES];
} pipe_mgr_tbl_prop_args_t;

// Wildcard when you want to specify all parsers
// keep in sync with PD_DEV_PIPE_PARSER_ALL
#define PIPE_MGR_PVS_PARSER_ALL 0xff

#define PIPE_DIR_MAX 2

/*!
 * Typedefs for pipeline object handles
 */
typedef uint32_t pipe_sess_hdl_t;

typedef uint32_t pipe_tbl_hdl_t;
typedef pipe_tbl_hdl_t pipe_mat_tbl_hdl_t;
typedef pipe_tbl_hdl_t pipe_adt_tbl_hdl_t;
typedef pipe_tbl_hdl_t pipe_sel_tbl_hdl_t;
typedef pipe_tbl_hdl_t pipe_stat_tbl_hdl_t;
typedef pipe_tbl_hdl_t pipe_meter_tbl_hdl_t;
typedef pipe_tbl_hdl_t pipe_lpf_tbl_hdl_t;
typedef pipe_tbl_hdl_t pipe_wred_tbl_hdl_t;
typedef pipe_tbl_hdl_t pipe_stful_tbl_hdl_t;
typedef pipe_tbl_hdl_t pipe_ind_res_hdl_t;
typedef pipe_tbl_hdl_t pipe_res_hdl_t;
typedef pipe_tbl_hdl_t pipe_prsr_instance_hdl_t;

typedef uint32_t pipe_ent_hdl_t;
typedef pipe_ent_hdl_t pipe_mat_ent_hdl_t;
typedef pipe_ent_hdl_t pipe_adt_ent_hdl_t;
typedef pipe_ent_hdl_t pipe_sel_grp_hdl_t;
typedef pipe_ent_hdl_t pipe_stat_ent_idx_t;
typedef pipe_ent_hdl_t pipe_meter_idx_t;
typedef pipe_ent_hdl_t pipe_stful_mem_idx_t;
typedef pipe_ent_hdl_t pipe_ind_res_idx_t;
typedef pipe_ent_hdl_t pipe_res_idx_t;

typedef uint32_t pipe_act_fn_hdl_t;
typedef uint32_t pipe_fld_lst_hdl_t;
typedef uint32_t pipe_reg_param_hdl_t;
typedef uint8_t dev_stage_t;
typedef int profile_id_t;
typedef uint32_t pipe_idx_t;
typedef uint8_t pipe_parser_id_t;

typedef void (*pipe_mgr_stat_ent_sync_cback_fn)(bf_dev_id_t device_id,
                                                void *cookie);
typedef void (*pipe_mgr_stat_tbl_sync_cback_fn)(bf_dev_id_t device_id,
                                                void *cookie);
typedef void (*pipe_stful_ent_sync_cback_fn)(bf_dev_id_t device_id,
                                             void *cookie);
typedef void (*pipe_stful_tbl_sync_cback_fn)(bf_dev_id_t device_id,
                                             void *cookie);
/*!
 * Flags that can be used in conjunction with API calls
 */
/*! Flag to make hardware synchronous API requests */
#define PIPE_FLAG_SYNC_REQ (1 << 0)

/* Not to be used */
#define PIPE_FLAG_INTERNAL (1 << 1)

/*!
 * Data types used with this library
 */

/*! Definitions used to identify the target of an API request */
#define DEV_PIPE_ALL BF_DEV_PIPE_ALL
typedef bf_dev_target_t dev_target_t;

typedef struct pipe_tbl_ha_reconc_report {
  /*!< Number of entries that were added after delta compute */
  uint32_t num_entries_added;
  /*!< Number of entries that were deleted after delta compute */
  uint32_t num_entries_deleted;
  /*!< Number of entries that were modified after delta compute */
  uint32_t num_entries_modified;
} pipe_tbl_ha_reconc_report_t;


/*! Flow learn notification entry format. This is interpreted
 * in terms of P4 fields by entry decoder routines
 */

/*! Flow learn notification message format
 */
typedef struct pipe_lrn_digest_msg {
  dev_target_t dev_tgt;
  /*< Device that is the originator of the learn notification */
  uint16_t num_entries;
  /*< number of learn notifications in this message */
  void *entries;
  /*< array of <num_entries> of
    pd_<program_name>_<lrn_digest_field_list_name>_digest_entry_t;*/
  pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl;
} pipe_flow_lrn_msg_t;

/* Prototype for flow learn notification handler */
typedef pipe_status_t (*pipe_flow_lrn_notify_cb)(
    pipe_sess_hdl_t sess_hdl,
    pipe_flow_lrn_msg_t *pipe_flow_lrn_msg,
    void *callback_fn_cookie);

/*! Idle Timers
 */
/*!
 * Enum for Idle timer hit state
 */
typedef enum pipe_idle_time_hit_state_ {
  ENTRY_IDLE,
  ENTRY_ACTIVE
} pipe_idle_time_hit_state_e;

/* Prototype for idle timer expiry notification handler */
typedef void (*pipe_idle_tmo_expiry_cb)(bf_dev_id_t dev_id,
                                        pipe_mat_ent_hdl_t mat_ent_hdl,
                                        void *client_data);

/* Second Prototype for idle timer expiry notification handler */
typedef void (*pipe_idle_tmo_expiry_cb_with_match_spec_copy)(
    bf_dev_id_t dev_id,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_tbl_match_spec_t *match_spec,
    void *client_data);

/* Prototype for idle time update complete callback handler */
typedef void (*pipe_idle_tmo_update_complete_cb)(bf_dev_id_t dev_id,
                                                 void *cb_data);

typedef enum pipe_idle_time_mode_ {
  POLL_MODE = 0,
  NOTIFY_MODE = 1,
  INVALID_MODE = 2
} pipe_idle_time_mode_e;
static inline const char *idle_time_mode_to_str(pipe_idle_time_mode_e mode) {
  switch (mode) {
    case POLL_MODE:
      return "poll";
    case NOTIFY_MODE:
      return "notify";
    case INVALID_MODE:
      return "disabled";
  }
  return "unknown";
}

typedef struct pipe_idle_time_params_ {
  /* Default mode is POLL_MODE */
  pipe_idle_time_mode_e mode;
  union {
    struct {
      uint32_t ttl_query_interval;
      /*< Minimum query interval with which the application will call
       *  pipe_mgr_idle_time_get_ttl to get the TTL of an entry.
       * If the API is called sooner than the query interval, then
       * the value received will be same
       */
      uint32_t max_ttl;
      /*< deprecated, will be ignored */
      uint32_t min_ttl;
      /*< deprecated, will be ignored */
      pipe_idle_tmo_expiry_cb callback_fn;
      pipe_idle_tmo_expiry_cb_with_match_spec_copy callback_fn2;
      /*< Callback function to call in case of notification mode */
      void *client_data;
      /*< Client data for the callback function */
      bool default_callback_choice;
      /*< 0 for callback_fn; 1 for callback_fn2 >*/
    } notify;
  } u;
} pipe_idle_time_params_t;

typedef struct pipe_stful_mem_query_t {
  int pipe_count; /* Number of valid indices in the "data" array */
  /* Use pipe_stful_direct_query_get_sizes to determine the size of the data
   * array to allocate.  Note that when using a pipe_res_get_data_t pipe_mgr
   * will allocate this, caller is responsible for freeing. */
  pipe_stful_mem_spec_t *data;
} pipe_stful_mem_query_t;

/* When set fetch match and action specs. */
#define PIPE_RES_GET_FLAG_ENTRY (1 << 0)
/* When set fetch stats. */
#define PIPE_RES_GET_FLAG_CNTR (1 << 1)
/* When set fetch the meter/lpf/wred spec. */
#define PIPE_RES_GET_FLAG_METER (1 << 2)
/* When set fetch the stateful spec. */
#define PIPE_RES_GET_FLAG_STFUL (1 << 3)
/* When set fetch the idle information (hit-state or TTL). */
#define PIPE_RES_GET_FLAG_IDLE (1 << 4)
/* When all flags set. */
#define PIPE_RES_GET_FLAG_ALL                         \
  (PIPE_RES_GET_FLAG_ENTRY | PIPE_RES_GET_FLAG_CNTR | \
   PIPE_RES_GET_FLAG_METER | PIPE_RES_GET_FLAG_IDLE | PIPE_RES_GET_FLAG_STFUL)
typedef struct pipe_res_get_data_t {
  pipe_stat_data_t counter;
  pipe_stful_mem_query_t stful;
  union {
    pipe_meter_spec_t meter;
    pipe_lpf_spec_t lpf;
    pipe_wred_spec_t red;
  } mtr;
  union {
    pipe_idle_time_hit_state_e hit_state;
    uint32_t ttl;
  } idle;
  /* The valid flags below will be set to indicate whether the above resource
   * specs are valid. */
  bool has_counter;
  bool has_stful;
  bool has_meter;
  bool has_lpf;
  bool has_red;
  bool has_ttl;
  bool has_hit_state;
} pipe_res_get_data_t;

/*!
 * Action data specification
 */
typedef struct pipe_action_data_spec pipe_action_data_spec_t;

/*!
 * Generalized action specification that encodes all types of action data refs
 */
typedef struct pipe_action_spec pipe_action_spec_t;

#define IS_ACTION_SPEC_ACT_DATA(act_spec) \
  ((act_spec)->pipe_action_datatype_bmap & PIPE_ACTION_DATA_TYPE)
#define IS_ACTION_SPEC_ACT_DATA_HDL(act_spec) \
  ((act_spec)->pipe_action_datatype_bmap & PIPE_ACTION_DATA_HDL_TYPE)
#define IS_ACTION_SPEC_SEL_GRP(act_spec) \
  ((act_spec)->pipe_action_datatype_bmap & PIPE_SEL_GRP_HDL_TYPE)

typedef enum bf_tbl_dbg_counter_type_e {
  BF_TBL_DBG_CNTR_DISABLED = 0,
  BF_TBL_DBG_CNTR_LOG_TBL_MISS = 1,
  BF_TBL_DBG_CNTR_LOG_TBL_HIT = 2,
  BF_TBL_DBG_CNTR_GW_TBL_MISS = 3,
  BF_TBL_DBG_CNTR_GW_TBL_HIT = 4,
  BF_TBL_DBG_CNTR_GW_TBL_INHIBIT = 5,
  BF_TBL_DBG_CNTR_MAX
} bf_tbl_dbg_counter_type_t;
static inline const char *bf_tbl_dbg_counter_type_to_str(
    bf_tbl_dbg_counter_type_t t) {
  switch (t) {
    case BF_TBL_DBG_CNTR_DISABLED:
      return "BF_TBL_DBG_CNTR_DISABLED";
    case BF_TBL_DBG_CNTR_LOG_TBL_MISS:
      return "BF_TBL_DBG_CNTR_LOG_TBL_MISS";
    case BF_TBL_DBG_CNTR_LOG_TBL_HIT:
      return "BF_TBL_DBG_CNTR_LOG_TBL_HIT";
    case BF_TBL_DBG_CNTR_GW_TBL_MISS:
      return "BF_TBL_DBG_CNTR_GW_TBL_MISS";
    case BF_TBL_DBG_CNTR_GW_TBL_HIT:
      return "BF_TBL_DBG_CNTR_GW_TBL_HIT";
    case BF_TBL_DBG_CNTR_GW_TBL_INHIBIT:
      return "BF_TBL_DBG_CNTR_GW_TBL_INHIBIT";
    case BF_TBL_DBG_CNTR_MAX:
      return "BF_TBL_DBG_CNTR_MAX";
  }
  return "Unknown";
}

#define BF_TBL_NAME_LEN 200
#define BF_MAX_LOG_TBLS 16
typedef enum {
  INPUT_FIELD_ATTR_TYPE_MASK = 0,
  INPUT_FIELD_ATTR_TYPE_VALUE = 1,
  INPUT_FIELD_ATTR_TYPE_STREAM = 2,
} pipe_input_field_attr_type_t;

typedef enum {
  INPUT_FIELD_EXCLUDED = 0,
  INPUT_FIELD_INCLUDED
} pipe_input_field_attr_value_mask_t;

typedef struct pipe_hash_calc_input_field_attribute {
  /* This indicates index in fl_list*/
  uint32_t input_field;
  /* Runtime slice start bit. This is different from
     P4 defined slice. This can be slice of a P4
     slice too. If P4 defined slice is
     src_ip[32:9], then slice_start_bit 2 will mean
     bit number 11 in src_ip */
  uint32_t slice_start_bit;
  /* Length of runtime slice as described above.
     Value of 0 means till end of field  */
  uint32_t slice_length;
  /* This would only be used by pipe_mgr to check for
     symmetric fields.
     0 here DOESN'T mean disable a field in pipe_mgr. To
     disable field, mask needs to be INPUT_FIELD_EXCLUDED.
     If 0 is present in order in pipe_mgr,
     it will be just treated as a normal field and
     symmetric hashing won't kick in. Otherwise, fields
     with same order are grouped as symmetric */
  uint32_t order;
  pipe_input_field_attr_type_t type;
  union {
    pipe_input_field_attr_value_mask_t mask;
    uint32_t val;
    uint8_t *stream;
  } value;
} pipe_hash_calc_input_field_attribute_t;

/******************************************************************************
 *
 * MAT Placement Callbacks
 *
 *****************************************************************************/
enum pipe_mat_update_type {
  PIPE_MAT_UPDATE_ADD,
  PIPE_MAT_UPDATE_ADD_MULTI, /* An add to multiple logical indices. */
  PIPE_MAT_UPDATE_SET_DFLT,  /* Set the table's default action. */
  PIPE_MAT_UPDATE_CLR_DFLT,  /* Clear the table's default action. */
  PIPE_MAT_UPDATE_DEL,
  PIPE_MAT_UPDATE_MOD,
  PIPE_MAT_UPDATE_MOV,
  PIPE_MAT_UPDATE_MOV_MULTI, /* A move involving mulitple logical indices. */
  PIPE_MAT_UPDATE_MOV_MOD,
  PIPE_MAT_UPDATE_MOV_MULTI_MOD,
  PIPE_MAT_UPDATE_ADD_IDLE
};
static inline const char *pipe_mgr_move_list_op_str(
    enum pipe_mat_update_type op) {
  switch (op) {
    case PIPE_MAT_UPDATE_ADD:
      return "ADD";
    case PIPE_MAT_UPDATE_ADD_MULTI:
      return "ADD_MULTI";
    case PIPE_MAT_UPDATE_SET_DFLT:
      return "SET_DFLT";
    case PIPE_MAT_UPDATE_CLR_DFLT:
      return "CLR_DFLT";
    case PIPE_MAT_UPDATE_DEL:
      return "DEL";
    case PIPE_MAT_UPDATE_MOD:
      return "MOD";
    case PIPE_MAT_UPDATE_MOV:
      return "MOV";
    case PIPE_MAT_UPDATE_MOV_MULTI:
      return "MOV_MULTI";
    case PIPE_MAT_UPDATE_MOV_MOD:
      return "MOV_MOD";
    case PIPE_MAT_UPDATE_MOV_MULTI_MOD:
      return "MOV_MULTI_MOD";
    case PIPE_MAT_UPDATE_ADD_IDLE:
      return "ADD_IDLE";
    default:
      return "UNKNOWN";
  }
}
struct pipe_mat_update_set_dflt_params {
  pipe_mat_ent_hdl_t ent_hdl;
  pipe_adt_ent_hdl_t action_profile_mbr;
  pipe_idx_t indirect_action_index;
  bool action_profile_mbr_exists;
  pipe_sel_grp_hdl_t sel_grp_hdl;
  pipe_idx_t indirect_selection_index;
  uint32_t num_selector_words;
  bool sel_grp_exists;
  void *data;
};
struct pipe_mat_update_clr_dflt_params {
  pipe_mat_ent_hdl_t ent_hdl;
};
struct pipe_mat_update_add_params {
  pipe_mat_ent_hdl_t ent_hdl;
  uint32_t priority; /* TCAM priority, only valid for TCAM tables. */
  pipe_idx_t logical_index;
  pipe_adt_ent_hdl_t action_profile_mbr;
  pipe_idx_t indirect_action_index;
  bool action_profile_mbr_exists;
  pipe_sel_grp_hdl_t sel_grp_hdl;
  pipe_idx_t indirect_selection_index;
  uint32_t num_selector_words;
  bool sel_grp_exists;
  void *data;
};
struct pipe_mat_update_del_params {
  pipe_mat_ent_hdl_t ent_hdl;
};
struct pipe_mat_update_mod_params {
  pipe_mat_ent_hdl_t ent_hdl;
  pipe_adt_ent_hdl_t action_profile_mbr;
  pipe_idx_t indirect_action_index;
  bool action_profile_mbr_exists;
  pipe_sel_grp_hdl_t sel_grp_hdl;
  pipe_idx_t indirect_selection_index;
  uint32_t num_selector_words;
  bool sel_grp_exists;
  void *data;
};
struct pipe_mat_update_mov_params {
  pipe_mat_ent_hdl_t ent_hdl;
  pipe_idx_t logical_index;
  pipe_adt_ent_hdl_t action_profile_mbr;
  pipe_idx_t indirect_action_index;
  bool action_profile_mbr_exists;
  pipe_sel_grp_hdl_t sel_grp_hdl;
  pipe_idx_t indirect_selection_index;
  uint32_t num_selector_words;
  bool sel_grp_exists;
  void *data;
};
struct pipe_multi_index {
  /* Base logical index. */
  pipe_idx_t logical_index_base;
  /* Count of consecutive indexes (a minimum of one). */
  uint8_t logical_index_count;
};
/* Represents an add to multiple logical indices.  There are
 * logical_index_array_length number of base indices, specified in
 * logical_index_base_array.  Associated with each base index is a count,
 * specified in logical_index_count_array, saying how many consecutive indices
 * are used.  For example, given:
 *   logical_index_array_length = 3
 *   location_array = [ [250,2], [200,1], [300,6] ]
 * The following nine logical indices would be used: 250-251, 200, and 300-305.
 */
struct pipe_mat_update_add_multi_params {
  pipe_mat_ent_hdl_t ent_hdl;
  uint32_t priority; /* TCAM priority, only valid for TCAM tables. */
  pipe_adt_ent_hdl_t action_profile_mbr;
  pipe_idx_t indirect_action_index;
  bool action_profile_mbr_exists;
  pipe_sel_grp_hdl_t sel_grp_hdl;
  pipe_idx_t indirect_selection_index;
  uint32_t num_selector_words;
  bool sel_grp_exists;
  int logical_index_array_length;
  struct pipe_multi_index *location_array;
  void *data;
};
/* Represents a move of an entry occupying multiple logical indices.  Similar
 * to struct pipe_mat_update_add_multi_params, logical_index_array_length
 * specifies how many sets of logical indices are moving.  The location_array
 * provides the new logical indexes of the entry specified as a series of base
 * and count pairs representing count number of logical indexes starting at and
 * includeing the base.  For example, given:
 *   logical_index_array_length = 2
 *   location_array = [[50,3], [100,1]]
 * The entry now occupies logical indices 50-52 and 100. */
struct pipe_mat_update_mov_multi_params {
  pipe_mat_ent_hdl_t ent_hdl;
  pipe_adt_ent_hdl_t action_profile_mbr;
  pipe_idx_t indirect_action_index;
  bool action_profile_mbr_exists;
  pipe_sel_grp_hdl_t sel_grp_hdl;
  pipe_idx_t indirect_selection_index;
  uint32_t num_selector_words;
  bool sel_grp_exists;
  int logical_index_array_length;
  struct pipe_multi_index *location_array;
  void *data;
};
/* A union representing all the possible parameters to a MAT update. */
union pipe_mat_update_params {
  struct pipe_mat_update_set_dflt_params set_dflt;
  struct pipe_mat_update_clr_dflt_params clr_dflt;
  struct pipe_mat_update_add_params add;
  struct pipe_mat_update_del_params del;
  struct pipe_mat_update_mod_params mod;
  struct pipe_mat_update_mov_params mov;
  struct pipe_mat_update_add_multi_params add_multi;
  struct pipe_mat_update_mov_multi_params mov_multi;
};
typedef void (*pipe_mat_update_cb)(bf_dev_target_t dev_tgt,
                                   pipe_tbl_hdl_t tbl_hdl,
                                   enum pipe_mat_update_type update_type,
                                   union pipe_mat_update_params *update_params,
                                   void *cookie);
bf_status_t pipe_register_mat_update_cb(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_mat_tbl_hdl_t tbl_hdl,
                                        pipe_mat_update_cb cb,
                                        void *cb_cookie);

/******************************************************************************
 *
 * ADT Placement Callbacks
 *
 *****************************************************************************/
enum pipe_adt_update_type {
  PIPE_ADT_UPDATE_ADD,
  PIPE_ADT_UPDATE_DEL,
  PIPE_ADT_UPDATE_MOD
};
struct pipe_adt_update_add_params {
  pipe_adt_ent_hdl_t ent_hdl;
  void *data;
};
struct pipe_adt_update_del_params {
  pipe_adt_ent_hdl_t ent_hdl;
};
struct pipe_adt_update_mod_params {
  pipe_adt_ent_hdl_t ent_hdl;
  void *data;
};
union pipe_adt_update_params {
  struct pipe_adt_update_add_params add;
  struct pipe_adt_update_del_params del;
  struct pipe_adt_update_mod_params mod;
};
typedef void (*pipe_adt_update_cb)(bf_dev_target_t dev_tgt,
                                   pipe_tbl_hdl_t tbl_hdl,
                                   enum pipe_adt_update_type update_type,
                                   union pipe_adt_update_params *update_params,
                                   void *cookie);
bf_status_t pipe_register_adt_update_cb(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_adt_tbl_hdl_t tbl_hdl,
                                        pipe_adt_update_cb cb,
                                        void *cb_cookie);

/******************************************************************************
 *
 * SEL Placement Callbacks
 *
 *****************************************************************************/
enum pipe_sel_update_type {
  PIPE_SEL_UPDATE_GROUP_CREATE,
  PIPE_SEL_UPDATE_GROUP_DESTROY,
  PIPE_SEL_UPDATE_ADD,
  PIPE_SEL_UPDATE_DEL,
  PIPE_SEL_UPDATE_ACTIVATE,
  PIPE_SEL_UPDATE_DEACTIVATE,
  PIPE_SEL_UPDATE_SET_FALLBACK,
  PIPE_SEL_UPDATE_CLR_FALLBACK
};
struct pipe_sel_update_group_create_params {
  pipe_sel_grp_hdl_t grp_hdl;
  uint32_t max_members;
  uint32_t num_indexes;
  pipe_idx_t base_logical_index;
  int logical_adt_index_array_length;
  struct pipe_multi_index *logical_adt_indexes;
};
struct pipe_sel_update_group_destroy_params {
  pipe_sel_grp_hdl_t grp_hdl;
};
struct pipe_sel_update_add_params {
  pipe_sel_grp_hdl_t grp_hdl;
  pipe_adt_ent_hdl_t ent_hdl;
  pipe_idx_t logical_index;
  pipe_idx_t logical_subindex;
  void *data;
};
struct pipe_sel_update_del_params {
  pipe_sel_grp_hdl_t grp_hdl;
  pipe_adt_ent_hdl_t ent_hdl;
  pipe_idx_t logical_index;
  pipe_idx_t logical_subindex;
};
struct pipe_sel_update_activate_params {
  pipe_sel_grp_hdl_t grp_hdl;
  pipe_adt_ent_hdl_t ent_hdl;
  pipe_idx_t logical_index;
  pipe_idx_t logical_subindex;
};
struct pipe_sel_update_deactivate_params {
  pipe_sel_grp_hdl_t grp_hdl;
  pipe_adt_ent_hdl_t ent_hdl;
  pipe_idx_t logical_index;
  pipe_idx_t logical_subindex;
};
struct pipe_sel_set_fallback_params {
  pipe_adt_ent_hdl_t ent_hdl;
  void *data;
};
union pipe_sel_update_params {
  struct pipe_sel_update_group_create_params grp_create;
  struct pipe_sel_update_group_destroy_params grp_destroy;
  struct pipe_sel_update_add_params add;
  struct pipe_sel_update_del_params del;
  struct pipe_sel_update_activate_params activate;
  struct pipe_sel_update_deactivate_params deactivate;
  struct pipe_sel_set_fallback_params set_fallback;
};
typedef void (*pipe_sel_update_cb)(bf_dev_target_t dev_tgt,
                                   pipe_tbl_hdl_t tbl_hdl,
                                   enum pipe_sel_update_type update_type,
                                   union pipe_sel_update_params *update_params,
                                   void *cookie);
bf_status_t pipe_register_sel_update_cb(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t device_id,
                                        pipe_sel_tbl_hdl_t tbl_hdl,
                                        pipe_sel_update_cb cb,
                                        void *cb_cookie);

/********************************************
 * Library init/cleanup API
 ********************************************/

/* API to invoke pipe_mgr initialization */
pipe_status_t pipe_mgr_init(void);

void pipe_mgr_cleanup(void);

/********************************************
 * CLIENT API
 ********************************************/

/* API to invoke client library registration */
pipe_status_t pipe_mgr_client_init(pipe_sess_hdl_t *sess_hdl);

/* API to invoke client library de-registration */
pipe_status_t pipe_mgr_client_cleanup(pipe_sess_hdl_t def_sess_hdl);

/********************************************
 * Transaction related API */
/*!
 * Begin a transaction on a session. Only one transaction can be in progress
 * on any given session
 *
 * @param shdl Handle to an active session
 * @param isAtomic If @c true, upon committing the transaction, all changes
 *        will be applied atomically such that a packet being processed will
 *        see either all of the changes or none of the changes.
 * @return Status of the API call
 */
pipe_status_t pipe_mgr_begin_txn(pipe_sess_hdl_t shdl, bool isAtomic);

/*!
 * Verify if all the API requests against the transaction in progress have
 * resources to be committed. This also ends the transaction implicitly
 *
 * @param Handle to an active session
 * @return Status of the API call
 */
pipe_status_t pipe_mgr_verify_txn(pipe_sess_hdl_t shdl);

/*!
 * Abort and rollback all API requests against the transaction in progress
 * This also ends the transaction implicitly
 *
 * @param Handle to an active session
 * @return Status of the API call
 */
pipe_status_t pipe_mgr_abort_txn(pipe_sess_hdl_t shdl);

/*!
 * Abort and rollback all API requests against the transaction in progress
 * This also ends the transaction implicitly
 *
 * @param Handle to an active session
 * @return Status of the API call
 */
pipe_status_t pipe_mgr_commit_txn(pipe_sess_hdl_t shdl, bool hwSynchronous);

/********************************************
 * Batch related API */
/*!
 * Begin a batch on a session. Only one batch can be in progress
 * on any given session.  Updates to the hardware will be batch together
 * and delayed until the batch is ended.
 *
 * @param shdl Handle to an active session
 * @return Status of the API call
 */
pipe_status_t pipe_mgr_begin_batch(pipe_sess_hdl_t shdl);

/*!
 * Flush a batch on a session pushing all pending updates to hardware.
 *
 * @param shdl Handle to an active session
 * @return Status of the API call
 */
pipe_status_t pipe_mgr_flush_batch(pipe_sess_hdl_t shdl);

/*!
 * End a batch on a session and push all batched updated to hardware.
 *
 * @param shdl Handle to an active session
 * @return Status of the API call
 */
pipe_status_t pipe_mgr_end_batch(pipe_sess_hdl_t shdl, bool hwSynchronous);

/*!
 * Helper function for of-tests. Return after all the pending operations
 * for the given session have been completed.
 *
 * @param Handle to an active session
 * @return Status of the API call
 */
pipe_status_t pipe_mgr_complete_operations(pipe_sess_hdl_t shdl);

pipe_status_t pipe_mgr_tbl_is_tern(bf_dev_id_t dev_id,
                                   pipe_tbl_hdl_t tbl_hdl,
                                   bool *is_tern);

/*!
 * API to get a match spec reference for the given match-entry handle
 * @output param : entry_pipe_id
 * @output param : match_spec
 * @Note It is user's responsibility to copy/caching the match spec since it can
 * be altered
 * as the table get updated.
 */
pipe_status_t pipe_mgr_ent_hdl_to_match_spec(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    bf_dev_pipe_t *ent_pipe_id,
    pipe_tbl_match_spec_t const **match_spec);

/*!
 * API to free a pipe_tbl_match_spec_t
 */
pipe_status_t pipe_mgr_match_spec_free(pipe_tbl_match_spec_t *match_spec);

/*!
 * API to duplicate an existing match spec to a new match spec
 * It allocates new memory. (dynamic memory)
 */
pipe_status_t pipe_mgr_match_spec_duplicate(
    pipe_tbl_match_spec_t **match_spec_dest,
    pipe_tbl_match_spec_t const *match_spec_src);

/*!
 * API to apply match key mask on a dynamic key mask table
 */
pipe_status_t pipe_mgr_match_key_mask_spec_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec);

pipe_status_t pipe_mgr_match_key_mask_spec_reset(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl);

pipe_status_t pipe_mgr_match_key_mask_spec_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec);

/*!
 * API to install default (miss) entry for a match action table
 */
pipe_status_t pipe_mgr_mat_default_entry_set(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_act_fn_hdl_t act_fn_hdl,
                                             pipe_action_spec_t *act_spec,
                                             uint32_t pipe_api_flags,
                                             pipe_mat_ent_hdl_t *ent_hdl_p);

/*!
 * API to get default (miss) entry handle for a match action table
 */
pipe_status_t pipe_mgr_table_get_default_entry_handle(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t *ent_hdl_p);

/**
 * Get default entry information
 *
 * @param  sess_hdl              Session handle
 * @param  dev_tgt               Device ID and pipe-id to query.
 * @param  mat_tbl_hdl           Table handle.
 * @param  pipe_action_spec      Action data spec to populate.
 * @param  act_fn_hdl            Action function handle to populate.
 * @param  from_hw               Read from HW.
 * @param  res_get_flags         Bitwise OR of PIPE_RES_GET_FLAG_xxx indicating
 *                               which direct resources should be queried.  If
 *                               an entry does not use a resource it will not be
 *                               queried even if requested.
 *                               Set to PIPE_RES_GET_FLAG_ENTRY for original
 *                               behavior.
 * @param  res_data              Pointer to a pipe_res_data_t to hold the
 *                               resource data requested by res_get_flags.  Can
 *                               be NULL if res_get_flags is zero.
 *                               Note that if PIPE_RES_GET_FLAG_STFUL is set
 *                               pipe_mgr will allocate the data in the
 *                               res_data.stful structure and the caller must
 *                               free it with bf_sys_free.
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_table_get_default_entry(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw,
    uint32_t res_get_flags,
    pipe_res_get_data_t *res_data);

/**
 * Query which direct resources are used by an action on a MAT
 *
 * @param  dev_id                Device ID
 * @param  mat_tbl_hdl           Table handle being queried
 * @param  act_fn_hdl            Action function handle being queried
 * @param  has_dir_stats         Pointer to a bool which will be set to true if
 *                               the action uses direct stats.  May be NULL.
 * @param  has_dir_meter         Pointer to a bool which will be set to true if
 *                               the action uses a direct meter.  May be NULL.
 * @param  has_dir_lpf           Pointer to a bool which will be set to true if
 *                               the action uses a direct LPF.  May be NULL.
 * @param  has_dir_wred          Pointer to a bool which will be set to true if
 *                               the action uses a direct WRED.  May be NULL.
 * @param  has_dir_stful         Pointer to a bool which will be set to true if
 *                               the action uses a direct stful.  May be NULL.
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_get_action_dir_res_usage(bf_dev_id_t dev_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                pipe_act_fn_hdl_t act_fn_hdl,
                                                bool *has_dir_stats,
                                                bool *has_dir_meter,
                                                bool *has_dir_lpf,
                                                bool *has_dir_wred,
                                                bool *has_dir_stful);

/*!
 * API function to clear all entries from a match action table. This API
 * doesn't clear default entry. Use pipe_mgr_mat_tbl_default_entry_reset
 * in conjunction with this API to do the same.
 */
pipe_status_t pipe_mgr_mat_tbl_clear(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                     uint32_t pipe_api_flags);

/*!
 * API function to delete an entry from a match action table using an ent hdl
 */
pipe_status_t pipe_mgr_mat_ent_del(pipe_sess_hdl_t sess_hdl,
                                   bf_dev_id_t device_id,
                                   pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                   pipe_mat_ent_hdl_t mat_ent_hdl,
                                   uint32_t pipe_api_flags);
/*!
 * API function to clear the default entry installed.
 */
pipe_status_t pipe_mgr_mat_tbl_default_entry_reset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t pipe_api_flags);

/*!
 * API function to modify action for a match action table entry using an ent hdl
 */
pipe_status_t pipe_mgr_mat_ent_set_action(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl,
                                          pipe_act_fn_hdl_t act_fn_hdl,
                                          pipe_action_spec_t *act_spec,
                                          uint32_t pipe_api_flags);

/*!
 * API function to modify action for a match action table entry using match spec
 */
pipe_status_t pipe_mgr_mat_ent_set_action_by_match_spec(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags);

pipe_status_t pipe_mgr_mat_ent_set_resource(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_mat_ent_hdl_t mat_ent_hdl,
                                            pipe_res_spec_t *resources,
                                            int resource_count,
                                            uint32_t pipe_api_flags);

/********************************************
 * API FOR ACTION DATA TABLE MANIPULATION   *
 ********************************************/

/* API function to update an action data entry */
pipe_status_t pipe_mgr_adt_ent_set(pipe_sess_hdl_t sess_hdl,
                                   bf_dev_id_t device_id,
                                   pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                   pipe_adt_ent_hdl_t adt_ent_hdl,
                                   pipe_act_fn_hdl_t act_fn_hdl,
                                   pipe_action_spec_t *action_spec,
                                   uint32_t pipe_api_flags);

/*********************************************
 * API FOR SELECTOR TABLE MANIPULATION *
 ********************************************/

/*!
 * Callback function prototype to track updates within a stateful selection
 * table.
 */
typedef pipe_status_t (*pipe_mgr_sel_tbl_update_callback)(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    void *cookie,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    int logical_table_index,
    bool is_add);

/*!
 * API function to register a callback function to track updates to groups in
 * the selection table.
 */
pipe_status_t pipe_mgr_sel_tbl_register_cb(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                           pipe_mgr_sel_tbl_update_callback cb,
                                           void *cb_cookie);

/*!
 * API function to set the group profile for a selection table
 */
pipe_status_t pipe_mgr_sel_tbl_profile_set(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_tbl_profile_t *sel_tbl_profile);

/*!
 * API function to add a new group into a selection table
 */
pipe_status_t pipe_mgr_sel_grp_add(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                   uint32_t max_grp_size,
                                   pipe_sel_grp_hdl_t *sel_grp_hdl_p,
                                   uint32_t pipe_api_flags);
/*!
 * API function to add a member to a group of a selection table
 */
pipe_status_t pipe_mgr_sel_grp_mbr_add(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                       pipe_sel_grp_hdl_t sel_grp_hdl,
                                       pipe_act_fn_hdl_t act_fn_hdl,
                                       pipe_adt_ent_hdl_t adt_ent_hdl,
                                       uint32_t pipe_api_flags);

/*!
 * API function to delete a member from a group of a selection table
 */
pipe_status_t pipe_mgr_sel_grp_mbr_del(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                       pipe_sel_grp_hdl_t sel_grp_hdl,
                                       pipe_adt_ent_hdl_t adt_ent_hdl,
                                       uint32_t pipe_api_flags);

enum pipe_mgr_grp_mbr_state_e {
  PIPE_MGR_GRP_MBR_STATE_ACTIVE = 0,
  PIPE_MGR_GRP_MBR_STATE_INACTIVE = 1
};

/* API function to disable a group member of a selection table */
pipe_status_t pipe_mgr_sel_grp_mbr_disable(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                           pipe_sel_grp_hdl_t sel_grp_hdl,
                                           pipe_adt_ent_hdl_t adt_ent_hdl,
                                           uint32_t pipe_api_flags);

/* API function to re-enable a group member of a selection table */
pipe_status_t pipe_mgr_sel_grp_mbr_enable(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                          pipe_sel_grp_hdl_t sel_grp_hdl,
                                          pipe_adt_ent_hdl_t adt_ent_hdl,
                                          uint32_t pipe_api_flags);

/* API function to get the current state of a selection member */
pipe_status_t pipe_mgr_sel_grp_mbr_state_get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    enum pipe_mgr_grp_mbr_state_e *mbr_state_p);

/* API function to get the member handle given a hash value */
pipe_status_t pipe_mgr_sel_grp_mbr_get_from_hash(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t grp_hdl,
    uint8_t *hash,
    uint32_t hash_len,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p);

/* API function to set the fallback member */
pipe_status_t pipe_mgr_sel_fallback_mbr_set(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_adt_ent_hdl_t adt_ent_hdl,
                                            uint32_t pipe_api_flags);

/* API function to reset the fallback member */
pipe_status_t pipe_mgr_sel_fallback_mbr_reset(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                              uint32_t pipe_api_flags);

/***************************************
 * API FOR FLOW LEARNING NOTIFICATIONS *
 ***************************************/
/* Flow learn notify registration */
pipe_status_t pipe_mgr_lrn_digest_notification_register(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
    pipe_flow_lrn_notify_cb callback_fn,
    void *callback_fn_cookie);

/* Flow learn notify de-registration */
pipe_status_t pipe_mgr_lrn_digest_notification_deregister(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl);

/* Flow learn notification processing completion acknowledgment */
pipe_status_t pipe_mgr_flow_lrn_notify_ack(
    pipe_sess_hdl_t sess_hdl,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
    pipe_flow_lrn_msg_t *pipe_flow_lrn_msg);

/* Flow learn notification set timeout */
pipe_status_t pipe_mgr_flow_lrn_set_timeout(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            uint32_t usecs);

/* Flow learn notification get timeout */
pipe_status_t pipe_mgr_flow_lrn_get_timeout(bf_dev_id_t device_id,
                                            uint32_t *usecs);

pipe_status_t pipe_mgr_flow_lrn_set_network_order_digest(bf_dev_id_t device_id,
                                                         bool network_order);

pipe_status_t pipe_mgr_flow_lrn_set_intr_mode(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t device_id,
                                              bool en);

pipe_status_t pipe_mgr_flow_lrn_get_intr_mode(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t device_id,
                                              bool *en);

/*****************************************
 * API FOR STATISTICS TABLE MANIPULATION *
 *****************************************/

/* API function to query a direct stats entry */
pipe_status_t pipe_mgr_mat_ent_direct_stat_query(pipe_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t device_id,
                                                 pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                 pipe_mat_ent_hdl_t mat_ent_hdl,
                                                 pipe_stat_data_t *stat_data);

/* API function to set/clear a direct stats entry */
pipe_status_t pipe_mgr_mat_ent_direct_stat_set(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t device_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               pipe_mat_ent_hdl_t mat_ent_hdl,
                                               pipe_stat_data_t *stat_data);

/* API function to load a direct stats entry */
pipe_status_t pipe_mgr_mat_ent_direct_stat_load(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                pipe_mat_ent_hdl_t mat_ent_hdl,
                                                pipe_stat_data_t *stat_data);

/* API function to reset a stat table */
pipe_status_t pipe_mgr_stat_table_reset(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                        pipe_stat_data_t *stat_data);

/* API function to query a stats entry */
pipe_status_t pipe_mgr_stat_ent_query(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_target,
                                      pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                      pipe_stat_ent_idx_t stat_ent_idx,
                                      pipe_stat_data_t *stat_data);

/* API function to set/clear a stats entry */
pipe_status_t pipe_mgr_stat_ent_set(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_tgt,
                                    pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                    pipe_stat_ent_idx_t stat_ent_idx,
                                    pipe_stat_data_t *stat_data);

/* API function to load a stats entry (in Hardware) */
pipe_status_t pipe_mgr_stat_ent_load(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                     pipe_stat_ent_idx_t stat_idx,
                                     pipe_stat_data_t *stat_data);

/* API to trigger a stats database sync on the indirectly referenced
 * stats table.
 */
pipe_status_t pipe_mgr_stat_database_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
    void *cookie);

/* API to trigger a stats database sync on the directly referenced
 * stats table.
 */
pipe_status_t pipe_mgr_direct_stat_database_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
    void *cookie);

/* API to trigger a stats entry database sync for an indirectly
 * addressed stat table.
 */
pipe_status_t pipe_mgr_stat_ent_database_sync(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                              pipe_stat_ent_idx_t stat_ent_idx);

/* API to trigger a stats entry database sync for a directly
 * addressed stat table.
 */
pipe_status_t pipe_mgr_direct_stat_ent_database_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl);

/*************************************
 * API FOR METER TABLE MANIPULATION  *
 *************************************/

/* API to reset a meter table */
pipe_status_t pipe_mgr_meter_reset(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                   uint32_t pipe_api_flags);

/* API to reset a lpf table */
pipe_status_t pipe_mgr_lpf_reset(pipe_sess_hdl_t sess_hdl,
                                 dev_target_t dev_tgt,
                                 pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
                                 uint32_t pipe_api_flags);

/* API to reset a wred table */
pipe_status_t pipe_mgr_wred_reset(pipe_sess_hdl_t sess_hdl,
                                  dev_target_t dev_tgt,
                                  pipe_wred_tbl_hdl_t wred_tbl_hdl,
                                  uint32_t pipe_api_flags);

/* API to update a meter entry specification */
pipe_status_t pipe_mgr_meter_ent_set(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                     pipe_meter_idx_t meter_idx,
                                     pipe_meter_spec_t *meter_spec,
                                     uint32_t pipe_api_flags);

/* API to set a meter table bytecount adjust */
pipe_status_t pipe_mgr_meter_set_bytecount_adjust(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    int bytecount);

/* API to get a meter table bytecount adjust */
pipe_status_t pipe_mgr_meter_get_bytecount_adjust(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    int *bytecount);

pipe_status_t pipe_mgr_meter_read_entry(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_mat_ent_hdl_t mat_ent_hdl,
                                        pipe_meter_spec_t *meter_spec);

pipe_status_t pipe_mgr_meter_read_entry_idx(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                            pipe_meter_idx_t index,
                                            pipe_meter_spec_t *meter_spec);


pipe_status_t pipe_mgr_exm_entry_activate(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl);

pipe_status_t pipe_mgr_exm_entry_deactivate(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_mat_ent_hdl_t mat_ent_hdl);

/* Set the Idle timeout TTL for a given match entry */
pipe_status_t pipe_mgr_mat_ent_set_idle_ttl(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t ttl, /*< TTL value in msecs */
    uint32_t pipe_api_flags,
    bool reset);

pipe_status_t pipe_mgr_mat_ent_reset_idle_ttl(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t device_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_mat_ent_hdl_t mat_ent_hdl);

/***************************
 * API FOR IDLE-TMEOUT MGMT*
 ***************************/

/* Configure idle timeout at table level */
pipe_status_t pipe_mgr_idle_get_params(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                       pipe_idle_time_params_t *params);

pipe_status_t pipe_mgr_idle_set_params(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                       pipe_idle_time_params_t params);

pipe_status_t pipe_mgr_idle_tmo_set_enable(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           bool enable);

pipe_status_t pipe_mgr_idle_tmo_get_enable(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           bool *enable);

pipe_status_t pipe_mgr_idle_register_tmo_cb(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_idle_tmo_expiry_cb cb,
                                            void *client_data);

pipe_status_t pipe_mgr_idle_register_tmo_cb_with_match_spec_copy(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_tmo_expiry_cb_with_match_spec_copy cb,
    void *client_data);

/* The below APIs are used for Poll mode operation only */

/* API function to poll idle timeout data for a table entry */
pipe_status_t pipe_mgr_idle_time_get_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_idle_time_hit_state_e *idle_time_data);

/* API function to set hit state data for a table entry */
pipe_status_t pipe_mgr_idle_time_set_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_idle_time_hit_state_e idle_time_data);

/* API function that should be called
 * periodically or on-demand prior to querying for the hit state
 * The function completes asynchronously and the client will
 * be notified of it's completion via the provided callback function
 * After fetching hit state HW values will be reset to idle.
 */
pipe_status_t pipe_mgr_idle_time_update_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_tmo_update_complete_cb callback_fn,
    void *cb_data);

/* The below APIs are used in notify mode */
/* API function to get the current TTL value of the table entry */
pipe_status_t pipe_mgr_mat_ent_get_idle_ttl(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_mat_ent_hdl_t mat_ent_hdl,
                                            uint32_t *ttl);

/***********************************************
 * API FOR STATEFUL MEMORY TABLE MANIPULATION  *
 ***********************************************/
pipe_status_t pipe_stful_ent_set(pipe_sess_hdl_t sess_hdl,
                                 dev_target_t dev_target,
                                 pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                 pipe_stful_mem_idx_t stful_ent_idx,
                                 pipe_stful_mem_spec_t *stful_spec,
                                 uint32_t pipe_api_flags);

pipe_status_t pipe_stful_database_sync(pipe_sess_hdl_t sess_hdl,
                                       dev_target_t dev_tgt,
                                       pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                       pipe_stful_tbl_sync_cback_fn cback_fn,
                                       void *cookie);

pipe_status_t pipe_stful_direct_database_sync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_stful_tbl_sync_cback_fn cback_fn,
    void *cookie);

pipe_status_t pipe_stful_query_get_sizes(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t device_id,
                                         pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                         int *num_pipes);

pipe_status_t pipe_stful_direct_query_get_sizes(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                int *num_pipes);

pipe_status_t pipe_stful_ent_query(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                   pipe_stful_mem_idx_t stful_ent_idx,
                                   pipe_stful_mem_query_t *stful_query,
                                   uint32_t pipe_api_flags);

pipe_status_t pipe_stful_direct_ent_query(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mat_ent_hdl_t mat_ent_hdl,
                                          pipe_stful_mem_query_t *stful_query,
                                          uint32_t pipe_api_flags);

pipe_status_t pipe_stful_table_reset(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                     pipe_stful_mem_spec_t *stful_spec);

pipe_status_t pipe_stful_table_reset_range(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                           pipe_stful_mem_idx_t stful_ent_idx,
                                           uint32_t num_indices,
                                           pipe_stful_mem_spec_t *stful_spec);
pipe_status_t pipe_stful_fifo_occupancy(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_tgt,
                                        pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                        int *occupancy);
pipe_status_t pipe_stful_fifo_reset(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_tgt,
                                    pipe_stful_tbl_hdl_t stful_tbl_hdl);
pipe_status_t pipe_stful_fifo_dequeue(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                      int num_to_dequeue,
                                      pipe_stful_mem_spec_t *values,
                                      int *num_dequeued);
pipe_status_t pipe_stful_fifo_enqueue(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                      int num_to_enqueue,
                                      pipe_stful_mem_spec_t *values);

pipe_status_t pipe_stful_ent_query_range(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_tgt,
                                         pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                         pipe_stful_mem_idx_t stful_ent_idx,
                                         uint32_t num_indices_to_read,
                                         pipe_stful_mem_query_t *stful_query,
                                         uint32_t *num_indices_read,
                                         uint32_t pipe_api_flags);

pipe_status_t pipe_stful_param_set(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_tbl_hdl_t stful_tbl_hdl,
                                   pipe_reg_param_hdl_t rp_hdl,
                                   int64_t value);

pipe_status_t pipe_stful_param_get(pipe_sess_hdl_t sess_hdl,
                                   dev_target_t dev_tgt,
                                   pipe_tbl_hdl_t stful_tbl_hdl,
                                   pipe_reg_param_hdl_t rp_hdl,
                                   int64_t *value);

pipe_status_t pipe_stful_param_reset(pipe_sess_hdl_t sess_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_tbl_hdl_t stful_tbl_hdl,
                                     pipe_reg_param_hdl_t rp_hdl);

pipe_status_t pipe_stful_param_get_hdl(bf_dev_id_t dev,
                                       const char *name,
                                       pipe_reg_param_hdl_t *hdl);

/**
 * Get pipe-id for a particular port
 *
 * @param  dev_port_id           Port-id.
 * @return                       Pipe
 */
bf_dev_pipe_t dev_port_to_pipe_id(uint16_t dev_port_id);

/**
 * Get next group members
 *
 * @param  sess_hdl              Session handle.
 * @param  tbl_hdl               Table handle.
 * @param  dev_id                Device ID.
 * @param  sel_grp_hdl           Group handle
 * @param  mbr_hdl               Member handle
 * @param  n                     Number of group member handles requested
 * @param  next_mbr_hdls         Array big enough to hold n member handles
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_get_next_group_members(
    pipe_sess_hdl_t sess_hdl,
    pipe_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t mbr_hdl,
    int n,
    pipe_adt_ent_hdl_t *next_mbr_hdls);

pipe_status_t pipe_mgr_get_word_llp_active_member_count(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t tbl_hdl,
    uint32_t word_index,
    uint32_t *count);

pipe_status_t pipe_mgr_get_word_llp_active_members(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t tbl_hdl,
    uint32_t word_index,
    uint32_t count,
    pipe_adt_ent_hdl_t *mbr_hdls);

/**
 * Get reserved entry count for the given table
 *
 * Returns the number of reserved table locations.
 * Table locations may be reserved by the driver for uses such as
 * default entry direct resources or atomic entry modification.
 * Currently supported only for TCAM tables and can be used by applications
 * which need to know exactly how many entries can be inserted into the table.
 *
 * @param  sess_hdl              Session handle.
 * @param  dev_tgt               Device target.
 * @param  tbl_hdl               Table handle.
 * @param  count                 Pointer to the size
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_get_reserved_entry_count(pipe_sess_hdl_t sess_hdl,
                                                dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                size_t *count);

/**
 * Get entry count for the given table
 *
 * @param  sess_hdl              Session handle.
 * @param  dev_tgt               Device target.
 * @param  tbl_hdl               Table handle.
 * @param  count                 Pointer to the entry count
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_get_entry_count(pipe_sess_hdl_t sess_hdl,
                                       dev_target_t dev_tgt,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       bool read_from_hw,
                                       uint32_t *count);

/**
 * Set the table property
 *
 * @param  sess_hdl              Session handle.
 * @param  tbl_hdl               Table handle.
 * @param  property              Property Type.
 * @param  value                 Value.
 * @param  args                  Scope args.
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_tbl_set_property(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t dev_id,
                                        pipe_mat_tbl_hdl_t tbl_hdl,
                                        pipe_mgr_tbl_prop_type_t property,
                                        pipe_mgr_tbl_prop_value_t value,
                                        pipe_mgr_tbl_prop_args_t args);

/**
 * Get the table property
 *
 * @param  sess_hdl              Session handle.
 * @param  tbl_hdl               Table handle.
 * @param  property              Property Type.
 * @param  value                 Value.
 * @param  args                  Scope args.
 * @return                       Status of the API call
 */
pipe_status_t pipe_mgr_tbl_get_property(pipe_sess_hdl_t sess_hdl,
                                        bf_dev_id_t dev_id,
                                        pipe_mat_tbl_hdl_t tbl_hdl,
                                        pipe_mgr_tbl_prop_type_t property,
                                        pipe_mgr_tbl_prop_value_t *value,
                                        pipe_mgr_tbl_prop_args_t *args);

pipe_status_t pipe_set_adt_ent_hdl_in_mat_data(void *data,
                                               pipe_adt_ent_hdl_t adt_ent_hdl);

pipe_status_t pipe_set_sel_grp_hdl_in_mat_data(void *data,
                                               pipe_adt_ent_hdl_t sel_grp_hdl);

pipe_status_t pipe_set_ttl_in_mat_data(void *data, uint32_t ttl);

/* Given the dev port, get the corresponding pipe id to which this port belongs
 */
pipe_status_t pipe_mgr_pipe_id_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   bf_dev_pipe_t *pipe_id);

/*
 * This function is used to get the pipe val to be Or'ed with the handle
 * from context.json. Since context.json is separate per pipeline, this
 * function takes in both program and pipeline names
 *
 * @param  dev_id           Device ID
 * @param  prog_name        Program name
 * @param  pipeline_name    pipeline name
 * @param  pipe_mask (out)  Pipe mask
 * @return                  Status of the API call
 */
bf_status_t pipe_mgr_tbl_hdl_pipe_mask_get(bf_dev_id_t dev_id,
                                           const char *prog_name,
                                           const char *pipeline_name,
                                           uint32_t *pipe_mask);

/*
 * This function is used to get the number of active pipelines
 *
 * @param  dev_id 	        Device ID
 * @param  num_pipes (out)	Number of pipes
 * @return 		        Status of the API call
 */
pipe_status_t pipe_mgr_get_num_pipelines(bf_dev_id_t dev_id,
                                         uint32_t *num_pipes);

 /**
 * This function is used to build the pipeline
 *
 * @param  dev_id               Device ID
 * @return                      Status of the API call
 */
pipe_status_t pipe_mgr_enable_pipeline(bf_dev_id_t dev_id);


/*
 * This function is used to get the number of active pipelines
 *
 * @param  dev_id 	        Device ID
 * @param  num_pipes (out)	Number of pipes
 * @return 		        Status of the API call
 */
pipe_status_t pipe_mgr_get_num_pipelines(bf_dev_id_t dev_id,
                                         uint32_t *num_pipes);


typedef struct adt_data_resources_ {
  pipe_res_hdl_t tbl_hdl;
  pipe_res_idx_t tbl_idx;
} adt_data_resources_t;

/*  ---- Table debug counter APIs start  ---- */

/**
 * The function be used to set the counter type for table
   debug counter.
 *
 * @param  dev_tgt               ASIC Device information.
 * @param  tbl_name              Table name.
 * @param  type                  Type.
 * @return                       Status of the API call
 */
pipe_status_t bf_tbl_dbg_counter_type_set(dev_target_t dev_tgt,
                                          char *tbl_name,
                                          bf_tbl_dbg_counter_type_t type);

pipe_status_t bf_log_tbl_dbg_counter_type_set(dev_target_t dev_tgt,
					  uint32_t stage,
                                          uint32_t tbl_name,
                                          bf_tbl_dbg_counter_type_t type);

/**
 * The function be used to get the counter value for table
   debug counter.
 *
 * @param  dev_tgt               ASIC Device information.
 * @param  tbl_name              Table name.
 * @param  type                  Type.
 * @param  value                 Value.
 * @return                       Status of the API call
 */
pipe_status_t bf_tbl_dbg_counter_get(dev_target_t dev_tgt,
                                     char *tbl_name,
                                     bf_tbl_dbg_counter_type_t *type,
                                     uint32_t *value);

pipe_status_t bf_log_tbl_dbg_counter_get(dev_target_t dev_tgt,
				     uint32_t stage,
                                     uint32_t log_tbl,
                                     bf_tbl_dbg_counter_type_t *type,
                                     uint32_t *value);

/**
 * The function be used to clear the counter value for table
   debug counter.
 *
 * @param  dev_tgt               ASIC Device information.
 * @param  tbl_name              Table name.
 * @return                       Status of the API call
 */
pipe_status_t bf_tbl_dbg_counter_clear(dev_target_t dev_tgt, char *tbl_name);
pipe_status_t bf_log_tbl_dbg_counter_clear(dev_target_t dev_tgt,
			     uint32_t stage,
			     uint32_t log_tbl);

/**
 * The function be used to set the debug counter type for all tables
   in a stage
 *
 * @param  dev_tgt               ASIC Device information.
 * @param  stage_id              Stage.
 * @param  type                  Counter type.
 * @return                       Status of the API call
 */
pipe_status_t bf_log_dbg_counter_type_stage_set(dev_target_t dev_tgt,
                                                dev_stage_t stage_id,
                                                bf_tbl_dbg_counter_type_t type);

/**
 * The function be used to get the debug counter value for all tables
   in a stage
 *
 * @param  dev_tgt               ASIC Device information.
 * @param  stage_id              Stage.
 * @param  type_arr              List of counter types.
 * @param  value_arr             List of counter values.
 * @param  tbl_name              List of table names.
 * @param  num_counters          Number of counters.
 * @return                       Status of the API call
 */
#define PIPE_MGR_TBL_NAME_LEN 200
pipe_status_t bf_log_dbg_counter_stage_get(
    dev_target_t dev_tgt,
    dev_stage_t stage_id,
    bf_tbl_dbg_counter_type_t *type_arr,
    uint32_t *value_arr,
    char tbl_name[][PIPE_MGR_TBL_NAME_LEN],
    int *num_counters);

/**
 * The function be used to clear the debug counter value for all tables
   in a stage
 *
 * @param  dev_tgt               ASIC Device information.
 * @param  stage_id              Stage.
 * @return                       Status of the API call
 */
pipe_status_t bf_log_dbg_counter_stage_clear(dev_target_t dev_tgt,
                                             dev_stage_t stage_id);

pipe_status_t pipe_mgr_tbl_dbg_counter_get_list (bf_dev_target_t dev_tgt,
                                                  char **tbl_list,
                                                  int *num_tbls);

/*  ---- Table debug counter APIs end  ---- */
#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _PIPE_MGR_INTF_H */

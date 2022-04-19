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

#ifndef _PIPE_MGR_MAT_H
#define _PIPE_MGR_MAT_H

/* OSAL includes */
#include <osdep/p4_sde_osdep.h>

/* Module header files */
#include <bf_types/bf_types.h>
#include <pipe_mgr/pipe_mgr_err.h>

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef BFRT_FIXED
/*!
 * Generalized match specification for any lookup table entry
 */
struct pipe_tbl_match_spec {
	u16 num_valid_match_bits; /*!< Number of match bits valid */
	u16 num_match_bytes;      /*!< Size of padded match_value_bits */
	u8 *match_value_bits;     /*!< Value of match bits */
	u8 *match_mask_bits;
	u32 priority;
	/*!< Priority dictates the position of a ternary table
	 * entry in relation to other entries in the table
	 */
	u64 cookie;
};
#else // BFRT_FIXED
/* TODO: Remove below code within this #else and #endif. This is provided till
 * BFRT layer is updated to use the new declarations.
 */
struct pipe_tbl_match_spec {
  uint32_t partition_index;      /*!< The partition index for this entry */
  uint16_t num_valid_match_bits; /*!< Number of match bits valid */
  uint16_t num_match_bytes;      /*!< Size of padded match_value_bits */
  uint8_t *match_value_bits;     /*!< Value of match bits */
  uint8_t *match_mask_bits;
  /*!< Mask for matching. Valid only for ternary tables */
  uint16_t num_match_validity_fields;
  /*!< Number of match fields that require have validity qual */
  uint16_t match_field_validity;
  /*!< Ordered list of match field validity values */
  uint16_t match_field_validity_mask;
  /*!< Mask for matching. Valid only for ternary tables */
  uint32_t priority;
  /*!< Priority dictates the position of a ternary table
   * entry in relation to other entries in the table
   */
};
#endif // !BFRT_FIXED

/*!
 * Enum to define meter rate unit types
 */
enum pipe_meter_type {
	METER_TYPE_COLOR_AWARE,   /*< Color aware meter */
	METER_TYPE_COLOR_UNAWARE, /*< Color unaware meter */
};

enum pipe_meter_rate_type {
	METER_RATE_TYPE_KBPS, /*< Type to measure rate in kilobits per sec */
	METER_RATE_TYPE_PPS,  /*< Type to measure rate in pkts per sec */
};

/*!
 * Structure for meter specification
 */
struct pipe_meter_rate {
	enum pipe_meter_rate_type type; /*< Type of rate specified */
	union {
		u64 kbps; /*< Rate in units of kilobits per second  */
		u64 pps;  /*< Rate units of pkts per second */
	} value;
};

/*! Structure for meter specification
 */
struct pipe_meter_spec {
	enum pipe_meter_type meter_type;
	/*< Meter type */
	struct pipe_meter_rate cir;
	/*< Meter committed information rate */
	u64 cburst;
	/*< Meter committed burst size */
	struct pipe_meter_rate pir;
	/*< Meter peak information rate */
	u64 pburst;
	/*< Meter peak burst size */
};

/*! Statstics state data
 */
struct pipe_stat_data {
	u64 bytes;   /*< Byte statistics */
	u64 packets; /*< Packet statistics */
};

/*! Meter state data
 */
struct pipe_meter_data {
	u64 comm_conformed_units;
	/*< bytes or packets that conformed to meter committed rate */
	u64 comm_exceeded_units;
	/*< bytes or packets that exceeded meter committed rate */
	u64 peak_exceeded_units;
	/*< bytes or packets that exceeded meter peak rate */
};

/* TODO: Need to understand more about selection group. */
/* Profile of number groups sizes of a certain size in a selection table */
struct pipe_sel_grp_profile {
	u16 grp_size; /* Max members in a grp <2..4K> */
	u16 num_grps; /* Number of groups of this <grp_size> */
};

struct pipe_sel_tbl_profile {
	u16 num_grp_profiles;
	struct pipe_sel_grp_profile *grp_profile_list; /* array */
};

#ifdef BFRT_FIXED
union pipe_res_data_spec {
	struct pipe_meter_spec meter;
	struct pipe_stat_data counter;
};

struct pipe_res_spec {
	u32 tbl_hdl;
	union pipe_res_data_spec data;
};
#else // BFRT_FIXED
/* TODO: Remove below code within this #else and #endif. This is provided till
 * BFRT layer is updated to use the new declarations.
 */
typedef struct pipe_tbl_match_spec pipe_tbl_match_spec_t;

/* Profile of number groups sizes of a certain size in a selection table */
typedef struct pipe_sel_grp_profile pipe_sel_grp_profile_t;

typedef struct pipe_sel_tbl_profile pipe_sel_tbl_profile_t;

/*!
 * Enum to define meter rate unit types
 */
typedef enum pipe_meter_type pipe_meter_type_e;

typedef enum pipe_meter_rate_type pipe_meter_rate_type_e;

/*!
 * Structure for meter specification
 */
typedef struct pipe_meter_rate pipe_meter_rate_t;

/*! Structure for meter specification
 */
typedef struct pipe_meter_spec pipe_meter_spec_t;

/*! Statstics state data
 */
typedef struct pipe_stat_data pipe_stat_data_t;

/*! Meter state data
 */
typedef struct pipe_meter_data pipe_meter_data_t;

/*!
 * Enum to define lpf type
 */
typedef enum pipe_lpf_type_ {
  LPF_TYPE_RATE,   /*< Rate LPF */
  LPF_TYPE_SAMPLE, /*< Sample LPF */
} pipe_lpf_type_e;

/*! Structure for a LPF specification
 */
typedef struct pipe_lpf_spec {
  pipe_lpf_type_e lpf_type;
  /*< Enum indicating the type of lpf */
  bool gain_decay_separate_time_constant;
  /*< A flag indicating if a separate rise/fall time constant is desired */
  float gain_time_constant;
  /*< Rise time constant, in nanoseconds, valid only if the above flag is set */
  float decay_time_constant;
  /*< Fall time constant, in nanoseconds valid only if
   * rise_fall_separate_time_constant
   *  flag is set
   */
  float time_constant;
  /*< A common time constant, in nanoseconds valid only if the
   *  rise_fall_separate_time_constant is not set
   */
  uint32_t output_scale_down_factor;
  /*< An integer indicating the scale down factor, right-shifted by these
   *  many bits. Values range from 0 to 31
   */

} pipe_lpf_spec_t;

/*! Structure for a WRED specification
 */
typedef struct pipe_wred_spec {
  float time_constant;
  /*< Time constant, in nanoseconds*/
  uint32_t red_min_threshold;
  /*< Queue threshold above which the probabilistic dropping starts in units
   *  of packet buffer cells
   */
  uint32_t red_max_threshold;
  /*< Queue threshold above which all packets are dropped in units cells*/
  float max_probability;
  /*< Maximum probability desired for marking the packet, with range from 0.0 to
   * 1.0 */

} pipe_wred_spec_t;

typedef union pipe_stful_mem_spec_t {
  bool bit;
  uint8_t byte;
  uint16_t half;
  uint32_t word;
  uint64_t dbl;
  struct {
    uint8_t hi;
    uint8_t lo;
  } dbl_byte;
  struct {
    uint16_t hi;
    uint16_t lo;
  } dbl_half;
  struct {
    uint32_t hi;
    uint32_t lo;
  } dbl_word;
  struct {
    uint64_t hi;
    uint64_t lo;
  } dbl_dbl;
} pipe_stful_mem_spec_t;

typedef union pipe_res_data_spec {
  pipe_stful_mem_spec_t stful;
  pipe_meter_spec_t meter;
  pipe_lpf_spec_t lpf;
  pipe_wred_spec_t red;
  pipe_stat_data_t counter;
} pipe_res_data_spec_t;

/*! Stateful memory data
 */
enum pipe_res_action_tag {
  PIPE_RES_ACTION_TAG_NO_CHANGE,
  PIPE_RES_ACTION_TAG_ATTACHED,
  PIPE_RES_ACTION_TAG_DETACHED
};

struct pipe_res_spec {
  u32 tbl_hdl;  // Use PIPE_GET_HDL_TYPE to decode
  u32 tbl_idx;
  pipe_res_data_spec_t data;
  enum pipe_res_action_tag tag;
};

typedef union pipe_res_data_spec pipe_res_data_spec_t;

typedef struct pipe_res_spec pipe_res_spec_t;

#endif // !BFRT_FIXED

/*!
 * Action data specification
 */
struct pipe_action_data_spec {
	u16 num_valid_action_data_bits;
	u16 num_action_data_bytes;
	/*!< Number of action data bits valid */
	u8 *action_data_bits;
	/*!< Action data */
};

/* Types of action data for a match-action table entry */
#define PIPE_ACTION_DATA_TYPE 0x1
#define PIPE_ACTION_DATA_HDL_TYPE 0x2
#define PIPE_SEL_GRP_HDL_TYPE 0x4

/*!
 * Generalized action specification that encodes all types of action data refs
 */
struct pipe_action_spec {
	u8 pipe_action_datatype_bmap;
	/* bitmap of action datatypes */
	struct pipe_action_data_spec act_data;
	u32 adt_ent_hdl;
	/* TODO: What is the equivalent for ADT in Asic? Modify content/blob?
	 * Revisit adt_ent_hdl and modify it for Asic.
	 */
	u32 sel_grp_hdl;
#define PIPE_NUM_TBL_RESOURCES 4
	/* TODO: How many resource spec should we support? */
	struct pipe_res_spec resources[PIPE_NUM_TBL_RESOURCES];
	/* Contains meters, counters and any there resources information. */
	int resource_count;
};

/*!
 * API to install an entry into a match action table
 */
int pipe_mgr_mat_ent_add
	 (u32 sess_hdl,
	  struct bf_dev_target_t dev_tgt,
	  u32 mat_tbl_hdl,
	  struct pipe_tbl_match_spec *match_spec,
	  u32 act_fn_hdl,
	  struct pipe_action_spec *act_data_spec,
	  u32 ttl, /*< TTL value in msecs, 0 for disable */
	  u32 pipe_api_flags,
	  /* Specify if the API should be hw synchronous or not. */
	  u32 *ent_hdl_p);

/*!
 * API function to delete an entry from a match action table using a match spec
 */
int pipe_mgr_mat_ent_del_by_match_spec
	 (u32 sess_hdl,
	  struct bf_dev_target_t dev_tgt,
	  u32 mat_tbl_hdl,
	  struct pipe_tbl_match_spec *match_spec,
	  u32 pipe_api_flags);


/*!
 * API function to get entry handle from a match action table using a
 * match spec
 */
int pipe_mgr_match_spec_to_ent_hdl(u32 sess_hdl,
	 struct bf_dev_target_t dev_tgt,
	 u32 mat_tbl_hdl,
	 struct pipe_tbl_match_spec *match_spec,
	 u32 *ent_hdl_p);

/*
 * API function to specifiy if the given MatchAction table stores entries in sofware.
 *
 * @param  sess_hdl		Session handle
 * @param  mat_tbl_hdl		Table handle.
 * @param  dev_tgt		Device Target.
 * @param  store_entries   	Specifies if entries are stored in SW.
 * @return			Status of the API call
 */
int pipe_mgr_store_entries(u32 sess_hdl,  u32 mat_tbl_hdl,
			   struct bf_dev_target_t dev_tgt,
			   bool *store_entries);

/**
 * Get entry information
 *
 * @param  sess_hdl		Session handle
 * @param  tbl_hdl		Table handle.
 * @param  dev_tgt		Device Target.
 * @param  entry_hdl		Entry handle.
 * @param  match_spec		Match spec to populate.
 * @param  act_data_spec	Action data spec to populate.
 * @param  act_fn_hdl		Action function handle to populate.
 * @param  from_hw		Read from HW.
 * @param  res_get_flags	Bitwise OR of PIPE_RES_GET_FLAG_xxx indicating
 * 				which direct resources should be queried.  If
 *				an entry does not use a resource it will not be
 *				queried even if requested.
 *				Set to PIPE_RES_GET_FLAG_ENTRY for original
 *				behavior.
 * @param  res_data		Pointer to a pipe_res_data_t to hold the
 *				resource data requested by res_get_flags.  Can
 *				be NULL if res_get_flags is zero.
 *				Note that if PIPE_RES_GET_FLAG_STFUL is set
 *				pipe_mgr will allocate the data in the
 *				res_data.stful structure and the caller must
 *				free it with bf_sys_free.
 * @return			Status of the API call
 */
int pipe_mgr_get_entry(u32 sess_hdl,
		u32 mat_tbl_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 entry_hdl,
		struct pipe_tbl_match_spec *match_spec,
		struct pipe_action_spec *act_data_spec,
		u32 *act_fn_hdl,
		bool from_hw,
		uint32_t res_get_flags,
		void *res_data);

/**
 * Get first entry key of the given table.
 *
 * @param  sess_hdl		Session handle.
 * @param  mat_tbl_hdl		Match Action Table handle.
 * @param  dev_tgt		Target device/pipe.
 * @param  match_spec		Match spec to be returned.
 * @param  act_data_spec	Action data spec to populate.
 * @param  act_fn_hdl		Action handle to populate.
 * @return			Status of the API call
 */
int pipe_mgr_get_first_entry(u32 sess_hdl,
			     u32 mat_tbl_hdl,
			     struct bf_dev_target_t dev_tgt,
			     struct pipe_tbl_match_spec *match_spec,
			     struct pipe_action_spec *act_data_spec,
			     u32 *act_fn_hdl);
/**
 * Get first entry handle of the given table.
 *
 * @param  sess_hdl		Session handle.
 * @param  mat_tbl_hdl		Match Action Table handle.
 * @param  dev_tgt		Target device/pipe.
 * @param  entry_handle		First entry handle returned.
 * @return			Status of the API call
 */
int pipe_mgr_get_first_entry_handle(u32 sess_hdl,
				    u32 mat_tbl_hdl,
				    struct bf_dev_target_t dev_tgt,
				    u32 *entry_handle);

/**
 * Get next N entry handle of the given table.
 *
 * @param  sess_hdl		Session handle.
 * @param  mat_tbl_hdl		Match Action Table handle.
 * @param  dev_tgt		Target device/pipe.
 * @param  entry_handle		Entry handle to start fetching next N handles.
 * @param  n			Number of entry handles to be returned.
 * @param  next_entry_handles	Caller provided array to return 'n' entry handles.
 * @return			Status of the API call
 */
int pipe_mgr_get_next_entry_handles(u32 sess_hdl,
				    u32 mat_tbl_hdl,
				    struct bf_dev_target_t dev_tgt,
				    int entry_handle,
				    int n,
				    u32 *next_entry_handles);
/**
 * Get Next N entries using key.
 *
 * @param  sess_hdl		Session handle.
 * @param  mat_tbl_hdl		Match Action Table handle.
 * @param  dev_tgt		Target device/pipe.
 * @param  cur_match_spec	Match Spec from which next N entries should be returned.
 * @param  n			Number of entries. Match_specs and act_specs array should be
 *				of n size.
 * @param  match_specs		Match specs to be returned.
 * @param  act_specs		Action specs to be returned. Array of pointers.
 * @param  act_fn_hdl		Action handles to populate.
 * @param  num			Number of entries returned.
 * @return			Status of the API call
 */
int pipe_mgr_get_next_n_by_key(u32 sess_hdl,
			       u32 mat_tbl_hdl,
			       struct bf_dev_target_t dev_tgt,
			       struct pipe_tbl_match_spec *cur_match_spec,
			       int n,
			       struct pipe_tbl_match_spec *match_specs,
			       struct pipe_action_spec **act_specs,
			       u32 *act_fn_hdls,
			       u32 *num);

/**
 * add entry information
 *
 * @param  sess_hdl		Session handle
 * @param  adt_tbl_hdl		ADT Table handle.
 * @param  adt_ent_hdl_p	Entry handle.
 * @param  action_spec		Action data spec to populate.
 * @param  act_fn_hdl		Action function handle to populate.
 * @return			Status of the API call
 */
int pipe_mgr_adt_ent_add(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 adt_tbl_hdl,
		u32 act_fn_hdl,
		struct pipe_action_spec *action_spec,
		u32 *adt_ent_hdl_p,
		uint32_t pipe_api_flags);

/**
 * add or delete ADT entry reference count
 *
 * @param  adt_tbl		ADT Table struct.
 * @param  adt_ent_hdl_p	Entry handle.
 * @param  op			op = 0 add / op = 1 delete.
 * @return			Status of the API call
 */
int pipe_mgr_adt_member_reference_add_delete(
		struct bf_dev_target_t dev_tgt,
		u32 adt_tbl_hdl,
		u32 adt_ent_hdl_p,
		int op);

/**
 * Get action data entry
 *
 * @param  tbl_hdl			Table handle.
 * @param  dev_tgt			Device target.
 * @param  pipe_action_data_spec	Action data spec.
 * @param  act_fn_hdl			Action function handle.
 * @param  from_hw			Read from HW.
 * @return				Status of the API call
 */
int pipe_mgr_get_action_data_entry(u32 sess_hdl,
		u32 tbl_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 entry_hdl,
		struct pipe_action_data_spec *pipe_action_data_spec,
		u32 *act_fn_hdl,
		bool from_hw);

/**
 * delete action data entry
 *
 * @param  tbl_hdl			Table handle.
 * @param  dev_tgt			Device target.
 * @param  pipe_action_data_spec	Action data spec.
 * @param  adt_ent_hdl			Action data entry handle.
 * @return				Status of the API call
 */
int pipe_mgr_adt_ent_del(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 adt_tbl_hdl,
		u32 adt_ent_hdl,
		uint32_t pipe_api_flags);

/**
 * Add selector group
 *
 * @param  sel_tbl_hdl		Table handle.
 * @param  max_grp_size		max group size.
 * @param  sel_grp_hdl_p	entry handle.
 * @param  act_fn_hdl		Action function handle.
 * @return			Status of the API call
 */
int pipe_mgr_sel_grp_add(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 sel_tbl_hdl,
		uint32_t max_grp_size,
		u32 *sel_grp_hdl_p,
		uint32_t pipe_api_flags);

/**
 * Add action member to selector group
 *
 * @param  sel_tbl_hdl		Table handle.
 * @param  sel_grp_hdl		Group handle.
 * @param  num_mbrs		Number of member.
 * @param  mbrs			Member array.
 * @return			Status of the API call
 */
int pipe_mgr_sel_grp_mbrs_set(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 sel_tbl_hdl,
		u32 sel_grp_hdl,
		uint32_t num_mbrs,
		u32 *mbrs,
		bool *enable,
		uint32_t pipe_api_flags);

/**
 * Get first group member
 *
 * @param  sess_hdl		Session handle
 * @param  tbl_hdl		Table handle.
 * @param  dev_tgt		Device target.
 * @param  sel_grp_hdl		Group handle
 * @param  mbr_hdl		Pointer to the member handle
 * @return			Status of the API call
 */
int pipe_mgr_get_first_group_member(u32 sess_hdl,
		u32 tbl_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 sel_grp_hdl,
		u32 *mbr_hdl);

/**
 * Get member count for the given selector group
 *
 * @param  sess_hdl	Session handle.
 * @param  dev_tgt	Device target.
 * @param  tbl_hdl	Selector table handle.
 * @param  grp_hdl	Selector group handle.
 * @param  count	Pointer to the member count.
 * @return		Status of the API call
 */
int pipe_mgr_get_sel_grp_mbr_count(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 tbl_hdl,
		u32 grp_hdl,
		uint32_t *count);

/**
 * Get members for the given selector group
 *
 * @param  sess_hdl		Session handle.
 * @param  dev_target	Device target.
 * @param  tbl_hdl		Selector table handle.
 * @param  grp_hdl		Selector group handle.
 * @param  mbrs_size		member count requested.
 * @param  mbrs			pointer to member requested.
 * @param  enable		pointer to member state.
 * @param  mbrs_populated	pointer to count of members returned.
 * @return			Status of the API call
 */
int pipe_mgr_sel_grp_mbrs_get(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 tbl_hdl,
		u32 grp_hdl,
		uint32_t mbrs_size,
		u32 *mbrs,
		bool *enable,
		u32 *mbrs_populated);

/**
 * DELETE group
 *
 * @param  sess_hdl	Session handle.
 * @param  dev_tgt	Device target.
 * @param  tbl_hdl	Selector table handle.
 * @param  grp_hdl	Selector group handle.
 * @return		Status of the API call
 */
int pipe_mgr_sel_grp_del(u32 sess_hdl,
		struct bf_dev_target_t dev_tgt,
		u32 tbl_hdl,
		u32 grp_hdl,
		uint32_t pipe_api_flags);

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _PIPE_MGR_MAT_H */

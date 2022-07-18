/*
 * Copyright(c) 2022 Intel Corporation.
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
 * @file pipe_mgr_value_lookup.h
 *
 * @Description Declarations for interfaces to value lookup table.
 */

#ifndef __PIPE_MGR_VALUE_LOOKUP_H__
#define __PIPE_MGR_VALUE_LOOKUP_H__

/* OSAL includes */
#include <osdep/p4_sde_osdep.h>

/* Module header files */
#include <bf_types/bf_types.h>
#include <pipe_mgr/pipe_mgr_err.h>
#include "pipe_mgr_mat.h"

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Struct contains the data info of number of bytes and actual data
 * for MatchValueLookupTable.
 */
struct pipe_data_spec {
	uint16_t num_data_bytes; /* Number of bytes of data */
	uint8_t *data_bytes;     /* Bytes of data values */
};

/*
 * Method to add entries to value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param match_spec: key spec for table.
 * @param data_spec: data to be stored in table.
 * @param ent_hdl_p: entry handle ptr.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_ent_add(uint32_t sess_hdl,
				  struct bf_dev_target_t dev_tgt,
				  uint32_t tbl_hdl,
				  struct pipe_tbl_match_spec *match_spec,
				  struct pipe_data_spec *data_spec,
				  uint32_t *ent_hdl_p);

/*
 * Method to delete entries from value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param match_spec: key spec for table.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_ent_del(uint32_t sess_hdl,
				  struct bf_dev_target_t dev_tgt,
				  uint32_t tbl_hdl,
				  struct pipe_tbl_match_spec *match_spec);

/*
 * Method to get entries from value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param ent_hdl: entry handle.
 * @param match_spec: key spec for table.
 * @param data_spec: data to be fetched from table.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_ent_get(uint32_t sess_hdl,
				  struct bf_dev_target_t dev_tgt,
				  uint32_t tbl_hdl,
				  uint32_t ent_hdl,
				  struct pipe_tbl_match_spec *match_spec,
				  struct pipe_data_spec *data_spec);

/*
 * Method to get first entry from value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param match_spec: key spec for table.
 * @param data_spec: data to be fetched from table.
 * @param ent_hdl_p: entry handle ptr.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_ent_get_first(uint32_t sess_hdl,
					struct bf_dev_target_t dev_tgt,
					uint32_t tbl_hdl,
					struct pipe_tbl_match_spec *match_spec,
					struct pipe_data_spec *data_spec,
					uint32_t *ent_hdl_p);

/*
 * Method to get next n entries from value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param cur_match_spec: current key spec.
 * @param n: number of entries to fetch.
 * @param match_specs: keys spec for table.
 * @param data_spec: data to be fetched from table.
 * @param num: number of entries fetched from table.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_ent_get_next_n_by_key(uint32_t sess_hdl,
						struct bf_dev_target_t dev_tgt,
						uint32_t tbl_hdl,
						struct pipe_tbl_match_spec *cur_match_spec,
						uint32_t n,
						struct pipe_tbl_match_spec *match_specs,
						struct pipe_data_spec **data_specs,
						uint32_t *num);

/*
 * Method to get first entry handle for value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param ent_hdl_p: entry handle ptr.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_get_first_ent_handle(uint32_t sess_hdl,
					       struct bf_dev_target_t dev_tgt,
					       uint32_t tbl_hdl,
					       uint32_t *ent_hdl_p);

/*
 * Method to get next n entry handles for value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param ent_hdl: entry handle.
 * @param num_hdl: num of next n handles to get.
 * @param next_ent_hdl: next n entry handles.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_get_next_n_ent_handle(uint32_t sess_hdl,
						struct bf_dev_target_t dev_tgt,
						uint32_t tbl_hdl,
						uint32_t ent_hdl,
						uint32_t num_hdl,
						uint32_t *next_ent_hdl);

/*
 * Method to map match spec with entry handle for value lookup table.
 * @param sess_hdl: session handle.
 * @param dev_tgt: device id and pipe id.
 * @param tbl_hdl: table handle.
 * @param match_spec: key spec for table.
 * @param ent_hdl_p: entry handle ptr.
 * return Status of API call.
 */
int pipe_mgr_value_lookup_match_spec_to_ent_hdl(uint32_t sess_hdl,
						struct bf_dev_target_t dev_tgt,
						uint32_t tbl_hdl,
						struct pipe_tbl_match_spec *match_spec,
						uint32_t *ent_hdl_p);

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* __PIPE_MGR_VALUE_LOOKUP_H__ */

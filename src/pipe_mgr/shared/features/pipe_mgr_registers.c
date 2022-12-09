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
 * @file pipe_mgr_registers.c
 *
 * @description Utilities for registers
 */
#include <pipe_mgr/pipe_mgr_intf.h>
#include "../infra/pipe_mgr_int.h"
#include "pipe_mgr_registers.h"
#include "../dal/dal_registers.h"
#include "../../shared/pipe_mgr_shared_intf.h"
#include "../../core/pipe_mgr_ctx_json.h"

/*!
 * Reads DDR to get the register value.
 *
 * @param dev_tgt device target
 * @param table_name table name
 * @param id indirect register id to be read for stats
 * @param stats buffer to fill stats
 * @return Status of the API call
 *
 */
bf_status_t
pipe_mgr_reg_read_indirect_register_set(dev_target_t dev_tgt,
					const char *name,
					pipe_stful_tbl_hdl_t stful_tbl_hdl,
					pipe_stful_mem_idx_t stful_ent_idx,
					pipe_stful_mem_query_t *stful_query,
					uint32_t pipe_api_flags)
{
	return dal_reg_read_indirect_register_set(dev_tgt, name, stful_ent_idx, stful_query);
}

bf_status_t pipe_mgr_reg_mod_assignable_register_set(dev_target_t dev_tgt,
						     const char *name,
						     pipe_stful_mem_idx_t stful_ent_idx,
						     pipe_stful_mem_spec_t *stful_spec)
{
	return dal_reg_write_assignable_register_set(dev_tgt, name, stful_ent_idx, stful_spec);
}

/*
 * routine to query a stful entry
 */
bf_status_t pipe_stful_ent_query(pipe_sess_hdl_t sess_hdl,
				 dev_target_t dev_tgt,
				 const char *table_name,
				 pipe_stful_tbl_hdl_t stful_tbl_hdl,
				 pipe_stful_mem_idx_t stful_ent_idx,
				 pipe_stful_mem_query_t *stful_query,
				 uint32_t pipe_api_flags)
{
	int status;

	LOG_TRACE("STUB:%s\n", __func__);
	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_reg_read_indirect_register_set(dev_tgt,
							 table_name,
							 stful_tbl_hdl,
							 stful_ent_idx,
							 stful_query,
							 pipe_api_flags);
	if (status) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);
	LOG_TRACE("Exiting %s", __func__);

	return status;
}

/*
 * routine to write a register index
 */
bf_status_t pipe_stful_ent_set(pipe_sess_hdl_t sess_hdl,
			       dev_target_t dev_tgt,
			       const char *table_name,
			       pipe_stful_tbl_hdl_t stful_tbl_hdl,
			       pipe_stful_mem_idx_t stful_ent_idx,
			       pipe_stful_mem_spec_t *stful_spec,
			       uint32_t pipe_api_flags)
{
	int status;

	LOG_TRACE("Entering %s", __func__);

	status = pipe_mgr_api_prologue(sess_hdl, dev_tgt);
	if (status) {
		LOG_ERROR("API prologue failed with err: %d", status);
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}

	status = pipe_mgr_reg_mod_assignable_register_set(dev_tgt,
							  table_name,
							  stful_ent_idx,
							  stful_spec);

	if (status) {
		LOG_TRACE("Exiting %s", __func__);
		return status;
	}
	pipe_mgr_api_epilogue(sess_hdl, dev_tgt);

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

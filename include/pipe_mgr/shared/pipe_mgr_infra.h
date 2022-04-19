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
 * @file pipe_mgr_infra.h
 * @date
 *
 * Definitions for pipeline manager infrastructure interfaces
 */

#ifndef _PIPE_MGR_INFRA_H
#define _PIPE_MGR_INFRA_H

/* OSAL includs */
#include <osdep/p4_sde_osdep.h>

/* Module header files */
/* TODO: Replace bf_ prefix in all file names and typedefs. */
#include <bf_types/bf_types.h>
#include "pipe_mgr_mat.h"

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

enum pipe_hdl_type {
	/* API session handles */
	PIPE_HDL_TYPE_SESSION = 0x00,
	/* Handles for logical table objects */
	PIPE_HDL_TYPE_MAT_TBL = 0x01,   /* Match action table */
#ifdef PIPE_MGR_TEMP_CODE
	PIPE_HDL_TYPE_ADT_TBL = 0x02,   /* Action data table */
#endif
	PIPE_HDL_TYPE_SEL_TBL = 0x03,   /* Selection table */
	PIPE_HDL_TYPE_STAT_TBL = 0x04,  /* Statistics table */
	PIPE_HDL_TYPE_METER_TBL = 0x05, /* Meter, LPF and WRED table */
#ifdef PIPE_MGR_TEMP_CODE
	PIPE_HDL_TYPE_STFUL_TBL = 0x06, /* Stateful table */
	PIPE_HDL_TYPE_COND_TBL = 0x07,  /* Gateway table */
#endif
	/* Handles for logical table entry objects */
	PIPE_HDL_TYPE_MAT_ENT = 0x10,  /* Match action table entry */
#ifdef PIPE_MGR_TEMP_CODE
	PIPE_HDL_TYPE_ADT_ENT = 0x11,  /* Action data table entry */
#endif
	PIPE_HDL_TYPE_SEL_GRP = 0x12,  /* Selection table group entry */
	PIPE_HDL_TYPE_STAT_ENT = 0x13, /* Statistics table entry */
	PIPE_HDL_TYPE_MET_ENT = 0x14,  /* Meter table entry */
#ifdef PIPE_MGR_TEMP_CODE
	PIPE_HDL_TYPE_SFUL_ENT = 0x15, /* Stateful memory table entry */
#endif
#ifdef PIPE_MGR_TEMP_CODE
	/* Handles for other P4 level objects */
	PIPE_HDL_TYPE_ACTION_FN = 0x20, /* P4 action function */
	PIPE_HDL_TYPE_FIELD_LST = 0x21, /* P4 field list */
	PIPE_HDL_TYPE_CALC_ALGO = 0x22, /* P4 calculation algorithm */
	/* Handle for Poor-mans selection table */
	PIPE_HDL_TYPE_POOR_MANS_SEL_TBL = 0x23,
#endif
	/* Reserve one type for invalid type */
	PIPE_HDL_TYPE_INVALID = 0x3F /* Reserved to indicate invalid handle */
};

#define PIPE_SET_HDL_TYPE(hdl, type) (((type) << 24) | (hdl))
#define PIPE_GET_HDL_TYPE(hdl) (((hdl) >> 24) & 0x3F)

/* TODO: How many bits to allocate for pipe? Below Macros need to be changed
 *       based on the number of bits for Pipe.
 */
#define PIPE_SET_HDL_PIPE(hdl, pipe) (((pipe) << 30) | (hdl))
#define PIPE_GET_HDL_PIPE(hdl) (((hdl) >> 30) & 0x3)
#define PIPE_GET_HDL_VAL(hdl) ((hdl) & 0x00FFFFFF)

/*!
 * Flags that can be used in conjunction with API calls
 */
/********************************************
 * CLIENT API
 ********************************************/
/* API to invoke client library registration */
int pipe_mgr_client_init(u32 *sess_hdl);
/* API to invoke client library de-registration */
int pipe_mgr_client_cleanup(u32 def_sess_hdl);
/********************************************
 * Transaction related API
 ********************************************/
/*!
 * Begin a transaction on a session. Only one transaction can be in progress
 * on any given session
 *
 * @param shdl Handle to an active session
 * @param is_atomic If @c true, upon committing the transaction, all changes
 *        will be applied atomically such that a packet being processed will
 *        see either all of the changes or none of the changes.
 * @return Status of the API call
 */
int pipe_mgr_begin_txn(u32 shdl, bool is_atomic);
/*!
 * Verify if all the API requests against the transaction in progress have
 * resources to be committed. This also ends the transaction implicitly
 *
 * @param Handle to an active session
 * @return Status of the API call
 */
int pipe_mgr_verify_txn(u32 shdl);
/*!
 * Abort and rollback all API requests against the transaction in progress
 * This also ends the transaction implicitly
 *
 * @param Handle to an active session
 * @return Status of the API call
 */
int pipe_mgr_abort_txn(u32 shdl);
/*!
 * Abort and rollback all API requests against the transaction in progress
 * This also ends the transaction implicitly
 *
 * @param Handle to an active session
 * @return Status of the API call
 */
int pipe_mgr_commit_txn(u32 shdl, bool hw_sync);

/********************************************
 * Batch related API
 ********************************************/
/*!
 * Begin a batch on a session. Only one batch can be in progress
 * on any given session.  Updates to the hardware will be batch together
 * and delayed until the batch is ended.
 *
 * @param shdl Handle to an active session
 * @return Status of the API call
 */
int pipe_mgr_begin_batch(u32 shdl);
/*!
 * Flush a batch on a session pushing all pending updates to hardware.
 *
 * @param shdl Handle to an active session
 * @return Status of the API call
 */
int pipe_mgr_flush_batch(u32 shdl);
/*!
 * End a batch on a session and push all batched updated to hardware.
 *
 * @param shdl Handle to an active session
 * @return Status of the API call
 */
int pipe_mgr_end_batch(u32 shdl, bool hw_sync);
/*!
 * Helper function for of-tests. Return after all the pending operations
 * for the given session have been completed.
 *
 * @param Handle to an active session
 * @return Status of the API call
 */
int pipe_mgr_complete_operations(u32 shdl);

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _PIPE_MGR_INFRA_H */

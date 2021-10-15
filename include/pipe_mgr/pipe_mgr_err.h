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
 * @file pipe_mgr_err.h
 * @date
 *
 * Error code definitions for pipeline management API
 *
 */

#ifndef _PIPE_MGR_ERR_H
#define _PIPE_MGR_ERR_H

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include "bf_types/bf_types.h"

/**
 * Pipeline management API error codes.
 */
typedef int pipe_status_t;
typedef enum pipe_status {
  /**< Operation successful. */
  PIPE_SUCCESS = BF_SUCCESS,
  PIPE_NOT_READY = BF_NOT_READY,
  /**< No system resources (e.g. malloc failures). */
  PIPE_NO_SYS_RESOURCES = BF_NO_SYS_RESOURCES,
  /**< Incorrect inputs. */
  PIPE_INVALID_ARG = BF_INVALID_ARG,
  PIPE_ALREADY_EXISTS = BF_ALREADY_EXISTS,
  PIPE_COMM_FAIL = BF_HW_COMM_FAIL,
  PIPE_OBJ_NOT_FOUND = BF_OBJECT_NOT_FOUND,
  PIPE_MAX_SESSIONS_EXCEEDED = BF_MAX_SESSIONS_EXCEEDED,
  PIPE_SESSION_NOT_FOUND = BF_SESSION_NOT_FOUND,
  PIPE_NO_SPACE = BF_NO_SPACE,
  /**< Temporarily out of resources, try again later (e.g.  no free DMA buffers,
     FIFOs full, etc.). */
  PIPE_TRY_AGAIN = BF_EAGAIN,
  PIPE_INIT_ERROR = BF_INIT_ERROR,
  /**< API call not supported in transactions. */
  PIPE_TXN_NOT_SUPPORTED = BF_TXN_NOT_SUPPORTED,
  PIPE_TABLE_LOCKED = BF_TABLE_LOCKED,
  PIPE_IO = BF_IO,
  PIPE_UNEXPECTED = BF_UNEXPECTED,
  PIPE_ENTRY_REFERENCES_EXIST = BF_ENTRY_REFERENCES_EXIST,
  PIPE_NOT_SUPPORTED = BF_NOT_SUPPORTED,
  PIPE_LLD_FAILED = BF_HW_UPDATE_FAILED,
  PIPE_NO_LEARN_CLIENTS = BF_NO_LEARN_CLIENTS,
  PIPE_IDLE_UPDATE_IN_PROGRESS = BF_IDLE_UPDATE_IN_PROGRESS,
  PIPE_DEVICE_LOCKED = BF_DEVICE_LOCKED,
  PIPE_INTERNAL_ERROR = BF_INTERNAL_ERROR,
  PIPE_TABLE_NOT_FOUND = BF_TABLE_NOT_FOUND,
} pipe_status_enum;

/* Routine to get error string corresponding to an error code */

static inline const char *pipe_str_err(pipe_status_t sts) {
  return bf_err_str((bf_status_t)sts);
}

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _PIPE_MGR_ERR_H */

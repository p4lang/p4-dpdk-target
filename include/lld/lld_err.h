/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef lld_err_h
#define lld_err_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  LLD_OK = 0,
  LLD_ERR_BAD_PARM = -1,
  LLD_ERR_NOT_READY = -2,
  LLD_ERR_LOCK_FAILED = -3,
  LLD_ERR_DR_FULL = -4,
  LLD_ERR_DR_EMPTY = -5,
  LLD_ERR_INVALID_CFG = -6,
  LLD_ERR_UT = -7,
} lld_err_t;

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // lld_err_h

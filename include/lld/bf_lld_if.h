/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_LLD_IF_H_INCLUDED
#define BF_LLD_IF_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bf_lld_if.h
 * \brief Details Device-level APIs.
 *
 */

/**
 * @addtogroup lld-api
 * @{
 * This is a description of some APIs.
 */

bf_status_t bf_lld_init(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // BF_LLD_IF_H_INCLUDED

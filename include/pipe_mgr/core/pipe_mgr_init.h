/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*!
 * @file pipe_mgr_init.h
 * @date
 *
 * Definitions for pipeline manager init/terminate interfaces
 */
#ifndef _PIPE_MGR_INIT_H
#define _PIPE_MGR_INIT_H

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/********************************************
 * Library init/cleanup API
 ********************************************/
/* API to invoke pipe_mgr initialization */
int pipe_mgr_init(void);
void pipe_mgr_cleanup(void);

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _PIPE_MGR_INIT_H */

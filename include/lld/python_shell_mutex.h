/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _LLD_PYTHON_SHELL_MUTEX_H
#define _LLD_PYTHON_SHELL_MUTEX_H

#include <target-sys/bf_sal/bf_sys_sem.h>

typedef struct py_shell_context_t_ {
  bf_sys_mutex_t python_exclude_mutex;
} py_shell_context_t;

extern py_shell_context_t py_shell_ctx;

static inline bool try_py_shell_lock(void) {
  if (0 == bf_sys_mutex_trylock(&py_shell_ctx.python_exclude_mutex)) {
    return true;
  } else {
    return false;
  }
}
#define INIT_PYTHON_SHL_LOCK() \
  { bf_sys_mutex_init(&py_shell_ctx.python_exclude_mutex); }

#define TRY_PYTHON_SHL_LOCK() try_py_shell_lock();

#define RELEASE_PYTHON_SHL_LOCK() \
  { bf_sys_mutex_unlock(&py_shell_ctx.python_exclude_mutex); }

#endif  //_LLD_PYTHON_SHELL_MUTEX_H

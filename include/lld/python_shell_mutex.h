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

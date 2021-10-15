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

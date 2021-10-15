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
#ifndef BF_DRV_VER_H
#define BF_DRV_VER_H

#include "bf_drv_bld_ver.h"

#define BF_DRV_REL_VER "9.7.0"
#define BF_DRV_VER BF_DRV_REL_VER "-" BF_DRV_BLD_VER

#define BF_DRV_INTERNAL_VER BF_DRV_VER "(" BF_DRV_GIT_VER ")"

#endif /* BF_DRV_VER_H */

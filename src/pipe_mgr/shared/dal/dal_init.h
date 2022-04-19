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
#include <dvm/bf_drv_intf.h>
#include "pipe_mgr/shared/pipe_mgr_infra.h"

int dal_add_device(int dev_id,
		enum bf_dev_family_t dev_family,
		struct bf_device_profile *prof,
		enum bf_dev_init_mode_s warm_init_mode);

int dal_remove_device(int dev_id);

int dal_enable_pipeline(bf_dev_id_t dev_id,
			int profile_id,
			void *spec_file,
			enum bf_dev_init_mode_s warm_init_mode);

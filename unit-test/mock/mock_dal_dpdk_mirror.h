/*
 * Copyright(c) 2022 Intel Corporation.
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
//Mocking rte_swx_ctl_pipeline_mirroring_session_set function which is defined
MOCK_GLOBAL_FUNC3(rte_swx_ctl_pipeline_mirroring_session_set, int(struct rte_swx_pipeline *p, uint32_t id, struct rte_swx_pipeline_mirroring_session_params *mir_params));

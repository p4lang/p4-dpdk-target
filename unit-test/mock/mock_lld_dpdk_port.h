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
//Mocking pipeline_find function which is defined
//in src/lld/dpdk/infra/dpdk_obj.c
MOCK_GLOBAL_FUNC1(pipeline_find, struct pipeline *(const char *name));

//Mocking mempool_find function which is defined
//in src/lld/dpdk/infra/dpdk_obj.c
MOCK_GLOBAL_FUNC1(mempool_find, struct mempool *(const char *name));

//Mocking tap_find function which is defined
//in src/lld/dpdk/infra/dpdk_obj.c
MOCK_GLOBAL_FUNC1(tap_find, struct tap *(const char *name));

//Mocking link_find function which is defined
//in src/lld/dpdk/infra/dpdk_obj.c
MOCK_GLOBAL_FUNC1(link_find, struct link *(const char *name));

//Mocking ring_find function which is defined
//in src/lld/dpdk/infra/dpdk_obj.c
MOCK_GLOBAL_FUNC1(ring_find, struct ring *(const char *name));


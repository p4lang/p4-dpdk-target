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

//Mocking bf_cjson_get_string function which is defined
//in src/ctx_json/ctx_json_utils.c
//
#ifndef MOCK_PIPE_MGR_H
#define MOCK_PIPE_MGR_H

MOCK_GLOBAL_FUNC3(bf_cjson_get_string, int (cJSON *, char*, char**));

MOCK_GLOBAL_FUNC3(bf_cjson_try_get_string, int (cJSON *, char*, char**));

MOCK_GLOBAL_FUNC3(bf_cjson_try_get_int, int (cJSON *, char *, int *));

MOCK_GLOBAL_FUNC3(bf_cjson_get_int, int (cJSON *, char *, int *));

MOCK_GLOBAL_FUNC3(bf_cjson_try_get_bool, int(cJSON *, char *, bool *));

MOCK_GLOBAL_FUNC3(bf_cjson_get_object, int (cJSON *, char *, cJSON **));

MOCK_GLOBAL_FUNC3(bf_cjson_try_get_object, int (cJSON *, char *, cJSON **));

MOCK_GLOBAL_FUNC5(bf_cjson_get_handle, int(int, int, cJSON *, char *, int *));

MOCK_GLOBAL_FUNC6(dal_parse_ctx_json_parse_value_lookup_stage_tables, int(int, int, cJSON *, void **, int *, struct pipe_mgr_value_lookup_ctx *));
#endif

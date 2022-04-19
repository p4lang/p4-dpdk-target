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
MOCK_GLOBAL_FUNC3(bf_cjson_get_string, int (cJSON*, char*, char**));   

//Writing stub for bf_sys_log_and_trace which is invoked using LOG_DBG
//and LOG_ERROR. 
int bf_sys_log_and_trace(int module, int level, const char *format, ...) {
    return 0;
}

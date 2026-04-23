/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
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

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

/*!
 * @file fixed_function_ctx.c
 *
 *
 * Definitions for Fixed Function Context Parsing
 */

/* Global Headers */
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

/* Third-Party */
#include <osdep/p4_sde_osdep_utils.h>
#include <target-utils/id/id.h>
#include <osdep/p4_sde_osdep.h>

/* P4 SDE Headers */
#include "fixed_function/fixed_function_log.h"
#include <ctx_json/ctx_json_utils.h>
#include "fixed_function_ctx.h"

/**
 * Fixed Function Mgr Map Structure
 */
struct fixed_function_mgr_map {
        char ff_name[P4_SDE_NAME_LEN];  /*!< Fixed Function Mgr String */
        enum fixed_function_mgr ff_mgr;            /*!< Fixed Function Mgr Enum */
};

/**
 * Fixed Function Mgr Map
 */
static struct fixed_function_mgr_map ff_mgr_map[] = {
	{"port", FF_MGR_PORT},           /*!< Port Manager */
	{"vport", FF_MGR_VPORT},         /*!< vPort Manager */
	{"crypto", FF_MGR_CRYPTO},       /*!< crypto Manager */
};

/**
 * Fixed Function Table Type Map Structure
 */
struct fixed_function_table_type_map {
        char table_type[P4_SDE_NAME_LEN];  /*!< Fixed Function Table Type String */
        enum fixed_function_table_type tbl_type;            /*!< Fixed Function Table Type Enum */
};

/**
 * Fixed Function Table Type Map
 */
static struct fixed_function_table_type_map ff_table_type_map[] = {
        {"config", FIXED_FUNCTION_TABLE_TYPE_CONFIG},           /*!< Config */
        {"state", FIXED_FUNCTION_TABLE_TYPE_STATE},         /*!< State */
};

/**
 * Get the Fixed Function Mgr Enum from
 * string
 * @param port_type  String
 * @return Port Type Enum
 */
enum fixed_function_mgr get_fixed_function_mgr_enum(char *ff_name)
{
        int num_ff_mgr;
        int i;

        num_ff_mgr = sizeof(ff_mgr_map) /
                        sizeof(struct fixed_function_mgr_map);

        for (i = 0; i < num_ff_mgr; i++) {
                if (strncmp(ff_name, ff_mgr_map[i].ff_name,
                            P4_SDE_NAME_LEN) == 0)
                        return ff_mgr_map[i].ff_mgr;
        }

        LOG_ERROR("Invalid fixed function type:%s", ff_name);
        return FF_MGR_INVALID;
}

/**
 * Get the Fixed Function Mgr Enum from
 * string
 * @param port_type  String
 * @return Port Type Enum
 */
static enum fixed_function_table_type get_fixed_function_table_type_enum(char *table_type)
{
        int num_table_types;
        int i;

        num_table_types = sizeof(ff_table_type_map) /
                        sizeof(struct fixed_function_table_type_map);

        for (i = 0; i < num_table_types; i++) {
                if (strncmp(table_type, ff_table_type_map[i].table_type,
                            P4_SDE_NAME_LEN) == 0)
                        return ff_table_type_map[i].tbl_type;
        }

        LOG_ERROR("Invalid fixed function table type:%s", table_type);
        return FIXED_FUNCTION_TABLE_TYPE_INVALID;
}

static int ctx_json_parse_key_field_json
           (cJSON *key_field_cjson,
            struct fixed_function_key_fields *key)
{
	char *name = NULL;
	int start_bit = 0;
	int bit_width = 0;
	int err = 0;

        err |= bf_cjson_get_string
                (key_field_cjson, CTX_JSON_FIXED_FUNC_KEY_NAME, &name);
	err |= bf_cjson_get_int(key_field_cjson, CTX_JSON_FIXED_FUNC_KEY_START_BIT, &start_bit);
	err |= bf_cjson_get_int(key_field_cjson, CTX_JSON_FIXED_FUNC_KEY_BIT_WIDTH, &bit_width);

        if (err)
                return BF_UNEXPECTED;

        strncpy(key->name, name, P4_SDE_NAME_LEN - 1);
        key->name[P4_SDE_NAME_LEN - 1] = '\0';
	key->start_bit = start_bit;
	key->bit_width = bit_width;

	return BF_SUCCESS;
}

static int ctx_json_parse_data_field_json
           (cJSON *data_field_cjson,
            struct fixed_function_data_fields *data)
{
        char *name = NULL;
        int start_bit = 0;
        int bit_width = 0;
	int err = 0;

        err |= bf_cjson_get_string
                (data_field_cjson, CTX_JSON_FIXED_FUNC_DATA_NAME, &name);
        err |= bf_cjson_get_int(data_field_cjson, CTX_JSON_FIXED_FUNC_DATA_START_BIT, &start_bit);
        err |= bf_cjson_get_int(data_field_cjson, CTX_JSON_FIXED_FUNC_DATA_BIT_WIDTH, &bit_width);

        if (err)
                return BF_UNEXPECTED;

        strncpy(data->name, name, P4_SDE_NAME_LEN - 1);
        data->name[P4_SDE_NAME_LEN - 1] = '\0';
        data->start_bit = start_bit;
        data->bit_width = bit_width;

        return BF_SUCCESS;
}

static int ctx_json_parse_table_json
           (cJSON *table_cjson,
            struct fixed_function_table_ctx *table_ctx)
{
	cJSON *data_fields_cjson = NULL;
	cJSON *key_fields_cjson = NULL;
	char *table_type = NULL;
	int rc = BF_SUCCESS;
	char *name = NULL;
	int size = 0;
	int err = 0;
	int i;

	err |= bf_cjson_get_string
                (table_cjson, CTX_JSON_TABLE_NAME, &name);
        err |= bf_cjson_get_string
                (table_cjson, CTX_JSON_TABLE_TABLE_TYPE, &table_type);
	err |= bf_cjson_try_get_int(table_cjson, CTX_JSON_TABLE_SIZE, &size);

        if (err)
                return BF_UNEXPECTED;

	strncpy(table_ctx->name, name, P4_SDE_NAME_LEN - 1);
	table_ctx->name[P4_SDE_NAME_LEN - 1] = '\0';
	table_ctx->table_type = get_fixed_function_table_type_enum
					(table_type);
	if (table_ctx->table_type == FIXED_FUNCTION_TABLE_TYPE_INVALID)
		return BF_INVALID_ARG;
	table_ctx->size = size;

        err |= bf_cjson_get_object
                (table_cjson, CTX_JSON_FIXED_FUNC_KEY_FIELDS, &key_fields_cjson);
	if (err)
		return BF_INVALID_ARG;

	table_ctx->key_fields_count = cJSON_GetArraySize(key_fields_cjson);
	table_ctx->key_fields = P4_SDE_CALLOC
			(table_ctx->key_fields_count, sizeof(*(table_ctx->key_fields)));

	if (!table_ctx->key_fields)
		return BF_NO_SYS_RESOURCES;

        for (i = 0; i < table_ctx->key_fields_count; i++) {
                cJSON *key_field_cjson = NULL;
                err |= bf_cjson_get_array_item(key_fields_cjson, i, &key_field_cjson);
                if (err) {
                        rc = BF_UNEXPECTED;
                        goto key_cleanup;
                }
                err |= ctx_json_parse_key_field_json(key_field_cjson, &table_ctx->key_fields[i]);
                if (err) {
                        rc = BF_UNEXPECTED;
                        goto key_cleanup;
                }
        }

        err |= bf_cjson_get_object
                (table_cjson, CTX_JSON_FIXED_FUNC_DATA_FIELDS, &data_fields_cjson);
        if (err) {
		rc = BF_UNEXPECTED;
                goto key_cleanup;
	}
        table_ctx->data_fields_count = cJSON_GetArraySize(data_fields_cjson);
        table_ctx->data_fields = P4_SDE_CALLOC
                        (table_ctx->data_fields_count, sizeof(*(table_ctx->data_fields)));

        if (!table_ctx->data_fields) {
                rc = BF_NO_SYS_RESOURCES;
		goto key_cleanup;
	}

        for (i = 0; i < table_ctx->data_fields_count; i++) {
                cJSON *data_field_cjson = NULL;
                err |= bf_cjson_get_array_item(data_fields_cjson, i, &data_field_cjson);
                if (err) {
                        rc = BF_UNEXPECTED;
                        goto data_cleanup;
                }
                err |= ctx_json_parse_data_field_json(data_field_cjson, &table_ctx->data_fields[i]);
                if (err) {
                        rc = BF_UNEXPECTED;
                        goto data_cleanup;
                }
        }

	return rc;

data_cleanup:
	if (table_ctx->data_fields)
		P4_SDE_FREE(table_ctx->data_fields);
key_cleanup:
	if (table_ctx->key_fields)
		P4_SDE_FREE(table_ctx->key_fields);

	return rc;
}

static int ctx_json_parse_tables_json
           (cJSON *root,
            struct fixed_function_mgr_ctx *ctx)
{
	cJSON *tables_cjson = NULL;
	int rc = BF_SUCCESS;
	int err = 0;
	int i;

        err |= bf_cjson_get_object
                (root, CTX_JSON_TABLES_NODE, &tables_cjson);

        if (err)
                return rc;

	ctx->num_tables = cJSON_GetArraySize(tables_cjson);
	ctx->table_ctx = P4_SDE_CALLOC(ctx->num_tables, sizeof(*(ctx->table_ctx)));

	if (!ctx->table_ctx)
		return BF_NO_SYS_RESOURCES;

	for (i = 0; i < ctx->num_tables; i++) {
		cJSON *table_cjson = NULL;
		err |= bf_cjson_get_array_item(tables_cjson, i, &table_cjson);
		err |= ctx_json_parse_table_json(table_cjson, &ctx->table_ctx[i]);
		if (err) {
			rc = BF_UNEXPECTED;
			goto table_cleanup;
		}
	}

	return rc;

table_cleanup:
	if (ctx->table_ctx)
		P4_SDE_FREE(ctx->table_ctx);
	return rc;
}

struct fixed_function_mgr_ctx *parse_fixed_function_json(int dev_id, char *ctx_file)
{
	struct fixed_function_mgr_ctx *ctx = NULL;
	char *ctx_file_buffer;
	struct stat stat_b;
	size_t to_allocate;
	size_t num_items;
	cJSON *root;
	FILE *file;
	int fd;
	int rc;

	ctx = P4_SDE_CALLOC(1, sizeof(*ctx));
	if (!ctx) {
		LOG_ERROR("%s: Could not allocate memory for fixed function ctx"
				, __func__);
		return NULL;
	}
	file = fopen(ctx_file, "r");
        if (!file) {
                LOG_ERROR("%s: Could not open configuration file: %s.\n",
                          __func__, ctx_file);
                goto ctx_file_fopen_err;
        }


        fd = fileno(file);
        fstat(fd, &stat_b);
        to_allocate = stat_b.st_size + 1;
        ctx_file_buffer = P4_SDE_CALLOC(1, to_allocate);
        if (!ctx_file_buffer) {
                LOG_ERROR
                        ("%s: Could not allocate memory for "
                         "config file buffer.",
                         __func__);
                goto ctx_file_buffer_alloc_err;
        }
        num_items = fread(ctx_file_buffer, stat_b.st_size, 1, file);
        if (num_items != 1) {
                if (ferror(file)) {
                        LOG_ERROR
                                ("%s: Error reading config file buffer",
                                 __func__);
                        goto ctx_file_fread_err;
                }
                LOG_DBG("%s: End of file reached before expected.",
                        __func__);
        }

        root = cJSON_Parse(ctx_file_buffer);
        if (!root) {
                LOG_ERROR("%s:%d: cJSON error while parsing ctx file.",
                          __func__,
                          __LINE__);
                goto cjson_parse_err;
        }
        rc = ctx_json_parse_tables_json(root, ctx);
        if (rc)
                goto table_parse_err;

	return ctx;

table_parse_err:
        cJSON_Delete(root);

cjson_parse_err:
ctx_file_fread_err:
        P4_SDE_FREE(ctx_file_buffer);
ctx_file_buffer_alloc_err:
        fclose(file);
ctx_file_fopen_err:
        if (ctx)
                P4_SDE_FREE(ctx);
        return NULL;
}

/**
 * Import Fixed Function Context File
 * @param dev_id The Device ID
 * @param dev_profile Device Profile
 * @return Status of the API call
 */
bf_status_t fixed_function_ctx_import(int dev_id,
			      struct bf_device_profile *dev_profile)
{
	struct bf_fixed_function_s *fixed_function = NULL;
	struct fixed_function_mgr_ctx *ff_mgr_ctx = NULL;
	bf_status_t status = BF_SUCCESS;
	char *json_file_path = NULL;
	int i = 0;

        LOG_TRACE("Enter %s", __func__);

        for (i = 0; i < dev_profile->num_fixed_functions; i++) {
                fixed_function = &dev_profile->fixed_functions[i];
                if (!fixed_function) {
                        LOG_ERROR
                        ("No Fixed Function for dev %d",
                        dev_id);
                        LOG_TRACE
                        ("Exit %s", __func__);
                        return BF_INVALID_ARG;
                }

                json_file_path = fixed_function->ctx_json;
                LOG_TRACE("%s: device %u, file %s",
                                   __func__,
                                   dev_id,
                                   json_file_path);

		if (json_file_path) {
			ff_mgr_ctx = parse_fixed_function_json(dev_id,
                                                               json_file_path);
			if (!ff_mgr_ctx) {
				LOG_ERROR
					("%s : Error in parsing fixed function context file",
					 __func__);
				LOG_TRACE("Exit %s", __func__);
				return BF_NO_SYS_RESOURCES;
			}
		} else {
				LOG_ERROR(" %s : Fixed function context json file path is"
					  " null", __func__);
				LOG_TRACE("Exit %s", __func__);
                                return BF_UNEXPECTED;
		}

		ff_mgr_ctx->ff_mgr = get_fixed_function_mgr_enum
					(fixed_function->name);
		if (ff_mgr_ctx->ff_mgr == FF_MGR_INVALID) {
			P4_SDE_FREE(ff_mgr_ctx);
			LOG_TRACE("Exit %s", __func__);
			return BF_INVALID_ARG;
		}

		status = fixed_function_ctx_map_add(ff_mgr_ctx);
		if (status) {
			P4_SDE_FREE(ff_mgr_ctx);
			LOG_ERROR("%s : Error in adding fixed function ctx to map",
					__func__);
			LOG_TRACE("Exit %s", __func__);
			return BF_NO_SYS_RESOURCES;
		}
        }

        LOG_TRACE("Exit %s", __func__);
        return BF_SUCCESS;
}

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

/* Global Headers */
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

/* P4 SDE Headers */
#include <cjson/cJSON.h>
#include <osdep/p4_sde_osdep_utils.h>
#include <target-utils/id/id.h>
#include <ctx_json/ctx_json_utils.h>
#include <osdep/p4_sde_osdep.h>

/* Local Headers */
#include "pipe_mgr_log.h"
/* TODO: Avoid pipe_mgr_int.h and use only pipe_mgr_shared_intf.h.
 *	 Move the required structures in pipe_mgr_int.h to
 *	 pipe_mgr_shared_intf.h
 */
#include "../shared/infra/pipe_mgr_int.h"
#include "../shared/pipe_mgr_shared_intf.h"
#include "dal/dal_parse.h"

#define NOACTION_STR "NoAction"
#define PIPE_MGR_TABLE_SIZE_DEFAULT 256

char *trim_classifier_str(char *str) {
	char *temp_p = NULL;

	temp_p = strrchr(str, '.');
	temp_p = temp_p? (temp_p + 1) : str;

	return temp_p;
}

struct pipe_mgr_match_type_map {
	char match_type_name[P4_SDE_NAME_LEN];
	enum pipe_mgr_match_type match_type;
};

static struct pipe_mgr_match_type_map match_type_map[] = {
	{"exact", PIPE_MGR_MATCH_TYPE_EXACT},
	{"ternary", PIPE_MGR_MATCH_TYPE_TERNARY},
	{"lpm", PIPE_MGR_MATCH_TYPE_LPM}
};

static enum pipe_mgr_match_type get_match_type_enum(char *match_type)
{
	int num_match_type;
	int i;

	num_match_type = sizeof(match_type_map)/
			  sizeof(struct pipe_mgr_match_type_map);

	for (i = 0; i < num_match_type; i++) {
		if (strncmp(match_type, match_type_map[i].match_type_name,
		    P4_SDE_NAME_LEN) == 0)
			return match_type_map[i].match_type;
	}

	LOG_ERROR("Invalid action:%s", match_type);
	return PIPE_MGR_MATCH_TYPE_INVALID;
}

static int ctx_json_parse_version(cJSON *root)
{
	char *version = NULL;
	int err = 0;

	err = bf_cjson_get_string(root, CTX_JSON_COMPILER_VERSION, &version);
	if (err)
		return BF_OBJECT_NOT_FOUND;

	LOG_DBG("Compiler version %s\n", version);
	return BF_SUCCESS;
}

static int ctx_json_compile_command(cJSON *root, struct pipe_mgr_p4_pipeline
				    *pkg_ctx)
{
	char *comp_cmd = NULL, *str, *token;
	int err = 0;

	err = bf_cjson_get_string(root, CTX_JSON_COMPILE_COMMAND, &comp_cmd);
	if (err)
		return BF_OBJECT_NOT_FOUND;

	str = strstr(comp_cmd, "--arch");
	token = strtok(str, " ");
	token = strtok(NULL, " ");
	if (token)
		strncpy(pkg_ctx->arch_name, token, 3);

	return BF_SUCCESS;
}

struct pipe_mgr_target_map {
	char target_name[P4_SDE_NAME_LEN];
	enum pipe_mgr_target target_type;
};

static struct pipe_mgr_target_map target_map[] = {
	{"DPDK", PIPE_MGR_TARGET_DPDK}
};

static int ctx_json_parse_target(cJSON *root,
		struct pipe_mgr_p4_pipeline *pkg_ctx)
{
	int num_target_type;
	char *target = NULL;
	int err = 0;
	int i;

	err = bf_cjson_get_string(root, CTX_JSON_CHIP_TARGET, &target);
	if (err) {
		LOG_ERROR("target get failed");
		return BF_OBJECT_NOT_FOUND;
	}

	num_target_type = sizeof(target_map)/sizeof(struct pipe_mgr_target_map);

	for (i = 0; i < num_target_type; i++) {
		if (strncmp(target, target_map[i].target_name,
					P4_SDE_NAME_LEN) == 0) {
			pkg_ctx->target = target_map[i].target_type;
		return BF_SUCCESS;
		}
	}

	LOG_ERROR("Invaild target type = %s", target);
	return BF_UNEXPECTED;
}

/*!
 * structure for pipe_mgr_byte_order_map
 */
struct pipe_mgr_byte_order_map {
	char byte_order_name[P4_SDE_NAME_LEN];
	enum pipe_mgr_byte_order byte_order;
};

/*!
 * structure for pipe_mgr_byte_order_map for Network and Host
 */
static struct pipe_mgr_byte_order_map byte_order_map[] = {
	{"NETWORK", PIPE_MGR_BYTE_ORDER_NETWORK},
	{"HOST", PIPE_MGR_BYTE_ORDER_HOST}
};

/**
 * Get byte order enum from byte order char pointer API.
 *
 * @param  byte_order        byte order in string format.
 * @return                   byte order in enum format
 */
static enum pipe_mgr_byte_order get_byte_order_enum(char *byte_order)
{
	int num_byte_order;
	int i;

	num_byte_order = sizeof(byte_order_map)/
			  sizeof(struct pipe_mgr_byte_order_map);

	for (i = 0; i < num_byte_order; i++) {
		if (strncmp(byte_order, byte_order_map[i].byte_order_name,
		    P4_SDE_NAME_LEN) == 0)
			return byte_order_map[i].byte_order;
	}

	LOG_ERROR("Invalid byte order:%s", byte_order);
	return PIPE_MGR_BYTE_ORDER_INVALID;
}

static int ctx_json_parse_mat_key_fields_json
	   (int dev_id, int prof_id,
	    cJSON *mat_key_fields_cjson,
	    struct pipe_mgr_match_key_fields *mat_key_fields)
{
	char *instance_name;
	int bit_width;
	int position;
	char *match_type;
	char *field_name;
	bool is_valid;
	int err = 0;
	char *name;

	err |= bf_cjson_get_string
		(mat_key_fields_cjson,
		 CTX_JSON_MATCH_KEY_FIELDS_NAME,
		 &name);

	err |= bf_cjson_get_int
		(mat_key_fields_cjson,
		 CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH,
		 &bit_width);

	err |= bf_cjson_try_get_int
		(mat_key_fields_cjson,
		 CTX_JSON_MATCH_KEY_FIELDS_POSITION,
		 &position);

	err |= bf_cjson_get_string
		(mat_key_fields_cjson,
		 CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE,
		 &match_type);

	err |= bf_cjson_try_get_bool
		(mat_key_fields_cjson,
		 CTX_JSON_MATCH_KEY_FIELDS_IS_VALID,
		 &is_valid);

	err |= bf_cjson_get_string
		(mat_key_fields_cjson,
		 CTX_JSON_MATCH_KEY_FIELDS_INSTANCE_NAME,
		 &instance_name);

	err |= bf_cjson_get_string
		(mat_key_fields_cjson,
		 CTX_JSON_MATCH_KEY_FIELDS_FIELD_NAME,
		 &field_name);

	if (err)
		return BF_UNEXPECTED;

	strncpy(mat_key_fields->name, name, P4_SDE_NAME_LEN - 1);
	mat_key_fields->name[P4_SDE_NAME_LEN - 1] = '\0';

	mat_key_fields->bit_width = bit_width;
	mat_key_fields->position = position;
	mat_key_fields->match_type = get_match_type_enum(match_type);
	mat_key_fields->is_valid = is_valid;

	strncpy(mat_key_fields->instance_name, instance_name,
		P4_SDE_NAME_LEN - 1);
	mat_key_fields->instance_name[P4_SDE_NAME_LEN - 1] = '\0';
	strncpy(mat_key_fields->field_name, field_name, P4_SDE_NAME_LEN - 1);
	mat_key_fields->field_name[P4_SDE_NAME_LEN - 1] = '\0';
	return BF_SUCCESS;
}

/**
 * Match Key Format parsing API.
 *
 * @param  dev_id                Device ID.
 * @param  prof_id               Profile ID.
 * @param  mat_key_format_cjson  Pointer to Match key format context.
 * @param  mat_key_fields		 Pointer to match key fields.
 * @param  mat_key_fields_count	 Match key field count.
 * @param  mat_key_format		 Pointer to match key format to be returned.
 * @return                       Status of the API call
 */
static int ctx_json_parse_mat_key_format_json
	   (int dev_id, int prof_id,
	    cJSON *mat_key_format_cjson,
	    struct pipe_mgr_match_key_fields *mat_key_fields,
	    int mat_key_fields_count,
	    struct pipe_mgr_match_key_format *mat_key_format)
{
	int match_key_handle;
	int byte_array_index;
	int start_bit_offset;
	char *byte_order;
	int bit_width;
	int err = 0;
	int i = 0;

	err |= bf_cjson_get_int
		(mat_key_format_cjson,
		 CTX_JSON_MATCH_KEY_FORMAT_KEY_HANDLE,
		 &match_key_handle);

	err |= bf_cjson_get_string
		(mat_key_format_cjson,
		 CTX_JSON_MATCH_KEY_FORMAT_BYTE_ORDER,
		 &byte_order);

	err |= bf_cjson_try_get_int
		(mat_key_format_cjson,
		 CTX_JSON_MATCH_KEY_FORMAT_BYTE_ARRAY_INDEX,
		 &byte_array_index);

	err |= bf_cjson_get_int
		(mat_key_format_cjson,
		 CTX_JSON_MATCH_KEY_FORMAT_START_BIT_OFFSET,
		 &start_bit_offset);

	err |= bf_cjson_get_int
		(mat_key_format_cjson,
		 CTX_JSON_MATCH_KEY_FORMAT_BIT_WIDTH,
		 &bit_width);

	if (err)
		return BF_UNEXPECTED;

	err = get_byte_order_enum(byte_order);

	if (err == PIPE_MGR_BYTE_ORDER_INVALID)
		return BF_UNEXPECTED;

	for (i = 0; i < match_key_handle; i++)
		mat_key_fields = mat_key_fields->next;

	mat_key_format->match_key_field_ref = mat_key_fields;
	mat_key_format->match_key_handle = match_key_handle;
	mat_key_format->byte_array_index = byte_array_index;
	mat_key_format->start_bit_offset = start_bit_offset;
	mat_key_format->byte_order = err;
	mat_key_format->bit_width = bit_width;
	return BF_SUCCESS;
}

static int ctx_json_parse_p4_parameters_json
		(int dev_id, int prof_id,
		cJSON *p4_parameter_cjson,
		struct pipe_mgr_p4_parameters *p4_para)
{
	bf_status_t rc = BF_SUCCESS;
	int start_bit;
	int bit_width;
	int position;
	char *name = NULL;
	int err = 0;

	err |= bf_cjson_get_string
		(p4_parameter_cjson,
		CTX_JSON_P4_PARAMETER_NAME, &name);

	err |= bf_cjson_try_get_int
		(p4_parameter_cjson,
		CTX_JSON_ACTION_P4_PARAMETER_START_BIT,
		&start_bit);

	err |= bf_cjson_try_get_int
		(p4_parameter_cjson,
		CTX_JSON_ACTION_P4_PARAMETER_POSITION,
		&position);

	err |= bf_cjson_get_int
		(p4_parameter_cjson,
		CTX_JSON_ACTION_P4_PARAMETER_BIT_WIDTH,
		&bit_width);

	if (err)
		return BF_UNEXPECTED;

	strncpy(p4_para->name, name, P4_SDE_NAME_LEN - 1);
	p4_para->name[P4_SDE_NAME_LEN - 1] = '\0';
	p4_para->start_bit = start_bit;
	p4_para->position = position;
	p4_para->bit_width = bit_width;

	return rc;
}

static int ctx_json_parse_mat_actions_json
	   (int dev_id, int prof_id,
	    cJSON *actions_cjson,
	    struct pipe_mgr_actions_list *action)
{
	struct pipe_mgr_p4_parameters *p4_para_temp;
	bool allowed_as_default_action = 0;
	bool is_compiler_added_action = 0;
	bool constant_default_action = 0;
	bool allowed_as_hit_action = 0;
	bf_status_t rc = BF_SUCCESS;
	char *action_name = NULL;
	char *target_action_name = NULL;
	char *name = NULL;
	int err = 0;
	int handle;

	err |= bf_cjson_get_string(actions_cjson,
			CTX_JSON_ACTION_NAME, &name);

	err |= bf_cjson_try_get_string(actions_cjson,
			CTX_JSON_TARGET_ACTION_NAME, &target_action_name);

	err |= bf_cjson_get_int(actions_cjson,
			CTX_JSON_ACTION_HANDLE,
			&handle);

	err |= bf_cjson_try_get_bool(actions_cjson,
			CTX_JSON_ACTION_ALLOWED_AS_HIT_ACTION,
			&allowed_as_hit_action);

	err |= bf_cjson_try_get_bool(actions_cjson,
			CTX_JSON_ACTION_ALLOWED_AS_DEFAULT_ACTION,
			&allowed_as_default_action);

	err |= bf_cjson_try_get_bool(actions_cjson,
			CTX_JSON_ACTION_IS_COMPILER_ADDED_ACTION,
			&is_compiler_added_action);

	err |= bf_cjson_try_get_bool(actions_cjson,
			CTX_JSON_ACTION_IS_CONSTANT_ACTION,
			&constant_default_action);

	if (err)
		return BF_UNEXPECTED;

	action_name = trim_classifier_str(name);
	if (!action_name) {
		LOG_ERROR("action %s trim_classifier_str failed", name);
		return BF_UNEXPECTED;
	}
	strncpy(action->name, action_name, P4_SDE_NAME_LEN - 1);
	action->name[P4_SDE_NAME_LEN - 1] = '\0';

	if (target_action_name) {
		action_name = trim_classifier_str(target_action_name);
		if (!action_name) {
			LOG_ERROR("trg_act_name %s trim_classifier_str failed"
				  , name);
			return BF_UNEXPECTED;
		}
		strncpy(action->target_action_name, action_name,
			P4_SDE_NAME_LEN - 1);
		action->target_action_name[P4_SDE_NAME_LEN - 1] = '\0';
	}

	action->handle = handle;
	action->allowed_as_hit_action = allowed_as_hit_action;
	action->allowed_as_default_action = allowed_as_default_action;
	action->is_compiler_added_action = is_compiler_added_action;
	action->constant_default_action = constant_default_action;

	cJSON *p4_parameters_cjson = NULL;

	err |= bf_cjson_try_get_object
				(actions_cjson,
				CTX_JSON_ACTION_P4_PARAMETERS,
				&p4_parameters_cjson);
	if (!p4_parameters_cjson)
		return rc;

	cJSON *p4_parameter_cjson = NULL;

	CTX_JSON_FOR_EACH(p4_parameter_cjson, p4_parameters_cjson) {
		p4_para_temp = P4_SDE_CALLOC
					(1,
					sizeof(*p4_para_temp));
		if (!p4_para_temp) {
			rc = BF_NO_SYS_RESOURCES;
			goto cleanup_p4_para;
		}

		rc |= ctx_json_parse_p4_parameters_json
				(dev_id, prof_id,
				 p4_parameter_cjson, p4_para_temp);
		if (rc) {
			rc = BF_UNEXPECTED;
			P4_SDE_FREE(p4_para_temp);
			goto cleanup_p4_para;
		}

		if (action->p4_parameters_list)
			p4_para_temp->next = action->p4_parameters_list;
		action->p4_parameters_list = p4_para_temp;
		action->p4_parameters_count++;
	}

	return rc;

cleanup_p4_para:
	PIPE_MGR_FREE_LIST(action->p4_parameters_list);
	action->p4_parameters_list = NULL;
	return rc;
}

static int ctx_json_parse_mat_adt_json
	   (int dev_id, int prof_id,
	    cJSON *adt_cjson,
	    struct action_data_table_refs *adt)
{
	char *table_name = NULL;
	char *how_ref = NULL;
	char *name = NULL;
	int handle = 0;
	int err = 0;

	err |= bf_cjson_get_string(adt_cjson,
			CTX_JSON_TABLE_NAME,
			&name);

	err |= bf_cjson_get_int(adt_cjson, CTX_JSON_INDIRECT_RESOURCE_HANDLE,
			&handle);

	err |= bf_cjson_try_get_string(adt_cjson,
			CTX_JSON_TABLE_HOW_REFERENCED,
			&how_ref);

	if (err)
		return BF_UNEXPECTED;

	table_name = trim_classifier_str(name);
	if (!table_name) {
		LOG_ERROR("table %s trim_classifier_str failed", name);
		return BF_UNEXPECTED;
	}

	strncpy(adt->name, table_name, P4_SDE_NAME_LEN - 1);
	adt->name[P4_SDE_NAME_LEN - 1] = '\0';
	if (how_ref) {
		strncpy(adt->how_referenced, how_ref, P4_SDE_NAME_LEN - 1);
		adt->how_referenced[P4_SDE_NAME_LEN - 1] = '\0';
	}
	adt->handle = handle;

	return BF_SUCCESS;
}

static int ctx_json_parse_mat_sel_json
	   (int dev_id, int prof_id,
	    cJSON *sel_cjson,
	    struct selection_table_refs *sel)
{
	char *table_name = NULL;
	char *how_ref = NULL;
	char *name = NULL;
	int handle = 0;
	int err = 0;

	err |= bf_cjson_get_string(sel_cjson,
			CTX_JSON_TABLE_NAME,
			&name);

	err |= bf_cjson_get_int(sel_cjson, CTX_JSON_INDIRECT_RESOURCE_HANDLE,
			&handle);

	err |= bf_cjson_try_get_string(sel_cjson,
			CTX_JSON_TABLE_HOW_REFERENCED,
			&how_ref);

	if (err)
		return BF_UNEXPECTED;

	table_name = trim_classifier_str(name);
	if (!table_name) {
		LOG_ERROR("table %s trim_classifier_str failed", name);
		return BF_UNEXPECTED;
	}

	strncpy(sel->name, table_name, P4_SDE_NAME_LEN - 1);
	sel->name[P4_SDE_NAME_LEN - 1] = '\0';
	if (how_ref) {
		strncpy(sel->how_referenced, how_ref, P4_SDE_NAME_LEN - 1);
		sel->how_referenced[P4_SDE_NAME_LEN - 1] = '\0';
	}
	sel->handle = handle;

	return BF_SUCCESS;
}

static int pipe_mgr_match_attr_set_match_type
		(struct pipe_mgr_mat_ctx *mat_ctx)
{
	struct pipe_mgr_match_key_fields *match_fields;
	int match_fields_count;
	int i;

	match_fields = mat_ctx->mat_key_fields;
	match_fields_count = mat_ctx->mat_key_fields_count;

	if (!match_fields)
		return BF_SUCCESS;

	/*
	 * TODO : Table with multiple match kind support
	 *
	 */
	for (i = 0; i < match_fields_count; i++) {
		if (match_fields->match_type == PIPE_MGR_MATCH_TYPE_TERNARY) {
			mat_ctx->match_attr.match_type =
				PIPE_MGR_MATCH_TYPE_TERNARY;
			return BF_SUCCESS;
		}

		if (match_fields->match_type == PIPE_MGR_MATCH_TYPE_LPM) {
                        mat_ctx->match_attr.match_type =
                                PIPE_MGR_MATCH_TYPE_LPM;
                        return BF_SUCCESS;
                }

		match_fields = match_fields->next;
	}

	mat_ctx->match_attr.match_type = PIPE_MGR_MATCH_TYPE_EXACT;
	return BF_SUCCESS;
}

static int ctx_json_parse_mat_match_attribute_json
	   (int dev_id, int prof_id,
	    cJSON *match_attribute_cjson,
	    struct pipe_mgr_mat_ctx *mat_ctx)
{
	struct pipe_mgr_match_attribute *match_attr;
	bf_status_t rc = BF_SUCCESS;
	int err = 0;

	match_attr = &mat_ctx->match_attr;

	rc = pipe_mgr_match_attr_set_match_type(mat_ctx);
	if (rc != BF_SUCCESS)
		return BF_UNEXPECTED;

	cJSON *stage_table_cjson = NULL;

	err |= bf_cjson_get_object(match_attribute_cjson,
			CTX_JSON_MATCH_ATTRIBUTES_STAGE_TABLES,
			&stage_table_cjson);
	if (err)
		return BF_UNEXPECTED;

	rc |= dal_parse_ctx_json_parse_stage_tables
		(dev_id, prof_id, stage_table_cjson,
		 &match_attr->stage_table,
		 &match_attr->stage_table_count,
		 mat_ctx);
	if (rc)
		return BF_UNEXPECTED;

	return rc;
}

static int ctx_json_parse_match_table_json
	   (int dev_id, int prof_id,
	    cJSON *table_cjson,
	    struct pipe_mgr_mat_ctx *mat_ctx)
{
	struct pipe_mgr_match_key_fields *mat_key_fields_temp = NULL;
	struct pipe_mgr_match_key_fields *mat_key_fields_head = NULL;
	struct pipe_mgr_match_key_format *mat_key_format_temp = NULL;
	struct pipe_mgr_actions_list *actions_temp;
	struct action_data_table_refs *adt_temp;
	struct selection_table_refs *sel_temp;
	bool idle_timeout_auto_delete = false;
	cJSON *match_key_formats_cjson = NULL;
	cJSON *match_key_format_cjson = NULL;
	cJSON *match_key_fields_cjson = NULL;
	cJSON *match_key_field_cjson = NULL;
	cJSON *match_attribute_cjson = NULL;
	cJSON *actions_list_cjson = NULL;
	char *target_table_name = NULL;
	bool is_resource_controllable;
	cJSON *adt_list_cjson = NULL;
	cJSON *sel_list_cjson = NULL;
	bf_status_t rc = BF_SUCCESS;
	cJSON *actions_cjson = NULL;
	int default_action_handle;
	bool add_on_miss = false;
	cJSON *adt_cjson = NULL;
	cJSON *sel_cjson = NULL;
	char *table_name = NULL;
	char *direction = NULL;
	int adt_handle = 0;
	char *name = NULL;
	bool uses_range;
	int handle = 0;
	int size = 0;
	int err = 0;
	int i = 0;

	err |= bf_cjson_get_string
		(table_cjson, CTX_JSON_TABLE_NAME, &name);
	err |= bf_cjson_try_get_string
		(table_cjson, CTX_JSON_TARGET_TABLE_NAME,
		 &target_table_name);
	err |= bf_cjson_try_get_string(table_cjson,
			CTX_JSON_TABLE_DIRECTION, &direction);
	err |= bf_cjson_get_handle
		(dev_id, prof_id,
		 table_cjson, CTX_JSON_TABLE_HANDLE, &handle);
	err |= bf_cjson_try_get_handle
		(dev_id, prof_id,
		 table_cjson,
		 CTX_JSON_SELECTION_TABLE_BOUND_TO_ACTION_DATA_TABLE_HANDLE,
		 &adt_handle);
	err |= bf_cjson_try_get_int(table_cjson, CTX_JSON_TABLE_SIZE, &size);
	if (!size)
		size = PIPE_MGR_TABLE_SIZE_DEFAULT;
	err |= bf_cjson_try_get_bool
		(table_cjson, CTX_JSON_TABLE_ADD_ON_MISS,
		 &add_on_miss);
	err |= bf_cjson_try_get_bool
		(table_cjson, CTX_JSON_TABLE_IDLE_TIMEOUT_AUTO_DELETE,
		 &idle_timeout_auto_delete);
	err |= bf_cjson_try_get_handle(dev_id,
			prof_id,
			table_cjson,
			CTX_JSON_MATCH_TABLE_DEFAULT_ACTION_HANDLE,
			&default_action_handle);
	err |= bf_cjson_try_get_bool
		(table_cjson, CTX_JSON_MATCH_TABLE_IS_RESOURCE_CONTROLLABLE,
		 &is_resource_controllable);
	err |= bf_cjson_try_get_bool
		(table_cjson,
		 CTX_JSON_MATCH_TABLE_USES_RANGE, &uses_range);

	if (err)
		return BF_UNEXPECTED;

	table_name = trim_classifier_str(name);
	if (!table_name) {
		LOG_ERROR("table %s trim_classifier_str failed", name);
		return BF_UNEXPECTED;
	}
	strncpy(mat_ctx->name, table_name, P4_SDE_NAME_LEN - 1);
	mat_ctx->name[P4_SDE_NAME_LEN - 1] = '\0';

	if (target_table_name) {
		table_name = trim_classifier_str(target_table_name);
		if (!table_name) {
			LOG_ERROR("target_table_name %s trim_str failed",
					target_table_name);
			return BF_UNEXPECTED;
		}
		strncpy(mat_ctx->target_table_name, table_name,
				P4_SDE_NAME_LEN - 1);
		mat_ctx->target_table_name[P4_SDE_NAME_LEN - 1] = '\0';
	}

	if (direction) {
		strncpy(mat_ctx->direction, direction, P4_SDE_NAME_LEN - 1);
		mat_ctx->direction[P4_SDE_NAME_LEN - 1] = '\0';
	}
	mat_ctx->handle = handle;
	mat_ctx->adt_handle = adt_handle;
	mat_ctx->size = size;
	mat_ctx->add_on_miss = add_on_miss;
	mat_ctx->idle_timeout_auto_delete = idle_timeout_auto_delete;
	mat_ctx->is_resource_controllable = is_resource_controllable;

	err |= bf_cjson_try_get_object
		(table_cjson, CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
		 &match_key_fields_cjson);

	if (!match_key_fields_cjson)
		goto skip_match_tbl_attribute;

	CTX_JSON_FOR_EACH(match_key_field_cjson, match_key_fields_cjson) {
		mat_key_fields_temp =
			P4_SDE_CALLOC
			(1, sizeof(*mat_key_fields_temp));
		if (!mat_key_fields_temp) {
			rc = BF_NO_SYS_RESOURCES;
			goto cleanup_mat_key_fields;
		}

		rc |= ctx_json_parse_mat_key_fields_json
			(dev_id, prof_id,
			 match_key_field_cjson,
			 mat_key_fields_temp);
		if (rc) {
			rc = BF_UNEXPECTED;
			P4_SDE_FREE(mat_key_fields_temp);
			goto cleanup_mat_key_fields;
		}

		if (mat_ctx->mat_key_fields)
			mat_ctx->mat_key_fields->next =
				mat_key_fields_temp;
		else
			mat_key_fields_head = mat_key_fields_temp;

		mat_ctx->mat_key_fields = mat_key_fields_temp;
		mat_ctx->mat_key_fields_count++;
	}

	mat_ctx->mat_key_fields = mat_key_fields_head;

	err |= bf_cjson_try_get_object
		(table_cjson, CTX_JSON_MATCH_TABLE_MATCH_KEY_FORMAT,
		 &match_key_formats_cjson);

	if (!match_key_formats_cjson)
		goto skip_match_key_format;

	mat_ctx->mat_key_format_count =
		cJSON_GetArraySize(match_key_formats_cjson);

	mat_key_format_temp =
		P4_SDE_CALLOC(mat_ctx->mat_key_format_count,
				sizeof(*mat_key_format_temp));
	if (!mat_key_format_temp) {
		rc = BF_NO_SYS_RESOURCES;
		goto cleanup_mat_key_format;
	}

	mat_ctx->mat_key_num_bytes = 0;
	CTX_JSON_FOR_EACH(match_key_format_cjson,
			match_key_formats_cjson) {
		rc |= ctx_json_parse_mat_key_format_json
			(dev_id, prof_id,
			 match_key_format_cjson,
			 mat_ctx->mat_key_fields,
			 mat_ctx->mat_key_fields_count,
			 &mat_key_format_temp[i]);
		if (rc) {
			rc = BF_UNEXPECTED;
			goto cleanup_mat_key_format_failure;
		}
		i++;
	}

	/* buffer length is last elemets byte_array_index +
	   bit_width */
	mat_ctx->mat_key_num_bytes =
		mat_key_format_temp[i - 1].byte_array_index +
		(mat_key_format_temp[i - 1].bit_width >> 3) +
		(mat_key_format_temp[i - 1].bit_width % 8 ? 1 : 0);


	mat_ctx->mat_key_format = mat_key_format_temp;

	mat_ctx->match_key_buffer =
		P4_SDE_CALLOC(1, mat_ctx->mat_key_num_bytes);
	if(!mat_ctx->match_key_buffer) {
		LOG_ERROR("Match Key Buffer allocation error");
		rc = BF_NO_SYS_RESOURCES;
		goto cleanup_mat_key_format_failure;
	}

	mat_ctx->match_key_mask_buffer =
		P4_SDE_CALLOC(1, mat_ctx->mat_key_num_bytes);
	if(!mat_ctx->match_key_mask_buffer) {
		LOG_ERROR("Match Key Buffer allocation error");
		rc = BF_NO_SYS_RESOURCES;
		goto cleanup_mat_key_mask_buffer;
	}

skip_match_key_format:
	err |= bf_cjson_try_get_object
		(table_cjson,
		 CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
		 &match_attribute_cjson);

	if (err) {
		rc = BF_UNEXPECTED;
		goto cleanup_mat_key_mask_buffer;
	}

	if (match_attribute_cjson) {
		rc = ctx_json_parse_mat_match_attribute_json
			(dev_id, prof_id,
			 match_attribute_cjson,
			 mat_ctx);
		if (rc) {
			rc = BF_UNEXPECTED;
			goto cleanup_mat_key_mask_buffer;
		}
	}

	err |= bf_cjson_try_get_object
		(table_cjson,
		 CTX_JSON_MATCH_TABLE_ACTION_DATA_TABLE_REFS,
		 &adt_list_cjson);

	if (adt_list_cjson) {
		CTX_JSON_FOR_EACH(adt_cjson, adt_list_cjson) {
			adt_temp = P4_SDE_CALLOC(1, sizeof(*adt_temp));
			if (!adt_temp) {
				rc = BF_NO_SYS_RESOURCES;
				goto cleanup_mat_adt;
			}

			rc |= ctx_json_parse_mat_adt_json
				(dev_id, prof_id,
				 adt_cjson, adt_temp);
			if (rc) {
				rc = BF_UNEXPECTED;
				P4_SDE_FREE(adt_temp);
				goto cleanup_mat_adt;
			}

			if (mat_ctx->adt)
				adt_temp->next = mat_ctx->adt;
			mat_ctx->adt = adt_temp;
			mat_ctx->adt_count++;
		}
	}

	err |= bf_cjson_try_get_object
		(table_cjson,
		 CTX_JSON_MATCH_TABLE_SELECTION_TABLE_REFS,
		 &sel_list_cjson);

	if (sel_list_cjson) {
		CTX_JSON_FOR_EACH(sel_cjson, sel_list_cjson) {
			sel_temp = P4_SDE_CALLOC(1, sizeof(*sel_temp));
			if (!sel_temp) {
				rc = BF_NO_SYS_RESOURCES;
				goto cleanup_mat_sel;
			}

			rc |= ctx_json_parse_mat_sel_json
				(dev_id, prof_id,
				 sel_cjson, sel_temp);
			if (rc) {
				rc = BF_UNEXPECTED;
				P4_SDE_FREE(sel_temp);
				goto cleanup_mat_sel;
			}

			if (mat_ctx->sel_tbl)
				sel_temp->next = mat_ctx->sel_tbl;
			mat_ctx->sel_tbl = sel_temp;
			mat_ctx->sel_tbl_count++;
		}
	}

skip_match_tbl_attribute:

	err |= bf_cjson_try_get_object(table_cjson,
			CTX_JSON_MATCH_TABLE_ACTIONS,
			&actions_list_cjson);

	if (actions_list_cjson) {
		CTX_JSON_FOR_EACH(actions_cjson, actions_list_cjson) {
			actions_temp = P4_SDE_CALLOC(1, sizeof(*actions_temp));
			if (!actions_temp) {
				rc = BF_NO_SYS_RESOURCES;
				goto cleanup_mat_actions;
			}

			rc |= ctx_json_parse_mat_actions_json
				(dev_id, prof_id,
				 actions_cjson, actions_temp);
			if (rc) {
				rc = BF_UNEXPECTED;
				P4_SDE_FREE(actions_temp);
				goto cleanup_mat_actions;
			}

			if (mat_ctx->actions)
				actions_temp->next = mat_ctx->actions;
			mat_ctx->actions = actions_temp;
			mat_ctx->action_count++;
		}
	}

	LOG_DBG
		("%s:%d: Parsing RMT configuration for"
		 " match table handle 0x%x and "
		 "name \"%s\".",
		 __func__,
		 __LINE__,
		 handle,
		 name);

	mat_ctx->duplicate_entry_check = 1;
	return rc;


	/* TODO: all these goto statement  are bug and can create a
	   heap corrurption,
	   it is not noticed because on error we exit all our stack and
	   core dump, but this has to be corrected in every pipe manager call*/
cleanup_mat_actions:
	PIPE_MGR_FREE_LIST(mat_ctx->actions);
	mat_ctx->actions = NULL;
cleanup_mat_sel:
	PIPE_MGR_FREE_LIST(mat_ctx->sel_tbl);
	mat_ctx->sel_tbl = NULL;
cleanup_mat_adt:
	PIPE_MGR_FREE_LIST(mat_ctx->adt);
	mat_ctx->adt = NULL;
cleanup_mat_key_mask_buffer:
	P4_SDE_FREE(mat_ctx->match_key_buffer);
	mat_ctx->match_key_buffer = NULL;
cleanup_mat_key_format_failure:
	P4_SDE_FREE(mat_key_format_temp);
cleanup_mat_key_fields:
	mat_ctx->mat_key_fields = mat_key_fields_head;
	PIPE_MGR_FREE_LIST(mat_ctx->mat_key_fields);
	mat_ctx->mat_key_fields = NULL;
cleanup_mat_key_format:
	mat_ctx->mat_key_format = NULL;
	return rc;
}

static int alloc_mat_state(int dev_id, struct pipe_mgr_mat_state *mat_state)
{
	unsigned int num_pipelines;
	int status;

	mat_state->entry_handle_array =
		P4_SDE_ID_INIT(ENTRY_HANDLE_ARRAY_SIZE, false);
	if (!mat_state->entry_handle_array)
		return BF_NO_SYS_RESOURCES;
	if (P4_SDE_MAP_INIT(&mat_state->entry_info_htbl))
		return BF_NO_SYS_RESOURCES;
	if (P4_SDE_MUTEX_INIT(&mat_state->lock))
		return BF_NO_SYS_RESOURCES;

	status = pipe_mgr_get_num_profiles(dev_id, &num_pipelines);
	if (status != BF_SUCCESS)
		return status;
	if (num_pipelines < 1) {
		LOG_ERROR("Invalid number of pipelines: %d", num_pipelines);
		return BF_UNEXPECTED;
	}
	mat_state->key_htbl = (bf_hashtable_t **)P4_SDE_CALLOC(num_pipelines,
						sizeof(bf_hashtable_t *));
	if (!mat_state->key_htbl)
		return BF_NO_SYS_RESOURCES;
	mat_state->num_htbls = num_pipelines;
	return BF_SUCCESS;
}

static int
ctx_json_parse_value_lookup_match_attribute_json(int dev_id, int prof_id,
						 cJSON *match_attribute_cjson,
						 struct pipe_mgr_value_lookup_ctx *val_lookup_ctx)
{
	struct pipe_mgr_match_attribute *match_attr;
	bf_status_t rc = BF_SUCCESS;
	char *match_type = NULL;
	int err = 0;

	match_attr = &val_lookup_ctx->match_attr;

	err |= bf_cjson_try_get_string(match_attribute_cjson,
				       CTX_JSON_MATCH_ATTRIBUTES_MATCH_TYPE,
				       &match_type);
	if (err)
		return BF_UNEXPECTED;
	if (match_type)
		match_attr->match_type = get_match_type_enum(match_type);

	cJSON *stage_table_cjson = NULL;

	err |= bf_cjson_get_object(match_attribute_cjson,
				   CTX_JSON_MATCH_ATTRIBUTES_STAGE_TABLES,
				   &stage_table_cjson);
	if (err)
		return BF_UNEXPECTED;

	rc |= dal_parse_ctx_json_parse_value_lookup_stage_tables
		(dev_id, prof_id, stage_table_cjson,
		 &match_attr->stage_table,
		 &match_attr->stage_table_count,
		 val_lookup_ctx);
	if (rc)
		return BF_UNEXPECTED;

	return rc;
}

static int ctx_json_parse_value_lookup_table_json(int dev_id, int prof_id,
						  cJSON *table_cjson,
						  struct pipe_mgr_value_lookup_ctx *val_lookup_ctx)
{
	struct pipe_mgr_match_key_fields *key_fields_temp;
	cJSON *match_attribute_cjson = NULL;
	cJSON *key_fields_cjson = NULL;
	cJSON *key_field_cjson = NULL;
	bf_status_t rc = BF_SUCCESS;
	char *tgt_tbl_name = NULL;
	char *tbl_name = NULL;
	char *name = NULL;
	int handle = 0;
	int size = 0;
	int err = 0;

	err |= bf_cjson_get_handle(dev_id, prof_id,
				   table_cjson, CTX_JSON_TABLE_HANDLE, &handle);
	err |= bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
	err |= bf_cjson_try_get_string(table_cjson, CTX_JSON_TARGET_TABLE_NAME,
				       &tgt_tbl_name);
	err |= bf_cjson_try_get_int(table_cjson, CTX_JSON_TABLE_SIZE, &size);
	if (!size)
		size = PIPE_MGR_TABLE_SIZE_DEFAULT;

	if (err)
		return BF_UNEXPECTED;

	tbl_name = trim_classifier_str(name);
	if (!tbl_name) {
		LOG_ERROR("table %s trim_classifier_str failed", name);
		return BF_UNEXPECTED;
	}
	strncpy(val_lookup_ctx->name, tbl_name, P4_SDE_NAME_LEN - 1);
	val_lookup_ctx->name[P4_SDE_NAME_LEN - 1] = '\0';

	if (tgt_tbl_name) {
		tbl_name = trim_classifier_str(tgt_tbl_name);
		if (!tbl_name) {
			LOG_ERROR("target_table_name %s trim_str failed", tgt_tbl_name);
			return BF_UNEXPECTED;
		}
		strncpy(val_lookup_ctx->target_table_name, tbl_name, P4_SDE_NAME_LEN - 1);
		val_lookup_ctx->target_table_name[P4_SDE_NAME_LEN - 1] = '\0';
	}

	val_lookup_ctx->handle = handle;
	val_lookup_ctx->size = size;

	err |= bf_cjson_try_get_object
		(table_cjson, CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
		&key_fields_cjson);

	CTX_JSON_FOR_EACH(key_field_cjson, key_fields_cjson) {
		key_fields_temp = P4_SDE_CALLOC(1, sizeof(*key_fields_temp));
		if (!key_fields_temp) {
			rc = BF_NO_SYS_RESOURCES;
			goto cleanup_key_fields;
		}

		rc |= ctx_json_parse_mat_key_fields_json(dev_id, prof_id,
							 key_field_cjson,
							 key_fields_temp);
		if (rc) {
			rc = BF_UNEXPECTED;
			P4_SDE_FREE(key_fields_temp);
			goto cleanup_key_fields;
		}

		if (val_lookup_ctx->key_fields)
			key_fields_temp->next = val_lookup_ctx->key_fields;
		val_lookup_ctx->key_fields = key_fields_temp;
		val_lookup_ctx->key_fields_count++;
	}

	err |= bf_cjson_try_get_object(table_cjson,
				       CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
				       &match_attribute_cjson);

	if (err) {
		rc = BF_UNEXPECTED;
		goto cleanup_key_fields;
	}

	if (match_attribute_cjson) {
		rc = ctx_json_parse_value_lookup_match_attribute_json(dev_id, prof_id,
								      match_attribute_cjson,
								      val_lookup_ctx);
		if (rc) {
			rc = BF_UNEXPECTED;
			goto cleanup_key_fields;
		}
	}
	val_lookup_ctx->duplicate_entry_check = 1;

cleanup_key_fields:
	PIPE_MGR_FREE_LIST(val_lookup_ctx->key_fields);
	val_lookup_ctx->key_fields = NULL;
	return rc;
}

static int ctx_json_parse_extern(int dev_id,
			         int prof_id,
		                 cJSON *root,
		                 struct pipe_mgr_p4_pipeline *ctx)
{
	return dal_ctx_json_parse_extern(dev_id,
				         prof_id,
					 root,
					 ctx);
}
/* This funtion can be used to establish the inter table relations.
 */
static int post_parse_processing
	   (int dev_id, int prof_id,
	    cJSON *root,
	    struct pipe_mgr_p4_pipeline *ctx)
{
	return dal_post_parse_processing(dev_id, prof_id,ctx);
}


static int ctx_json_parse_tables_json
	   (int dev_id, int prof_id,
	    cJSON *root,
	    struct pipe_mgr_p4_pipeline *ctx)
{
	struct pipe_mgr_value_lookup *value_lookup_temp = NULL;
	struct pipe_mgr_mat *mat_temp = NULL;
	int rc = BF_SUCCESS;
	int err = 0;
	cJSON *tables_cjson = NULL;
	err |= bf_cjson_get_object
		(root, CTX_JSON_TABLES_NODE, &tables_cjson);

	if (err)
		return rc;

	cJSON *table_cjson = NULL;
	CTX_JSON_FOR_EACH(table_cjson, tables_cjson) {
		char *table_type = NULL;
		err |= bf_cjson_get_string
				(table_cjson,
				 CTX_JSON_TABLE_TABLE_TYPE, &table_type);
		if (err)
			return BF_UNEXPECTED;

		if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_MATCH) ||
			!strcmp(table_type, CTX_JSON_TABLE_TYPE_ACTION_DATA) ||
			!strcmp(table_type, CTX_JSON_TABLE_TYPE_SELECTION)) {

			mat_temp =
				P4_SDE_CALLOC
					(1,
					sizeof(*mat_temp));
			if (!mat_temp) {
				rc = BF_NO_SYS_RESOURCES;
				goto mat_tbl_cleanup;
			}
			rc |= ctx_json_parse_match_table_json
					(dev_id, prof_id,
					 table_cjson, &mat_temp->ctx);
			if (rc) {
				rc = BF_UNEXPECTED;
				goto mat_tbl_cleanup;
			}
			mat_temp->ctx.store_entries =
				pipe_mgr_mat_store_entries(&mat_temp->ctx);
			if (mat_temp->ctx.store_entries) {
				mat_temp->state = P4_SDE_CALLOC
						(1,
						 sizeof(*mat_temp->state));
				if (!mat_temp->state) {
					rc = BF_NO_SYS_RESOURCES;
					goto mat_tbl_cleanup;
				}
				rc = alloc_mat_state(dev_id, mat_temp->state);
				if (rc)
					goto mat_tbl_cleanup;
			}
			if (ctx->mat_tables)
				mat_temp->next = ctx->mat_tables;
			ctx->mat_tables = mat_temp;
			ctx->num_mat_tables++;
		} else if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_VALUE_LOOKUP)) {
			value_lookup_temp = P4_SDE_CALLOC(1, sizeof(*value_lookup_temp));
			if (!value_lookup_temp) {
				rc = BF_NO_SYS_RESOURCES;
				goto value_lookup_tbl_cleanup;
			}

			rc |= ctx_json_parse_value_lookup_table_json(dev_id, prof_id,
								     table_cjson,
								     &value_lookup_temp->ctx);
			if (rc) {
				rc = BF_UNEXPECTED;
				goto value_lookup_tbl_cleanup;
			}

			value_lookup_temp->ctx.store_entries = true;
			if (value_lookup_temp->ctx.store_entries) {
				value_lookup_temp->state =
					P4_SDE_CALLOC(1, sizeof(*value_lookup_temp->state));
				if (!value_lookup_temp->state) {
					rc = BF_NO_SYS_RESOURCES;
					goto value_lookup_tbl_cleanup;
				}
				rc = alloc_mat_state(dev_id, value_lookup_temp->state);
				if (rc)
					goto value_lookup_tbl_cleanup;
			}

			if (ctx->value_lookup_tables)
				value_lookup_temp->next = ctx->value_lookup_tables;
			ctx->value_lookup_tables = value_lookup_temp;
			ctx->num_value_lookup_tables++;
		}
	}
	return rc;

value_lookup_tbl_cleanup:
	/* Free the node that is not in the list. */
	if (value_lookup_temp && value_lookup_temp->state) {
		pipe_mgr_free_mat_state(value_lookup_temp->state);
		P4_SDE_FREE(value_lookup_temp);
	} else if (value_lookup_temp) {
		P4_SDE_FREE(value_lookup_temp);
	}
mat_tbl_cleanup:
	/* Free the node that is not in the list. */
	if (mat_temp && mat_temp->state) {
		pipe_mgr_free_mat_state(mat_temp->state);
		P4_SDE_FREE(mat_temp);
	} else if (mat_temp) {
		P4_SDE_FREE(mat_temp);
	}
	pipe_mgr_free_mat_table(ctx);
	return rc;
}

/* bf-driver DB is flat and keyed off profile-id
 * Profile DB has program and p4_pipeline hierarchy
 * This API gets the p4-program DB based on the profile index
 */
static struct bf_p4_program *pipe_mgr_get_p4_program_profile_db
	(struct bf_device_profile *inp_profile,
	 uint32_t index)
{
	struct bf_p4_program *prog;
	uint32_t count = 0;
	int p = 0;
	int q = 0;

	for (p = 0; p < inp_profile->num_p4_programs; p++) {
		prog = &inp_profile->p4_programs[p];
		for (q = 0; q < prog->num_p4_pipelines; q++) {
			if (count == index)
				return prog;
			count++;
		}
	}

	return NULL;
}

/* bf-driver DB is flat and keyed off profile-id
 * Profile DB has program and p4_pipeline hierarchy
 * This API gets the p4-pipeline DB based on the profile index
 */
static struct bf_p4_pipeline *pipe_mgr_get_p4_pipeline_profile_db
	(struct bf_device_profile *inp_profile, uint32_t index)
{
	struct bf_p4_pipeline *pipeline = NULL;
	struct bf_p4_program *prog;
	uint32_t count = 0;
	int p = 0;
	int q = 0;

	for (p = 0; p < inp_profile->num_p4_programs; p++) {
		prog = &inp_profile->p4_programs[p];
		for (q = 0; q < prog->num_p4_pipelines; q++) {
			pipeline = &prog->p4_pipelines[q];
			if (count == index)
				return pipeline;
			count++;
		}
	}

	return NULL;
}

static struct pipe_mgr_p4_pipeline *parse_ctx_json
	      (int dev_id,
	       int profile_id,
	       char *ctx_file)
{
	struct pipe_mgr_p4_pipeline *pkg_ctx;
	char *ctx_file_buffer;
	struct stat stat_b;
	size_t to_allocate;
	size_t num_items;
	FILE *file;
	cJSON *root;
	int fd;
	int rc;

	LOG_DBG("Parsing context json file: %s.", ctx_file);
	pkg_ctx = P4_SDE_CALLOC(1, sizeof(*pkg_ctx));
	if (!pkg_ctx) {
		LOG_ERROR
			("%s:%d: Could not allocate memory for "
			 "pkg_ctx.",
			 __func__,
			 __LINE__);
		goto ctx_file_fopen_err;
	}
	file = fopen(ctx_file, "r");
	if (!file) {
		LOG_ERROR("%s:%d: Could not open configuration file: %s.\n",
			  __func__,
			  __LINE__,
			  ctx_file);
		goto ctx_file_fopen_err;
	}
	fd = fileno(file);
	fstat(fd, &stat_b);
	to_allocate = stat_b.st_size + 1;
	ctx_file_buffer = P4_SDE_CALLOC(1, to_allocate);
	if (!ctx_file_buffer) {
		LOG_ERROR
			("%s:%d: Could not allocate memory for "
			 "config file buffer.",
			 __func__,
			 __LINE__);
		goto ctx_file_buffer_alloc_err;
	}
	num_items = fread(ctx_file_buffer, stat_b.st_size, 1, file);
	if (num_items != 1) {
		if (ferror(file)) {
			LOG_ERROR
				("%s:%d: Error reading config file buffer",
				 __func__,
				 __LINE__);
			goto ctx_file_fread_err;
		}
		/* TODO: Add assert support in p4_sde_osdep.h and add here.
		 * PIPE_MGR_ASSERT(feof(file));
		 */
		LOG_DBG("%s:%d: End of file reached before expected.",
			__func__,
			__LINE__);
	}
	root = cJSON_Parse(ctx_file_buffer);
	if (!root) {
		LOG_ERROR("%s:%d: cJSON error while parsing ctx file.",
			  __func__,
			  __LINE__);
		goto cjson_parse_err;
	}
	rc = ctx_json_parse_version(root);
	if (rc) {
		LOG_ERROR
			("%s:%d: Failed to parse version from ContextJSON",
			 __func__, __LINE__);
		goto version_parse_err;
	}
	rc = ctx_json_parse_target(root, pkg_ctx);
	if (rc) {
		LOG_ERROR
			("%s:%d: Failed to parse version from ContextJSON",
			 __func__, __LINE__);
		goto version_parse_err;
	}
	if (pkg_ctx->target == PIPE_MGR_TARGET_DPDK) {
		rc = ctx_json_compile_command(root, pkg_ctx);
		if (rc) {
			LOG_ERROR
			("%s:%d: Failed to parse compile_cmd from ContextJSON",
			 __func__, __LINE__);
			goto version_parse_err;
		}
	}
	rc = ctx_json_parse_extern(dev_id, profile_id, root, pkg_ctx);

	if (rc)
		goto version_parse_err;
	rc = ctx_json_parse_tables_json(dev_id, profile_id, root, pkg_ctx);
	if (rc)
		goto version_parse_err;
	rc = post_parse_processing(dev_id, profile_id, root, pkg_ctx);
	if (rc)
		goto version_parse_err;
	cJSON_Delete(root);
	P4_SDE_FREE(ctx_file_buffer);
	fclose(file);
	return pkg_ctx;
version_parse_err:
	cJSON_Delete(root);
cjson_parse_err:
ctx_file_fread_err:
	P4_SDE_FREE(ctx_file_buffer);
ctx_file_buffer_alloc_err:
	fclose(file);
ctx_file_fopen_err:
	if (pkg_ctx)
		P4_SDE_FREE(pkg_ctx);
	return NULL;
}

/* This function parses context JSON describing the tables in a
 * given device;
 */
int pipe_mgr_ctx_import(int dev_id,
			struct bf_device_profile *inp_profile,
			enum bf_dev_init_mode_s warm_init_mode)
{

	struct pipe_mgr_p4_pipeline *parsed_pipe_ctx;
	struct bf_p4_pipeline *p4_pipeline = NULL;
	struct bf_p4_program *p4_program = NULL;
	char *json_file_path;
	int profile_id = 0;
	uint32_t num_profiles;
	int status;
	uint32_t p;

	LOG_TRACE("Enter %s", __func__);

	status = pipe_mgr_get_num_profiles(dev_id, &num_profiles);

	if (status) {

		LOG_TRACE("Exit %s", __func__);
		return status;
	}

	/* Process the context for all profiles */
	for (p = 0; p < num_profiles; p++) {
		if (inp_profile) {

			p4_program = pipe_mgr_get_p4_program_profile_db
					(inp_profile, p);
			if (!p4_program) {
				LOG_ERROR
				("No P4-program for profile index %d dev %d",
				 p, dev_id);
				LOG_TRACE("Exit %s", __func__);

				return BF_INVALID_ARG;
			}
			p4_pipeline = pipe_mgr_get_p4_pipeline_profile_db
					(inp_profile, p);

			if (!p4_pipeline) {
				LOG_ERROR
				("No P4-pipeline for profile index %d, dev %d",
				 p,
				 dev_id);
				LOG_TRACE("Exit %s", __func__);

				return BF_INVALID_ARG;
			}
		} else {

			p4_program = NULL;
			p4_pipeline = NULL;
		}

		if (p4_pipeline) {

			json_file_path = p4_pipeline->runtime_context_file;
			LOG_TRACE("%s: device %u, file %s, profile id %d ",
				  __func__,
				  dev_id,
				  json_file_path,
				  p);

			parsed_pipe_ctx = NULL;
			if (!json_file_path && warm_init_mode ==
					BF_DEV_WARM_INIT_FAST_RECFG) {
				LOG_ERROR(" %s : context json file path is"
						" null", __func__);

				return BF_UNEXPECTED;
			}

			if (json_file_path) {

				parsed_pipe_ctx = parse_ctx_json(dev_id,
						profile_id,
						json_file_path);

				if (!parsed_pipe_ctx) {

					LOG_ERROR("%s : Error in parsing the"
							" ctx file",
							__func__);
					LOG_TRACE("Exit %s", __func__);
					return BF_NO_SYS_RESOURCES;
				}

			}

			status = pipe_mgr_set_profile(dev_id, p, p4_program,
						      p4_pipeline,
						      parsed_pipe_ctx);

			if (parsed_pipe_ctx)
				P4_SDE_FREE(parsed_pipe_ctx);
		}

	}
	LOG_TRACE("Exit %s", __func__);
	return BF_SUCCESS;
}

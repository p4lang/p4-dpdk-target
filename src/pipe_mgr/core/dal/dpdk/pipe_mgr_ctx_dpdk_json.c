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

/* P4 SDE Headers */
#include <cjson/cJSON.h>
#include <ctx_json/ctx_json_utils.h>
#include <osdep/p4_sde_osdep.h>

/* Local Headers */
#include "../../pipe_mgr_log.h"
/* TODO: Avoid pipe_mgr_int.h and use only pipe_mgr_shared_intf.h.
 *       Move the required structures in pipe_mgr_int.h to
 *	 pipe_mgr_shared_intf.h
 */
#include "../../../shared/infra/pipe_mgr_int.h"
#include "../../../shared/pipe_mgr_shared_intf.h"
#include "../../../shared/dal/dpdk/pipe_mgr_dpdk_int.h"
#include "../../pipe_mgr_ctx_json.h"
#include "../dal_parse.h"

#define FREE_LIST(head) \
	{ \
		void *temp;\
		while (head) { \
			temp = head->next; \
			P4_SDE_FREE(head); \
			head = temp; \
		} \
	} \

/* TODO: currently only one profile is supported
 * once multiple profiles supported then
 * this has to be array
 */
struct pipe_mgr_dpdk_stage_table *stage_tbl_head;


static int ctx_json_parse_immediate_field_dpdk_json
		(int dev_id, int prof_id,
		cJSON *immediate_field_cjson,
		struct pipe_mgr_dpdk_immediate_fields *immediate_field)
{
	bf_status_t rc = BF_SUCCESS;
	char *param_type = NULL;
	int param_shift = 0;
	char *param_name;
	int dest_start;
	int dest_width;
	int err = 0;

	err |= bf_cjson_get_string
		(immediate_field_cjson,
		CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_PARAM_NAME,
		&param_name);

	err |= bf_cjson_try_get_string
		(immediate_field_cjson,
		CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_PARAM_TYPE,
		&param_type);

	err |= bf_cjson_try_get_int
		(immediate_field_cjson,
		CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_PARAM_SHIFT,
		&param_shift);

	err |= bf_cjson_get_int
		(immediate_field_cjson,
		CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_DEST_START,
		&dest_start);

	err |= bf_cjson_get_int
		(immediate_field_cjson,
		CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_DEST_WIDTH,
		&dest_width);

	if (err)
		return BF_UNEXPECTED;

	strncpy(immediate_field->param_name,
		param_name, P4_SDE_NAME_LEN - 1);
	immediate_field->param_name[P4_SDE_NAME_LEN-1] = '\0';

	if (param_type) {
		strncpy(immediate_field->param_type,
			param_type, P4_SDE_NAME_LEN - 1);
		immediate_field->param_type[P4_SDE_NAME_LEN-1] = '\0';
	}

	immediate_field->param_shift = param_shift;
	immediate_field->dest_start = dest_start;
	immediate_field->dest_width = dest_width;

	return rc;
}

static int ctx_json_parse_action_format_dpdk_json
		(int dev_id, int prof_id,
		cJSON *action_format_cjson,
		struct pipe_mgr_dpdk_action_format *act_fmt)
{
	struct pipe_mgr_dpdk_immediate_fields *immediate_field_temp;
	bf_status_t rc = BF_SUCCESS;
	char *action_str = NULL;
	char *action_name, *target_action_name;
	int action_handle;
	int err = 0;

	err |= bf_cjson_get_string(action_format_cjson,
			CTX_JSON_ACTION_FORMAT_ACTION_NAME,
			&action_name);

	err |= bf_cjson_get_string(action_format_cjson,
			CTX_JSON_ACTION_FORMAT_TARGET_ACTION_NAME,
			&target_action_name);

	err |= bf_cjson_get_int(action_format_cjson,
			CTX_JSON_ACTION_FORMAT_ACTION_HANDLE,
			&action_handle);

	if (err)
		return BF_UNEXPECTED;

	action_str = trim_classifier_str(action_name);
	if (!action_str) {
		LOG_ERROR("action %s trim_classifier_str failed", action_name);
		return BF_UNEXPECTED;
	}

	strncpy(act_fmt->action_name, action_str, P4_SDE_NAME_LEN - 1);
	act_fmt->action_name[P4_SDE_NAME_LEN - 1] = '\0';

	action_str = trim_classifier_str(target_action_name);
	if (!action_str) {
		LOG_ERROR("action %s trim_classifier_str failed",
			  target_action_name);
		return BF_UNEXPECTED;
	}

	strncpy(act_fmt->target_action_name, action_str, P4_SDE_NAME_LEN - 1);
	act_fmt->target_action_name[P4_SDE_NAME_LEN - 1] = '\0';

	act_fmt->action_handle = action_handle;

	cJSON *immediate_fields_list_cjson = NULL;

	bf_cjson_try_get_object(action_format_cjson,
				CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS,
				&immediate_fields_list_cjson);

	if (immediate_fields_list_cjson) {
		cJSON *immediate_field_cjson = NULL;

		CTX_JSON_FOR_EACH(immediate_field_cjson,
				  immediate_fields_list_cjson) {
			immediate_field_temp =
				P4_SDE_CALLOC
					(1,
					sizeof(*immediate_field_temp));
			if (!immediate_field_temp) {
				rc = BF_NO_SYS_RESOURCES;
				goto cleanup_immediate_field;
			}

			rc |= ctx_json_parse_immediate_field_dpdk_json
					(dev_id, prof_id,
					immediate_field_cjson,
					immediate_field_temp);
			if (rc) {
				rc = BF_UNEXPECTED;
				P4_SDE_FREE(immediate_field_temp);
				goto cleanup_immediate_field;
			}

			if (act_fmt->immediate_field)
				immediate_field_temp->next =
					act_fmt->immediate_field;
			act_fmt->immediate_field = immediate_field_temp;
			act_fmt->immediate_fields_count++;
		}
	}
	return rc;

cleanup_immediate_field:
	FREE_LIST(act_fmt->immediate_field);
	act_fmt->immediate_field = NULL;
	return rc;
}

int ctx_json_parse_mat_mat_attr_stage_tables(int dev_id, int prof_id,
	cJSON *stage_table_list_cjson,
	void **stage_table,
	int *stage_table_count,
	struct pipe_mgr_mat_ctx *mat_ctx)
{
	struct pipe_mgr_dpdk_action_format *action_format_temp = NULL;
	struct pipe_mgr_dpdk_stage_table *stage_tbl_temp = NULL;
	bf_status_t rc = BF_SUCCESS;
	int err = 0;

	cJSON *stage_table_cjson = NULL;

	CTX_JSON_FOR_EACH(stage_table_cjson,
			  stage_table_list_cjson) {

		cJSON *action_format_list_cjson = NULL;

		err |= bf_cjson_get_object(stage_table_cjson,
				CTX_JSON_STAGE_TABLE_ACTION_FORMAT,
				&action_format_list_cjson);
		if (err) {
			rc = BF_UNEXPECTED;
			goto cleanup;
		}

		stage_tbl_temp = P4_SDE_CALLOC(1, sizeof(*stage_tbl_temp));
		if (!stage_tbl_temp) {
			rc =  BF_NO_SYS_RESOURCES;
			goto cleanup;
		}


		if (stage_tbl_head)
			stage_tbl_temp->next = stage_tbl_head;
		stage_tbl_head = stage_tbl_temp;
		(*stage_table_count)++;

		cJSON *action_format_cjson = NULL;

		CTX_JSON_FOR_EACH(action_format_cjson,
				  action_format_list_cjson) {
			action_format_temp =
				P4_SDE_CALLOC(1, sizeof(*action_format_temp));
			if (!action_format_temp) {
				rc = BF_NO_SYS_RESOURCES;
				goto cleanup;
			}

			rc |= ctx_json_parse_action_format_dpdk_json
				(dev_id, prof_id,
				 action_format_cjson, action_format_temp);
			if (rc) {
				rc = BF_UNEXPECTED;
				P4_SDE_FREE(action_format_temp);
				goto cleanup;
			}

			if (stage_tbl_temp->act_fmt)
				action_format_temp->next =
					stage_tbl_temp->act_fmt;
			stage_tbl_temp->act_fmt = action_format_temp;
			stage_tbl_temp->action_format_count++;
		}
	}

	/* Indirect tables don't have stage tables in context json,
	 * allocate one stage table structure.
	 * This stage table will be used for storing dpdk table metadata
	 */
	if(*stage_table_count == 0) {
		stage_tbl_head = P4_SDE_CALLOC(1, sizeof(*stage_tbl_temp));
		if (!stage_tbl_temp) {
			rc =  BF_NO_SYS_RESOURCES;
			goto cleanup;
		}
		(*stage_table_count)++;
	}

	*stage_table = stage_tbl_head;
	return rc;

cleanup:
	stage_tbl_temp = stage_tbl_head;
	while (stage_tbl_temp) {
		FREE_LIST(stage_tbl_temp->act_fmt);
		stage_tbl_temp->act_fmt = NULL;
		stage_tbl_temp = stage_tbl_temp->next;
	}
	FREE_LIST(stage_tbl_head);
	stage_tbl_head = NULL;
	(*stage_table_count) = 0;
	return rc;
}

int dal_ctx_json_parse_global_config(int dev_id, int prof_id,
		cJSON *root, void **dal_global_config)
{
	return BF_SUCCESS;
}

int dal_post_parse_processing(int dev_id, int prof_id,
                struct pipe_mgr_p4_pipeline *ctx)
{
	return BF_SUCCESS;
}


int dal_parse_ctx_json_parse_stage_tables(int dev_id, int prof_id,
	cJSON *stage_table_list_cjson,
	void **stage_table,
	int *stage_table_count,
	struct pipe_mgr_mat_ctx *mat_ctx)
{
	return ctx_json_parse_mat_mat_attr_stage_tables
		(dev_id, prof_id, stage_table_list_cjson,
		 stage_table, stage_table_count, mat_ctx);
}

bool dal_mat_store_state(struct pipe_mgr_mat_ctx *mat_ctx)
{
	return true;
}

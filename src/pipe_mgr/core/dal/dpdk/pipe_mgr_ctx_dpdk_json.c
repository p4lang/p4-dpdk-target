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
	char *action_name;
	int action_handle;
	int err = 0;

	err |= bf_cjson_get_string(action_format_cjson,
			CTX_JSON_ACTION_FORMAT_ACTION_NAME,
			&action_name);

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

int ctx_json_parse_value_lookup_mat_attr_stage_tables(int dev_id, int prof_id,
		cJSON *stage_table_list_cjson, void **stage_table,
		int *stage_table_count, struct pipe_mgr_value_lookup_ctx *val_lookup_ctx)
{
	struct pipe_mgr_dpdk_immediate_fields *immediate_field_temp;
	struct pipe_mgr_dpdk_stage_table *stage_tbl_temp = NULL;
	cJSON *stage_table_cjson = NULL;
	int rc = BF_SUCCESS;
	int resource_id;
	char *resource;
	int err = 0;

	CTX_JSON_FOR_EACH(stage_table_cjson,
			  stage_table_list_cjson) {

		stage_tbl_temp = P4_SDE_CALLOC(1, sizeof(*stage_tbl_temp));
		if (!stage_tbl_temp) {
			rc =  BF_NO_SYS_RESOURCES;
			goto cleanup_stage_table;
		}

		if (stage_tbl_head)
			stage_tbl_temp->next = stage_tbl_head;
		stage_tbl_head = stage_tbl_temp;
		(*stage_table_count)++;

		err |= bf_cjson_get_string(stage_table_cjson,
				CTX_JSON_RESOURCE_NAME,
				&resource);

		err |= bf_cjson_get_int(stage_table_cjson,
				CTX_JSON_RESOURCE_ID,
				&resource_id);

		cJSON *immediate_fields_list_cjson = NULL;

		err |= bf_cjson_try_get_object(stage_table_cjson,
					       CTX_JSON_STAGE_TABLE_IMMEDIATE_FIELDS,
					       &immediate_fields_list_cjson);
		if (err) {
			rc = BF_UNEXPECTED;
			goto cleanup_stage_table;
		}

		if (immediate_fields_list_cjson) {
			cJSON *immediate_field_cjson = NULL;

			CTX_JSON_FOR_EACH(immediate_field_cjson,
					  immediate_fields_list_cjson) {
				immediate_field_temp = P4_SDE_CALLOC(1, sizeof(*immediate_field_temp));
				if (!immediate_field_temp) {
					rc = BF_NO_SYS_RESOURCES;
					goto cleanup_immediate_field;
				}

				rc |= ctx_json_parse_immediate_field_dpdk_json(dev_id, prof_id,
						immediate_field_cjson,
						immediate_field_temp);
				if (rc) {
					rc = BF_UNEXPECTED;
					P4_SDE_FREE(immediate_field_temp);
					goto cleanup_immediate_field;
				}

				if (stage_tbl_temp->immediate_field)
					immediate_field_temp->next = stage_tbl_temp->immediate_field;
				stage_tbl_temp->immediate_field = immediate_field_temp;
				stage_tbl_temp->immediate_fields_count++;
			}
		}

		strncpy(stage_tbl_temp->resource, resource, P4_SDE_NAME_LEN - 1);
		switch (resource_id) {
			case PIPE_MGR_DPDK_RES_MIR_SESSION:
				stage_tbl_temp->resource_id = PIPE_MGR_DPDK_RES_MIR_SESSION;
				break;
			default:
				LOG_ERROR("Unexpected resource_id : %d in ctx json", resource_id);
				rc = BF_UNEXPECTED;
				goto cleanup_immediate_field;
		}

		/* Indirect tables don't have stage tables in context json,
		 * allocate one stage table structure.
		 * This stage table will be used for storing dpdk table metadata
		 */
		if(*stage_table_count == 0) {
			stage_tbl_head = P4_SDE_CALLOC(1, sizeof(*stage_tbl_temp));
			if (!stage_tbl_temp) {
				rc =  BF_NO_SYS_RESOURCES;
				goto cleanup_immediate_field;
			}
			(*stage_table_count)++;
		}

		*stage_table = stage_tbl_head;
		return rc;
	}

cleanup_immediate_field:
	stage_tbl_temp = stage_tbl_head;
	while (stage_tbl_temp) {
		FREE_LIST(stage_tbl_temp->immediate_field);
		stage_tbl_temp->immediate_field = NULL;
		stage_tbl_temp = stage_tbl_temp->next;
	}
cleanup_stage_table:
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

int dal_parse_ctx_json_parse_value_lookup_stage_tables
	(int dev_id, int prof_id,
	 cJSON *stage_table_list_cjson,
	 void **stage_table,
	 int *stage_table_count,
	 struct pipe_mgr_value_lookup_ctx *val_lookup_ctx)
{
	return ctx_json_parse_value_lookup_mat_attr_stage_tables
		(dev_id, prof_id, stage_table_list_cjson,
		 stage_table, stage_table_count, val_lookup_ctx);
}

bool dal_mat_store_state(struct pipe_mgr_mat_ctx *mat_ctx)
{
	return true;
}

static int pipe_mgr_externs_key_cmp_fn(const void *arg, const void *key1)
{
	void *key2 = NULL;
	struct pipe_mgr_externs_ctx *externs_entry = NULL;

	if (!key1 || !arg)
		return (-1);

	key2 = bf_hashtbl_get_cmp_data(key1);

	if (!key2)
		return (-1);

	externs_entry = (struct pipe_mgr_externs_ctx *)key2;

	return strncmp(arg, externs_entry->name, P4_SDE_NAME_LEN);
}

void pipe_mgr_externs_free_htbl_node(void *node)
{
	struct pipe_mgr_externs_ctx *externs_node = node;

	if (!externs_node)
		return;

	P4_SDE_FREE(externs_node);
}

/**
 * Parse each individual externs Object
 * @param dev_id The Device ID
 * @param prof_id The profile ID
 * @param extern_cjson extern cJSON object
 * @param entry structure to hold extern info
 * @return Status of the API call
 */
static bf_status_t ctx_json_parse_externs_entry (
		int dev_id,
		int prof_id,
		cJSON *extern_cjson,
		struct pipe_mgr_externs_ctx *entry)
{
	char *name = NULL, *externs_name = NULL;
	cJSON *extern_attr_cjson  = NULL;
	char *extern_attr_type  = NULL;
	bf_status_t rc = BF_SUCCESS;
	char *target_name = NULL;
	char *target_type = NULL;
	int table_id = 0;
	int err = 0;

	err |= bf_cjson_get_string(extern_cjson,
			CTX_JSON_EXTERN_NAME,
			&name);

	err |= bf_cjson_get_string(extern_cjson,
			CTX_JSON_EXTERN_TARGET_NAME,
			&target_name);

	err |= bf_cjson_get_string(extern_cjson,
			CTX_JSON_EXTERN_TYPE,
			&target_type);

	err |= bf_cjson_try_get_object(extern_cjson,
			CTX_JSON_EXTERN_ATTRIBUTES,
			&extern_attr_cjson);

	if (err)
		return (BF_UNEXPECTED);

	externs_name = trim_classifier_str(name);
	if (!externs_name) {
		LOG_ERROR("extern \"%s\" trim_classifier_str failed", name);
		return BF_UNEXPECTED;
	}
	strncpy(entry->name, externs_name, P4_SDE_NAME_LEN - 1);
	entry->name[P4_SDE_NAME_LEN - 1] = '\0';

	if (target_name) {
		strncpy(entry->target_name, target_name, P4_SDE_NAME_LEN - 1);
		entry->target_name[P4_SDE_NAME_LEN - 1] = '\0';
	}

	if (!strncmp(target_type, CTX_JSON_EXTERN_TYPE_COUNTER,
				sizeof(CTX_JSON_EXTERN_TYPE_COUNTER))) {
		entry->type = EXTERNS_COUNTER;
	}

	if (entry->type == EXTERNS_COUNTER) {
		err |= bf_cjson_get_string(extern_attr_cjson,
				CTX_JSON_EXTERN_ATTRIBUTE_TYPE,
				&extern_attr_type);
		if (err)
			return BF_UNEXPECTED;

		/* verify string for attribute type "bytes" */
		if (!strncmp(extern_attr_type,
			     CTX_JSON_EXTERN_ATTR_TYPE_BYTES,
			     sizeof(CTX_JSON_EXTERN_ATTR_TYPE_BYTES))) {
			entry->attr_type = EXTERNS_ATTR_TYPE_BYTES;
		} else if (!strncmp(extern_attr_type,
				    CTX_JSON_EXTERN_ATTR_TYPE_PACKETS,
				    sizeof(CTX_JSON_EXTERN_ATTR_TYPE_PACKETS))) {
				entry->attr_type = EXTERNS_ATTR_TYPE_PACKETS;
		} else if (!strncmp(extern_attr_type,
			    CTX_JSON_EXTERN_ATTR_TYPE_PACKETS_AND_BYTES,
			    sizeof(CTX_JSON_EXTERN_ATTR_TYPE_PACKETS_AND_BYTES))) {
					entry->attr_type =
						EXTERNS_ATTR_TYPE_PACKETS_AND_BYTES;
		} else {
			LOG_ERROR("unexpected attribute type for extern %s ", name);
			return BF_UNEXPECTED;
		}
		/* Extract table id */
		err |= bf_cjson_get_int(extern_attr_cjson,
                                        CTX_JSON_EXTERN_ATTRIBUTE_TABLE_ID,
                                        &table_id);
		entry->externs_attr_table_id = table_id;
	}

	return rc;
}

/**
 * Parse externs Object
 * @param dev_id The Device ID
 * @param prof_id The profile ID
 * @param root cJSON object
 * @param ctx pipe mgr pipline ctx info.
 * @return Status of the API call
 */
int dal_ctx_json_parse_extern(int dev_id,
			      int prof_id,
			      cJSON *root,
			      struct pipe_mgr_p4_pipeline *ctx)
{
	bf_status_t rc            = BF_SUCCESS;
	cJSON *externs_cjson      = NULL;
	struct pipe_mgr_externs_ctx
		*externs_entry  = NULL;
	bf_hashtbl_sts_t htbl_sts = BF_HASHTBL_OK;
	char key_name[P4_SDE_TABLE_NAME_LEN] = {0};
	int num_of_externs = 0;

	/* We return SUCCESS if extern does not exist, because its a
	 * valid use case, externs is not mandatory.
	 */
	bf_cjson_try_get_object(root,
			CTX_JSON_EXTERN,
			&externs_cjson);

	if (externs_cjson) {
		cJSON *extern_cjson = NULL;
		/* Initialize context for service */
		ctx->bf_externs_htbl = (bf_hashtable_t *)P4_SDE_CALLOC(1,
				sizeof(bf_hashtable_t));

		htbl_sts = bf_hashtbl_init(
				ctx->bf_externs_htbl,
				pipe_mgr_externs_key_cmp_fn,
				pipe_mgr_externs_free_htbl_node,
				P4_SDE_NAME_LEN,
				sizeof (struct pipe_mgr_externs_ctx),
				0x98733423);
		if (htbl_sts != BF_HASHTBL_OK) {
			LOG_ERROR("%s:%d Error in initializing hashtable"
					"for externs",
					__func__,
					__LINE__);
			rc = BF_UNEXPECTED;
			goto externs_cleanup;
		}

               num_of_externs = cJSON_GetArraySize(externs_cjson);

		if (num_of_externs == 0) {
			rc = BF_SUCCESS;
			goto externs_cleanup;
		}

	       ctx->externs_tables_name = P4_SDE_CALLOC (num_of_externs,
							 sizeof(char*));
               if (!ctx->externs_tables_name) {
		       LOG_ERROR("not able to allocate memory for \t "
				 "externs_tables_name");
		       rc = BF_NO_SPACE;
		       goto externs_cleanup;
	       }

		CTX_JSON_FOR_EACH(extern_cjson, externs_cjson) {
			externs_entry = P4_SDE_CALLOC (1,
					sizeof(*externs_entry));

		        if (!externs_entry) {
				rc = BF_NO_SYS_RESOURCES;
				goto externs_cleanup;
			}

			ctx->externs_tables_name[ctx->num_externs_tables] =
				P4_SDE_CALLOC (1, P4_SDE_TABLE_NAME_LEN);
                       if (!ctx->externs_tables_name[ctx->num_externs_tables]) {
                                LOG_ERROR("not able to allocate memory for \t"
                                          "externs_tables_name");
				rc = BF_NO_SPACE;
				goto externs_cleanup;
                       }

			rc  = ctx_json_parse_externs_entry(
					dev_id,
					prof_id,
					extern_cjson,
					externs_entry);

			if (rc) {
				P4_SDE_FREE(externs_entry);
				P4_SDE_FREE(ctx->externs_tables_name[
					    ctx->num_externs_tables]);
				goto externs_cleanup;
			}

			memset(key_name, 0x0, P4_SDE_TABLE_NAME_LEN);
			strncpy(key_name, externs_entry->name,
					P4_SDE_TABLE_NAME_LEN-1);

			/* Insert into the string-entry map */
			htbl_sts = bf_hashtbl_insert(
					ctx->bf_externs_htbl,
					externs_entry,
					key_name);

			if (htbl_sts != BF_HASHTBL_OK) {
				LOG_ERROR("%s:%d Error in inserting extern"
					  "object into the key hash tbl"
					  " for extern %s",
					   __func__,
					   __LINE__,
					   externs_entry->name);
				rc =  BF_UNEXPECTED;
				P4_SDE_FREE(externs_entry);
				goto externs_cleanup;
			}

			strncpy(ctx->externs_tables_name[ctx->num_externs_tables],
				externs_entry->name, P4_SDE_TABLE_NAME_LEN-1);

			ctx->num_externs_tables++;
		}
	}

	return rc;
externs_cleanup:
	pipe_mgr_free_externs_htbl(ctx);
	pipe_mgr_free_pipe_ctx_ext_table_name(ctx);

	return rc;
}

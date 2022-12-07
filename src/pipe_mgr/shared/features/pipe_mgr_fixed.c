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
 * @file pipe_mgr_fixed.c
 *
 * @description Utilities for fixed function tables
 */
#include <pipe_mgr/pipe_mgr_intf.h>
#include "../infra/pipe_mgr_int.h"
#include "../../shared/pipe_mgr_shared_intf.h"
#include "../../core/pipe_mgr_ctx_json.h"
#include "pipe_mgr/shared/pipe_mgr_fixed.h"
#include "../../core/pipe_mgr_log.h"
#include <port_mgr/port_mgr_port.h>

/* IPSEC Yang module name prefix */
/* TODO: need to create mapping of yang module name with fixed funciton manager
*/
#define IPSEC_YANG_MODULE_PREFIX_PYTHON "ipsec_"   /* python input */
#define IPSEC_YANG_MODULE_PREFIX_OVS    "ipsec-"   /* input from OVS */

static int get_fixed_func_mgr_from_table(const char *table_name,
                                         enum fixed_function_mgr *ff_mgr) {
	char *target = strndup(table_name, P4_SDE_TABLE_NAME_LEN);
	char fixed_func_mgr_name[P4_SDE_TABLE_NAME_LEN+1] = {0};
	char *saveptr  = NULL;
	char *mgr_name = NULL;

	//extract manager name from table name
	if (!target)
		return BF_NO_SYS_RESOURCES;

	mgr_name = strtok_r(target, ".", &saveptr);

	if (mgr_name == NULL) {
		P4_SDE_FREE(target);
		LOG_ERROR("%s: invalid fixed function manager table %s",
				__func__,
				table_name);
		return BF_UNEXPECTED;
	}

	strncpy(fixed_func_mgr_name, mgr_name, P4_SDE_TABLE_NAME_LEN);

	/*  TODO: currently hard coding IPSEC module
	         "ipsec_" -> "crypto"
	         "ipsec-" -> "crypto"
	 * Need to create a MAP for yang module name -> Fixed func mgr module
	 * and discuss on naming part weather "-" should be removed with "_"
	 * as expected by pythong.
	 */
	if (!strncmp(fixed_func_mgr_name, IPSEC_YANG_MODULE_PREFIX_PYTHON,
				strlen(IPSEC_YANG_MODULE_PREFIX_PYTHON)) ||
	    !strncmp(fixed_func_mgr_name, IPSEC_YANG_MODULE_PREFIX_OVS,
				strlen(IPSEC_YANG_MODULE_PREFIX_PYTHON)))
		strncpy(fixed_func_mgr_name, "crypto", P4_SDE_TABLE_NAME_LEN);

	*ff_mgr = get_fixed_function_mgr_enum(fixed_func_mgr_name);

	P4_SDE_FREE(target);

	return BF_SUCCESS;
}

/**
 * API to add an entry
 */
int fixed_func_mgr_ent_add(u32 sess_hdl,
		           bf_dev_target_t dev_tgt,
		           const char *table_name,
		           struct fixed_function_key_spec  *key_spec,
		           struct fixed_function_data_spec *data_spec)
{
	enum fixed_function_mgr ff_mgr = FF_MGR_INVALID;
	int status = BF_SUCCESS;

	LOG_TRACE("Entering %s", __func__);

	status = get_fixed_func_mgr_from_table(table_name, &ff_mgr);

	if (status)
		goto exit;

	switch (ff_mgr)
	{
		case FF_MGR_PORT:
			status = port_cfg_table_add(dev_tgt.device_id,
                                                    key_spec,
                                                    data_spec);
			break;
		case FF_MGR_VPORT:
			LOG_DBG("in vport manager");
			break;
		case FF_MGR_CRYPTO:
			LOG_DBG("in crypto manager\n");
			break;
		case FF_MGR_INVALID:
		default:
			LOG_ERROR("%s: unsupported fixed function manager",
					__func__);
			status = BF_INVALID_ARG;
	}
exit:

	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/**
 * API to delete an entry
 */
int fixed_func_mgr_ent_del(u32 sess_hdl,
                           struct bf_dev_target_t dev_tgt,
                           const char *table_name,
                           struct fixed_function_key_spec  *key_spec)
{
	enum fixed_function_mgr ff_mgr = FF_MGR_INVALID;
	int  status = BF_SUCCESS;

	LOG_TRACE("Entering %s", __func__);

	status = get_fixed_func_mgr_from_table(table_name, &ff_mgr);

	if (status)
		goto exit;

	switch (ff_mgr)
	{
		case FF_MGR_PORT:
			LOG_DBG("in port manager delete operation");
			break;
		case FF_MGR_VPORT:
			LOG_DBG("in vport manager delete operation");
			break;
		case FF_MGR_CRYPTO:
			LOG_DBG("in crypto managre delete operation\n");
			break;
		case FF_MGR_INVALID:
		default:
			LOG_ERROR("%s: unsupported fixed function manager",
					__func__);
			status = BF_INVALID_ARG;
	}
exit:
	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/**
 * API to get default entry for key less table
 */
int fixed_func_mgr_get_default_entry(u32 sess_hdl,
                                     bf_dev_target_t dev_tgt,
                                     const char *table_name,
                                     struct fixed_function_data_spec *data_spec)
{
	enum fixed_function_mgr ff_mgr = FF_MGR_INVALID;
	int  status = BF_SUCCESS;

	LOG_TRACE("Entering %s", __func__);

	status = get_fixed_func_mgr_from_table(table_name, &ff_mgr);

	if (status)
		goto exit;

	switch (ff_mgr)
	{
		case FF_MGR_PORT:
		case FF_MGR_VPORT:
			status = BF_NOT_SUPPORTED;
		        break;
		case FF_MGR_CRYPTO:
			LOG_DBG("in crypto managre default get operation");
			status = BF_NOT_SUPPORTED;
			break;
		case FF_MGR_INVALID:
		default:
			LOG_ERROR("%s: unsupported fixed function manager",
					__func__);
			status = BF_INVALID_ARG;
	}

exit:
	LOG_TRACE("Exiting %s", __func__);
	return status;
}

/**
 * API to register auto-notification with back-end manager
 */
int fixed_func_mgr_notification_register(struct bf_dev_target_t dev_tgt,
                                         const char *table_name,
                                         fixed_func_mgr_update_callback cb,
                                         tdi_rt_attributes_type_e _attr_type,
                                         void *cb_cookie)
{
       enum fixed_function_mgr ff_mgr = FF_MGR_INVALID;
       int  status = BF_SUCCESS;

       LOG_TRACE("Entering %s", __func__);

       status = get_fixed_func_mgr_from_table(table_name, &ff_mgr);

       if (status)
	       goto exit;

	switch (ff_mgr)
	{
		case FF_MGR_PORT:
		case FF_MGR_VPORT:
			status = BF_NOT_SUPPORTED;
		        break;
		case FF_MGR_CRYPTO:
			status = BF_NOT_SUPPORTED;
			break;
		case FF_MGR_INVALID:
		default:
			LOG_ERROR("%s: unsupported fixed function manager",
					__func__);
			status = BF_INVALID_ARG;
	}
exit:
       LOG_TRACE("Exiting %s", __func__);
       return status;
}

/**
 * API to retrieve stats/state info
 */
int fixed_func_mgr_get_stats(u32 sess_hdl,
                             struct bf_dev_target_t dev_tgt,
                             const char *table_name,
                             struct fixed_function_key_spec  *key_spec,
                             struct fixed_function_data_spec *data_spec)
{
       enum fixed_function_mgr ff_mgr = FF_MGR_INVALID;
       int  status = BF_SUCCESS;

       LOG_TRACE("Entering %s", __func__);

       status = get_fixed_func_mgr_from_table(table_name, &ff_mgr);

       if (status)
	       goto ff_exit;

	switch (ff_mgr)
	{
		case FF_MGR_PORT:
			status = port_all_stats_get(dev_tgt.device_id,
                                                    key_spec,
                                                    data_spec);
		        break;
		case FF_MGR_VPORT:
			status = BF_NOT_SUPPORTED;
		        break;
		case FF_MGR_CRYPTO:
			status = BF_NOT_SUPPORTED;
			break;
		case FF_MGR_INVALID:
		default:
			LOG_ERROR("%s: unsupported fixed function manager",
					__func__);
			status = BF_INVALID_ARG;
	}
ff_exit:
       LOG_TRACE("Exiting %s", __func__);
       return status;
}

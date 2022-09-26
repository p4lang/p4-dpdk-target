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


#include "fixed_function/fixed_function_init.h"
#include "fixed_function/fixed_function_int.h"
#include "fixed_function_ctx.h"
#include "fixed_function/fixed_function_log.h"
#include <dvm/bf_drv_intf.h>

static struct fixed_function_ctx *fixed_function_ctx_obj;

/*
 * Get the Fixed Function Context Object
 */
struct fixed_function_ctx *get_fixed_function_ctx()
{
        return fixed_function_ctx_obj;
}

struct fixed_function_mgr_ctx *get_fixed_function_mgr_ctx(enum fixed_function_mgr ff_mgr)
{
	struct fixed_function_mgr_ctx *mgr_ctx = NULL;
	struct fixed_function_ctx *ctx = NULL;
	p4_sde_map_sts status;
	p4_sde_map *map;

	LOG_TRACE("Entering %s", __func__);

	ctx = get_fixed_function_ctx();
	if (!ctx) {
		LOG_ERROR("%s : Fixed Function Ctx not allocated",
				 __func__);
		LOG_TRACE("Exiting %s", __func__);
		return NULL;
	}

	map = &ctx->ff_mgr_ctx_map;
	status = P4_SDE_MAP_GET(map, (u64) ff_mgr, (void **)&mgr_ctx);
	if (!mgr_ctx)
		LOG_ERROR("%s: Fixed Function Mgr Ctx not found for mgr %d",
				   __func__, ff_mgr);

	LOG_TRACE("Exiting %s", __func__);
	return ((status == BF_MAP_OK) ? mgr_ctx : NULL);
}

struct fixed_function_table_ctx *get_fixed_function_table_ctx(struct fixed_function_mgr_ctx *ff_mgr_ctx,
							      enum fixed_function_table_type tbl_type)
{
	int i;

	for (i = 0; i < ff_mgr_ctx->num_tables; i++) {
		if (tbl_type == ff_mgr_ctx->table_ctx[i].table_type)
			return &(ff_mgr_ctx->table_ctx[i]);
	}

	LOG_ERROR("%s : Fixed Function Table Ctx not found", __func__);	
	return NULL;
}

bf_status_t fixed_function_encode_key_spec(struct fixed_function_key_spec *key_spec,
					   int start_bit,
					   int bit_width,
					   u8 *key)
{
        int num_bytes;

        if (!key_spec || !key)
                return BF_INVALID_ARG;

        num_bytes = bit_width / 8;

	if ((start_bit + num_bytes) > key_spec->num_bytes)
		return BF_INVALID_ARG;

        memcpy(key_spec->array + start_bit, key, num_bytes);
        return BF_SUCCESS;
}

bf_status_t fixed_function_encode_data_spec(struct fixed_function_data_spec *data_spec,
					    int start_bit,
					    int bit_width,
					    u8 *data)
{
        int num_bytes;

        if (!data_spec || !data)
                return BF_INVALID_ARG;

        num_bytes = bit_width / 8;

        if ((start_bit + num_bytes) > data_spec->num_bytes)
		return BF_INVALID_ARG;

        memcpy(data_spec->array + start_bit, data, num_bytes);
        return BF_SUCCESS;
}

bf_status_t fixed_function_decode_key_spec(struct fixed_function_key_spec *key_spec,
					   int start_bit,
					   int bit_width,
					   u8 *key)
{
        int num_bytes;

        if (!key_spec || !key)
                return BF_INVALID_ARG;

        num_bytes = bit_width / 8;

	if ((start_bit + num_bytes) > key_spec->num_bytes)
		return BF_INVALID_ARG;

        memcpy(key, key_spec->array + start_bit, num_bytes);
        return BF_SUCCESS;
}

bf_status_t fixed_function_decode_data_spec(struct fixed_function_data_spec *data_spec,
					    int start_bit,
					    int bit_width,
					    u8 *data)
{
        int num_bytes;

        if (!data_spec || !data)
                return BF_INVALID_ARG;

        num_bytes = bit_width / 8;

        if ((start_bit + num_bytes) > data_spec->num_bytes)
                return BF_INVALID_ARG;

        memcpy(data, data_spec->array + start_bit, num_bytes);
        return BF_SUCCESS;
}

bf_status_t fixed_function_ctx_map_add(struct fixed_function_mgr_ctx *mgr_ctx)
{
	struct fixed_function_ctx *ctx = NULL;
	p4_sde_map_sts status;
	p4_sde_map *map;

	LOG_TRACE("Entering %s", __func__);

	ctx = get_fixed_function_ctx();
	if (!ctx) {
		LOG_ERROR("%s : Fixed Function Ctx not allocated",
				__func__);
		LOG_TRACE("Exiting %s", __func__);
		return BF_OBJECT_NOT_FOUND;
	}

	map = &ctx->ff_mgr_ctx_map;

        /* Insert Port Info to Hash Map */
        status = P4_SDE_MAP_ADD(map, (u64) mgr_ctx->ff_mgr, (void *)mgr_ctx);
        if (status != BF_MAP_OK) {
                LOG_ERROR("Failed to save fixed function context,"
			  "sts %d", status);
		LOG_TRACE("Exiting %s", __func__);
                return BF_UNEXPECTED;
        }

	LOG_TRACE("Exiting %s", __func__);
	return BF_SUCCESS;

}

/*
 * Fixed Function Device Add Callback
 */
bf_status_t fixed_function_dev_add(bf_dev_id_t dev_id,
				bf_dev_family_t dev_family,
				bf_device_profile_t *profile,
				bf_dev_init_mode_t warm_init_mode)
{
        int status = BF_SUCCESS;

        LOG_TRACE("Entering %s", __func__);

        status = fixed_function_ctx_import(dev_id, profile);

	LOG_TRACE("Exiting %s", __func__);
        return status;
}

/*
 * Fixed Function Device Remove Callback
 */
bf_status_t fixed_function_dev_remove(bf_dev_id_t dev_id)
{
        LOG_TRACE("STUB %s", __func__);
        return BF_SUCCESS;
}

int fixed_function_init(void)
{
	bf_drv_client_callbacks_t callbacks = {0};
        bf_drv_client_handle_t bf_drv_hdl;
	struct fixed_function_ctx *ctx;
	int status;

	LOG_TRACE("Entering %s", __func__);

        if (fixed_function_ctx_obj) {
		LOG_DBG("Fixed function Ctx Obj already initialized");
                LOG_TRACE("Exiting %s", __func__);
                return BF_SUCCESS;
        }

	ctx = P4_SDE_CALLOC(1, sizeof(struct fixed_function_ctx));
        if (!ctx) {
                LOG_ERROR("%s: failed to allocate memory for context",
                          __func__);
                LOG_TRACE("Exiting %s", __func__);
                return BF_NO_SYS_RESOURCES;
        }

        status = P4_SDE_MAP_INIT(&ctx->ff_mgr_ctx_map);
        if (status) {
		P4_SDE_FREE(ctx);
                LOG_ERROR("%s: failed to initialize fixed function context map",
				__func__);
		LOG_TRACE("Exiting %s", __func__);
                return BF_NO_SYS_RESOURCES;
        }

	fixed_function_ctx_obj = ctx;

        // register callbacks with DVM
        status = bf_drv_register("fixed_function", &bf_drv_hdl);
        if (status != BF_SUCCESS) {
                LOG_ERROR("%s: Registration failed, sts %s",
                                   __func__, bf_err_str(status));
                goto err_cleanup;
        }

        callbacks.device_add = fixed_function_dev_add;
        callbacks.device_del = fixed_function_dev_remove;
        bf_drv_client_register_callbacks(bf_drv_hdl, &callbacks,
                                         BF_CLIENT_PRIO_4);

        return BF_SUCCESS;

err_cleanup:
        fixed_function_cleanup();
	return BF_INVALID_ARG;
}

void fixed_function_cleanup(void)
{
	LOG_TRACE("Entering %s", __func__);

	if (fixed_function_ctx_obj) {
		P4_SDE_MAP_DESTROY(&fixed_function_ctx_obj->ff_mgr_ctx_map);
		P4_SDE_FREE(fixed_function_ctx_obj);
		fixed_function_ctx_obj = NULL;
	}
	LOG_TRACE("Exiting %s", __func__);
}

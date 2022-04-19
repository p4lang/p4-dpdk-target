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

#include "port_mgr.h"
#include <target-utils/third-party/xxHash/xxHash/xxhash.h>

#define HASH_SEED 0x12345678

/* Pointer to global port_mgr context */
static struct port_mgr_ctx_t *port_mgr_ctx_obj;

struct port_mgr_ctx_t *get_port_mgr_ctx()
{
	return port_mgr_ctx_obj;
}

void port_mgr_ctx_cleanup(struct port_mgr_ctx_t *ctx)
{
	port_mgr_log_trace("Entering %s", __func__);

	P4_SDE_MAP_DESTROY(&ctx->port_info_map);
	P4_SDE_MAP_DESTROY(&ctx->port_name_map);

	port_mgr_log_trace("Exiting %s", __func__);
}

int port_mgr_set_port_info(bf_dev_port_t dev_port,
			   struct port_attributes_t *port_attrib)
{
	struct port_info_t *port_info;
	struct port_mgr_ctx_t *ctx;
	p4_sde_map_sts status;
	p4_sde_map *map;

	port_mgr_log_trace("Entering %s", __func__);

	port_info = P4_SDE_CALLOC(1, sizeof(struct port_info_t));
	if (!port_info)
		return BF_NO_SYS_RESOURCES;

	port_info->dev_port = dev_port;
	memcpy(&port_info->port_attrib, port_attrib,
	       sizeof(struct port_attributes_t));

	/* Get the global port_mgr context */
	ctx = get_port_mgr_ctx();
	if (!ctx) {
		P4_SDE_FREE(port_info);
		return BF_OBJECT_NOT_FOUND;
	}

	map = &ctx->port_info_map;

	/* Insert Port Info to Hash Map */
	status = P4_SDE_MAP_ADD(map, dev_port, (void *)port_info);
	if (status != BF_MAP_OK) {
		P4_SDE_FREE(port_info);
		port_mgr_log_error("Failed to save port info,"
				   "dev port %d sts %d",
				   dev_port, status);
		return BF_UNEXPECTED;
	}

	port_mgr_log_trace("Exiting %s", __func__);

	return BF_SUCCESS;
}

int port_mgr_remove_port_info(bf_dev_port_t dev_port)
{
	struct port_info_t *port_info;
	struct port_mgr_ctx_t *ctx;
	p4_sde_map_sts status;
	p4_sde_map *map;

	port_mgr_log_trace("Entering %s", __func__);

	/* Get the global port_mgr context */
	ctx = get_port_mgr_ctx();
	if (!ctx)
		return BF_OBJECT_NOT_FOUND;

	map = &ctx->port_info_map;

	port_info = port_mgr_get_port_info(dev_port);
	if (!port_info)
		return BF_OBJECT_NOT_FOUND;

	P4_SDE_FREE(port_info);

	/* Remove Port Info from Hash Map */
	status = P4_SDE_MAP_RMV(map, dev_port);
	if (status != BF_MAP_OK) {
		port_mgr_log_error("%s: failed to remove object from map",
				   __func__);
		return BF_UNEXPECTED;
	}

	port_mgr_log_trace("Exiting %s", __func__);

	return BF_SUCCESS;
}

struct port_info_t *port_mgr_get_port_info(bf_dev_port_t dev_port)
{
	struct port_info_t *port_info = NULL;
	struct port_mgr_ctx_t *ctx;
	p4_sde_map_sts status;
	p4_sde_map *map;

	port_mgr_log_trace("Entering %s", __func__);

	/* Get the global port_mgr context */
	ctx = get_port_mgr_ctx();
	if (!ctx)
		return NULL;

	map = &ctx->port_info_map;

	/* Get Port Info from Hash Map */
	status = P4_SDE_MAP_GET(map, dev_port, (void **)&port_info);
    
	if (!port_info)
		port_mgr_log_error("%s: Port info not found for dev port %d",
							__func__, dev_port);

	port_mgr_log_trace("Exiting %s", __func__);

	return ((status == BF_MAP_OK) ? port_info : NULL);
}

static u32 construct_hash(char *str)
{
	return XXH32(str, strlen(str), HASH_SEED);
}

u32 *port_mgr_get_port_from_name(char *port_name)
{
        p4_sde_map *map;
        u32 *dev_port_obj = NULL;
	struct port_mgr_ctx_t *ctx;
        u32 port_name_hash = 0;
        int status;

        port_mgr_log_trace("Entering %s", __func__);

        port_name_hash = construct_hash(port_name);
        port_mgr_log_trace(" %s : Port Name Hash %u", __func__,
                           port_name_hash);

        /* Get the global port_mgr context */
        ctx = get_port_mgr_ctx();
        if (!ctx)
                return NULL;

        map = &ctx->port_name_map;

        /* Get Port Info from Hash Map */
        status = P4_SDE_MAP_GET(map, port_name_hash, (void **)&dev_port_obj);

        port_mgr_log_trace("Exiting %s", __func__);
        return ((status == BF_MAP_OK) ? dev_port_obj : NULL);
}

int port_mgr_add_name(char *port_name, u32 dev_port)
{
        u32 *dev_port_obj = NULL;
        u32 port_name_hash = 0;
	struct port_mgr_ctx_t *ctx;
        p4_sde_map_sts status;
        p4_sde_map *map;

        port_mgr_log_trace("Entering %s", __func__);

        /* Get the global port_mgr context */
        ctx = get_port_mgr_ctx();
        if (!ctx) {
                return BF_OBJECT_NOT_FOUND;
        }
        map = &ctx->port_name_map;

        port_name_hash = construct_hash(port_name);
        port_mgr_log_trace(" %s : Port Name Hash %u", __func__,
                           port_name_hash);

	if (P4_SDE_MAP_GET(map, port_name_hash, (void **)&dev_port_obj) == 
			BF_MAP_OK) {
		port_mgr_log_trace
			("%s : Port Name Hash Collision, Entry already exists",
			__func__);
		return BF_UNEXPECTED;
	}

        dev_port_obj = P4_SDE_CALLOC(1, sizeof(*dev_port_obj));
        if (!dev_port_obj)
                return BF_NO_SYS_RESOURCES;

	*dev_port_obj = dev_port;


        status = P4_SDE_MAP_ADD(map, port_name_hash, (void *)dev_port_obj);
        if (status != BF_MAP_OK) {
                port_mgr_log_error("Adding dev port %d to port name map "
                                   "failed . sts %d", dev_port, status);
                status = BF_UNEXPECTED;
                goto cleanup;
        }

        port_mgr_log_trace("Exiting %s", __func__);
        return BF_SUCCESS;

cleanup:
        P4_SDE_FREE(dev_port_obj);
        port_mgr_log_trace("Exiting %s", __func__);
        return BF_SUCCESS;
}

int port_mgr_remove_name(char *port_name)
{
        u32 *dev_port_obj = NULL;
        int status;
        u32 port_name_hash = 0;
	struct port_mgr_ctx_t *ctx;
        p4_sde_map *map;

        port_mgr_log_trace("Entering %s", __func__);

        port_name_hash = construct_hash(port_name);
        port_mgr_log_trace(" %s : Port Name Hash %u", __func__,
                           port_name_hash);

        dev_port_obj = port_mgr_get_port_from_name(port_name);
        if (!dev_port_obj) {
                port_mgr_log_error("Dev Port not found in port name map. "
                                   "Exiting %s", __func__);
                status = BF_OBJECT_NOT_FOUND;
                goto end;
        }

        P4_SDE_FREE(dev_port_obj);

        /* Get the global port_mgr context */
        ctx = get_port_mgr_ctx();
        if (!ctx)
                return BF_OBJECT_NOT_FOUND;

        map = &ctx->port_name_map;

        /* Remove Port Info from Hash Map */
        status = P4_SDE_MAP_RMV(map, port_name_hash);
        if (status != BF_MAP_OK) {
                port_mgr_log_error("%s: failed to remove object from map",
                                   __func__);
                status = BF_UNEXPECTED;
                goto end;
        }
        status = BF_SUCCESS;

end:
        port_mgr_log_trace("Exiting %s", __func__);
        return status;
}

int port_mgr_shared_init(void)
{
	struct port_mgr_ctx_t *ctx;

	port_mgr_log_trace("Entering %s", __func__);

	/* If port_mgr is already initialized ignore */
	if (get_port_mgr_ctx()) {
		port_mgr_log_trace("Already initialized. Exiting %s",
				   __func__);
		return BF_SUCCESS;
	}

	/* Initialize context for service */
	ctx = P4_SDE_CALLOC(1, sizeof(struct port_mgr_ctx_t));
	if (!ctx) {
		port_mgr_log_error("%s: failed to allocate memory for context",
				   __func__);
		return BF_NO_SYS_RESOURCES;
	}

	P4_SDE_MAP_INIT(&ctx->port_info_map);
	P4_SDE_MAP_INIT(&ctx->port_name_map);

	/* Store the context pointer */
	port_mgr_ctx_obj = ctx;

	port_mgr_log_trace("Exiting %s", __func__);
	return BF_SUCCESS;
}

void port_mgr_init(void)
{
	bf_drv_client_callbacks_t callbacks = {0};
	bf_drv_client_handle_t bf_drv_hdl;
	bf_status_t status;

	port_mgr_log_trace("Entering %s", __func__);

	status = port_mgr_shared_init();
	if (status != BF_SUCCESS) {
		port_mgr_log_trace("Exiting %s", __func__);
		return;
	}

	// register callbacks with DVM
	status = bf_drv_register("port_mgr", &bf_drv_hdl);
	if (status != BF_SUCCESS) {
		port_mgr_log_error("%s: Registration failed, sts %s",
				   __func__, bf_err_str(status));
		goto err_cleanup;
	}

	callbacks.device_add = port_mgr_dev_add;
	callbacks.device_del = port_mgr_dev_remove;
	bf_drv_client_register_callbacks(bf_drv_hdl, &callbacks,
					 BF_CLIENT_PRIO_4);

	// Platform specific initialization
	port_mgr_platform_init();
	port_mgr_log_trace("Exiting %s", __func__);
	return;

err_cleanup:
	port_mgr_cleanup();
}

void port_mgr_cleanup(void)
{
	port_mgr_log_trace("Entering %s", __func__);

	if (!port_mgr_ctx_obj) {
		port_mgr_log_trace("Already cleaned up. Exiting %s",
				   __func__);
		return;
	}

	port_mgr_ctx_cleanup(port_mgr_ctx_obj);
	P4_SDE_FREE(port_mgr_ctx_obj);
	port_mgr_ctx_obj = NULL;

	port_mgr_log_trace("Exiting %s", __func__);
}

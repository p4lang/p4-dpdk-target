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
#include <osdep/p4_sde_osdep.h>
#include <port_mgr/dpdk/bf_dpdk_port_if.h>

#include "../dal_init.h"
#include "../../infra/pipe_mgr_int.h"
#include "../../../core/pipe_mgr_log.h"
#include <lld_dpdk_port.h>
#include <infra/dpdk_infra.h>
#include "../../pipe_mgr_shared_intf.h"
#include "pipe_mgr_dpdk_int.h"
#include "pipe_mgr_dpdk_ctx_util.h"
#define BUF_SIZE 2048
#define PATH_SIZE 512
#define P4_OBJ_FILE_PATH "/tmp/p4"

int dal_add_device(int dev_id,
		   enum bf_dev_family_t dev_family,
		   struct bf_device_profile *prof,
		   enum bf_dev_init_mode_s warm_init_mode)
{
	LOG_TRACE("Entering %s", __func__);
	LOG_TRACE("Exit %s", __func__);
	return BF_SUCCESS;
}

int dal_remove_device(int dev_id)
{
    //    P4_SDE_FREE(dal_dpdk_ctx);
	return BF_SUCCESS;
}

/**
 * Trigger from bf_shell which call bf-rt api at runtime.
 */
int dal_enable_pipeline(bf_dev_id_t dev_id,
		int profile_id,
		void *spec_file,
		enum bf_dev_init_mode_s warm_init_mode)
{
	struct pipe_mgr_profile *profile;
	struct pipeline *pipe;
	const char *err_msg;
	uint32_t err_line;
	int status;
	uint32_t i, n_ports;
	struct rte_swx_ctl_pipeline_info pipeline;
	uint64_t net_port_mask;
	char c_filepath[PATH_SIZE], o_filepath[PATH_SIZE];
	char i_filepath[PATH_SIZE], so_filepath[PATH_SIZE];
	char buffer[BUF_SIZE], *install_dir;
	FILE *fd = NULL;

	LOG_TRACE("Entering %s dev_id %d init_mode %d",
			__func__, dev_id, warm_init_mode);

	status = pipe_mgr_get_profile(dev_id, profile_id,
				      &profile);
	if (status) {
		LOG_ERROR("not able find profile with device_id  %d",
				dev_id);
		return BF_OBJECT_NOT_FOUND;
	}

	/* get dpdk pipeline, table and action info */
	pipe = pipeline_find(profile->pipeline_name);
	if (!pipe) {
		LOG_ERROR("dpdk pipeline %s get failed",
				profile->pipeline_name);
		return BF_OBJECT_NOT_FOUND;
	}

	snprintf(c_filepath, sizeof(c_filepath), "%s.c", P4_OBJ_FILE_PATH);
	snprintf(o_filepath, sizeof(o_filepath), "%s.o", P4_OBJ_FILE_PATH);
	snprintf(so_filepath, sizeof(so_filepath), "%s.so", P4_OBJ_FILE_PATH);

	fd = fopen(c_filepath, "w");
	if (!fd) {
		LOG_ERROR("%s line:%d  Cannot open file %s\n", __func__,
			  __LINE__, c_filepath);
		return BF_INTERNAL_ERROR;
	}

	status = rte_swx_pipeline_codegen(spec_file, fd, &err_line, &err_msg);
	if (status) {
		LOG_ERROR("%s line:%d Error %d at line %u: %s\n", __func__,
			  __LINE__, status, err_line, err_msg);
		fclose(fd);
		return BF_UNEXPECTED;
	}
	fclose(fd);

	install_dir = getenv("SDE_INSTALL");
	if (install_dir) {
		memset(i_filepath, 0, sizeof(i_filepath));
		snprintf(i_filepath, sizeof(i_filepath), "%s/include/"
			 , install_dir);

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "gcc -c -O3 -fpic "
			 "-Wno-deprecated-declarations -o %s %s -I %s",
			 o_filepath, c_filepath, i_filepath);
		status = system(buffer);
		if (status) {
			LOG_ERROR("%s line:%d  Cannot generate %s file\n",
				  __func__, __LINE__, o_filepath);
			return BF_INTERNAL_ERROR;
		}

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "gcc -shared %s -o %s ",
			 o_filepath, so_filepath);
		status = system(buffer);
		if (status) {
			LOG_ERROR("%s line:%d  Cannot generate %s file\n",
				  __func__, __LINE__, so_filepath);
			return BF_INTERNAL_ERROR;
		}

		fd = fopen(IOSPEC_FILE_PATH, "r");
		if (!fd) {
			LOG_ERROR("%s line:%d  Cannot open %s file\n",
				  __func__, __LINE__, IOSPEC_FILE_PATH);
			return BF_INTERNAL_ERROR;
		}
		status = rte_swx_pipeline_build_from_lib(&pipe->p,
							 profile->pipeline_name,
							 so_filepath, fd,
							 pipe->numa_node);
		if (status) {
			LOG_ERROR("%s line:%d  Pipeline build failed\n",
				  __func__, __LINE__);
			fclose(fd);
			return BF_INTERNAL_ERROR;
		}
		fclose(fd);
	}

	if (strncmp(profile->pipe_ctx.arch_name, "pna", 3))
		goto create_pipeline;

	rte_swx_ctl_pipeline_info_get(pipe->p, &pipeline);
	n_ports = pipeline.n_ports_in > pipeline.n_ports_out ?
		pipeline.n_ports_in : pipeline.n_ports_out;
	for (i = 0; i < n_ports; i++) {
		net_port_mask = pipe->net_port_mask[i / 64];
		if (net_port_mask & (1 << i)) {
			status = rte_swx_ctl_pipeline_regarray_write
				(pipe->p, PNA_DIR_REG_NAME, i, 0);
			if (status)
				LOG_ERROR("set 0 to dir_reg failed Err:%d\n",
					  status);
		} else {
			status = rte_swx_ctl_pipeline_regarray_write
				(pipe->p, PNA_DIR_REG_NAME, i, 1);
			if (status)
				LOG_ERROR("set 1 to dir_reg failed Err:%d\n",
					  status);
		}
	}

create_pipeline:
	pipe->ctl = rte_swx_ctl_pipeline_create(pipe->p);
	if (!pipe->ctl) {
		LOG_ERROR("Pipeline control create failed.");
		rte_swx_pipeline_free(pipe->p);
		return BF_UNEXPECTED;
	}

	status = thread_pipeline_enable(profile->core_id,
					profile->pipeline_name);
	if (status) {
		LOG_ERROR("error :thread pipeline enable");
		return BF_UNEXPECTED;
	}

	LOG_TRACE("Exit %s", __func__);
	return BF_SUCCESS;
}

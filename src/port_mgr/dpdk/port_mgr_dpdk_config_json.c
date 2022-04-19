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

/*!
 * @file port_mgr_dpdk_config_json.c
 *
 *
 * Definitions for DPDK Port Manager Config Parsing
 */

/* Global Headers */
#include <sys/stat.h>
#include <stdio.h>

/* P4 SDE Headers */
#include <cjson/cJSON.h>
#include <ctx_json/ctx_json_utils.h>
#include <osdep/p4_sde_osdep.h>

/* Local Headers */
#include "port_mgr/bf_port_if.h"
#include "port_mgr/port_mgr_port.h"
#include "port_mgr/port_mgr_config_json.h"
#include "port_mgr/port_mgr_log.h"

#define PORT_CONFIG_JSON_PORTS_NODE "ports"
#define PORT_CONFIG_JSON_DEV_PORT "dev_port"
#define PORT_CONFIG_JSON_PORT_NAME "port_name"
#define PORT_CONFIG_JSON_MEMPOOL_NAME "mempool_name"
#define PORT_CONFIG_JSON_PIPE_IN "pipe_in"
#define PORT_CONFIG_JSON_PIPE_OUT "pipe_out"
#define PORT_CONFIG_JSON_PORT_DIR "port_dir"
#define PORT_CONFIG_JSON_PORT_IN_ID "port_in_id"
#define PORT_CONFIG_JSON_PORT_OUT_ID "port_out_id"
#define PORT_CONFIG_JSON_PORT_TYPE "port_type"
#define PORT_CONFIG_JSON_TAP_PORT_ATTRIB_NODE "tap_port_attributes"
#define PORT_CONFIG_JSON_LINK_PORT_ATTRIB_NODE "link_port_attributes"
#define PORT_CONFIG_JSON_SOURCE_PORT_ATTRIB_NODE "source_port_attributes"
#define PORT_CONFIG_JSON_SINK_PORT_ATTRIB_NODE "sink_port_attributes"
#define PORT_CONFIG_JSON_RING_PORT_ATTRIB_NODE "ring_port_attributes"
#define PORT_CONFIG_JSON_PORT_MTU "mtu"
#define PORT_CONFIG_JSON_PORT_PCIE_BDF "pcie_bdf"
#define PORT_CONFIG_JSON_PORT_DEV_ARGS "dev_args"
#define PORT_CONFIG_JSON_PORT_DEV_HOTPLUG_ENABLED "dev_hotplug_enabled"
#define PORT_CONFIG_JSON_PORT_FILE_NAME "file_name"
#define PORT_CONFIG_JSON_PORT_SIZE "size"

#define PORT_CONFIG_JSON_FOR_EACH(it, parent) \
	for ((it) = (parent)->child; (it) != NULL; (it) = (it)->next)

#define BUF_SIZE 2048

/**
 * DPDK Port Type Map Structure
 */
struct port_mgr_port_type_map {
	char port_type_name[P4_SDE_NAME_LEN];  /*!< DPDK Port Type String */
	enum dpdk_port_type_t port_type;            /*!< DPDK Port Type Enum */
};

/**
 * DPDK Port Type Map
 */
static struct port_mgr_port_type_map port_type_map[] = {
	{"tap", BF_DPDK_TAP},		/*!< DPDK Tap Port */
	{"link", BF_DPDK_LINK},		/*!< DPDK Link Port */
	{"source", BF_DPDK_SOURCE},	/*!< DPDK Source Port */
	{"sink", BF_DPDK_SINK},		/*!< DPDK Sink Port */
	{"ring", BF_DPDK_RING}      /*!< DPDK Ring Port */
};

/**
 * DPDK Port Direction Map Structure
 */
struct port_mgr_port_dir_map {
	char port_dir_name[P4_SDE_NAME_LEN];  /*!< DPDK Port Direction String */
	bf_pm_port_dir_e port_dir;	      /*!< DPDK Port Direction Enum */
};

/**
 * DPDK Port Direction Map
 */
static struct port_mgr_port_dir_map port_dir_map[] = {
	{"default", PM_PORT_DIR_DEFAULT},	/*!< Default Direction */
	{"in", PM_PORT_DIR_RX_ONLY},		/*!< RX Only */
	{"out", PM_PORT_DIR_TX_ONLY}		/*!< TX Only */
};

/**
 * Get the DPDK Port Type Enum from
 * string
 * @param port_type Port Type String
 * @return Port Type Enum
 */
static enum dpdk_port_type_t get_port_type_enum(char *port_type)
{
	int num_port_type;
	int i;

	num_port_type = sizeof(port_type_map) /
			sizeof(struct port_mgr_port_type_map);

	for (i = 0; i < num_port_type; i++) {
		if (strncmp(port_type, port_type_map[i].port_type_name,
			    P4_SDE_NAME_LEN) == 0)
			return port_type_map[i].port_type;
	}

	port_mgr_log_error("Invalid port type:%s", port_type);
	return BF_DPDK_PORT_MAX;
}

/**
 * Get the DPDK Port Direction Enum from
 * string
 * @param port_dir Port Direction String
 * @return Port Direction Enum
 */
static bf_pm_port_dir_e get_port_dir_enum(char *port_dir)
{
	int num_port_dir;
	int i;

	num_port_dir = sizeof(port_dir_map) /
			sizeof(struct port_mgr_port_dir_map);

	for (i = 0; i < num_port_dir; i++) {
		if (strncmp(port_dir, port_dir_map[i].port_dir_name,
			    P4_SDE_NAME_LEN) == 0)
			return port_dir_map[i].port_dir;
	}

	port_mgr_log_error("Invalid port dir:%s", port_dir);
	return PM_PORT_DIR_MAX;
}

/**
 * Parse Tap Port Attributes
 * @param port_cjson Port cJSON Object
 * @param tap Tap Port Attributes
 * @return Status of the API call
 */
static int port_config_json_parse_tap_port(cJSON *port_cjson,
					   struct tap_port_attributes_t *tap)
{
	int mtu;
	cJSON *tap_port_attrib_cjson = NULL;
	int err = 0;

	err |= bf_cjson_get_object(port_cjson,
				   PORT_CONFIG_JSON_TAP_PORT_ATTRIB_NODE,
				   &tap_port_attrib_cjson);

	err |= bf_cjson_get_int(tap_port_attrib_cjson,
				PORT_CONFIG_JSON_PORT_MTU,
				&mtu);

	if (err)
		return BF_UNEXPECTED;

	tap->mtu = mtu;
	return BF_SUCCESS;
}

/**
 * Parse Link Port Attributes
 * @param port_cjson Port cJSON Object
 * @param link Link Port Attributes
 * @return Status of the API call
 */
static int port_config_json_parse_link_port(cJSON *port_cjson,
					    struct link_port_attributes_t *link)
{
	char *pcie_bdf = NULL;
	char *dev_args = NULL;
	int dev_hotplug_enabled = 0;
	cJSON *link_port_attrib_cjson = NULL;
	int err = 0;

	err |= bf_cjson_get_object(port_cjson,
				PORT_CONFIG_JSON_LINK_PORT_ATTRIB_NODE,
				&link_port_attrib_cjson);

	err |= bf_cjson_get_string(link_port_attrib_cjson,
				PORT_CONFIG_JSON_PORT_PCIE_BDF,
				&pcie_bdf);

	err |= bf_cjson_try_get_string(link_port_attrib_cjson,
				PORT_CONFIG_JSON_PORT_DEV_ARGS,
				&dev_args);

	err |= bf_cjson_try_get_int(link_port_attrib_cjson,
				PORT_CONFIG_JSON_PORT_DEV_HOTPLUG_ENABLED,
				&dev_hotplug_enabled);
	if (err)
		return BF_UNEXPECTED;

	strncpy(link->pcie_domain_bdf, pcie_bdf, PCIE_BDF_LEN - 1);
	link->pcie_domain_bdf[PCIE_BDF_LEN - 1] = '\0';
	if (dev_args) {
		strncpy(link->dev_args, dev_args, DEV_ARGS_LEN - 1);
		link->dev_args[DEV_ARGS_LEN - 1] = '\0';
	}
	link->dev_hotplug_enabled = dev_hotplug_enabled;

	return BF_SUCCESS;
}

/**
 * Parse Source Port Attributes
 * @param port_cjson Port cJSON Object
 * @param source Source Port Attributes
 * @return Status of the API call
 */
static int port_config_json_parse_source_port(cJSON *port_cjson,
								struct source_port_attributes_t *source)
{
	char *file_name = NULL;
	cJSON *source_port_attrib_cjson = NULL;
	int err = 0;

	err |= bf_cjson_get_object(port_cjson,
				PORT_CONFIG_JSON_SOURCE_PORT_ATTRIB_NODE,
				&source_port_attrib_cjson);

	err |= bf_cjson_get_string(source_port_attrib_cjson,
				PORT_CONFIG_JSON_PORT_FILE_NAME,
				&file_name);

	if (err)
		return BF_UNEXPECTED;

	strncpy(source->file_name, file_name, PCAP_FILE_NAME_LEN - 1);
	source->file_name[PCAP_FILE_NAME_LEN - 1] = '\0';
	return BF_SUCCESS;
}

/**
 * Parse Sink Port Attributes
 * @param port_cjson Port cJSON Object
 * @param sink Sink Port Attributes
 * @return Status of the API call
 */
static int port_config_json_parse_sink_port(cJSON *port_cjson,
					    struct sink_port_attributes_t *sink)
{
	char *file_name = NULL;
	cJSON *sink_port_attrib_cjson = NULL;
	int err = 0;

	err |= bf_cjson_get_object(port_cjson,
				PORT_CONFIG_JSON_SINK_PORT_ATTRIB_NODE,
				&sink_port_attrib_cjson);

	err |= bf_cjson_get_string(sink_port_attrib_cjson,
				PORT_CONFIG_JSON_PORT_FILE_NAME,
				&file_name);

	if (err)
		return BF_UNEXPECTED;

	strncpy(sink->file_name, file_name, PCAP_FILE_NAME_LEN - 1);
	sink->file_name[PCAP_FILE_NAME_LEN - 1] = '\0';
	return BF_SUCCESS;
}

/**
 * Parse Ring Port Attributes
 * @param port_cjson Port cJSON Object
 * @param ring Ring Port Attributes
 * @return Status of the API call
 */
static int port_config_json_parse_ring_port(cJSON *port_cjson,
							struct ring_port_attributes_t *ring)
{
		int size;
		cJSON *ring_port_attrib_cjson = NULL;
		int err = 0;

		err |= bf_cjson_get_object(port_cjson,
					PORT_CONFIG_JSON_RING_PORT_ATTRIB_NODE,
					&ring_port_attrib_cjson);

		err |= bf_cjson_get_int(ring_port_attrib_cjson,
					PORT_CONFIG_JSON_PORT_SIZE,
					&size);

		if (err)
			return BF_UNEXPECTED;

		ring->size = size;
		return BF_SUCCESS;
}

/**
 * Parse each individual Port Object
 * @param dev_id The Device ID
 * @param port_cjson Port cJSON object
 * @param port_info Port Information
 * @return Status of the API call
 */
static int port_config_json_parse_port(int dev_id,
				       cJSON *port_cjson,
				       struct port_info_t *port_info)
{
	int dev_port, port_in_id = 0, port_out_id = 0;
	char *port_name = NULL;
	char *pipe_in = NULL, *pipe_out = NULL;
	char *mempool_name = NULL;
	char *port_type = NULL;
	char *port_dir = NULL;
	int err = 0;
	int status = BF_SUCCESS;

	err |= bf_cjson_get_int(port_cjson, PORT_CONFIG_JSON_DEV_PORT,
				&dev_port);

	err |= bf_cjson_get_string(port_cjson, PORT_CONFIG_JSON_PORT_NAME,
				&port_name);

	err |= bf_cjson_get_string(port_cjson, PORT_CONFIG_JSON_MEMPOOL_NAME,
				&mempool_name);

	err |= bf_cjson_get_string(port_cjson, PORT_CONFIG_JSON_PORT_DIR,
				&port_dir);

	if (err)
		return BF_UNEXPECTED;

	port_info->dev_port = dev_port;

	strncpy(port_info->port_attrib.port_name, port_name, PORT_NAME_LEN - 1);
	port_info->port_attrib.port_name[PORT_NAME_LEN - 1] = '\0';
	strncpy(port_info->port_attrib.mempool_name, mempool_name,
		MEMPOOL_NAME_LEN - 1);
	port_info->port_attrib.mempool_name[MEMPOOL_NAME_LEN - 1] = '\0';

	if (get_port_dir_enum(port_dir) != PM_PORT_DIR_MAX) {
		port_info->port_attrib.port_dir = get_port_dir_enum(port_dir);
	} else {
		port_mgr_log_error
		("%s:%d Incorrect Port Direction", __func__, __LINE__);
		return BF_INVALID_ARG;
	}

	if ((port_info->port_attrib.port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_info->port_attrib.port_dir == PM_PORT_DIR_RX_ONLY)) {
		err |= bf_cjson_get_int(port_cjson,
					PORT_CONFIG_JSON_PORT_IN_ID,
					&port_in_id);
		err |= bf_cjson_get_string(port_cjson,
					   PORT_CONFIG_JSON_PIPE_IN,
					   &pipe_in);
	}

	if ((port_info->port_attrib.port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_info->port_attrib.port_dir == PM_PORT_DIR_TX_ONLY)) {
		err |= bf_cjson_get_int(port_cjson,
					PORT_CONFIG_JSON_PORT_OUT_ID,
					&port_out_id);
		err |= bf_cjson_get_string(port_cjson,
					PORT_CONFIG_JSON_PIPE_OUT,
					&pipe_out);
	}

	err |= bf_cjson_get_string(port_cjson, PORT_CONFIG_JSON_PORT_TYPE,
				&port_type);

	if (err)
		return BF_UNEXPECTED;

	port_info->port_attrib.port_in_id = port_in_id;
	port_info->port_attrib.port_out_id = port_out_id;

	if (pipe_in) {
		strncpy(port_info->port_attrib.pipe_in, pipe_in,
			PIPE_NAME_LEN - 1);
		port_info->port_attrib.pipe_in[PIPE_NAME_LEN - 1] = '\0';
	}

	if (pipe_out) {
		strncpy(port_info->port_attrib.pipe_out, pipe_out,
			PIPE_NAME_LEN - 1);
		port_info->port_attrib.pipe_out[PIPE_NAME_LEN - 1] = '\0';
	}

	if (get_port_type_enum(port_type) != BF_DPDK_PORT_MAX) {
		port_info->port_attrib.port_type =
				get_port_type_enum(port_type);
	} else {
		port_mgr_log_error
		("%s:%d Incorrect Port Type", __func__, __LINE__);
		return BF_INVALID_ARG;
	}

	switch (port_info->port_attrib.port_type) {
	case BF_DPDK_TAP:
	{
		status = port_config_json_parse_tap_port
			 (port_cjson,
			  &port_info->port_attrib.tap);
		break;
	}
	case BF_DPDK_LINK:
	{
		status = port_config_json_parse_link_port
			 (port_cjson,
			  &port_info->port_attrib.link);
		break;
	}
	case BF_DPDK_SOURCE:
	{
		status = port_config_json_parse_source_port
			 (port_cjson,
			  &port_info->port_attrib.source);
		break;
	}
	case BF_DPDK_SINK:
	{
		status = port_config_json_parse_sink_port
			 (port_cjson,
			  &port_info->port_attrib.sink);
		break;
	}
	case BF_DPDK_RING:
	{
		status = port_config_json_parse_ring_port
				(port_cjson,
				&port_info->port_attrib.ring);
		break;
	}
	default:
		port_mgr_log_error
		("%s:%d Incorrect Port Type", __func__, __LINE__);
		status = BF_INVALID_ARG;
		break;
	}

	if (status != BF_SUCCESS) {
		port_mgr_log_error
		("%s:%d: Failed to parse port attributes in Port Config JSON",
		__func__, __LINE__);
	}
	return status;
}

/**
 * Parse Port Objects from the Port Config JSON File
 * and configure the DPDK Target
 * @param dev_id The Device ID
 * @param root Root cJSON Object
 * @return Status of the API call
 */
static int port_config_json_parse_ports(int dev_id,
					cJSON *root)
{
	struct port_info_t port_info;
	cJSON *ports_cjson = NULL;
	cJSON *port_cjson = NULL;
	int status = BF_SUCCESS;
	int err = 0;

	err |= bf_cjson_get_object
		(root, PORT_CONFIG_JSON_PORTS_NODE, &ports_cjson);

	if (err)
		return BF_UNEXPECTED;

	PORT_CONFIG_JSON_FOR_EACH(port_cjson, ports_cjson) {
		memset(&port_info, 0, sizeof(port_info));

		status = port_config_json_parse_port(dev_id,
						     port_cjson,
						     &port_info);
		if (status != BF_SUCCESS) {
			port_mgr_log_error
			("%s:%d: Failed to parse port from Port Config JSON",
			 __func__, __LINE__);
			return status;
		}

		status = port_mgr_port_add(dev_id,
					   port_info.dev_port,
					   &port_info.port_attrib);

		if (status != BF_SUCCESS) {
			port_mgr_log_error
			("%s:%d: Failed to add port %s",
			 __func__, __LINE__, port_info.port_attrib.port_name);
			return status;
		}
	}
	return BF_SUCCESS;
}

/**
 * Read Port Configuration JSON File
 * and begin parsing
 * @param dev_id The Device ID
 * @param port_config_file Path of the Port Config JSON File
 * @return Status of the API call
 */
static int parse_port_config_json(int dev_id,
				  char *port_config_file)
{
	char *port_config_file_buf;
	struct stat stat_b;
	size_t to_allocate;
	size_t num_items;
	FILE *file;
	cJSON *root;
	int fd, status;

	port_mgr_log_trace("Parsing port config file: %s", port_config_file);

	file = fopen(port_config_file, "r");
	if (!file) {
		port_mgr_log_error
		("%s:%d: Could not open configuration file: %s.\n",
		__func__,
		__LINE__,
		port_config_file);
		goto port_config_file_fopen_err;
	}

	fd = fileno(file);
	fstat(fd, &stat_b);
	to_allocate = stat_b.st_size + 1;

	port_config_file_buf = P4_SDE_CALLOC(1, to_allocate);
	if (!port_config_file_buf) {
		port_mgr_log_error
		("%s:%d: Could not allocate memory for config file buffer.",
		__func__,
		__LINE__);
		goto port_config_file_buf_alloc_err;
	}

	num_items = fread(port_config_file_buf, stat_b.st_size, 1, file);
	if (num_items != 1) {
		if (ferror(file)) {
			port_mgr_log_error
			("%s:%d: Error reading config file buffer",
			__func__,
			__LINE__);
			goto port_config_file_fread_err;
		}

		port_mgr_log_trace
		("%s:%d: End of file reached before expected.",
		__func__,
		__LINE__);
	}

	root = cJSON_Parse(port_config_file_buf);
	if (!root) {
		port_mgr_log_error
		("%s:%d: cJSON error while parsing port config file.",
		__func__,
		__LINE__);
		goto cjson_parse_err;
	}

	status = port_config_json_parse_ports(dev_id, root);
	if (status) {
		port_mgr_log_error
		("%s:%d: Failed to parse ports from Port Config JSON",
		__func__, __LINE__);
		goto ports_parse_err;
	}

	cJSON_Delete(root);
	P4_SDE_FREE(port_config_file_buf);
	fclose(file);
	return BF_SUCCESS;

ports_parse_err:
	cJSON_Delete(root);
cjson_parse_err:
port_config_file_fread_err:
	P4_SDE_FREE(port_config_file_buf);
port_config_file_buf_alloc_err:
	fclose(file);
port_config_file_fopen_err:
	return BF_INVALID_ARG;
}

/**
 * Import Port Configuration From JSON File
 * @param dev_id The Device ID
 * @param dev_profile Device Profile
 * @return Status of the API call
 */
int port_mgr_config_import(int dev_id,
			   struct bf_device_profile *dev_profile)
{
	bf_p4_program_t *p4_program = NULL;
	char *json_file_path = NULL;
	int i = 0, status = BF_SUCCESS;

	port_mgr_log_trace("Enter %s", __func__);

	for (i = 0; i < dev_profile->num_p4_programs; i++) {
		p4_program = &dev_profile->p4_programs[i];
		if (!p4_program) {
			port_mgr_log_error
			("No P4-Program for dev %d",
			dev_id);
			port_mgr_log_trace
			("Exit %s", __func__);
			return BF_INVALID_ARG;
		}

		json_file_path = p4_program->port_config_json;
		port_mgr_log_trace("%s: device %u, file %s",
				   __func__,
				   dev_id,
				   json_file_path);

		if (json_file_path)
			status = parse_port_config_json(dev_id,
							json_file_path);

		if (status != BF_SUCCESS) {
			port_mgr_log_error
			("%s : Error in parsing the port config file",
			__func__);
			port_mgr_log_trace("Exit %s", __func__);
			return BF_NO_SYS_RESOURCES;
		}
	}

	port_mgr_log_trace("Exit %s", __func__);
	return BF_SUCCESS;
}

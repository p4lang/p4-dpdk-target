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


#include <stdlib.h>

#include "port_mgr/dal/dal_port_mgr.h"
#include "port_mgr/port_mgr_port.h"
#include "port_mgr/port_mgr_log.h"

extern enum dpdk_port_type_t get_port_type_enum(char *port_type);
extern bf_pm_port_dir_e get_port_dir_enum(char *port_dir);

bf_status_t port_mgr_decode_key_fields(struct fixed_function_key_spec *key,
                                       struct port_mgr_key_fields *key_fields,
                                       struct fixed_function_table_ctx *tbl_ctx)
{
	struct fixed_function_key_fields *ff_key_fields = NULL;

	ff_key_fields = tbl_ctx->key_fields;

	if (fixed_function_decode_key_spec(key,
				ff_key_fields[DEV_PORT_ID].start_bit,
				ff_key_fields[DEV_PORT_ID].bit_width,
				(u8 *) &key_fields->dev_port) != BF_SUCCESS)
		return BF_INVALID_ARG;

	return BF_SUCCESS;
}

bf_status_t port_mgr_decode_data_fields(struct fixed_function_data_spec *data,
                                        struct port_mgr_data_fields *data_fields,
                                        struct fixed_function_table_ctx *tbl_ctx)
{
	struct fixed_function_data_fields *ff_data_fields = NULL;

	ff_data_fields = tbl_ctx->data_fields;

        if (fixed_function_decode_data_spec(data,
				ff_data_fields[PORT_NAME_ID].start_bit,
				ff_data_fields[PORT_NAME_ID].bit_width,
				(u8 *) data_fields->port_name) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
				ff_data_fields[PORT_TYPE_ID].start_bit,
				ff_data_fields[PORT_TYPE_ID].bit_width,
				(u8 *) data_fields->port_type) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[PORT_DIR_ID].start_bit,
                                         ff_data_fields[PORT_DIR_ID].bit_width,
                                         (u8 *) data_fields->port_dir) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[MTU_ID].start_bit,
                                         ff_data_fields[MTU_ID].bit_width,
                                         (u8 *) &data_fields->mtu) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[PIPE_IN_ID].start_bit,
                                         ff_data_fields[PIPE_IN_ID].bit_width,
                                         (u8 *) data_fields->pipe_in) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[PIPE_OUT_ID].start_bit,
                                         ff_data_fields[PIPE_OUT_ID].bit_width,
                                         (u8 *) data_fields->pipe_out) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[PORT_IN_ID].start_bit,
                                         ff_data_fields[PORT_IN_ID].bit_width,
                                         (u8 *) &data_fields->port_in_id) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[PORT_OUT_ID].start_bit,
                                         ff_data_fields[PORT_OUT_ID].bit_width,
                                         (u8 *) &data_fields->port_out_id) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[MEMPOOL_ID].start_bit,
                                         ff_data_fields[MEMPOOL_ID].bit_width,
                                         (u8 *) data_fields->mempool_name) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[PCIE_BDF_ID].start_bit,
                                         ff_data_fields[PCIE_BDF_ID].bit_width,
                                         (u8 *) data_fields->pcie_domain_bdf) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[FILE_NAME_ID].start_bit,
                                         ff_data_fields[FILE_NAME_ID].bit_width,
                                         (u8 *) data_fields->file_name) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[DEV_ARGS_ID].start_bit,
                                         ff_data_fields[DEV_ARGS_ID].bit_width,
                                         (u8 *) data_fields->dev_args) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[DEV_HOTPLUG_ENABLED_ID].start_bit,
                                         ff_data_fields[DEV_HOTPLUG_ENABLED_ID].bit_width,
                                         (u8 *) &data_fields->dev_hotplug_enabled) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[SIZE_ID].start_bit,
                                         ff_data_fields[SIZE_ID].bit_width,
                                         (u8 *) &data_fields->size) != BF_SUCCESS)
		return BF_INVALID_ARG;

        if (fixed_function_decode_data_spec(data,
                                         ff_data_fields[NET_PORT_ID].start_bit,
                                         ff_data_fields[NET_PORT_ID].bit_width,
                                         (u8 *) &data_fields->net_port) != BF_SUCCESS)
		return BF_INVALID_ARG;

	return BF_SUCCESS;
}

bf_status_t port_mgr_populate_port_attributes(
		struct port_mgr_data_fields *data_fields,
		struct port_attributes_t *port_attr)
{
	if ((data_fields->port_name[0] == '\0') || (data_fields->port_type[0] == '\0')
	    || (data_fields->port_dir[0] == '\0'))
		return BF_INVALID_ARG;

	strncpy(port_attr->port_name, data_fields->port_name, PORT_NAME_LEN - 1);
	port_attr->port_name[PORT_NAME_LEN - 1] = '\0';

        if (get_port_type_enum(data_fields->port_type) != BF_DPDK_PORT_MAX) {
                port_attr->port_type =
                                get_port_type_enum(data_fields->port_type);
        } else {
                port_mgr_log_error
                ("%s:%d Incorrect Port Type", __func__, __LINE__);
                return BF_INVALID_ARG;
        }

        if (get_port_dir_enum(data_fields->port_dir) != PM_PORT_DIR_MAX) {
                port_attr->port_dir = get_port_dir_enum(data_fields->port_dir);
        } else {
                port_mgr_log_error
                ("%s:%d Incorrect Port Direction", __func__, __LINE__);
                return BF_INVALID_ARG;
        }

	port_attr->net_port = data_fields->net_port;

	if (data_fields->mempool_name[0] != '\0') {
		strncpy(port_attr->mempool_name, data_fields->mempool_name,
			MEMPOOL_NAME_LEN - 1);
		port_attr->mempool_name[MEMPOOL_NAME_LEN - 1] = '\0';
	} else {
		if (port_attr->port_type != BF_DPDK_SINK) {
			port_mgr_log_error
			("%s:%d Mempool name is needed", __func__, __LINE__);
			return BF_INVALID_ARG;
		}
	}

	if ((port_attr->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attr->port_dir == PM_PORT_DIR_RX_ONLY)) {
		port_attr->port_in_id = data_fields->port_in_id;
		if (data_fields->pipe_in[0] != '\0') {
			strncpy(port_attr->pipe_in, data_fields->pipe_in,
				PIPE_NAME_LEN - 1);
			port_attr->pipe_in[PIPE_NAME_LEN - 1] = '\0';
		} else {
			port_mgr_log_error
			("%s:%d Pipe In Needed", __func__, __LINE__);
			return BF_INVALID_ARG;
		}
	}

        if ((port_attr->port_dir == PM_PORT_DIR_DEFAULT) ||
            (port_attr->port_dir == PM_PORT_DIR_TX_ONLY)) {
                port_attr->port_out_id = data_fields->port_out_id;
                if (data_fields->pipe_out[0] != '\0') {
                        strncpy(port_attr->pipe_out, data_fields->pipe_out,
				PIPE_NAME_LEN - 1);
                        port_attr->pipe_out[PIPE_NAME_LEN - 1] = '\0';
                } else {
                        port_mgr_log_error
                        ("%s:%d Pipe Out Needed", __func__, __LINE__);
                        return BF_INVALID_ARG;
                }
        }

	switch (port_attr->port_type) {
	case BF_DPDK_TAP:
	{
		port_attr->tap.mtu = data_fields->mtu;
		break;
	}
	case BF_DPDK_LINK:
	{
                if (data_fields->pcie_domain_bdf[0] != '\0') {
                        strncpy(port_attr->link.pcie_domain_bdf,
                                data_fields->pcie_domain_bdf,
                                PCIE_BDF_LEN - 1);
                        port_attr->link.pcie_domain_bdf[PCIE_BDF_LEN - 1]
                                = '\0';
                } else {
                        port_mgr_log_error
                        ("%s:%d Link Port needs PCIE BDF", __func__, __LINE__);
                        return BF_INVALID_ARG;
		}

		if (data_fields->dev_args[0] != '\0') {
			strncpy(port_attr->link.dev_args,
				data_fields->dev_args,
				DEV_ARGS_LEN - 1);
			port_attr->link.dev_args[DEV_ARGS_LEN - 1]
				= '\0';
		}
		port_attr->link.dev_hotplug_enabled =
			data_fields->dev_hotplug_enabled;
		break;
	}
	case BF_DPDK_SOURCE:
	{
                if (data_fields->file_name[0] != '\0') {
                        strncpy(port_attr->source.file_name,
                                data_fields->file_name,
                                PCAP_FILE_NAME_LEN - 1);
                        port_attr->source.file_name[PCAP_FILE_NAME_LEN - 1]
                                = '\0';
                } else {
                        port_mgr_log_error
                        ("%s:%d Source Port needs file name", __func__, __LINE__);
                        return BF_INVALID_ARG;
                }
		break;
	}
	case BF_DPDK_SINK:
	{
                if (data_fields->file_name[0] != '\0') {
                        strncpy(port_attr->sink.file_name,
                                data_fields->file_name,
                                PCAP_FILE_NAME_LEN - 1);
                        port_attr->sink.file_name[PCAP_FILE_NAME_LEN - 1]
                                = '\0';
                } else {
                        port_mgr_log_error
                        ("%s:%d Sink Port needs file name", __func__, __LINE__);
                        return BF_INVALID_ARG;
                }
		break;
	}
	case BF_DPDK_RING:
	{
		port_attr->ring.size = data_fields->size;
		break;
	}
	default:
		port_mgr_log_error
		("%s:%d Incorrect Port Type", __func__, __LINE__);
		return BF_INVALID_ARG;
	}

	return BF_SUCCESS;
}

bf_status_t dal_port_cfg_table_add(bf_dev_id_t dev_id,
                                   struct fixed_function_key_spec *key,
                                   struct fixed_function_data_spec *data,
                                   struct fixed_function_table_ctx *tbl_ctx)
{
	struct port_mgr_data_fields data_fields;
	struct port_mgr_key_fields key_fields;
	struct port_attributes_t port_attr;
	bf_status_t status;

	status = port_mgr_decode_key_fields(key, &key_fields, tbl_ctx);
	if (status != BF_SUCCESS)
		return status;

	status = port_mgr_decode_data_fields(data, &data_fields, tbl_ctx);
	if (status != BF_SUCCESS)
		return status;

	status = port_mgr_populate_port_attributes(&data_fields, &port_attr);
        if (status != BF_SUCCESS)
                return status;

	status = port_mgr_port_add(dev_id, key_fields.dev_port, &port_attr);
	if (status != BF_SUCCESS)
		return status;

	return BF_SUCCESS;
}

/**
 * API to encode port statistics in data spec
 */
bf_status_t port_mgr_encode_data_fields(struct fixed_function_data_spec *data,
					u64 *stats,
					struct fixed_function_table_ctx *tbl_ctx)
{
	struct fixed_function_data_fields *ff_data_fields = NULL;
	int i = 0;

	ff_data_fields = tbl_ctx->data_fields;

	for (i = 0; i < BF_PORT_NUM_COUNTERS; i++) {
		if (fixed_function_encode_data_spec(data,
                                                    ff_data_fields[i].start_bit,
                                                    ff_data_fields[i].bit_width,
                                                    (u8 *)&stats[i]) != BF_SUCCESS)
			return BF_INVALID_ARG;
	}

	return BF_SUCCESS;
}

/**
 * API to retrieve port statistics and encode in data spec
 */
bf_status_t dal_port_stats_get(bf_dev_id_t dev_id,
                               struct fixed_function_key_spec *key,
                               struct fixed_function_data_spec *data,
                               struct fixed_function_table_ctx *tbl_ctx)
{
	struct port_mgr_key_fields key_fields;
	uint64_t stats[BF_PORT_NUM_COUNTERS];
	bf_status_t status;

	memset(stats, 0, BF_PORT_NUM_COUNTERS * sizeof(uint64_t));

	status = port_mgr_decode_key_fields(key, &key_fields, tbl_ctx);
	if (status != BF_SUCCESS)
		return status;

	/* retrieve port statistic */
	status = port_mgr_port_all_stats_get(dev_id, key_fields.dev_port, stats);

	if (status != BF_SUCCESS)
		return status;

	status = port_mgr_encode_data_fields(data, stats, tbl_ctx);
	if (status != BF_SUCCESS)
		return status;

	return status;
}

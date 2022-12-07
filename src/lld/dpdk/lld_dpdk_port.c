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
 * @file lld_dpdk_port.c
 *
 *
 * Definitions for LLD DPDK APIs.
 */

#include "lld_dpdk_port.h"
#include "../lldlib_log.h"
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define BUF_SIZE 512

int write_to_iospec_file(char *buf)
{
	FILE *io_file = NULL;
	int status;

	io_file = fopen(IOSPEC_FILE_PATH, "a+");
	if (!io_file) {
		LOG_ERROR("%s line:%d  Can't open %s file\n"
			  , __func__, __LINE__, IOSPEC_FILE_PATH);
		return BF_INTERNAL_ERROR;
	}

	status = fwrite(buf, 1, strlen(buf), io_file);
	if (!status) {
		LOG_ERROR("%s line:%d  Can't write to %s file\n"
			  , __func__, __LINE__, IOSPEC_FILE_PATH);
		fclose(io_file);
		return BF_INTERNAL_ERROR;
	}
	fclose(io_file);

	return 0;
}
int lld_dpdk_tap_port_create(struct port_attributes_t *port_attrib)
{
	if (!tap_create(port_attrib->port_name)) {
		LOG_ERROR("Creation of Tap Port %s failed\n",
			  port_attrib->port_name);
		return BF_INVALID_ARG;
	}

	return BF_SUCCESS;
}

int lld_dpdk_pipeline_tap_port_add(bf_dev_port_t dev_port,
				   struct port_attributes_t *port_attrib,
				   struct pipeline *pipe_in,
				   struct pipeline *pipe_out,
				   struct mempool *mp)
{
	struct rte_swx_port_fd_reader_params params_in;
	struct rte_swx_port_fd_writer_params params_out;
	struct tap *tap = NULL;
	struct ifreq ifr;
	int sfd = -1;
	int status;
	char buffer[BUF_SIZE];
	memset(&params_in, 0, sizeof(params_in));
	memset(&params_out, 0, sizeof(params_out));

	tap = tap_find(port_attrib->port_name);
	if (!tap) {
		LOG_ERROR("Tap Port %s not found\n", port_attrib->port_name);
		return BF_INVALID_ARG;
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_RX_ONLY)) {
		params_in.fd = tap->fd;
		params_in.mempool = mp->m;
		params_in.mtu = port_attrib->tap.mtu;
		params_in.burst_size = PORT_IN_BURST_SIZE;
		if (port_attrib->tap.mtu > PORT_MTU_MAX) {
			LOG_ERROR("MTU for Tap Port %s is greater than max limit\n",
				   port_attrib->port_name);
			return BF_INVALID_ARG;
		}
		sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
		if (sfd == -1) {
			LOG_ERROR("Socket creation failed\n");
			return BF_INVALID_ARG;
		}
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, port_attrib->port_name, sizeof(ifr.ifr_name) - 1);
		ifr.ifr_mtu = port_attrib->tap.mtu;
		if (ioctl(sfd, SIOCSIFMTU, &ifr) < 0)
			LOG_ERROR("ioctl SIOCSIFMTU Failed...\n");
		close(sfd);

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer),
			 "port in %d fd %d mtu %d mempool %s bsz %d\n",
			 port_attrib->port_in_id, params_in.fd,
			 params_in.mtu, params_in.mempool->name,
			 params_in.burst_size);
		status = write_to_iospec_file(buffer);
		if (status) {
			LOG_ERROR("%s line:%d fail to write to %s file\n"
				  , __func__, __LINE__, IOSPEC_FILE_PATH);
			return BF_INTERNAL_ERROR;
		}
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_TX_ONLY)) {
		params_out.fd = tap->fd;
		params_out.burst_size = PORT_OUT_BURST_SIZE;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "port out %d fd %d bsz %d\n",
			 port_attrib->port_out_id,  params_out.fd,
			 params_out.burst_size);
		status = write_to_iospec_file(buffer);
		if (status) {
			LOG_ERROR("%s line:%d fail to write to %s file\n"
				  , __func__, __LINE__, IOSPEC_FILE_PATH);
			return BF_INTERNAL_ERROR;
		}
	}

	return BF_SUCCESS;
}

int lld_dpdk_tap_port_add(bf_dev_port_t dev_port,
			  struct port_attributes_t *port_attrib,
			  struct pipeline *pipe_in,
			  struct pipeline *pipe_out,
			  struct mempool *mp)
{
	if (lld_dpdk_tap_port_create(port_attrib) != BF_SUCCESS)
		return BF_INVALID_ARG;

	if (lld_dpdk_pipeline_tap_port_add(dev_port, port_attrib, pipe_in,
					   pipe_out, mp) != BF_SUCCESS)
		return BF_INVALID_ARG;

	return BF_SUCCESS;
}

int lld_dpdk_link_port_create(struct port_attributes_t *port_attrib)
{
	struct link_params p;

	memset(&p, 0, sizeof(p));

	p.dev_name = port_attrib->link.pcie_domain_bdf;
	p.dev_args = port_attrib->link.dev_args;
	p.dev_hotplug_enabled = port_attrib->link.dev_hotplug_enabled;
	p.rx.mempool_name = port_attrib->mempool_name;
	p.rx.n_queues = 1;
	p.rx.queue_size = 128;
	p.tx.n_queues = 1;
	p.tx.queue_size = 512;
	p.promiscuous = 1;
	p.rx.rss = NULL;

	if (!link_create(port_attrib->port_name, &p)) {
		LOG_ERROR("Creation of Link Port %s failed\n",
			  port_attrib->port_name);
		return BF_INVALID_ARG;
	}

	return BF_SUCCESS;
}

int lld_dpdk_pipeline_link_port_add(bf_dev_port_t dev_port,
				    struct port_attributes_t *port_attrib,
				    struct pipeline *pipe_in,
				    struct pipeline *pipe_out)
{
	struct rte_swx_port_ethdev_reader_params params_in;
	struct rte_swx_port_ethdev_writer_params params_out;
	struct link *link = NULL;
	int status;
	char buffer[BUF_SIZE];

	memset(&params_in, 0, sizeof(params_in));
	memset(&params_out, 0, sizeof(params_out));

	link = link_find(port_attrib->port_name);
	if (!link) {
		LOG_ERROR("Link Port %s not found\n", port_attrib->port_name);
		return BF_INVALID_ARG;
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_RX_ONLY)) {
		params_in.dev_name = link->dev_name;
		params_in.queue_id = 0;
		params_in.burst_size = PORT_IN_BURST_SIZE;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer),
			"port in %d ethdev %s rxq %d bsz %d\n",
			port_attrib->port_in_id, params_in.dev_name,
			params_in.queue_id, params_in.burst_size);
		status = write_to_iospec_file(buffer);
		if (status) {
			LOG_ERROR("%s line:%d fail to write to %s file\n"
				  , __func__, __LINE__, IOSPEC_FILE_PATH);
			return BF_INTERNAL_ERROR;
		}
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_TX_ONLY)) {
		params_out.dev_name = link->dev_name;
		params_out.queue_id = 0;
		params_out.burst_size = PORT_OUT_BURST_SIZE;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer),
			"port out %d ethdev %s txq %d bsz %d\n",
			port_attrib->port_out_id, params_out.dev_name,
			params_out.queue_id, params_out.burst_size);
		status = write_to_iospec_file(buffer);
		if (status) {
			LOG_ERROR("%s line:%d fail to write to %s file\n"
				  , __func__, __LINE__, IOSPEC_FILE_PATH);
			return BF_INTERNAL_ERROR;
		}
	}

	return BF_SUCCESS;
}

int lld_dpdk_link_port_add(bf_dev_port_t dev_port,
			   struct port_attributes_t *port_attrib,
			   struct pipeline *pipe_in,
			   struct pipeline *pipe_out)
{
	if (lld_dpdk_link_port_create(port_attrib) != BF_SUCCESS)
		return BF_INVALID_ARG;

	if (lld_dpdk_pipeline_link_port_add(dev_port, port_attrib, pipe_in,
					    pipe_out) != BF_SUCCESS)
		return BF_INVALID_ARG;

	return BF_SUCCESS;
}

int lld_dpdk_source_port_add(bf_dev_port_t dev_port,
			     struct port_attributes_t *port_attrib,
			     struct pipeline *pipe_in,
			     struct mempool *mp)
{
	struct rte_swx_port_source_params params;
	int status;
	char buffer[BUF_SIZE];

	memset(&params, 0, sizeof(params));

	params.pool = mp->m;
	params.file_name = port_attrib->source.file_name;

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer),
		 "port in %d source mempool %s file %s loop %ld packets %d\n"
		 , port_attrib->port_in_id, params.pool->name,
		 params.file_name, params.n_loops, params.n_pkts_max);
	status = write_to_iospec_file(buffer);
	if (status) {
		LOG_ERROR("%s line:%d fail to write to %s file\n"
			  , __func__, __LINE__, IOSPEC_FILE_PATH);
		return BF_INTERNAL_ERROR;
	}
	return BF_SUCCESS;
}

int lld_dpdk_sink_port_add(bf_dev_port_t dev_port,
			struct port_attributes_t *port_attrib,
			struct pipeline *pipe_out)
{
	struct rte_swx_port_sink_params params;
	int status;
	char buffer[BUF_SIZE];

	memset(&params, 0, sizeof(params));

	if (!strcmp(port_attrib->sink.file_name, "none"))
		params.file_name = NULL;
	else
		params.file_name = port_attrib->sink.file_name;

	memset(buffer, 0, sizeof(buffer));
	if(params.file_name)
		snprintf(buffer, sizeof(buffer), "port out %d sink file %s\n",
			 port_attrib->port_out_id, params.file_name);
	else
		snprintf(buffer, sizeof(buffer), "port out %d sink file NULL\n",
			 port_attrib->port_out_id);

	status = write_to_iospec_file(buffer);
	if (status) {
		LOG_ERROR("%s line:%d fail to write to %s file\n"
			  , __func__, __LINE__, IOSPEC_FILE_PATH);
		return BF_INTERNAL_ERROR;
	}
	return BF_SUCCESS;
}

int lld_dpdk_ring_port_create(struct port_attributes_t *port_attrib)
{
	struct ring_params p;

	memset(&p, 0, sizeof(p));

	p.size = port_attrib->ring.size;
	p.numa_node = 0;

	if (!ring_create(port_attrib->port_name, &p)) {
		LOG_ERROR("Creation of Ring Port %s failed\n",
			  port_attrib->port_name);
		return BF_INVALID_ARG;
	}

	return BF_SUCCESS;
}

int lld_dpdk_pipeline_ring_port_add(bf_dev_port_t dev_port,
                                    struct port_attributes_t *port_attrib,
                                    struct pipeline *pipe_in,
                                    struct pipeline *pipe_out)
{
	struct rte_swx_port_ring_reader_params params_in;
	struct rte_swx_port_ring_writer_params params_out;
	struct ring *ring = NULL;
	int status;
	char buffer[BUF_SIZE];

        memset(&params_in, 0, sizeof(params_in));
        memset(&params_out, 0, sizeof(params_out));

	ring = ring_find(port_attrib->port_name);
	if (!ring) {
		LOG_ERROR("Ring Port %s not found\n", port_attrib->port_name);
		return BF_INVALID_ARG;
	}

        if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
            (port_attrib->port_dir == PM_PORT_DIR_RX_ONLY)) {
                params_in.name = ring->name;
                params_in.burst_size = PORT_IN_BURST_SIZE;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "port in %d ring %s bsz %d\n",
			 port_attrib->port_in_id, params_in.name,
			 params_in.burst_size);
		status = write_to_iospec_file(buffer);
		if (status) {
			LOG_ERROR("%s line:%d fail to write to %s file\n"
				  , __func__, __LINE__, IOSPEC_FILE_PATH);
			return BF_INTERNAL_ERROR;
		}
        }

        if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
            (port_attrib->port_dir == PM_PORT_DIR_TX_ONLY)) {
                params_out.name = ring->name;
                params_out.burst_size = PORT_OUT_BURST_SIZE;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "port out %d ring %s bsz %d\n"
			 ,port_attrib->port_out_id, params_out.name,
			 params_out.burst_size);
		status = write_to_iospec_file(buffer);
		if (status) {
			LOG_ERROR("%s line:%d fail to write to %s file\n"
				  , __func__, __LINE__, IOSPEC_FILE_PATH);
			return BF_INTERNAL_ERROR;
		}
	}


	return BF_SUCCESS;
}
int lld_dpdk_ring_port_add(bf_dev_port_t dev_port,
                           struct port_attributes_t *port_attrib,
                           struct pipeline *pipe_in,
                           struct pipeline *pipe_out)
{
	if (lld_dpdk_ring_port_create(port_attrib) != BF_SUCCESS)
		return BF_INVALID_ARG;

	if (lld_dpdk_pipeline_ring_port_add(dev_port, port_attrib, pipe_in,
					    pipe_out) != BF_SUCCESS)
		return BF_INVALID_ARG;

	return BF_SUCCESS;
}

int lld_dpdk_port_stats_get(bf_dev_port_t dev_port,
			struct port_attributes_t *port_attrib,
			u64 *stats)
{
	struct pipeline *pipe_in = NULL, *pipe_out = NULL;
	struct rte_swx_port_in_stats in_stats;
	struct rte_swx_port_out_stats out_stats;
	int status;

	memset(&in_stats, 0, sizeof(in_stats));
	memset(&out_stats, 0, sizeof(out_stats));

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_RX_ONLY)) {
		pipe_in = pipeline_find(port_attrib->pipe_in);
		if (!pipe_in || !pipe_in->ctl) {
			LOG_ERROR("Ingress Pipeline %s is not valid\n",
				  port_attrib->pipe_in);
			return BF_INVALID_ARG;
		}

		status = rte_swx_ctl_pipeline_port_in_stats_read(pipe_in->p,
						    port_attrib->port_in_id,
						    &in_stats);

		if (status) {
			LOG_ERROR("Failed to Read Input Stats for Port %s\n",
				  port_attrib->port_name);
			return BF_INVALID_ARG;
		}

		stats[RX_PACKETS] = in_stats.n_pkts;
		stats[RX_BYTES] = in_stats.n_bytes;
		stats[RX_EMPTY_POLLS] = in_stats.n_empty;
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
	    (port_attrib->port_dir == PM_PORT_DIR_TX_ONLY)) {
                pipe_out = pipeline_find(port_attrib->pipe_out);
                if (!pipe_out || !pipe_out->ctl) {
                        LOG_ERROR("Egress Pipeline %s is not valid\n",
                                  port_attrib->pipe_out);
                        return BF_INVALID_ARG;
                }

		status = rte_swx_ctl_pipeline_port_out_stats_read(pipe_out->p,
						    port_attrib->port_out_id,
						    &out_stats);

		if (status) {
			LOG_ERROR("Failed to Read Output Stats for Port %s\n",
				  port_attrib->port_name);
			return BF_INVALID_ARG;
		}

		stats[TX_PACKETS] = out_stats.n_pkts;
		stats[TX_BYTES] = out_stats.n_bytes;
	}

	return BF_SUCCESS;
}

int lld_dpdk_port_add(bf_dev_port_t dev_port,
		      struct port_attributes_t *port_attrib)
{
	int status = BF_SUCCESS;
	struct pipeline *pipe_in = NULL, *pipe_out = NULL;
	struct mempool *mp = NULL;

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
            (port_attrib->port_dir == PM_PORT_DIR_RX_ONLY)) {
		pipe_in = pipeline_find(port_attrib->pipe_in);
		if (!pipe_in || pipe_in->ctl) {
			LOG_ERROR("Pipeline %s is not valid\n",
				  port_attrib->pipe_in);
			return BF_INVALID_ARG;
		}
	}

	if ((port_attrib->port_dir == PM_PORT_DIR_DEFAULT) ||
            (port_attrib->port_dir == PM_PORT_DIR_TX_ONLY)) {
		pipe_out = pipeline_find(port_attrib->pipe_out);
		if (!pipe_out || pipe_out->ctl) {
			LOG_ERROR("Pipeline %s is not valid\n",
				  port_attrib->pipe_out);
			return BF_INVALID_ARG;
		}
	}

	if ((port_attrib->port_type != BF_DPDK_SINK) &&
	    (port_attrib->port_type != BF_DPDK_RING)) {
		mp = mempool_find(port_attrib->mempool_name);
		if (!mp) {
			LOG_ERROR("Mempool %s not found\n",
				  port_attrib->mempool_name);
			return BF_INVALID_ARG;
		}
	}

	if (port_attrib->net_port) {
		if (pipe_in && port_attrib->port_in_id < DIR_REG_ARRAY_SIZE)
			pipe_in->net_port_mask[port_attrib->port_in_id / 64] |=
				1ULL << port_attrib->port_in_id;
		if (pipe_out && port_attrib->port_out_id < DIR_REG_ARRAY_SIZE)
			pipe_out->net_port_mask[port_attrib->port_out_id / 64]
				|= 1ULL << port_attrib->port_out_id;
	}

	switch (port_attrib->port_type) {
	case BF_DPDK_TAP:
	{
		status = lld_dpdk_tap_port_add(dev_port, port_attrib, pipe_in,
					       pipe_out, mp);
		break;
	}
	case BF_DPDK_LINK:
	{
		status = lld_dpdk_link_port_add(dev_port, port_attrib, pipe_in,
						pipe_out);
		break;
	}
	case BF_DPDK_SOURCE:
	{
		status = lld_dpdk_source_port_add(dev_port, port_attrib,
						  pipe_in, mp);
		break;
	}
	case BF_DPDK_SINK:
	{
		status = lld_dpdk_sink_port_add(dev_port, port_attrib,
						pipe_out);
		break;
	}
	case BF_DPDK_RING:
	{
		status = lld_dpdk_ring_port_add(dev_port, port_attrib, pipe_in,
						pipe_out);
		break;
	}
	default:
		LOG_TRACE("%s:%d Incorrect Port Type", __func__, __LINE__);
		status = BF_INVALID_ARG;
		break;
	}

	return status;
}

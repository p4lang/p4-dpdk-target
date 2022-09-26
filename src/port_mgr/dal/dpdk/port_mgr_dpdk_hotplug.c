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
 * @file port_mgr_dpdk_port.c
 *
 *
 * Definitions for DPDK Port Manager APIs.
 */

#include "port_mgr/port_mgr_hotplug.h"
#include "port_mgr/port_mgr_log.h"
#include "port_mgr/port_mgr.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

typedef enum qemu_cmd_type {
        CHARDEV_ADD,
        NETDEV_ADD,
        DEVICE_ADD,
        CHARDEV_DEL,
        NETDEV_DEL,
        DEVICE_DEL
} qemu_cmd_type;

#define CMD_BUFFER_LEN 512
#define RECV_BUFFER_LEN 512

bf_status_t SendQemuCmdsHelper(int sockfd,
                               char *cmd) {
	char recvBuff[RECV_BUFFER_LEN];
	int sock_ret = 0, n = 0;

	memset(recvBuff, 0, sizeof(recvBuff));
	sock_ret = write(sockfd, cmd, strlen(cmd));
	if(!sock_ret) {
		port_mgr_log_error("QEMU error while writing, sock_ret = %d\n", sock_ret);
		return BF_INTERNAL_ERROR;
        }

	while ((n = read(sockfd, recvBuff,sizeof(recvBuff)-1)) > 0)
	{
		recvBuff[n] = '\0';
		if (strstr(recvBuff, "Error") || strstr(recvBuff, "Duplicate")) {
			port_mgr_log_error("QEMU error encountered, QEMU returned %s", recvBuff);
			return BF_INTERNAL_ERROR;
	        }
	}
	return BF_SUCCESS;
}

void PrepQemuCmdsHelper(qemu_cmd_type cmd_type,
                        struct hotplug_attributes_t *hotplug_attrib, char *qemu_cmd) {

	switch(cmd_type) {
	case CHARDEV_ADD:
		snprintf(qemu_cmd, CMD_BUFFER_LEN, "chardev-add socket,id=%s,path=%s\n",
			 hotplug_attrib->qemu_vm_chardev_id,
			 hotplug_attrib->native_socket_path);
		break;

	case NETDEV_ADD:
		snprintf(qemu_cmd, CMD_BUFFER_LEN, "netdev_add type=vhost-user,id=%s,chardev=%s,vhostforce\n",
			 hotplug_attrib->qemu_vm_netdev_id,
			 hotplug_attrib->qemu_vm_chardev_id);
		break;

	case DEVICE_ADD:
		snprintf(qemu_cmd, CMD_BUFFER_LEN, "device_add virtio-net-pci,mac=%s,netdev=%s,id=%s\n",
			 hotplug_attrib->qemu_vm_mac_address,
			 hotplug_attrib->qemu_vm_netdev_id,
			 hotplug_attrib->qemu_vm_device_id);
		break;

	case CHARDEV_DEL:
		snprintf(qemu_cmd, CMD_BUFFER_LEN, "chardev_remove %s\n",
			 hotplug_attrib->qemu_vm_chardev_id);
		break;

	case NETDEV_DEL:
		snprintf(qemu_cmd, CMD_BUFFER_LEN, "netdev_del %s\n",
			 hotplug_attrib->qemu_vm_netdev_id);
		break;

	case DEVICE_DEL:
		snprintf(qemu_cmd, CMD_BUFFER_LEN, "device_del %s\n",
			 hotplug_attrib->qemu_vm_device_id);
		break;

	default:
		break;
	}

	port_mgr_log_debug("Qemu cmd is %s",qemu_cmd);
	return;
}

int CreateHelperSocket(struct hotplug_attributes_t *hotplug_attrib) {
	struct sockaddr_in serv_addr;
	struct timeval tv;
	int sockfd = 0;

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		port_mgr_log_error("Failed to create socket to connect to Qemu monitor socket \n");
		return -1;
	}

	/* Specifying the timeout of 1 second, if qemu is not responding. Without this
	 * timeout socket will block if qemu doesn't respond
	 */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_addr.s_addr = inet_addr(hotplug_attrib->qemu_socket_ip);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(hotplug_attrib->qemu_socket_port);

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		close(sockfd);
		port_mgr_log_error("Failed to connect to Qemu monitor socket \n");
		return -1;
	}
	return sockfd;
}

bf_status_t port_mgr_hotplug_add(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 struct hotplug_attributes_t *hotplug_attrib)
{
	char cmd[CMD_BUFFER_LEN] = {0};
	bf_status_t status = BF_SUCCESS;

	int sockfd = CreateHelperSocket(hotplug_attrib);
	if (sockfd == -1) {
		port_mgr_log_error("Unable to qemu hoplug the device due to socket connection error \n");
		return BF_INTERNAL_ERROR;
	}

	PrepQemuCmdsHelper(CHARDEV_ADD, hotplug_attrib, cmd);
	status = SendQemuCmdsHelper(sockfd, cmd);
	if (status != BF_SUCCESS) {
		close(sockfd);
		port_mgr_log_error("Failed to hotplug the port due to QEMU error when adding character device \n");
		return BF_INTERNAL_ERROR;
	}

	PrepQemuCmdsHelper(NETDEV_ADD, hotplug_attrib, cmd);
	status = SendQemuCmdsHelper(sockfd, cmd);
	if (status != BF_SUCCESS) {
		// Best effort to remove character dev created before
		PrepQemuCmdsHelper(CHARDEV_DEL, hotplug_attrib, cmd);
		SendQemuCmdsHelper(sockfd, cmd);
		close(sockfd);
		port_mgr_log_error("Failed to hotplug the port due to QEMU error when adding netdev device \n");
		return BF_INTERNAL_ERROR;
	}

	PrepQemuCmdsHelper(DEVICE_ADD, hotplug_attrib, cmd);
	status = SendQemuCmdsHelper(sockfd, cmd);
	if (status != BF_SUCCESS) {
		PrepQemuCmdsHelper(NETDEV_DEL, hotplug_attrib, cmd);
		SendQemuCmdsHelper(sockfd, cmd);
		close(sockfd);
		port_mgr_log_error("Failed to hotplug the port due to QEMU error when adding device\n");
		return BF_INTERNAL_ERROR;
	}
	close(sockfd);

	return status;
}

bf_status_t port_mgr_hotplug_del(bf_dev_id_t dev_id, bf_dev_port_t dev_port,
                               struct hotplug_attributes_t *hotplug_attrib)

{
	bf_status_t status = BF_SUCCESS;
	char cmd[CMD_BUFFER_LEN] = {0};

	int sockfd = CreateHelperSocket(hotplug_attrib);
	if (sockfd == -1) {
		port_mgr_log_error("Unable to qemu hotplug the device due to socket connection error");
		return BF_INTERNAL_ERROR;
	}


	PrepQemuCmdsHelper(DEVICE_DEL, hotplug_attrib, cmd);
	status = SendQemuCmdsHelper(sockfd, cmd);
	if (status != BF_SUCCESS) {
		close(sockfd);
		port_mgr_log_error("Failed to delete the hotplugged port due to QEMU error when deleting device \n");
		return BF_INTERNAL_ERROR;
	}

	PrepQemuCmdsHelper(NETDEV_DEL, hotplug_attrib, cmd);
	status = SendQemuCmdsHelper(sockfd, cmd);
	if (status != BF_SUCCESS) {
		close(sockfd);
		port_mgr_log_error("Failed to delete the hotplugged port due to QEMU error when deleting netdev \n");
		return BF_INTERNAL_ERROR;
	}
	close(sockfd);
	return status;
}

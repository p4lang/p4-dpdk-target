/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_PORT_IF_H_INCLUDED
#define BF_PORT_IF_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <bf_types/bf_types.h>

#include "port_mgr/dpdk/bf_dpdk_port_if.h"

#define IOSPEC_FILE_PATH "/tmp/iospec.io"
int write_to_iospec_file(char *buf);

/**
 * @file bf_port_if.h
 * \brief Details Port-level APIs.
 *
 */

/** \brief   Enumeration of supported port direction configuration modes.
 */
typedef enum {
  BF_PORT_DIR_DUPLEX = 0,
  BF_PORT_DIR_TX_ONLY,
  BF_PORT_DIR_RX_ONLY,
  BF_PORT_DIR_MAX
} bf_port_dir_e;

/**
 * Port Info Structure
 */
struct port_info_t {
  bf_dev_port_t dev_port;               /*!< Port ID */
  struct port_attributes_t port_attrib; /*!< Port Attributes */
};

/**
 * Enum identifying Port Counters
 */
enum port_counters_t {
	RX_BYTES,               /*!< RX Bytes */
	RX_PACKETS,             /*!< RX Packets */
	RX_UNICAST,             /*!< RX Unicast Packets */
	RX_MULTICAST,           /*!< RX Multicast Packets */
	RX_BROADCAST,           /*!< RX Broadcast Packets */
	RX_DISCARDS,            /*!< RX Discards */
	RX_ERRORS,              /*!< RX Errors */
	RX_EMPTY_POLLS,         /*!< RX Empty Polls */
	TX_BYTES,               /*!< TX Bytes */
	TX_PACKETS,             /*!< TX Packets */
	TX_UNICAST,             /*!< TX Unicast Packets */
	TX_MULTICAST,           /*!< TX Multicast Packets */
	TX_BROADCAST,           /*!< TX Broadcast Packets */
	TX_DISCARDS,            /*!< TX Discards */
	TX_ERRORS,              /*!< TX Errors */
	BF_PORT_NUM_COUNTERS,    /*!< Total Number of Counters */
};

bf_status_t bf_port_mgr_init(void);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // BF_PORT_IF_H_INCLUDED

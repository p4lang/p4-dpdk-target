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
#ifndef _BF_TYPES_H
#define _BF_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file bf_types.h
 * \brief Details bf specific types.
 */
/**
 * @addtogroup bf_types-bf_types
 * @{
 */

/*! Identifies an ASIC in the system. */
typedef int bf_dev_id_t;

/*! Identifies a specific die within an ASIC in the system. */
typedef int bf_subdev_id_t;

/** Identifies a port on an ASIC.  This is a 9-bit field where the upper two
 *  bits identify the pipeline and the lower 7 bits identify the port number
 *  local to that pipeline.  The valid range for the lower 7 bits is 0-71. */
typedef int bf_dev_port_t;

/** Identifies a pipe on an ASIC.  This is a 2-bit field where the bits identify
 * pipeline.
 */
typedef uint32_t bf_dev_pipe_t;

/** Identifies an invalid value for a pipeline on an ASIC. */
#define BF_INVALID_PIPE 0xFFFFFFFF

/** Identifies a pipeline on an ASIC.  Note dev_pipe_id can be set to
 *  BF_DEV_PIPE_ALL as a special value to indicate "all pipelines". */
#define BF_DEV_PIPE_ALL 0xFFFF

#define DEV_PORT_TO_PIPE(x) (((x) >> 7) & 3)
#define MAKE_DEV_PORT(pipe, port) (((pipe) << 7) | (port))

typedef struct bf_dev_target_t {
  bf_dev_id_t device_id;
  bf_dev_pipe_t dev_pipe_id;
} bf_dev_target_t;

/** Strcture for direction definition
 */
typedef enum {
  BF_DEV_DIR_INGRESS = 0,
  BF_DEV_DIR_EGRESS = 1,
  BF_DEV_DIR_ALL = 0xff
} bf_dev_direction_t;

/** Wildcard to specify all parser engines in a pipe */
#define BF_DEV_PIPE_PARSER_ALL 0xFF

/** Identifies an error code. */
typedef int bf_status_t;

#define BF_STATUS_VALUES                                                    \
  BF_STATUS_(BF_SUCCESS, "Success"), BF_STATUS_(BF_NOT_READY, "Not ready"), \
      BF_STATUS_(BF_NO_SYS_RESOURCES, "No system resources"),               \
      BF_STATUS_(BF_INVALID_ARG, "Invalid arguments"),                      \
      BF_STATUS_(BF_ALREADY_EXISTS, "Already exists"),                      \
      BF_STATUS_(BF_HW_COMM_FAIL, "HW access fails"),                       \
      BF_STATUS_(BF_OBJECT_NOT_FOUND, "Object not found"),                  \
      BF_STATUS_(BF_MAX_SESSIONS_EXCEEDED, "Max sessions exceeded"),        \
      BF_STATUS_(BF_SESSION_NOT_FOUND, "Session not found"),                \
      BF_STATUS_(BF_NO_SPACE, "Not enough space"),                          \
      BF_STATUS_(BF_EAGAIN,                                                 \
                 "Resource temporarily not available, try again later"),    \
      BF_STATUS_(BF_INIT_ERROR, "Initialization error"),                    \
      BF_STATUS_(BF_TXN_NOT_SUPPORTED, "Not supported in transaction"),     \
      BF_STATUS_(BF_TABLE_LOCKED, "Resource held by another session"),      \
      BF_STATUS_(BF_IO, "IO error"),                                        \
      BF_STATUS_(BF_UNEXPECTED, "Unexpected error"),                        \
      BF_STATUS_(BF_ENTRY_REFERENCES_EXIST,                                 \
                 "Action data entry is being referenced by match entries"), \
      BF_STATUS_(BF_NOT_SUPPORTED, "Operation not supported"),              \
      BF_STATUS_(BF_HW_UPDATE_FAILED, "Updating hardware failed"),          \
      BF_STATUS_(BF_NO_LEARN_CLIENTS, "No learning clients registered"),    \
      BF_STATUS_(BF_IDLE_UPDATE_IN_PROGRESS,                                \
                 "Idle time update state already in progress"),             \
      BF_STATUS_(BF_DEVICE_LOCKED, "Device locked"),                        \
      BF_STATUS_(BF_INTERNAL_ERROR, "Internal error"),                      \
      BF_STATUS_(BF_TABLE_NOT_FOUND, "Table not found"),                    \
      BF_STATUS_(BF_IN_USE, "In use"),                                      \
      BF_STATUS_(BF_NOT_IMPLEMENTED, "Object not implemented")
enum bf_status_enum {
#define BF_STATUS_(x, y) x
  BF_STATUS_VALUES,
  BF_STS_MAX
#undef BF_STATUS_
};
static const char *bf_err_strings[BF_STS_MAX + 1] = {
#define BF_STATUS_(x, y) y
    BF_STATUS_VALUES, "Unknown error"
#undef BF_STATUS_
};
static inline const char *bf_err_str(bf_status_t sts) {
  if (BF_STS_MAX <= sts || 0 > sts) {
    return bf_err_strings[BF_STS_MAX];
  } else {
    return bf_err_strings[sts];
  }
}

/** The max number of devices in the domain of the driver */
#define BF_MAX_DEV_COUNT 1
/** The number of pipes in the ASIC. */
#define BF_PIPE_COUNT 4
/** The number of ports per pipe in the ASIC. */
#define BF_PIPE_PORT_COUNT 4
/** The number of ports in the ASIC. */
#define BF_PORT_COUNT (BF_PIPE_PORT_COUNT * BF_PIPE_COUNT)

/**
 * @}
 */

typedef enum bf_dev_family_t {
  BF_DEV_FAMILY_DPDK,
  BF_DEV_FAMILY_UNKNOWN,
} bf_dev_family_t;

static inline const char *bf_dev_family_str(bf_dev_family_t fam) {
  switch (fam) {
    case BF_DEV_FAMILY_DPDK:
      return "DPDK";
    case BF_DEV_FAMILY_UNKNOWN:
      return "Unknown";
  }
  return "Unknown";
}

typedef enum bf_dev_flag_e {
  BF_DEV_IS_SW_MODEL = (1 << 0),
  BF_DEV_IS_VIRTUAL_DEV_SLAVE = (1 << 1),
} bf_dev_flag_t;

typedef uint32_t bf_dev_flags_t;

#define BF_DEV_IS_SW_MODEL_GET(flags) \
  ((flags & BF_DEV_IS_SW_MODEL) ? true : false)
#define BF_DEV_IS_SW_MODEL_SET(flags, value) \
  (flags |= (value & BF_DEV_IS_SW_MODEL))

#define BF_DEV_IS_VIRTUAL_DEV_SLAVE_GET(flags) \
  ((flags & BF_DEV_IS_VIRTUAL_DEV_SLAVE) ? true : false)
#define BF_DEV_IS_VIRTUAL_DEV_SLAVE_SET(flags) \
  (flags |= BF_DEV_IS_VIRTUAL_DEV_SLAVE)

#define P4_SDE_TABLE_NAME_LEN 64
#define P4_SDE_NAME_LEN 64
#define P4_SDE_PROG_NAME_LEN 50
#define P4_SDE_VERSION_LEN 3
#define P4_SDE_MAX_SESSIONS 16
#define P4_SDE_NAME_SUFFIX 16
#define P4_SDE_ARCH_NAME_LEN 4

#define MAC_FORMAT "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"
#define MAC_FORMAT_VALUE(var) \
        (int)var[0], \
        (int)var[1], \
        (int)var[2], \
        (int)var[3], \
        (int)var[4], \
        (int)var[5]

#define IP_FORMAT "%u.%u.%u.%u"
#define IP_FORMAT_VALUE(var) \
        ((unsigned char*)&var)[3], \
        ((unsigned char*)&var)[2], \
        ((unsigned char*)&var)[1], \
        ((unsigned char*)&var)[0]

#endif

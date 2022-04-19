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
#include <dvm/bf_drv_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/port_mgr.h>
#include <port_mgr/port_mgr_port.h>
#include <lld/lld_err.h>
#include <target-utils/map/map.h>
#include "bf_pal_log.h"

static bf_map_t bf_pm_port_map_db[BF_MAX_DEV_COUNT];

struct dev_port {
	u32 conn_id; /* port ID */
	u32 chnl_id;
};

struct bf_pal_port_info {
	bf_dev_port_t dev_port;
};

#define MAX_NUM_CONN 64
#define MAX_NUM_CHNL 4

static int bf_pal_port_str_to_info(const char *str,
				   u32 *conn,
				   u32 *chnl)
{
  int len, i;
  char *tmp_str, *ptr;
  int ret = 0;

  len = strlen(str);
  tmp_str = (char *)malloc(len + 1);
  if (tmp_str == NULL) {
    return -1;
  }
  memcpy(tmp_str, str, len);
  tmp_str[len] = '\0';

  ptr = tmp_str;
  i = 0;
  while (*ptr != '/' && i < len) {
    ptr++;
    i++;
  }
  if (i == len) {
    ret = -1;
    goto end;
  }
  *ptr = '\0';
  if (*tmp_str == '-') {
    *conn = -1;
  } else {
    *conn = strtoul(tmp_str, NULL, 10);
  }
  ptr++;
  if (*ptr == '-') {
    *chnl = -1;
  } else {
    *chnl = strtoul(ptr, NULL, 10);
  }

end:
  free(tmp_str);
  return ret;
}

/**
 * Given a port to be used to index the port database map
 */
static inline unsigned long pm_port_info_map_key_get(u32 conn_id, u32 chnl_id)
{
#define CHNLS_PER_PORT 4
	return ((conn_id << CHNLS_PER_PORT) | (chnl_id));
}

/**
 * pm_port_info_get_from_port_hdl
 *
 * Look up the given conn/chnl (port_hdl) in the DB,
 * across all devices
 */
static struct bf_pal_port_info *pm_port_info_get_from_port_map
		(bf_dev_id_t dev_id, u32 conn_id, u32 chnl_id)
{
	bf_map_sts_t map_sts;
	struct bf_pal_port_info *port_info = NULL;
	unsigned long port_key;

	port_key = pm_port_info_map_key_get(conn_id, chnl_id);
	map_sts = bf_map_get(&bf_pm_port_map_db[dev_id], port_key,
			     (void **)&port_info);
	if (map_sts == BF_MAP_OK)
		return port_info;
	return NULL;
}

/**
 * Build information of all the ports
 */
bf_status_t bf_pal_create_port_info(bf_dev_id_t dev_id)
{
	struct bf_pal_port_info *port_info;
	bf_dev_port_t dev_port;
	bf_map_sts_t map_sts;
	unsigned long port_key = 0;
	int chnl_cnt, conn_cnt;
	for (conn_cnt = 1; conn_cnt <= MAX_NUM_CONN; conn_cnt++) {
		for (chnl_cnt = 0; chnl_cnt <= MAX_NUM_CHNL; chnl_cnt++) {
			dev_port = ((conn_cnt - 1) * 4) + chnl_cnt;

			port_info =
				(struct bf_pal_port_info *)P4_SDE_CALLOC(1, 1 *
						sizeof(struct bf_pal_port_info));
			if (!port_info) {
				return BF_NO_SYS_RESOURCES;
			}
			port_info->dev_port = dev_port;
			port_key = pm_port_info_map_key_get(conn_cnt, chnl_cnt);
			map_sts = bf_map_add(&bf_pm_port_map_db[dev_id],
					port_key,
					(void *)port_info);
			if (map_sts != BF_MAP_OK) {
				return map_sts;
			}
		}
	}
	return BF_SUCCESS;
}

bf_status_t bf_pal_port_add(bf_dev_id_t dev_id, bf_dev_port_t dev_port,
			    struct port_attributes_t *port_attrib)
{
	return port_mgr_port_add(dev_id, dev_port, port_attrib);
}

bf_status_t bf_pal_port_del(bf_dev_id_t dev_id, bf_dev_port_t dev_port)
{
	return BF_SUCCESS;
}

bf_status_t bf_pal_port_all_stats_get(bf_dev_id_t dev_id,
				      bf_dev_port_t dev_port,
				      u64 *stats)
{
	return port_mgr_port_all_stats_get(dev_id, dev_port, stats);
}

bf_status_t bf_pal_get_port_id_from_mac(bf_dev_id_t dev_id, char *mac,
					u32 *port_id)
{
	return port_mgr_get_port_id_from_mac(dev_id, mac, port_id);
}

bf_status_t bf_pal_get_port_id_from_name(bf_dev_id_t dev_id, char *port_name,
					 u32 *port_id)
{
	return port_mgr_get_port_id_from_name(dev_id, port_name, port_id);
}

bf_status_t bf_pal_port_str_to_dev_port_map(bf_dev_id_t dev_id,
					    char *port_str,
					    bf_dev_port_t *dev_port)
{
	u32 conn = 0, chnl = 0;
	struct bf_pal_port_info *port_info = NULL;

	bf_pal_port_str_to_info(port_str, &conn, &chnl);
	port_info = pm_port_info_get_from_port_map(dev_id, conn, chnl);
	if (port_info) {
		*dev_port = port_info->dev_port;
		return BF_SUCCESS;
	}
	return BF_UNEXPECTED;
}

bf_status_t bf_pal_port_info_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port,
				 struct port_info_t **port_info)
{
	/* Get the Port Info from Hash Map */
	*port_info = port_mgr_get_port_info(dev_port);
	if (!port_info)
		return BF_OBJECT_NOT_FOUND;

	return BF_SUCCESS;
}

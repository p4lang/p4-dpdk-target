/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DAL_PORT_MGR_H__
#define __DAL_PORT_MGR_H__

#include "fixed_function/fixed_function_int.h"

bf_status_t dal_port_cfg_table_add(bf_dev_id_t dev_id,
                                   struct fixed_function_key_spec *key,
                                   struct fixed_function_data_spec *data,
                                   struct fixed_function_table_ctx *tbl_ctx);

/**
 * API to retrieve port statistics
 *
 * @param  dev_id       Device id.
 * @param  key          key  spec
 * @param  data         data spec
 * @param  tbl_ctx      table context
 * @return              Status of the API call
 */
bf_status_t dal_port_stats_get(bf_dev_id_t dev_id,
                               struct fixed_function_key_spec *key,
                               struct fixed_function_data_spec *data,
                               struct fixed_function_table_ctx *tbl_ctx);
#endif /* __DAL_PORT_MGR_H__ */

/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef port_mgr_intf_included
#define port_mgr_intf_included

/**
 * Initialize Port Manager
 *
 * @param void
 * @return void
 */
void port_mgr_init(void);

/**
 * Clean up Port Manager
 *
 * @param void
 * @return void
 */
void port_mgr_cleanup(void);

#endif  // port_mgr_intf_included

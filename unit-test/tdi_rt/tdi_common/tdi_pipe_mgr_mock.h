/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

MOCK_GLOBAL_FUNC1(pipe_mgr_client_init, int (u32 *sess_hdl));
MOCK_GLOBAL_FUNC1(pipe_mgr_client_cleanup, int (u32 sess_hdl));

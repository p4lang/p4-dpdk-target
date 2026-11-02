/*
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 * Copyright (C) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FIXED_FUNCTION_INIT_H__
#define __FIXED_FUNCTION_INIT_H__

/**
 * Initialize Fixed Function Manager
 *
 * @param void
 * @return int Status of the function call
 */
int fixed_function_init(void);

/**
 * Clean up Fixed Function Manager
 *
 * @param void
 * @return void
 */
void fixed_function_cleanup(void);


#endif  // __FIXED_FUNCTION_INIT_H__

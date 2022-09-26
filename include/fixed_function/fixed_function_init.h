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

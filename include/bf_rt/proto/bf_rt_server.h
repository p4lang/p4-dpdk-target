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
/** @file bf_rt_server.h
 *
 *  @brief Contains BF-RT gRPC server APIs
 */
#ifndef _BF_RT_SERVER_H
#define _BF_RT_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start server and bind to default address (0.0.0.0:50052)
 *
 * @param[in] program_name      P4 program name to use
 * @param[in] local_only        Specifies if server should bind to local
 *                              loopback interface only
 */
void bf_rt_grpc_server_run(const char *program_name, bool local_only);

/**
 * @brief Start server and bind specified address
 *
 * @param[in] server_address    Server address to bind to represented as string
 *                              in format ip:port
 */
void bf_rt_grpc_server_run_with_addr(const char *server_address);

#ifdef __cplusplus
}
#endif

#endif  // _BF_RT_SERVER_H

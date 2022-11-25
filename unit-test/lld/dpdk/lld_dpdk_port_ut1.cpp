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

/*Each testcase file can atmost have 5k checks
 *Note: Please update the number of checks included in the below field
 *Number of checks = 5*/

#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include <gmock-global.h>

extern "C"{
    #include "lld_dpdk_port.c"
    #include "mock.h"
    #include "../mock_lld_dpdk_port.h"
}

using namespace std;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;

TEST(NetPortMask, case0) {
   struct pipeline *pipe_in, *pipe_out, pipe;
    struct port_attributes_t port_attrib;
    struct mempool mempool;
    bf_dev_port_t dev_port;
    int actual_result, expected_result, ret;
    memset(&pipe, 0, sizeof(struct pipeline));
    strcpy(port_attrib.pipe_in,"pipe");
    strcpy(port_attrib.pipe_out,"pipe");
    port_attrib.port_dir = PM_PORT_DIR_DEFAULT;
    port_attrib.port_type = (enum dpdk_port_type_t)-1;
    port_attrib.port_in_id = 0;
    port_attrib.port_out_id = 0;
    port_attrib.net_port = 1;
    dev_port = 0;
    pipe_in = &pipe;
    pipe_out = &pipe;

    EXPECT_GLOBAL_CALL(pipeline_find, pipeline_find(_))
	    .Times(2).
	    WillOnce(Return(&pipe)).
	    WillOnce(Return(&pipe));
    EXPECT_GLOBAL_CALL(mempool_find, mempool_find(_))
	    .Times(1).
	    WillOnce(Return(&mempool));

    //Function call
    ret = lld_dpdk_port_add(dev_port, &port_attrib);
    expected_result = 1 << dev_port;
    actual_result = pipe_in->net_port_mask[0];

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);

}

TEST(NetPortMask, case1) {
   struct pipeline *pipe_in, *pipe_out, pipe;
    struct port_attributes_t port_attrib;
    struct mempool mempool;
    bf_dev_port_t dev_port;
    int actual_result, expected_result, ret;
    memset(&pipe, 0, sizeof(struct pipeline));
    strcpy(port_attrib.pipe_in,"pipe");
    strcpy(port_attrib.pipe_out,"pipe");
    port_attrib.port_dir = PM_PORT_DIR_DEFAULT;
    port_attrib.port_type = (enum dpdk_port_type_t)-1;
    port_attrib.port_in_id = 2;
    port_attrib.port_out_id = 2;
    port_attrib.net_port = 1;
    dev_port = 2;
    pipe_in = &pipe;
    pipe_out = &pipe;

    EXPECT_GLOBAL_CALL(pipeline_find, pipeline_find(_))
	    .Times(2).
	    WillOnce(Return(&pipe)).
	    WillOnce(Return(&pipe));
    EXPECT_GLOBAL_CALL(mempool_find, mempool_find(_))
	    .Times(1).
	    WillOnce(Return(&mempool));

    //Function call
    ret = lld_dpdk_port_add(dev_port, &port_attrib);
    expected_result = 1 << dev_port;
    actual_result = pipe_in->net_port_mask[0];

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);

}

TEST(NetPortMask, case2) {
   struct pipeline *pipe_in, *pipe_out, pipe;
    struct port_attributes_t port_attrib;
    struct mempool mempool;
    bf_dev_port_t dev_port;
    int actual_result, expected_result, ret;
    memset(&pipe, 0, sizeof(struct pipeline));
    strcpy(port_attrib.pipe_in,"pipe");
    strcpy(port_attrib.pipe_out,"pipe");
    port_attrib.port_dir = PM_PORT_DIR_DEFAULT;
    port_attrib.port_type = (enum dpdk_port_type_t)-1;
    port_attrib.port_in_id = 4;
    port_attrib.port_out_id = 4;
    port_attrib.net_port = 0;
    dev_port = 4;
    pipe_in = &pipe;
    pipe_out = &pipe;

    EXPECT_GLOBAL_CALL(pipeline_find, pipeline_find(_))
	    .Times(2).
	    WillOnce(Return(&pipe)).
	    WillOnce(Return(&pipe));
    EXPECT_GLOBAL_CALL(mempool_find, mempool_find(_))
	    .Times(1).
	    WillOnce(Return(&mempool));

    //Function call
    ret = lld_dpdk_port_add(dev_port, &port_attrib);
    expected_result = 0;
    actual_result = pipe_in->net_port_mask[0];

    //Macro checking expected result against the actual result
    ASSERT_EQ(actual_result, expected_result);

}

TEST(CompileMode, case0) {
   struct pipeline *pipe_in, *pipe_out, pipe;
    struct port_attributes_t port_attrib;
    struct mempool mempool;
    struct rte_mempool m;
    struct tap tap;
    bf_dev_port_t dev_port;
    int expected_result, ret;
    memset(&pipe, 0, sizeof(struct pipeline));
    mempool.m = &m;
    strcpy(mempool.m->name,"MEMPOOL2");
    port_attrib.port_dir = PM_PORT_DIR_DEFAULT;
    port_attrib.port_in_id = 0;
    port_attrib.port_out_id = 0;
    port_attrib.tap.mtu = 1500;
    tap.fd = 32;
    dev_port = 0;
    pipe_in = &pipe;
    pipe_out = &pipe;

    EXPECT_GLOBAL_CALL(tap_find, tap_find(_))
	    .Times(1).
	    WillOnce(Return(&tap));

    //Function call
    ret = lld_dpdk_pipeline_tap_port_add(dev_port, &port_attrib, pipe_in, pipe_out, &mempool);
    expected_result = 0;

    //Macro checking expected result against the actual result
    ASSERT_EQ(ret, expected_result);

}

TEST(CompileMode, case1) {
   struct pipeline *pipe_in, *pipe_out, pipe;
    struct port_attributes_t port_attrib;
    struct link link;
    bf_dev_port_t dev_port;
    int expected_result, ret;
    memset(&pipe, 0, sizeof(struct pipeline));
    strcpy(link.dev_name,"link");
    port_attrib.port_dir = PM_PORT_DIR_DEFAULT;
    port_attrib.port_in_id = 0;
    port_attrib.port_out_id = 0;
    dev_port = 0;
    pipe_in = &pipe;
    pipe_out = &pipe;

    EXPECT_GLOBAL_CALL(link_find, link_find(_))
	    .Times(1).
	    WillOnce(Return(&link));

    //Function call
    ret = lld_dpdk_pipeline_link_port_add(dev_port, &port_attrib, pipe_in, pipe_out);
    expected_result = 0;

    //Macro checking expected result against the actual result
    ASSERT_EQ(ret, expected_result);

}

TEST(CompileMode, case2) {
    struct pipeline *pipe_in, pipe;
    struct port_attributes_t port_attrib;
    struct mempool mempool;
    struct rte_mempool m;
    bf_dev_port_t dev_port;
    int expected_result, ret;
    memset(&pipe, 0, sizeof(struct pipeline));
    strcpy(port_attrib.source.file_name,"./source.file");
    port_attrib.port_in_id = 4;
    mempool.m = &m;
    strcpy(mempool.m->name,"MEMPOOL2");
    dev_port = 4;
    pipe_in = &pipe;

    //Function call
    ret = lld_dpdk_source_port_add(dev_port, &port_attrib, pipe_in, &mempool);
    expected_result = 0;

    //Macro checking expected result against the actual result
    ASSERT_EQ(ret, expected_result);

}

TEST(CompileMode, case3) {
   struct pipeline *pipe_out, pipe;
    struct port_attributes_t port_attrib;
    bf_dev_port_t dev_port;
    int expected_result, ret;
    memset(&pipe, 0, sizeof(struct pipeline));
    strcpy(port_attrib.sink.file_name,"./sink.file");
    port_attrib.port_out_id = 4;
    dev_port = 4;
    pipe_out = &pipe;

    //Function call
    ret = lld_dpdk_sink_port_add(dev_port, &port_attrib, pipe_out);
    expected_result = 0;

    //Macro checking expected result against the actual result
    ASSERT_EQ(ret, expected_result);

}

TEST(CompileMode, case4) {
   struct pipeline *pipe_in, *pipe_out, pipe;
    struct port_attributes_t port_attrib;
    struct mempool mempool;
    struct ring ring;
    bf_dev_port_t dev_port;
    int expected_result, ret;
    memset(&pipe, 0, sizeof(struct pipeline));
    strcpy(ring.name,"RING_0");
    strcpy(port_attrib.port_name,"RING");
    port_attrib.port_dir = PM_PORT_DIR_DEFAULT;
    port_attrib.port_in_id = 4;
    port_attrib.port_out_id = 4;
    dev_port = 4;
    pipe_in = &pipe;
    pipe_out = &pipe;

    EXPECT_GLOBAL_CALL(ring_find, ring_find(_))
	    .Times(1).
	    WillOnce(Return(&ring));

    //Function call
    ret = lld_dpdk_pipeline_ring_port_add(dev_port, &port_attrib, pipe_in, pipe_out);
    expected_result = 0;

    //Macro checking expected result against the actual result
    ASSERT_EQ(ret, expected_result);

}

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
#ifndef _TDI_TEST_HPP
#define _TDI_TEST_HPP

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock-global.h>
#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include <gmock-global.h>

#include <bf_types/bf_types.h>
#include <dvm/bf_drv_profile.h>
#include <dvm/bf_drv_intf.h>
#include <dvm/bf_drv_profile.h>
#include <tdi_rt/tdi_common/tdi_rt_init.cpp>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>

extern "C"{
    #include "tdi_pipe_mgr_mock.h"
}

namespace tdi {
namespace pna {
namespace rt {

using namespace std;
using ::testing::Return;
using ::testing::_;

class TdiTableTest : public ::testing::TestWithParam<std::tuple<std::string,
								std::string,
								std::string,
								std::string,
								std::string,
								std::string>> {

public:
	TdiTableTest() {};

	~TdiTableTest() {};

	void tdiDevice_add() {
		auto t = this->GetParam();

		//read P4 artifacts
		std::string prog_name = std::get<0>(t);
		std::string tdi_p4_json_file = std::get<1>(t);
		std::string context_p4_json_file = std::get<2>(t);
		std::string spec_file = std::get<3>(t);

		//read fixed artifacts
		std::string fixed_tdi_json_file = std::get<4>(t);
		std::string fixed_ctx_json_file = std::get<5>(t);

		this->prog_name = prog_name;

		//prepare path
		std::string file_path = std::string(JSONDIR) + std::string("/") + prog_name + std::string("/");
		std::string tdi_p4_json_file_path = file_path + tdi_p4_json_file;
	        std::string context_p4_json_file_path = file_path + context_p4_json_file;
		std::string spec_file_path = file_path + spec_file;
		std::string fixed_tdi_json_file_path = file_path + fixed_tdi_json_file;
		std::string fixed_ctx_json_file_path = file_path + fixed_ctx_json_file;

		//setup dummy device profile
		bf_dev_family_t dev_family = BF_DEV_FAMILY_DPDK;
		bf_device_profile_t dev_profile = {0};

		dev_profile.num_p4_programs = 1;

		strncpy(dev_profile.p4_programs[dev_id].prog_name,
				prog_name.c_str(),
				sizeof(dev_profile.p4_programs[dev_id].prog_name));
		dev_profile.p4_programs[dev_id].bfrt_json_file = const_cast<char *>(tdi_p4_json_file_path.c_str());
		dev_profile.p4_programs[dev_id].num_p4_pipelines = 1;


		strncpy(dev_profile.p4_programs[dev_id].p4_pipelines[0].p4_pipeline_name,
				"pipe",
				sizeof(dev_profile.p4_programs[dev_id].p4_pipelines[0].p4_pipeline_name));
		dev_profile.p4_programs[dev_id].p4_pipelines[0].cfg_file =
			const_cast<char *>(spec_file_path.c_str());
		dev_profile.p4_programs[dev_id].p4_pipelines[0].runtime_context_file =
			const_cast<char *>(context_p4_json_file_path.c_str());

		dev_profile.p4_programs[dev_id].p4_pipelines[0].core_id = 1;
		dev_profile.p4_programs[dev_id].p4_pipelines[0].numa_node = 0;
		dev_profile.p4_programs[dev_id].p4_pipelines[0].num_pipes_in_scope = 0;
		dev_profile.p4_programs[dev_id].p4_pipelines[0].pipe_scope[0] = 0;
		dev_profile.p4_programs[dev_id].p4_pipelines[0].pipe_scope[1] = 1;
		dev_profile.p4_programs[dev_id].p4_pipelines[0].pipe_scope[2] = 2;
		dev_profile.p4_programs[dev_id].p4_pipelines[0].pipe_scope[3] = 3;

		dev_profile.p4_programs[dev_id].p4_pipelines[0].mir_cfg.n_slots = 4;
		dev_profile.p4_programs[dev_id].p4_pipelines[0].mir_cfg.n_sessions = 64;
		dev_profile.p4_programs[dev_id].p4_pipelines[0].mir_cfg.fast_clone = 0;

		//fill fixed function json path
		dev_profile.num_fixed_functions = 1;
		strncpy(dev_profile.fixed_functions[0].name,
				"port",
				sizeof(dev_profile.fixed_functions[0].name));
		dev_profile.fixed_functions[0].tdi_json =
			const_cast<char *>(fixed_tdi_json_file_path.c_str());
		dev_profile.fixed_functions[0].ctx_json =
			const_cast<char *>(fixed_ctx_json_file_path.c_str());

		//add dummy TDI device
		ASSERT_EQ(tdi_device_add(dev_id, dev_family, &dev_profile, BF_DEV_INIT_COLD), TDI_SUCCESS);
	}

	virtual void SetUp() {
		const uint64_t flag_value = 0;

		// add dummy device
		tdiDevice_add();

		tdi::DevMgr &devMgrObj = tdi::DevMgr::getInstance();

		ASSERT_EQ(devMgrObj.deviceGet(dev_id, &devObj), TDI_SUCCESS);
		ASSERT_EQ(devObj->tdiInfoGet(prog_name, &tdiInfo), TDI_SUCCESS);
		ASSERT_EQ(tdiInfo->tablesGet(&tables), TDI_SUCCESS);
#if 0
		for (auto *table : tables) {
			std::cout << table->tableInfoGet()->nameGet().c_str() << "\n";
		}
#endif
		// create target
		devObj->createTarget(&target);
		//create session
		EXPECT_GLOBAL_CALL(pipe_mgr_client_init, pipe_mgr_client_init(_))
			.Times(1).
			WillOnce(Return(BF_SUCCESS));
		devObj->createSession(&session);
		// tdi_flags_create
		flags = new tdi::Flags(flag_value);
	}

	virtual void TearDown() {
		EXPECT_GLOBAL_CALL(pipe_mgr_client_cleanup, pipe_mgr_client_cleanup(_))
			.Times(1).
			WillOnce(Return(BF_SUCCESS));
		//destroy session
		session->destroy();
		//remove tdi device
		tdi_device_remove(dev_id);
	}
protected:
	std::string prog_name;
	tdi_dev_id_t dev_id{0};
	tdi::Flags *flags{nullptr};
	const TdiInfo *tdiInfo{nullptr};
	const tdi::Device *devObj{nullptr};
	std::shared_ptr<tdi::Session> session;
	std::unique_ptr<tdi::Target> target;
	std::vector<const tdi::Table *> tables;
};
} //namespace rt
} //namespace pna
} //namespace tdi
#endif /* _TDI_TEST_HPP */

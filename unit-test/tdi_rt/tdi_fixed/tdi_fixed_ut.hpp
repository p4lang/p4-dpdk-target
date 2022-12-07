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
#ifndef _TDI_FIXED_UT_HPP
#define _TDI_FIXED_UT_HPP

// base class include
#include "tdi_test.hpp"
extern "C"{
    #include "tdi_fixed_mock.h"
}

namespace tdi {
namespace pna {
namespace rt {

using namespace std;
using ::testing::Return;
using ::testing::_;

//fixed function confi table test
class FixedFunctionConfigTableTest : public TdiTableTest {
protected:
	FixedFunctionConfigTableTest() {};

	void SetUp() override {
		TdiTableTest::SetUp();
	}

	tdi_status_t table_entry_add(const tdi::Table &table);
	tdi_status_t table_entry_delete(const tdi::Table &table);
	tdi_status_t table_entry_default_get(const tdi::Table &table);
	tdi_status_t table_attributes_set(const tdi::Table &table);
	tdi_status_t table_attributes_get(const tdi::Table &table);
};

INSTANTIATE_TEST_SUITE_P(FixedFunctionConfigTableRulePortSuite,
                        FixedFunctionConfigTableTest,
                        ::testing::Values(std::make_tuple("fixed_port",
					"tdi.json",
					"context.json",
					"counter.spec",
					"port_tdi.json",
					"port_context.json"
					)));
} //namespace rt
} //namespace pna
} //namespace tdi
#endif  /* _TDI_FIXED_UT_HPP */

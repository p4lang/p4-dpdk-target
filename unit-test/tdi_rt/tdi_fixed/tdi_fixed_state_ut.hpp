// SPDX-FileCopyrightText: 2022 Intel Corporation
// Copyright (C) 2022 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0
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

//fixed function state table test
class FixedFunctionStateTableTest : public TdiTableTest {
protected:
	FixedFunctionStateTableTest() {};

	void SetUp() override {
		TdiTableTest::SetUp();
	}

	tdi_status_t table_state_get(const tdi::Table &table);
};

INSTANTIATE_TEST_SUITE_P(FixedFunctionStateTableRulePortSuite,
                        FixedFunctionStateTableTest,
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

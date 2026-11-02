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

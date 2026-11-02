// SPDX-FileCopyrightText: 2022 Intel Corporation
// Copyright (C) 2022 Intel Corporation.
//
// SPDX-License-Identifier: Apache-2.0
#ifndef _TDI_FIXED_UT_HPP
#define _TDI_FIXED_UT_HPP

// base class include
#include "tdi_test.hpp"
extern "C"{
    #include "tdi_p4_mock.h"
}

namespace tdi {
namespace pna {
namespace rt {

using namespace std;
using ::testing::Return;
using ::testing::_;

//Register table test
class RegisterTableTest : public TdiTableTest {
protected:
	RegisterTableTest() {};

	void SetUp() override {
		TdiTableTest::SetUp();
	}

	tdi_status_t table_entry_mod(const tdi::Table &table);
  tdi_status_t table_entry_get(const tdi::Table &table);
};

INSTANTIATE_TEST_SUITE_P(RegisterTableRuleSuite,
                        RegisterTableTest,
                        ::testing::Values(std::make_tuple("register", /* program name */
					"tdi.json",                       /* p4 tdi.json  */
					"context.json",                   /* p4 context.json */
					"psa_register.spec",                   /* P4 dpdk spec file */
					" ",     /* fixed function tdi json file */
					" "      /* fixed function ctx file */
					)));
} //namespace rt
} //namespace pna
} //namespace tdi
#endif  /* _TDI_FIXED_UT_HPP */

/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "gtest/gtest.h"

extern "C" {
#include "amdsmi.h"
}

#include <cstdio>

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

class AmdsmiVersionTests : public amdsmi::AmdSmiTest {
};

TEST_F(AmdsmiVersionTests, GetVersion)
{
	int ret;
	amdsmi_version_t version;

	ret = amdsmi_get_lib_version(&version);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdsmiVersionTests, GetVersionInval)
{
	int ret;

	ret = amdsmi_get_lib_version(NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

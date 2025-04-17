/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
	ASSERT_EQ(27, version.major);
	ASSERT_EQ(0, version.minor);
	ASSERT_EQ(0, version.release);
}

TEST_F(AmdsmiVersionTests, GetVersionInval)
{
	int ret;

	ret = amdsmi_get_lib_version(NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

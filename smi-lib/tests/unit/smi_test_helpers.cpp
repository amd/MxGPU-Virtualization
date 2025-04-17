/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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
#include "smi_defines.h"
#include "common/smi_cmd.h"
}

#include "smi_test_helpers.hpp"
#include "smi_system_mock.hpp"
#include "common/smi_device_handle.h"

amdsmi_bdf_t MOCK_BDF = { { 0x4, 0x3, 0x2, 0x1 } }; // 0001:02:03.04
const char *GPU_MOCK_UUID{"9aff0003-0000-1000-801f-188c37cb1ee6"};
const char *VF_MOCK_UUID{"9a010003-0000-1000-801f-188c37cb1ee6"};
smi_device_handle_t GPU_MOCK_HANDLE = { (0x1234ULL << 32) | 0x1234 };
amdsmi_vf_handle_t VF_MOCK_HANDLE = { (0x1234ULL << 32) | 0x4567 };

namespace amdsmi
{
std::unique_ptr<NiceMock<SystemMock>> g_system_mock;

SystemMock *GetSystemMock()
{
	return g_system_mock.get();
}

void AmdSmiTest::initialize_smi_lib(uint32_t version)
{
	handshake_version = version;
	// set the appropriate version for successful handshake
	EXPECT_CALL(*GetSystemMock(), Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_HANDSHAKE)))
		.WillOnce(testing::DoAll(SetPayload(smi_handshake{ handshake_version }),
					 testing::Return(0)));

	// set the one processor handle and bdf
	smi_server_static_info server_info_mock = {};
	server_info_mock.devices[0].bdf.as_uint = MOCK_BDF.as_uint;
	server_info_mock.devices[0].dev_id = GPU_MOCK_HANDLE;
	server_info_mock.num_devices = 1;
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(testing::DoAll(SetPayload(server_info_mock), testing::Return(0)));

	int res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	ASSERT_EQ(res, AMDSMI_STATUS_SUCCESS);

	Mock::VerifyAndClearExpectations(GetSystemMock());
}

void AmdSmiTest::finalize_smi_lib()
{
	int res = amdsmi_shut_down();
	ASSERT_EQ(res, AMDSMI_STATUS_SUCCESS);
}

::testing::AssertionResult vf_configs_equal(smi_vf_info expect,
					    amdsmi_vf_info_t actual)
{
	SMI_ASSERT_EQ(expect.fb.fb_offset, actual.fb.fb_offset);
	SMI_ASSERT_EQ(expect.fb.fb_size, actual.fb.fb_size);
	SMI_ASSERT_EQ(expect.gfx_timeslice, actual.gfx_timeslice);

	return ::testing::AssertionSuccess();
}

::testing::AssertionResult equal_handles(smi_device_handle_t expect,
					 smi_device_handle_t actual)
{
	SMI_ASSERT_EQ(expect.handle, actual.handle);
	return ::testing::AssertionSuccess();
}

::testing::AssertionResult equal_bdfs(amdsmi_bdf_t bdf_expect, amdsmi_bdf_t bdf_actual)
{
	SMI_ASSERT_EQ(bdf_expect.as_uint, bdf_actual.as_uint);
	return ::testing::AssertionSuccess();
}

void *mem_aligned_alloc(void **mem, size_t alignment, size_t size)
{
#ifdef _WIN64
	return _aligned_malloc(size, alignment);
#elif !defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_ADDRESS__)
	AMDSMI_UNUSED(mem);
	return aligned_alloc(alignment, size);
#else
	return posix_memalign(mem, alignment, size) == 0 ? *mem : NULL;
#endif
}

} // namespace amdsmi

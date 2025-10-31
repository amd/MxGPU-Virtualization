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
#include "smi_processor_handle.h"
}

#include "smi_test_helpers.hpp"
#include "smi_system_mock.hpp"
#include "common/smi_device_handle.h"

amdsmi_bdf_t MOCK_BDF = { { 0x4, 0x3, 0x2, 0x1 } }; // 0001:02:03.04
const char *GPU_MOCK_UUID{"9aff0003-0000-1000-801f-188c37cb1ee6"};
const char *VF_MOCK_UUID{"9a0174b5-0000-1000-801f-188c37cb1ee6"};
struct smi_gpu_handle GPU_MOCK_HANDLE = {
	SMI_PROCESSOR_TYPE_AMD_GPU,
	{ { 0x4, 0x3, 0x2, 0x1 } },
	(0x1234ULL << 32) | 0x1234,
	0x5678
};
amdsmi_vf_handle_t VF_MOCK_HANDLE = { (0x1234ULL << 32) | 0x4567 };
struct smi_nic_handle NIC_MOCK_HANDLE = {
	SMI_PROCESSOR_TYPE_AMD_NIC,
	{ { 0x4, 0x3, 0x2, 0x1 } }
};

namespace amdsmi
{
std::unique_ptr<NiceMock<SystemMock>> g_system_mock;

SystemMock *GetSystemMock()
{
	return g_system_mock.get();
}

void AmdSmiTest::initialize_smi_lib(uint32_t version, uint8_t num_dev)
{
	handshake_version = version;
	// set the appropriate version for successful handshake
	EXPECT_CALL(*GetSystemMock(), Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_HANDSHAKE)))
		.WillOnce(testing::DoAll(SetPayload(smi_handshake{ handshake_version }),
					 testing::Return(0)));

	smi_server_static_info server_info_mock = {};
	for (uint32_t i = 0; i < num_dev; i++) {
		server_info_mock.devices[i].bdf.as_uint = MOCK_BDF.as_uint;
		server_info_mock.devices[i].bdf.bdf.device_number = (server_info_mock.devices[i].bdf.bdf.device_number + i) % 32;

		server_info_mock.devices[i].dev_id.handle = GPU_MOCK_HANDLE.handle;
		server_info_mock.devices[i].dev_id.handle += i;
	}
	server_info_mock.num_devices = num_dev;
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
					 struct smi_gpu_handle actual)
{
	SMI_ASSERT_EQ(expect.handle, actual.handle);
	return ::testing::AssertionSuccess();
}

::testing::AssertionResult equal_bdfs(amdsmi_bdf_t bdf_expect, amdsmi_bdf_t bdf_actual)
{
	SMI_ASSERT_EQ(bdf_expect.as_uint, bdf_actual.as_uint);
	return ::testing::AssertionSuccess();
}

::testing::AssertionResult equal_dpm_policy(smi_dpm_policy expect,
							amdsmi_dpm_policy_t actual)
{
	SMI_ASSERT_EQ(expect.num_supported, actual.num_supported);
	SMI_ASSERT_EQ(expect.cur, actual.current);
	for (uint32_t i = 0; i < expect.num_supported; i++) {
		SMI_ASSERT_STR_EQ(expect.policies[i].policy_description, actual.policies[i].policy_description)
			<< " for i = " << i;
		SMI_ASSERT_EQ(expect.policies[i].policy_id, actual.policies[i].policy_id)
			<< " for i = " << i;
	}

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

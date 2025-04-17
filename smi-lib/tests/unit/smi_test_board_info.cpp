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
#include "amdsmi.h"
#include "common/smi_cmd.h"
#include "common/smi_device_handle.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::equal_handles;
using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;


class AmdSmiBoardTests : public amdsmi::AmdSmiTest {
public:
protected:
	::testing::AssertionResult equal_asic_info(smi_asic_info expect,
						   amdsmi_asic_info_t actual)
	{
		SMI_ASSERT_STR_EQ(expect.market_name, actual.market_name);
		SMI_ASSERT_EQ(expect.vendor_id, actual.vendor_id);
		SMI_ASSERT_EQ(expect.subvendor_id, actual.subvendor_id);
		SMI_ASSERT_EQ(expect.device_id, actual.device_id);
		SMI_ASSERT_EQ(expect.rev_id, actual.rev_id);
		SMI_ASSERT_STR_EQ(expect.asic_serial, actual.asic_serial);
		SMI_ASSERT_EQ(expect.oam_id, actual.oam_id);
		SMI_ASSERT_EQ(expect.num_of_compute_units, actual.num_of_compute_units);
		SMI_ASSERT_EQ(expect.target_graphics_version, actual.target_graphics_version);
		SMI_ASSERT_EQ(expect.subsystem_id, actual.subsystem_id);

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_power_info(smi_power_cap_info expect,
						    amdsmi_power_cap_info_t actual)
	{
		SMI_ASSERT_EQ(expect.power_cap, actual.power_cap);
		SMI_ASSERT_EQ(expect.default_power_cap, actual.default_power_cap);
		SMI_ASSERT_EQ(expect.dpm_cap, actual.dpm_cap);
		SMI_ASSERT_EQ(expect.min_power_cap, actual.min_power_cap);
		SMI_ASSERT_EQ(expect.max_power_cap, actual.max_power_cap);

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_fb_info(smi_pf_fb_info expect,
						 amdsmi_pf_fb_info_t actual)
	{
		SMI_ASSERT_EQ(expect.fb_alignment, actual.fb_alignment);
		SMI_ASSERT_EQ(expect.pf_fb_offset, actual.pf_fb_offset);
		SMI_ASSERT_EQ(expect.pf_fb_reserved, actual.pf_fb_reserved);
		SMI_ASSERT_EQ(expect.total_fb_size, actual.total_fb_size);
		SMI_ASSERT_EQ(expect.max_vf_fb_usable, actual.max_vf_fb_usable);
		SMI_ASSERT_EQ(expect.min_vf_fb_usable, actual.min_vf_fb_usable);

		return ::testing::AssertionSuccess();
	}
};

TEST_F(AmdSmiBoardTests, InvalidParams)
{
	int ret;
	uint32_t sensor_ind = 0;

	ret = amdsmi_get_gpu_asic_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_asic_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_vram_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_power_cap_info(&GPU_MOCK_HANDLE, sensor_ind, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_set_power_cap(NULL, sensor_ind, 0);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_fb_layout(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_pcie_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiBoardTests, IoctlFailed)
{
	int ret;
	amdsmi_asic_info_t asic_res;
	amdsmi_vram_info_t vram_info;
	amdsmi_power_cap_info_t power_res;
	amdsmi_pf_fb_info_t fb_res;
	uint32_t sensor_ind = 0;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_asic_info(&GPU_MOCK_HANDLE, &asic_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_gpu_vram_info(&GPU_MOCK_HANDLE, &vram_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_power_cap_info(&GPU_MOCK_HANDLE, sensor_ind, &power_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_set_power_cap(&GPU_MOCK_HANDLE, sensor_ind, 0);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_fb_layout(&GPU_MOCK_HANDLE, &fb_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiBoardTests, GetAsicInfo)
{
	int ret;
	smi_asic_info gpu_info_mock = {};
	smi_device_info in_payload;
	std::string name = "abcdef";
#ifdef _WIN64
	strcpy_s(gpu_info_mock.market_name, sizeof(gpu_info_mock.market_name), name.c_str());
#else
	strcpy(gpu_info_mock.market_name, name.c_str());
#endif
	gpu_info_mock.vendor_id = 2;
	gpu_info_mock.device_id = 3;
	gpu_info_mock.rev_id = 4;
#ifdef _WIN64
	strcpy_s(gpu_info_mock.asic_serial, sizeof(gpu_info_mock.asic_serial), "1234567");
#else
	strcpy(gpu_info_mock.asic_serial, "1234567");
#endif
	gpu_info_mock.oam_id = 0;

	amdsmi_asic_info_t asic_info;
	WhenCalling(std::bind(amdsmi_get_gpu_asic_info, &GPU_MOCK_HANDLE, &asic_info));
	ExpectCommand(SMI_CMD_CODE_GET_ASIC_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_asic_info(gpu_info_mock, asic_info));
}


TEST_F(AmdSmiBoardTests, GetVramInfo)
{
	int ret;
	smi_vram_info gpu_info_mock = {};
	smi_device_info in_payload;
	gpu_info_mock.vram_type = SMI_VRAM_TYPE_GDDR5;
	gpu_info_mock.vram_vendor = SMI_VRAM_VENDOR_SAMSUNG;
	gpu_info_mock.vram_size = 512;
	gpu_info_mock.vram_bit_width = 8192;

	amdsmi_vram_info_t vram_info;
	WhenCalling(std::bind(amdsmi_get_gpu_vram_info, &GPU_MOCK_HANDLE, &vram_info));
	ExpectCommand(SMI_CMD_CODE_GET_VRAM_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(vram_info.vram_type, gpu_info_mock.vram_type);
	ASSERT_EQ(vram_info.vram_vendor, gpu_info_mock.vram_vendor);
	ASSERT_EQ(vram_info.vram_size, gpu_info_mock.vram_size);
	ASSERT_EQ(vram_info.vram_bit_width, gpu_info_mock.vram_bit_width);
}

TEST_F(AmdSmiBoardTests, GetVramInfoNotSupported)
{
	int ret;
	smi_vram_info gpu_info_mock = {};
	smi_device_info in_payload;
	gpu_info_mock.vram_size = 0xFFFFFFFF;

	amdsmi_vram_info_t vram_info;
	WhenCalling(std::bind(amdsmi_get_gpu_vram_info, &GPU_MOCK_HANDLE, &vram_info));
	ExpectCommand(SMI_CMD_CODE_GET_VRAM_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}

TEST_F(AmdSmiBoardTests, GetPowerInfo)
{
	int ret;
	struct smi_in_hdr in_hdr_enabled;
	struct smi_device_info_ex in_dev_enabled;
	smi_power_cap_info gpu_info_mock = {};
	uint32_t sensor_ind = 0;

	gpu_info_mock.power_cap = 120;
	gpu_info_mock.dpm_cap = 2;
	gpu_info_mock.default_power_cap = 0;
	gpu_info_mock.max_power_cap = 280;
	gpu_info_mock.min_power_cap = 0;

	PrepareIoctl(SMI_CMD_CODE_GET_POWER_CAP_INFO, &in_hdr_enabled, &in_dev_enabled, gpu_info_mock);

	amdsmi_power_cap_info_t info;
	ret = amdsmi_get_power_cap_info(&GPU_MOCK_HANDLE, sensor_ind, &info);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_power_info(gpu_info_mock, info));
}

TEST_F(AmdSmiBoardTests, SetPowerCap)
{
	int ret;
	amdsmi_power_cap_info_t mocked_resp = {};
	uint32_t sensor_ind = 0;
	uint64_t cap = 160;

	mocked_resp.power_cap = 120;
	mocked_resp.dpm_cap = 2;
	mocked_resp.default_power_cap = 0;
	mocked_resp.max_power_cap = 280;
	mocked_resp.min_power_cap = 0;

	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_SET_GPU_POWER_CAP)))
		.WillRepeatedly(testing::DoAll(amdsmi::SetPayload(mocked_resp), testing::Return(0)));

	ret = amdsmi_set_power_cap(MOCK_GPU_HANDLE, sensor_ind, cap);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiBoardTests, GetFbInfo)
{
	int ret;
	smi_device_info in_payload;
	smi_pf_fb_info gpu_info_mock = {};

	gpu_info_mock.fb_alignment = 1;
	gpu_info_mock.pf_fb_offset = 2;
	gpu_info_mock.pf_fb_reserved = 3;
	gpu_info_mock.min_vf_fb_usable = 4;
	gpu_info_mock.max_vf_fb_usable = 5;
	gpu_info_mock.total_fb_size = 6;

	amdsmi_pf_fb_info_t fb_info;
	WhenCalling(std::bind(amdsmi_get_fb_layout, &GPU_MOCK_HANDLE, &fb_info));
	ExpectCommand(SMI_CMD_CODE_GET_PF_FB_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_fb_info(gpu_info_mock, fb_info));
}

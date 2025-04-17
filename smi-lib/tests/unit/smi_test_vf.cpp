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
#include <cstdlib>

extern "C" {
#include "amdsmi.h"
#include "common/smi_cmd.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::equal_handles;
using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;
using amdsmi::vf_configs_equal;

static ::testing::AssertionResult sched_info_equal(smi_sched_info expect,
						   amdsmi_sched_info_t actual)
{
	SMI_ASSERT_EQ((int)expect.state, (int)actual.state);
	SMI_ASSERT_EQ(expect.flr_count, actual.flr_count);
	SMI_ASSERT_EQ(expect.boot_up_time, actual.boot_up_time);
	SMI_ASSERT_EQ(expect.reset_time, actual.reset_time);
	SMI_ASSERT_EQ(expect.shutdown_time, actual.shutdown_time);
	SMI_ASSERT_STR_EQ(expect.last_boot_start, actual.last_boot_start);
	SMI_ASSERT_STR_EQ(expect.last_boot_end, actual.last_boot_end);
	SMI_ASSERT_STR_EQ(expect.last_reset_start, actual.last_reset_start);
	SMI_ASSERT_STR_EQ(expect.last_reset_end, actual.last_reset_end);
	SMI_ASSERT_STR_EQ(expect.last_shutdown_start, actual.last_shutdown_start);
	SMI_ASSERT_STR_EQ(expect.last_shutdown_end, actual.last_shutdown_end);
	SMI_ASSERT_STR_EQ(expect.total_active_time, actual.total_active_time);
	SMI_ASSERT_STR_EQ(expect.total_running_time, actual.total_running_time);
	SMI_ASSERT_STR_EQ(expect.current_active_time, actual.current_active_time);
	SMI_ASSERT_STR_EQ(expect.current_running_time, actual.current_running_time);

	return ::testing::AssertionSuccess();
}

static ::testing::AssertionResult guard_info_equal(smi_guard_info expect,
						   amdsmi_guard_info_t actual)
{
	SMI_ASSERT_EQ(expect.enabled, actual.enabled);

	for (int i = 0; i < AMDSMI_GUARD_EVENT__MAX; i++) {
		SMI_ASSERT_EQ((int)expect.guard[i].state, (int)actual.guard[i].state)
			<< " for i = " << i;
		SMI_ASSERT_EQ(expect.guard[i].amount, actual.guard[i].amount)
			<< " for i = " << i;
		SMI_ASSERT_EQ(expect.guard[i].threshold, actual.guard[i].threshold)
			<< " for i = " << i;
		SMI_ASSERT_EQ(expect.guard[i].active, actual.guard[i].active)
			<< " for i = " << i;
	}

	return ::testing::AssertionSuccess();
}

static ::testing::AssertionResult vf_partition_info_equal(smi_partition_info expect,
							  amdsmi_partition_info_t actual)
{
	SMI_ASSERT_EQ(expect.fb.fb_size, actual.fb.fb_size);
	SMI_ASSERT_EQ(expect.fb.fb_offset, actual.fb.fb_offset);

	return ::testing::AssertionSuccess();
}

class AmdsmiVfTests : public amdsmi::AmdSmiTest {
public:
};

TEST_F(AmdsmiVfTests, InvalidParams)
{
	int ret;
	unsigned int buf_num = 6;

	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	amdsmi_vf_handle_t MOCK_VF_HANDLE = VF_MOCK_HANDLE;

	ret = amdsmi_get_num_vf(MOCK_GPU_HANDLE, NULL, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_vf_partition_info(MOCK_GPU_HANDLE, buf_num, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_vf_info(MOCK_VF_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_vf_data(MOCK_VF_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdsmiVfTests, IoctlFailed)
{
	int ret;
	uint32_t num_vf = 4;
	unsigned int buf_num = 6;
	uint32_t num_vf_enabled;
	uint32_t num_vf_supported;
	amdsmi_partition_info_t part_res;
	amdsmi_vf_info_t config_res;
	amdsmi_vf_data_t info_res;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	amdsmi_vf_handle_t MOCK_VF_HANDLE = VF_MOCK_HANDLE;
	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_set_num_vf(MOCK_GPU_HANDLE, num_vf);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_num_vf(MOCK_GPU_HANDLE, &num_vf_enabled, &num_vf_supported);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_vf_partition_info(MOCK_GPU_HANDLE, buf_num, &part_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_vf_info(MOCK_VF_HANDLE, &config_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_vf_data(MOCK_VF_HANDLE, &info_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_clear_vf_fb(MOCK_VF_HANDLE);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdsmiVfTests, GetNumVf)
{
	int ret;
	struct smi_device_info in_payload;
	smi_vf_partition_info mocked_resp = {};
	uint32_t num_vf_enabled;
	uint32_t num_vf_supported;

	mocked_resp.num_vf_supported = 4;
	mocked_resp.num_vf_enabled = 4;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	WhenCalling(std::bind(amdsmi_get_num_vf, MOCK_GPU_HANDLE, &num_vf_enabled, &num_vf_supported));
	ExpectCommand(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(num_vf_enabled, 4);
	ASSERT_EQ(num_vf_supported, 4);
	ASSERT_TRUE(Mock::VerifyAndClearExpectations(g_system_mock.get()));
}

TEST_F(AmdsmiVfTests, GetNumVfApiFailed)
{
	int ret;
	struct smi_device_info in_payload;
	smi_vf_partition_info mocked_resp = {};
	uint32_t num_vf_enabled;
	uint32_t num_vf_supported;

	mocked_resp.num_vf_supported = 4;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	mocked_resp.num_vf_enabled = 4;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO)))
		.WillOnce(amdsmi::SetResponseStatus(AMDSMI_STATUS_API_FAILED));
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = amdsmi_get_num_vf(MOCK_GPU_HANDLE, &num_vf_enabled, &num_vf_supported);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdsmiVfTests, SetNumVf)
{
	int ret;
	smi_vf_partition_config in_payload;
	uint32_t num_vf = 4;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	WhenCalling(std::bind(amdsmi_set_num_vf, MOCK_GPU_HANDLE, num_vf));
	ExpectCommand(SMI_CMD_CODE_SET_VF_PARTITIONING_INFO);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(in_payload.num_vf_enable, num_vf);
}

TEST_F(AmdsmiVfTests, ClearVfFb)
{
	int ret;
	struct smi_device_info in_payload;
	amdsmi_vf_handle_t MOCK_VF_HANDLE = VF_MOCK_HANDLE;

	WhenCalling(std::bind(amdsmi_clear_vf_fb, MOCK_VF_HANDLE));
	ExpectCommand(SMI_CMD_CODE_CLEAR_VF_FB);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(in_payload.dev_id.handle, VF_MOCK_HANDLE.handle);
}

TEST_F(AmdsmiVfTests, GetPartitionInfo)
{
	int ret;
	struct smi_device_info in_payload;
	smi_vf_partition_info mocked_resp = {};
	uint32_t num_vf = 1;
	amdsmi_partition_info_t partition_info;

	mocked_resp.num_vf_enabled = num_vf;
	mocked_resp.partition[0].fb.fb_size = 4048;
	mocked_resp.partition[0].fb.fb_offset = 4064;

	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	WhenCalling(std::bind(amdsmi_get_vf_partition_info, MOCK_GPU_HANDLE, num_vf, &partition_info));
	ExpectCommand(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(vf_partition_info_equal(mocked_resp.partition[0], partition_info));
}

TEST_F(AmdsmiVfTests, GetPartitionInfo_BufferTooSmall)
{
	int ret;
	struct smi_device_info in_payload;
	struct smi_vf_partition_info mocked_resp = {};
	unsigned int num_vf = 1;
	amdsmi_partition_info_t partition_info;

	mocked_resp.num_vf_enabled = 8;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	WhenCalling(std::bind(amdsmi_get_vf_partition_info, MOCK_GPU_HANDLE, num_vf, &partition_info));
	ExpectCommand(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdsmiVfTests, GetVfConfig)
{
	int ret;
	struct smi_device_info in_payload;
	struct smi_vf_static_info mocked_resp = {};
	amdsmi_vf_info_t vf_config;
	amdsmi_vf_handle_t MOCK_VF_HANDLE = VF_MOCK_HANDLE;
	memset(&mocked_resp.config, 0, sizeof(amdsmi_vf_info_t));
	mocked_resp.config.fb.fb_size = 4048;
	mocked_resp.config.fb.fb_offset = 4064;
	mocked_resp.config.gfx_timeslice = 3000;

	WhenCalling(std::bind(amdsmi_get_vf_info, MOCK_VF_HANDLE, &vf_config));
	ExpectCommand(SMI_CMD_CODE_GET_VF_STATIC_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(in_payload.dev_id.handle, VF_MOCK_HANDLE.handle);
	ASSERT_TRUE(vf_configs_equal(mocked_resp.config, vf_config));
}

TEST_F(AmdsmiVfTests, GetVfData)
{
	int ret;
	struct smi_device_info in_payload;
	struct smi_vf_data mocked_resp = {};
	amdsmi_vf_data_t vf_data;
	amdsmi_vf_handle_t MOCK_VF_HANDLE = VF_MOCK_HANDLE;
	mocked_resp.sched.state = SMI_VF_STATE_DEFAULT_AVAILABLE;
	mocked_resp.sched.flr_count = 1;
#ifdef _WIN64
	strcpy_s(mocked_resp.sched.last_boot_start, sizeof(mocked_resp.sched.last_boot_start), "01:02:03.004");
	strcpy_s(mocked_resp.sched.last_boot_end, sizeof(mocked_resp.sched.last_boot_end), "05:06:07.008");
#else
	strcpy(mocked_resp.sched.last_boot_start, "01:02:03.004");
	strcpy(mocked_resp.sched.last_boot_end, "05:06:07.008");
#endif
	mocked_resp.sched.boot_up_time = 2;
#ifdef _WIN64
	strcpy_s(mocked_resp.sched.last_reset_start, sizeof(mocked_resp.sched.last_reset_start), "00:09:10.011");
	strcpy_s(mocked_resp.sched.last_reset_end, sizeof(mocked_resp.sched.last_reset_end), "00:12:13.014");
#else
	strcpy(mocked_resp.sched.last_reset_start, "00:09:10.011");
	strcpy(mocked_resp.sched.last_reset_end, "00:12:13.014");
#endif
	mocked_resp.sched.reset_time = 3;
#ifdef _WIN64
	strcpy_s(mocked_resp.sched.last_shutdown_start, sizeof(mocked_resp.sched.last_shutdown_start), "00:00:15.016");
	strcpy_s(mocked_resp.sched.last_shutdown_end, sizeof(mocked_resp.sched.last_shutdown_end), "00:00:17.018");
#else
	strcpy(mocked_resp.sched.last_shutdown_start, "00:00:15.016");
	strcpy(mocked_resp.sched.last_shutdown_end, "00:00:17.018");
#endif
	mocked_resp.sched.shutdown_time = 4;
#ifdef _WIN64
	strcpy_s(mocked_resp.sched.current_active_time, sizeof(mocked_resp.sched.current_active_time), "01:01:01.001");
	strcpy_s(mocked_resp.sched.current_running_time, sizeof(mocked_resp.sched.current_running_time), "02:02:02.002");
	strcpy_s(mocked_resp.sched.total_active_time, sizeof(mocked_resp.sched.total_active_time), "03:03:03.003");
	strcpy_s(mocked_resp.sched.total_running_time, sizeof(mocked_resp.sched.total_running_time), "04:04:04.004");
#else
	strcpy(mocked_resp.sched.current_active_time, "01:01:01.001");
	strcpy(mocked_resp.sched.current_running_time, "02:02:02.002");
	strcpy(mocked_resp.sched.total_active_time, "03:03:03.003");
	strcpy(mocked_resp.sched.total_running_time, "04:04:04.004");
#endif

	mocked_resp.guard.enabled = true;

	for (unsigned i = 0; i < AMDSMI_GUARD_EVENT__MAX; i++) {
		mocked_resp.guard.guard[i].state = static_cast<smi_guard_state>(i % 3);
		mocked_resp.guard.guard[i].amount = i;
		mocked_resp.guard.guard[i].interval = i * 2;
		mocked_resp.guard.guard[i].threshold = i * 3;
		mocked_resp.guard.guard[i].active = i * 4;
	}

	WhenCalling(std::bind(amdsmi_get_vf_data, MOCK_VF_HANDLE, &vf_data));
	ExpectCommand(SMI_CMD_CODE_GET_VF_DYNAMIC_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(in_payload.dev_id.handle, VF_MOCK_HANDLE.handle);
	ASSERT_TRUE(sched_info_equal(mocked_resp.sched, vf_data.sched));
	ASSERT_TRUE(guard_info_equal(mocked_resp.guard, vf_data.guard));
}

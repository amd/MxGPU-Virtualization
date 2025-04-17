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
#include "smi_utils.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;
using testing::_;
using testing::DoAll;
using testing::Return;

const char *AmdsmiClockType[] = {
	"AMDSMI_CLK_TYPE_GFX",
	"AMDSMI_CLK_TYPE_MEM",
	"AMDSMI_CLK_TYPE_VCLK0",
	"AMDSMI_CLK_TYPE_VCLK1",
	"AMDSMI_CLK_TYPE_DCLK0",
	"AMDSMI_CLK_TYPE_DCLK1"
};

const char *AmdsmiTemperatureType[] = {
	"AMDSMI_AMDSMI_TEMPERATURE_TYPE_EDGE",
	"AMDSMI_AMDSMI_TEMPERATURE_TYPE_HOTSPOT",
	"AMDSMI_TEMPERATURE_TYPE_VRAM",
	"AMDSMI_TEMPERATURE_TYPE_PLX",
};

class AmdsmiGpuMonitoring : public amdsmi::AmdSmiTest {
protected:
	::testing::AssertionResult equal_engine_usage(smi_engine_usage expect,
						      amdsmi_engine_usage_t actual)
	{
		SMI_ASSERT_EQ(expect.gfx_activity, actual.gfx_activity);
		SMI_ASSERT_EQ(expect.umc_activity, actual.umc_activity);
		SMI_ASSERT_EQ(expect.mm_activity, actual.mm_activity);

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_power_measure(smi_power_info expect,
						       amdsmi_power_info_t actual)
	{
		SMI_ASSERT_EQ(expect.socket_power, actual.socket_power);
		SMI_ASSERT_EQ(expect.gfx_voltage, actual.gfx_voltage);
		SMI_ASSERT_EQ(expect.soc_voltage, actual.soc_voltage);
		SMI_ASSERT_EQ(expect.mem_voltage, actual.mem_voltage);

		return ::testing::AssertionSuccess();
	}



	::testing::AssertionResult equal_clock_measure(smi_clock_measure expect,
						       amdsmi_clk_info_t actual,
						       unsigned domain_index)
	{
		SMI_ASSERT_EQ(expect.cur_clk[domain_index], actual.clk)
			<< " for domain "  << AmdsmiClockType[domain_index];
		SMI_ASSERT_EQ(expect.max_clk[domain_index], actual.max_clk)
			<< " for domain "  << AmdsmiClockType[domain_index];
		SMI_ASSERT_EQ(expect.min_clk[domain_index], actual.min_clk)
			<< " for domain "  << AmdsmiClockType[domain_index];
		SMI_ASSERT_EQ(expect.clk_locked[domain_index], actual.clk_locked)
			<< " for domain "  << AmdsmiClockType[domain_index];
		SMI_ASSERT_EQ(expect.clk_deep_sleep[domain_index], actual.clk_deep_sleep)
			<< " for domain "  << AmdsmiClockType[domain_index];

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_temperature_measure(smi_temp_measure expect,
							 int64_t actual,
							 unsigned domain_index)
	{
		SMI_ASSERT_EQ(expect.temp[domain_index], actual)
			<< " for domain "  << AmdsmiTemperatureType[domain_index];

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_temperature_limit(smi_temp_limit expect,
							 int64_t actual,
							 unsigned domain_index)
	{
		SMI_ASSERT_EQ(expect.temp[domain_index], actual)
			<< " for domain "  << AmdsmiTemperatureType[domain_index];

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_pcie_info(smi_pcie_info expect,
						   amdsmi_pcie_info_t actual)
	{
		uint32_t expected_speed = 0;
		uint64_t dev_id = 0;
		amdsmi_get_pcie_speed_from_pcie_type(expect.pcie_metric.pcie_speed, &expected_speed, dev_id);
		SMI_ASSERT_EQ(expect.pcie_static.max_pcie_width, actual.pcie_static.max_pcie_width);
		SMI_ASSERT_EQ(expect.pcie_static.max_pcie_speed, actual.pcie_static.max_pcie_speed);
		SMI_ASSERT_EQ(expect.pcie_static.pcie_interface_version, actual.pcie_static.pcie_interface_version);
		SMI_ASSERT_EQ(expect.pcie_static.slot_type, (enum smi_card_form_factor)actual.pcie_static.slot_type);
		SMI_ASSERT_EQ(expect.pcie_static.max_pcie_interface_version, actual.pcie_static.max_pcie_interface_version);
		SMI_ASSERT_EQ(expected_speed, actual.pcie_metric.pcie_speed);
		SMI_ASSERT_EQ(expect.pcie_metric.pcie_width, actual.pcie_metric.pcie_width);
		SMI_ASSERT_EQ(expect.pcie_metric.pcie_bandwidth, actual.pcie_metric.pcie_bandwidth);
		SMI_ASSERT_EQ(expect.pcie_metric.pcie_replay_count, actual.pcie_metric.pcie_replay_count);
		SMI_ASSERT_EQ(expect.pcie_metric.pcie_l0_to_recovery_count, actual.pcie_metric.pcie_l0_to_recovery_count);
		SMI_ASSERT_EQ(expect.pcie_metric.pcie_replay_roll_over_count, actual.pcie_metric.pcie_replay_roll_over_count);
		SMI_ASSERT_EQ(expect.pcie_metric.pcie_nak_sent_count, actual.pcie_metric.pcie_nak_sent_count);
		SMI_ASSERT_EQ(expect.pcie_metric.pcie_nak_received_count, actual.pcie_metric.pcie_nak_received_count);
		SMI_ASSERT_EQ(expect.pcie_metric.pcie_lc_perf_other_end_recovery_count, actual.pcie_metric.pcie_lc_perf_other_end_recovery_count);

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_cache_info(smi_gpu_cache_info expect,
						      amdsmi_gpu_cache_info_t actual)
	{
		SMI_ASSERT_EQ(expect.num_cache_types, actual.num_cache_types);

		for (uint32_t i=0; i < expect.num_cache_types; i++){
			SMI_ASSERT_EQ((int) expect.cache[i].cache_size, (int)actual.cache[i].cache_size);
			SMI_ASSERT_EQ((int) expect.cache[i].cache_level, (int)actual.cache[i].cache_level);
			SMI_ASSERT_EQ((int) expect.cache[i].cache_properties, (int)actual.cache[i].cache_properties);
			SMI_ASSERT_EQ((int) expect.cache[i].max_num_cu_shared, (int)actual.cache[i].max_num_cu_shared);
			SMI_ASSERT_EQ((int) expect.cache[i].num_cache_instance, (int)actual.cache[i].num_cache_instance);
		}

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_dpm_policy(smi_dpm_policy expect,
						       amdsmi_dpm_policy_t actual)
	{
		SMI_ASSERT_EQ(expect.num_supported, actual.num_supported);
		SMI_ASSERT_EQ(expect.cur, actual.cur);
		for (uint32_t i = 0; i < expect.num_supported; i++) {
			SMI_ASSERT_STR_EQ(expect.policies[i].policy_description, actual.policies[i].policy_description)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.policies[i].policy_id, actual.policies[i].policy_id)
				<< " for i = " << i;
		}

		return ::testing::AssertionSuccess();
	}
};

TEST_F(AmdsmiGpuMonitoring, InvalidParams)
{
	int ret;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	uint32_t sensor_ind = 0;

	ret = amdsmi_get_gpu_activity(MOCK_GPU_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_power_info(MOCK_GPU_HANDLE, sensor_ind, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_is_gpu_power_management_enabled(MOCK_GPU_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_clock_info(MOCK_GPU_HANDLE, AMDSMI_CLK_TYPE_GFX, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_temp_metric(MOCK_GPU_HANDLE, AMDSMI_TEMPERATURE_TYPE_EDGE,
								 AMDSMI_TEMP_CURRENT, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_temp_metric(MOCK_GPU_HANDLE, AMDSMI_TEMPERATURE_TYPE_EDGE,
								 AMDSMI_TEMP_CRITICAL, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_temp_metric(MOCK_GPU_HANDLE, AMDSMI_TEMPERATURE_TYPE_EDGE,
								 AMDSMI_TEMP_SHUTDOWN, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_pcie_info(MOCK_GPU_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_cache_info(MOCK_GPU_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_soc_pstate(MOCK_GPU_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdsmiGpuMonitoring, InvalSensorType)
{
	int ret;
	int64_t temperature;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	int temp_type = AMDSMI_TEMPERATURE_TYPE__MAX+1;
	ret = amdsmi_get_temp_metric(MOCK_GPU_HANDLE, (amdsmi_temperature_type_t)temp_type,
								 AMDSMI_TEMP_CURRENT, &temperature);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdsmiGpuMonitoring, IoctlFailed)
{
	int ret;
	amdsmi_engine_usage_t engine_res;
	amdsmi_power_info_t power_res;
	amdsmi_clk_info_t clock_res;
	bool is_power_management_enabled;
	int64_t temperature_res;
	int64_t temperature_limit_res;
	amdsmi_gpu_cache_info_t cache_res;
	amdsmi_pcie_info_t pcie_info;
	amdsmi_dpm_policy_t dpm_policy_info;
	uint32_t sensor_ind = 0;

	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_activity(MOCK_GPU_HANDLE, &engine_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_power_info(MOCK_GPU_HANDLE, sensor_ind, &power_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_is_gpu_power_management_enabled(MOCK_GPU_HANDLE, &is_power_management_enabled);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_clock_info(MOCK_GPU_HANDLE, AMDSMI_CLK_TYPE_GFX, &clock_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_temp_metric(MOCK_GPU_HANDLE, AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMP_CURRENT, &temperature_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_temp_metric(MOCK_GPU_HANDLE, AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMP_CRITICAL, &temperature_limit_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_gpu_cache_info(MOCK_GPU_HANDLE, &cache_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_pcie_info(MOCK_GPU_HANDLE, &pcie_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_soc_pstate(MOCK_GPU_HANDLE, &dpm_policy_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_set_soc_pstate(MOCK_GPU_HANDLE, 0);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdsmiGpuMonitoring, GetGpuActivity)
{
	int ret;
	amdsmi_engine_usage_t engine_usage;
	smi_device_info_ex in_payload;
	smi_gpu_performance_info mocked_resp = {};

	mocked_resp.usage.gfx_activity = 30;
	mocked_resp.usage.umc_activity = 40;
	mocked_resp.usage.mm_activity = 0;

	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	WhenCalling(std::bind(amdsmi_get_gpu_activity, MOCK_GPU_HANDLE, &engine_usage));
	ExpectCommand(SMI_CMD_CODE_GET_GPU_PERFORMANCE_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_engine_usage(mocked_resp.usage, engine_usage));
}

TEST_F(AmdsmiGpuMonitoring, GetPowerMeasure)
{
	int ret;
	amdsmi_power_info_t power_info;
	smi_device_info_ex in_payload;
	smi_gpu_performance_info mocked_resp = {};
	uint32_t sensor_ind = 0;

	mocked_resp.power.socket_power = 20;
	mocked_resp.power.gfx_voltage = 800;
	mocked_resp.power.soc_voltage = 61;
	mocked_resp.power.mem_voltage = 62;

	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	WhenCalling(std::bind(amdsmi_get_power_info, MOCK_GPU_HANDLE, sensor_ind, &power_info));
	ExpectCommand(SMI_CMD_CODE_GET_GPU_PERFORMANCE_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_power_measure(mocked_resp.power, power_info));
}

TEST_F(AmdsmiGpuMonitoring, GetIsPowerManagementEnabled)
{
	int ret;
	struct smi_data_query in_dev_enabled;
	bool is_power_management_enabled;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	union smi_data gpu_power_management = {};
	gpu_power_management.power_management_enabled = false;

	WhenCalling(std::bind(amdsmi_is_gpu_power_management_enabled, MOCK_GPU_HANDLE, &is_power_management_enabled));
	ExpectCommand(SMI_CMD_CODE_GET_SMI_DATA);
	SaveInputPayloadIn(&in_dev_enabled);
	PlantMockOutput(&gpu_power_management);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(gpu_power_management.power_management_enabled, is_power_management_enabled);
}

TEST_F(AmdsmiGpuMonitoring, GetClockMeasure)
{
	int ret;
	amdsmi_clk_info_t clock_measure;
	smi_device_info_ex in_payload;
	smi_gpu_performance_info gpu_perf_mocked = {};

	amdsmi_clk_type_t supported_ckl_type[7] = {AMDSMI_CLK_TYPE_GFX, AMDSMI_CLK_TYPE_MEM, AMDSMI_CLK_TYPE_MEM, AMDSMI_CLK_TYPE_VCLK0, AMDSMI_CLK_TYPE_VCLK1, AMDSMI_CLK_TYPE_DCLK0, AMDSMI_CLK_TYPE_DCLK1};
	for (unsigned i = 0; i < 7; i++) {
		gpu_perf_mocked.clock.min_clk[i] = 300 + i;
		gpu_perf_mocked.clock.cur_clk[i] = 400 + i;
		gpu_perf_mocked.clock.max_clk[i] = 500 + i;
		amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
		WhenCalling(std::bind(amdsmi_get_clock_info, MOCK_GPU_HANDLE,
				     supported_ckl_type[i], &clock_measure));
		ExpectCommand(SMI_CMD_CODE_GET_GPU_PERFORMANCE_INFO);
		SaveInputPayloadIn(&in_payload);
		PlantMockOutput(&gpu_perf_mocked);
		ret = performCall();

		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
		ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
		ASSERT_TRUE(equal_clock_measure(gpu_perf_mocked.clock, clock_measure, supported_ckl_type[i]));
	}
}

TEST_F(AmdsmiGpuMonitoring, GetThermalMeasure)
{
	int ret;
	int64_t temperature_measure;
	smi_device_info_ex in_payload;
	smi_gpu_performance_info mocked_resp = {};

	amdsmi_temperature_type_t supported_temp_type[3] = {AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMPERATURE_TYPE_HOTSPOT, AMDSMI_TEMPERATURE_TYPE_VRAM};

	for (unsigned i = 0; i < 3; i++) {
		mocked_resp.temp.temp[i] = 50 + i;
		amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
		WhenCalling(std::bind(amdsmi_get_temp_metric, MOCK_GPU_HANDLE,
				      supported_temp_type[i], AMDSMI_TEMP_CURRENT, &temperature_measure));
		ExpectCommand(SMI_CMD_CODE_GET_GPU_PERFORMANCE_INFO);
		SaveInputPayloadIn(&in_payload);
		PlantMockOutput(&mocked_resp);
		ret = performCall();
		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
		ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
		ASSERT_TRUE(equal_temperature_measure(mocked_resp.temp, temperature_measure, supported_temp_type[i]));
	}
}

TEST_F(AmdsmiGpuMonitoring, GetThermalLimit)
{
	int ret;
	int64_t temperature_limit;
	smi_device_info_ex in_payload;
	smi_gpu_performance_info mocked_resp = {};

	amdsmi_temperature_type_t supported_temp_type[4] = {AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMPERATURE_TYPE_HOTSPOT, AMDSMI_TEMPERATURE_TYPE_VRAM};

	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	for (unsigned i = 0; i < 4; i++) {
		mocked_resp.temp.temp[i] = 0;
		WhenCalling(std::bind(amdsmi_get_temp_metric, MOCK_GPU_HANDLE,
				      supported_temp_type[i], AMDSMI_TEMP_CRITICAL, &temperature_limit));
		ExpectCommand(SMI_CMD_CODE_GET_GPU_PERFORMANCE_INFO);
		SaveInputPayloadIn(&in_payload);
		PlantMockOutput(&mocked_resp);
		ret = performCall();
		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
		ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
		ASSERT_TRUE(equal_temperature_measure(mocked_resp.temp, temperature_limit, supported_temp_type[i]));
	}
}

TEST_F(AmdsmiGpuMonitoring, GetThermalLimitPLX)
{
	int ret;
	int64_t temperature_limit;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	ret = amdsmi_get_temp_metric(MOCK_GPU_HANDLE,
					AMDSMI_TEMPERATURE_TYPE_PLX, AMDSMI_TEMP_CRITICAL, &temperature_limit);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdsmiGpuMonitoring, GetThermalLimitHBM_0)
{
	int ret;
	int64_t temperature_limit;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	ret = amdsmi_get_temp_metric(MOCK_GPU_HANDLE,
					AMDSMI_TEMPERATURE_TYPE_HBM_0, AMDSMI_TEMP_CRITICAL, &temperature_limit);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(temperature_limit, 0);
}

TEST_F(AmdsmiGpuMonitoring, GetThermalLimitBadMetric)
{
	int ret;
	int64_t temperature_limit;
	int metric = AMDSMI_TEMP_CRITICAL+1;
	smi_device_info_ex in_payload;
	smi_gpu_performance_info mocked_resp = {};
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	WhenCalling(std::bind(amdsmi_get_temp_metric, MOCK_GPU_HANDLE,
					AMDSMI_TEMPERATURE_TYPE_EDGE, (amdsmi_temperature_metric_t)metric, &temperature_limit));
	ExpectCommand(SMI_CMD_CODE_GET_GPU_PERFORMANCE_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}

TEST_F(AmdsmiGpuMonitoring, GetThermalLimitZeroValue)
{
	int ret;
	int64_t temperature_limit;
	smi_device_info_ex in_payload;
	smi_gpu_performance_info mocked_resp = {};
	amdsmi_temperature_type_t supported_temp_type[4] = {AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMPERATURE_TYPE_HOTSPOT, AMDSMI_TEMPERATURE_TYPE_VRAM};
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	for (unsigned i = 0; i < 4; i++) {
		mocked_resp.temp_limit.temp[i] = 0;
		WhenCalling(std::bind(amdsmi_get_temp_metric, MOCK_GPU_HANDLE,
				      supported_temp_type[i], AMDSMI_TEMP_CRITICAL, &temperature_limit));
		ExpectCommand(SMI_CMD_CODE_GET_GPU_PERFORMANCE_INFO);
		SaveInputPayloadIn(&in_payload);
		PlantMockOutput(&mocked_resp);
		ret = performCall();
		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	}
}

TEST_F(AmdsmiGpuMonitoring, GetThermalShutdown)
{
	int ret;
	int64_t temperature_limit;
	smi_device_info_ex in_payload;
	smi_gpu_performance_info mocked_resp = {};

	amdsmi_temperature_type_t supported_temp_type[4] = {AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMPERATURE_TYPE_HOTSPOT, AMDSMI_TEMPERATURE_TYPE_VRAM};

	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	for (unsigned i = 0; i < 4; i++) {
		mocked_resp.temp.temp[i] = 0;
		WhenCalling(std::bind(amdsmi_get_temp_metric, MOCK_GPU_HANDLE,
				      supported_temp_type[i], AMDSMI_TEMP_SHUTDOWN, &temperature_limit));
		ExpectCommand(SMI_CMD_CODE_GET_GPU_PERFORMANCE_INFO);
		SaveInputPayloadIn(&in_payload);
		PlantMockOutput(&mocked_resp);
		ret = performCall();
		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
		ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
		ASSERT_TRUE(equal_temperature_measure(mocked_resp.temp, temperature_limit, supported_temp_type[i]));
	}
}

TEST_F(AmdsmiGpuMonitoring, GetPcieInfo)
{
	int ret;
	smi_device_info in_payload;
	amdsmi_pcie_info_t pcie_info;
	struct smi_pcie_info mocked_resp = {};
	smi_in_hdr in_header;

	mocked_resp.pcie_static.max_pcie_width = 16;
	mocked_resp.pcie_static.max_pcie_speed = 32;
	mocked_resp.pcie_static.pcie_interface_version = 4;
	mocked_resp.pcie_static.slot_type = SMI_CARD_FORM_FACTOR_PCIE;
	mocked_resp.pcie_static.max_pcie_interface_version = 5;

	mocked_resp.pcie_metric.pcie_speed = 2;
	mocked_resp.pcie_metric.pcie_width = 16;
	mocked_resp.pcie_metric.pcie_bandwidth = 300;
	mocked_resp.pcie_metric.pcie_replay_count = 1;
	mocked_resp.pcie_metric.pcie_l0_to_recovery_count = 2;
	mocked_resp.pcie_metric.pcie_replay_roll_over_count = 3;
	mocked_resp.pcie_metric.pcie_nak_sent_count = 4;
	mocked_resp.pcie_metric.pcie_nak_received_count = 4;

	smi_asic_info gpu_info_mock = {};
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
	PrepareIoctl(SMI_CMD_CODE_GET_ASIC_INFO, &in_header, &in_payload, gpu_info_mock, AMDSMI_STATUS_SUCCESS);

	WhenCalling(std::bind(amdsmi_get_pcie_info, &GPU_MOCK_HANDLE, &pcie_info));
	ExpectCommand(SMI_CMD_CODE_GET_PCIE_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_pcie_info(mocked_resp, pcie_info));

	mocked_resp.pcie_static.pcie_interface_version = 5;
	gpu_info_mock.device_id = 0x74A1;
	PrepareIoctl(SMI_CMD_CODE_GET_ASIC_INFO, &in_header, &in_payload, gpu_info_mock, AMDSMI_STATUS_SUCCESS);
	WhenCalling(std::bind(amdsmi_get_pcie_info, &GPU_MOCK_HANDLE, &pcie_info));
	ExpectCommand(SMI_CMD_CODE_GET_PCIE_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_pcie_info(mocked_resp, pcie_info));
}

TEST_F(AmdsmiGpuMonitoring, GetPcieInfoApiFailed)
{
	int ret;
	smi_device_info in_payload;
	amdsmi_pcie_info_t pcie_info;
	struct smi_pcie_info mocked_resp = {};
	smi_in_hdr in_header;

	mocked_resp.pcie_static.max_pcie_width = 16;
	mocked_resp.pcie_static.max_pcie_speed = 32;
	mocked_resp.pcie_static.pcie_interface_version = 4;
	mocked_resp.pcie_static.slot_type = SMI_CARD_FORM_FACTOR_PCIE;
	mocked_resp.pcie_static.max_pcie_interface_version = 5;

	mocked_resp.pcie_metric.pcie_speed = 2;
	mocked_resp.pcie_metric.pcie_width = 16;
	mocked_resp.pcie_metric.pcie_bandwidth = 300;
	mocked_resp.pcie_metric.pcie_replay_count = 1;
	mocked_resp.pcie_metric.pcie_l0_to_recovery_count = 2;
	mocked_resp.pcie_metric.pcie_replay_roll_over_count = 3;
	mocked_resp.pcie_metric.pcie_nak_sent_count = 4;
	mocked_resp.pcie_metric.pcie_nak_received_count = 4;

	smi_asic_info gpu_info_mock = {};
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
	PrepareIoctl(SMI_CMD_CODE_GET_ASIC_INFO, &in_header, &in_payload, gpu_info_mock, AMDSMI_STATUS_SUCCESS);

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_PCIE_INFO)))
		.WillOnce(amdsmi::SetResponseStatus(AMDSMI_STATUS_API_FAILED));
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = amdsmi_get_pcie_info(&GPU_MOCK_HANDLE, &pcie_info);

	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdsmiGpuMonitoring, GetPcieInfoFail)
{
	int ret;
	smi_device_info in_payload;
	struct smi_pcie_info mocked_resp = {};
	amdsmi_pcie_info_t pcie_info;
	smi_in_hdr in_header;

	mocked_resp.pcie_static.max_pcie_width = 16;
	mocked_resp.pcie_static.max_pcie_speed = 32;
	mocked_resp.pcie_static.pcie_interface_version = 5;
	mocked_resp.pcie_static.slot_type = SMI_CARD_FORM_FACTOR_PCIE;
	mocked_resp.pcie_static.max_pcie_interface_version = 5;

	mocked_resp.pcie_metric.pcie_speed = UINT32_MAX;
	mocked_resp.pcie_metric.pcie_width = 16;
	mocked_resp.pcie_metric.pcie_bandwidth = 300;
	mocked_resp.pcie_metric.pcie_replay_count = 1;
	mocked_resp.pcie_metric.pcie_l0_to_recovery_count = 2;
	mocked_resp.pcie_metric.pcie_replay_roll_over_count = 3;
	mocked_resp.pcie_metric.pcie_nak_sent_count = 4;
	mocked_resp.pcie_metric.pcie_nak_received_count = 4;

	smi_asic_info gpu_info_mock = {};
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
	PrepareIoctl(SMI_CMD_CODE_GET_ASIC_INFO, &in_header, &in_payload, gpu_info_mock, AMDSMI_STATUS_SUCCESS);

	WhenCalling(std::bind(amdsmi_get_pcie_info, &GPU_MOCK_HANDLE, &pcie_info));
	ExpectCommand(SMI_CMD_CODE_GET_PCIE_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdsmiGpuMonitoring, TestPcieLinkSpeedConversion)
{
	int ret;
	uint32_t returned_speed;
	uint64_t dev_id = 29601;

	// from PCIe standard, in MT/s
	uint32_t expected_speed[] = { 2500, 5000, 8000, 16000, 32000, 64000 };

	for (uint8_t pcie_gen = 1; pcie_gen < 6; pcie_gen++) {
		ret = amdsmi_get_pcie_speed_from_pcie_type(pcie_gen, &returned_speed, dev_id);
		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
		ASSERT_EQ(returned_speed, expected_speed[pcie_gen]);
	}

	dev_id = 29856;
	for (uint8_t pcie_gen = 1; pcie_gen < 6; pcie_gen++) {
		ret = amdsmi_get_pcie_speed_from_pcie_type(pcie_gen, &returned_speed, dev_id);
		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
		ASSERT_EQ(returned_speed, expected_speed[pcie_gen-1]);
	}

	// unsupported standards
	ret = amdsmi_get_pcie_speed_from_pcie_type(UINT32_MAX, &returned_speed, dev_id);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);


	ret = amdsmi_get_pcie_speed_from_pcie_type(7, &returned_speed, dev_id);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdsmiGpuMonitoring, TestCacheInfo)
{
	int ret;
	amdsmi_gpu_cache_info_t cache_info;
	smi_device_info in_payload;
	smi_gpu_cache_info mocked_resp = {};

	mocked_resp.num_cache_types = AMDSMI_MAX_CACHE_TYPES;

	for (int i = 0; i < AMDSMI_MAX_CACHE_TYPES; i++) {
		mocked_resp.cache[i].cache_size = i * 2 + 1;
		mocked_resp.cache[i].cache_level = i * 2 + 2;
		mocked_resp.cache[i].cache_properties = i * 2 + 3;
		mocked_resp.cache[i].max_num_cu_shared = i * 2 + 4;
		mocked_resp.cache[i].num_cache_instance = i * 2 + 5;
	}

	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	WhenCalling(std::bind(amdsmi_get_gpu_cache_info, MOCK_GPU_HANDLE, &cache_info));
	ExpectCommand(SMI_CMD_CODE_GET_GPU_CACHE_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_cache_info(mocked_resp, cache_info));
}

TEST_F(AmdsmiGpuMonitoring, TestCacheInfWrongNumCacheType)
{
	int ret;
	amdsmi_gpu_cache_info_t cache_info;
	smi_gpu_cache_info mocked_resp = {};

	mocked_resp.num_cache_types = 0xFFFFFFFF;

	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	WhenCalling(std::bind(amdsmi_get_gpu_cache_info, MOCK_GPU_HANDLE, &cache_info));
	ExpectCommand(SMI_CMD_CODE_GET_GPU_CACHE_INFO);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdsmiGpuMonitoring, GetSocPstate)
{
	int ret;
	amdsmi_dpm_policy_t dpm_policy_info;
	smi_device_info in_payload;
	smi_dpm_policy mocked_resp = {};

	mocked_resp.num_supported = 4;
	mocked_resp.cur = 1;
	mocked_resp.policies[0].policy_id = 0;
#ifdef _WIN64
	strcpy_s(mocked_resp.policies[0].policy_description, sizeof(mocked_resp.policies[0].policy_description), "pstate_default");
#else
	strcpy(mocked_resp.policies[0].policy_description, "pstate_default");
#endif
	mocked_resp.policies[1].policy_id = 1;
#ifdef _WIN64
	strcpy_s(mocked_resp.policies[1].policy_description, sizeof(mocked_resp.policies[1].policy_description), "soc_pstate_0");
#else
	strcpy(mocked_resp.policies[1].policy_description, "soc_pstate_0");
#endif
	mocked_resp.policies[2].policy_id = 2;
#ifdef _WIN64
	strcpy_s(mocked_resp.policies[2].policy_description, sizeof(mocked_resp.policies[2].policy_description), "soc_pstate_1");
#else
	strcpy(mocked_resp.policies[2].policy_description, "soc_pstate_1");
#endif
	mocked_resp.policies[3].policy_id = 3;
#ifdef _WIN64
	strcpy_s(mocked_resp.policies[3].policy_description, sizeof(mocked_resp.policies[3].policy_description), "soc_pstate_2");
#else
	strcpy(mocked_resp.policies[3].policy_description, "soc_pstate_2");
#endif

	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	WhenCalling(std::bind(amdsmi_get_soc_pstate, MOCK_GPU_HANDLE, &dpm_policy_info));
	ExpectCommand(SMI_CMD_CODE_GET_SOC_PSTATE);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_dpm_policy(mocked_resp, dpm_policy_info));
}

TEST_F(AmdsmiGpuMonitoring, SetSocPstate)
{
	int ret;
	struct smi_set_dpm_policy in_payload;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;
	uint32_t policy_id = 1;

	WhenCalling(std::bind(amdsmi_set_soc_pstate, MOCK_GPU_HANDLE, policy_id));
	ExpectCommand(SMI_CMD_CODE_SET_SOC_PSTATE);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

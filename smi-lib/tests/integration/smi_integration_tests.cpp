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

#include <iostream>
#include <chrono>
#include <thread>
#include <limits>
#include <cmath>
#include <cinttypes>

#include <gtest/gtest.h>
extern "C" {
#include "amdsmi.h"
}

TEST(amdsmiIntegrationTests, InitTest)
{
	EXPECT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);

	EXPECT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	EXPECT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);

	EXPECT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	EXPECT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	EXPECT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
}

TEST(amdsmiIntegrationTests, InitLibaryWithAmdCPUsFlagShouldPass)
{
	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_AMD_CPUS), AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
}

TEST(amdsmiIntegrationTests, InitLibaryWithAmdGPUsFlagShouldPass)
{
	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_AMD_GPUS), AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
}

TEST(amdsmiIntegrationTests, InitLibaryWithNonAmdCPUsFlagShouldPass)
{
	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_NON_AMD_CPUS), AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
}

TEST(amdsmiIntegrationTests, InitLibaryWithAmdNonGPUsFlagShouldPass)
{
	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_NON_AMD_GPUS), AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
}

static void walkthrough_test()
{
	uint32_t dev_cnt;
	int ret;
	amdsmi_socket_handle socket = NULL;
	amdsmi_processor_handle processor_handle;
	amdsmi_driver_info_t driver_version;
	amdsmi_asic_info_t asic_info;
	amdsmi_gpu_cache_info_t cache_info;
	char uuid[AMDSMI_GPU_UUID_SIZE];
	unsigned int uuid_length = AMDSMI_GPU_UUID_SIZE;
	amdsmi_pcie_info_t pcie_info;
	amdsmi_power_cap_info_t pwr_info;
	amdsmi_pf_fb_info_t pf_fb_info;
	amdsmi_vbios_info_t vbios_info;
	amdsmi_fw_info_t fw_info;
	uint32_t num_vf_supported;
	uint32_t num_vf_enabled;
	amdsmi_partition_info_t *partitioning_info;
	amdsmi_vf_data_t vf_data;
	amdsmi_board_info_t board_info;
	amdsmi_vf_handle_t vf_handle;
	amdsmi_bdf_t vf_bdf;
	amdsmi_version_t version;
	uint32_t processor_index = AMDSMI_MAX_DEVICES + 100;
	amdsmi_driver_model_type_t driver_model;
	uint32_t sensor_ind = 0;

	ASSERT_EQ(amdsmi_get_lib_version(&version), AMDSMI_STATUS_SUCCESS);
	std::cout << "AMD SMI Library version: " <<
		version.major << "." << version.minor << "." << version.release << std::endl;

	ASSERT_EQ(amdsmi_get_processor_handles(socket, &dev_cnt, NULL), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, (unsigned)32);
	std::cout << "Number of processors on server: " << dev_cnt << '\n';

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(socket, &dev_cnt, &processors[0]), AMDSMI_STATUS_SUCCESS);

	ASSERT_EQ(amdsmi_get_index_from_processor_handle(processors[0], &processor_index), AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(processor_index, 0);

	/* below use printf since data format can be hex and dec mixed */
	for (uint32_t i = 0; i < dev_cnt; ++i) {
		printf("GPU %u\n", i);

		ASSERT_EQ(amdsmi_get_gpu_driver_info(processors[i], &driver_version), AMDSMI_STATUS_SUCCESS);
		printf("Host driver name: %s\n", driver_version.driver_name);
		printf("Host driver version: %s\n", driver_version.driver_version);
		printf("Host driver date: %s\n", driver_version.driver_date);

		ret = amdsmi_get_gpu_driver_model(processors[i], &driver_model);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		if (ret == AMDSMI_STATUS_SUCCESS) {
			printf("Driver model: %d\n", driver_model);
		} else {
			printf("Driver model: %s\n", "N/A");
		}

		ASSERT_EQ(amdsmi_get_gpu_asic_info(processors[i], &asic_info), AMDSMI_STATUS_SUCCESS);
		printf("ASIC info:\n");
		printf("    Market name: %s\n", asic_info.market_name);
		printf("    Vendor ID: 0x%x\n", asic_info.vendor_id);
		printf("    Subvendor ID: 0x%x\n", asic_info.subvendor_id);
		printf("    Device ID: 0x%" PRIx64 "\n", asic_info.device_id);
		printf("    Revision ID: 0x%x\n", asic_info.rev_id);
		printf("    ASIC Serial: 0x%s\n", asic_info.asic_serial);
		printf("    OAM ID: %d\n", asic_info.oam_id);
		printf("    Num Of Compute Units: %d\n", asic_info.num_of_compute_units);
		printf("    Subsystem ID: 0x%x\n", asic_info.subsystem_id);
		printf("    Target Graphics Version: %" PRIu64 "\n", asic_info.target_graphics_version);
		ASSERT_EQ(amdsmi_get_gpu_device_uuid(processors[i], &uuid_length, uuid),
			  AMDSMI_STATUS_SUCCESS);
		printf("GPU UUID:\n");
		printf("    UUID: %s\n", uuid);
		printf("    UUID Length: %u\n", uuid_length);

		ASSERT_EQ(amdsmi_get_processor_handle_from_uuid(uuid, &processor_handle), AMDSMI_STATUS_SUCCESS);
		ASSERT_EQ(processor_handle, processors[i]);

		ret = amdsmi_get_pcie_info(processors[i], &pcie_info);
		EXPECT_TRUE((ret == AMDSMI_STATUS_SUCCESS) || (ret == AMDSMI_STATUS_NOT_SUPPORTED));
		if (ret == AMDSMI_STATUS_SUCCESS) {
			EXPECT_TRUE(pcie_info.pcie_static.max_pcie_speed > 0);
			EXPECT_TRUE((pcie_info.pcie_static.max_pcie_width >= 16) &&
					(pcie_info.pcie_static.max_pcie_width <= 32));

			printf("PCIe static info:\n");
			printf("    Maximum PCIe width: %d\n", pcie_info.pcie_static.max_pcie_width);
			printf("    Maximum PCIe speed: %d\n", pcie_info.pcie_static.max_pcie_speed);
			printf("    PCIe interface version: %u\n", pcie_info.pcie_static.pcie_interface_version);
			printf("    Slot type: %d\n", pcie_info.pcie_static.slot_type);
			printf("    Maximum PCIe interface version: %d\n", pcie_info.pcie_static.max_pcie_interface_version);


			printf("PCIe metric info:\n");
			printf("    PCIe speed: %u\n", pcie_info.pcie_metric.pcie_speed);
			printf("    PCIe width: %u\n", pcie_info.pcie_metric.pcie_width);
			printf("    PCIe bandwidth: %u\n", pcie_info.pcie_metric.pcie_bandwidth);
			printf("    PCIe replay count: %" PRIu64 "\n", pcie_info.pcie_metric.pcie_replay_count);
			printf("    PCIe l0 to recovery count: %" PRIu64 "\n", pcie_info.pcie_metric.pcie_l0_to_recovery_count);
			printf("    PCIe replay roll over count: %" PRIu64 "\n", pcie_info.pcie_metric.pcie_replay_roll_over_count);
			printf("    PCIe nak sent count: %" PRIu64 "\n", pcie_info.pcie_metric.pcie_nak_sent_count);
			printf("    PCIe nak received count: %" PRIu64 "\n", pcie_info.pcie_metric.pcie_nak_received_count);
			printf("    PCIe lc perf other end recovery count: %u\n", pcie_info.pcie_metric.pcie_lc_perf_other_end_recovery_count);
		}

		ASSERT_EQ(amdsmi_get_power_cap_info(processors[i], sensor_ind, &pwr_info), AMDSMI_STATUS_SUCCESS);

		EXPECT_TRUE(pwr_info.default_power_cap == 0);
		EXPECT_TRUE(pwr_info.dpm_cap <= 16);
		EXPECT_TRUE((pwr_info.max_power_cap == UINT64_MAX) || (pwr_info.power_cap <= pwr_info.max_power_cap));
		EXPECT_TRUE((pwr_info.min_power_cap == UINT64_MAX) || (pwr_info.power_cap >= pwr_info.min_power_cap));

		printf("Power capabilities:\n");
		printf("    DPM: %" PRIu64 "\n", pwr_info.dpm_cap);
		printf("    Power: %" PRIu64 "W\n", pwr_info.power_cap);
		printf("    Max power: %" PRIu64 "W\n", pwr_info.max_power_cap);
		printf("    Min power: %" PRIu64 "W\n", pwr_info.min_power_cap);

		ASSERT_EQ(amdsmi_get_fb_layout(processors[i], &pf_fb_info), AMDSMI_STATUS_SUCCESS);

		EXPECT_TRUE((pf_fb_info.total_fb_size >= 1024) &&
			    (pf_fb_info.total_fb_size <= 262144));
		EXPECT_TRUE((pf_fb_info.pf_fb_reserved >= 1) &&
			    (pf_fb_info.pf_fb_reserved <= 512));
		EXPECT_TRUE((pf_fb_info.max_vf_fb_usable >= 1024) &&
			    (pf_fb_info.max_vf_fb_usable <= pf_fb_info.total_fb_size));
		EXPECT_TRUE((pf_fb_info.min_vf_fb_usable > 0) &&
			    (pf_fb_info.min_vf_fb_usable <= pf_fb_info.total_fb_size));
		EXPECT_EQ(pf_fb_info.max_vf_fb_usable, pf_fb_info.total_fb_size - pf_fb_info.pf_fb_reserved);


		printf("FB info:\n");
		printf("    Total size: %" PRIu64 " MB\n", (uint64_t)pf_fb_info.total_fb_size);
		printf("    Reserved size: %" PRIu64 " MB\n", (uint64_t)pf_fb_info.pf_fb_reserved);
		printf("    Maximum usable: %" PRIu64 " MB\n", (uint64_t)pf_fb_info.max_vf_fb_usable);
		printf("    Minimum usable: %" PRIu64 " MB\n", (uint64_t)pf_fb_info.min_vf_fb_usable);

		ASSERT_EQ(amdsmi_get_gpu_vbios_info(processors[i], &vbios_info), AMDSMI_STATUS_SUCCESS);
		printf("vBIOS info:\n");
		printf("    Build date: %s\n", vbios_info.build_date);
		printf("    Name: %s\n", vbios_info.name);
		printf("    Part num (P/N): %s\n", vbios_info.part_number);
		printf("    VBIOS Version: %s\n", vbios_info.version);

		ret = amdsmi_get_gpu_cache_info(processors[i], &cache_info);
		EXPECT_TRUE((ret == AMDSMI_STATUS_SUCCESS) || (ret == AMDSMI_STATUS_NOT_SUPPORTED));
		if (ret == AMDSMI_STATUS_SUCCESS) {
			printf("Cache info:\n");
			printf("    Cache count: %d\n", cache_info.num_cache_types);
			for (uint32_t j = 0; j < cache_info.num_cache_types; ++j) {
				printf("    Cache properties: %d\n",
					cache_info.cache[j].cache_properties);
				printf("    Cache size: %d KB\n",
						cache_info.cache[j].cache_size);
				printf("    Cache level: %d\n",
						cache_info.cache[j].cache_level);
				printf("    Num Compute Units share: %d\n",
						cache_info.cache[j].max_num_cu_shared);
				printf("    Max num of instance of this cache type: %d\n",
						cache_info.cache[j].num_cache_instance);
			}
		}

		ASSERT_EQ(amdsmi_get_fw_info(processors[i], &fw_info), AMDSMI_STATUS_SUCCESS);

		EXPECT_GE(fw_info.num_fw_info, 1);
		EXPECT_LE(fw_info.num_fw_info, AMDSMI_FW_ID__MAX - 1);

		printf("Firmware info:\n");
		printf("    Firmware count: %d\n", fw_info.num_fw_info);
		for (uint32_t j = 0; j < fw_info.num_fw_info; ++j) {
			if (fw_info.fw_info_list[j].fw_version != 0) {
				printf("    Firmware ID: 0x%x\n",
				       fw_info.fw_info_list[j].fw_id);
				printf("    Firmware version: 0x%" PRIx64 "\n",
				       fw_info.fw_info_list[j].fw_version);
			}
		}
		ASSERT_EQ(amdsmi_get_num_vf(processors[i], &num_vf_enabled, &num_vf_supported),
			  AMDSMI_STATUS_SUCCESS);
		ret = amdsmi_set_num_vf(processors[i], num_vf_enabled);
		EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS
					|| ret == AMDSMI_STATUS_BUSY);

		EXPECT_TRUE(num_vf_supported <= 31);
		printf("Num VF supported: %d\n", num_vf_supported);

		EXPECT_TRUE(num_vf_enabled <= 31);
		EXPECT_LE(num_vf_enabled, num_vf_supported);
		printf("Num VF enabled: %d\n", num_vf_enabled);
		ASSERT_EQ(amdsmi_get_gpu_board_info(processors[i], &board_info), AMDSMI_STATUS_SUCCESS);
		printf("Board model: %s\n", board_info.model_number);
		printf("Board product serial: %s\n", board_info.product_serial);
		printf("Board FRU ID: %s\n", board_info.fru_id);
		printf("Board manufacturer name: %s\n", board_info.manufacturer_name);
		printf("Board product name: %s\n", board_info.product_name);

		partitioning_info = new amdsmi_partition_info_t[num_vf_enabled];
		ASSERT_EQ(amdsmi_get_vf_partition_info(processors[i], num_vf_enabled,
							partitioning_info),
			  AMDSMI_STATUS_SUCCESS);

		for (uint32_t j = 0; j < num_vf_enabled; ++j) {
			ASSERT_EQ(amdsmi_get_vf_handle_from_vf_index(processors[i], j, &vf_handle),
				  AMDSMI_STATUS_SUCCESS);
			ASSERT_EQ(vf_handle.handle, partitioning_info[j].id.handle);
			ASSERT_EQ(amdsmi_get_vf_bdf(vf_handle, &vf_bdf), AMDSMI_STATUS_SUCCESS);

			printf("Partitioning info for VF%d\n", j);
			printf("    Handle: 0x%" PRIx64 "\n", partitioning_info[j].id.handle);
			printf("    FB offset: %" PRIu64 " MB\n", (uint64_t)partitioning_info[j].fb.fb_offset);
			printf("    FB size:   %" PRIu64 " MB\n", (uint64_t)partitioning_info[j].fb.fb_size);
		}
		for (uint32_t j = 0; j < num_vf_enabled; ++j) {
			printf("UUID info for VF%d\n", j);
			ASSERT_EQ(amdsmi_get_vf_uuid(partitioning_info[j].id,
							  &uuid_length, uuid),
				  AMDSMI_STATUS_SUCCESS);
			printf("    UUID: %s\n", uuid);
			printf("    UUID Length: %u\n", uuid_length);
			ASSERT_EQ(amdsmi_get_vf_handle_from_uuid(uuid, &vf_handle), AMDSMI_STATUS_SUCCESS);
			ASSERT_EQ(vf_handle.handle, partitioning_info[j].id.handle);
			printf("Scheduler info for VF%d\n", j);
			ASSERT_EQ(amdsmi_get_vf_data(partitioning_info[j].id,
							 &vf_data),
				  AMDSMI_STATUS_SUCCESS);
			printf("    Boot up time: %" PRIu64 " us\n", vf_data.sched.boot_up_time);
			printf("    Flr count: %" PRIu64 "\n", vf_data.sched.flr_count);

			switch (vf_data.sched.state) {
			case AMDSMI_VF_STATE_UNAVAILABLE:
				printf("    State: AMDSMI_VF_STATE_UNAVAILABLE(%d)\n",
				       vf_data.sched.state);
				break;
			case AMDSMI_VF_STATE_AVAILABLE:
				printf("    State: AMDSMI_VF_STATE_AVAILABLE(%d)\n",
				       vf_data.sched.state);
				break;
			case AMDSMI_VF_STATE_ACTIVE:
				printf("    State: AMDSMI_VF_STATE_ACTIVE(%d)\n",
				       vf_data.sched.state);
				break;
			case AMDSMI_VF_STATE_SUSPENDED:
				printf("    State: AMDSMI_VF_STATE_SUSPENDED(%d)\n",
				       vf_data.sched.state);
				break;
			case AMDSMI_VF_STATE_FULLACCESS:
				printf("    State: AMDSMI_VF_STATE_FULLACCESS(%d)\n",
				       vf_data.sched.state);
				break;
			case AMDSMI_VF_STATE_DEFAULT_AVAILABLE:
				printf("    State: AMDSMI_VF_STATE_DEFAULT_AVAILABLE(%d)\n",
				       vf_data.sched.state);
				break;
			default:
				FAIL() << "VF state invalid: " << vf_data.sched.state;
				break;
			}
			printf("    Last boot start: %s\n", vf_data.sched.last_boot_start);
			printf("    Last boot end: %s\n", vf_data.sched.last_boot_end);
			printf("    Last shutdown start: %s\n", vf_data.sched.last_shutdown_start);
			printf("    Last shutdown end: %s\n", vf_data.sched.last_shutdown_end);
			printf("    Shutdown time: %" PRIu64 " us\n", vf_data.sched.shutdown_time);
			printf("    Last reset start: %s\n", vf_data.sched.last_reset_start);
			printf("    Last reset end: %s\n", vf_data.sched.last_reset_end);
			printf("    Reset time: %" PRIu64 " us\n", vf_data.sched.reset_time);
			printf("    Current active time: %s\n", vf_data.sched.current_active_time);
			printf("    Current running time: %s\n", vf_data.sched.current_running_time);
			printf("    Total active time: %s\n", vf_data.sched.total_active_time);
			printf("    Total running time: %s\n", vf_data.sched.total_running_time);

			printf("    Is enabled: %d\n", vf_data.guard.enabled);

			for (unsigned guard_type = 0; guard_type < AMDSMI_GUARD_EVENT__MAX;
			     ++guard_type) {
				if (guard_type == 0)
					printf("    Guard type: GUARD_EVENT_FLR \n");
				if (guard_type == 1)
					printf("    Guard type: GUARD_EVENT_EXCLUSIVE_MOD \n");
				if (guard_type == 2)
					printf("    Guard type: GUARD_EVENT_EXCLUSIVE_TIMEOUT \n");
				if (guard_type == 3)
					printf("    Guard type: GUARD_EVENT_ALL_INT \n");

				switch (vf_data.guard.guard[guard_type].state) {
				case AMDSMI_GUARD_STATE_NORMAL:
					printf("		State: AMDSMI_GUARD_STATE_NORMAL(%d)\n", vf_data.guard.guard[guard_type].state);
					break;
				case AMDSMI_GUARD_STATE_FULL:
					printf("		State: AMDSMI_GUARD_STATE_FULL(%d)\n", vf_data.guard.guard[guard_type].state);
					break;
				case AMDSMI_GUARD_STATE_OVERFLOW:
					printf("		State: AMDSMI_GUARD_STATE_OVERFLOW(%d)\n", vf_data.guard.guard[guard_type].state);
					break;
				default:
					printf("		State: INVALID(%d)\n", vf_data.guard.guard[guard_type].state);
					break;
				}

				printf("		Amount: %u\n", vf_data.guard.guard[guard_type].amount);
				printf("		Interval: %" PRIu64 "\n", vf_data.guard.guard[guard_type].interval);
				printf("		Threshold: %u\n", vf_data.guard.guard[guard_type].threshold);
				printf("		Active: %u\n", vf_data.guard.guard[guard_type].active);
			}
		}
		delete[] partitioning_info;
		std::cout << "\n----------------------------------------------------------\n";
	}

	free(processors);
}

TEST(amdsmiIntegrationTests, WalkthroughTest)
{
	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	walkthrough_test();
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
}

#if THREAD_SAFE
TEST(amdsmiIntegrationTests, WalkthroughTestMultithreaded)
{
	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	std::thread t0(walkthrough_test);
	std::thread t1(walkthrough_test);
	std::thread t2(walkthrough_test);
	std::thread t3(walkthrough_test);

	t0.join();
	t1.join();
	t2.join();
	t3.join();
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
}
#endif

TEST(amdsmiIntegrationTests, DeviceBDFTests)
{
	uint32_t dev_cnt = AMDSMI_MAX_DEVICES;

	amdsmi_bdf_t dev_bdf;
	amdsmi_processor_handle dev_handle;

	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(NULL, &dev_cnt, &processors[0]), AMDSMI_STATUS_SUCCESS);

	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, (unsigned)AMDSMI_MAX_DEVICES);

	for (uint32_t i = 0; i < dev_cnt; ++i) {
		printf("GPU %u\n", i);
		ASSERT_EQ(amdsmi_get_gpu_device_bdf(processors[i], &dev_bdf), AMDSMI_STATUS_SUCCESS);
		ASSERT_EQ(amdsmi_get_processor_handle_from_bdf(dev_bdf, &dev_handle), AMDSMI_STATUS_SUCCESS);
	}

	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	free(processors);
}

TEST(amdsmiIntegrationTests, GpuPerformanceTest)
{
	uint32_t dev_cnt = AMDSMI_MAX_DEVICES;
	amdsmi_asic_info_t asic_info;
	amdsmi_engine_usage_t engine_usage;
	amdsmi_power_info_t power_info;
	amdsmi_dpm_policy_t dpm_policy_info;
	uint32_t dpm_policy_default;
	amdsmi_clk_info_t clock_measure;
	int64_t temp_measure;
	amdsmi_vram_info_t vram_info;
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;
	uint32_t sensor_ind = 0;

	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(NULL, &dev_cnt, &processors[0]), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, (unsigned)AMDSMI_MAX_DEVICES);
	std::cout << "Number of processors on server: " << dev_cnt << '\n';

	for (uint32_t i = 0; i < dev_cnt; ++i) {
		std::cout << "GPU " << i << '\n';
		ASSERT_EQ(amdsmi_get_gpu_asic_info(processors[i], &asic_info), AMDSMI_STATUS_SUCCESS);

		ret = amdsmi_get_gpu_activity(processors[i], &engine_usage);

		EXPECT_TRUE((ret == AMDSMI_STATUS_SUCCESS) || (ret == AMDSMI_STATUS_NOT_SUPPORTED));
		if (ret == AMDSMI_STATUS_SUCCESS) {
			EXPECT_TRUE(engine_usage.gfx_activity <= 100);

			std::cout << "GPU activity: " << std::endl;
			std::cout << "    GFX usage: " << engine_usage.gfx_activity << std::endl;
			std::cout << "    Memory usage: "
				<< std::to_string(engine_usage.umc_activity)
				<< std::endl;

			std::cout << "    mm_activity: " << engine_usage.mm_activity << std::endl;
		}

		ret = amdsmi_get_power_info(processors[i], sensor_ind, &power_info);

		EXPECT_TRUE((ret == AMDSMI_STATUS_SUCCESS) || (ret == AMDSMI_STATUS_NOT_SUPPORTED));
		if (ret == AMDSMI_STATUS_SUCCESS) {
			EXPECT_TRUE(power_info.socket_power <= 750);
			EXPECT_TRUE(power_info.gfx_voltage <= 12000 || power_info.gfx_voltage == std::numeric_limits<uint32_t>::max());
			EXPECT_TRUE(power_info.soc_voltage <= 12000 || power_info.soc_voltage == std::numeric_limits<uint32_t>::max());
			EXPECT_TRUE(power_info.mem_voltage <= 12000 || power_info.mem_voltage == std::numeric_limits<uint32_t>::max());

			std::cout << "Power measure: " << std::endl;
			std::cout << "    power: " << power_info.socket_power << std::endl;
			std::cout << "    gfx voltage: "
				<< std::to_string(power_info.gfx_voltage)
				<< std::endl;
			std::cout << "    soc voltage: " << power_info.soc_voltage << std::endl;
			std::cout << "    mem voltage: " << power_info.mem_voltage << std::endl;
		}

		ret = amdsmi_get_soc_pstate(processors[i], &dpm_policy_info);
		EXPECT_TRUE((ret == AMDSMI_STATUS_SUCCESS) || (ret == AMDSMI_STATUS_NOT_SUPPORTED));

		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "Soc pstate: " << std::endl;
			std::cout << "    num supported: " << dpm_policy_info.num_supported << std::endl;
			std::cout << "    current: " << dpm_policy_info.cur << std::endl;
			std::cout << "	  policies: " << std::endl;
			for (unsigned int j = 0; j < dpm_policy_info.num_supported; ++j) {
				std::cout << "	  	  policy id: " << dpm_policy_info.policies[j].policy_id << std::endl;
				std::cout << "	  	  policy description: " << dpm_policy_info.policies[j].policy_description << std::endl;
			}
			dpm_policy_default = dpm_policy_info.cur;
			ret = amdsmi_set_soc_pstate(processors[i], 0);
			EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS);
			ret = amdsmi_get_soc_pstate(processors[i], &dpm_policy_info);
			EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS);
			EXPECT_TRUE(dpm_policy_info.cur == 0);
			ret = amdsmi_set_soc_pstate(processors[i], 1);
			EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS);
			ret = amdsmi_get_soc_pstate(processors[i], &dpm_policy_info);
			EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS);
			EXPECT_TRUE(dpm_policy_info.cur == 1);
			ret = amdsmi_set_soc_pstate(processors[i], 2);
			EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS);
			ret = amdsmi_get_soc_pstate(processors[i], &dpm_policy_info);
			EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS);
			EXPECT_TRUE(dpm_policy_info.cur == 2);
			ret = amdsmi_set_soc_pstate(processors[i], 3);
			EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS);
			ret = amdsmi_get_soc_pstate(processors[i], &dpm_policy_info);
			EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS);
			EXPECT_TRUE(dpm_policy_info.cur == 3);
			ret = amdsmi_set_soc_pstate(processors[i], dpm_policy_default);
			EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS);
			ret = amdsmi_get_soc_pstate(processors[i], &dpm_policy_info);
			EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS);
			EXPECT_TRUE(dpm_policy_info.cur == dpm_policy_default);
		}

		std::string clk_deep_sleep_str = "N/A";
		std::string clk_locked_str = "N/A";
		ret = amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_MEM,
						    &clock_measure);
		if(clock_measure.clk_deep_sleep != (uint8_t)-1) {
			clk_deep_sleep_str = clock_measure.clk_deep_sleep ? "true" : "false";
		}

		EXPECT_TRUE((ret == AMDSMI_STATUS_SUCCESS) || (ret == AMDSMI_STATUS_NOT_SUPPORTED));
		if (ret == AMDSMI_STATUS_SUCCESS) {
			EXPECT_TRUE(clock_measure.max_clk <= 3000);
			EXPECT_TRUE(clock_measure.clk <= clock_measure.max_clk);

			std::cout << "memory clock measure: " << std::endl;
			std::cout << "    cur clk: " << clock_measure.clk << std::endl;
			std::cout << "    max clk: " << clock_measure.max_clk << std::endl;
			std::cout << "    min clk: " << clock_measure.min_clk << std::endl;
			std::cout << "    mem clk deep sleep: " << clk_deep_sleep_str.c_str() << std::endl;
		}

		ret = amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_GFX,
						    &clock_measure);
		if(clock_measure.clk_deep_sleep != (uint8_t)-1) {
			clk_deep_sleep_str = clock_measure.clk_deep_sleep ? "true" : "false";
		} else {
			clk_deep_sleep_str = "N/A";
		}
		if(clock_measure.clk_locked != (uint8_t)-1) {
			clk_locked_str = clock_measure.clk_locked ? "true" : "false";
		} else {
			clk_locked_str = "N/A";
		}

		EXPECT_TRUE((ret == AMDSMI_STATUS_SUCCESS) || (ret == AMDSMI_STATUS_NOT_SUPPORTED));
		if (ret == AMDSMI_STATUS_SUCCESS) {
			EXPECT_TRUE(clock_measure.max_clk <= 3000);

			std::cout << "gfx clock measure: " << std::endl;
			std::cout << "    cur clk: " << clock_measure.clk << std::endl;
			std::cout << "    max clk: " << clock_measure.max_clk << std::endl;
			std::cout << "    min clk: " << clock_measure.min_clk << std::endl;
			std::cout << "    clk locked: " << clk_locked_str.c_str() << std::endl;
			std::cout << "    gfx clk deep sleep: " << clk_deep_sleep_str.c_str() << std::endl;
		}

		ret = amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_VCLK0,
						&clock_measure);
		if(clock_measure.clk_deep_sleep != (uint8_t)-1) {
			clk_deep_sleep_str = clock_measure.clk_deep_sleep ? "true" : "false";
		} else {
			clk_deep_sleep_str = "N/A";
		}
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		std::cout << "vclk0 clock measure: " << std::endl;

		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "    cur vclk0 clk: " << clock_measure.clk << std::endl;
			std::cout << "    max vclk0 clk: " << clock_measure.max_clk << std::endl;
			std::cout << "    min vclk0 clk: " << clock_measure.min_clk << std::endl;
			std::cout << "    vclk0 clk deep sleep: " << clk_deep_sleep_str.c_str() << std::endl;
		} else {
			std::cout << "    N/A" << std::endl;
		}

		ret = amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_VCLK1,
						&clock_measure);
		if(clock_measure.clk_deep_sleep != (uint8_t)-1) {
			clk_deep_sleep_str = clock_measure.clk_deep_sleep ? "true" : "false";
		} else {
			clk_deep_sleep_str = "N/A";
		}
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		std::cout << "vclk1 clock measure: " << std::endl;
		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "    cur vclk1 clk: " << clock_measure.clk << std::endl;
			std::cout << "    max vclk1 clk: " << clock_measure.max_clk << std::endl;
			std::cout << "    min vclk1 clk: " << clock_measure.min_clk << std::endl;
			std::cout << "    vclk1 clk deep sleep: " << clk_deep_sleep_str.c_str() << std::endl;
		} else {
			std::cout << "    N/A" << std::endl;
		}

		ret = amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_DCLK0,
						&clock_measure);
		if(clock_measure.clk_deep_sleep != (uint8_t)-1) {
			clk_deep_sleep_str = clock_measure.clk_deep_sleep ? "true" : "false";
		} else {
			clk_deep_sleep_str = "N/A";
		}
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		std::cout << "dclk0 clock measure: " << std::endl;

		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "    cur dclk0 clk: " << clock_measure.clk << std::endl;
			std::cout << "    max dclk0 clk: " << clock_measure.max_clk << std::endl;
			std::cout << "    min dclk0 clk: " << clock_measure.min_clk << std::endl;
			std::cout << "    dclk0 clk deep sleep: " << clk_deep_sleep_str.c_str() << std::endl;
		} else {
			std::cout << "    N/A" << std::endl;
		}

		ret = amdsmi_get_clock_info(processors[i], AMDSMI_CLK_TYPE_DCLK1,
						&clock_measure);
		if(clock_measure.clk_deep_sleep != (uint8_t)-1) {
			clk_deep_sleep_str = clock_measure.clk_deep_sleep ? "true" : "false";
		} else {
			clk_deep_sleep_str = "N/A";
		}
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		std::cout << "dclk1 clock measure: " << std::endl;
		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "    cur dclk1 clk: " << clock_measure.clk << std::endl;
			std::cout << "    max dclk1 clk: " << clock_measure.max_clk << std::endl;
			std::cout << "    min dclk1 clk: " << clock_measure.min_clk << std::endl;
			std::cout << "    dclk1 clk deep sleep: " << clk_deep_sleep_str.c_str() << std::endl;
		} else {
			std::cout << "    N/A" << std::endl;
		}

		ret = amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_VRAM,
						      AMDSMI_TEMP_CURRENT, &temp_measure);
		ASSERT_TRUE(ret ==(unsigned int) AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);

		int ret = amdsmi_get_gpu_vram_info(processors[i], &vram_info);
		EXPECT_TRUE((ret == AMDSMI_STATUS_SUCCESS) || (ret == AMDSMI_STATUS_NOT_SUPPORTED));

		if (ret == AMDSMI_STATUS_SUCCESS) {
			EXPECT_TRUE(temp_measure <= 105);
			std::cout << "memory temperature: " << temp_measure << std::endl;
		}

		ret = amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
						      AMDSMI_TEMP_CURRENT, &temp_measure);
		ASSERT_TRUE(ret ==(unsigned int) AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			EXPECT_TRUE(temp_measure <= 108);
			std::cout << "hotspot temperature: " << temp_measure << std::endl;
		}

		ret = amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_EDGE,
						      AMDSMI_TEMP_CURRENT, &temp_measure);
		ASSERT_TRUE(ret ==(unsigned int) AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			EXPECT_TRUE(temp_measure <= 100 || std::numeric_limits<uint32_t>::max());
			std::cout << "edge temperature: " << temp_measure << std::endl;
		}

		ret = amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_VRAM,
						      AMDSMI_TEMP_CRITICAL, &temp_measure);
		ASSERT_TRUE(ret ==(unsigned int) AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		std::cout << "memory temperature limit: " << temp_measure << std::endl;

		ret = amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
						      AMDSMI_TEMP_CRITICAL, &temp_measure);
		ASSERT_TRUE(ret ==(unsigned int) AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		std::cout << "hotspot temperature limit: " << temp_measure << std::endl;

		ret = amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_EDGE,
						      AMDSMI_TEMP_CRITICAL, &temp_measure);
		ASSERT_TRUE(ret ==(unsigned int) AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		std::cout << "edge temperature limit: " << temp_measure << std::endl;

		ret = amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_VRAM,
						      AMDSMI_TEMP_SHUTDOWN, &temp_measure);
		ASSERT_TRUE(ret ==(unsigned int) AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		std::cout << "memory temperature shutdown: " << temp_measure << std::endl;

		ret = amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
						      AMDSMI_TEMP_SHUTDOWN, &temp_measure);
		ASSERT_TRUE(ret ==(unsigned int) AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		std::cout << "hotspot temperature shutdown: " << temp_measure << std::endl;

		ret = amdsmi_get_temp_metric(processors[i], AMDSMI_TEMPERATURE_TYPE_EDGE,
						      AMDSMI_TEMP_SHUTDOWN, &temp_measure);
		ASSERT_TRUE(ret ==(unsigned int) AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		std::cout << "edge temperature shutdown: " << temp_measure << std::endl;
	}
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	free(processors);
}

TEST(amdsmiIntegrationTests, PowerCapTests)
{
	amdsmi_power_cap_info_t changed_power_cap;
	amdsmi_power_cap_info_t default_power_cap;
	uint64_t power_cap = 60;
	uint32_t sensor_ind = 0;
	int ret;

	unsigned int dev_cnt = AMDSMI_MAX_DEVICES;

	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(NULL, &dev_cnt, processors), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, (unsigned)AMDSMI_MAX_DEVICES);

	for (unsigned int i = 0; i < dev_cnt; ++i) {
		ASSERT_EQ(amdsmi_get_power_cap_info(processors[i], sensor_ind, &default_power_cap), AMDSMI_STATUS_SUCCESS);
		ret = amdsmi_set_power_cap(processors[i], sensor_ind, power_cap);
		EXPECT_TRUE((ret == AMDSMI_STATUS_SUCCESS) || (ret == AMDSMI_STATUS_NOT_SUPPORTED));
		if (ret == AMDSMI_STATUS_SUCCESS) {
			ASSERT_EQ(amdsmi_get_power_cap_info(processors[i], sensor_ind, &changed_power_cap), AMDSMI_STATUS_SUCCESS);
			EXPECT_EQ(changed_power_cap.power_cap, power_cap);
#ifdef _WIN64
			printf(" GPU power cap is set on: %lldW\n", changed_power_cap.power_cap);
#else
			printf(" GPU power cap is set on: %ldW\n", changed_power_cap.power_cap);
#endif
			ASSERT_EQ(amdsmi_set_power_cap(processors[i], sensor_ind, default_power_cap.power_cap), AMDSMI_STATUS_SUCCESS);
		}
	}

	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	free(processors);
}

TEST(amdsmiIntegrationTests, ECCTests)
{
	uint32_t dev_cnt = AMDSMI_MAX_DEVICES;

	amdsmi_error_count_t ec = {0};
	amdsmi_ras_feature_t ras_feature;
	uint64_t enabled_blocks = 0;
	amdsmi_asic_info_t asic_info;
	int ret;

	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(NULL, &dev_cnt, processors), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, (unsigned)AMDSMI_MAX_DEVICES);

	for (uint32_t i = 0; i < dev_cnt; ++i) {
		std::cout << "GPU " << i << '\n';

		ASSERT_EQ(amdsmi_get_gpu_ecc_enabled(processors[i], &enabled_blocks),
			  AMDSMI_STATUS_SUCCESS);

		std::cout << "Enabled GPU blocks:      " << enabled_blocks << std::endl;

		ASSERT_EQ(amdsmi_get_gpu_asic_info(processors[i], &asic_info), AMDSMI_STATUS_SUCCESS);
		if (asic_info.device_id == 29857) {
			ASSERT_EQ(amdsmi_get_gpu_ras_feature_info(processors[i], &ras_feature),
			  	AMDSMI_STATUS_SUCCESS);
			std::cout << "RAS EEPROM version:      " << ras_feature.ras_eeprom_version << std::endl;
			std::cout << "ECC correction schema:      " << ras_feature.supported_ecc_correction_schema << std::endl;
		}

		ret = amdsmi_get_gpu_total_ecc_count(processors[i], &ec);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "ECC:"
			  << "\n\t Correctable errors: " << ec.correctable_count
			  << "\n\t Uncorrectable errors: " << ec.uncorrectable_count
			  << "\n\t Deferred errors: " << ec.deferred_count << std::endl;

			EXPECT_GE(ec.uncorrectable_count, 0);
			EXPECT_GE(ec.correctable_count, 0);
			EXPECT_GE(ec.deferred_count, 0);
		}

		ret = amdsmi_get_gpu_total_ecc_count(processors[i], &ec);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "ECC:"
			  << "\n\t Correctable errors: " << ec.correctable_count
			  << "\n\t Uncorrectable errors: " << ec.uncorrectable_count
			  << "\n\t Deferred errors: " << ec.deferred_count << std::endl;

			EXPECT_GE(ec.uncorrectable_count, 0);
			EXPECT_GE(ec.correctable_count, 0);
			EXPECT_GE(ec.deferred_count, 0);
		}

		ret = amdsmi_get_gpu_total_ecc_count(processors[i], &ec);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "ECC:"
			  << "\n\t Correctable errors: " << ec.correctable_count
			  << "\n\t Uncorrectable errors: " << ec.uncorrectable_count
			  << "\n\t Deferred errors: " << ec.deferred_count << std::endl;
			EXPECT_GE(ec.uncorrectable_count, 0);
			EXPECT_GE(ec.correctable_count, 0);
			EXPECT_GE(ec.deferred_count, 0);
		}
	}

	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	free(processors);
}

TEST(amdsmiIntegrationTests, DISABLED_ECCTestsPerBlock)
{
	uint32_t dev_cnt = AMDSMI_MAX_DEVICES;

	amdsmi_error_count_t ec = {0};
	amdsmi_gpu_block_t block = AMDSMI_GPU_BLOCK_FIRST;
	uint64_t enabled_blocks = 0;
	uint32_t j = 0;
	bool all_blocks_not_supported = true;
	int ret = 0;

	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(NULL, &dev_cnt, processors), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, (unsigned)AMDSMI_MAX_DEVICES);

	for (uint32_t i = 0; i < dev_cnt; ++i) {
		all_blocks_not_supported = true;
		block = AMDSMI_GPU_BLOCK_FIRST;
		std::cout << "GPU " << i << '\n';

		ASSERT_EQ(amdsmi_get_gpu_ecc_enabled(processors[i], &enabled_blocks),
			AMDSMI_STATUS_SUCCESS);

		std::cout << "Enabled GPU blocks:      " << enabled_blocks << std::endl;

		j = 0;
		while (block <= AMDSMI_GPU_BLOCK_LAST) {
			block = (amdsmi_gpu_block_t)pow(2, j);
			ret = amdsmi_get_gpu_ecc_count(processors[i], block, &ec);
			ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_API_FAILED || ret == AMDSMI_STATUS_NOT_SUPPORTED);
			if (ret == AMDSMI_STATUS_SUCCESS) {
				std::cout << "ECC:"
					<< "\n\t Correctable errors: " << ec.correctable_count
					<< "\n\t Uncorrectable errors: " << ec.uncorrectable_count
					<< "\n\t Deferred errors: " << ec.deferred_count << std::endl;
				EXPECT_GE(ec.uncorrectable_count, 0);
				EXPECT_GE(ec.correctable_count, 0);
				EXPECT_GE(ec.deferred_count, 0);
				all_blocks_not_supported = false;
			}
			j++;
		}

		ASSERT_EQ(all_blocks_not_supported, false);
	}

	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	free(processors);
}

TEST(amdsmiIntegrationTests, BadPageInfoTest)
{
	uint32_t dev_cnt = 0;
	amdsmi_socket_handle socket_handle = NULL;
	amdsmi_processor_handle *processors = NULL;
	amdsmi_eeprom_table_record_t *eeprom_table_records;
	uint32_t bad_page_count = AMDSMI_MAX_BAD_PAGE_RECORD;
	struct tm dtime;
	int ret = 0;

	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	ASSERT_EQ(amdsmi_get_processor_handles(socket_handle, &dev_cnt, NULL), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, AMDSMI_MAX_DEVICES);

	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(socket_handle, &dev_cnt, &processors[0]), AMDSMI_STATUS_SUCCESS);

	for (uint32_t i = 0; i < dev_cnt; i++) {
		bad_page_count = AMDSMI_MAX_BAD_PAGE_RECORD;
		ret = amdsmi_get_gpu_bad_page_info(processors[i], &bad_page_count, NULL);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NO_DATA);
		if (bad_page_count != 0) {
			eeprom_table_records = (amdsmi_eeprom_table_record_t *)malloc(sizeof(amdsmi_eeprom_table_record_t) * bad_page_count);
			ASSERT_EQ(amdsmi_get_gpu_bad_page_info(processors[i], &bad_page_count, eeprom_table_records), AMDSMI_STATUS_SUCCESS);
			printf("Bad page count: %u\n", bad_page_count);
			for (uint32_t j = 0; j < bad_page_count; ++j) {
				printf("Page %u\n", j);
				printf("Retired page address: 0x%" PRIx64 "\n", eeprom_table_records[j].retired_page);
				time_t rawtime = eeprom_table_records[j].ts;
#ifdef _WIN64
				if(localtime_s(&dtime, &rawtime) != 0)
				{
					printf("Retired page timestamp: N/A\n");
				}
				else {
					printf("Retired page timestamp: %d/%d/%d:%d/%d/%d\n", dtime.tm_year + 1900, dtime.tm_mon + 1, dtime.tm_mday,
											dtime.tm_hour, dtime.tm_min, dtime.tm_sec);
				}
#else
				if(localtime_r(&rawtime, &dtime) != 0)
				{
					printf("Retired page timestamp: N/A\n");
				}
				else {
					printf("Retired page timestamp: %d/%d/%d:%d/%d/%d\n", dtime.tm_year + 1900, dtime.tm_mon + 1, dtime.tm_mday,
											dtime.tm_hour, dtime.tm_min, dtime.tm_sec);
				}
#endif
				printf("Retired page error type: %u\n", eeprom_table_records[j].err_type);
				printf("Retired page mem channel: %u\n", eeprom_table_records[j].mem_channel);
				printf("Retired page mcumc id: %u\n", eeprom_table_records[j].mcumc_id);
			}
			free(eeprom_table_records);
		}
	}

	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	free(processors);
}

static void dfc_fw_table_test()
{
	uint32_t dev_cnt = AMDSMI_MAX_DEVICES;
	amdsmi_dfc_fw_t dfc_fw;

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(NULL, &dev_cnt, processors), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, (unsigned)AMDSMI_MAX_DEVICES);

	for (uint32_t i = 0; i < dev_cnt; i++) {
#ifdef _WIN64
		ASSERT_EQ(amdsmi_get_dfc_fw_table(processors[i], &dfc_fw), AMDSMI_STATUS_SUCCESS);
		printf("DFC FW Table:\n");
		printf("   DFC Version: %u\n", dfc_fw.header.dfc_fw_version);
		printf("   Num DFC Entries: %u\n", dfc_fw.header.dfc_fw_total_entries);
		printf("   DFC gart wr guest min: %u\n", dfc_fw.header.dfc_gart_wr_guest_min);
		printf("   DFC gart wr guest max: %u\n", dfc_fw.header.dfc_gart_wr_guest_max);
		for (uint32_t j = 0; j < dfc_fw.header.dfc_fw_total_entries; j++) {
			printf("entry %u:\n", j+1);
			printf("dfc fw type: %u\n",dfc_fw.data[j].dfc_fw_type);
			printf("verification enabled: %s\n",dfc_fw.data[j].verification_enabled ? "True" : "False");
			printf("customer ordinal: %u\n",dfc_fw.data[j].customer_ordinal);
			for (uint32_t k = 0; k < 16; k++) {
				if (dfc_fw.data[j].white_list[k].latest != 0) {
					printf("white list[%u] latest: %u\n", k, dfc_fw.data[j].white_list[k].latest);
				}
				if (dfc_fw.data[j].white_list[k].oldest != 0) {
					printf("white list[%u] oldest: %u\n", k, dfc_fw.data[j].white_list[k].oldest);
				}
			}
			for (uint32_t k = 0; k < 64; k++) {
				if (dfc_fw.data[j].black_list[k] != 0) {
					printf("black list[%u]: %u\n", k, dfc_fw.data[j].black_list[k]);
				}
			}
		}
#else
		ASSERT_EQ(amdsmi_get_dfc_fw_table(processors[i], &dfc_fw), AMDSMI_STATUS_NOT_SUPPORTED);
#endif

	}

	free(processors);
}

TEST(amdsmiIntegrationTests, DFCTableTest)
{
	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);
	dfc_fw_table_test();
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
}

static void firmware_attestation_test()
{
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;
	uint32_t dev_cnt = AMDSMI_MAX_DEVICES;
	amdsmi_fw_error_record_t records;
	amdsmi_fw_info_t fw_info;
	amdsmi_partition_info_t partitions[AMDSMI_MAX_VF_COUNT];
	amdsmi_vf_data_t vf_data;
	uint32_t num_vf_enabled;
	uint32_t num_vf_supported;

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(NULL, &dev_cnt, processors), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, (unsigned)AMDSMI_MAX_DEVICES);

	for (uint32_t i = 0; i < dev_cnt; i++) {
		ASSERT_EQ(amdsmi_get_num_vf(processors[i], &num_vf_enabled, &num_vf_supported),
			  AMDSMI_STATUS_SUCCESS);
		ret = amdsmi_set_num_vf(processors[i], num_vf_enabled);
		EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS
					|| ret == AMDSMI_STATUS_BUSY);
		ASSERT_EQ(amdsmi_get_vf_partition_info(processors[i], num_vf_enabled,
			  partitions), AMDSMI_STATUS_SUCCESS);

		for (uint32_t j = 0; j < num_vf_enabled; j++) {
			ASSERT_EQ(amdsmi_get_vf_data(partitions[j].id, &vf_data),
				  AMDSMI_STATUS_SUCCESS);

			ret = amdsmi_get_vf_fw_info(partitions[j].id, &fw_info);

			if (vf_data.sched.state == AMDSMI_VF_STATE_ACTIVE) {
				// If VF is active we expect it has FW info
				EXPECT_EQ(ret, AMDSMI_STATUS_SUCCESS);
			} else {
				// If VF is in another state, we cannot be sure
				EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS
					|| ret == AMDSMI_STATUS_NOT_SUPPORTED
					|| ret == AMDSMI_STATUS_API_FAILED);
			}

			if (ret == AMDSMI_STATUS_SUCCESS) {
				EXPECT_LE(fw_info.num_fw_info, AMDSMI_FW_ID__MAX - 1);

				printf("VF firmware info:\n");
				printf("   Firmware count: %d\n", fw_info.num_fw_info);
				for (uint32_t j = 0; j < fw_info.num_fw_info; ++j) {
					if (fw_info.fw_info_list[j].fw_version != 0) {
						printf("    Firmware ID: 0x%x\n",
						fw_info.fw_info_list[j].fw_id);
						printf("    Firmware version: 0x%" PRIx64 "\n",
						fw_info.fw_info_list[j].fw_version);
					}
				}
			}
		}

		ret = amdsmi_get_fw_error_records(processors[i], &records);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		if (ret == AMDSMI_STATUS_SUCCESS) {
			printf("Firmware error records:\n");
			printf("   Number of firmware error records: %d\n", records.num_err_records);
			for (uint8_t j = 0; j < records.num_err_records; j++) {
				printf("timestamp: %" PRIu64 "\n", records.err_records[j].timestamp);
				printf("vf idx: %u\n", records.err_records[j].vf_idx);
				printf("fw id: %u\n", records.err_records[j].fw_id);
				printf("status: %u\n", records.err_records[j].status);
			}
		}
	}

	free(processors);
}

TEST(amdsmiIntegrationTests, FirmwareAttestationTest)
{
	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);
	firmware_attestation_test();
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
}

TEST(amdsmiIntegrationTests, Vf2PfTest)
{
	uint32_t dev_cnt = 0;
	amdsmi_socket_handle socket_handle = NULL;
	amdsmi_processor_handle *processors = NULL;
	uint32_t num_vf_enabled;
	uint32_t num_vf_supported;
	amdsmi_guest_data_t guest_data;
	amdsmi_partition_info_t partitions[AMDSMI_MAX_VF_COUNT];
	amdsmi_vf_data_t vf_data;

	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	ASSERT_EQ(amdsmi_get_processor_handles(socket_handle, &dev_cnt, NULL), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, AMDSMI_MAX_DEVICES);

	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(socket_handle, &dev_cnt, &processors[0]), AMDSMI_STATUS_SUCCESS);

	for (uint32_t i = 0; i < dev_cnt; i++) {
		ASSERT_EQ(amdsmi_get_num_vf(processors[i], &num_vf_enabled, &num_vf_supported),
			  AMDSMI_STATUS_SUCCESS);
		int ret = amdsmi_set_num_vf(processors[i], num_vf_enabled);
		EXPECT_TRUE(ret == AMDSMI_STATUS_SUCCESS
					|| ret == AMDSMI_STATUS_BUSY);
		ASSERT_EQ(amdsmi_get_vf_partition_info(processors[i], num_vf_enabled,
			  partitions), AMDSMI_STATUS_SUCCESS);

		for (uint32_t j = 0; j < num_vf_enabled; j++) {
			ASSERT_EQ(amdsmi_get_vf_data(partitions[j].id, &vf_data),
				  AMDSMI_STATUS_SUCCESS);

			if (vf_data.sched.state == AMDSMI_VF_STATE_ACTIVE ||
				vf_data.sched.state == AMDSMI_VF_STATE_DEFAULT_AVAILABLE) {
				ASSERT_EQ(amdsmi_get_guest_data(partitions[j].id,
					  &guest_data), AMDSMI_STATUS_SUCCESS);

				printf("Guest data:\n");
				printf("    driver_version: %s\n", guest_data.driver_version);
				EXPECT_TRUE(guest_data.fb_usage <= partitions[j].fb.fb_size);
				printf("    fb_usage: %d\n", guest_data.fb_usage);
				printf("    Guest microcode info:\n");
			}
			if (vf_data.sched.state == AMDSMI_VF_STATE_UNAVAILABLE) {
				ASSERT_EQ(amdsmi_get_guest_data(partitions[j].id,
					  &guest_data), AMDSMI_STATUS_API_FAILED);
			}
		}
	}

	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	free(processors);
}

TEST(amdsmiIntegrationTests, DISABLED_EventTimeoutTests)
{
	uint32_t dev_cnt = 0;
	amdsmi_processor_handle *processors = NULL;
	amdsmi_socket_handle socket_handle = NULL;
	amdsmi_event_set set;
	amdsmi_event_entry_t amdsmi_event;
	int ret;
	uint64_t events = AMDSMI_MASK_INIT;
	events = AMDSMI_MASK_INCLUDE_INFO_SEVERITY(events);
	events = AMDSMI_MASK_INCLUDE_CATEGORY(events, AMDSMI_EVENT_CATEGORY_RESET);

	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	ASSERT_EQ(amdsmi_get_processor_handles(socket_handle, &dev_cnt, NULL), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, AMDSMI_MAX_DEVICES);

	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(socket_handle, &dev_cnt, &processors[0]), AMDSMI_STATUS_SUCCESS);

	ret = amdsmi_event_create(&processors[0], 1, events, &set);
	if (ret != AMDSMI_STATUS_NOT_SUPPORTED) {
		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
		EXPECT_EQ(amdsmi_event_read(set, 1 * 1000 * 1000, &amdsmi_event), AMDSMI_STATUS_TIMEOUT);
		ASSERT_EQ(amdsmi_event_destroy(set), AMDSMI_STATUS_SUCCESS);
	}

	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	free(processors);
}

TEST(amdsmiIntegrationTests, DISABLED_XgmiTest)
{
	uint32_t dev_cnt = 0;
	amdsmi_socket_handle socket_handle = NULL;
	amdsmi_processor_handle *processors = NULL;
	amdsmi_link_metrics_t link_metrics;
	amdsmi_link_topology_t topology_info;
	amdsmi_xgmi_fb_sharing_caps_t caps;
	uint8_t fb_sharing;
	int ret = 0;

	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	ASSERT_EQ(amdsmi_get_processor_handles(socket_handle, &dev_cnt, NULL), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, AMDSMI_MAX_DEVICES);

	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(socket_handle, &dev_cnt, &processors[0]), AMDSMI_STATUS_SUCCESS);

	for (uint32_t i = 0; i < dev_cnt; i++) {
		ASSERT_EQ(amdsmi_get_link_metrics(processors[i], &link_metrics), AMDSMI_STATUS_SUCCESS);
		printf("XGMI METRICS:\n");
		printf("[GPU%d]:\n", i);
		printf("	Num links: %u\n", link_metrics.num_links);
		for (uint32_t k = 0; k < link_metrics.num_links; k++) {
			printf("	Speed: %u\n", link_metrics.links[k].bit_rate);
			printf("	Max bandwidth: %u\n", link_metrics.links[k].max_bandwidth);
			printf("	Type: %d\n", link_metrics.links[k].link_type);
			printf("	Read: %" PRIu64 "\n", link_metrics.links[k].read);
			printf("	Write: %" PRIu64 "\n", link_metrics.links[k].write);
		}
		ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_caps(processors[i], &caps), AMDSMI_STATUS_SUCCESS);
		printf("XGMI CAPS:\n");
		printf("XGMI CUSTOM MODE: %s\n", caps.cap.mode_custom_cap ? "Supported" : "Unsupported");
		printf("XGMI MODE 1: %s\n",  caps.cap.mode_1_cap ? "Supported" : "Unsupported");
		printf("XGMI MODE 2: %s\n",  caps.cap.mode_2_cap ? "Supported" : "Unsupported");
		printf("XGMI MODE 4: %s\n",  caps.cap.mode_4_cap ? "Supported" : "Unsupported");
		printf("XGMI MODE 8: %s\n",  caps.cap.mode_8_cap ? "Supported" : "Unsupported");
		for (uint32_t j = 0; j < dev_cnt; j++) {
			ASSERT_EQ(amdsmi_get_link_topology(processors[i], processors[j], &topology_info), AMDSMI_STATUS_SUCCESS);
			printf("[GPU%d, GPU%d] XGMI TOPOLOGY:\n", i, j);
			printf("	Weight: %" PRIu64 "\n", topology_info.weight);
			printf("	Status: %d\n", topology_info.link_status);
			printf("	Type: %d\n", topology_info.link_type);
			printf("	Num hops: %d\n", topology_info.num_hops);
			printf("	Is fb sharing enabled: %d\n", topology_info.fb_sharing);
			ret = amdsmi_get_xgmi_fb_sharing_mode_info(processors[i], processors[j], AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM, &fb_sharing);
			ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
			ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_mode_info(processors[i], processors[j], AMDSMI_XGMI_FB_SHARING_MODE_1, &fb_sharing), AMDSMI_STATUS_SUCCESS);
			printf("[GPU%d, GPU%d] XGMI FB SHARING MODE 1:\n", i, j);
			printf("	Is fb sharing enabled: %d\n", fb_sharing);
			ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_mode_info(processors[i], processors[j], AMDSMI_XGMI_FB_SHARING_MODE_2, &fb_sharing), AMDSMI_STATUS_SUCCESS);
			printf("[GPU%d, GPU%d] XGMI FB SHARING MODE 2:\n", i, j);
			printf("	Is fb sharing enabled: %d\n", fb_sharing);
			ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_mode_info(processors[i], processors[j], AMDSMI_XGMI_FB_SHARING_MODE_4, &fb_sharing), AMDSMI_STATUS_SUCCESS);
			printf("[GPU%d, GPU%d] XGMI FB SHARING MODE 4:\n", i, j);
			printf("	Is fb sharing enabled: %d\n", fb_sharing);
			ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_mode_info(processors[i], processors[j], AMDSMI_XGMI_FB_SHARING_MODE_8, &fb_sharing), AMDSMI_STATUS_SUCCESS);
			printf("[GPU%d, GPU%d] XGMI FB SHARING MODE 8:\n", i, j);
			printf("	Is fb sharing enabled: %d\n", fb_sharing);
		}
	}

	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	free(processors);
}

TEST(amdsmiIntegrationTests, ChipletMetricTest)
{
	uint32_t dev_cnt = 0;
	amdsmi_socket_handle socket_handle = NULL;
	amdsmi_processor_handle *processors = NULL;
	amdsmi_metric_t *metrics;
	uint32_t size = AMDSMI_MAX_NUM_METRICS;
	int ret = AMDSMI_STATUS_SUCCESS;

	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	ASSERT_EQ(amdsmi_get_processor_handles(socket_handle, &dev_cnt, NULL), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, AMDSMI_MAX_DEVICES);

	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(socket_handle, &dev_cnt, &processors[0]), AMDSMI_STATUS_SUCCESS);

	for (uint32_t i = 0; i < dev_cnt; i++) {
		ret = amdsmi_get_gpu_metrics(processors[i], &size, NULL);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		if (!ret) {
			metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t) * size);
			ASSERT_EQ(amdsmi_get_gpu_metrics(processors[i], &size, metrics), AMDSMI_STATUS_SUCCESS);
			free(metrics);
		}
	}

	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	free(processors);
}

static void partition_tests()
{
	unsigned int dev_cnt = AMDSMI_MAX_DEVICES;
	amdsmi_processor_handle *processors = NULL;
	amdsmi_accelerator_partition_profile_t profile;
	uint32_t partition_id[AMDSMI_MAX_ACCELERATOR_PROFILE];
	amdsmi_memory_partition_config_t memory_setting;
	amdsmi_accelerator_partition_profile_config_t profile_configs;
	unsigned int i = 0;
	int ret = 0;

	memset(&profile, 0, sizeof(profile));
	memset(&memory_setting, 0, sizeof(memory_setting));
	memset(&profile_configs, 0, sizeof(profile_configs));

	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(NULL, &dev_cnt, processors), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, (unsigned)AMDSMI_MAX_DEVICES);

	for (i = 0; i < dev_cnt; i++) {
		ret = amdsmi_get_gpu_accelerator_partition_profile_config(processors[i], &profile_configs);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		printf("   Number of resource profiles: %d\n", profile_configs.num_resource_profiles);
		printf("   Number of profiles: %d\n", profile_configs.num_profiles);
		printf("   Default profile index: %d\n", profile_configs.default_profile_index);
		for (uint32_t j = 0; j < profile_configs.num_resource_profiles; j++) {
			printf("   Index of a profile: %d\n", profile_configs.resource_profiles[j].profile_index);
			printf("   Resource type: %d\n", profile_configs.resource_profiles[j].resource_type);
			printf("   Resource in a partition: %d\n", profile_configs.resource_profiles[j].partition_resource);
			printf("   Number of partitions share resource: %d\n", profile_configs.resource_profiles[j].num_partitions_share_resource);
		}
		for (uint32_t j = 0; j < profile_configs.num_profiles; j++) {
			printf("   Profile type: %d\n", profile_configs.profiles[j].profile_type);
			printf("   Number of partitions: %d\n", profile_configs.profiles[j].num_partitions);
			printf("   Number of partitions: %d\n", profile_configs.profiles[j].memory_caps.nps_cap_mask);
			printf("   Index of profile: %d\n", profile_configs.profiles[j].profile_index);
			printf("   Number of resources: %d\n", profile_configs.profiles[j].num_resources);
			for (uint32_t k = 0; k < profile_configs.profiles[j].num_partitions; k++) {
				for (uint32_t m = 0; m < profile_configs.profiles[j].num_resources; m++) {
					printf("   Index of resource profile: %d\n", profile_configs.profiles[j].resources[k][m]);
				}
			}
		}

		ret = amdsmi_get_gpu_accelerator_partition_profile(processors[i], &profile, partition_id);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		printf("   Profile type: %d\n", profile.profile_type);
		printf("   Number of partitions: %d\n", profile.num_partitions);
		printf("   Memory capability: %d\n", profile.memory_caps.nps_cap_mask);
		printf("   Index of profile: %d\n", profile.profile_index);
		printf("   Number of resources: %d\n", profile.num_resources);
		for (uint32_t j = 0; j < profile.num_partitions; j++) {
			for (uint32_t k = 0; k < profile.num_resources; k++) {
				printf("   Index of resource profile: %d\n", profile.resources[j][k]);
			}
		}

		for (uint32_t j = 0; j < profile.num_partitions; j++) {
				printf("   Partition id%d: %d\n", i, partition_id[j]);
		}

		ret = amdsmi_set_gpu_accelerator_partition_profile(processors[i], profile.profile_index);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_API_FAILED || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		printf(" Set accelerator partition to profile index: %d\n", profile.profile_index);

		ret = amdsmi_get_gpu_memory_partition_config(processors[i], &memory_setting);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_NOT_SUPPORTED);
		printf("   NPS caps: %d\n", memory_setting.partition_caps.nps_cap_mask);
		printf("   Memory partition mode: %d\n", memory_setting.mp_mode);
		printf("   Number of numa ranges: %d\n", memory_setting.num_numa_ranges);
		for (uint32_t j = 0; j < memory_setting.num_numa_ranges; j++) {
			printf("   Memory type: %d\n", memory_setting.numa_range[j].memory_type);
			printf("   Numa range start: %" PRIu64 "\n", memory_setting.numa_range[j].start);
			printf("   Numa range end: %" PRIu64 "\n", memory_setting.numa_range[j].end);
		}

		ret = amdsmi_set_gpu_memory_partition_mode(processors[i], memory_setting.mp_mode);
		ASSERT_TRUE(ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_API_FAILED || ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_INVAL);
		printf(" Set memory partition mode: %d\n", memory_setting.mp_mode);

	}
	free(processors);
}

TEST(amdsmiIntegrationTests, PartitionTests)
{
	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);
	partition_tests();
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
}

TEST(amdsmiIntegrationTests, DISABLED_CperTests)
{
	uint32_t dev_cnt = AMDSMI_MAX_DEVICES;
	amdsmi_processor_handle *processors = NULL;
	int ret = 0;

	char cper_data[1024*1024];
	uint64_t buf_size = sizeof(cper_data);
	amdsmi_cper_hdr *cper_hdrs[1024];
	uint64_t entry_count = 1024;
	uint64_t cursor = 0;
	uint32_t severity_mask = 3;

	processors =  (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(amdsmi_get_processor_handles(NULL, &dev_cnt, processors), AMDSMI_STATUS_SUCCESS);
	ASSERT_GE(dev_cnt, (unsigned)1);
	ASSERT_LE(dev_cnt, (unsigned)AMDSMI_MAX_DEVICES);

	do {
		for (uint32_t i = 0; i < dev_cnt; i++) {
			ret = amdsmi_gpu_get_cper_entries(processors[i], severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor);
			EXPECT_TRUE((ret == AMDSMI_STATUS_SUCCESS) || (ret == AMDSMI_STATUS_NOT_SUPPORTED) || (ret == AMDSMI_STATUS_MORE_DATA));
			if (ret == AMDSMI_STATUS_SUCCESS || ret == AMDSMI_STATUS_MORE_DATA) {
				for (uint64_t j = 0; j < entry_count; j++) {
					printf("Record id: 		%s \n", cper_hdrs[i]->record_id);
					printf("Error severity: %d \n", cper_hdrs[i]->error_severity);
			}
		}
	}
	} while (ret == AMDSMI_STATUS_MORE_DATA);

	free(processors);
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
}

TEST(amdsmiIntegrationTests, WrongParamsTests)
{
	uint32_t dev_cnt = AMDSMI_MAX_DEVICES;
	uint64_t fake_handle = { 123446 };
	amdsmi_processor_handle fake_dev_handle = &fake_handle;
	amdsmi_vf_handle_t vf_handle;
	amdsmi_driver_info_t driver_version;
	amdsmi_partition_info_t *partitioning_info;
	amdsmi_bdf_t dev_bdf;
	uint32_t num_vf_enabled;
	uint32_t num_vf_supported;
	amdsmi_temperature_type_t sensor_type = AMDSMI_TEMPERATURE_TYPE_EDGE;
	amdsmi_temperature_metric_t metric_current = AMDSMI_TEMP_CURRENT;
	amdsmi_socket_handle socket = NULL;
	amdsmi_event_set set = NULL;
	amdsmi_event_entry_t entry;
	amdsmi_link_topology_t topology_info;
	uint8_t fb_sharing;
	amdsmi_metric_t metrics;
	amdsmi_eeprom_table_record_t bad_pages;
	uint32_t processor_index = AMDSMI_MAX_DEVICES + 100;
	char uuid[AMDSMI_GPU_UUID_SIZE];
	unsigned int uuid_length = AMDSMI_GPU_UUID_SIZE;
	amdsmi_accelerator_partition_profile_t profile;
	uint32_t partition_id[AMDSMI_MAX_ACCELERATOR_PROFILE];
	uint32_t sensor_ind = 0;
	char cper_data[1024*1024];
	amdsmi_cper_hdr* cper_hdrs[1024];
	uint32_t severity_mask = 3;
	uint64_t buf_size = sizeof(cper_data);
	uint64_t entry_count = 1024;
	uint64_t cursor = 0;

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * dev_cnt);
	ASSERT_EQ(amdsmi_get_processor_handles(socket, &dev_cnt, processors), AMDSMI_STATUS_NOT_INIT);

	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

	ASSERT_EQ(amdsmi_get_processor_handles(socket, NULL, processors), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_processor_handles(socket, &dev_cnt, NULL), AMDSMI_STATUS_SUCCESS);

	ASSERT_EQ(amdsmi_get_processor_handles(socket, &dev_cnt, processors), AMDSMI_STATUS_SUCCESS);

	ASSERT_EQ(amdsmi_get_index_from_processor_handle(NULL, &processor_index), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_index_from_processor_handle(processors[0], NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_gpu_driver_info(fake_dev_handle, &driver_version),
		  AMDSMI_STATUS_NOT_FOUND);

	ASSERT_EQ(amdsmi_get_gpu_driver_info(processors[0], NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_driver_model(processors[0], NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_asic_info(processors[0], NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_power_cap_info(processors[0], sensor_ind, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_fb_layout(processors[0], NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_gpu_vbios_info(processors[0], NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_fw_info(processors[0], NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_num_vf(processors[0], NULL, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_num_vf(processors[0], NULL, &num_vf_supported), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_num_vf(processors[0], &num_vf_enabled, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_board_info(processors[0], NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_num_vf(processors[0], &num_vf_enabled, &num_vf_supported), AMDSMI_STATUS_SUCCESS);
	partitioning_info = (amdsmi_partition_info_t*)malloc(sizeof(amdsmi_partition_info_t) * num_vf_enabled);
	ASSERT_EQ(amdsmi_get_vf_partition_info(processors[0], 0, partitioning_info),
		  AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_vf_partition_info(processors[0], num_vf_enabled, NULL),
		  AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_vf_partition_info(processors[0], num_vf_enabled, partitioning_info),
		  AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(amdsmi_get_vf_handle_from_vf_index(processors[0], 0, NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_vf_handle_from_vf_index(processors[0], 0, &vf_handle), AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(vf_handle.handle, partitioning_info[0].id.handle);

	ASSERT_EQ(amdsmi_get_vf_uuid(partitioning_info[0].id, &uuid_length, uuid), AMDSMI_STATUS_SUCCESS);

	ASSERT_EQ(amdsmi_get_processor_handle_from_uuid(uuid, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_vf_handle_from_uuid(uuid, NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_vf_bdf(vf_handle, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_device_bdf(processors[0], &dev_bdf), AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(amdsmi_get_processor_handle_from_bdf(dev_bdf, NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_vf_data(partitioning_info[0].id, NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_gpu_activity(processors[0], NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_power_info(processors[0], sensor_ind, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_soc_pstate(processors[0], NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_soc_pstate(NULL, 0), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_clock_info(processors[0], AMDSMI_CLK_TYPE_MEM, NULL),
		  AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_temp_metric(processors[0], sensor_type, metric_current, NULL),
		  AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_gpu_total_ecc_count(processors[0], NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_ecc_count(processors[0], AMDSMI_GPU_BLOCK_GFX, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_ecc_enabled(processors[0], NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_fw_error_records(processors[0], NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_guest_data(vf_handle, NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_event_create(NULL, 0, 0, &set), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_event_create(processors, 0, 0, &set), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_event_read(set, 0, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_event_read(set, 0, &entry), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_event_destroy(set), AMDSMI_STATUS_SUCCESS);

	ASSERT_EQ(amdsmi_get_link_metrics(processors[0], NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_caps(processors[0], NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_link_topology(processors[0], NULL, &topology_info), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_link_topology(NULL, processors[0], &topology_info), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_link_topology(processors[0], processors[0], NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_mode_info(processors[0], processors[0], AMDSMI_XGMI_FB_SHARING_MODE_4, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_mode_info(NULL, processors[0], AMDSMI_XGMI_FB_SHARING_MODE_4, &fb_sharing), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_mode_info(processors[0], NULL, AMDSMI_XGMI_FB_SHARING_MODE_4, &fb_sharing), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_xgmi_fb_sharing_mode(NULL, AMDSMI_XGMI_FB_SHARING_MODE_4), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_gpu_metrics(processors[0], NULL, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_metrics(processors[0], NULL, &metrics), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_gpu_bad_page_info(processors[0], NULL, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_bad_page_info(processors[0], NULL, &bad_pages), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_get_gpu_accelerator_partition_profile_config(processors[0], NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_accelerator_partition_profile(processors[0], NULL, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_accelerator_partition_profile(processors[0], &profile, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_accelerator_partition_profile(processors[0], NULL, partition_id), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_memory_partition_config(processors[0], NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_gpu_get_cper_entries(processors[0], severity_mask, NULL, &buf_size, cper_hdrs, &entry_count, &cursor), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_gpu_get_cper_entries(processors[0], severity_mask, cper_data, NULL, cper_hdrs, &entry_count, &cursor), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_gpu_get_cper_entries(processors[0], severity_mask, cper_data, &buf_size, NULL, &entry_count, &cursor), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_gpu_get_cper_entries(processors[0], severity_mask, cper_data, &buf_size, cper_hdrs, NULL, &cursor), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_gpu_get_cper_entries(processors[0], severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, NULL), AMDSMI_STATUS_INVAL);

	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(amdsmi_get_processor_handles(socket, &dev_cnt, processors), AMDSMI_STATUS_NOT_INIT);
	free(partitioning_info);
	free(processors);
}

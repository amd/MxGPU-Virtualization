/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
#include "smi_processor_handle.h"
#include "smi_nic_interface.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"
#include "smi_fake_nic_interface.h"

class AmdSmiNicTests : public amdsmi::AmdSmiTest {
public:
	struct statusPair {
		smi_nic_status nic_status;
		amdsmi_status_t amdsmi_status;
	};

	statusPair map_status[8] = {
		{SMI_NIC_STATUS_SUCCESS, AMDSMI_STATUS_SUCCESS},
		{SMI_NIC_STATUS_ERROR, AMDSMI_STATUS_API_FAILED},
		{SMI_NIC_STATUS_WRONG_PARAM, AMDSMI_STATUS_INVAL},
		{SMI_NIC_STATUS_NOT_FOUND, AMDSMI_STATUS_NOT_FOUND},
		{SMI_NIC_STATUS_NO_RESOURCE, AMDSMI_STATUS_OUT_OF_RESOURCES},
		{SMI_NIC_STATUS_NOT_SUPPORTED, AMDSMI_STATUS_NOT_SUPPORTED},
		{SMI_NIC_STATUS_NOT_INIT, AMDSMI_STATUS_NOT_INIT},
		{SMI_NIC_STATUS_NO_DATA, AMDSMI_STATUS_NO_DATA}
	};
};

TEST_F(AmdSmiNicTests, GetNicDriverInfo) {
	amdsmi_nic_driver_info_t driver_info;
	int ret;
	smi_nic_status status;
	int num_status_codes = 0;

	status = get_nic_api_status();
	num_status_codes = sizeof(map_status)/sizeof(map_status[0]);
	for (int i = SMI_NIC_STATUS_SUCCESS; i < num_status_codes; i++) {
		set_nic_api_status((smi_nic_status)i);
		ret = amdsmi_get_nic_driver_info(&NIC_MOCK_HANDLE, &driver_info);
		ASSERT_EQ(ret, map_status[i].amdsmi_status);
		if (i == SMI_NIC_STATUS_SUCCESS) {
			EXPECT_STREQ(driver_info.name, "driver_mock");
			EXPECT_STREQ(driver_info.version, "1.0.0");
		}
	}
	set_nic_api_status(status);

	ret = amdsmi_get_nic_driver_info(nullptr, &driver_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_nic_driver_info(&NIC_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	struct smi_gpu_handle gpu_handle = {SMI_PROCESSOR_TYPE_AMD_GPU, {0}};
	ret = amdsmi_get_nic_driver_info(&gpu_handle, &driver_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiNicTests, GetNicAsicInfo) {
	amdsmi_nic_asic_info_t asic_info;
	int ret;
	smi_nic_status status;
	int num_status_codes = 0;

	status = get_nic_api_status();
	num_status_codes = sizeof(map_status)/sizeof(map_status[0]);
	for (int i = SMI_NIC_STATUS_SUCCESS; i < num_status_codes; i++) {
		set_nic_api_status((smi_nic_status)i);
		ret = amdsmi_get_nic_asic_info(&NIC_MOCK_HANDLE, &asic_info);
		ASSERT_EQ(ret, map_status[i].amdsmi_status);
		if (i == SMI_NIC_STATUS_SUCCESS) {
			EXPECT_EQ(asic_info.vendor_id, 0x1002);
			EXPECT_EQ(asic_info.subvendor_id, 0x1234);
			EXPECT_EQ(asic_info.device_id, 0x5678);
			EXPECT_EQ(asic_info.subsystem_id, 0x9ABC);
			EXPECT_EQ(asic_info.revision, 0x01);
			EXPECT_STREQ(asic_info.permanent_address, "aa:bb:cc:dd:ee:ff");
			EXPECT_STREQ(asic_info.product_name, "AMD NIC");
			EXPECT_STREQ(asic_info.vendor_name, "AMD");
			EXPECT_STREQ(asic_info.part_number, "AMD-NIC-1234");
			EXPECT_STREQ(asic_info.serial_number, "SN0123456789");
		}
	}
	set_nic_api_status(status);

	ret = amdsmi_get_nic_asic_info(nullptr, &asic_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_nic_asic_info(&NIC_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	struct smi_gpu_handle gpu_handle = {SMI_PROCESSOR_TYPE_AMD_GPU, {0}};
	ret = amdsmi_get_nic_asic_info(&gpu_handle, &asic_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiNicTests, GetNicBusInfo) {
	amdsmi_nic_bus_info_t bus_info;
	int ret;
	smi_nic_status status;
	int num_status_codes = 0;

	status = get_nic_api_status();
	num_status_codes = sizeof(map_status)/sizeof(map_status[0]);
	for (int i = SMI_NIC_STATUS_SUCCESS; i < num_status_codes; i++) {
		set_nic_api_status((smi_nic_status)i);
		ret = amdsmi_get_nic_bus_info(&NIC_MOCK_HANDLE, &bus_info);
		ASSERT_EQ(ret, map_status[i].amdsmi_status);
		if (i == SMI_NIC_STATUS_SUCCESS) {
			EXPECT_EQ(bus_info.bdf.bdf.domain_number, 0x0001);
			EXPECT_EQ(bus_info.bdf.bdf.bus_number, 0x02);
			EXPECT_EQ(bus_info.bdf.bdf.device_number, 0x03);
			EXPECT_EQ(bus_info.bdf.bdf.function_number, 0x4);
			EXPECT_EQ(bus_info.max_pcie_width, 16);
			EXPECT_EQ(bus_info.max_pcie_speed, 16);
		}
	}
	set_nic_api_status(status);

	ret = amdsmi_get_nic_bus_info(nullptr, &bus_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_nic_bus_info(&NIC_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	struct smi_gpu_handle gpu_handle = {SMI_PROCESSOR_TYPE_AMD_GPU, {0}};
	ret = amdsmi_get_nic_bus_info(&gpu_handle, &bus_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiNicTests, GetNicNumaInfo) {
	amdsmi_nic_numa_info_t numa_info;
	int ret;
	smi_nic_status status;
	int num_status_codes = 0;

	status = get_nic_api_status();
	num_status_codes = sizeof(map_status)/sizeof(map_status[0]);
	for (int i = SMI_NIC_STATUS_SUCCESS; i < num_status_codes; i++) {
		set_nic_api_status((smi_nic_status)i);
		ret = amdsmi_get_nic_numa_info(&NIC_MOCK_HANDLE, &numa_info);
		ASSERT_EQ(ret, map_status[i].amdsmi_status);
		if (i == SMI_NIC_STATUS_SUCCESS) {
			EXPECT_EQ(numa_info.node, 0);
			EXPECT_STREQ(numa_info.affinity, "0,1,2,3");
		}
	}
	set_nic_api_status(status);

	ret = amdsmi_get_nic_numa_info(nullptr, &numa_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_nic_numa_info(&NIC_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	struct smi_gpu_handle gpu_handle = {SMI_PROCESSOR_TYPE_AMD_GPU, {0}};
	ret = amdsmi_get_nic_numa_info(&gpu_handle, &numa_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiNicTests, GetNicPortInfo) {
	amdsmi_nic_port_info_t port_info;
	int ret;
	smi_nic_status status;
	int num_status_codes = 0;

	status = get_nic_api_status();
	num_status_codes = sizeof(map_status)/sizeof(map_status[0]);
	for (int i = SMI_NIC_STATUS_SUCCESS; i < num_status_codes; i++) {
		set_nic_api_status((smi_nic_status)i);
		ret = amdsmi_get_nic_port_info(&NIC_MOCK_HANDLE, &port_info);
		ASSERT_EQ(ret, map_status[i].amdsmi_status);
		if (i == SMI_NIC_STATUS_SUCCESS) {
			EXPECT_EQ(port_info.num_ports, 1);
			EXPECT_EQ(port_info.ports[0].port_num, 0);
			EXPECT_STREQ(port_info.ports[0].netdev, "eth0");
			EXPECT_STREQ(port_info.ports[0].type, "Ethernet");
			EXPECT_EQ(port_info.ports[0].ifindex, 2);
			EXPECT_EQ(port_info.ports[0].carrier, 1);
			EXPECT_EQ(port_info.ports[0].mtu, 1500);
			EXPECT_STREQ(port_info.ports[0].mac_address, "aa:bb:cc:dd:ee:ff");
			EXPECT_STREQ(port_info.ports[0].link_state, "UP");
			EXPECT_EQ(port_info.ports[0].link_speed, 10000);
			EXPECT_EQ(port_info.ports[0].active_fec, 1);
			EXPECT_STREQ(port_info.ports[0].autoneg, "ENABLED");
			EXPECT_STREQ(port_info.ports[0].pause_autoneg, "ON");
			EXPECT_STREQ(port_info.ports[0].pause_rx, "ON");
			EXPECT_STREQ(port_info.ports[0].pause_tx, "ON");
		}
	}
	set_nic_api_status(status);

	ret = amdsmi_get_nic_port_info(nullptr, &port_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_nic_port_info(&NIC_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	struct smi_gpu_handle gpu_handle = {SMI_PROCESSOR_TYPE_AMD_GPU, {0}};
	ret = amdsmi_get_nic_port_info(&gpu_handle, &port_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiNicTests, GetNicVendorStatistics) {
	uint32_t num_stats;
	amdsmi_nic_stat_t *stats;
	int ret;
	smi_nic_status status;
	int num_status_codes = 0;

	status = get_nic_api_status();
	num_status_codes = sizeof(map_status)/sizeof(map_status[0]);
	for (int i = SMI_NIC_STATUS_SUCCESS; i < num_status_codes; i++) {
		set_nic_api_status((smi_nic_status)i);

		ret = amdsmi_get_nic_vendor_statistics(&NIC_MOCK_HANDLE, 0, &num_stats, nullptr);
		ASSERT_EQ(ret, map_status[i].amdsmi_status);
		if (i == SMI_NIC_STATUS_SUCCESS) {
			EXPECT_EQ(num_stats, 7);
		}

		if (i == SMI_NIC_STATUS_SUCCESS) {
			stats = (amdsmi_nic_stat_t*)malloc(num_stats * sizeof(amdsmi_nic_stat_t));
			uint32_t actual_count = num_stats;
			ret = amdsmi_get_nic_vendor_statistics(&NIC_MOCK_HANDLE, 0, &actual_count, stats);
			ASSERT_EQ(ret, map_status[i].amdsmi_status);
			EXPECT_EQ(actual_count, 7);
			EXPECT_STREQ(stats[0].name, "vendor_stat1");
			EXPECT_EQ(stats[0].value, 300);
			free(stats);
		}
	}
	set_nic_api_status(status);

	ret = amdsmi_get_nic_vendor_statistics(nullptr, 0, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_nic_vendor_statistics(&NIC_MOCK_HANDLE, 0, nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	struct smi_gpu_handle gpu_handle = {SMI_PROCESSOR_TYPE_AMD_GPU, {0}};
	ret = amdsmi_get_nic_vendor_statistics(&gpu_handle, 0, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiNicTests, GetNicPortStatistics) {
	uint32_t num_stats;
	amdsmi_nic_stat_t *stats;
	int ret;
	smi_nic_status status;
	int num_status_codes = 0;

	status = get_nic_api_status();
	num_status_codes = sizeof(map_status)/sizeof(map_status[0]);
	for (int i = SMI_NIC_STATUS_SUCCESS; i < num_status_codes; i++) {
		set_nic_api_status((smi_nic_status)i);

		ret = amdsmi_get_nic_port_statistics(&NIC_MOCK_HANDLE, 0, &num_stats, nullptr);
		ASSERT_EQ(ret, map_status[i].amdsmi_status);
		if (i == SMI_NIC_STATUS_SUCCESS) {
			EXPECT_EQ(num_stats, 7);
		}

		if (i == SMI_NIC_STATUS_SUCCESS) {
			stats = (amdsmi_nic_stat_t*)malloc(num_stats * sizeof(amdsmi_nic_stat_t));
			uint32_t actual_count = num_stats;
			ret = amdsmi_get_nic_port_statistics(&NIC_MOCK_HANDLE, 0, &actual_count, stats);
			ASSERT_EQ(ret, map_status[i].amdsmi_status);
			EXPECT_EQ(actual_count, 7);
			EXPECT_STREQ(stats[0].name, "port_stat1");
			EXPECT_EQ(stats[0].value, 300);
			free(stats);
		}
	}
	set_nic_api_status(status);

	ret = amdsmi_get_nic_port_statistics(nullptr, 0, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_nic_port_statistics(&NIC_MOCK_HANDLE, 0, nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	struct smi_gpu_handle gpu_handle = {SMI_PROCESSOR_TYPE_AMD_GPU, {0}};
	ret = amdsmi_get_nic_port_statistics(&gpu_handle, 0, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiNicTests, GetNicRdmaDevInfo) {
	amdsmi_nic_rdma_devices_info_t rdma_info;
	int ret;
	smi_nic_status status;
	int num_status_codes = 0;

	status = get_nic_api_status();
	num_status_codes = sizeof(map_status)/sizeof(map_status[0]);
	for (int i = SMI_NIC_STATUS_SUCCESS; i < num_status_codes; i++) {
		set_nic_api_status((smi_nic_status)i);
		ret = amdsmi_get_nic_rdma_dev_info(&NIC_MOCK_HANDLE, &rdma_info);
		ASSERT_EQ(ret, map_status[i].amdsmi_status);
		if (i == SMI_NIC_STATUS_SUCCESS) {
			EXPECT_EQ(rdma_info.num_rdma_dev, 1);
			EXPECT_STREQ(rdma_info.rdma_dev_info[0].rdma_dev, "mlx5_0");
			EXPECT_STREQ(rdma_info.rdma_dev_info[0].node_guid, "0x1234567890abcdef");
			EXPECT_STREQ(rdma_info.rdma_dev_info[0].node_type, "CA");
			EXPECT_STREQ(rdma_info.rdma_dev_info[0].sys_image_guid, "0xfedcba0987654321");
			EXPECT_STREQ(rdma_info.rdma_dev_info[0].fw_ver, "20.32.1010");
			EXPECT_EQ(rdma_info.rdma_dev_info[0].num_rdma_ports, 2);
			EXPECT_EQ(rdma_info.rdma_dev_info[0].rdma_port_info[0].rdma_port, 0);
			EXPECT_STREQ(rdma_info.rdma_dev_info[0].rdma_port_info[0].netdev, "eth0");
			EXPECT_STREQ(rdma_info.rdma_dev_info[0].rdma_port_info[0].state, "ACTIVE");
			EXPECT_EQ(rdma_info.rdma_dev_info[0].rdma_port_info[0].max_mtu, 4096);
			EXPECT_EQ(rdma_info.rdma_dev_info[0].rdma_port_info[0].active_mtu, 4096);
		}
	}
	set_nic_api_status(status);

	ret = amdsmi_get_nic_rdma_dev_info(nullptr, &rdma_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_nic_rdma_dev_info(&NIC_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	struct smi_gpu_handle gpu_handle = {SMI_PROCESSOR_TYPE_AMD_GPU, {0}};
	ret = amdsmi_get_nic_rdma_dev_info(&gpu_handle, &rdma_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiNicTests, GetNicRdmaPortStatistics) {
	uint32_t num_stats;
	amdsmi_nic_stat_t *stats;
	int ret;
	smi_nic_status status;
	int num_status_codes = 0;

	status = get_nic_api_status();
	num_status_codes = sizeof(map_status)/sizeof(map_status[0]);
	for (int i = SMI_NIC_STATUS_SUCCESS; i < num_status_codes; i++) {
		set_nic_api_status((smi_nic_status)i);

		ret = amdsmi_get_nic_rdma_port_statistics(&NIC_MOCK_HANDLE, 0, &num_stats, nullptr);
		ASSERT_EQ(ret, map_status[i].amdsmi_status);
		if (i == SMI_NIC_STATUS_SUCCESS) {
			EXPECT_EQ(num_stats, 7);
		}

		if (i == SMI_NIC_STATUS_SUCCESS) {
			stats = (amdsmi_nic_stat_t*)malloc(num_stats * sizeof(amdsmi_nic_stat_t));
			uint32_t actual_count = num_stats;
			ret = amdsmi_get_nic_rdma_port_statistics(&NIC_MOCK_HANDLE, 0, &actual_count, stats);
			ASSERT_EQ(ret, map_status[i].amdsmi_status);
			EXPECT_EQ(actual_count, 7);
			EXPECT_STREQ(stats[0].name, "rdma_stat1");
			EXPECT_EQ(stats[0].value, 300);
			free(stats);
		}
	}
	set_nic_api_status(status);


	ret = amdsmi_get_nic_rdma_port_statistics(nullptr, 0, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_nic_rdma_port_statistics(&NIC_MOCK_HANDLE, 0, nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	struct smi_gpu_handle gpu_handle = {SMI_PROCESSOR_TYPE_AMD_GPU, {0}};
	ret = amdsmi_get_nic_rdma_port_statistics(&gpu_handle, 0, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

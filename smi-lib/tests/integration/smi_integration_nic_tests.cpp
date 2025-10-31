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

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <cstring>

#include <gtest/gtest.h>
extern "C" {
#include "amdsmi.h"
}

class AmdSmiNicIntegrationTests : public ::testing::Test {
protected:
	void SetUp() override {
		ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);

		uint32_t nic_count = AMDSMI_MAX_DEVICES;
		nic_processors_ = std::make_unique<amdsmi_processor_handle[]>(nic_count);

		amdsmi_status_t ret = amdsmi_get_processor_handles_by_type(
			nullptr,
			AMDSMI_PROCESSOR_TYPE_AMD_NIC,
			nic_processors_.get(),
			&nic_count
		);

		if (ret == AMDSMI_STATUS_SUCCESS && nic_count > 0) {
			nic_count_ = nic_count;
			std::cout << "Found " << nic_count_ << " NIC device(s)" << std::endl;
		} else {
			nic_count_ = 0;
			GTEST_SKIP() << "No NIC devices available - skipping entire test suite";
		}
	}

	void TearDown() override {
		nic_processors_.reset();
		ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);
	}

	std::unique_ptr<amdsmi_processor_handle[]> nic_processors_;
	uint32_t nic_count_ = 0;
};

TEST_F(AmdSmiNicIntegrationTests, NicDeviceDiscovery)
{
	amdsmi_processor_type_t processor_type;

	for (uint32_t i = 0; i < nic_count_; i++) {
		ASSERT_EQ(amdsmi_get_processor_type(nic_processors_[i], &processor_type),
			AMDSMI_STATUS_SUCCESS);
		ASSERT_EQ(processor_type, AMDSMI_PROCESSOR_TYPE_AMD_NIC);

	}
}

TEST_F(AmdSmiNicIntegrationTests, NicDriverInfoTest)
{
	amdsmi_nic_driver_info_t driver_info;

	for (uint32_t i = 0; i < nic_count_; i++) {
		amdsmi_status_t ret = amdsmi_get_nic_driver_info(nic_processors_[i], &driver_info);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "NIC " << i << " Driver Info:" << std::endl;
			std::cout << "  Name: " << driver_info.name << std::endl;
			std::cout << "  Version: " << driver_info.version << std::endl;

			EXPECT_GT(strlen(driver_info.name), 0);
			EXPECT_GT(strlen(driver_info.version), 0);
		} else {
			std::cout << "NIC " << i << " driver info not available (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
		}
	}
}

TEST_F(AmdSmiNicIntegrationTests, NicAsicInfoTest)
{
	amdsmi_nic_asic_info_t asic_info;
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;

	for (uint32_t i = 0; i < nic_count_; i++) {
		ret = amdsmi_get_nic_asic_info(nic_processors_[i], &asic_info);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "NIC " << i << " ASIC Info:" << std::endl;
			std::cout << "  Vendor ID: 0x" << std::hex << asic_info.vendor_id << std::dec << std::endl;
			std::cout << "  Device ID: 0x" << std::hex << asic_info.device_id << std::dec << std::endl;
			std::cout << "  Subvendor ID: 0x" << std::hex << asic_info.subvendor_id << std::dec << std::endl;
			std::cout << "  Subsystem ID: 0x" << std::hex << asic_info.subsystem_id << std::dec << std::endl;
			std::cout << "  Revision: 0x" << std::hex << (int)asic_info.revision << std::dec << std::endl;
			std::cout << "  Permanent Address: " << asic_info.permanent_address << std::endl;
			std::cout << "  Product Name: " << asic_info.product_name << std::endl;
			std::cout << "  Part Number: " << asic_info.part_number << std::endl;
			std::cout << "  Serial Number: " << asic_info.serial_number << std::endl;
			std::cout << "  Vendor Name: " << asic_info.vendor_name << std::endl;

			EXPECT_NE(asic_info.vendor_id, 0);
			EXPECT_NE(asic_info.device_id, 0);
		} else {
			std::cout << "NIC " << i << " ASIC info not available (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
		}
	}
}

TEST_F(AmdSmiNicIntegrationTests, NicBusInfoTest)
{
	amdsmi_nic_bus_info_t bus_info;
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;

	for (uint32_t i = 0; i < nic_count_; i++) {
		ret = amdsmi_get_nic_bus_info(nic_processors_[i], &bus_info);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "NIC " << i << " Bus Info:" << std::endl;
			printf("  BDF: %04x:%02x:%02x.%1x\n", 
				(unsigned int)bus_info.bdf.bdf.domain_number, bus_info.bdf.bdf.bus_number,
				bus_info.bdf.bdf.device_number, bus_info.bdf.bdf.function_number);
			std::cout << "  Max PCIe Width: " << (int)bus_info.max_pcie_width << std::endl;
			std::cout << "  Max PCIe Speed: " << bus_info.max_pcie_speed << " GT/s" << std::endl;

			EXPECT_GT(bus_info.bdf.as_uint, 0);
			EXPECT_GT(bus_info.max_pcie_width, 0);
			EXPECT_GT(bus_info.max_pcie_speed, 0);
		} else {
			std::cout << "NIC " << i << " bus info not available (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
		}
	}
}

TEST_F(AmdSmiNicIntegrationTests, NicNumaInfoTest)
{
	amdsmi_nic_numa_info_t numa_info;
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;

	for (uint32_t i = 0; i < nic_count_; i++) {
		ret = amdsmi_get_nic_numa_info(nic_processors_[i], &numa_info);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "NIC " << i << " NUMA Info:" << std::endl;
			std::cout << "  Node: " << (int)numa_info.node << std::endl;
			std::cout << "  Affinity: " << numa_info.affinity << std::endl;

			EXPECT_GE(numa_info.node, 0);
			EXPECT_GT(strlen(numa_info.affinity), 0);
		} else {
			std::cout << "NIC " << i << " NUMA info not available (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
		}
	}
}

TEST_F(AmdSmiNicIntegrationTests, NicPortInfoTest)
{
	amdsmi_nic_port_info_t port_info;
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;

	for (uint32_t i = 0; i < nic_count_; i++) {
		ret = amdsmi_get_nic_port_info(nic_processors_[i], &port_info);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "NIC " << i << " Port Info:" << std::endl;
			std::cout << "  Number of ports: " << port_info.num_ports << std::endl;
			for (uint32_t j = 0; j < port_info.num_ports; j++) {
				const auto& port = port_info.ports[j];
				std::cout << "  Port " << j << ":" << std::endl;
				std::cout << "    Port Number: " << port.port_num << std::endl;
				std::cout << "    Netdev: " << port.netdev << std::endl;
				std::cout << "    Type: " << port.type << std::endl;
				std::cout << "    Interface Index: " << (int)port.ifindex << std::endl;
				std::cout << "    Carrier: " << (int)port.carrier << std::endl;
				std::cout << "    MTU: " << port.mtu << std::endl;
				std::cout << "    MAC Address: " << port.mac_address << std::endl;
				std::cout << "    Link State: " << port.link_state << std::endl;
				std::cout << "    Link Speed: " << port.link_speed << std::endl;
				std::cout << "    Active FEC: " << port.active_fec << std::endl;
				std::cout << "    Autoneg: " << port.autoneg << std::endl;
				std::cout << "    Pause Autoneg: " << port.pause_autoneg << std::endl;
				std::cout << "    Pause RX: " << port.pause_rx << std::endl;
				std::cout << "    Pause TX: " << port.pause_tx << std::endl;

				EXPECT_GT(strlen(port.netdev), 0);
				EXPECT_GT(strlen(port.type), 0);
			}
		} else {
			std::cout << "NIC " << i << " port info not available (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
		}
    }
}

TEST_F(AmdSmiNicIntegrationTests, NicVendorStatisticsTest)
{
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;

	for (uint32_t i = 0; i < nic_count_; i++) {
		std::cout << "NIC " << i << " Device Statistics:" << std::endl;

		uint32_t num_stats = 0;
		ret = amdsmi_get_nic_vendor_statistics(nic_processors_[i], 0, &num_stats, nullptr);

		if (ret != AMDSMI_STATUS_SUCCESS) {
			std::cout << "  Failed to get vendor statistics count (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
			continue;
		}

		if (num_stats == 0) {
			std::cout << "  No vendor statistics available" << std::endl;
			continue;
		}

		amdsmi_nic_stat_t* stats = new amdsmi_nic_stat_t[num_stats];
		ret = amdsmi_get_nic_vendor_statistics(nic_processors_[i], 0, &num_stats, stats);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			for (uint32_t j = 0; j < num_stats; j++) {
				std::cout << "  " << stats[j].name << ": " << stats[j].value << std::endl;
				EXPECT_GE(stats[j].value, 0);
			}
		} else {
			std::cout << "  Failed to get vendor statistics (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
		}

		delete[] stats;
	}
}

TEST_F(AmdSmiNicIntegrationTests, NicPortStatisticsTest)
{
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;

	for (uint32_t i = 0; i < nic_count_; i++) {
		std::cout << "NIC " << i << " Port Statistics:" << std::endl;

		uint32_t num_stats = 0;
		ret = amdsmi_get_nic_port_statistics(nic_processors_[i], 0, &num_stats, nullptr);

		if (ret != AMDSMI_STATUS_SUCCESS) {
			std::cout << "  Failed to get port statistics count (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
			continue;
		}

		if (num_stats == 0) {
			std::cout << "  No port statistics available" << std::endl;
			continue;
		}

		amdsmi_nic_stat_t* stats = new amdsmi_nic_stat_t[num_stats];
		ret = amdsmi_get_nic_port_statistics(nic_processors_[i], 0, &num_stats, stats);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			for (uint32_t j = 0; j < num_stats; j++) {
				std::cout << "  Port 0: " << stats[j].name << ": " << stats[j].value << std::endl;
				EXPECT_GE(stats[j].value, 0);
			}
		} else {
			std::cout << "  Failed to get port statistics (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
		}

		delete[] stats;
	}
}

TEST_F(AmdSmiNicIntegrationTests, NicRdmaDevInfoTest)
{
	amdsmi_nic_rdma_devices_info_t rdma_info;
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;

	for (uint32_t i = 0; i < nic_count_; i++) {
		ret = amdsmi_get_nic_rdma_dev_info(nic_processors_[i], &rdma_info);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "NIC " << i << " RDMA Info:" << std::endl;
			std::cout << "  Number of RDMA devices: " << (int)rdma_info.num_rdma_dev << std::endl;

			EXPECT_LE(rdma_info.num_rdma_dev, AMDSMI_MAX_NIC_RDMA_DEV);

			for (uint8_t j = 0; j < rdma_info.num_rdma_dev && j < AMDSMI_MAX_NIC_RDMA_DEV; j++) {
				const auto& dev_info = rdma_info.rdma_dev_info[j];
				std::cout << "  RDMA Device " << (int)j << ":" << std::endl;
				std::cout << "    Device: " << dev_info.rdma_dev << std::endl;
				std::cout << "    Node GUID: " << dev_info.node_guid << std::endl;
				std::cout << "    Node Type: " << dev_info.node_type << std::endl;
				std::cout << "    System Image GUID: " << dev_info.sys_image_guid << std::endl;
				std::cout << "    FW Version: " << dev_info.fw_ver << std::endl;
				std::cout << "    Number of RDMA ports: " << (int)dev_info.num_rdma_ports << std::endl;

				EXPECT_GT(strlen(dev_info.rdma_dev), 0);
				EXPECT_LE(dev_info.num_rdma_ports, AMDSMI_MAX_NIC_PORTS);

				for (uint8_t k = 0; k < dev_info.num_rdma_ports && k < AMDSMI_MAX_NIC_PORTS; k++) {
					const auto& port_info = dev_info.rdma_port_info[k];
					std::cout << "            Port " << (int)k << ":" << std::endl;
					std::cout << "                Netdev: " << port_info.netdev << std::endl;
					std::cout << "                State: " << port_info.state << std::endl;
					std::cout << "                Max MTU: " << port_info.max_mtu << std::endl;
					std::cout << "                Active MTU: " << port_info.active_mtu << std::endl;
					std::cout << "                RDMA Port: " << (int)port_info.rdma_port << std::endl;
				}
			}
		} else {
			std::cout << "NIC " << i << " RDMA info not available (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND || ret == AMDSMI_STATUS_DRIVER_NOT_LOADED);
		}
	}
}

TEST_F(AmdSmiNicIntegrationTests, NicRdmaPortStatisticsTest)
{
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;

	for (uint32_t i = 0; i < nic_count_; i++) {
		std::cout << "NIC " << i << " RDMA Port Statistics:" << std::endl;

		uint32_t num_stats = 0;
		ret = amdsmi_get_nic_rdma_port_statistics(nic_processors_[i], 0, &num_stats, nullptr);

		if (ret != AMDSMI_STATUS_SUCCESS) {
			std::cout << "  Failed to get RDMA port statistics count (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
			continue;
		}

		if (num_stats == 0) {
			std::cout << "  No RDMA port statistics available" << std::endl;
			continue;
		}

		amdsmi_nic_stat_t* stats = new amdsmi_nic_stat_t[num_stats];
		ret = amdsmi_get_nic_rdma_port_statistics(nic_processors_[i], 0, &num_stats, stats);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			for (uint32_t j = 0; j < num_stats; j++) {
				std::cout << "  RDMA Port 0: " << stats[j].name << ": " << stats[j].value << std::endl;
				EXPECT_GE(stats[j].value, 0);
			}
		} else {
			std::cout << "  Failed to get RDMA port statistics (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
		}

		delete[] stats;
	}
}

TEST_F(AmdSmiNicIntegrationTests, NicBdfTest)
{
	amdsmi_bdf_t bdf;
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;
	amdsmi_processor_handle handle;

	for (uint32_t i = 0; i < nic_count_; i++) {
		ret = amdsmi_get_nic_device_bdf(nic_processors_[i], &bdf);

		if (ret == AMDSMI_STATUS_SUCCESS) {
			std::cout << "NIC " << i << " BDF: " << std::hex
				<< "Domain: 0x" << bdf.bdf.domain_number
				<< ", Bus: 0x" << bdf.bdf.bus_number
				<< ", Device: 0x" << bdf.bdf.device_number
				<< ", Function: 0x" << bdf.bdf.function_number
				<< std::dec << std::endl;

			ret = amdsmi_get_processor_handle_from_bdf(bdf, &handle);
			ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
			ASSERT_EQ(handle, nic_processors_[i]);
		} else {
			std::cout << "NIC " << i << " BDF not available (status: " << ret << ")" << std::endl;
			EXPECT_TRUE(ret == AMDSMI_STATUS_NOT_SUPPORTED || ret == AMDSMI_STATUS_NOT_FOUND);
		}
	}
}

static void nic_walkthrough_test(amdsmi_processor_handle *nic_processors, uint32_t nic_count, bool verbose = true)
{
	if (verbose) {
		std::cout << "Testing " << nic_count << " NIC device(s)" << std::endl;
	}

	for (uint32_t i = 0; i < nic_count; i++) {
		if (verbose) {
			std::cout << "\n---- NIC Device " << i << " ---" << std::endl;
		}

		// 1. Driver Information
		if (verbose) {
			std::cout << "\n1. Driver Information:" << std::endl;
		}
		amdsmi_nic_driver_info_t driver_info;
		ASSERT_EQ(amdsmi_get_nic_driver_info(nic_processors[i], &driver_info), AMDSMI_STATUS_SUCCESS);
		if (verbose) {
			std::cout << "       Driver Name: " << driver_info.name << std::endl;
			std::cout << "       Driver Version: " << driver_info.version << std::endl;
		}
		EXPECT_GT(strlen(driver_info.name), 0);
		EXPECT_GT(strlen(driver_info.version), 0);

		// 2. ASIC Information
		if (verbose) {
			std::cout << "\n2. ASIC Information:" << std::endl;
		}
		amdsmi_nic_asic_info_t asic_info;
		ASSERT_EQ(amdsmi_get_nic_asic_info(nic_processors[i], &asic_info), AMDSMI_STATUS_SUCCESS);
		if (verbose) {
			std::cout << "       Vendor ID: 0x" << std::hex << asic_info.vendor_id << std::dec << std::endl;
			std::cout << "       Device ID: 0x" << std::hex << asic_info.device_id << std::dec << std::endl;
			std::cout << "       Subvendor ID: 0x" << std::hex << asic_info.subvendor_id << std::dec << std::endl;
			std::cout << "       Subsystem ID: 0x" << std::hex << asic_info.subsystem_id << std::dec << std::endl;
			std::cout << "       Revision: 0x" << std::hex << (int)asic_info.revision << std::dec << std::endl;
			std::cout << "       Permanent Address: " << asic_info.permanent_address << std::endl;
			std::cout << "       Product Name: " << asic_info.product_name << std::endl;
			std::cout << "       Part Number: " << asic_info.part_number << std::endl;
			std::cout << "       Serial Number: " << asic_info.serial_number << std::endl;
			std::cout << "       Vendor Name: " << asic_info.vendor_name << std::endl;
		}
		EXPECT_NE(asic_info.vendor_id, 0);
		EXPECT_NE(asic_info.device_id, 0);

		// 3. Bus Information
		if (verbose) {
			std::cout << "\n3. Bus Information:" << std::endl;
		}
		amdsmi_nic_bus_info_t bus_info;
		ASSERT_EQ(amdsmi_get_nic_bus_info(nic_processors[i], &bus_info), AMDSMI_STATUS_SUCCESS);
		if (verbose) {
			printf("       BDF: %04x:%02x:%02x.%1x\n",
				(unsigned int)bus_info.bdf.bdf.domain_number, bus_info.bdf.bdf.bus_number,
				bus_info.bdf.bdf.device_number, bus_info.bdf.bdf.function_number);
			std::cout << "       Max PCIe Width: " << (int)bus_info.max_pcie_width << " lanes" << std::endl;
			std::cout << "       Max PCIe Speed: " << bus_info.max_pcie_speed << " GT/s" << std::endl;
		}
		EXPECT_GT(bus_info.bdf.as_uint, 0);
		EXPECT_GT(bus_info.max_pcie_width, 0);
		EXPECT_GT(bus_info.max_pcie_speed, 0);

		// 4. NUMA Information
		if (verbose) {
			std::cout << "\n4. NUMA Information:" << std::endl;
		}
		amdsmi_nic_numa_info_t numa_info;
		ASSERT_EQ(amdsmi_get_nic_numa_info(nic_processors[i], &numa_info), AMDSMI_STATUS_SUCCESS);
		if (verbose) {
			std::cout << "       NUMA Node: " << (int)numa_info.node << std::endl;
			std::cout << "       CPU Affinity: " << numa_info.affinity << std::endl;
		}
		EXPECT_GE(numa_info.node, 0);
		EXPECT_GT(strlen(numa_info.affinity), 0);

		// 5. Port Information
		if (verbose) {
			std::cout << "\n5. Port Information:" << std::endl;
		}
		amdsmi_nic_port_info_t port_info;
		ASSERT_EQ(amdsmi_get_nic_port_info(nic_processors[i], &port_info), AMDSMI_STATUS_SUCCESS);
		if (verbose) {
			std::cout << "       Number of ports: " << port_info.num_ports << std::endl;
			for (uint32_t port_idx = 0; port_idx < port_info.num_ports; port_idx++) {
				const auto& port = port_info.ports[port_idx];
				std::cout << "       Port " << port_idx << ":" << std::endl;
				std::cout << "         Port Number: " << port.port_num << std::endl;
				std::cout << "         Network Device: " << port.netdev << std::endl;
				std::cout << "         Port Type: " << port.type << std::endl;
				std::cout << "         Interface Index: " << (int)port.ifindex << std::endl;
				std::cout << "         Carrier Status: " << (port.carrier ? "UP" : "DOWN") << std::endl;
				std::cout << "         MTU: " << port.mtu << " bytes" << std::endl;
				std::cout << "         MAC Address: " << port.mac_address << std::endl;
				std::cout << "         Link State: " << port.link_state << std::endl;
				std::cout << "         Link Speed: " << port.link_speed << std::endl;
				std::cout << "         Active FEC: " << port.active_fec << std::endl;
				std::cout << "         Autoneg: " << port.autoneg << std::endl;
				std::cout << "         Pause Autoneg: " << port.pause_autoneg << std::endl;
				std::cout << "         Pause RX: " << port.pause_rx << std::endl;
				std::cout << "         Pause TX: " << port.pause_tx << std::endl;
			}
		}
		EXPECT_GT(port_info.num_ports, 0);
		if (port_info.num_ports > 0) {
			EXPECT_GT(strlen(port_info.ports[0].netdev), 0);
			EXPECT_GT(strlen(port_info.ports[0].type), 0);
		}

		// 6. Device Statistics
		if (verbose) {
			std::cout << "\n6. Device Statistics:" << std::endl;
		}
		uint32_t num_vendor_stats = 0;
		ASSERT_EQ(amdsmi_get_nic_vendor_statistics(nic_processors[i], 0, &num_vendor_stats, nullptr), AMDSMI_STATUS_SUCCESS);
		if (num_vendor_stats > 0) {
			amdsmi_nic_stat_t* vendor_stats = new amdsmi_nic_stat_t[num_vendor_stats];
			ASSERT_EQ(amdsmi_get_nic_vendor_statistics(nic_processors[i], 0, &num_vendor_stats, vendor_stats), AMDSMI_STATUS_SUCCESS);
			if (verbose) {
				for (uint32_t j = 0; j < num_vendor_stats; j++) {
					std::cout << "       Device " << vendor_stats[j].name << ": " << vendor_stats[j].value << std::endl;
					EXPECT_GE(vendor_stats[j].value, 0);
				}
			}
			delete[] vendor_stats;
		}

		// 7. Port Statistics
		if (verbose) {
			std::cout << "\n7. Port Statistics:" << std::endl;
		}
		uint32_t num_port_stats = 0;
		ASSERT_EQ(amdsmi_get_nic_port_statistics(nic_processors[i], 0, &num_port_stats, nullptr), AMDSMI_STATUS_SUCCESS);
		if (num_port_stats > 0) {
			amdsmi_nic_stat_t* port_stats = new amdsmi_nic_stat_t[num_port_stats];
			ASSERT_EQ(amdsmi_get_nic_port_statistics(nic_processors[i], 0, &num_port_stats, port_stats), AMDSMI_STATUS_SUCCESS);
			if (verbose) {
				for (uint32_t j = 0; j < num_port_stats; j++) {
					std::cout << "       Port 0 " << port_stats[j].name << ": " << port_stats[j].value << std::endl;
					EXPECT_GE(port_stats[j].value, 0);
				}
			}
			delete[] port_stats;
		}

		// 8. RDMA Device Information
		if (verbose) {
			std::cout << "\n8. RDMA Device Information:" << std::endl;
		}
		amdsmi_nic_rdma_devices_info_t rdma_info;
		amdsmi_status_t rdma_ret = amdsmi_get_nic_rdma_dev_info(nic_processors[i], &rdma_info);

		if (rdma_ret == AMDSMI_STATUS_SUCCESS) {
			if (verbose) {
				std::cout << "       Number of RDMA devices: " << (int)rdma_info.num_rdma_dev << std::endl;
			}
			EXPECT_LE(rdma_info.num_rdma_dev, AMDSMI_MAX_NIC_RDMA_DEV);

			for (uint8_t j = 0; j < rdma_info.num_rdma_dev && j < AMDSMI_MAX_NIC_RDMA_DEV; j++) {
				const auto& dev_info = rdma_info.rdma_dev_info[j];
				if (verbose) {
					std::cout << "       RDMA Device " << (int)j << ":" << std::endl;
					std::cout << "          Device: " << dev_info.rdma_dev << std::endl;
					std::cout << "          Node GUID: " << dev_info.node_guid << std::endl;
					std::cout << "          Node Type: " << dev_info.node_type << std::endl;
					std::cout << "          System Image GUID: " << dev_info.sys_image_guid << std::endl;
					std::cout << "          FW Version: " << dev_info.fw_ver << std::endl;
					std::cout << "          Number of RDMA ports: " << (int)dev_info.num_rdma_ports << std::endl;
				}

				EXPECT_GT(strlen(dev_info.rdma_dev), 0);
				EXPECT_LE(dev_info.num_rdma_ports, AMDSMI_MAX_NIC_PORTS);

				for (uint8_t k = 0; k < dev_info.num_rdma_ports && k < AMDSMI_MAX_NIC_PORTS; k++) {
					const auto& rdma_port_info = dev_info.rdma_port_info[k];
					if (verbose) {
						std::cout << "              Port " << (int)k << ":" << std::endl;
						std::cout << "                  Netdev: " << rdma_port_info.netdev << std::endl;
						std::cout << "                  State: " << rdma_port_info.state << std::endl;
						std::cout << "                  Max MTU: " << rdma_port_info.max_mtu << std::endl;
						std::cout << "                  Active MTU: " << rdma_port_info.active_mtu << std::endl;
						std::cout << "                  RDMA Port: " << (int)rdma_port_info.rdma_port << std::endl;
					}
				}
			}
		} else {
			if (verbose) {
				std::cout << "       RDMA info not available (status: " << rdma_ret << ")" << std::endl;
			}
			EXPECT_TRUE(rdma_ret == AMDSMI_STATUS_NOT_SUPPORTED || rdma_ret == AMDSMI_STATUS_NOT_FOUND || rdma_ret == AMDSMI_STATUS_DRIVER_NOT_LOADED);
		}

		// 9. RDMA Port Statistics
		if (verbose) {
			std::cout << "\n9. RDMA Port Statistics:" << std::endl;
		}
		uint32_t num_rdma_stats = 0;
		ASSERT_EQ(amdsmi_get_nic_rdma_port_statistics(nic_processors[i], 0, &num_rdma_stats, nullptr), AMDSMI_STATUS_SUCCESS);
		if (num_rdma_stats > 0) {
			amdsmi_nic_stat_t* rdma_stats = new amdsmi_nic_stat_t[num_rdma_stats];
			ASSERT_EQ(amdsmi_get_nic_rdma_port_statistics(nic_processors[i], 0, &num_rdma_stats, rdma_stats), AMDSMI_STATUS_SUCCESS);
			if (verbose) {
				for (uint32_t j = 0; j < num_rdma_stats; j++) {
					std::cout << "       RDMA Port 0 " << rdma_stats[j].name << ": " << rdma_stats[j].value << std::endl;
					EXPECT_GE(rdma_stats[j].value, 0);
				}
			}
			delete[] rdma_stats;
		}

		// 10. BDF Information
		if (verbose) {
			std::cout << "\n10. BDF Information:" << std::endl;
		}
		amdsmi_bdf_t bdf;
		ASSERT_EQ(amdsmi_get_nic_device_bdf(nic_processors[i], &bdf), AMDSMI_STATUS_SUCCESS);
		if (verbose) {
			std::cout << "  BDF:" << std::endl;
			std::cout << "       Domain: 0x" << std::hex << bdf.bdf.domain_number << std::dec << std::endl;
			std::cout << "       Bus: 0x" << std::hex << bdf.bdf.bus_number << std::dec << std::endl;
			std::cout << "       Device: 0x" << std::hex << bdf.bdf.device_number << std::dec << std::endl;
			std::cout << "       Function: 0x" << std::hex << bdf.bdf.function_number << std::dec << std::endl;
		}

		amdsmi_processor_handle handle;
		ASSERT_EQ(amdsmi_get_processor_handle_from_bdf(bdf, &handle), AMDSMI_STATUS_SUCCESS);
		ASSERT_EQ(handle, nic_processors[i]);
	}
}

TEST_F(AmdSmiNicIntegrationTests, NicWalkthroughTest)
{
	nic_walkthrough_test(nic_processors_.get(), nic_count_, true);
}

#if THREAD_SAFE
TEST_F(AmdSmiNicIntegrationTests, NicWalkthroughTestMultithreaded)
{
	std::thread t0(nic_walkthrough_test, nic_processors_.get(), nic_count_, false);
	std::thread t1(nic_walkthrough_test, nic_processors_.get(), nic_count_, false);
	std::thread t2(nic_walkthrough_test, nic_processors_.get(), nic_count_, false);
	std::thread t3(nic_walkthrough_test, nic_processors_.get(), nic_count_, false);

	t0.join();
	t1.join();
	t2.join();
	t3.join();
}

TEST_F(AmdSmiNicIntegrationTests, NicMultithreadedTest_Success)
{
	std::vector<std::thread> threads;

	// All threads call amdsmi_init before API usage
	auto safe_api_thread = [](amdsmi_processor_handle *nic_processors, uint32_t nic_count) {
		amdsmi_status_t init_result = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
		EXPECT_EQ(init_result, AMDSMI_STATUS_SUCCESS);

		if (nic_count > 0) {
			amdsmi_bdf_t bdf;
			amdsmi_status_t bdf_result = amdsmi_get_nic_device_bdf(nic_processors[0], &bdf);
			EXPECT_EQ(bdf_result, AMDSMI_STATUS_SUCCESS);

			amdsmi_nic_bus_info_t bus_info;
			amdsmi_status_t bus_result = amdsmi_get_nic_bus_info(nic_processors[0], &bus_info);
			EXPECT_EQ(bus_result, AMDSMI_STATUS_SUCCESS);
		}

		amdsmi_status_t shutdown_result = amdsmi_shut_down();
		EXPECT_EQ(shutdown_result, AMDSMI_STATUS_SUCCESS);
	};

	for (int i = 0; i < 8; ++i) {
		threads.emplace_back(safe_api_thread, nic_processors_.get(), nic_count_);
	}
	for (auto& t : threads) t.join();
}

TEST_F(AmdSmiNicIntegrationTests, NicMultithreadedTest_NotInit)
{
	// First shut down the library to test not-initialized scenarios
	ASSERT_EQ(amdsmi_shut_down(), AMDSMI_STATUS_SUCCESS);

	std::vector<std::thread> threads;

	// Half threads call init, half do not
	auto mixed_api_thread = [](amdsmi_processor_handle *nic_processors, uint32_t nic_count, int thread_id) {
		bool do_init = (thread_id % 2 == 0);
		bool init_succeeded = false;

		if (do_init) {
			amdsmi_status_t init_result = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
			if (init_result == AMDSMI_STATUS_SUCCESS) {
				init_succeeded = true;
			}
		}

		if (nic_count > 0) {
			amdsmi_bdf_t bdf;
			amdsmi_status_t result = amdsmi_get_nic_device_bdf(nic_processors[0], &bdf);
			if (do_init) {
				EXPECT_EQ(result, AMDSMI_STATUS_SUCCESS);
			} else {
				EXPECT_EQ(result, AMDSMI_STATUS_NOT_INIT);
			}
		}

		if (init_succeeded) {
			amdsmi_status_t shutdown_result = amdsmi_shut_down();
			EXPECT_EQ(shutdown_result, AMDSMI_STATUS_SUCCESS);
		}
	};

	for (int i = 0; i < 8; ++i) {
		threads.emplace_back(mixed_api_thread, nic_processors_.get(), nic_count_, i);
	}
	for (auto& t : threads) t.join();

	// Reinitialize for subsequent tests in the suite
	ASSERT_EQ(amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS), AMDSMI_STATUS_SUCCESS);
}
#endif

TEST_F(AmdSmiNicIntegrationTests, NicErrorHandlingTest)
{
	amdsmi_nic_driver_info_t driver_info;
	amdsmi_nic_asic_info_t asic_info;
	amdsmi_nic_bus_info_t bus_info;
	amdsmi_nic_numa_info_t numa_info;
	amdsmi_nic_port_info_t port_info;
	amdsmi_nic_rdma_devices_info_t rdma_info;
	amdsmi_bdf_t bdf;

	ASSERT_EQ(amdsmi_get_nic_driver_info(nullptr, &driver_info), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_driver_info(nullptr, nullptr), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_asic_info(nullptr, &asic_info), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_asic_info(nullptr, nullptr), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_bus_info(nullptr, &bus_info), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_bus_info(nullptr, nullptr), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_numa_info(nullptr, &numa_info), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_numa_info(nullptr, nullptr), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_port_info(nullptr, &port_info), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_port_info(nullptr, nullptr), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_rdma_dev_info(nullptr, &rdma_info), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_rdma_dev_info(nullptr, nullptr), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_device_bdf(nullptr, &bdf), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_nic_device_bdf(nullptr, nullptr), AMDSMI_STATUS_INVAL);

	if (nic_count_ > 0) {
		amdsmi_processor_handle h = nic_processors_[0];
		ASSERT_EQ(amdsmi_get_nic_driver_info(h, nullptr), AMDSMI_STATUS_INVAL);
		ASSERT_EQ(amdsmi_get_nic_asic_info(h, nullptr), AMDSMI_STATUS_INVAL);
		ASSERT_EQ(amdsmi_get_nic_bus_info(h, nullptr), AMDSMI_STATUS_INVAL);
		ASSERT_EQ(amdsmi_get_nic_numa_info(h, nullptr), AMDSMI_STATUS_INVAL);
		ASSERT_EQ(amdsmi_get_nic_port_info(h, nullptr), AMDSMI_STATUS_INVAL);
		ASSERT_EQ(amdsmi_get_nic_rdma_dev_info(h, nullptr), AMDSMI_STATUS_INVAL);
		ASSERT_EQ(amdsmi_get_nic_device_bdf(h, nullptr), AMDSMI_STATUS_INVAL);
	}
}

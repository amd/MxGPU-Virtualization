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
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

class AmdSmiNicStubTests : public amdsmi::AmdSmiTest {
protected:
};

TEST_F(AmdSmiNicStubTests, GetNicDriverInfoStub)
{
	amdsmi_nic_driver_info_t driver_info;
	amdsmi_status_t ret;

	ret = amdsmi_get_nic_driver_info(&GPU_MOCK_HANDLE, &driver_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_driver_info(nullptr, &driver_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_driver_info(&GPU_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_driver_info(nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNicStubTests, GetNicAsicInfoStub)
{
	amdsmi_nic_asic_info_t asic_info;
	amdsmi_status_t ret;

	ret = amdsmi_get_nic_asic_info(&GPU_MOCK_HANDLE, &asic_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_asic_info(nullptr, &asic_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_asic_info(&GPU_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_asic_info(nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNicStubTests, GetNicBusInfoStub)
{
	amdsmi_nic_bus_info_t bus_info;
	amdsmi_status_t ret;

	ret = amdsmi_get_nic_bus_info(&GPU_MOCK_HANDLE, &bus_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_bus_info(nullptr, &bus_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_bus_info(&GPU_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_bus_info(nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNicStubTests, GetNicNumaInfoStub)
{
	amdsmi_nic_numa_info_t numa_info;
	amdsmi_status_t ret;

	ret = amdsmi_get_nic_numa_info(&GPU_MOCK_HANDLE, &numa_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_numa_info(nullptr, &numa_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_numa_info(&GPU_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_numa_info(nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNicStubTests, GetNicPortInfoStub)
{
	amdsmi_nic_port_info_t port_info;
	amdsmi_status_t ret;

	ret = amdsmi_get_nic_port_info(&GPU_MOCK_HANDLE, &port_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_port_info(nullptr, &port_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_port_info(&GPU_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_port_info(nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNicStubTests, GetNicRdmaDevInfoStub)
{
	amdsmi_nic_rdma_devices_info_t rdma_info;
	amdsmi_status_t ret;

	ret = amdsmi_get_nic_rdma_dev_info(&GPU_MOCK_HANDLE, &rdma_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_rdma_dev_info(nullptr, &rdma_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_rdma_dev_info(&GPU_MOCK_HANDLE, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_rdma_dev_info(nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}


TEST_F(AmdSmiNicStubTests, GetNicPortStatisticsStub)
{
	uint32_t num_stats;
	amdsmi_nic_stat_t *stats;
	amdsmi_status_t ret;
	uint32_t port_index = 0;


	ret = amdsmi_get_nic_port_statistics(&GPU_MOCK_HANDLE, port_index, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_port_statistics(nullptr, port_index, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_port_statistics(&GPU_MOCK_HANDLE, port_index, nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	stats = (amdsmi_nic_stat_t*)malloc(7 * sizeof(amdsmi_nic_stat_t));
	num_stats = 7;
	ret = amdsmi_get_nic_port_statistics(&GPU_MOCK_HANDLE, port_index, &num_stats, stats);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
	free(stats);
}

TEST_F(AmdSmiNicStubTests, GetNicVendorStatisticsStub)
{
	uint32_t num_stats;
	amdsmi_nic_stat_t *stats;
	amdsmi_status_t ret;

	ret = amdsmi_get_nic_vendor_statistics(&GPU_MOCK_HANDLE, 0, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_vendor_statistics(nullptr, 0, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_vendor_statistics(&GPU_MOCK_HANDLE, 0, nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	stats = (amdsmi_nic_stat_t*)malloc(7 * sizeof(amdsmi_nic_stat_t));
	num_stats = 7;
	ret = amdsmi_get_nic_vendor_statistics(&GPU_MOCK_HANDLE, 0, &num_stats, stats);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
	free(stats);
}


TEST_F(AmdSmiNicStubTests, GetNicRdmaPortStatisticsStub)
{
	uint32_t num_stats;
	amdsmi_nic_stat_t *stats;
	amdsmi_status_t ret;
	uint32_t rdma_port_index = 0;


	ret = amdsmi_get_nic_rdma_port_statistics(&GPU_MOCK_HANDLE, rdma_port_index, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_rdma_port_statistics(nullptr, rdma_port_index, &num_stats, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);

	ret = amdsmi_get_nic_rdma_port_statistics(&GPU_MOCK_HANDLE, rdma_port_index, nullptr, nullptr);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);


	stats = (amdsmi_nic_stat_t*)malloc(7 * sizeof(amdsmi_nic_stat_t));
	num_stats = 7;
	ret = amdsmi_get_nic_rdma_port_statistics(&GPU_MOCK_HANDLE, rdma_port_index, &num_stats, stats);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
	free(stats);
}

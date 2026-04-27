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
#include "common/smi_device_handle.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::equal_handles;
using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;


class AmdSmiNumaTests : public amdsmi::AmdSmiTest {
};

#ifdef __linux__
TEST_F(AmdSmiNumaTests, GetCpuAffinityWithScope_InvalidParams)
{
	int ret;
	uint32_t numa_node = 0;
	uint64_t cpu_set[4] = {0};

	ret = amdsmi_topo_get_numa_node_number(NULL, &numa_node);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, NULL);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_topo_get_numa_node_number(&NIC_MOCK_HANDLE, &numa_node);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_cpu_affinity_with_scope(NULL, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_cpu_affinity_with_scope(&GPU_MOCK_HANDLE, 4, NULL, AMDSMI_AFFINITY_SCOPE_NODE);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_cpu_affinity_with_scope(&NIC_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_cpu_affinity_with_scope(&GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_SOCKET);
	EXPECT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNumaTests, GetCpuAffinityWithScope_CreateSysFSFailure)
{
	int ret;
	uint64_t cpu_set[4] = {0};

	EXPECT_CALL(*g_system_mock, Snprintf(testing::_, testing::_, testing::_))
				.WillOnce(testing::Return(-1));

	ret = amdsmi_get_cpu_affinity_with_scope(&GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);
	EXPECT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiNumaTests, TopoGetNumaNodeNum_CreateSysFSFailure)
{
	int ret;
	uint32_t numa_node = 0;

	EXPECT_CALL(*g_system_mock, Snprintf(testing::_, testing::_, testing::_))
				.WillOnce(testing::Return(-1));

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, &numa_node);
	EXPECT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiNumaTests, GetCpuAffinityWithScope_FOpenFailure)
{
	int ret;
	uint64_t cpu_set[4] = {0};

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_))
				.WillOnce(testing::Return((FILE *)NULL));

	ret = amdsmi_get_cpu_affinity_with_scope(&GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);

	EXPECT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNumaTests, TopoGetNumaNodeNum_FOpenFailure)
{
	int ret;
	uint32_t numa_node = 0;

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_))
				.WillOnce(testing::Return((FILE *)NULL));

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, &numa_node);

	EXPECT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNumaTests, GetCpuAffinityWithScope_Success)
{
	int ret;
	uint64_t cpu_set[4] = {0};
	char buffer[1024];
	FILE* f1 = fmemopen(buffer, sizeof(buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_))
				.WillOnce(testing::Return(f1));

	ret = amdsmi_get_cpu_affinity_with_scope(&GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);

	EXPECT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiNumaTests, TopoGetNumaNodeNum_Success)
{
	int ret;
	uint32_t numa_node = 0;
	char buffer[1024];
	FILE* f1 = fmemopen(buffer, sizeof(buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_))
				.WillOnce(testing::Return(f1));

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, &numa_node);

	EXPECT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiNumaTests, GetCpuAffinityWithScope_FgetsFailure)
{
	int ret;
	uint64_t cpu_set[4] = {0};
	char buffer[1024];
	FILE* f1 = fmemopen(buffer, sizeof(buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_))
				.WillOnce(testing::Return(f1));
	EXPECT_CALL(*g_system_mock, Fgets(testing::_, testing::_, testing::_))
				.WillOnce(testing::Return((char *)NULL));

	ret = amdsmi_get_cpu_affinity_with_scope(&GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);

	EXPECT_EQ(ret, AMDSMI_STATUS_IO);
}

TEST_F(AmdSmiNumaTests, TopoGetNumaNodeNum_FgetsFailure)
{
	int ret;
	uint32_t numa_node = 0;
	char buffer[1024];
	FILE* f1 = fmemopen(buffer, sizeof(buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_))
				.WillOnce(testing::Return(f1));
	EXPECT_CALL(*g_system_mock, Fgets(testing::_, testing::_, testing::_))
				.WillOnce(testing::Return((char *)NULL));

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, &numa_node);

	EXPECT_EQ(ret, AMDSMI_STATUS_IO);
}

TEST_F(AmdSmiNumaTests, GetCpuAffinityWithScope_NotFound)
{
	int ret;
	uint64_t cpu_set[4] = {0};

	ret = amdsmi_get_cpu_affinity_with_scope(&GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);

	EXPECT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNumaTests, TopoGetNumaNodeNum_NotFound)
{
	int ret;
	uint32_t numa_node = 0;

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, &numa_node);

	EXPECT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}
#else
TEST_F(AmdSmiNumaTests, OtherPlatforms_NotSupported)
{
	int ret;
	uint32_t numa_node = 0;

	ret = amdsmi_topo_get_numa_node_number(NULL, &numa_node);
	EXPECT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}
#endif

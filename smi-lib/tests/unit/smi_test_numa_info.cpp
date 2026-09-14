/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

class AmdSmiNumaTests : public amdsmi::AmdSmiTest {};

#ifdef __linux__
TEST_F(AmdSmiNumaTests, GetCpuAffinityWithScope_InvalidParams)
{
	int ret;
	uint32_t numa_node  = 0;
	uint64_t cpu_set[4] = {0};

	ret = amdsmi_topo_get_numa_node_number(NULL, &numa_node);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, NULL);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_topo_get_numa_node_number(&NIC_MOCK_HANDLE, &numa_node);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_cpu_affinity_with_scope(NULL, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_cpu_affinity_with_scope(
	    &GPU_MOCK_HANDLE, 4, NULL, AMDSMI_AFFINITY_SCOPE_NODE);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_cpu_affinity_with_scope(
	    &NIC_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_cpu_affinity_with_scope(
	    &GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_SOCKET);
	EXPECT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNumaTests, GetCpuAffinityWithScope_CreateSysFSFailure)
{
	int ret;
	uint64_t cpu_set[4] = {0};

	EXPECT_CALL(*g_system_mock, Snprintf(testing::_, testing::_, testing::_))
	    .WillOnce(testing::Return(-1));

	ret = amdsmi_get_cpu_affinity_with_scope(
	    &GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);
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

	ret = amdsmi_get_cpu_affinity_with_scope(
	    &GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);

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
	uint64_t cpu_set[4]	  = {0};
	char numa_buffer[1024]	  = "0\n";
	char cpulist_buffer[1024] = "0-7\n";
	FILE *f1		  = fmemopen(numa_buffer, sizeof(numa_buffer), "r+");
	FILE *f2		  = fmemopen(cpulist_buffer, sizeof(cpulist_buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_))
	    .WillOnce(testing::Return(f1))
	    .WillOnce(testing::Return(f2));

	ret = amdsmi_get_cpu_affinity_with_scope(
	    &GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);

	EXPECT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(cpu_set[0], 0xffull);
}

TEST_F(AmdSmiNumaTests, GetCpuAffinityWithScope_NoNumaAffinity)
{
	int ret;
	uint64_t cpu_set[4] = {0};
	char buffer[1024]   = "-1\n";
	FILE *f1	    = fmemopen(buffer, sizeof(buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_)).WillOnce(testing::Return(f1));

	ret = amdsmi_get_cpu_affinity_with_scope(
	    &GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);

	EXPECT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNumaTests, TopoGetNumaNodeNum_Success)
{
	int ret;
	uint32_t numa_node = 0;
	char buffer[1024]  = "3\n";
	FILE *f1	   = fmemopen(buffer, sizeof(buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_)).WillOnce(testing::Return(f1));

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, &numa_node);

	EXPECT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	EXPECT_EQ(numa_node, 3u);
}

TEST_F(AmdSmiNumaTests, TopoGetNumaNodeNum_NoNumaAffinity)
{
	int ret;
	uint32_t numa_node = 0xdeadbeef;
	char buffer[1024]  = "-1\n";
	FILE *f1	   = fmemopen(buffer, sizeof(buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_)).WillOnce(testing::Return(f1));

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, &numa_node);

	EXPECT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
	EXPECT_EQ(numa_node, 0xdeadbeefu); // output left untouched
}

TEST_F(AmdSmiNumaTests, TopoGetNumaNodeNum_NonNumericContent)
{
	int ret;
	uint32_t numa_node = 0;
	char buffer[1024]  = "not-a-number\n";
	FILE *f1	   = fmemopen(buffer, sizeof(buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_)).WillOnce(testing::Return(f1));

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, &numa_node);

	EXPECT_EQ(ret, AMDSMI_STATUS_IO);
}

TEST_F(AmdSmiNumaTests, TopoGetNumaNodeNum_OutOfRangeContent)
{
	int ret;
	uint32_t numa_node = 0;
	char buffer[1024]  = "99999999999999\n";
	FILE *f1	   = fmemopen(buffer, sizeof(buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_)).WillOnce(testing::Return(f1));

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, &numa_node);

	EXPECT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiNumaTests, GetCpuAffinityWithScope_FgetsFailure)
{
	int ret;
	uint64_t cpu_set[4] = {0};
	char buffer[1024];
	FILE *f1 = fmemopen(buffer, sizeof(buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_)).WillOnce(testing::Return(f1));
	EXPECT_CALL(*g_system_mock, Fgets(testing::_, testing::_, testing::_))
	    .WillOnce(testing::Return((char *)NULL));

	ret = amdsmi_get_cpu_affinity_with_scope(
	    &GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);

	EXPECT_EQ(ret, AMDSMI_STATUS_IO);
}

TEST_F(AmdSmiNumaTests, TopoGetNumaNodeNum_FgetsFailure)
{
	int ret;
	uint32_t numa_node = 0;
	char buffer[1024];
	FILE *f1 = fmemopen(buffer, sizeof(buffer), "r+");

	EXPECT_CALL(*g_system_mock, Fopen(testing::_, testing::_)).WillOnce(testing::Return(f1));
	EXPECT_CALL(*g_system_mock, Fgets(testing::_, testing::_, testing::_))
	    .WillOnce(testing::Return((char *)NULL));

	ret = amdsmi_topo_get_numa_node_number(&GPU_MOCK_HANDLE, &numa_node);

	EXPECT_EQ(ret, AMDSMI_STATUS_IO);
}

TEST_F(AmdSmiNumaTests, GetCpuAffinityWithScope_NotFound)
{
	int ret;
	uint64_t cpu_set[4] = {0};

	ret = amdsmi_get_cpu_affinity_with_scope(
	    &GPU_MOCK_HANDLE, 4, cpu_set, AMDSMI_AFFINITY_SCOPE_NODE);

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

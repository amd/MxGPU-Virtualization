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

class AmdSmiFabricInfoTests : public amdsmi::AmdSmiTest {
public:
protected:
	::testing::AssertionResult equal_fabric_info_v1(smi_fabric_info_v1 expect,
						amdsmi_fabric_info_v1_t actual)
	{
		SMI_ASSERT_EQ(expect.accelerator_id, actual.accelerator_id);
		SMI_ASSERT_EQ(expect.fabric_type, (smi_fabric_type)actual.fabric_type);
		SMI_ASSERT_EQ(expect.bandwidth, actual.bandwidth);
		SMI_ASSERT_EQ(expect.latency, actual.latency);
		SMI_ASSERT_EQ(expect.ppod_size, actual.ppod_size);
		SMI_ASSERT_EQ(expect.vpod_id, actual.vpod_id);
		SMI_ASSERT_EQ(expect.vpod_size, actual.vpod_size);
		SMI_ASSERT_EQ(expect.addr_mode, (smi_fabric_npa_address_mode)actual.addr_mode);
		SMI_ASSERT_EQ(expect.accel_state, (smi_fabric_accelerator_vpod_state)actual.accel_state);

		// Check ppod_id (128-bit UUID)
		for (uint32_t i = 0; i < SMI_FABRIC_PPOD_ID_SIZE; i++) {
			SMI_ASSERT_EQ(expect.ppod_id[i], actual.ppod_id[i])
				<< " for ppod_id byte index " << i;
		}

		// Check active accelerators bitmap
		for (uint32_t i = 0; i < SMI_FABRIC_ACTIVE_ACCELERATORS_BITMAP_SIZE; i++) {
			SMI_ASSERT_EQ(expect.vpod_active_accelerators[i],
				      actual.vpod_active_accelerators[i])
				<< " for active accelerator index " << i;
		}

		// Check local accelerators (raw pass-through from driver).
		for (uint32_t i = 0; i < SMI_FABRIC_MAX_LOCAL_GPUS; i++) {
			SMI_ASSERT_EQ(expect.local_accelerators[i],
				      actual.local_accelerators[i])
				<< " for local accelerator index " << i;
		}

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_fabric_info_ver(smi_fabric_info_ver expect,
						amdsmi_fabric_info_ver_t actual)
	{
		SMI_ASSERT_EQ(expect.version, actual.version);

		if (AMDSMI_FABRIC_VERSION_MAJOR(expect.version) == 1) {
			return equal_fabric_info_v1(expect.fabric_info.v1, actual.fabric_info.v1);
		}

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_fabric_info(smi_fabric_info_ver expect,
						amdsmi_fabric_info_t actual)
	{
		// Check BDF
		SMI_ASSERT_EQ(GPU_MOCK_HANDLE.bdf.as_uint, actual.bdf.as_uint);

		// Check fabric info version structure
		return equal_fabric_info_ver(expect, actual.info);
	}
};

TEST_F(AmdSmiFabricInfoTests, InvalidParams)
{
	int ret;
	amdsmi_fabric_info_t fabric_info;

	ret = amdsmi_get_gpu_fabric_info(NULL, &fabric_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_fabric_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_fabric_info(&NIC_MOCK_HANDLE, &fabric_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_fabric_info(NULL, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiFabricInfoTests, IoctlFailed)
{
	int ret;
	amdsmi_fabric_info_t fabric_info;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_fabric_info(&GPU_MOCK_HANDLE, &fabric_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiFabricInfoTests, GetFabricInfoSuccess)
{
	int ret;
	smi_device_info in_payload;
	smi_fabric_info_ver fabric_info_mock = {};

	// Setup mock fabric info v1 (packed UAL version: major=1, minor=2)
	fabric_info_mock.version = (1u << 16) | 2u;
	fabric_info_mock.fabric_info.v1.accelerator_id = 42;
	fabric_info_mock.fabric_info.v1.fabric_type = SMI_FABRIC_TYPE_UALINK;
	fabric_info_mock.fabric_info.v1.bandwidth = 100000;
	fabric_info_mock.fabric_info.v1.latency = 500;
	fabric_info_mock.fabric_info.v1.ppod_size = 8;
	fabric_info_mock.fabric_info.v1.vpod_id = 678;  // vpod_id fits 10-bit limit (0-1023)
	fabric_info_mock.fabric_info.v1.vpod_size = 4;
	fabric_info_mock.fabric_info.v1.addr_mode = SMI_FABRIC_NPA_ADDRESS_MODE_SOURCE_IDENTIFICATION;
	fabric_info_mock.fabric_info.v1.accel_state = SMI_FABRIC_ACCELERATOR_VPOD_STATE_ACTIVE;

	// Set up a sample 128-bit UUID for ppod_id
	for (uint32_t i = 0; i < SMI_FABRIC_PPOD_ID_SIZE; i++) {
		fabric_info_mock.fabric_info.v1.ppod_id[i] = static_cast<uint8_t>(i + 1);
	}

	// Set up active accelerators bitmap
	for (uint32_t i = 0; i < SMI_FABRIC_ACTIVE_ACCELERATORS_BITMAP_SIZE; i++) {
		fabric_info_mock.fabric_info.v1.vpod_active_accelerators[i] = 0;
	}
	fabric_info_mock.fabric_info.v1.vpod_active_accelerators[0] = 0x0F; // First 4 accelerators active

	// currently reported as not supported
	for (uint32_t i = 0; i < SMI_FABRIC_MAX_LOCAL_GPUS; i++) {
		fabric_info_mock.fabric_info.v1.local_accelerators[i] = 0xFFFFFFFFu;
	}

	amdsmi_fabric_info_t fabric_info;
	WhenCalling(std::bind(amdsmi_get_gpu_fabric_info, &GPU_MOCK_HANDLE, &fabric_info));
	ExpectCommand(SMI_CMD_CODE_GET_FABRIC_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&fabric_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_fabric_info(fabric_info_mock, fabric_info));
}

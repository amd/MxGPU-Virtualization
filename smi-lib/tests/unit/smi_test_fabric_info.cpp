/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "gtest/gtest.h"

#include <cstring>

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
		SMI_ASSERT_EQ(expect.fabric_type, (smi_fabric_type)actual.fabric_type);
		SMI_ASSERT_EQ(expect.accel_state,
			      (smi_fabric_accelerator_vpod_state)actual.accel_state);

		SMI_ASSERT_EQ(expect.ppod.accelerator_id, actual.ppod.accelerator_id);
		SMI_ASSERT_EQ(expect.ppod.bandwidth, actual.ppod.bandwidth);
		SMI_ASSERT_EQ(expect.ppod.latency, actual.ppod.latency);
		SMI_ASSERT_EQ(expect.ppod.ppod_size, actual.ppod.ppod_size);
		SMI_ASSERT_EQ(expect.ppod.local_accelerator_count,
			      actual.ppod.local_accelerator_count);

		SMI_ASSERT_EQ(expect.vpod.vpod_id, actual.vpod.vpod_id);
		SMI_ASSERT_EQ(expect.vpod.vpod_size, actual.vpod.vpod_size);
		SMI_ASSERT_EQ(expect.vpod.addr_mode,
			      (smi_fabric_npa_address_mode)actual.vpod.addr_mode);

		SMI_ASSERT_EQ(expect.station.station_flags, actual.station.station_flags);
		SMI_ASSERT_EQ(expect.station.num_stations, actual.station.num_stations);

		// Check ppod_id (128-bit UUID)
		for (uint32_t i = 0; i < SMI_FABRIC_PPOD_ID_SIZE; i++) {
			SMI_ASSERT_EQ(expect.ppod.ppod_id[i], actual.ppod.ppod_id[i])
			    << " for ppod_id byte index " << i;
		}

		// Check active accelerators bitmap
		for (uint32_t i = 0; i < SMI_FABRIC_ACTIVE_ACCELERATORS_BITMAP_SIZE; i++) {
			SMI_ASSERT_EQ(expect.vpod.vpod_active_accelerators[i],
				      actual.vpod.vpod_active_accelerators[i])
			    << " for active accelerator index " << i;
		}

		// Check local accelerators (unsupported slots use 0xFFFFFFFF sentinel).
		for (uint32_t i = 0; i < SMI_FABRIC_MAX_LOCAL_GPUS; i++) {
			SMI_ASSERT_EQ(expect.ppod.local_accelerators[i],
				      actual.ppod.local_accelerators[i])
			    << " for local accelerator index " << i;
		}

		// Check DF/station lane enable bitmap
		for (uint32_t i = 0; i < SMI_FABRIC_MAX_BITMAP_SIZE; i++) {
			SMI_ASSERT_EQ(expect.station.lane_en_bitmap[i],
				      actual.station.lane_en_bitmap[i])
			    << " for lane_en_bitmap byte index " << i;
		}

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_fabric_info(smi_fabric_info_ver expect,
						     amdsmi_fabric_info_t actual)
	{
		// Check BDF
		SMI_ASSERT_EQ(GPU_MOCK_HANDLE.bdf.as_uint, actual.bdf.as_uint);
		SMI_ASSERT_EQ(expect.version, actual.fabric_version);

		if (AMDSMI_FABRIC_VERSION_MAJOR(expect.version) == 1) {
			return equal_fabric_info_v1(expect.fabric_info.v1, actual.fabric_info.v1);
		}

		return ::testing::AssertionSuccess();
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
	fabric_info_mock.version		    = (1u << 16) | 2u;
	fabric_info_mock.fabric_info.v1.fabric_type = SMI_FABRIC_TYPE_UALINK;
	fabric_info_mock.fabric_info.v1.accel_state = SMI_FABRIC_ACCELERATOR_VPOD_STATE_ACTIVE;

	fabric_info_mock.fabric_info.v1.ppod.accelerator_id	     = 42;
	fabric_info_mock.fabric_info.v1.ppod.bandwidth		     = 100000;
	fabric_info_mock.fabric_info.v1.ppod.latency		     = 500;
	fabric_info_mock.fabric_info.v1.ppod.ppod_size		     = 8;
	fabric_info_mock.fabric_info.v1.ppod.local_accelerator_count = 0;

	fabric_info_mock.fabric_info.v1.vpod.vpod_id   = 678; // vpod_id fits 10-bit limit (0-1023)
	fabric_info_mock.fabric_info.v1.vpod.vpod_size = 4;
	fabric_info_mock.fabric_info.v1.vpod.addr_mode =
	    SMI_FABRIC_NPA_ADDRESS_MODE_SOURCE_IDENTIFICATION;

	fabric_info_mock.fabric_info.v1.station.station_flags = 0;
	fabric_info_mock.fabric_info.v1.station.num_stations  = 0;

	// Set up a sample 128-bit UUID for ppod_id
	for (uint32_t i = 0; i < SMI_FABRIC_PPOD_ID_SIZE; i++) {
		fabric_info_mock.fabric_info.v1.ppod.ppod_id[i] = static_cast<uint8_t>(i + 1);
	}

	// Set up active accelerators bitmap
	for (uint32_t i = 0; i < SMI_FABRIC_ACTIVE_ACCELERATORS_BITMAP_SIZE; i++) {
		fabric_info_mock.fabric_info.v1.vpod.vpod_active_accelerators[i] = 0;
	}
	fabric_info_mock.fabric_info.v1.vpod.vpod_active_accelerators[0] =
	    0x0F; // First 4 accelerators active

	// currently reported as not supported
	for (uint32_t i = 0; i < SMI_FABRIC_MAX_LOCAL_GPUS; i++) {
		fabric_info_mock.fabric_info.v1.ppod.local_accelerators[i] = 0xFFFFFFFFu;
	}

	for (uint32_t i = 0; i < SMI_FABRIC_MAX_BITMAP_SIZE; i++) {
		fabric_info_mock.fabric_info.v1.station.lane_en_bitmap[i] = 0;
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

/* ------------------------------------------------------------------ */
/* amdsmi_set_gpu_fabric_ppod_config                                   */
/* ------------------------------------------------------------------ */

TEST_F(AmdSmiFabricInfoTests, SetPpodConfigInvalidParams)
{
	amdsmi_fabric_ppod_config_t ppod_cfg = {};
	ppod_cfg.version		     = AMDSMI_FABRIC_PPOD_CONFIG_V1;
	ppod_cfg.mask			     = AMDSMI_FABRIC_PPOD_FIELD_ACCEL_ID;

	ASSERT_EQ(amdsmi_set_gpu_fabric_ppod_config(NULL, &ppod_cfg), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_gpu_fabric_ppod_config(&GPU_MOCK_HANDLE, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_gpu_fabric_ppod_config(&NIC_MOCK_HANDLE, &ppod_cfg),
		  AMDSMI_STATUS_INVAL);

	/* mask == 0 must be rejected */
	amdsmi_fabric_ppod_config_t empty_cfg = {};
	empty_cfg.version		      = AMDSMI_FABRIC_PPOD_CONFIG_V1;
	ASSERT_EQ(amdsmi_set_gpu_fabric_ppod_config(&GPU_MOCK_HANDLE, &empty_cfg),
		  AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiFabricInfoTests, SetPpodConfigIoctlFailed)
{
	amdsmi_fabric_ppod_config_t ppod_cfg = {};
	ppod_cfg.version		     = AMDSMI_FABRIC_PPOD_CONFIG_V1;
	ppod_cfg.mask			     = AMDSMI_FABRIC_PPOD_FIELD_ACCEL_ID;

	WhenCalling(std::bind(amdsmi_set_gpu_fabric_ppod_config, &GPU_MOCK_HANDLE, &ppod_cfg));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_FABRIC_PPOD_CONFIG);
	int ret = performCall(-1);
	ASSERT_NE(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiFabricInfoTests, SetPpodConfigSuccess)
{
	struct smi_set_gpu_fabric_ppod_config in_payload;
	amdsmi_fabric_ppod_config_t ppod_cfg = {};
	ppod_cfg.version		     = AMDSMI_FABRIC_PPOD_CONFIG_V1;
	ppod_cfg.mask = AMDSMI_FABRIC_PPOD_FIELD_ACCEL_ID | AMDSMI_FABRIC_PPOD_FIELD_BANDWIDTH;
	ppod_cfg.data.accelerator_id = 7;
	ppod_cfg.data.bandwidth	     = 50000;

	WhenCalling(std::bind(amdsmi_set_gpu_fabric_ppod_config, &GPU_MOCK_HANDLE, &ppod_cfg));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_FABRIC_PPOD_CONFIG);
	SaveInputPayloadIn(&in_payload);
	int ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(in_payload.version, ppod_cfg.version);
	ASSERT_EQ(in_payload.mask, ppod_cfg.mask);
	ASSERT_EQ(in_payload.accelerator_id, ppod_cfg.data.accelerator_id);
	ASSERT_EQ(in_payload.bandwidth, ppod_cfg.data.bandwidth);
}

TEST_F(AmdSmiFabricInfoTests, SetPpodConfigRemainingFieldsSuccess)
{
	struct smi_set_gpu_fabric_ppod_config in_payload;
	amdsmi_fabric_ppod_config_t ppod_cfg = {};
	ppod_cfg.version		     = AMDSMI_FABRIC_PPOD_CONFIG_V1;
	ppod_cfg.mask = AMDSMI_FABRIC_PPOD_FIELD_PPOD_ID | AMDSMI_FABRIC_PPOD_FIELD_PPOD_SIZE |
			AMDSMI_FABRIC_PPOD_FIELD_LOCAL_ACCELS | AMDSMI_FABRIC_PPOD_FIELD_LATENCY;
	ppod_cfg.data.ppod_size = 16;
	ppod_cfg.data.latency	= 250;
	for (uint32_t i = 0; i < AMDSMI_FABRIC_PPOD_ID_SIZE; i++)
		ppod_cfg.data.ppod_id[i] = static_cast<uint8_t>(i + 1);
	ppod_cfg.data.local_accelerator_count = 2;
	ppod_cfg.data.local_accelerators[0]   = 10;
	ppod_cfg.data.local_accelerators[1]   = 11;

	WhenCalling(std::bind(amdsmi_set_gpu_fabric_ppod_config, &GPU_MOCK_HANDLE, &ppod_cfg));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_FABRIC_PPOD_CONFIG);
	SaveInputPayloadIn(&in_payload);
	int ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(in_payload.ppod_size, ppod_cfg.data.ppod_size);
	ASSERT_EQ(in_payload.latency, ppod_cfg.data.latency);
	ASSERT_EQ(in_payload.local_accelerator_count, ppod_cfg.data.local_accelerator_count);
	ASSERT_EQ(in_payload.local_accelerators[0], ppod_cfg.data.local_accelerators[0]);
	ASSERT_EQ(in_payload.local_accelerators[1], ppod_cfg.data.local_accelerators[1]);
	ASSERT_EQ(0, memcmp(in_payload.ppod_id, ppod_cfg.data.ppod_id, AMDSMI_FABRIC_PPOD_ID_SIZE));
}

/* ------------------------------------------------------------------ */
/* amdsmi_set_gpu_fabric_vpod_config                                   */
/* ------------------------------------------------------------------ */

TEST_F(AmdSmiFabricInfoTests, SetVpodConfigInvalidParams)
{
	amdsmi_fabric_vpod_config_t vpod_cfg = {};
	vpod_cfg.version		     = AMDSMI_FABRIC_VPOD_CONFIG_V1;
	vpod_cfg.mask			     = AMDSMI_FABRIC_VPOD_FIELD_VPOD_ID;

	ASSERT_EQ(amdsmi_set_gpu_fabric_vpod_config(NULL, &vpod_cfg), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_gpu_fabric_vpod_config(&GPU_MOCK_HANDLE, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_gpu_fabric_vpod_config(&NIC_MOCK_HANDLE, &vpod_cfg),
		  AMDSMI_STATUS_INVAL);

	amdsmi_fabric_vpod_config_t empty_cfg = {};
	empty_cfg.version		      = AMDSMI_FABRIC_VPOD_CONFIG_V1;
	ASSERT_EQ(amdsmi_set_gpu_fabric_vpod_config(&GPU_MOCK_HANDLE, &empty_cfg),
		  AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiFabricInfoTests, SetVpodConfigIoctlFailed)
{
	amdsmi_fabric_vpod_config_t vpod_cfg = {};
	vpod_cfg.version		     = AMDSMI_FABRIC_VPOD_CONFIG_V1;
	vpod_cfg.mask			     = AMDSMI_FABRIC_VPOD_FIELD_VPOD_ID;

	WhenCalling(std::bind(amdsmi_set_gpu_fabric_vpod_config, &GPU_MOCK_HANDLE, &vpod_cfg));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_FABRIC_VPOD_CONFIG);
	int ret = performCall(-1);
	ASSERT_NE(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiFabricInfoTests, SetVpodConfigSuccess)
{
	struct smi_set_gpu_fabric_vpod_config in_payload;
	amdsmi_fabric_vpod_config_t vpod_cfg = {};
	vpod_cfg.version		     = AMDSMI_FABRIC_VPOD_CONFIG_V1;
	vpod_cfg.mask = AMDSMI_FABRIC_VPOD_FIELD_VPOD_ID | AMDSMI_FABRIC_VPOD_FIELD_VPOD_SIZE;
	vpod_cfg.data.vpod_id	= 3;
	vpod_cfg.data.vpod_size = 8;

	WhenCalling(std::bind(amdsmi_set_gpu_fabric_vpod_config, &GPU_MOCK_HANDLE, &vpod_cfg));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_FABRIC_VPOD_CONFIG);
	SaveInputPayloadIn(&in_payload);
	int ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(in_payload.version, vpod_cfg.version);
	ASSERT_EQ(in_payload.mask, vpod_cfg.mask);
	ASSERT_EQ(in_payload.vpod_id, vpod_cfg.data.vpod_id);
	ASSERT_EQ(in_payload.vpod_size, vpod_cfg.data.vpod_size);
}

TEST_F(AmdSmiFabricInfoTests, SetVpodConfigRemainingFieldsSuccess)
{
	struct smi_set_gpu_fabric_vpod_config in_payload;
	amdsmi_fabric_vpod_config_t vpod_cfg = {};
	vpod_cfg.version		     = AMDSMI_FABRIC_VPOD_CONFIG_V1;
	vpod_cfg.mask =
	    AMDSMI_FABRIC_VPOD_FIELD_VPOD_ACTIVE_ACCELS | AMDSMI_FABRIC_VPOD_FIELD_ADDR_MODE;
	vpod_cfg.data.vpod_active_accelerators[0] = 0x12345678;
	vpod_cfg.data.addr_mode = AMDSMI_FABRIC_NPA_ADDRESS_MODE_SOURCE_IDENTIFICATION;

	WhenCalling(std::bind(amdsmi_set_gpu_fabric_vpod_config, &GPU_MOCK_HANDLE, &vpod_cfg));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_FABRIC_VPOD_CONFIG);
	SaveInputPayloadIn(&in_payload);
	int ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(0,
		  memcmp(in_payload.vpod_active_accelerators,
			 vpod_cfg.data.vpod_active_accelerators,
			 sizeof(in_payload.vpod_active_accelerators)));
	ASSERT_EQ(static_cast<int>(in_payload.addr_mode),
		  static_cast<int>(vpod_cfg.data.addr_mode));
}

/* ------------------------------------------------------------------ */
/* amdsmi_set_gpu_fabric_station_config                                */
/* ------------------------------------------------------------------ */

TEST_F(AmdSmiFabricInfoTests, SetStationConfigInvalidParams)
{
	amdsmi_fabric_station_config_t station_cfg = {};
	station_cfg.version			   = AMDSMI_FABRIC_STATION_CONFIG_V1;
	station_cfg.mask			   = AMDSMI_FABRIC_DF_FIELD_NUM_STATIONS;

	ASSERT_EQ(amdsmi_set_gpu_fabric_station_config(NULL, &station_cfg), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_gpu_fabric_station_config(&GPU_MOCK_HANDLE, NULL),
		  AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_gpu_fabric_station_config(&NIC_MOCK_HANDLE, &station_cfg),
		  AMDSMI_STATUS_INVAL);

	amdsmi_fabric_station_config_t empty_cfg = {};
	empty_cfg.version			 = AMDSMI_FABRIC_STATION_CONFIG_V1;
	ASSERT_EQ(amdsmi_set_gpu_fabric_station_config(&GPU_MOCK_HANDLE, &empty_cfg),
		  AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiFabricInfoTests, SetStationConfigIoctlFailed)
{
	amdsmi_fabric_station_config_t station_cfg = {};
	station_cfg.version			   = AMDSMI_FABRIC_STATION_CONFIG_V1;
	station_cfg.mask			   = AMDSMI_FABRIC_DF_FIELD_NUM_STATIONS;

	WhenCalling(
	    std::bind(amdsmi_set_gpu_fabric_station_config, &GPU_MOCK_HANDLE, &station_cfg));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_FABRIC_STATION_CONFIG);
	int ret = performCall(-1);
	ASSERT_NE(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiFabricInfoTests, SetStationConfigSuccess)
{
	struct smi_set_gpu_fabric_station_config in_payload;
	amdsmi_fabric_station_config_t station_cfg = {};
	station_cfg.version			   = AMDSMI_FABRIC_STATION_CONFIG_V1;
	station_cfg.mask =
	    AMDSMI_FABRIC_DF_FIELD_NUM_STATIONS | AMDSMI_FABRIC_DF_FIELD_STATION_FLAGS;
	station_cfg.data.num_stations  = 4;
	station_cfg.data.station_flags = 2;

	WhenCalling(
	    std::bind(amdsmi_set_gpu_fabric_station_config, &GPU_MOCK_HANDLE, &station_cfg));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_FABRIC_STATION_CONFIG);
	SaveInputPayloadIn(&in_payload);
	int ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(in_payload.version, station_cfg.version);
	ASSERT_EQ(in_payload.mask, station_cfg.mask);
	ASSERT_EQ(in_payload.num_stations, station_cfg.data.num_stations);
	ASSERT_EQ(in_payload.station_flags, station_cfg.data.station_flags);
}

TEST_F(AmdSmiFabricInfoTests, SetStationConfigLaneBitmapSuccess)
{
	struct smi_set_gpu_fabric_station_config in_payload;
	amdsmi_fabric_station_config_t station_cfg = {};
	station_cfg.version			   = AMDSMI_FABRIC_STATION_CONFIG_V1;
	station_cfg.mask			   = AMDSMI_FABRIC_DF_FIELD_LANE_EN_BITMAP;
	station_cfg.data.lane_en_bitmap[0]	   = 0xAB;
	station_cfg.data.lane_en_bitmap[1]	   = 0xCD;

	WhenCalling(
	    std::bind(amdsmi_set_gpu_fabric_station_config, &GPU_MOCK_HANDLE, &station_cfg));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_FABRIC_STATION_CONFIG);
	SaveInputPayloadIn(&in_payload);
	int ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(0,
		  memcmp(in_payload.lane_en_bitmap,
			 station_cfg.data.lane_en_bitmap,
			 sizeof(in_payload.lane_en_bitmap)));
}

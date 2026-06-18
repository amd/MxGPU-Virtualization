/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_TEST_HELPERS_HPP__
#define __SMI_TEST_HELPERS_HPP__

#include "gtest/gtest.h"

#include "smi_system_mock.hpp"

#include <functional>

#include "common/smi_device_handle.h"
#include "smi_processor_handle.h"

extern amdsmi_bdf_t MOCK_BDF;
extern const char *GPU_MOCK_UUID;
extern const char *VF_MOCK_UUID;
extern struct smi_gpu_handle GPU_MOCK_HANDLE;
extern amdsmi_vf_handle_t VF_MOCK_HANDLE;
extern struct smi_nic_handle NIC_MOCK_HANDLE;
extern struct smi_nic_handle BRCM_NIC_MOCK_HANDLE;
extern struct smi_node_handle NODE_MOCK_HANDLE;

namespace amdsmi
{
extern std::unique_ptr<NiceMock<SystemMock>> g_system_mock;

extern SystemMock *GetSystemMock();

void initialize_smi_lib();
void finalize_smi_lib();

#define SMI_ASSERT_EQ(first, second) \
	if (first != second) \
		return ::testing::AssertionFailure() \
			       << #first " vs " #second " differs - " \
			       << " expected: " << first \
			       << " actual: " << second

#define SMI_ASSERT_STR_EQ(first, second) \
	if (strcmp(first, second) != 0) \
		return ::testing::AssertionFailure() \
			       << #first " vs " #second " differs - " \
			       << " expected: " << first \
			       << " actual: " << second

#define SMI_ASSERT_MEM_EQ(first, second, size) \
	if (memcmp(first, second, size) != 0) \
		return ::testing::AssertionFailure() \
			       << #first " vs " #second " differs - " \
			       << " expected: " << first \
			       << " actual: " << second

void *mem_aligned_alloc(void **mem, size_t alignment, size_t size);

::testing::AssertionResult vf_configs_equal(smi_vf_info expect,
					    amdsmi_vf_info_t actual);

::testing::AssertionResult guard_thresholds_equal(uint32_t *expect_thresholds,
						  uint32_t *actual_thresholds);

::testing::AssertionResult equal_handles(smi_device_handle_t expect,
					 struct smi_gpu_handle actual);

::testing::AssertionResult equal_bdfs(amdsmi_bdf_t bdf_expect, amdsmi_bdf_t bdf_actual);

::testing::AssertionResult equal_dpm_policy(smi_dpm_policy expect,
					amdsmi_dpm_policy_t actual);

class AmdSmiTest : public ::testing::Test {
protected:
	AmdSmiTest() { num_devices = 1; };
	AmdSmiTest(uint8_t num_dev) { num_devices = num_dev; };
	void initialize_smi_lib(uint32_t version = SMI_VERSION_MAX, uint8_t num_dev = 1);

	void finalize_smi_lib();

	void SetUp() override
	{
		g_system_mock.reset(new NiceMock<amdsmi::SystemMock>);
		func_to_call = nullptr;
		cmd_to_expect = (uint32_t)-1;
		input_payload_ptr = nullptr;
		input_payload_size = 0;
		output_payload_ptr = nullptr;
		output_payload_size = 0;

		initialize_smi_lib(SMI_VERSION_MAX, num_devices);
	}

	void TearDown() override
	{
		finalize_smi_lib();

		g_system_mock.reset();
	}

	void WhenCalling(std::function<int()> to_call)
	{
		func_to_call = to_call;
	}

	void ExpectCommand(uint32_t cmd)
	{
		cmd_to_expect = cmd;
	}

	template <class InPayloadType>
	void SaveInputPayloadIn(InPayloadType *in_ptr)
	{
		input_payload_ptr = in_ptr;
		input_payload_size = sizeof(InPayloadType);
	}

	template <class OutPayloadType>
	void PlantMockOutput(OutPayloadType *out_ptr)
	{
		output_payload_ptr = out_ptr;
		output_payload_size = sizeof(OutPayloadType);
	}

	int performCall(int mock_result = 0)
	{
		smi_in_hdr actual_in_hdr;
		EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(cmd_to_expect)))
			.WillOnce(DoAll(amdsmi::SaveInputHeader(&actual_in_hdr),
					amdsmi::SaveInputPayloadWithSize(input_payload_ptr,
									  input_payload_size),
					amdsmi::SetPayloadWithSize(output_payload_ptr,
								    output_payload_size),
					Return(mock_result)));
		int result = func_to_call();
		if (result == AMDSMI_STATUS_SUCCESS) {
			EXPECT_EQ(actual_in_hdr.code, cmd_to_expect);
			EXPECT_EQ(actual_in_hdr.in_len, input_payload_size);
			EXPECT_EQ(actual_in_hdr.out_len, output_payload_size);
		}
		return result;
	}

	template <class InPayloadType, class OutPayloadType>
	void PrepareIoctl(uint32_t cmd, smi_in_hdr *in_header, InPayloadType *in_payload,
			  OutPayloadType out_payload, int return_code = 0)
	{
		EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(cmd)))
			.WillOnce(DoAll(amdsmi::SaveInputHeader(in_header),
					amdsmi::SaveInputPayload(in_payload),
					amdsmi::SetPayload(out_payload),
					Return(return_code)));
	}

private:
	std::function<int()> func_to_call;
	uint32_t cmd_to_expect;
	void *input_payload_ptr;
	size_t input_payload_size;
	void *output_payload_ptr;
	size_t output_payload_size;
	uint32_t handshake_version;
	uint8_t num_devices;
};

} // namespace amdsmi

#endif // __SMI_TEST_HELPERS_HPP__

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

#ifndef __SMI_TEST_HELPERS_HPP__
#define __SMI_TEST_HELPERS_HPP__

#include "gtest/gtest.h"

#include "smi_system_mock.hpp"

#include <functional>

#include "common/smi_device_handle.h"

extern amdsmi_bdf_t MOCK_BDF;
extern const char *GPU_MOCK_UUID;
extern const char *VF_MOCK_UUID;
extern smi_device_handle_t GPU_MOCK_HANDLE;
extern amdsmi_vf_handle_t VF_MOCK_HANDLE;

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
					 smi_device_handle_t actual);

::testing::AssertionResult equal_bdfs(amdsmi_bdf_t bdf_expect, amdsmi_bdf_t bdf_actual);

class AmdSmiTest : public ::testing::Test {
protected:

	void initialize_smi_lib(uint32_t version = SMI_VERSION_MAX);

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

		initialize_smi_lib();
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
};

} // namespace amdsmi

#endif // __SMI_TEST_HELPERS_HPP__

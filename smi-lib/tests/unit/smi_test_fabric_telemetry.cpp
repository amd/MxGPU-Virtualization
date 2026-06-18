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
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;
using amdsmi::equal_handles;

class AmdSmiFabricTelemetryTests : public amdsmi::AmdSmiTest {
};

TEST_F(AmdSmiFabricTelemetryTests, InvalidParams)
{
	int ret;
	amdsmi_fabric_telemetry_t* telemetry = nullptr;
	uint32_t category_mask = AMDSMI_FABRIC_TELEMETRY_CATEGORY_MASK_UALOE;

	// When UALOE is not enabled, stub implementations return NOT_SUPPORTED
	// When UALOE is enabled, invalid params should return INVAL

	// Test amdsmi_alloc_fabric_telemetry with null processor handle
	ret = amdsmi_alloc_fabric_telemetry(NULL, category_mask, &telemetry);
	ASSERT_TRUE(ret == AMDSMI_STATUS_INVAL || ret == AMDSMI_STATUS_NOT_SUPPORTED);

	// Test amdsmi_alloc_fabric_telemetry with null telemetry pointer
	ret = amdsmi_alloc_fabric_telemetry(&GPU_MOCK_HANDLE, category_mask, NULL);
	ASSERT_TRUE(ret == AMDSMI_STATUS_INVAL || ret == AMDSMI_STATUS_NOT_SUPPORTED);

	// Test amdsmi_get_fabric_telemetry_data with null processor handle
	ret = amdsmi_get_fabric_telemetry_data(NULL, telemetry);
	ASSERT_TRUE(ret == AMDSMI_STATUS_INVAL || ret == AMDSMI_STATUS_NOT_SUPPORTED);

	// Test amdsmi_get_fabric_telemetry_data with null telemetry pointer
	ret = amdsmi_get_fabric_telemetry_data(&GPU_MOCK_HANDLE, NULL);
	ASSERT_TRUE(ret == AMDSMI_STATUS_INVAL || ret == AMDSMI_STATUS_NOT_SUPPORTED);

	// Test amdsmi_free_fabric_telemetry with null processor handle
	ret = amdsmi_free_fabric_telemetry(NULL, telemetry);
	ASSERT_TRUE(ret == AMDSMI_STATUS_INVAL || ret == AMDSMI_STATUS_NOT_SUPPORTED);

	// Test amdsmi_free_fabric_telemetry with null telemetry pointer
	ret = amdsmi_free_fabric_telemetry(&GPU_MOCK_HANDLE, NULL);
	ASSERT_TRUE(ret == AMDSMI_STATUS_INVAL || ret == AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiFabricTelemetryTests, SuccessfulWorkflow)
{
	int ret;
	amdsmi_fabric_telemetry_t* telemetry = nullptr;
	uint32_t category_mask = AMDSMI_FABRIC_TELEMETRY_CATEGORY_MASK_UALOE |
							AMDSMI_FABRIC_TELEMETRY_CATEGORY_MASK_SWITCH;

	// Allocate telemetry
	ret = amdsmi_alloc_fabric_telemetry(&GPU_MOCK_HANDLE, category_mask, &telemetry);

	// Only continue if UALOE is supported (returns SUCCESS)
	if (ret == AMDSMI_STATUS_SUCCESS) {
		ASSERT_NE(telemetry, nullptr);

		// Get telemetry data
		ret = amdsmi_get_fabric_telemetry_data(&GPU_MOCK_HANDLE, telemetry);
		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);

	// Verify telemetry structure is valid
	for (unsigned int i = 0; i < AMDSMI_FABRIC_TELEMETRY_CATEGORY_MAX; i++) {
			if (telemetry->datasets[i] != nullptr) {
				// Check dataset fields are initialized
				ASSERT_GE(telemetry->datasets[i]->category, 0);
				ASSERT_LT(telemetry->datasets[i]->category, AMDSMI_FABRIC_TELEMETRY_CATEGORY_MAX);
				ASSERT_GE(telemetry->datasets[i]->generation_count, 0);
				ASSERT_GE(telemetry->datasets[i]->instance_count, 0);

				// If there are instances, validate them
				for (unsigned int j = 0; j < telemetry->datasets[i]->instance_count; j++) {
					auto& instance = telemetry->datasets[i]->instances[j];
					ASSERT_GE(instance.item_count, 0);
					// Verify telemetry items
					for (unsigned int k = 0; k < instance.item_count; k++) {
						ASSERT_GE(instance.items[k].id, 0);
					}
				}
			}
		}

		// Free telemetry
		ret = amdsmi_free_fabric_telemetry(&GPU_MOCK_HANDLE, telemetry);
		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	} else {
		// UALOE not supported - this is expected in builds without UALOE
		ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
	}
}

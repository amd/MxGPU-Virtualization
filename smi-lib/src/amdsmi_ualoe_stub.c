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

/**
 * @file amdsmi_ualoe_stub.c
 *
 * AMD SMI UALOE Stub Implementation
 *
 * This file contains stub implementations for all UALOE fabric telemetry
 * functions when UALOE is not supported in the library. All functions return
 * AMDSMI_STATUS_NOT_SUPPORTED to indicate that fabric functionality is
 * not available.
 */

#include "amdsmi.h"
#include "smi_os_defines.h"
#include <stdbool.h>

#ifdef __linux__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

/* Define stub types matching the real UALOE context */
typedef int ualoe_handle_t;
typedef int pthread_mutex_t;  /* Stub mutex type */

#define UALOE_MAX_DEVICES AMDSMI_MAX_DEVICES

/* Structure for UALOE handle with mutex protection */
typedef struct {
	ualoe_handle_t handle;
	bool initialized;
	pthread_mutex_t mutex;        /* Mutex protecting this UALOE handle */
	bool mutex_initialized;
} ualoe_handle_ctx_t;

/* Structure for UALOE fabric telemetry context */
typedef struct {
	uint32_t num_handles;
	ualoe_handle_ctx_t handles[UALOE_MAX_DEVICES];
	bool global_init;
} fabric_ualoe_context_t;

/* Stub UALOE context - always uninitialized since UALOE is not enabled */
fabric_ualoe_context_t g_fabric_ualoe_ctx = {
	.num_handles = 0,
	.handles = {{0}},
	.global_init = false
};

/**
 * @brief Stub implementation for amdsmi_alloc_fabric_telemetry
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_alloc_fabric_telemetry(amdsmi_processor_handle processor_handle,
					      uint32_t category_mask,
					      amdsmi_fabric_telemetry_t **telemetry)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)category_mask;
	(void)telemetry;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_get_fabric_telemetry_data
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_fabric_telemetry_data(amdsmi_processor_handle processor_handle,
						 amdsmi_fabric_telemetry_t *telemetry)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)telemetry;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_free_fabric_telemetry
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_free_fabric_telemetry(amdsmi_processor_handle processor_handle,
					     amdsmi_fabric_telemetry_t *telemetry)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)telemetry;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_get_fabric_cper_entries
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_fabric_cper_entries(amdsmi_processor_handle processor_handle,
					       uint32_t severity_mask, char *cper_data,
					       uint64_t *buf_size, amdsmi_cper_hdr_t **cper_hdrs,
					       uint64_t *entry_count, uint64_t *cursor)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)severity_mask;
	(void)cper_data;
	(void)buf_size;
	(void)cper_hdrs;
	(void)entry_count;
	(void)cursor;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

#ifdef __linux__
#pragma GCC diagnostic pop
#endif


/*
 * Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include <inttypes.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "smi_sys_wrapper.h"

#include "amdsmi.h"
#include "smi_utils.h"
#include "smi_defines.h"
#include "common/smi_device_handle.h"
#include "smi_debug.h"
#include "smi_os_defines.h"
#include "aca-decode/ras_decode_api.h"
#include "smi_processor_handle.h"

#ifdef AMD_SMI_NIC_SUPPORT
#include "smi_nic_interface.h"
#endif

#ifdef __linux__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

#define VENDOR_NAME "Advanced Micro Devices, Inc. [AMD/ATI]"

static struct smi_gpu_device_handles g_gpu_device_handles = { 0 };
static struct smi_nic_device_handles g_nic_device_handles = { 0 };
static struct smi_node_handles g_node_handles = { 0 };

static int amdsmi_handshake(smi_req_ctx *smi_req)
{
	struct smi_handshake *in_out;
	int code;
	in_out = (struct smi_handshake *)&smi_req->thread->ioctl_cmd.payload;
	in_out->version = SMI_VERSION_MAX; /* ask the driver what version should we support */

	code = amdsmi_request(smi_req, (uint32_t)SMI_CMD_CODE_HANDSHAKE, sizeof(struct smi_handshake),
			   sizeof(struct smi_handshake));

	if (code == AMDSMI_STATUS_SUCCESS) {
		// Early success, driver supports the version
		smi_req->handle->version = (int) in_out->version;
		smi_req->handle->init = true;
		return code;
	}

	if (in_out->version < SMI_VERSION_MIN) {
		// If the version is lesser than supported by the smi-lib,
		// try to use the minimal version
		in_out->version = SMI_VERSION_MIN;
	} else if (in_out->version > SMI_VERSION_MAX) {
		// If the version is greater than supported by the smi-lib,
		// try to use the maximal version
		in_out->version = SMI_VERSION_MAX;
	}
	// In other cases, If the version is within the supported range of the smi-lib,
	// use the version specified by the driver.
	code = amdsmi_request(smi_req, (uint32_t)SMI_CMD_CODE_HANDSHAKE, sizeof(struct smi_handshake),
			   sizeof(struct smi_handshake));

	if (code != AMDSMI_STATUS_SUCCESS) {
		// Nothing to do, driver doesn't support smi-lib API version
		SMI_ERROR("Driver doesn't support smi-lib API version. Return code: %d", code);
		return code;
	}

	smi_req->handle->version = (int) in_out->version;
	smi_req->handle->init = true;
	return AMDSMI_STATUS_SUCCESS;
}

static amdsmi_status_t smi_get_gpu_handles(amdsmi_processor_handle *gpu_handles, uint32_t *gpu_count)
{
	if (gpu_count == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	if (gpu_handles == NULL) {
		*gpu_count = g_gpu_device_handles.num_gpus;
		return AMDSMI_STATUS_SUCCESS;
	}

	if (g_gpu_device_handles.num_gpus == 0) {
		return AMDSMI_STATUS_NO_DATA;
	}

	*gpu_count = *gpu_count >= g_gpu_device_handles.num_gpus ? g_gpu_device_handles.num_gpus : *gpu_count;

	memset(gpu_handles, 0, (*gpu_count)*sizeof(amdsmi_processor_handle));
	for (uint32_t i = 0; i < *gpu_count; i++) {
		gpu_handles[i] = &g_gpu_device_handles.gpus[i];
	}

	return AMDSMI_STATUS_SUCCESS;
}

#ifdef AMD_SMI_NIC_SUPPORT
static int smi_get_nic_devices(smi_req_ctx *smi_req)
{
	smi_nic_discovery_t discovery = {0};
	uint64_t domain_number, bus_number, device_number, function_number = 0;
	int ret = AMDSMI_STATUS_SUCCESS;

	ret = smi_discover_nics(smi_req->thread->nic_ctx, &discovery);
	if (ret == SMI_NIC_STATUS_SUCCESS) {
		g_nic_device_handles.num_nics = discovery.count;
		for (uint32_t i = 0; i < discovery.count; i++) {
			g_nic_device_handles.nics[i].type = SMI_HANDLE_TYPE_AMD_NIC;
			sscanf(discovery.devices[i].bdf, "%04lx:%02lx:%02lx.%1lx",
					&domain_number,
					&bus_number,
					&device_number,
					&function_number);
			g_nic_device_handles.nics[i].bdf.as_uint = (domain_number << 16) | (bus_number << 8) | (device_number << 3) | function_number;
		}
	} else {
		g_nic_device_handles.num_nics = 0;
		return AMDSMI_STATUS_API_FAILED;
	}

	return AMDSMI_STATUS_SUCCESS;
}

static amdsmi_status_t smi_get_nic_handles(amdsmi_processor_handle *nic_handles, uint32_t *nic_count)
{
	if (nic_count == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	if (nic_handles == NULL) {
		*nic_count = g_nic_device_handles.num_nics;
		return AMDSMI_STATUS_SUCCESS;
	}

	if (g_nic_device_handles.num_nics == 0) {
		return AMDSMI_STATUS_NOT_FOUND;
	}

	*nic_count = *nic_count >= g_nic_device_handles.num_nics ? g_nic_device_handles.num_nics : *nic_count;

	memset(nic_handles, 0, (*nic_count)*sizeof(amdsmi_processor_handle));
	for (uint32_t i = 0; i < *nic_count; i++) {
		nic_handles[i] = &g_nic_device_handles.nics[i];
	}

	return AMDSMI_STATUS_SUCCESS;
}
#endif

static int get_available_devices(smi_req_ctx *smi_req)
{
	struct smi_server_static_info *info = NULL;

	int code = amdsmi_request(smi_req, (uint32_t)SMI_CMD_CODE_GET_SERVER_STATIC_INFO, 0, sizeof(struct smi_server_static_info));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return AMDSMI_STATUS_API_FAILED;
	}

	info = (struct smi_server_static_info *)&smi_req->thread->ioctl_cmd.payload;

	g_gpu_device_handles.num_gpus = info->num_devices;

	for (uint8_t i = 0; i < info->num_devices; i++) {
		g_gpu_device_handles.gpus[i].type = SMI_HANDLE_TYPE_AMD_GPU;
		g_gpu_device_handles.gpus[i].bdf.as_uint = info->devices[i].bdf.as_uint;
		g_gpu_device_handles.gpus[i].handle = info->devices[i].dev_id.handle;
		g_gpu_device_handles.gpus[i].dev_id = info->devices[i].dev_id.device_id;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_init(uint64_t init_flags)
{
	AMDSMI_UNUSED(init_flags);
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = AMDSMI_STATUS_SUCCESS;

	AMDSMI_GET_HANDLE;
	if (smi_req.handle->init) {
		AMDSMI_HANDLE_UNLOCK;
		return AMDSMI_STATUS_SUCCESS;
	}

	if (sys_wrapper->smi_access() == -1) {
		SMI_ERROR("Host driver not loaded. Return code: %d", AMDSMI_STATUS_DRIVER_NOT_LOADED);
		AMDSMI_HANDLE_UNLOCK_AND_FREE;
		return AMDSMI_STATUS_DRIVER_NOT_LOADED;
	}

	smi_req.handle->fd = sys_wrapper->smi_open(SMI_RDWR);
	if (smi_req.handle->fd == SMI_INVAL_HANDLE) {
		// fallback to not privileged
		smi_req.handle->fd = sys_wrapper->smi_open(SMI_READONLY);
		if (smi_req.handle->fd == SMI_INVAL_HANDLE) {
			if (SMI_LAST_ERROR == SMI_ACCESS_DENIED) {
				SMI_ERROR("Insufficient permission to access processor. Return code: %d", AMDSMI_STATUS_NO_PERM);
				AMDSMI_HANDLE_UNLOCK_AND_FREE;
				return AMDSMI_STATUS_NO_PERM;
			}
			AMDSMI_HANDLE_UNLOCK_AND_FREE;
			SMI_ERROR("SMI request handle fd is invalid, equals to SMI_INVAL_HANDLE. Return code: %d", AMDSMI_STATUS_API_FAILED);
			return AMDSMI_STATUS_API_FAILED;
		}
	}
	const int handshake_code = amdsmi_handshake(&smi_req);
	if (handshake_code != AMDSMI_STATUS_SUCCESS) {
		sys_wrapper->smi_close(smi_req.handle->fd);
		SMI_ERROR("Handshake request failed. Return code: %d", handshake_code);
		AMDSMI_HANDLE_UNLOCK_AND_FREE;
		return handshake_code;
	}
	ret = get_available_devices(&smi_req);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		sys_wrapper->smi_close(smi_req.handle->fd);
		smi_req.handle->init = false;
		AMDSMI_HANDLE_UNLOCK_AND_FREE;
		return ret;
	}

#ifdef AMD_SMI_NIC_SUPPORT
	if (!smi_req.thread->nic_init) {
		ret = smi_nic_create_context(&smi_req.thread->nic_ctx);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			smi_req.thread->nic_ctx = NULL;
			smi_req.thread->nic_init = false;
		} else {
			smi_req.thread->nic_init = true;
		}
	}

	ret = smi_get_nic_devices(&smi_req);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		SMI_DEBUG("NIC device discovery failed, continuing without NIC support. Return code: %d", ret);
	}
#endif

	AMDSMI_HANDLE_SET;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_shut_down(void)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	system_wrapper *sys_wrapper = get_system_wrapper();

	AMDSMI_ESCAPE_IF_NOT_INIT_ON_FINI;

#ifdef AMD_SMI_NIC_SUPPORT
	if (smi_req.thread && smi_req.thread->nic_init && smi_req.thread->nic_ctx) {
		smi_nic_destroy_context(smi_req.thread->nic_ctx);
		smi_req.thread->nic_ctx = NULL;
		smi_req.thread->nic_init = false;
	}
#endif

	if ((int)(intptr_t)smi_req.handle->fd != (int)(intptr_t)SMI_INVAL_HANDLE) {
		if (sys_wrapper->smi_close(smi_req.handle->fd) == -1) {
			SMI_ERROR("Couldn't close the fd. Return code: %d", AMDSMI_STATUS_IO);
			AMDSMI_HANDLE_UNLOCK;
			return AMDSMI_STATUS_IO;
		}
		smi_req.handle->fd = SMI_INVAL_HANDLE;
	}
	smi_req.handle->init = false;

#ifdef THREAD_SAFE
	if (smi_req.thread) {
		free(smi_req.thread);
		smi_tss_set(smi_thread_key, NULL);
	}
#endif

	AMDSMI_HANDLE_UNLOCK;
	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_socket_handles(uint32_t *socket_count, amdsmi_socket_handle *socket_handles)
{
	#pragma SMI_EXPORT
	AMDSMI_UNUSED(socket_count);
	AMDSMI_UNUSED(socket_handles);
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

amdsmi_status_t amdsmi_get_socket_info(amdsmi_socket_handle socket_handle, size_t len, char *name)
{
	#pragma SMI_EXPORT
	AMDSMI_UNUSED(socket_handle);
	AMDSMI_UNUSED(name);
	AMDSMI_UNUSED(len);
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

amdsmi_status_t amdsmi_get_processor_handles(amdsmi_socket_handle socket_handle, uint32_t *processor_count, amdsmi_processor_handle *processor_handles)
{
	#pragma SMI_EXPORT
	AMDSMI_UNUSED(socket_handle);
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_count == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	if (processor_handles == NULL) {
		*processor_count = g_gpu_device_handles.num_gpus;
		return AMDSMI_STATUS_SUCCESS;
	}

	*processor_count = *processor_count >= g_gpu_device_handles.num_gpus ? g_gpu_device_handles.num_gpus : *processor_count;

	memset(processor_handles, 0, (*processor_count)*sizeof(amdsmi_processor_handle));
	for (uint32_t i = 0; i < *processor_count; i++) {
		processor_handles[i] = &g_gpu_device_handles.gpus[i];
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_processor_type(amdsmi_processor_handle processor_handle, processor_type_t *processor_type)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	enum smi_handle_type type;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || processor_type == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	switch (type) {
	case SMI_HANDLE_TYPE_AMD_GPU:
		*processor_type = AMDSMI_PROCESSOR_TYPE_AMD_GPU;
		break;
	case SMI_HANDLE_TYPE_AMD_NIC:
		*processor_type = AMDSMI_PROCESSOR_TYPE_AMD_NIC;
		break;
	default:
		*processor_type = AMDSMI_PROCESSOR_TYPE_UNKNOWN;
		break;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_processor_handles_by_type(amdsmi_socket_handle socket_handle, processor_type_t processor_type,
						     amdsmi_processor_handle *processor_handles,
						     uint32_t *processor_count)
{
	#pragma SMI_EXPORT
	AMDSMI_UNUSED(socket_handle);
	smi_req_ctx smi_req;
	int ret = AMDSMI_STATUS_API_FAILED;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	switch (processor_type) {
	case AMDSMI_PROCESSOR_TYPE_AMD_GPU:
		return smi_get_gpu_handles(processor_handles, processor_count);
	case AMDSMI_PROCESSOR_TYPE_AMD_NIC:
#ifdef AMD_SMI_NIC_SUPPORT
		return smi_get_nic_handles(processor_handles, processor_count);
#else
		return AMDSMI_STATUS_NOT_SUPPORTED;
#endif
	case AMDSMI_PROCESSOR_TYPE_AMD_CPU:
	case AMDSMI_PROCESSOR_TYPE_NON_AMD_GPU:
	case AMDSMI_PROCESSOR_TYPE_NON_AMD_CPU:
	case AMDSMI_PROCESSOR_TYPE_AMD_CPU_CORE:
	case AMDSMI_PROCESSOR_TYPE_AMD_APU:
		return AMDSMI_STATUS_NOT_SUPPORTED;
	case AMDSMI_PROCESSOR_TYPE_UNKNOWN:
		return AMDSMI_STATUS_INVAL;
	}

	return ret;
}

amdsmi_status_t amdsmi_get_processor_handle_from_bdf(amdsmi_bdf_t bdf, amdsmi_processor_handle *processor_handle)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	memset(processor_handle, 0, sizeof(amdsmi_processor_handle));
	for (uint32_t i = 0; i < g_gpu_device_handles.num_gpus; i++) {
		amdsmi_bdf_t temp_bdf;
		int ret = amdsmi_get_gpu_device_bdf(&g_gpu_device_handles.gpus[i], &temp_bdf);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			return ret;
		}
		if (temp_bdf.as_uint == bdf.as_uint) {
			*processor_handle = &g_gpu_device_handles.gpus[i];
			return AMDSMI_STATUS_SUCCESS;
		}
	}

	for (uint32_t i = 0; i < g_nic_device_handles.num_nics; i++) {
		amdsmi_bdf_t temp_bdf;
		int ret = amdsmi_get_nic_device_bdf(&g_nic_device_handles.nics[i], &temp_bdf);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			return ret;
		}
		if (temp_bdf.as_uint == bdf.as_uint) {
			*processor_handle = &g_nic_device_handles.nics[i];
			return AMDSMI_STATUS_SUCCESS;
		}
	}

	return AMDSMI_STATUS_NOT_FOUND;
}

amdsmi_status_t amdsmi_get_index_from_processor_handle(amdsmi_processor_handle processor_handle, uint32_t *processor_index)
{
	#pragma SMI_EXPORT
	struct smi_gpu_handle *gpu = NULL;
	struct smi_nic_handle *nic = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_index == NULL || processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type == SMI_HANDLE_TYPE_AMD_GPU) {
		for (uint32_t i = 0; i < g_gpu_device_handles.num_gpus; i++) {
			gpu = (struct smi_gpu_handle *)processor_handle;
			if (gpu->handle == g_gpu_device_handles.gpus[i].handle) {
				*processor_index = i;
				return AMDSMI_STATUS_SUCCESS;
			}
		}
		return AMDSMI_STATUS_NOT_FOUND;
	} else if (type == SMI_HANDLE_TYPE_AMD_NIC) {
		for (uint32_t i = 0; i < g_nic_device_handles.num_nics; i++) {
			nic = ((struct smi_nic_handle *)processor_handle);
			if (nic->bdf.as_uint == g_nic_device_handles.nics[i].bdf.as_uint) {
				*processor_index = i;
				return AMDSMI_STATUS_SUCCESS;
			}
		}
		return AMDSMI_STATUS_NOT_FOUND;
	} else {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}
}

amdsmi_status_t amdsmi_get_processor_handle_from_index(uint32_t processor_index, amdsmi_processor_handle *processor_handle)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	if (!(processor_index < g_gpu_device_handles.num_gpus)) {
		SMI_ERROR("Invalid argument value(s) for index passed. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	memset(processor_handle, 0, sizeof(amdsmi_processor_handle));
	*processor_handle = &g_gpu_device_handles.gpus[processor_index];
	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_vf_handle_from_bdf(amdsmi_bdf_t bdf, amdsmi_vf_handle_t *vf_handle)
{
	#pragma SMI_EXPORT
	struct smi_get_handle_info *handle_info;
	struct smi_get_handle_resp *handle_resp;
	smi_req_ctx smi_req;

	if (vf_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	AMDSMI_ESCAPE_IF_NOT_INIT;

	handle_info = (struct smi_get_handle_info *)&smi_req.thread->ioctl_cmd.payload;
	handle_info->bdf.as_uint = (uint32_t)bdf.as_uint;

	const int code =
		amdsmi_request(&smi_req, SMI_CMD_CODE_GET_HANDLE, sizeof(struct smi_get_handle_info),
			    sizeof(struct smi_get_handle_resp));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	handle_resp = (struct smi_get_handle_resp *)&smi_req.thread->ioctl_cmd.payload;
	memset(vf_handle, 0, sizeof(amdsmi_vf_handle_t));
	if (handle_resp->vf_id.handle == 0) {
		return AMDSMI_STATUS_NOT_FOUND;
	}

	vf_handle->handle = handle_resp->vf_id.handle;
	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_device_bdf(amdsmi_processor_handle processor_handle, amdsmi_bdf_t *bdf)
{
	#pragma SMI_EXPORT
	enum smi_handle_type type;
	struct smi_gpu_handle *gpu = NULL;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || bdf == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	for (uint32_t i = 0; i < g_gpu_device_handles.num_gpus; i++) {
		gpu = (struct smi_gpu_handle *)processor_handle;
		if (gpu->handle == g_gpu_device_handles.gpus[i].handle) {
			bdf->as_uint = gpu->bdf.as_uint;
			return AMDSMI_STATUS_SUCCESS;
		}
	}

	return AMDSMI_STATUS_NOT_FOUND;
}

amdsmi_status_t amdsmi_get_nic_device_bdf(amdsmi_processor_handle processor_handle, amdsmi_bdf_t *bdf)
{
	#pragma SMI_EXPORT
	enum smi_handle_type type;
	struct smi_nic_handle *nic = NULL;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || bdf == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	for (uint32_t i = 0; i < g_nic_device_handles.num_nics; i++) {
		nic = ((struct smi_nic_handle *)processor_handle);
		if (nic->bdf.as_uint == g_nic_device_handles.nics[i].bdf.as_uint) {
			bdf->as_uint = nic->bdf.as_uint;
			return AMDSMI_STATUS_SUCCESS;
		}
	}

	return AMDSMI_STATUS_NOT_FOUND;
}

amdsmi_status_t amdsmi_get_processor_bdf(amdsmi_processor_handle processor_handle, amdsmi_bdf_t *bdf)
{
	#pragma SMI_EXPORT
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || bdf == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	switch (type) {
	case SMI_HANDLE_TYPE_AMD_GPU:
		return amdsmi_get_gpu_device_bdf(processor_handle, bdf);
	case SMI_HANDLE_TYPE_AMD_NIC:
		return amdsmi_get_nic_device_bdf(processor_handle, bdf);
	default:
		return AMDSMI_STATUS_API_FAILED;
	}
}

amdsmi_status_t amdsmi_get_vf_bdf(amdsmi_vf_handle_t vf_handle, amdsmi_bdf_t *bdf)
{
	#pragma SMI_EXPORT
	struct smi_vf_static_info *vf_info;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (bdf == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}
	smi_device_handle_t device_handle;
	device_handle.handle = vf_handle.handle;

	const int code = amdsmi_ioctl_get_vf_static_info(&smi_req, device_handle);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}
	vf_info = (struct smi_vf_static_info *)&smi_req.thread->ioctl_cmd.payload;
	memset(bdf, 0, sizeof(amdsmi_bdf_t));
	(*bdf).as_uint = (*vf_info).bdf.as_uint;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_vf_handle_from_vf_index(amdsmi_processor_handle processor_handle, uint32_t fcn_idx, amdsmi_vf_handle_t *vf_handle)
{
	#pragma SMI_EXPORT
	struct smi_vf_partition_info *vf_part_info = NULL;
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	enum smi_handle_type type;
	struct smi_gpu_handle *gpu = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || vf_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	const int code = amdsmi_ioctl_get_vf_partitioning_info(&smi_req, pf);
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	vf_part_info = (struct smi_vf_partition_info *)&smi_req.thread->ioctl_cmd.payload;
	memset(vf_handle, 0, sizeof(amdsmi_vf_handle_t));

	if (!(fcn_idx < vf_part_info->num_vf_enabled)) {
		SMI_ERROR("Invalid argument value(s) for index passed. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	vf_handle->handle = vf_part_info->partition[fcn_idx].id.handle;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_processor_handle_from_uuid(const char *uuid, amdsmi_processor_handle *processor_handle)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || uuid == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}
	if (!is_uuid_valid(uuid)) {
		SMI_ERROR("Wrong UUID format: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}
	char handle_uuid[AMDSMI_GPU_UUID_SIZE];
	unsigned int handle_uuid_length = AMDSMI_GPU_UUID_SIZE;
	memset(processor_handle, 0, sizeof(amdsmi_processor_handle));
	for (uint32_t i = 0; i < g_gpu_device_handles.num_gpus; i++) {
		int ret = amdsmi_get_gpu_device_uuid(&g_gpu_device_handles.gpus[i], &handle_uuid_length, handle_uuid);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			return ret;
		}
		if (strncmp(handle_uuid, uuid, handle_uuid_length) == 0) {
			*processor_handle = &g_gpu_device_handles.gpus[i];
			return AMDSMI_STATUS_SUCCESS;
		}
	}
	return AMDSMI_STATUS_NOT_FOUND;

}

amdsmi_status_t amdsmi_get_vf_handle_from_uuid(const char *uuid, amdsmi_vf_handle_t *vf_handle)
{
	#pragma SMI_EXPORT

	struct smi_vf_partition_info *vf_part_info = NULL;
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	amdsmi_vf_handle_t vf;
	char vf_handle_uuid[AMDSMI_GPU_UUID_SIZE];
	unsigned int vf_handle_uuid_length = AMDSMI_GPU_UUID_SIZE;
	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (vf_handle == NULL || uuid == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}
	if (!is_uuid_valid(uuid)) {
		SMI_ERROR("Wrong UUID format: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	for (uint32_t i = 0; i < g_gpu_device_handles.num_gpus; i++) {
		pf.handle = g_gpu_device_handles.gpus[i].handle;
		const int code = amdsmi_ioctl_get_vf_partitioning_info(&smi_req, pf);
		if (code != AMDSMI_STATUS_SUCCESS) {
			SMI_ERROR("Ioctl call failed. Return code: %d", code);
			return code;
		}

		vf_part_info = (struct smi_vf_partition_info *)&smi_req.thread->ioctl_cmd.payload;
		memset(vf_handle, 0, sizeof(amdsmi_vf_handle_t));
		for (uint32_t idx_vf = 0; idx_vf < vf_part_info->num_vf_enabled; idx_vf++) {
			vf.handle = vf_part_info->partition[idx_vf].id.handle;
			amdsmi_status_t ret = amdsmi_get_vf_uuid(vf, &vf_handle_uuid_length, vf_handle_uuid);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				return ret;
			}
			if (strncmp(vf_handle_uuid, uuid, vf_handle_uuid_length) == 0) {
				vf_handle->handle = vf_part_info->partition[idx_vf].id.handle;
				return AMDSMI_STATUS_SUCCESS;
			}
		}
	}
	return AMDSMI_STATUS_NOT_FOUND;
}

amdsmi_status_t amdsmi_get_gpu_device_uuid(amdsmi_processor_handle processor_handle, unsigned int *uuid_length, char *uuid)
{
	#pragma SMI_EXPORT
	amdsmi_asic_info_t asic_info;
	struct smi_device_info *dev = NULL;
	struct smi_asic_info *gpu_info = NULL;
	uint8_t fcn;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;

	smi_req_ctx smi_req;
	smi_device_handle_t pf;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || uuid == NULL || uuid_length == NULL || *uuid_length < AMDSMI_GPU_UUID_SIZE) {
		SMI_ERROR("Invalid argument value(s) for uuid passed. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_ASIC_INFO,
			sizeof(struct smi_device_info),
			sizeof(struct smi_asic_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Getting asic info failed. Return code: %d", code);
		return code;
	}
	gpu_info = (struct smi_asic_info *)&smi_req.thread->ioctl_cmd.payload;
	memcpy(&asic_info, gpu_info, sizeof(amdsmi_asic_info_t));

	fcn = 0xff;

	smi_uuid_gen(uuid, strtoull(asic_info.asic_serial, NULL, 0), (uint16_t)asic_info.device_id, fcn);

	*uuid_length = AMDSMI_GPU_UUID_SIZE;
	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_vf_uuid(amdsmi_vf_handle_t vf_handle, unsigned int *uuid_length, char *uuid)
{
	#pragma SMI_EXPORT
	amdsmi_asic_info_t asic_info;
	struct smi_device_info *gpu = NULL;
	struct smi_asic_info *gpu_info = NULL;
	int code = 0;
	uint8_t fcn;
	uint32_t i;
	struct smi_vf_partition_info *vf_part_info = NULL;
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	uint64_t vf_device_id;
	AMDSMI_ESCAPE_IF_NOT_INIT;
	if (uuid == NULL || uuid_length == NULL || *uuid_length < AMDSMI_GPU_UUID_SIZE) {
		SMI_ERROR("Invalid argument value(s) for uuid passed. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}
	smi_device_handle_t dev_handle;
	dev_handle.handle = vf_handle.handle;

	pf.handle = make_parent_handle(dev_handle.handle);
	gpu = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	gpu->dev_id.handle = pf.handle;
	code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_ASIC_INFO,
			sizeof(struct smi_device_info),
			sizeof(struct smi_asic_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Getting asic info failed. Return code: %d", code);
		return code;
	}
	gpu_info = (struct smi_asic_info *)&smi_req.thread->ioctl_cmd.payload;
	memcpy(&asic_info, gpu_info, sizeof(amdsmi_asic_info_t));

	code = amdsmi_ioctl_get_vf_partitioning_info(&smi_req, pf);
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}
	vf_part_info =
		(struct smi_vf_partition_info *)&smi_req.thread->ioctl_cmd.payload;
	for (i = 0; i < vf_part_info->num_vf_enabled; i++) {
		if (dev_handle.handle == vf_part_info->partition[i].id.handle)
			break;
	}
	if (i == vf_part_info->num_vf_enabled) {
		code = AMDSMI_STATUS_INVAL;
		SMI_ERROR("Given processor handle not found in any of the vf partitions. Return code: %d", code);
		return code;
	}
	fcn = (uint8_t)i;
	smi_get_vf_device_id_from_pf(asic_info.device_id, &vf_device_id);

	smi_uuid_gen(uuid, strtoull(asic_info.asic_serial, NULL, 0), (uint16_t)vf_device_id, fcn);
	*uuid_length = AMDSMI_GPU_UUID_SIZE;
	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_asic_info(amdsmi_processor_handle processor_handle, amdsmi_asic_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_asic_info *gpu_info = NULL;
	struct smi_device_info *dev = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	system_wrapper *sys_wrapper = get_system_wrapper();

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (info == NULL || processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_ASIC_INFO,
			sizeof(struct smi_device_info),
			sizeof(struct smi_asic_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	gpu_info = (struct smi_asic_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_asic_info_t));
	memcpy(info->market_name, gpu_info->market_name, strlen(gpu_info->market_name)+1);
	info->vendor_id = gpu_info->vendor_id;

	sys_wrapper->smi_strncpy(info->vendor_name, sizeof(info->vendor_name), VENDOR_NAME, AMDSMI_MAX_STRING_LENGTH);
	info->subvendor_id = gpu_info->subvendor_id;
	info->device_id = gpu_info->device_id;
	info->rev_id = gpu_info->rev_id;
	memcpy(info->asic_serial, gpu_info->asic_serial, strlen(gpu_info->asic_serial)+1);
	info->oam_id = gpu_info->oam_id;
	info->num_of_compute_units = gpu_info->num_of_compute_units;
	info->target_graphics_version = gpu_info->target_graphics_version;
	info->subsystem_id = gpu_info->subsystem_id;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_vram_info(amdsmi_processor_handle processor_handle, amdsmi_vram_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_vram_info *gpu_info = NULL;
	struct smi_device_info *dev = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	system_wrapper *sys_wrapper = get_system_wrapper();

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_VRAM_INFO,
			sizeof(struct smi_device_info),
			sizeof(struct smi_vram_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	gpu_info = (struct smi_vram_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_vram_info_t));
	if (gpu_info->vram_size == (uint32_t)SMI_NOT_SUPPORTED)
		return AMDSMI_STATUS_NOT_SUPPORTED;

	info->vram_type = (amdsmi_vram_type_t)gpu_info->vram_type;
	sys_wrapper->smi_strncpy(info->vram_vendor, sizeof(info->vram_vendor),
				 gpu_info->vram_vendor, AMDSMI_MAX_STRING_LENGTH);

	info->vram_size = gpu_info->vram_size;
	info->vram_bit_width = gpu_info->vram_bit_width;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_driver_info(amdsmi_processor_handle processor_handle, amdsmi_driver_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_driver_info *gpu_info = NULL;
	struct smi_device_info *dev = NULL;
	char driver_name[AMDSMI_MAX_STRING_LENGTH];
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	system_wrapper *sys_wrapper = get_system_wrapper();

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;

	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_GPU_DRIVER_INFO,
			sizeof(struct smi_device_info),
			sizeof(struct smi_driver_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	gpu_info = (struct smi_driver_info *)&smi_req.thread->ioctl_cmd.payload;

	switch (gpu_info->id) {
	case AMDSMI_DRIVER_LIBGV:
		sys_wrapper->smi_strncpy(driver_name, sizeof(driver_name),
					 "GIM", AMDSMI_MAX_STRING_LENGTH);
		break;
	case AMDSMI_DRIVER_AMDGPUV:
		sys_wrapper->smi_strncpy(driver_name, sizeof(driver_name),
					 "AMDGPUV", AMDSMI_MAX_STRING_LENGTH);
		break;
	case AMDSMI_DRIVER_VMWGPUV:
		sys_wrapper->smi_strncpy(driver_name, sizeof(driver_name),
					 "VMWGPUV", AMDSMI_MAX_STRING_LENGTH);
		break;
	default:
		sys_wrapper->smi_strncpy(driver_name, sizeof(driver_name),
					 "UNKNOWN", AMDSMI_MAX_STRING_LENGTH);
		break;
	}

	memset(info, 0, sizeof(amdsmi_driver_info_t));
	sys_wrapper->smi_strncpy(info->driver_name, sizeof(info->driver_name), driver_name, AMDSMI_MAX_STRING_LENGTH);
	memcpy(info->driver_version, gpu_info->version, gpu_info->version_len);
	memcpy(info->driver_date, gpu_info->driver_date, AMDSMI_MAX_STRING_LENGTH);

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_driver_model(amdsmi_processor_handle processor_handle, amdsmi_driver_model_type_t *model)
{
	#pragma SMI_EXPORT
	smi_device_handle_t pf;
	smi_req_ctx smi_req;
	struct smi_gpu_driver_model *driver_model = NULL;
	struct smi_device_info *dev = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || model == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id = pf;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_GPU_DRIVER_MODEL,
						sizeof(struct smi_device_info),
						sizeof(struct smi_gpu_driver_model));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	driver_model = (struct smi_gpu_driver_model *)&smi_req.thread->ioctl_cmd.payload;

	*model = (amdsmi_driver_model_type_t)driver_model->model;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_power_cap_info(amdsmi_processor_handle processor_handle, uint32_t sensor_ind, amdsmi_power_cap_info_t *info)
{
	AMDSMI_UNUSED(sensor_ind);
	#pragma SMI_EXPORT
	struct smi_power_cap_info *gpu_info = NULL;
	struct smi_device_info_ex *dev = NULL;
	int code = 0;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info_ex *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;
	code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_POWER_CAP_INFO,
			sizeof(struct smi_device_info_ex),
			sizeof(struct smi_power_cap_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	gpu_info = (struct smi_power_cap_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_power_cap_info_t));
	info->power_cap = gpu_info->power_cap;
	info->dpm_cap = gpu_info->dpm_cap;
	info->min_power_cap = gpu_info->min_power_cap;
	info->max_power_cap = gpu_info->max_power_cap;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_pcie_info(amdsmi_processor_handle processor_handle, amdsmi_pcie_info_t *info)
{
	#pragma SMI_EXPORT
	smi_device_handle_t pf;
	struct smi_device_info *dev = NULL;
	smi_req_ctx smi_req;
	struct smi_pcie_info *pcie = NULL;
	amdsmi_asic_info_t asic_info;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	int code = amdsmi_get_gpu_asic_info(processor_handle, &asic_info);
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Get gpu asic info failed. Return code: %d", code);
		return code;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id = pf;
	code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_PCIE_INFO,
				sizeof(struct smi_device_info),
				sizeof(struct smi_pcie_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	pcie = (struct smi_pcie_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_pcie_info_t));
	// static PCIe info
	info->pcie_static.max_pcie_width = (uint16_t)pcie->pcie_static.max_pcie_width;
	info->pcie_static.max_pcie_speed = pcie->pcie_static.max_pcie_speed;
	info->pcie_static.max_pcie_interface_version = pcie->pcie_static.max_pcie_interface_version;
	info->pcie_static.slot_type = (amdsmi_card_form_factor_t)pcie->pcie_static.slot_type;
	if ((asic_info.device_id == 0x74A0) || (asic_info.device_id == 0x74A1) ||
		(asic_info.device_id == 0x74A2))
		info->pcie_static.pcie_interface_version = 5;
	else
		info->pcie_static.pcie_interface_version = 4;
	// metric PCIe info
	info->pcie_metric.pcie_width = (uint16_t)pcie->pcie_metric.pcie_width;
	info->pcie_metric.pcie_bandwidth = pcie->pcie_metric.pcie_bandwidth;
	info->pcie_metric.pcie_replay_count = pcie->pcie_metric.pcie_replay_count;
	info->pcie_metric.pcie_l0_to_recovery_count = pcie->pcie_metric.pcie_l0_to_recovery_count;
	info->pcie_metric.pcie_replay_roll_over_count = pcie->pcie_metric.pcie_replay_roll_over_count;
	info->pcie_metric.pcie_nak_sent_count = pcie->pcie_metric.pcie_nak_sent_count;
	info->pcie_metric.pcie_nak_received_count = pcie->pcie_metric.pcie_nak_received_count;
	info->pcie_metric.pcie_lc_perf_other_end_recovery_count = pcie->pcie_metric.pcie_lc_perf_other_end_recovery_count;

	int ret = amdsmi_get_pcie_speed_from_pcie_type(pcie->pcie_metric.pcie_speed, &info->pcie_metric.pcie_speed,
												asic_info.device_id);
	if (ret) {
		SMI_ERROR("Get pcie speed from pcie type failed. Return code: %d", ret);
		return ret;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_fb_layout(amdsmi_processor_handle processor_handle, amdsmi_pf_fb_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_pf_fb_info *gpu_info = NULL;
	struct smi_device_info *dev = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id = pf;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_PF_FB_INFO,
				sizeof(struct smi_device_info),
				sizeof(struct smi_pf_fb_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("AMDSMI Ioctl get gpu static info call failed. Return code: %d", code);
		return code;
	}

	gpu_info = (struct smi_pf_fb_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_pf_fb_info_t));
	info->total_fb_size = gpu_info->total_fb_size;
	info->pf_fb_reserved = gpu_info->pf_fb_reserved;
	info->pf_fb_offset = gpu_info->pf_fb_offset;
	info->fb_alignment = gpu_info->fb_alignment;
	info->max_vf_fb_usable = gpu_info->max_vf_fb_usable;
	info->min_vf_fb_usable = gpu_info->min_vf_fb_usable;


	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_vbios_info(amdsmi_processor_handle processor_handle, amdsmi_vbios_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_vbios_info *gpu_info = NULL;
	struct smi_device_info *dev = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_VBIOS_INFO,
			sizeof(struct smi_device_info),
			sizeof(struct smi_vbios_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	gpu_info = (struct smi_vbios_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_vbios_info_t));
	memcpy(info->name, gpu_info->name, strlen(gpu_info->name)+1);
	memcpy(info->build_date, gpu_info->build_date, strlen(gpu_info->build_date)+1);
	memcpy(info->part_number, gpu_info->part_number, strlen(gpu_info->part_number)+1);
	memcpy(info->version, gpu_info->version, strlen(gpu_info->version)+1);
	memcpy(info->boot_firmware, gpu_info->boot_firmware, strlen(gpu_info->boot_firmware)+1);

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_board_info(amdsmi_processor_handle processor_handle, amdsmi_board_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_board_info *gpu_info = NULL;
	struct smi_device_info *dev = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	system_wrapper *sys_wrapper = get_system_wrapper();

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_BOARD_INFO,
			sizeof(struct smi_device_info),
			sizeof(struct smi_board_info));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	gpu_info = (struct smi_board_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_board_info_t));
	sys_wrapper->smi_strncpy(info->model_number, sizeof(info->model_number), gpu_info->model_number, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->product_serial, sizeof(info->product_serial), gpu_info->product_serial, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->fru_id, sizeof(info->fru_id), gpu_info->fru_id, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->product_name, sizeof(info->product_name), gpu_info->product_name, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->manufacturer_name, sizeof(info->manufacturer_name), gpu_info->manufacturer_name, AMDSMI_MAX_STRING_LENGTH);

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_fw_info(amdsmi_processor_handle processor_handle, amdsmi_fw_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_fw_info *gpu_info = NULL;
	struct smi_device_info *dev = NULL;
	uint32_t j = 0;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_GPU_FW_INFO,
		sizeof(struct smi_device_info),
		sizeof(struct smi_fw_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	gpu_info = (struct smi_fw_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_fw_info_t));
	for (uint32_t i = 0; i < gpu_info->num_fw_info; i++) {
		if (gpu_info->fw_info_list[i].fw_version != 0) {
			info->fw_info_list[j].fw_id = (amdsmi_fw_block_t)gpu_info->fw_info_list[i].fw_id;
			info->fw_info_list[j].fw_version = gpu_info->fw_info_list[i].fw_version;
			j++;
		}
	}

	info->num_fw_info = (uint8_t)j;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_fw_error_records(amdsmi_processor_handle processor_handle, amdsmi_fw_error_record_t *records)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	struct smi_device_info *dev = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	amdsmi_fw_error_record_t *err_records = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || records == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_UCODE_ERR_RECORDS,
				     sizeof(struct smi_device_info),
				     sizeof(struct smi_fw_error_record));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	err_records = (amdsmi_fw_error_record_t *)&smi_req.thread->ioctl_cmd.payload;

	memset(records, 0, sizeof(amdsmi_fw_error_record_t));
	records->num_err_records = err_records->num_err_records;
	for (uint32_t i = 0; i < AMDSMI_MAX_ERR_RECORDS; i++) {
		records->err_records[i].timestamp = err_records->err_records[i].timestamp;
		records->err_records[i].vf_idx = err_records->err_records[i].vf_idx;
		records->err_records[i].fw_id = err_records->err_records[i].fw_id;
		records->err_records[i].status = err_records->err_records[i].status;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_dfc_fw_table(amdsmi_processor_handle processor_handle, amdsmi_dfc_fw_t *info)
{
	#pragma SMI_EXPORT
#ifdef __linux__
	AMDSMI_UNUSED(processor_handle);
	AMDSMI_UNUSED(info);
	return AMDSMI_STATUS_NOT_SUPPORTED;
#else
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	struct smi_device_info *dev = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	amdsmi_dfc_fw_t *dfc_fw = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_DFC_FW_TABLE,
				     sizeof(struct smi_device_info),
				     sizeof(struct smi_dfc_fw));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	dfc_fw = (amdsmi_dfc_fw_t *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_dfc_fw_t));
	info->header.dfc_fw_total_entries = dfc_fw->header.dfc_fw_total_entries;
	info->header.dfc_fw_version = dfc_fw->header.dfc_fw_version;
	info->header.dfc_gart_wr_guest_min = dfc_fw->header.dfc_gart_wr_guest_min;
	info->header.dfc_gart_wr_guest_max = dfc_fw->header.dfc_gart_wr_guest_max;

	for (uint32_t i = 0; i < dfc_fw->header.dfc_fw_total_entries; i++) {
		info->data[i].dfc_fw_type = dfc_fw->data[i].dfc_fw_type;
		info->data[i].verification_enabled = dfc_fw->data[i].verification_enabled;
		info->data[i].customer_ordinal = dfc_fw->data[i].customer_ordinal;
		for (uint32_t j = 0; j < AMDSMI_MAX_WHITE_LIST_ELEMENTS; j++) {
			info->data[i].white_list[j].latest = dfc_fw->data[i].white_list[j].latest;
			info->data[i].white_list[j].oldest = dfc_fw->data[i].white_list[j].oldest;
		}
		for (int z = 0; z < AMDSMI_MAX_BLACK_LIST_ELEMENTS; z++) {
			info->data[i].black_list[z] = dfc_fw->data[i].black_list[z];
		}
	}

	return AMDSMI_STATUS_SUCCESS;
#endif
}

amdsmi_status_t amdsmi_get_gpu_activity(amdsmi_processor_handle processor_handle, amdsmi_engine_usage_t *info)
{
	#pragma SMI_EXPORT
	struct smi_gpu_performance_info *gpu_performance_info = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	const int code = amdsmi_ioctl_get_gpu_performance_info(&smi_req, pf);
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	gpu_performance_info = (struct smi_gpu_performance_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_engine_usage_t));
	info->gfx_activity = gpu_performance_info->usage.gfx_activity;
	info->umc_activity = gpu_performance_info->usage.umc_activity;
	info->mm_activity = gpu_performance_info->usage.mm_activity;


	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_power_info(amdsmi_processor_handle processor_handle, amdsmi_power_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_gpu_performance_info *gpu_performance_info = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	const int code = amdsmi_ioctl_get_gpu_performance_info(&smi_req, pf);
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	gpu_performance_info = (struct smi_gpu_performance_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_power_info_t));
	info->socket_power = gpu_performance_info->power.socket_power;
	info->gfx_voltage = gpu_performance_info->power.gfx_voltage;
	info->soc_voltage = gpu_performance_info->power.soc_voltage;
	info->mem_voltage = gpu_performance_info->power.mem_voltage;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_set_power_cap(amdsmi_processor_handle processor_handle, uint32_t sensor_ind, uint64_t cap)
{
	AMDSMI_UNUSED(sensor_ind);
	#pragma SMI_EXPORT
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	struct smi_set_gpu_power_cap *dev_power_cap = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev_power_cap = (struct smi_set_gpu_power_cap *)&smi_req.thread->ioctl_cmd.payload;
	dev_power_cap->dev_id = pf;
	dev_power_cap->power_cap = cap;

	int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_SET_GPU_POWER_CAP,
								sizeof(struct smi_set_gpu_power_cap), 0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_is_gpu_power_management_enabled(amdsmi_processor_handle processor_handle, bool *enabled)
{
	#pragma SMI_EXPORT
	struct smi_data_query *smi_query = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	union smi_data *data = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || enabled == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;

	smi_query = (struct smi_data_query *)&smi_req.thread->ioctl_cmd.payload;
	smi_query->dev.dev_id = pf;
	smi_query->type = SMI_POWER_MANAGEMENT;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_SMI_DATA,
				     sizeof(struct smi_data_query),
				     sizeof(union smi_data));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	data = (union smi_data *)&smi_req.thread->ioctl_cmd.payload;
	*enabled = data->power_management_enabled;

	return AMDSMI_STATUS_SUCCESS;

}

amdsmi_status_t amdsmi_get_clock_info(amdsmi_processor_handle processor_handle, amdsmi_clk_type_t clk_type, amdsmi_clk_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_gpu_performance_info *gpu_performance_info = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	int code;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	code = amdsmi_ioctl_get_gpu_performance_info(&smi_req, pf);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	gpu_performance_info = (struct smi_gpu_performance_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_clk_info_t));
	info->clk = gpu_performance_info->clock.cur_clk[clk_type];
	info->max_clk = gpu_performance_info->clock.max_clk[clk_type];
	info->min_clk = gpu_performance_info->clock.min_clk[clk_type];
	info->clk_locked = gpu_performance_info->clock.clk_locked[clk_type];
	info->clk_deep_sleep = gpu_performance_info->clock.clk_deep_sleep[clk_type];

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_temp_metric(amdsmi_processor_handle processor_handle, amdsmi_temperature_type_t sensor_type,
										amdsmi_temperature_metric_t metric, int64_t *temperature)
{
	#pragma SMI_EXPORT
	struct smi_gpu_performance_info *gpu_performance_info = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	int ret;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || temperature == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	switch (sensor_type) {
	case AMDSMI_TEMPERATURE_TYPE_EDGE:
	case AMDSMI_TEMPERATURE_TYPE_HOTSPOT:
	case AMDSMI_TEMPERATURE_TYPE_VRAM:
		break;
	case AMDSMI_TEMPERATURE_TYPE_PLX:
	case AMDSMI_TEMPERATURE_TYPE_HBM_0:
	case AMDSMI_TEMPERATURE_TYPE_HBM_1:
	case AMDSMI_TEMPERATURE_TYPE_HBM_2:
	case AMDSMI_TEMPERATURE_TYPE_HBM_3:
		*temperature = (int64_t)0;
		return AMDSMI_STATUS_SUCCESS;
	default:
		SMI_ERROR("Passed GPU sensor type is not valid. Return code:: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	ret = amdsmi_ioctl_get_gpu_performance_info(&smi_req, pf);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", ret);
		return ret;
	}

	gpu_performance_info = (struct smi_gpu_performance_info *)&smi_req.thread->ioctl_cmd.payload;

	switch (metric) {
	case AMDSMI_TEMP_CURRENT:
		*temperature = gpu_performance_info->temp.temp[sensor_type];
		break;
	case AMDSMI_TEMP_CRITICAL:
		*temperature = gpu_performance_info->temp_limit.temp[sensor_type];
		break;
	case AMDSMI_TEMP_SHUTDOWN:
		*temperature = gpu_performance_info->temp_shutdown.temp[sensor_type];
		break;
	default:
		*temperature = (int64_t)0;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_cache_info(amdsmi_processor_handle processor_handle, amdsmi_gpu_cache_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_gpu_cache_info *gpu_info = NULL;
	struct smi_device_info *dev = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_device_handle_t pf;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;
	int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_GPU_CACHE_INFO,
			sizeof(struct smi_device_info),
			sizeof(struct smi_gpu_cache_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	gpu_info = (struct smi_gpu_cache_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_gpu_cache_info_t));
	if (gpu_info->num_cache_types == (uint32_t)SMI_NOT_SUPPORTED)
		return AMDSMI_STATUS_NOT_SUPPORTED;

	info->num_cache_types = gpu_info->num_cache_types;
	for (uint32_t i = 0; i < gpu_info->num_cache_types; i++) {
		info->cache[i].cache_size = gpu_info->cache[i].cache_size;
		info->cache[i].cache_level = gpu_info->cache[i].cache_level;
		info->cache[i].cache_properties = gpu_info->cache[i].cache_properties;
		info->cache[i].max_num_cu_shared = gpu_info->cache[i].max_num_cu_shared;
		info->cache[i].num_cache_instance = gpu_info->cache[i].num_cache_instance;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_total_ecc_count(amdsmi_processor_handle processor_handle, amdsmi_error_count_t *ec)
{
	#pragma SMI_EXPORT
	struct smi_ecc_info *ecc_info = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || ec == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	const int code = amdsmi_ioctl_get_ecc_error_count(&smi_req, pf);
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	ecc_info = (struct smi_ecc_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(ec, 0, sizeof(amdsmi_error_count_t));
	ec->correctable_count = ecc_info->err_count.correctable_count;
	ec->uncorrectable_count = ecc_info->err_count.uncorrectable_count;
	ec->deferred_count = ecc_info->err_count.deferred_count;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_ecc_count(amdsmi_processor_handle processor_handle, amdsmi_gpu_block_t block, amdsmi_error_count_t *ec)
{
	#pragma SMI_EXPORT
	struct smi_ecc_info *ecc_info = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	struct smi_ras_query_if *ras = NULL;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || ec == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	ras = (struct smi_ras_query_if *)&smi_req.thread->ioctl_cmd.payload;
	ras->dev_id = pf;
	ras->head.block = (enum smi_gpu_block)block;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_BLOCK_ECC_STATUS,
					sizeof(struct smi_ras_query_if),
					sizeof(struct smi_ecc_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	ecc_info = (struct smi_ecc_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(ec, 0, sizeof(amdsmi_error_count_t));
	ec->correctable_count = ecc_info->err_count.correctable_count;
	ec->uncorrectable_count = ecc_info->err_count.uncorrectable_count;
	ec->deferred_count = ecc_info->err_count.deferred_count;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_ecc_enabled(amdsmi_processor_handle processor_handle, uint64_t *enabled_blocks)
{
	#pragma SMI_EXPORT
	struct smi_data_query *smi_query = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	union smi_data *data = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || enabled_blocks == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;

	smi_query = (struct smi_data_query *)&smi_req.thread->ioctl_cmd.payload;
	smi_query->dev.dev_id = pf;
	smi_query->type = SMI_RAS_CAPS;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_SMI_DATA,
				     sizeof(struct smi_data_query),
				     sizeof(union smi_data));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	data = (union smi_data *)&smi_req.thread->ioctl_cmd.payload;
	*enabled_blocks = data->ras_caps;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_status_code_to_string(amdsmi_status_t status, const char **status_string)
{
	#pragma SMI_EXPORT
	if (amdsmi_get_string_from_status_enum(status, status_string) != AMDSMI_STATUS_SUCCESS) {
		return AMDSMI_STATUS_API_FAILED;
	}
	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_bad_page_info(amdsmi_processor_handle processor_handle, uint32_t *bad_page_size,
					amdsmi_eeprom_table_record_t *bad_pages)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	struct smi_bad_page_info *bad_page_info = NULL;
	struct smi_bad_page_record *bad_page_record = NULL;
	system_wrapper *sys_wrapper = get_system_wrapper();

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || bad_page_size == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	if ((*bad_page_size == 0) && (bad_pages != NULL)) {
		SMI_ERROR("Insufficient buffer size given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	if (*bad_page_size > AMDSMI_MAX_BAD_PAGE_RECORD) {
		*bad_page_size = AMDSMI_MAX_BAD_PAGE_RECORD;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	bad_page_info = (struct smi_bad_page_info *)&smi_req.thread->ioctl_cmd.payload;
	bad_page_info->dev_id = pf;
#ifdef _WIN64
	bad_page_record = sys_wrapper->smi_calloc(1, sizeof(struct smi_bad_page_record));
#else
	long page_size;
	size_t alloc_size;
	page_size = sys_wrapper->smi_sysconf(_SC_PAGESIZE);
	if (page_size == -1) {
		SMI_ERROR("Failed to get system configuration. Couldn't get page size. Return code: %d", AMDSMI_STATUS_API_FAILED);
		return AMDSMI_STATUS_API_FAILED;
	}
	// Round up size to be a multiple of page_size for aligned_alloc
	alloc_size = ((sizeof(struct smi_bad_page_record) + (size_t)page_size - 1) / (size_t)page_size) * (size_t)page_size;
	bad_page_record = sys_wrapper->smi_aligned_alloc((void **)&bad_page_record, (size_t)page_size, alloc_size);
#endif
	if (bad_page_record == NULL) {
		SMI_ERROR("Memory allocation call failed for eeprom table. Return code: %d", AMDSMI_STATUS_OUT_OF_RESOURCES);
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}
	bad_page_info->bad_pages = bad_page_record;
	bad_page_info->size = *bad_page_size;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_BAD_PAGE_INFO,
				     sizeof(struct smi_bad_page_info),
				     0);

	if (code == AMDSMI_STATUS_NO_DATA) {
		*bad_page_size = 0;
		sys_wrapper->smi_free(bad_page_record);
		return code;
	}
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		sys_wrapper->smi_free(bad_page_record);
		return code;
	}

	if ((*bad_page_size < bad_page_info->bad_pages->num_bad_page) && (bad_pages != NULL)) {
		SMI_ERROR("Size of the allocated buffer is not sufficient. Return code: %d", AMDSMI_STATUS_OUT_OF_RESOURCES);
		sys_wrapper->smi_free(bad_page_record);
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}

	if (bad_pages == NULL) {
		*bad_page_size = bad_page_info->bad_pages->num_bad_page;
	} else {
		memset(bad_pages, 0, bad_page_info->bad_pages->num_bad_page*sizeof(amdsmi_eeprom_table_record_t));
		*bad_page_size = bad_page_info->bad_pages->num_bad_page;
		for (uint32_t i = 0; i < bad_page_info->bad_pages->num_bad_page; i++) {
			bad_pages[i].retired_page = bad_page_info->bad_pages->bad_page[i].retired_page;
			bad_pages[i].ts = bad_page_info->bad_pages->bad_page[i].ts;
			bad_pages[i].err_type = bad_page_info->bad_pages->bad_page[i].err_type;
			bad_pages[i].bank = bad_page_info->bad_pages->bad_page[i].bank;
			bad_pages[i].cu = bad_page_info->bad_pages->bad_page[i].cu;
			bad_pages[i].mem_channel = bad_page_info->bad_pages->bad_page[i].mem_channel;
			bad_pages[i].mcumc_id = bad_page_info->bad_pages->bad_page[i].mcumc_id;
		}
	}

	sys_wrapper->smi_free(bad_page_record);

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_ras_feature_info(amdsmi_processor_handle processor_handle, amdsmi_ras_feature_t *ras_feature)
{
	#pragma SMI_EXPORT
	struct smi_ras_feature *ras_info = NULL;
	smi_device_handle_t pf;
	struct smi_device_info *dev = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || ras_feature == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id = pf;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_RAS_FEATURE_INFO,
					sizeof(struct smi_device_info),
					sizeof(struct smi_ras_feature));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	ras_info = (struct smi_ras_feature *)&smi_req.thread->ioctl_cmd.payload;

	memset(ras_feature, 0, sizeof(amdsmi_ras_feature_t));
	ras_feature->ras_eeprom_version = ras_info->ras_eeprom_version;
	ras_feature->ecc_correction_schema_flag = ras_info->ecc_correction_schema_flag;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_bad_page_threshold(amdsmi_processor_handle processor_handle, uint32_t *threshold)
{
	#pragma SMI_EXPORT
	struct smi_ras_feature *ras_info = NULL;
	smi_device_handle_t pf;
	struct smi_device_info *dev = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || threshold == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id = pf;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_BAD_PAGE_THRESHOLD,
					sizeof(struct smi_device_info),
					sizeof(struct smi_ras_feature));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	ras_info = (struct smi_ras_feature *)&smi_req.thread->ioctl_cmd.payload;

	*threshold = ras_info->bad_page_record_threshold;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_num_vf(amdsmi_processor_handle processor_handle, uint32_t *num_vf_enabled, uint32_t *num_vf_supported)
{
	#pragma SMI_EXPORT
	struct smi_vf_partition_info *vf_part_info = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || num_vf_enabled == NULL || num_vf_supported == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	const int code = amdsmi_ioctl_get_vf_partitioning_info(&smi_req, pf);
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	vf_part_info = (struct smi_vf_partition_info *)&smi_req.thread->ioctl_cmd.payload;

	*num_vf_enabled = vf_part_info->num_vf_enabled;
	*num_vf_supported = vf_part_info->num_vf_supported;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_set_num_vf(amdsmi_processor_handle processor_handle, uint32_t num_vf)
{
	#pragma SMI_EXPORT
#ifdef _WIN64
	AMDSMI_UNUSED(processor_handle);
	AMDSMI_UNUSED(num_vf);
	return AMDSMI_STATUS_NOT_SUPPORTED;
#else
	struct smi_vf_partition_config *vf_partition_config = NULL;
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	vf_partition_config =
		(struct smi_vf_partition_config *)&smi_req.thread->ioctl_cmd.payload;
	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	vf_partition_config->dev_id.handle = pf.handle;
	vf_partition_config->num_vf_enable = num_vf;
	const int code = amdsmi_request(&smi_req, SMI_CMD_CODE_SET_VF_PARTITIONING_INFO,
				     sizeof(struct smi_vf_partition_config), 0);
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	return AMDSMI_STATUS_SUCCESS;
#endif
}

amdsmi_status_t amdsmi_clear_vf_fb(amdsmi_vf_handle_t vf_handle)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_device_info *vf;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	vf = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	vf->dev_id.handle = vf_handle.handle;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_CLEAR_VF_FB,
				     sizeof(struct smi_device_info), 0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_vf_partition_info(amdsmi_processor_handle processor_handle, unsigned int vf_buffer_num, amdsmi_partition_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_vf_partition_info *vf_part_info = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	const int code = amdsmi_ioctl_get_vf_partitioning_info(&smi_req, pf);
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	vf_part_info = (struct smi_vf_partition_info *)&smi_req.thread->ioctl_cmd.payload;

	if (vf_buffer_num < vf_part_info->num_vf_enabled) {
		SMI_ERROR("Buffer size too small. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	memset(info, 0, sizeof(amdsmi_partition_info_t) * vf_part_info->num_vf_enabled);
	for (uint32_t i = 0; i < vf_part_info->num_vf_enabled; i++) {
		info[i].id.handle = vf_part_info->partition[i].id.handle;
		info[i].fb.fb_size = vf_part_info->partition[i].fb.fb_size;
		info[i].fb.fb_offset = vf_part_info->partition[i].fb.fb_offset;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_vf_info(amdsmi_vf_handle_t vf_handle, amdsmi_vf_info_t *config)
{
	#pragma SMI_EXPORT
	struct smi_vf_static_info *vf_part_info = NULL;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (config == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	smi_device_handle_t device_handle;
	device_handle.handle = vf_handle.handle;
	const int code = amdsmi_ioctl_get_vf_static_info(&smi_req, device_handle);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	vf_part_info = (struct smi_vf_static_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(config, 0, sizeof(amdsmi_vf_info_t));
	config->fb.fb_offset = vf_part_info->config.fb.fb_offset;
	config->fb.fb_size = vf_part_info->config.fb.fb_size;
	config->gfx_timeslice = vf_part_info->config.gfx_timeslice;
	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t
amdsmi_get_vf_data(amdsmi_vf_handle_t vf_handle, amdsmi_vf_data_t *info)
{
	#pragma SMI_EXPORT
	struct smi_vf_data *vf_dynamic_info = NULL;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	smi_device_handle_t device_handle;
	device_handle.handle = vf_handle.handle;

	int code = amdsmi_ioctl_get_vf_dynamic_info(&smi_req, device_handle);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	vf_dynamic_info = (struct smi_vf_data *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_vf_data_t));
	info->sched.state = (amdsmi_vf_sched_state_t)vf_dynamic_info->sched.state;
	info->sched.flr_count = vf_dynamic_info->sched.flr_count;
	info->sched.boot_up_time = vf_dynamic_info->sched.boot_up_time;
	info->sched.shutdown_time = vf_dynamic_info->sched.shutdown_time;
	info->sched.reset_time = vf_dynamic_info->sched.reset_time;
	info->guard.enabled = vf_dynamic_info->guard.enabled;
#ifdef _WIN64
	strncpy_s(info->sched.last_boot_start, sizeof(info->sched.last_boot_start), vf_dynamic_info->sched.last_boot_start, AMDSMI_MAX_STRING_LENGTH);
	strncpy_s(info->sched.last_boot_end, sizeof(info->sched.last_boot_end), vf_dynamic_info->sched.last_boot_end, AMDSMI_MAX_STRING_LENGTH);
	strncpy_s(info->sched.last_shutdown_start, sizeof(info->sched.last_shutdown_start), vf_dynamic_info->sched.last_shutdown_start, AMDSMI_MAX_STRING_LENGTH);
	strncpy_s(info->sched.last_shutdown_end, sizeof(info->sched.last_shutdown_end), vf_dynamic_info->sched.last_shutdown_end, AMDSMI_MAX_STRING_LENGTH);
	strncpy_s(info->sched.last_reset_start, sizeof(info->sched.last_reset_start), vf_dynamic_info->sched.last_reset_start, AMDSMI_MAX_STRING_LENGTH);
	strncpy_s(info->sched.last_reset_end, sizeof(info->sched.last_reset_end), vf_dynamic_info->sched.last_reset_end, AMDSMI_MAX_STRING_LENGTH);
	strncpy_s(info->sched.current_active_time, sizeof(info->sched.current_active_time), vf_dynamic_info->sched.current_active_time, AMDSMI_MAX_STRING_LENGTH);
	strncpy_s(info->sched.current_running_time, sizeof(info->sched.current_running_time), vf_dynamic_info->sched.current_running_time, AMDSMI_MAX_STRING_LENGTH);
	strncpy_s(info->sched.total_active_time, sizeof(info->sched.total_active_time), vf_dynamic_info->sched.total_active_time, AMDSMI_MAX_STRING_LENGTH);
	strncpy_s(info->sched.total_running_time, sizeof(info->sched.total_running_time), vf_dynamic_info->sched.total_running_time, AMDSMI_MAX_STRING_LENGTH);
#else
	strncpy(info->sched.last_boot_start, vf_dynamic_info->sched.last_boot_start, AMDSMI_MAX_STRING_LENGTH);
	strncpy(info->sched.last_boot_end, vf_dynamic_info->sched.last_boot_end, AMDSMI_MAX_STRING_LENGTH);
	strncpy(info->sched.last_shutdown_start, vf_dynamic_info->sched.last_shutdown_start, AMDSMI_MAX_STRING_LENGTH);
	strncpy(info->sched.last_shutdown_end, vf_dynamic_info->sched.last_shutdown_end, AMDSMI_MAX_STRING_LENGTH);
	strncpy(info->sched.last_reset_start, vf_dynamic_info->sched.last_reset_start, AMDSMI_MAX_STRING_LENGTH);
	strncpy(info->sched.last_reset_end, vf_dynamic_info->sched.last_reset_end, AMDSMI_MAX_STRING_LENGTH);
	strncpy(info->sched.current_active_time, vf_dynamic_info->sched.current_active_time, AMDSMI_MAX_STRING_LENGTH);
	strncpy(info->sched.current_running_time, vf_dynamic_info->sched.current_running_time, AMDSMI_MAX_STRING_LENGTH);
	strncpy(info->sched.total_active_time, vf_dynamic_info->sched.total_active_time, AMDSMI_MAX_STRING_LENGTH);
	strncpy(info->sched.total_running_time, vf_dynamic_info->sched.total_running_time, AMDSMI_MAX_STRING_LENGTH);
#endif
	for (uint32_t i = 0; i < AMDSMI_GUARD_EVENT__MAX; i++) {
		info->guard.guard[i].active = vf_dynamic_info->guard.guard[i].active;
		info->guard.guard[i].interval = vf_dynamic_info->guard.guard[i].interval/1000000;
		info->guard.guard[i].threshold = vf_dynamic_info->guard.guard[i].threshold;
		info->guard.guard[i].amount = vf_dynamic_info->guard.guard[i].amount;
		info->guard.guard[i].state = (amdsmi_guard_state_t)vf_dynamic_info->guard.guard[i].state;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_event_create(amdsmi_processor_handle *processor_list, uint32_t num_devices, uint64_t event_types, amdsmi_event_set *set)
{
	#pragma SMI_EXPORT
	struct smi_event_set_config *config = NULL;
	smi_event_handle_t *event_set = NULL;
	system_wrapper *sys_wrapper = get_system_wrapper();
	struct smi_event_set_s *event_set_handle = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_list == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	if (num_devices == 0) {
		SMI_ERROR("Number of processors must be greater than 0. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	event_set_handle = sys_wrapper->smi_malloc(sizeof(struct smi_event_set_s));
	if (event_set_handle == NULL) {
		SMI_ERROR("Failed to allocate memory for event handler. Return code: %d", AMDSMI_STATUS_OUT_OF_RESOURCES);
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}

	event_set_handle->devices = sys_wrapper->smi_malloc(num_devices * sizeof(smi_device_handle_t));
	if (event_set_handle->devices == NULL) {
		sys_wrapper->smi_free(event_set_handle);
		SMI_ERROR("Failed to allocate memory for device handles. Return code: %d", AMDSMI_STATUS_OUT_OF_RESOURCES);
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}

	event_set_handle->handles = sys_wrapper->smi_malloc(num_devices * sizeof(smi_event_handle_t));
	if (event_set_handle->handles == NULL) {
		sys_wrapper->smi_free(event_set_handle->devices);
		sys_wrapper->smi_free(event_set_handle);
		SMI_ERROR("Failed to allocate memory for event handles. Return code: %d", AMDSMI_STATUS_OUT_OF_RESOURCES);
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}

	event_set_handle->num_handles = num_devices;

	config = (struct smi_event_set_config *)&smi_req.thread->ioctl_cmd.payload;
	event_set = (smi_event_handle_t *)&smi_req.thread->ioctl_cmd.payload;

	config->event_mask = event_types;
	for (uint32_t i = 0; i < num_devices; ++i) {
		type = *((enum smi_handle_type *)processor_list[i]);
		if (type != SMI_HANDLE_TYPE_AMD_GPU) {
			SMI_ERROR("Wrong processor handle [%d]. Return code: %d", i, AMDSMI_STATUS_INVAL);
			sys_wrapper->smi_free(event_set_handle->devices);
			sys_wrapper->smi_free(event_set_handle->handles);
			sys_wrapper->smi_free(event_set_handle);
			return AMDSMI_STATUS_INVAL;
		}
		gpu = ((struct smi_gpu_handle *)processor_list[i]);
		config->dev_id.handle = gpu->handle;
#ifdef _WIN64
		config->event_set.fd = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (config->event_set.fd == NULL) {
			SMI_ERROR("CreateEvent call failed. GetLastError(): %d, Return code: %d", GetLastError(), AMDSMI_STATUS_API_FAILED);
			sys_wrapper->smi_free(event_set_handle->devices);
			sys_wrapper->smi_free(event_set_handle->handles);
			sys_wrapper->smi_free(event_set_handle);
			return AMDSMI_STATUS_API_FAILED;
		}
#endif
		const int code = amdsmi_request(&smi_req, SMI_CMD_CODE_CREATE_EVENT,
					     sizeof(struct smi_event_set_config),
					     sizeof(smi_event_handle_t));
		if (code != AMDSMI_STATUS_SUCCESS) {
#ifdef _WIN64
			sys_wrapper->smi_close(config->event_set.fd);
#endif
			for (uint32_t j = 0; j <= i; ++j) {
				sys_wrapper->smi_close(event_set_handle->handles[j].fd);
			}
			sys_wrapper->smi_free(event_set_handle->devices);
			sys_wrapper->smi_free(event_set_handle->handles);
			sys_wrapper->smi_free(event_set_handle);
			SMI_ERROR("Ioctl call failed. Return code: %d", code);
			return code;
		}
		event_set_handle->devices[i].handle = gpu->handle;
		event_set_handle->devices[i].device_id = gpu->dev_id;
#ifdef _WIN64
		event_set_handle->handles[i].as_ptr = config->event_set.fd;
#else
		event_set_handle->handles[i].as_ptr = event_set->as_ptr;
#endif
	}
	event_set_handle->_private = sys_wrapper->smi_poll_alloc(event_set_handle->handles,
							     event_set_handle->num_handles);
	*set = event_set_handle;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_event_read(amdsmi_event_set set, int64_t timeout_usec, amdsmi_event_entry_t *event)
{
	#pragma SMI_EXPORT
	system_wrapper *sys_wrapper = get_system_wrapper();
	struct smi_event_set_s *event_set = (struct smi_event_set_s *)set;
	struct smi_event_entry *info = NULL;
#ifdef _WIN64
	struct smi_device_info *gpu = NULL;
	smi_req_ctx smi_req;
	uint32_t signaled_device_index;
	AMDSMI_ESCAPE_IF_NOT_INIT;
#else
	struct smi_event_entry event_entry;
	memset(&event_entry, 0, sizeof(struct smi_event_entry));
	info = &event_entry;
#endif


	if (set == NULL || event == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	int poll_res = sys_wrapper->smi_poll(event_set, info, timeout_usec);

	if (poll_res == AMDSMI_STATUS_TIMEOUT) {
		SMI_ERROR("Result of poll call is AMDSMI_STATUS_TIMEOUT. Return code: %d", AMDSMI_STATUS_TIMEOUT);
		return AMDSMI_STATUS_TIMEOUT;
	} else if (poll_res != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Poll call failed. Return code: %d", AMDSMI_STATUS_API_FAILED);
		return AMDSMI_STATUS_API_FAILED;
	}
#ifdef _WIN64
	gpu = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	signaled_device_index = event_set->signaled_device_index;
	gpu->dev_id.handle = event_set->devices[signaled_device_index].handle;

	const int code = amdsmi_request(&smi_req, SMI_CMD_CODE_READ_EVENT,
						sizeof(struct smi_device_info),
						sizeof(struct smi_event_entry));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	info = (struct smi_event_entry *)&smi_req.thread->ioctl_cmd.payload;
#endif

	memset(event, 0, sizeof(amdsmi_event_entry_t));
	event->timestamp = info->timestamp;
	event->category = info->category;
	event->subcode = info->subcode;
	event->level = info->level;
	event->data = info->data;
	event->fcn_id.handle = info->fcn_id.handle;
	memcpy(event->date, info->date, sizeof(info->date));
	memcpy(event->message, info->message, sizeof(info->message));
	for (uint32_t i = 0; i < g_gpu_device_handles.num_gpus; ++i) {
		if (info->processor_handle.handle == g_gpu_device_handles.gpus[i].handle) {
			event->processor_handle = &(g_gpu_device_handles.gpus[i]);
			event->dev_id = g_gpu_device_handles.gpus[i].dev_id;
			break;
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_event_destroy(amdsmi_event_set set)
{
	#pragma SMI_EXPORT
	system_wrapper *sys_wrapper = get_system_wrapper();
	struct smi_event_set_s *amdsmi_event_set = (struct smi_event_set_s *)set;
#ifdef _WIN64
	struct smi_device_info *gpu = NULL;
	smi_req_ctx smi_req;
	AMDSMI_ESCAPE_IF_NOT_INIT;
#endif
	if (set == NULL) {
		return AMDSMI_STATUS_SUCCESS;
	}

	for (uint32_t i = 0; i < amdsmi_event_set->num_handles; ++i) {
#ifdef _WIN64
		gpu = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
		gpu->dev_id.handle = amdsmi_event_set->devices[i].handle;
		const int code = amdsmi_request(&smi_req, SMI_CMD_CODE_DESTROY_EVENT,
					     sizeof(struct smi_device_info),
					     0);

		if (code != AMDSMI_STATUS_SUCCESS) {
			SMI_ERROR("Ioctl call failed. Return code: %d", code);
			return code;
		}
#endif
		sys_wrapper->smi_close(amdsmi_event_set->handles[i].fd);
	}

	sys_wrapper->smi_free(amdsmi_event_set->devices);
	sys_wrapper->smi_free(amdsmi_event_set->handles);
	sys_wrapper->smi_free(amdsmi_event_set->_private);
	sys_wrapper->smi_free(amdsmi_event_set);

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_guest_data(amdsmi_vf_handle_t vf_handle, amdsmi_guest_data_t *info)
{
	#pragma SMI_EXPORT
	struct smi_guest_info *guest_info = NULL;
	smi_req_ctx smi_req;
	struct smi_device_info *vf;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	vf = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	vf->dev_id.handle = vf_handle.handle;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_GUEST_DATA,
				     sizeof(struct smi_device_info),
				     sizeof(struct smi_guest_info));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	guest_info = (struct smi_guest_info *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_guest_data_t));
	memcpy(info->driver_version, guest_info->guest_data.driver_version, sizeof(uint8_t) * AMDSMI_MAX_STRING_LENGTH);
	info->fb_usage = guest_info->guest_data.fb_usage;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_vf_fw_info(amdsmi_vf_handle_t vf_handle, amdsmi_fw_info_t *info)
{
	#pragma SMI_EXPORT
	struct smi_device_info *vf;
	amdsmi_fw_info_t *ucode_info = NULL;
	smi_req_ctx smi_req;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	vf = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	vf->dev_id.handle = vf_handle.handle;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_VF_UCODE_INFO,
				sizeof(struct smi_device_info),
				sizeof(struct smi_fw_info));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	ucode_info = (amdsmi_fw_info_t *)&smi_req.thread->ioctl_cmd.payload;

	memset(info, 0, sizeof(amdsmi_fw_info_t));
	info->num_fw_info = ucode_info->num_fw_info;
	for (uint32_t i = 0; i < ucode_info->num_fw_info; i++) {
		info->fw_info_list[i].fw_id = ucode_info->fw_info_list[i].fw_id;
		info->fw_info_list[i].fw_version = ucode_info->fw_info_list[i].fw_version;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_partition_profile_info(amdsmi_processor_handle processor_handle, amdsmi_profile_info_t *profile_info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	struct smi_device_info *dev = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	amdsmi_profile_info_t *partition_profile_info = NULL;
	uint32_t i, j;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || profile_info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id = pf;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_PARTITION_PROFILE_INFO,
				     sizeof(struct smi_device_info),
				     sizeof(struct smi_profile_info));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	partition_profile_info = (amdsmi_profile_info_t *)&smi_req.thread->ioctl_cmd.payload;

	memset(profile_info, 0, sizeof(amdsmi_profile_info_t));
	profile_info->profile_count = partition_profile_info->profile_count;
	profile_info->current_profile_index = partition_profile_info->current_profile_index;

	for (i = 0; i < profile_info->profile_count; i++) {
		profile_info->profiles[i].vf_count =
			partition_profile_info->profiles[i].vf_count;
		for (j = 0; j < AMDSMI_PROFILE_CAPABILITY__MAX; j++) {
			profile_info->profiles[i].profile_caps[j].total =
				partition_profile_info->profiles[i].profile_caps[j].total;
			profile_info->profiles[i].profile_caps[j].available =
				partition_profile_info->profiles[i].profile_caps[j].available;
			profile_info->profiles[i].profile_caps[j].optimal =
				partition_profile_info->profiles[i].profile_caps[j].optimal;
			profile_info->profiles[i].profile_caps[j].min_value =
				partition_profile_info->profiles[i].profile_caps[j].min_value;
			profile_info->profiles[i].profile_caps[j].max_value =
				partition_profile_info->profiles[i].profile_caps[j].max_value;
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_link_metrics(amdsmi_processor_handle processor_handle,
					amdsmi_link_metrics_t *link_metrics)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	struct smi_device_info *dev = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	struct smi_link_metrics *link = NULL;
	uint32_t i = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || link_metrics == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id = pf;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_LINK_METRICS,
				     sizeof(struct smi_device_info),
				     sizeof(struct smi_link_metrics));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	link = (struct smi_link_metrics *)&smi_req.thread->ioctl_cmd.payload;

	memset(link_metrics, 0, sizeof(amdsmi_link_metrics_t));
	link_metrics->num_links = link->num_links;
	for (i = 0; i < link_metrics->num_links; i++) {
		link_metrics->links[i].bdf.as_uint = link->links[i].bdf.as_uint;
		link_metrics->links[i].bit_rate = link->links[i].bit_rate;
		link_metrics->links[i].max_bandwidth = link->links[i].max_bandwidth;
		link_metrics->links[i].link_type = (amdsmi_link_type_t)link->links[i].link_type;
		link_metrics->links[i].read = link->links[i].read;
		link_metrics->links[i].write = link->links[i].write;
		link_metrics->links[i].link_status = (amdsmi_link_status_t)link->links[i].link_status;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_link_topology(amdsmi_processor_handle processor_handle_src,
					 amdsmi_processor_handle processor_handle_dst,
					 amdsmi_link_topology_t *topology_info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_device_pair_info *gpu_pair = NULL;
	struct smi_gpu_handle *src_gpu = NULL;
	struct smi_gpu_handle *dst_gpu = NULL;
	enum smi_handle_type type;
	struct smi_io_link *link = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle_src == NULL || processor_handle_dst == NULL || topology_info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle_src);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong source processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle_dst);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong destination processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	src_gpu = ((struct smi_gpu_handle *)processor_handle_src);
	dst_gpu = ((struct smi_gpu_handle *)processor_handle_dst);

	if (is_cmd_supported(src_gpu->dev_id) == AMDSMI_STATUS_NOT_SUPPORTED) {
		SMI_ERROR("Command not supported for source device. Return code: %d", AMDSMI_STATUS_NOT_SUPPORTED);
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}

	if (is_cmd_supported(dst_gpu->dev_id) == AMDSMI_STATUS_NOT_SUPPORTED) {
		SMI_ERROR("Command not supported for destination device. Return code: %d", AMDSMI_STATUS_NOT_SUPPORTED);
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}

	if (src_gpu->handle == dst_gpu->handle) {
		topology_info->weight = 0;
		topology_info->link_status = AMDSMI_LINK_STATUS_ENABLED;
		topology_info->link_type = AMDSMI_LINK_TYPE_NOT_APPLICABLE;
		topology_info->num_hops = 0;
		topology_info->fb_sharing = 1;
		return AMDSMI_STATUS_SUCCESS;
	}

	gpu_pair = (struct smi_device_pair_info *)&smi_req.thread->ioctl_cmd.payload;
	gpu_pair->src.dev_id.handle = src_gpu->handle;
	gpu_pair->dst.dev_id.handle = dst_gpu->handle;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_LINK_TOPOLOGY,
				     sizeof(struct smi_device_pair_info),
				     sizeof(struct smi_io_link));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	link = (struct smi_io_link *)&smi_req.thread->ioctl_cmd.payload;

	memset(topology_info, 0, sizeof(amdsmi_link_topology_t));
	topology_info->weight = link->link_topology.weight;
	topology_info->link_status = (amdsmi_link_status_t)link->link_topology.link_status;
	topology_info->link_type = (amdsmi_link_type_t)link->link_topology.link_type;
	topology_info->num_hops = link->link_topology.num_hops;
	topology_info->fb_sharing = link->link_topology.fb_sharing;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_link_topology_nearest(amdsmi_processor_handle processor_handle,
						amdsmi_link_type_t link_type,
						amdsmi_topology_nearest_t *topology_nearest_info)
{
	#pragma SMI_EXPORT
	int code;
	uint32_t index = 0;
	int min_hops = -1;
	uint32_t j = 0;
	amdsmi_processor_handle devices_list[AMDSMI_MAX_DEVICES];
	uint8_t devices_hops[AMDSMI_MAX_DEVICES];
	amdsmi_link_topology_t topology_info;
	enum smi_handle_type type;

	if (processor_handle == NULL || topology_nearest_info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	switch (link_type) {
	case AMDSMI_LINK_TYPE_XGMI:
	case AMDSMI_LINK_TYPE_PCIE:
		break;
	case AMDSMI_LINK_TYPE_INTERNAL:
	case AMDSMI_LINK_TYPE_NOT_APPLICABLE:
	case AMDSMI_LINK_TYPE_UNKNOWN:
		SMI_ERROR("Type of link, given as input, is not supported. Return code: %d", AMDSMI_STATUS_NOT_SUPPORTED);
		return AMDSMI_STATUS_NOT_SUPPORTED;
	default:
		SMI_ERROR("Type of link, given as input, is not valid. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}
	for (uint32_t i = 0; i < g_gpu_device_handles.num_gpus; i++) {
		code = amdsmi_get_link_topology(processor_handle, &g_gpu_device_handles.gpus[i], &topology_info);
		if (code != AMDSMI_STATUS_SUCCESS) {
			SMI_ERROR("Function call failed. Return code: %d", code);
			return code;
		}
		if (topology_info.link_type == link_type) {
			if (min_hops == -1) {
				min_hops = topology_info.num_hops;
			} else if (topology_info.num_hops < min_hops) {
				min_hops = topology_info.num_hops;
			}
			devices_list[index] = (amdsmi_processor_handle)&g_gpu_device_handles.gpus[i];
			devices_hops[index] = topology_info.num_hops;
			index++;
		}
	}
	if (min_hops == -1) {
		topology_nearest_info->count = 0;
		return AMDSMI_STATUS_NO_DATA;
	}
	for (uint32_t i = 0; i < index; i++) {
		if (devices_hops[i] == min_hops) {
			topology_nearest_info->processor_list[j] = devices_list[i];
			j++;
		}
	}
	topology_nearest_info->count = j;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_xgmi_fb_sharing_caps(amdsmi_processor_handle processor_handle,
						amdsmi_xgmi_fb_sharing_caps_t *caps)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	struct smi_device_info *dev = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	union smi_xgmi_fb_sharing_caps *xgmi_caps = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || caps == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id = pf;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_XGMI_FB_SHARING_CAPS,
				     sizeof(struct smi_device_info),
				     sizeof(union smi_xgmi_fb_sharing_caps));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	xgmi_caps = (union smi_xgmi_fb_sharing_caps *)&smi_req.thread->ioctl_cmd.payload;
	memset(caps, 0, sizeof(amdsmi_xgmi_fb_sharing_caps_t));
	caps->xgmi_fb_sharing_cap_mask = xgmi_caps->xgmi_fb_sharing_cap_mask;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_xgmi_fb_sharing_mode_info(amdsmi_processor_handle processor_handle_src,
						     amdsmi_processor_handle processor_handle_dst,
						     amdsmi_xgmi_fb_sharing_mode_t mode,
						     uint8_t *fb_sharing)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_xgmi_fb_sharing *xgmi = NULL;
	struct smi_xgmi_fb_sharing_flag *fb_sharing_flag = NULL;
	struct smi_gpu_handle *src_gpu = NULL;
	struct smi_gpu_handle *dst_gpu = NULL;
	enum smi_handle_type type;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle_src == NULL || processor_handle_dst == NULL || fb_sharing == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle_src);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong source processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle_dst);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong destination processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	src_gpu = ((struct smi_gpu_handle *)processor_handle_src);
	dst_gpu = ((struct smi_gpu_handle *)processor_handle_dst);

	if (is_cmd_supported(src_gpu->dev_id) == AMDSMI_STATUS_NOT_SUPPORTED) {
		SMI_ERROR("Command not supported for source device. Return code: %d", AMDSMI_STATUS_NOT_SUPPORTED);
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}

	if (is_cmd_supported(dst_gpu->dev_id) == AMDSMI_STATUS_NOT_SUPPORTED) {
		SMI_ERROR("Command not supported for destination device. Return code: %d", AMDSMI_STATUS_NOT_SUPPORTED);
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}

	if (src_gpu->handle == dst_gpu->handle) {
		*fb_sharing = 1;
		return AMDSMI_STATUS_SUCCESS;
	}

	xgmi = (struct smi_xgmi_fb_sharing *)&smi_req.thread->ioctl_cmd.payload;
	xgmi->src_dev.handle = src_gpu->handle;
	xgmi->dst_dev.handle = dst_gpu->handle;
	xgmi->mode = mode;

	int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_XGMI_FB_SHARING_MODE_INFO,
				     sizeof(struct smi_xgmi_fb_sharing),
				     sizeof(struct smi_xgmi_fb_sharing_flag));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	fb_sharing_flag = (struct smi_xgmi_fb_sharing_flag *)&smi_req.thread->ioctl_cmd.payload;
	*fb_sharing = fb_sharing_flag->fb_sharing;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_set_xgmi_fb_sharing_mode(amdsmi_processor_handle processor_handle,
						amdsmi_xgmi_fb_sharing_mode_t mode)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	struct smi_set_xgmi_fb_sharing_mode *xgmi_mode = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	xgmi_mode = (struct smi_set_xgmi_fb_sharing_mode *)&smi_req.thread->ioctl_cmd.payload;
	xgmi_mode->dev_id = pf;
	xgmi_mode->mode = mode;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_SET_XGMI_FB_SHARING_MODE,
				     sizeof(struct smi_set_xgmi_fb_sharing_mode), 0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_set_xgmi_fb_sharing_mode_v2(amdsmi_processor_handle *processor_list, uint32_t num_processors,
						amdsmi_xgmi_fb_sharing_mode_t mode)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_set_xgmi_fb_custom_sharing_mode *xgmi_share_mode = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	amdsmi_status_t ret = AMDSMI_STATUS_SUCCESS;
	uint32_t i;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_list == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	if (mode == AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM) {
		if (num_processors == 0) {
			SMI_ERROR("Insufficient buffer size given as input. Return code: %d", AMDSMI_STATUS_INVAL);
			return AMDSMI_STATUS_INVAL;
		}
	} else {
		if (num_processors != 1) {
			SMI_ERROR("Wrong buffer size given as input. Return code: %d", AMDSMI_STATUS_INVAL);
			return AMDSMI_STATUS_INVAL;
		}
		if (processor_list[0] == NULL) {
			SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
			return AMDSMI_STATUS_INVAL;
		}
		ret = amdsmi_set_xgmi_fb_sharing_mode(processor_list[0], mode);
		return ret;
	}

	for (uint32_t i = 0; i < num_processors; i++) {
		for (uint32_t j = i+1; j < num_processors; j++) {
			if (processor_list[i] == processor_list[j]) {
				return AMDSMI_STATUS_INVAL;
			}
		}
	}

	xgmi_share_mode = (struct smi_set_xgmi_fb_custom_sharing_mode *)&smi_req.thread->ioctl_cmd.payload;

	xgmi_share_mode->num_processors = num_processors;
	for (i = 0; i < num_processors; i++) {
		type = *((enum smi_handle_type *)processor_list[i]);
		if (type != SMI_HANDLE_TYPE_AMD_GPU) {
			SMI_ERROR("Wrong processor handle [%d]. Return code: %d", i, AMDSMI_STATUS_INVAL);
			return AMDSMI_STATUS_INVAL;
		}
		gpu = ((struct smi_gpu_handle *)processor_list[i]);
		xgmi_share_mode->dev_id_list[i].handle = gpu->handle;
	}
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_SET_XGMI_FB_SHARING_MODE_VER_2,
				     sizeof(struct smi_set_xgmi_fb_custom_sharing_mode), 0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_metrics(amdsmi_processor_handle processor_handle, uint32_t *metrics_size,
					amdsmi_metric_t *metrics)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	struct smi_metrics_table *table = NULL;
	struct smi_metrics *metrics_table = NULL;
	system_wrapper *sys_wrapper = get_system_wrapper();

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || metrics_size == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	if (*metrics_size == 0) {
		SMI_ERROR("Insufficient buffer size given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	if (*metrics_size > AMDSMI_MAX_NUM_METRICS) {
		*metrics_size = AMDSMI_MAX_NUM_METRICS;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	table = (struct smi_metrics_table *)&smi_req.thread->ioctl_cmd.payload;
	table->dev_id = pf;
#ifdef _WIN64
	metrics_table = sys_wrapper->smi_calloc(1, sizeof(struct smi_metrics));
#else
	long page_size;
	size_t alloc_size;
	page_size = sys_wrapper->smi_sysconf(_SC_PAGESIZE);
	if (page_size == -1) {
		SMI_ERROR("Failed to get system configuration. Couldn't get page size. Return code: %d", AMDSMI_STATUS_API_FAILED);
		return AMDSMI_STATUS_API_FAILED;
	}
	// Round up size to be a multiple of page_size for aligned_alloc
	alloc_size = ((sizeof(struct smi_metrics) + (size_t)page_size - 1) / (size_t)page_size) * (size_t)page_size;
	metrics_table = sys_wrapper->smi_aligned_alloc((void **)&metrics_table, (size_t)page_size, alloc_size);
	if (metrics_table != NULL) {
		memset(metrics_table, 0, sizeof(struct smi_metrics));
	}
#endif
	if (metrics_table == NULL) {
		SMI_ERROR("Memory allocation call failed for metrics table. Return code: %d", AMDSMI_STATUS_OUT_OF_RESOURCES);
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}

	table->metrics = metrics_table;
	table->size = *metrics_size;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_METRICS_TABLE,
				     sizeof(struct smi_metrics_table),
				     0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		sys_wrapper->smi_free(metrics_table);
		return code;
	}

	if (*metrics_size < table->metrics->num_metric) {
		SMI_ERROR("Size of the allocated buffer is not sufficient. Return code: %d", AMDSMI_STATUS_OUT_OF_RESOURCES);
		sys_wrapper->smi_free(metrics_table);
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}

	if (metrics == NULL) {
		*metrics_size = table->metrics->num_metric;
	} else {
		memset(metrics, 0, table->metrics->num_metric*sizeof(amdsmi_metric_t));
		*metrics_size = table->metrics->num_metric;
		for (uint32_t i = 0; i < table->metrics->num_metric; i++) {
			metrics[i].flags = 0;
			metrics[i].category = (amdsmi_metric_category_t)table->metrics->metric[i].metric_union.metric.category;
			metrics[i].name = (amdsmi_metric_name_t)table->metrics->metric[i].metric_union.metric.name;
			metrics[i].unit = (amdsmi_metric_unit_t)table->metrics->metric[i].metric_union.metric.unit;
			metrics[i].vf_mask = table->metrics->metric[i].vf_mask;
			metrics[i].val = table->metrics->metric[i].val;
			metrics[i].flags |= table->metrics->metric[i].metric_union.code & (1ULL << SMI_METRIC_TYPE_COUNTER) ?
					    AMDSMI_METRIC_TYPE_COUNTER : 0;
			metrics[i].flags |= table->metrics->metric[i].metric_union.code & (1ULL << SMI_METRIC_TYPE_CHIPLET) ?
					    AMDSMI_METRIC_TYPE_CHIPLET : 0;
			metrics[i].flags |= table->metrics->metric[i].metric_union.code & (1ULL << SMI_METRIC_TYPE_INST) ?
					    AMDSMI_METRIC_TYPE_INST : 0;
			metrics[i].flags |= table->metrics->metric[i].metric_union.code & (1ULL << SMI_METRIC_TYPE_ACC) ?
					    AMDSMI_METRIC_TYPE_ACC : 0;
			metrics[i].res_group = (amdsmi_metric_res_group_t)table->metrics->metric[i].metric_union.metric.res_group;
			metrics[i].res_subgroup = (amdsmi_metric_res_subgroup_t)table->metrics->metric[i].metric_union.metric.res_subgroup;
			metrics[i].res_instance = table->metrics->metric[i].res_instance;
		}
	}

	sys_wrapper->smi_free(metrics_table);

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_lib_version(amdsmi_version_t *version)
{
	#pragma SMI_EXPORT

	if (version == NULL)
		return AMDSMI_STATUS_INVAL;

	version->major = AMDSMI_VERSION_MAJOR;
	version->minor = AMDSMI_VERSION_MINOR;
	version->release = AMDSMI_VERSION_RELEASE;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_virtualization_mode(amdsmi_processor_handle processor_handle, amdsmi_virtualization_mode_t *mode)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	enum smi_handle_type type;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || mode == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	*mode = AMDSMI_VIRTUALIZATION_MODE_HOST;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_memory_partition_config(amdsmi_processor_handle processor_handle,
										 amdsmi_memory_partition_config_t *config)
{
	#pragma SMI_EXPORT
	amdsmi_memory_partition_config_t *memory_setting = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	struct smi_device_info *dev = NULL;
	smi_req_ctx smi_req;
	uint32_t i;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || config == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id = pf;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_CURR_MEMORY_PARTITION_SETTING,
						sizeof(struct smi_device_info),
						sizeof(struct smi_memory_partition_config));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	memory_setting = (amdsmi_memory_partition_config_t *)&smi_req.thread->ioctl_cmd.payload;

	memset(config, 0, sizeof(amdsmi_memory_partition_config_t));
	config->partition_caps.nps_cap_mask = memory_setting->partition_caps.nps_cap_mask;
	config->mp_mode = memory_setting->mp_mode;
	config->num_numa_ranges = memory_setting->num_numa_ranges;
	for (i = 0; i < config->num_numa_ranges; i++) {
		config->numa_range[i].memory_type = memory_setting->numa_range[i].memory_type;
		config->numa_range[i].start = memory_setting->numa_range[i].start;
		config->numa_range[i].end = memory_setting->numa_range[i].end;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_set_gpu_memory_partition_mode(amdsmi_processor_handle processor_handle,
										amdsmi_memory_partition_type_t mode)
{
	#pragma SMI_EXPORT
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	struct smi_set_gpu_memory_partition_setting *partition_mode = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	switch (mode) {
	case AMDSMI_MEMORY_PARTITION_NPS1:
	case AMDSMI_MEMORY_PARTITION_NPS2:
	case AMDSMI_MEMORY_PARTITION_NPS4:
	case AMDSMI_MEMORY_PARTITION_NPS8:
		break; // valid setting, do nothing
	default:
		SMI_ERROR("Invalid setting given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	partition_mode = (struct smi_set_gpu_memory_partition_setting *)&smi_req.thread->ioctl_cmd.payload;
	partition_mode->dev_id = pf;
	partition_mode->mode = (enum smi_memory_partition_type)mode;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_SET_GPU_MEMORY_PARTITION_SETTING,
								sizeof(struct smi_set_gpu_memory_partition_setting), 0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_accelerator_partition_profile_config(amdsmi_processor_handle processor_handle,
											amdsmi_accelerator_partition_profile_config_t *profile_config)
{
	#pragma SMI_EXPORT
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	struct smi_profile_configs *accelerator_profile_configs = NULL;
	amdsmi_accelerator_partition_profile_config_t *configs = NULL;
	system_wrapper *sys_wrapper = get_system_wrapper();
	uint32_t i, j, k;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || profile_config == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	accelerator_profile_configs = (struct smi_profile_configs *)&smi_req.thread->ioctl_cmd.payload;
	accelerator_profile_configs->dev_id = pf;
#ifdef _WIN64
	configs = sys_wrapper->smi_calloc(1, sizeof(amdsmi_accelerator_partition_profile_config_t));
#else
	long page_size;
	size_t alloc_size;
	page_size = sys_wrapper->smi_sysconf(_SC_PAGESIZE);
	if (page_size == -1) {
		SMI_ERROR("Failed to get system configuration. Couldn't get page size. Return code: %d", AMDSMI_STATUS_API_FAILED);
		return AMDSMI_STATUS_API_FAILED;
	}
	// Round up size to be a multiple of page_size for aligned_alloc
	alloc_size = ((sizeof(amdsmi_accelerator_partition_profile_config_t) + (size_t)page_size - 1) / (size_t)page_size) * (size_t)page_size;
	configs = sys_wrapper->smi_aligned_alloc((void **)&configs, (size_t)page_size, alloc_size);
#endif

	if (configs == NULL) {
		SMI_ERROR("Memory allocation call failed for accelerator partition configuration. Return code: %d", AMDSMI_STATUS_OUT_OF_RESOURCES);
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}

	memset(configs, 0, sizeof(amdsmi_accelerator_partition_profile_config_t));

	accelerator_profile_configs->profile_configs = (struct smi_accelerator_partition_profile_config *)configs;


	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_ACCELERATOR_PARTITION_PROFILE_CONFIG,
						sizeof(struct smi_profile_configs),
						0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		sys_wrapper->smi_free(configs);
		return code;
	}

	memset(profile_config, 0, sizeof(amdsmi_accelerator_partition_profile_config_t));
	profile_config->num_resource_profiles = accelerator_profile_configs->profile_configs->num_resource_profiles;
	profile_config->num_profiles = accelerator_profile_configs->profile_configs->num_profiles;
	profile_config->default_profile_index = accelerator_profile_configs->profile_configs->default_profile_index;

	for (i = 0; i < profile_config->num_resource_profiles; i++) {
		profile_config->resource_profiles[i].profile_index = accelerator_profile_configs->profile_configs->resource_profiles[i].profile_index;
		profile_config->resource_profiles[i].resource_type = (amdsmi_accelerator_partition_resource_type_t)accelerator_profile_configs->profile_configs->resource_profiles[i].resource_type;
		profile_config->resource_profiles[i].partition_resource = accelerator_profile_configs->profile_configs->resource_profiles[i].partition_resource;
		profile_config->resource_profiles[i].num_partitions_share_resource = accelerator_profile_configs->profile_configs->resource_profiles[i].num_partitions_share_resource;
	}

	for (i = 0; i < profile_config->num_profiles; i++) {
		profile_config->profiles[i].profile_type = (amdsmi_accelerator_partition_type_t)accelerator_profile_configs->profile_configs->profiles[i].profile_type;
		profile_config->profiles[i].num_partitions = accelerator_profile_configs->profile_configs->profiles[i].num_partitions;
		profile_config->profiles[i].memory_caps.nps_cap_mask = accelerator_profile_configs->profile_configs->profiles[i].memory_caps.nps_cap_mask;
		profile_config->profiles[i].profile_index = accelerator_profile_configs->profile_configs->profiles[i].profile_index;
		profile_config->profiles[i].num_resources = accelerator_profile_configs->profile_configs->profiles[i].num_resources;
		for (j = 0; j < profile_config->profiles[i].num_partitions; j++) {
			for (k = 0; k < profile_config->profiles[i].num_resources; k++) {
				profile_config->profiles[i].resources[j][k] = accelerator_profile_configs->profile_configs->profiles[i].resources[j][k];
			}
		}
	}

	sys_wrapper->smi_free(configs);
	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_accelerator_partition_profile_config_global(amdsmi_processor_handle processor_handle,
										amdsmi_accelerator_partition_profile_config_global_t *config)
{
	#pragma SMI_EXPORT
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	struct smi_profile_configs_global *accelerator_profile_configs_global = NULL;
	amdsmi_accelerator_partition_profile_config_global_t *configs = NULL;
	system_wrapper *sys_wrapper = get_system_wrapper();
	uint32_t i, j, k;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || config == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	accelerator_profile_configs_global = (struct smi_profile_configs_global *)&smi_req.thread->ioctl_cmd.payload;
	accelerator_profile_configs_global->dev_id = pf;
#ifdef _WIN64
	configs = sys_wrapper->smi_calloc(1, sizeof(amdsmi_accelerator_partition_profile_config_global_t));
#else
	long page_size;
	size_t alloc_size;
	page_size = sys_wrapper->smi_sysconf(_SC_PAGESIZE);
	if (page_size == -1) {
		SMI_ERROR("Failed to get system configuration. Couldn't get page size. Return code: %d", AMDSMI_STATUS_API_FAILED);
		return AMDSMI_STATUS_API_FAILED;
	}
	// Round up size to be a multiple of page_size for aligned_alloc
	alloc_size = ((sizeof(amdsmi_accelerator_partition_profile_config_global_t) + (size_t)page_size - 1) / (size_t)page_size) * (size_t)page_size;
	configs = sys_wrapper->smi_aligned_alloc((void **)&configs, (size_t)page_size, alloc_size);
#endif

	if (configs == NULL) {
		SMI_ERROR("Memory allocation call failed for accelerator partition configuration. Return code: %d", AMDSMI_STATUS_OUT_OF_RESOURCES);
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}

	memset(configs, 0, sizeof(amdsmi_accelerator_partition_profile_config_global_t));

	accelerator_profile_configs_global->profile_configs = (struct smi_accelerator_partition_profile_config_global *)configs;


	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_ACCELERATOR_PARTITION_PROFILE_CONFIG_GLOBAL,
						sizeof(struct smi_profile_configs_global),
						0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		sys_wrapper->smi_free(configs);
		return code;
	}

	memset(config, 0, sizeof(amdsmi_accelerator_partition_profile_config_global_t));
	config->num_profiles = accelerator_profile_configs_global->profile_configs->num_profiles;
	config->num_resource_profiles = accelerator_profile_configs_global->profile_configs->num_resource_profiles;
	config->default_profile_index = accelerator_profile_configs_global->profile_configs->default_profile_index;


	for (i = 0; i < config->num_resource_profiles; i++) {
		config->resource_profiles[i].profile_index = accelerator_profile_configs_global->profile_configs->resource_profiles[i].profile_index;
		config->resource_profiles[i].resource_type = (amdsmi_accelerator_partition_resource_type_t)accelerator_profile_configs_global->profile_configs->resource_profiles[i].resource_type;
		config->resource_profiles[i].partition_resource = accelerator_profile_configs_global->profile_configs->resource_profiles[i].partition_resource;
		config->resource_profiles[i].num_partitions_share_resource = accelerator_profile_configs_global->profile_configs->resource_profiles[i].num_partitions_share_resource;
	}

	for (i = 0; i < config->num_profiles; i++) {
		config->profiles[i].profile.profile_type = (amdsmi_accelerator_partition_type_t)accelerator_profile_configs_global->profile_configs->profiles[i].profile.profile_type;
		config->profiles[i].profile.num_partitions = accelerator_profile_configs_global->profile_configs->profiles[i].profile.num_partitions;
		config->profiles[i].profile.memory_caps.nps_cap_mask = accelerator_profile_configs_global->profile_configs->profiles[i].profile.memory_caps.nps_cap_mask;
		config->profiles[i].profile.profile_index = accelerator_profile_configs_global->profile_configs->profiles[i].profile.profile_index;
		config->profiles[i].profile.num_resources = accelerator_profile_configs_global->profile_configs->profiles[i].profile.num_resources;
		for (j = 0; j < config->profiles[i].profile.num_partitions; j++) {
			for (k = 0; k < config->profiles[i].profile.num_resources; k++) {
				config->profiles[i].profile.resources[j][k] = accelerator_profile_configs_global->profile_configs->profiles[i].profile.resources[j][k];
			}
		}
		config->profiles[i].vf_mode = accelerator_profile_configs_global->profile_configs->profiles[i].vf_mode;
	}

	sys_wrapper->smi_free(configs);
	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_accelerator_partition_profile(amdsmi_processor_handle processor_handle,
											amdsmi_accelerator_partition_profile_t *profile,  uint32_t *partition_id)
{
	#pragma SMI_EXPORT
	smi_device_handle_t pf;
	struct smi_device_info *dev = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	struct smi_accelerator_partition_profile_cap *config = NULL;
	uint32_t i, j;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || profile == NULL || partition_id == NULL) {
		SMI_ERROR("Processor handle is NULL. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id = pf;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_GPU_ACCELERATOR_PARTITION,
						sizeof(struct smi_device_info),
						sizeof(struct smi_accelerator_partition_profile_cap));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	config = (struct smi_accelerator_partition_profile_cap *)&smi_req.thread->ioctl_cmd.payload;

	memset(profile, 0, sizeof(amdsmi_accelerator_partition_profile_t));
	memset(partition_id, 0, profile->num_partitions*sizeof(uint32_t));
	profile->profile_type = (amdsmi_accelerator_partition_type_t)config->config.profile_type;
	profile->num_partitions = config->config.num_partitions;
	profile->memory_caps.nps_cap_mask = config->config.memory_caps.nps_cap_mask;
	profile->profile_index = config->config.profile_index;
	profile->num_resources = config->config.num_resources;

	for (i = 0; i < profile->num_partitions; i++) {
		partition_id[i] = config->partition_id[i];
		for (j = 0; j < profile->num_resources; j++) {
			profile->resources[i][j] = config->config.resources[i][j];
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_set_gpu_accelerator_partition_profile(amdsmi_processor_handle processor_handle,
										 uint32_t profile_index)
{
	#pragma SMI_EXPORT
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	struct smi_set_gpu_accelerator_partition_setting *partition_mode = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	partition_mode = (struct smi_set_gpu_accelerator_partition_setting *)&smi_req.thread->ioctl_cmd.payload;
	partition_mode->dev_id = pf;
	partition_mode->index = profile_index;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_SET_GPU_ACCELERATOR_PARTITION_SETTING,
								sizeof(struct smi_set_gpu_accelerator_partition_setting), 0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_soc_pstate(amdsmi_processor_handle processor_handle,
					amdsmi_dpm_policy_t *policy)
{
	#pragma SMI_EXPORT
	struct smi_dpm_policy *dpm_policy = NULL;
	smi_device_handle_t pf;
	struct smi_device_info *dev = NULL;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;
	system_wrapper *sys_wrapper = get_system_wrapper();

	uint32_t i;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || policy == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id = pf;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_SOC_PSTATE,
					sizeof(struct smi_device_info),
					sizeof(struct smi_dpm_policy));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	dpm_policy = (struct smi_dpm_policy *)&smi_req.thread->ioctl_cmd.payload;
	memset(policy, 0, sizeof(amdsmi_dpm_policy_t));

	policy->num_supported = dpm_policy->num_supported;
	policy->current = dpm_policy->cur;

	for (i = 0; i < dpm_policy->num_supported; i++) {
		policy->policies[i].policy_id = dpm_policy->policies[i].policy_id;
		sys_wrapper->smi_strncpy(policy->policies[i].policy_description, sizeof(policy->policies[i].policy_description), dpm_policy->policies[i].policy_description, AMDSMI_MAX_STRING_LENGTH);
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_set_soc_pstate(amdsmi_processor_handle processor_handle,
					uint32_t policy_id)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	struct smi_set_dpm_policy *dpm_policy_id = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dpm_policy_id = (struct smi_set_dpm_policy *)&smi_req.thread->ioctl_cmd.payload;
	dpm_policy_id->dev_id = pf;
	dpm_policy_id->policy_id = policy_id;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_SET_SOC_PSTATE,
				     sizeof(struct smi_set_dpm_policy), 0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_cper_entries(amdsmi_processor_handle processor_handle, uint32_t severity_mask,
    char *cper_data, uint64_t *buf_size, amdsmi_cper_hdr_t** cper_hdrs, uint64_t *entry_count, uint64_t *cursor)
{
	#pragma SMI_EXPORT

	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	enum smi_handle_type type;
	smi_req_ctx smi_req;

	struct smi_cper_config *cper_config = NULL;
	struct smi_cper *cper = NULL;
	amdsmi_cper_hdr_t *hdr = NULL;
	uint32_t entries_count = 0;
	uint32_t real_buffer_size = 0;
	system_wrapper *sys_wrapper = get_system_wrapper();

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || cper_data == NULL || buf_size == NULL || cper_hdrs == NULL ||
		entry_count == NULL || cursor == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	cper_config = (struct smi_cper_config *)&smi_req.thread->ioctl_cmd.payload;
	cper_config->dev_id = pf;
	cper_config->severity_mask = severity_mask;
	cper_config->input_cursor = *cursor;
#ifdef _WIN64
	cper = sys_wrapper->smi_calloc(1, sizeof(struct smi_cper));
#else
	long page_size;
	size_t alloc_size;
	page_size = sys_wrapper->smi_sysconf(_SC_PAGESIZE);
	if (page_size == -1) {
		SMI_ERROR("Failed to get system configuration. Couldn't get page size. Return code: %d", AMDSMI_STATUS_API_FAILED);
		return AMDSMI_STATUS_API_FAILED;
	}
	// Round up size to be a multiple of page_size for aligned_alloc
	alloc_size = ((sizeof(struct smi_cper) + (size_t)page_size - 1) / (size_t)page_size) * (size_t)page_size;
	cper = sys_wrapper->smi_aligned_alloc((void **)&cper, (size_t)page_size, alloc_size);
#endif
	if (cper == NULL) {
		SMI_ERROR("Memory allocation call failed for cper. Return code: %d", AMDSMI_STATUS_OUT_OF_RESOURCES);
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}
	memset(cper, 0, sizeof(struct smi_cper));

	cper_config->cper = cper;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_CPER,
					sizeof(struct smi_cper_config),
					0);
	if (code != AMDSMI_STATUS_SUCCESS && code != AMDSMI_STATUS_MORE_DATA) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		sys_wrapper->smi_free(cper);
		return code;
	}

	*cursor = cper_config->cper->cursor;

	for (uint32_t i = 0; i < cper_config->cper->entry_count; i++) {
		hdr = (amdsmi_cper_hdr_t*)(cper_config->cper->cper_data + cper_config->cper->cper_hdrs[i]);
		if ((severity_mask & (1U << hdr->error_severity)) || (severity_mask == (1U << AMDSMI_CPER_SEV_NUM))) {

			memcpy(cper_data + real_buffer_size,
				cper_config->cper->cper_data + cper_config->cper->cper_hdrs[i],
				hdr->record_length);
			// Set cper_hdrs[entries_count] to point to the new offset in output buffer
			cper_hdrs[entries_count] = (amdsmi_cper_hdr_t*)(cper_data + real_buffer_size);

			entries_count++;
			real_buffer_size += hdr->record_length;
		}
	}

	*entry_count = entries_count;
	*buf_size = real_buffer_size;

	sys_wrapper->smi_free(cper);
	return code;
}


amdsmi_status_t
amdsmi_topo_get_p2p_status(amdsmi_processor_handle processor_handle_src,
				amdsmi_processor_handle processor_handle_dst,
				amdsmi_link_type_t *type, amdsmi_p2p_capability_t *cap)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_device_pair_info *gpu_pair = NULL;
	struct smi_gpu_handle *src_gpu = NULL;
	struct smi_gpu_handle *dst_gpu = NULL;
	enum smi_handle_type processor_type;
	struct smi_io_link *link = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle_src == NULL || processor_handle_dst == NULL || type == NULL || cap == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	processor_type = *((enum smi_handle_type *)processor_handle_src);
	if (processor_type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong source processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	processor_type = *((enum smi_handle_type *)processor_handle_dst);
	if (processor_type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong destination processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	src_gpu = ((struct smi_gpu_handle *)processor_handle_src);
	dst_gpu = ((struct smi_gpu_handle *)processor_handle_dst);

	if (is_cmd_supported(src_gpu->dev_id) == AMDSMI_STATUS_NOT_SUPPORTED) {
		SMI_ERROR("Command not supported for source device. Return code: %d", AMDSMI_STATUS_NOT_SUPPORTED);
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}
	if (is_cmd_supported(dst_gpu->dev_id) == AMDSMI_STATUS_NOT_SUPPORTED) {
		SMI_ERROR("Command not supported for destination device. Return code: %d", AMDSMI_STATUS_NOT_SUPPORTED);
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}

	if (src_gpu->handle == dst_gpu->handle) {
		cap->is_iolink_coherent = (uint8_t)SMI_NOT_SUPPORTED;
		cap->is_iolink_atomics_32bit = (uint8_t)SMI_NOT_SUPPORTED;
		cap->is_iolink_atomics_64bit = (uint8_t)SMI_NOT_SUPPORTED;
		cap->is_iolink_dma = (uint8_t)SMI_NOT_SUPPORTED;
		cap->is_iolink_bi_directional = (uint8_t)SMI_NOT_SUPPORTED;

		return AMDSMI_STATUS_SUCCESS;
	}

	gpu_pair = (struct smi_device_pair_info *)&smi_req.thread->ioctl_cmd.payload;
	gpu_pair->src.dev_id.handle = src_gpu->handle;
	gpu_pair->dst.dev_id.handle = dst_gpu->handle;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_LINK_TOPOLOGY,
				     sizeof(struct smi_device_pair_info),
				     sizeof(struct smi_io_link));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	link = (struct smi_io_link *)&smi_req.thread->ioctl_cmd.payload;

	memset(cap, 0, sizeof(amdsmi_p2p_capability_t));
	*type = (amdsmi_link_type_t)link->link_topology.link_type;

	cap->is_iolink_coherent = link->p2p_capability.is_iolink_coherent;
	cap->is_iolink_atomics_32bit = link->p2p_capability.is_iolink_atomics_32bit;
	cap->is_iolink_atomics_64bit = link->p2p_capability.is_iolink_atomics_64bit;
	cap->is_iolink_dma = link->p2p_capability.is_iolink_dma;
	cap->is_iolink_bi_directional = link->p2p_capability.is_iolink_bi_directional;

	return AMDSMI_STATUS_SUCCESS;
}

static const guid_t CRASHDUMP					= AMD_CRASHDUMP
static const guid_t GPU_NONSTANDARD_ERROR		= AMD_GPU_NONSTANDARD_ERROR;

amdsmi_status_t amdsmi_get_afids_from_cper(char *cper_buffer, uint32_t buf_size, uint64_t *afids, uint32_t *num_afids)
{
	#pragma SMI_EXPORT

	amdsmi_cper_hdr_t* hdr = (amdsmi_cper_hdr_t*)cper_buffer;
	uint64_t register_array[17];
	uint32_t number_of_afids = 0;
	int sec_afid = 0;

	if (cper_buffer == NULL || afids == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	amdsmi_cper_hdr_t *cper = (amdsmi_cper_hdr_t *)(cper_buffer);
	if(cper->record_length > buf_size) {
		return AMDSMI_STATUS_INVAL;
	}
	else if(cper->signature[0] != 'C' || cper->signature[1] != 'P' ||
		cper->signature[2] != 'E' || cper->signature[3] != 'R') {
		return AMDSMI_STATUS_INVAL;
	}

	memset(afids, 0, MAX_NUMBER_OF_AFIDS_PER_RECORD * sizeof(afids));

	for (uint32_t i = 0; i < hdr->sec_cnt; ++i) {
		struct cper_sec_desc *section;
		section = (struct cper_sec_desc*)(cper_buffer + sizeof(amdsmi_cper_hdr_t) + i*sizeof(struct cper_sec_desc));

		uint16_t hardware_revision = (uint16_t)((section->revision_major << 8) | section->revision_minor);
		uint32_t section_flag = section->flag_mask;

		void* section_start = cper_buffer + section->sec_offset;

		if (guid_equals(&section->sec_type, &GPU_NONSTANDARD_ERROR)) {
			// correctable cper
			struct cper_sec_nonstd_err *nonstd_err;
			nonstd_err = (struct cper_sec_nonstd_err *) section_start;

			amdsmi_get_register_array((uint8_t*)(&nonstd_err->ctx.reg_dump), CPER_ACA_REG_COUNT*sizeof(uint32_t), register_array);
			sec_afid = decode_afid(register_array, 16, section_flag, hardware_revision, nonstd_err->ctx.reg_ctx_type);
			afids[number_of_afids] = (uint64_t)sec_afid;
			number_of_afids++;
		}
		else if (guid_equals(&section->sec_type, &CRASHDUMP)) {
			// fatal cper
			struct cper_sec_crashdump_fatal *crashdump;
			crashdump = (struct cper_sec_crashdump_fatal *) section_start;
			amdsmi_get_register_array((uint8_t*)(&crashdump->body.data), sizeof(crashdump->body.data), register_array);
			sec_afid = decode_afid(register_array, 4, section_flag, hardware_revision, crashdump->body.reg_ctx_type);
			afids[number_of_afids] = (uint64_t)sec_afid;
			number_of_afids++;
		} else {
			SMI_ERROR("  Unknown Section Type\n");
			number_of_afids++;
			afids[number_of_afids] = 0xFFFFFFFF;
		}
	}

	*num_afids = number_of_afids;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_reset_gpu(amdsmi_processor_handle processor_handle)
{
	#pragma SMI_EXPORT
	struct smi_device_info *gpu = NULL;
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	enum smi_handle_type type;
	struct smi_gpu_handle *dev_handle = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	dev_handle = (struct smi_gpu_handle *)processor_handle;

	pf.handle = dev_handle->handle;
	gpu = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	gpu->dev_id.handle = pf.handle;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_RESET_GPU,
					sizeof(struct smi_device_info),
					0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_cpu_affinity_with_scope(amdsmi_processor_handle processor_handle,
		uint32_t cpu_set_size, uint64_t* cpu_set, amdsmi_affinity_scope_t scope)
{
	#pragma SMI_EXPORT
#ifdef __linux__
	char prefix[AMDSMI_MAX_STRING_LENGTH/2];
	char path[AMDSMI_MAX_STRING_LENGTH];
	FILE *f;
	char buf[AMDSMI_MAX_STRING_LENGTH];
	system_wrapper *sys_wrapper = get_system_wrapper();
	enum smi_handle_type type;
	struct smi_gpu_handle *gpu = NULL;

	if (processor_handle == NULL || cpu_set == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	if (scope != AMDSMI_AFFINITY_SCOPE_NODE) {
		SMI_ERROR("Only NUMA node affinity is currently supported.");
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	if (make_sysfs_pci_device_prefix(gpu->bdf, prefix, sizeof(prefix)) != 0) {
		SMI_ERROR("Failed to create sysfs prefix\n");
		return AMDSMI_STATUS_API_FAILED;
	}

	sys_wrapper->snprintf(path, sizeof(path), "%slocal_cpulist", prefix);

	f = sys_wrapper->fopen(path, "r");
	if (!f) {
		SMI_ERROR("Cannot open %s: %s\n", path, strerror(errno));
		return AMDSMI_STATUS_NOT_FOUND;
	}

	if (sys_wrapper->fgets(buf, sizeof(buf), f) == NULL) {
		fclose(f);
		return AMDSMI_STATUS_IO;
	}

	buf[strcspn(buf, "\n")] = 0; // Remove newline
	parse_cpu_list(buf, cpu_set, cpu_set_size);
	fclose(f);

	return AMDSMI_STATUS_SUCCESS;
#else
	AMDSMI_UNUSED(processor_handle);
	AMDSMI_UNUSED(cpu_set_size);
	AMDSMI_UNUSED(cpu_set);
	AMDSMI_UNUSED(scope);

	return AMDSMI_STATUS_NOT_SUPPORTED;
#endif
}

amdsmi_status_t amdsmi_topo_get_numa_node_number(amdsmi_processor_handle processor_handle, uint32_t *numa_node)
{
	#pragma SMI_EXPORT
#ifdef __linux__
	char prefix[AMDSMI_MAX_STRING_LENGTH/2];
	char path[AMDSMI_MAX_STRING_LENGTH];
	FILE *f;
	char buf[AMDSMI_MAX_STRING_LENGTH];
	system_wrapper *sys_wrapper = get_system_wrapper();
	enum smi_handle_type type;
	struct smi_gpu_handle *gpu = NULL;

	if (processor_handle == NULL || numa_node == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	if (make_sysfs_pci_device_prefix(gpu->bdf, prefix, sizeof(prefix)) != 0) {
		SMI_ERROR("Failed to create sysfs prefix\n");
		return AMDSMI_STATUS_API_FAILED;
	}

	sys_wrapper->snprintf(path, sizeof(path), "%snuma_node", prefix);

	f = sys_wrapper->fopen(path, "r");
	if (!f) {
		SMI_ERROR("Cannot open %s: %s\n", path, strerror(errno));
		return AMDSMI_STATUS_NOT_FOUND;
	}

	if (sys_wrapper->fgets(buf, sizeof(buf), f) == NULL) {
		fclose(f);
		return AMDSMI_STATUS_IO;
	}

	buf[strcspn(buf, "\n")] = 0; // Remove newline
	uint32_t value = (uint32_t)strtoul(buf, NULL, 10);

	*numa_node = value;

	fclose(f);

	return AMDSMI_STATUS_SUCCESS;
#else
	AMDSMI_UNUSED(processor_handle);
	AMDSMI_UNUSED(numa_node);

	return AMDSMI_STATUS_NOT_SUPPORTED;
#endif

}
amdsmi_status_t amdsmi_get_xgmi_plpd(amdsmi_processor_handle processor_handle,
					amdsmi_dpm_policy_t *xgmi_plpd)
{
	#pragma SMI_EXPORT
	struct smi_dpm_policy *dpm_policy = NULL;
	smi_device_handle_t pf;
	struct smi_device_info *gpu = NULL;
	smi_req_ctx smi_req;
	uint32_t i;
	enum smi_handle_type type;
	struct smi_gpu_handle *dev_handle = NULL;
	system_wrapper *sys_wrapper = get_system_wrapper();

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || xgmi_plpd == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	dev_handle = ((struct smi_gpu_handle *)processor_handle);
	pf.handle = dev_handle->handle;
	gpu = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	gpu->dev_id = pf;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_XGMI_PLPD,
					sizeof(struct smi_device_info),
					sizeof(struct smi_dpm_policy));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	dpm_policy = (struct smi_dpm_policy *)&smi_req.thread->ioctl_cmd.payload;
	memset(xgmi_plpd, 0, sizeof(amdsmi_dpm_policy_t));

	xgmi_plpd->num_supported = dpm_policy->num_supported;
	xgmi_plpd->current = dpm_policy->cur;

	for (i = 0; i < dpm_policy->num_supported; i++) {
		xgmi_plpd->policies[i].policy_id = dpm_policy->policies[i].policy_id;

#ifdef _WIN64
	strncpy_s(xgmi_plpd->policies[i].policy_description, sizeof(xgmi_plpd->policies[i].policy_description), dpm_policy->policies[i].policy_description, AMDSMI_MAX_STRING_LENGTH);
#else
	sys_wrapper->smi_strncpy(xgmi_plpd->policies[i].policy_description,
					 sizeof(xgmi_plpd->policies[i].policy_description),
					 dpm_policy->policies[i].policy_description,
					 AMDSMI_MAX_STRING_LENGTH);
#endif
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_set_xgmi_plpd(amdsmi_processor_handle processor_handle,
					uint32_t policy_id)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	struct smi_set_dpm_policy *dpm_policy_id = NULL;
	enum smi_handle_type type;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dpm_policy_id = (struct smi_set_dpm_policy *)&smi_req.thread->ioctl_cmd.payload;
	dpm_policy_id->dev_id = pf;
	dpm_policy_id->policy_id = policy_id;

	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_SET_XGMI_PLPD,
				     sizeof(struct smi_set_dpm_policy), 0);

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_node_handle(amdsmi_processor_handle processor_handle, amdsmi_node_handle* node_handle)
{
	#pragma SMI_EXPORT
	struct smi_device_info *dev = NULL;
	smi_device_handle_t pf;
	smi_req_ctx smi_req;
	struct smi_gpu_handle *gpu = NULL;
	struct smi_node_info *node = NULL;
	enum smi_handle_type type;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || node_handle == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d\n", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;

	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;

	int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_NODE_HANDLE,
							sizeof(struct smi_device_info), sizeof(struct smi_node_info));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	node = (struct smi_node_info *)&smi_req.thread->ioctl_cmd.payload;
	g_node_handles.nodes[0].handle = node->node.handle;
	g_node_handles.nodes[0].type = SMI_HANDLE_TYPE_NODE;
	g_node_handles.num_nodes = 1;
	*node_handle = &g_node_handles.nodes[0];

	return AMDSMI_STATUS_SUCCESS;

}

amdsmi_status_t amdsmi_get_npm_info(amdsmi_node_handle node_handle, amdsmi_npm_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	enum smi_handle_type type;
	struct smi_node_info *node = NULL;
	struct smi_node_handle *handle = NULL;
	struct smi_npm_info *npm_info = NULL;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (node_handle == NULL || info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d\n", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)node_handle);
	if (type != SMI_HANDLE_TYPE_NODE) {
		SMI_ERROR("Wrong node handle. Return code: %d\n", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	handle = (struct smi_node_handle *)node_handle;
	node = (struct smi_node_info *)&smi_req.thread->ioctl_cmd.payload;
	node->node.handle = handle->handle;

	int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_GPU_NPM_INFO,
								sizeof(struct smi_node_info), sizeof(struct smi_npm_info));
	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	npm_info = (struct smi_npm_info *)&smi_req.thread->ioctl_cmd.payload;
	memset(info, 0, sizeof(amdsmi_npm_info_t));

	info->status = (amdsmi_npm_status_t)npm_info->status;
	info->limit = npm_info->limit;

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_gpu_ras_policy_info(amdsmi_processor_handle processor_handle,
					       amdsmi_gpu_ras_policy_info_t *policy_info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_device_info *dev = NULL;
	smi_device_handle_t pf;
	struct smi_gpu_handle *gpu = NULL;
	struct smi_gpu_ras_policy_info *ras_policy_info = NULL;
	enum smi_handle_type type;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || policy_info == NULL) {
		SMI_ERROR("Nullpointer given as input. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_GPU) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	gpu = (struct smi_gpu_handle *)processor_handle;
	pf.handle = gpu->handle;
	dev = (struct smi_device_info *)&smi_req.thread->ioctl_cmd.payload;
	dev->dev_id.handle = pf.handle;

	ras_policy_info = (struct smi_gpu_ras_policy_info *)&smi_req.thread->ioctl_cmd.payload;
	const int code = amdsmi_request(&smi_req, (uint32_t)SMI_CMD_CODE_GET_RAS_POLICY_INFO,
					sizeof(struct smi_device_info),
					sizeof(struct smi_gpu_ras_policy_info));

	if (code != AMDSMI_STATUS_SUCCESS) {
		SMI_ERROR("Ioctl call failed. Return code: %d", code);
		return code;
	}

	policy_info->major_version = ras_policy_info->major_version;
	policy_info->minor_version = ras_policy_info->minor_version;
	memcpy(&policy_info->policy_data, (uint8_t *)&ras_policy_info->policy_data,
		sizeof(ras_policy_info->policy_data));

	return AMDSMI_STATUS_SUCCESS;
}

#ifdef __linux__
#pragma GCC diagnostic pop
#endif

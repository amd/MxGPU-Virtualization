/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_utils.h"

#include "smi_debug.h"
#include "smi_vcs.h"
#include "smi_sys_wrapper.h"

#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#ifdef SMI_ESXI_BUILD
#include <unistd.h>   /* access()/F_OK for get_numa_node_from_vsish (ESXi-only) */
#endif

amdsmi_status_t amdsmi_request(smi_req_ctx *smi_req, uint32_t cmd_code, size_t input_size, size_t output_size)
{
	system_wrapper *sys_wrapper;

	if (smi_req == NULL) {
		SMI_ERROR("Invalid param value, NULL pointer passed as smi_req parameter. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}
	sys_wrapper = get_system_wrapper();
	if (!smi_is_supported((enum smi_cmd_code)cmd_code, smi_req->handle->version)) {
		SMI_ERROR("Specified command is not compatible with the negotiated API version. Return code: %d", AMDSMI_STATUS_NOT_SUPPORTED);
		return AMDSMI_STATUS_NOT_SUPPORTED;
	} else {
		smi_req->thread->ioctl_cmd.in_hdr.code = cmd_code;
		smi_req->thread->ioctl_cmd.in_hdr.in_len = (int16_t) input_size;
		smi_req->thread->ioctl_cmd.in_hdr.out_len = (int16_t) output_size;
		const int ret = sys_wrapper->smi_ioctl(smi_req->handle->fd, &smi_req->thread->ioctl_cmd);
		if (ret != 0) {
			if (SMI_LAST_ERROR == SMI_EIO) {
				SMI_ERROR("SMI_LAST_ERROR errno code equals to AMDSMI_EIO error code. Return code: %d", smi_req->thread->ioctl_cmd.out_hdr.status);
				return smi_req->thread->ioctl_cmd.out_hdr.status;
			}
			if (SMI_LAST_ERROR == SMI_ACCESS_DENIED || SMI_LAST_ERROR == SMI_INVAL_ARG) {
				SMI_ERROR("SMI_LAST_ERROR errno code equals to AMDSMI_ACCESS_DENIED or AMDSMI_INVAL_ARG error code. Return code: %d", AMDSMI_STATUS_NOT_SUPPORTED);
				return AMDSMI_STATUS_NOT_SUPPORTED;
			}
			SMI_ERROR("Unknown error occured, ioctl call failed, result not equal to 0. Return code: %d", AMDSMI_STATUS_UNKNOWN_ERROR);
			return AMDSMI_STATUS_UNKNOWN_ERROR;
		}
		if (smi_req->thread->ioctl_cmd.out_hdr.status != AMDSMI_STATUS_SUCCESS) {
			SMI_ERROR("Status of smi request output not returned AMDSMI_STATUS_SUCCESS. Return code: %d", smi_req->thread->ioctl_cmd.out_hdr.status);
			return smi_req->thread->ioctl_cmd.out_hdr.status;
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_ioctl_get_vf_partitioning_info(smi_req_ctx *smi_req, smi_device_handle_t handle)
{
	struct smi_device_info *gpu =
		(struct smi_device_info *)&smi_req->thread->ioctl_cmd.payload;
	gpu->dev_id.handle = handle.handle;

	return amdsmi_request(smi_req, (uint32_t)SMI_CMD_CODE_GET_VF_PARTITIONING_INFO,
			   sizeof(struct smi_device_info),
			   sizeof(struct smi_vf_partition_info));
}

amdsmi_status_t amdsmi_ioctl_get_vf_static_info(smi_req_ctx *smi_req, smi_device_handle_t vf_handle)
{
	struct smi_device_info *vf =
		(struct smi_device_info *)&smi_req->thread->ioctl_cmd.payload;
	vf->dev_id.handle = vf_handle.handle;

	const int code =
		amdsmi_request(smi_req, (uint32_t)SMI_CMD_CODE_GET_VF_STATIC_INFO,
			    sizeof(struct smi_device_info),
			    sizeof(struct smi_vf_static_info));

	return code;
}

amdsmi_status_t amdsmi_ioctl_get_gpu_performance_info(smi_req_ctx *smi_req, smi_device_handle_t handle)
{
	struct smi_device_info_ex *gpu =
		(struct smi_device_info_ex *)&smi_req->thread->ioctl_cmd.payload;
	gpu->dev_id.handle = handle.handle;

	const int code = amdsmi_request(smi_req, (uint32_t)SMI_CMD_CODE_GET_GPU_PERFORMANCE_INFO,
			sizeof(struct smi_device_info_ex),
			sizeof(struct smi_gpu_performance_info));

	return code;
}

amdsmi_status_t amdsmi_ioctl_get_ecc_error_count(smi_req_ctx *smi_req, smi_device_handle_t handle)
{
	struct smi_device_info *gpu =
		(struct smi_device_info *)&smi_req->thread->ioctl_cmd.payload;
	gpu->dev_id.handle = handle.handle;

	return amdsmi_request(smi_req, (uint32_t)SMI_CMD_CODE_GET_ECC_STATUS,
			   sizeof(struct smi_device_info),
			   sizeof(struct smi_ecc_info));
}

amdsmi_status_t amdsmi_ioctl_get_vf_dynamic_info(smi_req_ctx *smi_req, smi_device_handle_t vf_handle)
{
	struct smi_device_info *vf =
		(struct smi_device_info *)&smi_req->thread->ioctl_cmd.payload;
	vf->dev_id.handle = vf_handle.handle;

	const int code = amdsmi_request(smi_req, (uint32_t)SMI_CMD_CODE_GET_VF_DYNAMIC_INFO,
				     sizeof(struct smi_device_info),
				     sizeof(struct smi_vf_data));

	return code;
}

amdsmi_status_t amdsmi_get_pcie_speed_from_pcie_type(uint32_t pcie_type, uint32_t *pcie_speed, uint64_t dev_id)
{
	uint64_t case_start_from_zero = 0x73a1;
	switch (pcie_type + (dev_id == case_start_from_zero)) {
	case 1:
		*pcie_speed = 2500;
		break;
	case 2:
		*pcie_speed = 5000;
		break;
	case 3:
		*pcie_speed = 8000;
		break;
	case 4:
		*pcie_speed = 16000;
		break;
	case 5:
		*pcie_speed = 32000;
		break;
	case 6:
		*pcie_speed = 64000;
		break;
	default:
		return AMDSMI_STATUS_API_FAILED;
	}
	return AMDSMI_STATUS_SUCCESS;
}

uint64_t pcie_gen_to_transfer_rate(uint8_t gen_speed)
{
	switch (gen_speed) {
	case 0: return 2500000000ULL;
	case 1: return 5000000000ULL;
	case 2: return 8000000000ULL;
	case 3: return 16000000000ULL;
	case 4: return 32000000000ULL;
	case 5: return 64000000000ULL;
	default: return 0;
	}
}

uint32_t pcie_lane_count_to_lanes(uint8_t lane_count)
{
	switch (lane_count) {
	case 1: return 1;
	case 2: return 2;
	case 3: return 4;
	case 4: return 8;
	case 5: return 12;
	case 6: return 16;
	default: return 0;
	}
}

amdsmi_status_t amdsmi_get_string_from_status_enum(amdsmi_status_t status, const char **out)
{
	switch (status) {
	case AMDSMI_STATUS_SUCCESS:
		*out = "AMDSMI_STATUS_SUCCESS - Command has been executed successfully";
		break;
	case AMDSMI_STATUS_INVAL:
		*out = "AMDSMI_STATUS_INVAL - Invalid parameters";
		break;
	case AMDSMI_STATUS_NOT_SUPPORTED:
		*out = "AMDSMI_STATUS_NOT_SUPPORTED - Command not supported";
		break;
	case AMDSMI_STATUS_NOT_YET_IMPLEMENTED:
		*out = "AMDSMI_STATUS_NOT_YET_IMPLEMENTED - Not implemented yet";
		break;
	case AMDSMI_STATUS_FAIL_LOAD_MODULE:
		*out = "AMDSMI_STATUS_FAIL_LOAD_MODULE - Fail to load lib";
		break;
	case AMDSMI_STATUS_FAIL_LOAD_SYMBOL:
		*out = "AMDSMI_STATUS_FAIL_LOAD_SYMBOL - Fail to load symbol";
		break;
	case AMDSMI_STATUS_DRM_ERROR:
		*out = "AMDSMI_STATUS_DRM_ERROR - Error when call libdrm";
		break;
	case AMDSMI_STATUS_API_FAILED:
		*out = "AMDSMI_STATUS_API_FAILED - API call failed";
		break;
	case AMDSMI_STATUS_TIMEOUT:
		*out = "AMDSMI_STATUS_TIMEOUT - Timeout in API call";
		break;
	case AMDSMI_STATUS_RETRY:
		*out = "AMDSMI_STATUS_RETRY - Retry operation";
		break;
	case AMDSMI_STATUS_NO_PERM:
		*out = "AMDSMI_STATUS_NO_PERM - Permission Denied";
		break;
	case AMDSMI_STATUS_INTERRUPT:
		*out = "AMDSMI_STATUS_INTERRUPT - An interrupt occurred during execution of function";
		break;
	case AMDSMI_STATUS_IO:
		*out = "AMDSMI_STATUS_IO - I/O Error";
		break;
	case AMDSMI_STATUS_ADDRESS_FAULT:
		*out = "AMDSMI_STATUS_ADDRESS_FAULT - Bad address";
		break;
	case AMDSMI_STATUS_FILE_ERROR:
		*out = "AMDSMI_STATUS_FILE_ERROR - Problem accessing a file";
		break;
	case AMDSMI_STATUS_OUT_OF_RESOURCES:
		*out = "AMDSMI_STATUS_OUT_OF_RESOURCES - Not enough memory";
		break;
	case AMDSMI_STATUS_INTERNAL_EXCEPTION:
		*out = "AMDSMI_STATUS_INTERNAL_EXCEPTION - An internal exception was caught";
		break;
	case AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS:
		*out = "AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS - The provided input is out of allowable or safe range";
		break;
	case AMDSMI_STATUS_INIT_ERROR:
		*out = "AMDSMI_STATUS_INIT_ERROR - An error occurred when initializing internal data structures";
		break;
	case AMDSMI_STATUS_REFCOUNT_OVERFLOW:
		*out = "AMDSMI_STATUS_REFCOUNT_OVERFLOW - An internal reference counter exceeded INT32_MAX";
		break;
	case AMDSMI_STATUS_BUSY:
		*out = "AMDSMI_STATUS_BUSY - Processor busy";
		break;
	case AMDSMI_STATUS_NOT_FOUND:
		*out = "AMDSMI_STATUS_NOT_FOUND - Processor not found";
		break;
	case AMDSMI_STATUS_NOT_INIT:
		*out = "AMDSMI_STATUS_NOT_INIT - Processor not initialized";
		break;
	case AMDSMI_STATUS_NO_SLOT:
		*out = "AMDSMI_STATUS_NO_SLOT - No more free slot";
		break;
	case AMDSMI_STATUS_DRIVER_NOT_LOADED:
		*out = "AMDSMI_STATUS_DRIVER_NOT_LOADED - Processor driver not loaded";
		break;
	case AMDSMI_STATUS_NO_DATA:
		*out = "AMDSMI_STATUS_NO_DATA - No data was found for a given input";
		break;
	case AMDSMI_STATUS_INSUFFICIENT_SIZE:
		*out = "AMDSMI_STATUS_INSUFFICIENT_SIZE - Not enough resources were available for the operation";
		break;
	case AMDSMI_STATUS_UNEXPECTED_SIZE:
		*out = "AMDSMI_STATUS_UNEXPECTED_SIZE - An unexpected amount of data was read";
		break;
	case AMDSMI_STATUS_UNEXPECTED_DATA:
		*out = "AMDSMI_STATUS_UNEXPECTED_DATA - The data read or provided to function is not what was expected";
		break;
	case AMDSMI_STATUS_NON_AMD_CPU:
		*out = "AMDSMI_STATUS_NON_AMD_CPU - System has different cpu than AMD";
		break;
	case AMDSMI_STATUS_NO_ENERGY_DRV:
		*out = "AMDSMI_STATUS_NO_ENERGY_DRV - Energy driver not found";
		break;
	case AMDSMI_STATUS_NO_MSR_DRV:
		*out = "AMDSMI_STATUS_NO_MSR_DRV - MSR driver not found";
		break;
	case AMDSMI_STATUS_NO_HSMP_DRV:
		*out = "AMDSMI_STATUS_NO_HSMP_DRV - HSMP driver not found";
		break;
	case AMDSMI_STATUS_NO_HSMP_SUP:
		*out = "AMDSMI_STATUS_NO_HSMP_SUP - HSMP not supported";
		break;
	case AMDSMI_STATUS_NO_HSMP_MSG_SUP:
		*out = "AMDSMI_STATUS_NO_HSMP_MSG_SUP - HSMP message/feature not supported";
		break;
	case AMDSMI_STATUS_HSMP_TIMEOUT:
		*out = "AMDSMI_STATUS_HSMP_TIMEOUT - HSMP message is timedout";
		break;
	case AMDSMI_STATUS_NO_DRV:
		*out = "AMDSMI_STATUS_NO_DRV - No Energy and HSMP driver present";
		break;
	case AMDSMI_STATUS_FILE_NOT_FOUND:
		*out = "AMDSMI_STATUS_FILE_NOT_FOUND - File or directory not found";
		break;
	case AMDSMI_STATUS_ARG_PTR_NULL:
		*out = "AMDSMI_STATUS_ARG_PTR_NULL - Parsed argument is invalid";
		break;
	case AMDSMI_STATUS_AMDGPU_RESTART_ERR:
		*out = "AMDSMI_STATUS_AMDGPU_RESTART_ERR - AMDGPU restart failed";
		break;
	case AMDSMI_STATUS_SETTING_UNAVAILABLE:
		*out = "AMDSMI_STATUS_SETTING_UNAVAILABLE - Setting is not available";
		break;
	case AMDSMI_STATUS_MAP_ERROR:
		*out = "AMDSMI_STATUS_MAP_ERROR - The internal library error did not map to a status code";
		break;
	case AMDSMI_STATUS_UNKNOWN_ERROR:
		*out = "AMDSMI_STATUS_UNKNOWN_ERROR - An unknown error occurred";
		break;
	default:
		return AMDSMI_STATUS_API_FAILED;
	};
	return AMDSMI_STATUS_SUCCESS;
}

typedef struct uuid_s {
	union {
		struct {
			uint32_t did	: 16;
			uint32_t fcn	: 8;
			uint32_t asic_7 : 8;
		};
		uint32_t time_low;
	};
	uint32_t time_mid  : 16;
	uint32_t time_high : 12;
	uint32_t version   : 4;
	uint8_t clk_seq_hi : 6;
	uint8_t variant	   : 2;
	union {
		uint8_t clk_seq_low;
		uint8_t asic_6;
	};
	uint16_t asic_4;
	uint32_t asic_0;
} uuid_t;

static void print_uuid(char *str, uuid_t *uuid)
{
#ifdef _WIN64
	sprintf_s(str,
		  AMDSMI_GPU_UUID_SIZE,
		  "%08x-%04x-%04x-%02x%02x-%04x%08x", uuid->time_low,
						      uuid->time_mid,
						      ((uuid->version << 12) | uuid->time_high),
						      ((uuid->variant << 6) | uuid->clk_seq_hi),
						      uuid->clk_seq_low,
						      uuid->asic_4,
						      uuid->asic_0);
#else
	snprintf(str,
		  AMDSMI_GPU_UUID_SIZE,
		  "%08x-%04x-%04x-%02x%02x-%04x%08x", uuid->time_low,
						      uuid->time_mid,
						      ((uuid->version << 12) | uuid->time_high),
						      ((uuid->variant << 6) | uuid->clk_seq_hi),
						      uuid->clk_seq_low,
						      uuid->asic_4,
						      uuid->asic_0);
#endif
}

static void insert_asic_serial(uuid_t *uuid, uint64_t serial)
{
	uuid->asic_0 = (uint32_t)serial;
	uuid->asic_4 = (uint16_t)(serial >> 4 * 8) & 0xFFFF;
	uuid->asic_6 = (uint8_t)(serial >> 6 * 8) & 0xFF;
	uuid->asic_7 = (uint32_t)(serial >> 7 * 8) & 0xFF;
}

static void insert_did(uuid_t *uuid, uint16_t did)
{
	uuid->did = did;
}

static void insert_fcn(uuid_t *uuid, uint8_t fcn_idx)
{
	uuid->fcn = fcn_idx;
}

static void insert_clk_seq(uuid_t *uuid, uint16_t seq)
{
	uuid->clk_seq_low = (uint8_t)seq;
	uuid->clk_seq_hi = (seq >> 8) & 0x3fU;
}

int smi_get_vf_device_id_from_pf(uint64_t pf_device_id, uint64_t *vf_device_id)
{
    static const struct {
		uint64_t pf_id;
		uint64_t vf_id;
	} pf_vf_map[] = {
		{0x74A1, 0x74B5},
		{0x74A2, 0x74B6},
		{0x74A8, 0x74BC},
		{0x74A9, 0x74BD},
		{0x75A0, 0x75B0},
		{0x75A3, 0x75B3},
		{0x7460, 0x7461}
	};

	for (size_t i = 0; i < sizeof(pf_vf_map) / sizeof(pf_vf_map[0]); i++) {
		if (pf_device_id == pf_vf_map[i].pf_id) {
			*vf_device_id = pf_vf_map[i].vf_id;
			return 0;
		}
	}

	*vf_device_id = 0xFFFF;
	return -1;
}

int smi_uuid_gen(char *str, uint64_t serial, uint16_t did, uint8_t idx)
{
	uuid_t uuid;
	memset(&uuid, 0, sizeof(uuid_t));

	insert_clk_seq(&uuid, 0);
	insert_did(&uuid, did);
	insert_fcn(&uuid, idx);
	insert_asic_serial(&uuid, serial);

	uuid.version = 1;
	uuid.variant = 2;

	print_uuid(str, &uuid);

	return 0;
}

static bool is_hex_digit(char c)
{
	return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static bool are_hex_sequence_match(const char *str, int *index, int length)
{
	for (int i = 0; i < length; i++) {
		if (!is_hex_digit(str[*index])) {
			return false;
		}
		(*index)++;
	}
	return true;
}

static bool are_char_match(const char *str,  int *index, char expected)
{
	if (str[*index] != expected) {
		return false;
	}
	(*index)++;
	return true;
}

bool is_uuid_valid(const char *uuid)
{
	int index = 0;

	if (!are_hex_sequence_match(uuid, &index, 8)) {
		return false;
	}
	if (!are_char_match(uuid, &index, '-')) {
		return false;
	}
	if (!are_hex_sequence_match(uuid, &index, 4)) {
		return false;
	}
	if (!are_char_match(uuid, &index, '-')) {
		return false;
	}
	if (!are_hex_sequence_match(uuid, &index, 4)) {
		return false;
	}
	if (!are_char_match(uuid, &index, '-')) {
		return false;
	}
	if (!are_hex_sequence_match(uuid, &index, 4)) {
		return false;
	}
	if (!are_char_match(uuid, &index, '-')) {
		return false;
	}
	if (!are_hex_sequence_match(uuid, &index, 12)) {
		return false;
	}
	if (uuid[index] != '\0') {
		return false;
	}
	return true;
}

bool guid_equals(const guid_t* guid1,const guid_t* guid2) {
    return memcmp(guid1->b, guid2->b, sizeof(guid1->b)) == 0;
}

void amdsmi_get_register_array(const uint8_t* data, size_t size, uint64_t *register_array) {
	uint8_t index = 0;
    for (size_t i = 0; i < size; i += sizeof(uint64_t)) {
        uint64_t value = 0;
        if (i + sizeof(uint64_t) <= size) {
            value = *(uint64_t*)(data + i);
			register_array[index] = value;
			index++;
        } else {
            memcpy(&value, data + i, size - i); // Handle remaining bytes
        }
    }
}

int make_sysfs_pci_device_prefix(amdsmi_bdf_t bdf, char *out_path, size_t out_path_size)
{
	system_wrapper *sys_wrapper = get_system_wrapper();
	// Format: /sys/bus/pci/devices/0000:22:00.0/
	int n = sys_wrapper->snprintf(
		out_path, out_path_size,
		"/sys/bus/pci/devices/%04x:%02x:%02x.%01x/",
		(unsigned)bdf.bdf.domain_number,
		bdf.bdf.bus_number,
		bdf.bdf.device_number,
		bdf.bdf.function_number
	);
	if (n < 0 || (size_t)n >= out_path_size) {
		return -1; // Buffer too small or snprintf error
	}
	return 0;
}

int parse_cpu_list(const char *cpu_list, uint64_t *cpu_set, uint32_t cpu_set_size)
{
	// Clear the cpu_set
	memset(cpu_set, 0, cpu_set_size * sizeof(uint64_t));

	const char *p = cpu_list;
	while (*p) {
		while (*p && !isdigit(*p)) p++;
		if (!*p) break;
		int start = atoi(p);
		while (*p && isdigit(*p)) p++;
		int end = start;
		if (*p == '-') {
			p++;
			end = atoi(p);
			while (*p && isdigit(*p)) p++;
		}

		// Set bits from start to end
		for (int cpu = start; cpu <= end; ++cpu) {
			uint32_t idx = (unsigned)cpu / 64;
			uint32_t bit = (unsigned)cpu % 64;
			if (idx < cpu_set_size)
				cpu_set[idx] |= (1ULL << bit);
		}
		while (*p && *p != ',') p++;
		if (*p == ',') p++;
	}
	return 0;
}

amdsmi_status_t is_cmd_supported(uint64_t device_id)
{
	static const uint64_t dev_id_list_nv[] = {
		0x73C4,
		0x73C5,
		0x73C8,
		0x7460,
		0x7461,
		0x73A1,
		0x73AE
	};

	static const uint64_t dev_id_list_mi2plus[] = {
		0x7410
	};

	for (size_t i = 0; i < sizeof(dev_id_list_nv) / sizeof(dev_id_list_nv[0]); i++) {
		if (device_id == dev_id_list_nv[i]) {
			return AMDSMI_STATUS_NOT_SUPPORTED;
		}
	}

	for (size_t i = 0; i < sizeof(dev_id_list_mi2plus) / sizeof(dev_id_list_mi2plus[0]); i++) {
		if (device_id == dev_id_list_mi2plus[i]) {
			return AMDSMI_STATUS_NOT_SUPPORTED;
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

#ifdef SMI_ESXI_BUILD
int get_numa_node_from_vsish(amdsmi_bdf_t bdf, uint32_t *numa_node)
{
	/*
	 * Absolute locations for vsish. ESXi layouts differ across versions:
	 * vsish lives at /bin/vsish on some and /sbin/vsish on others.
	 */
	static const char * const vsish_paths[] = {
		"/bin/vsish",
		"/sbin/vsish",
	};
	system_wrapper *sys_wrapper = get_system_wrapper();
	char cmd[AMDSMI_MAX_STRING_LENGTH];
	char buf[AMDSMI_MAX_STRING_LENGTH];
	const char *vsish = NULL;
	FILE *fp;
	int found = 0;
	size_t i;

	if (numa_node == NULL) {
		return -1;
	}

	/*
	 * Resolve vsish to an absolute path from the fixed allowlist above;
	 * use the first that exists and never fall back to a bare name.
	 */
	for (i = 0; i < sizeof(vsish_paths) / sizeof(vsish_paths[0]); i++) {
		if (access(vsish_paths[i], F_OK) == 0) {
			vsish = vsish_paths[i];
			break;
		}
	}
	if (vsish == NULL) {
		return -1;
	}

	sys_wrapper->snprintf(cmd, sizeof(cmd),
		"%s -e get /hardware/pci/seg/%llu/bus/%d/slot/%d/func/%d/pciConfigHeader 2>/dev/null",
		vsish,
		(unsigned long long)bdf.bdf.domain_number,
		bdf.bdf.bus_number,
		bdf.bdf.device_number,
		bdf.bdf.function_number);

	fp = popen(cmd, "r");
	if (!fp) {
		return -1;
	}

	/* Parse output to find "Numa node:" line */
	while (sys_wrapper->fgets(buf, sizeof(buf), fp) != NULL) {
		char *numa_line = strstr(buf, "Numa node:");
		if (numa_line) {
			/* Extract the number after "Numa node:" */
			char *value_str = numa_line + strlen("Numa node:");
			/* Skip whitespace */
			while (*value_str == ' ' || *value_str == '\t') {
				value_str++;
			}
			*numa_node = (uint32_t)strtoul(value_str, NULL, 10);
			found = 1;
			break;
		}
	}

	pclose(fp);

	return found ? 0 : -1;
}
#endif /* SMI_ESXI_BUILD */
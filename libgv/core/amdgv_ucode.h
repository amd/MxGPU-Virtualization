/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_UCODE_H
#define AMDGV_UCODE_H

enum amdgv_firmware_id;

struct amdgv_ucode {
	int (*load)(struct amdgv_adapter *adapt,
		enum amdgv_firmware_id *ucode_id_list, uint32_t ucode_id_count);
	int (*prepare_ucode_engine)(struct amdgv_adapter *adapt, uint32_t ucode_id);
	int (*get_ucode_start_addr)(struct amdgv_adapter *adapt, uint32_t ucode_id, uint64_t *ucode_start_addr);
	/* A0/B0 RLC/MEC ucode blob accessors, registered by the owning GC block. */
	int (*get_rlc_ucode)(struct amdgv_adapter *adapt, enum amdgv_firmware_id fw_id,
		unsigned char **ptr, uint32_t *size);
	int (*get_mec_ucode)(struct amdgv_adapter *adapt, enum amdgv_firmware_id fw_id,
		unsigned char **ptr, uint32_t *size);
	void (*get_mec_ucode_start_addr)(struct amdgv_adapter *adapt, uint32_t *lo, uint32_t *hi);
};

#endif

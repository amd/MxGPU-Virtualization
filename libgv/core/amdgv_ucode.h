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
};

#endif

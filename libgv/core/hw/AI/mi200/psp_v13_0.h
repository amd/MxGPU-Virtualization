/*
 * Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */



#ifndef AMDGV_PSP_V13_H
#define AMDGV_PSP_V13_H

#include "amdgv_psp_gfx_if.h"

#define PSP_REGISTER_VALUE_INVALID    0xFFFFFFFF

/* Reserved size for PSP migration data */
#define MI200_MIGRATION_PSP_STATIC_DATA_SIZE		(4 * 1024 * 1024)
#define MI200_MIGRATION_PSP_DYNAMIC_DATA_SIZE		(4 * 1024 * 1024)

/* All sizes, offsets, address should be aligned to 32bytes */
#define MI200_MIGRATION_ALIGNMENT					32
#define MI200_MIGRATION_ALIGN(addr) \
	roundup(addr, MI200_MIGRATION_ALIGNMENT)

bool psp_v13_wait_sos_loaded_status(struct amdgv_adapter *adapt);
int psp_v13_ring_destroy(struct amdgv_adapter *adapt);
int psp_v13_hw_start(struct amdgv_adapter *adapt);
enum psp_status psp_v13_init(struct amdgv_adapter *adapt);
enum psp_status psp_v13_hw_stop(struct amdgv_adapter *adapt);
enum psp_status psp_v13_fw_init(struct amdgv_adapter *adapt);
enum psp_status psp_v13_fw_fini(struct amdgv_adapter *adapt);
enum psp_status psp_v13_ring_init(struct amdgv_adapter *adapt);
enum psp_status psp_v13_ring_fini(struct amdgv_adapter *adapt);
enum psp_status psp_v13_cmd_km_init(struct amdgv_adapter *adapt);
enum psp_status psp_v13_cmd_km_fini(struct amdgv_adapter *adapt);
enum psp_status psp_v13_ring_start(struct amdgv_adapter *adapt);
enum psp_status psp_v13_cmd_km_submit(
				struct amdgv_adapter *adapt,
				struct psp_cmd_km *input_index);
enum psp_status psp_v13_ring_km_submit(
				struct amdgv_adapter *adapt,
				uint64_t cmd_buf_mc_addr,
				uint64_t fence_mc_addr,
				uint32_t fence_value);
enum psp_status psp_v13_set_sriov_mode(struct amdgv_adapter *adapt);
enum psp_status psp_v13_apply_security_policy(
				struct amdgv_adapter *adapt);
enum psp_status psp_v13_load_sysdrv(struct amdgv_adapter *adapt,
			unsigned char *fw_image, uint32_t fw_image_size);
enum psp_status psp_v13_load_sos(struct amdgv_adapter *adapt,
			unsigned char *fw_image, uint32_t fw_image_size);
enum psp_status psp_v13_load_key_db(struct amdgv_adapter *adapt,
			unsigned char *fw_image, uint32_t fw_image_size);

enum psp_status psp_v13_program_guest_mc_settings(struct amdgv_adapter *adapt,
			uint32_t idx_vf);

enum psp_status psp_v13_dump_tracelog(struct amdgv_adapter *adapt,
			uint64_t buf_bus_addr,
			uint32_t buf_size,
			uint32_t *buf_used_size);

enum psp_status psp_v13_set_snapshot_addr(struct amdgv_adapter *adapt,
			uint64_t buf_bus_addr,
			uint32_t buf_size);

enum psp_status psp_v13_trigger_snapshot(struct amdgv_adapter *adapt,
			uint32_t vfid,
			uint32_t sections,
			uint32_t *buf_used_size);
#endif

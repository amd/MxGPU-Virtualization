/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
			const unsigned char *fw_image, uint32_t fw_image_size);
enum psp_status psp_v13_load_sos(struct amdgv_adapter *adapt,
			const unsigned char *fw_image, uint32_t fw_image_size);
enum psp_status psp_v13_load_key_db(struct amdgv_adapter *adapt,
			const unsigned char *fw_image, uint32_t fw_image_size);

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

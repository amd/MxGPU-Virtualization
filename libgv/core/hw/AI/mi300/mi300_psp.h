/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_MI300_PSP_H
#define AMDGV_MI300_PSP_H

#include "amdgv_psp_gfx_if.h"

#define PSP_REGISTER_VALUE_INVALID 0xFFFFFFFF
//TODO: get the size from psp pkg
#define MI300_MIGRATION_PSP_STATIC_DATA_SIZE		(10 * 1024 * 1024)
#define MI300_MIGRATION_PSP_DYNAMIC_DATA_SIZE		(2 * 1024 * 1024)

uint32_t mi300_psp_get_sos_loaded_status(struct amdgv_adapter *adapt);
bool mi300_psp_wait_sos_loaded_status(struct amdgv_adapter *adapt, uint32_t wait_flag);
int mi300_psp_ring_destroy(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_init(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_hw_stop(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_fw_init(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_fw_fini(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_ring_init(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_ring_fini(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_cmd_km_init(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_cmd_km_fini(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_ring_start(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_cmd_km_submit(struct amdgv_adapter *adapt,
					struct psp_cmd_km *input_index);
enum psp_status mi300_psp_ring_km_submit(struct amdgv_adapter *adapt, uint64_t cmd_buf_mc_addr,
					 uint64_t fence_mc_addr, uint32_t fence_value);
enum psp_status mi300_psp_set_sriov_mode(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_apply_security_policy(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_load_sys_drv(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				      uint32_t fw_image_size);
enum psp_status mi300_psp_load_ras_drv(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				      uint32_t fw_image_size);
enum psp_status mi300_psp_load_sos(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				      uint32_t fw_image_size);
enum psp_status mi300_psp_load_key_db(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				      uint32_t fw_image_size);
enum psp_status mi300_psp_load_soc_drv(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				      uint32_t fw_image_size);
enum psp_status mi300_psp_load_intf_drv(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				      uint32_t fw_image_size);
enum psp_status mi300_psp_load_dbg_drv(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				      uint32_t fw_image_size);

enum psp_status mi300_psp_program_guest_mc_settings(struct amdgv_adapter *adapt,
						    uint32_t idx_vf);
enum psp_status mi300_psp_dump_tracelog(struct amdgv_adapter *adapt, uint64_t buf_bus_addr,
					uint32_t buf_size, uint32_t *buf_used_size);

enum psp_status mi300_psp_set_snapshot_addr(struct amdgv_adapter *adapt, uint64_t buf_bus_addr,
					uint32_t buf_size);

enum psp_status mi300_psp_trigger_snapshot(struct amdgv_adapter *adapt, uint32_t target_vfid,
					uint32_t sections, uint32_t aid_mask,
					uint32_t xcc_mask, uint32_t *buf_used_size);

void mi300_psp_get_fw_version(enum amdgv_firmware_id firmware_id,
			    uint32_t image_version, char *fw_version,
			    uint32_t size);
enum psp_status mi300_psp_fb_addr_bound_check(struct amdgv_adapter *adapt,
					    uint64_t fb_addr, uint64_t size);
enum psp_status mi300_psp_fw_attestation_support(struct amdgv_adapter *adapt);
enum psp_status
mi300_psp_get_fw_attestation_database_addr(struct amdgv_adapter *adapt);
enum psp_status mi300_psp_get_fw_attestation_info(struct amdgv_adapter *adapt,
						uint32_t idx_vf);
enum psp_status
mi300_psp_clear_fw_attestation_database_addr(struct amdgv_adapter *adapt);

enum psp_status
mi300_psp_wait_for_bootloader_steady(struct amdgv_adapter *adapt);
uint32_t mi300_psp_get_bootloader_version(struct amdgv_adapter *adapt);
enum psp_status
mi300_psp_set_accelerator_partition_mode(struct amdgv_adapter *adapt,
					enum amdgv_accelerator_partition_mode accelerator_partition_mode);
enum psp_status
mi300_psp_set_memory_partition_mode(struct amdgv_adapter *adapt,
					enum amdgv_memory_partition_mode memory_partition_mode);

/* PTL (Peak TOPS Limiter) Support */
/*
 * Note: PTL-related definitions are in common headers:
 * - PSP_PERF_HW_REQ_* and TEE_ERROR_* in amdgv_psp_gfx_if.h
 * - psp_gfx_format_type, psp_gfx_cmd_req_perf_hw, psp_gfx_cmd_resp_perf_hw in amdgv_psp.h
 */

/* PTL function prototypes */
enum psp_status mi300_psp_send_perf_hw_cmd(struct amdgv_adapter *adapt,
					    struct psp_gfx_cmd_req_perf_hw *req,
					    struct psp_gfx_cmd_resp_perf_hw *resp);

#endif

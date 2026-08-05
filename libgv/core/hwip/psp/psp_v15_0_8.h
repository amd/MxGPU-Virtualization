/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PSP_v15_0_8_0_H
#define PSP_v15_0_8_0_H

#include "amdgv_psp_gfx_if.h"

#define PSP_REGISTER_VALUE_INVALID 0xFFFFFFFF

enum psp_v15_0_8_spatial_partition_mode {
	SP_MODE_SPX = 0,
	SP_MODE_DPX = 1,
	SP_MODE_QPX = 3,
	SP_MODE_CPX = 4,
};

uint32_t psp_v15_0_8_get_sos_loaded_status(struct amdgv_adapter *adapt);
bool psp_v15_0_8_wait_sos_loaded_status(struct amdgv_adapter *adapt);
int psp_v15_0_8_ring_destroy(struct amdgv_adapter *adapt);
enum psp_status psp_v15_0_8_ring_start(struct amdgv_adapter *adapt);
enum psp_status psp_v15_0_8_ring_stop(struct amdgv_adapter *adapt);

enum psp_status psp_v15_0_8_program_guest_mc_settings(struct amdgv_adapter *adapt,
						    uint32_t idx_vf);
enum psp_status psp_v15_0_8_fw_attestation_support(struct amdgv_adapter *adapt);

enum psp_status psp_v15_0_8_wait_for_bootloader_steady(struct amdgv_adapter *adapt);
uint32_t psp_v15_0_8_get_bootloader_version(struct amdgv_adapter *adapt);
enum psp_status psp_v15_0_8_set_accelerator_partition_mode(struct amdgv_adapter *adapt,
			enum amdgv_accelerator_partition_mode accelerator_partition_mode);
enum psp_status psp_v15_0_8_get_cc_mode(struct amdgv_adapter *adapt,
			enum amdgv_cc_mode *cc_mode);
enum psp_status psp_v15_0_8_set_cc_mode(struct amdgv_adapter *adapt,
			enum amdgv_cc_mode cc_mode);
enum psp_status psp_v15_0_8_ual_get_interface_version(struct amdgv_adapter *adapt, uint32_t *version);
enum psp_status psp_v15_0_8_ual_get_config(struct amdgv_adapter *adapt,
	uint64_t data_addr, uint32_t size);
enum psp_status psp_v15_0_8_ual_set_ppod_config(struct amdgv_adapter *adapt,
		struct amdgv_gpumon_set_ppod_config_req_ual_v1 *config);
enum psp_status psp_v15_0_8_ual_set_vpod_config(struct amdgv_adapter *adapt,
		struct amdgv_gpumon_set_vpod_config_req_ual_v1 *config);
enum psp_status psp_v15_0_8_ual_set_station_config(struct amdgv_adapter *adapt,
		struct amdgv_gpumon_set_station_config_req_ual_v1 *config);
enum psp_status psp_v15_0_8_ual_send_completion(struct amdgv_adapter *adapt,
		uint32_t cmd_id, uint32_t status);
enum psp_status psp_v15_0_8_set_memory_partition_mode(struct amdgv_adapter *adapt,
		enum amdgv_memory_partition_mode memory_partition_mode);

/*
 * Wire adapt->psp.get_ras_{ta,rl}_fw to the embedded RAS firmware accessors
 * matching the detected PSP (MP0) IP version. Exposed for unit testing the
 * version-routing table (including the unlisted-version path).
 */
void psp_set_ras_fw_accessors(struct amdgv_adapter *adapt);
#endif

/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI32_PSP_H
#define NAVI32_PSP_H

#include "amdgv_psp_gfx_if.h"

#define PSP_C2P_MAILBOX_TIMEOUT       (1*1000*1000)
#define PSP_C2P_MAILBOX_CLOSED        0xFFFFFFFF
#define PSP_REGISTER_VALUE_INVALID    0xFFFFFFFF
#define NAVI32_PSP_TMR_ALIGNMENT         0x200000      /* 2M */
#define PSP_TMR_SIZE_ALIGNMENT        0x1000000    /* 16M */

#define C2PMSG_CMD_SPI_UPDATE_ROM_IMAGE_ADDR_LO 0x2
#define C2PMSG_CMD_SPI_UPDATE_ROM_IMAGE_ADDR_HI 0x3
#define C2PMSG_CMD_SPI_UPDATE_FLASH_IMAGE 0x4

//TODO: get the size from psp pkg
#define NAVI32_MIGRATION_PSP_STATIC_DATA_SIZE		(10 * 1024 * 1024)
#define NAVI32_MIGRATION_PSP_DYNAMIC_DATA_SIZE		(20 * 1024 * 1024)

enum { PSP_TMR_INC_SIZE = 0x200000 }; /* 2M */

enum { PSP_TMR_BASE_SIZE = 0xB600000 }; /* 182M */

#define PSP_TMR_SIZE(max_vf_num) (PSP_TMR_BASE_SIZE + PSP_TMR_INC_SIZE * (max_vf_num))

enum psp_status navi32_psp_ring_start(struct amdgv_adapter *adapt);

uint32_t navi32_psp_get_bootloader_version(struct amdgv_adapter *adapt);

uint32_t navi32_psp_ring_get_wptr(struct amdgv_adapter *adapt);

void navi32_psp_ring_set_wptr(struct amdgv_adapter *adapt, uint32_t value);

/* support functions */
enum psp_status navi32_psp_program_register(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   uint32_t reg_value, uint32_t reg_value_hi,
					   enum psp_ih_reg reg_id);

bool navi32_psp_check_register_response(struct amdgv_adapter *adapt, uint32_t reg_index,
				       uint32_t reg_value, uint32_t reg_mask,
				       bool check_changed);

bool navi32_psp_check_memory_response(uint32_t *memory_address, uint32_t memory_value);

bool navi32_psp_wait_boot_complete(struct amdgv_adapter *adapt, uint32_t offset, const char *name);

bool navi32_psp_wait_sos_loaded_status(struct amdgv_adapter *adapt);

enum psp_status navi32_psp_wait_for_memory(struct amdgv_adapter *adapt,
					  uint32_t *memory_address, uint32_t memory_value);

enum psp_status navi32_psp_load_keydb(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				     uint32_t fw_image_size);
enum psp_status navi32_psp_load_spl(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				   uint32_t fw_image_size);
enum psp_status navi32_psp_load_sysdrv(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				      uint32_t fw_image_size);
enum psp_status navi32_psp_load_sos(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				   uint32_t fw_image_size);
enum psp_status navi32_psp_load_psp_ucode(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				   uint32_t fw_image_size, uint32_t fw_id);

enum psp_status navi32_psp_wait_rlcg_ready(struct amdgv_adapter *adapt);

enum psp_status navi32_psp_program_guest_mc_settings(struct amdgv_adapter *adapt,
						    uint32_t idx_vf);
uint32_t navi32_psp_get_wptr(struct amdgv_adapter *adapt);
void navi32_psp_set_wptr(struct amdgv_adapter *adapt, uint32_t value);
void navi32_psp_get_fw_version(enum amdgv_firmware_id firmware_id,
					uint32_t image_version, char *fw_version, uint32_t size);
enum psp_status navi32_psp_fb_addr_bound_check(struct amdgv_adapter *adapt, uint64_t fb_addr, uint64_t size);
enum psp_status navi32_psp_fw_attestation_support(struct amdgv_adapter *adapt);
enum psp_status navi32_psp_get_fw_attestation_database_addr(struct amdgv_adapter *adapt);
enum psp_status navi32_psp_get_fw_attestation_info(struct amdgv_adapter *adapt, uint32_t idx_vf);
enum psp_status navi32_psp_clear_fw_attestation_database_addr(struct amdgv_adapter *adapt);
enum psp_status navi32_psp_vf_cmd_relay(struct amdgv_adapter *adapt, uint32_t vf_id);
enum psp_status navi32_psp_load_asd_fw_to_mem(struct amdgv_adapter *adapt,
						struct psp_local_memory *asd_bin_mem, uint32_t *size);
void navi32_grbm_select(struct amdgv_adapter *adapt,
		     uint32_t me, uint32_t pipe, uint32_t queue, uint32_t vmid);

enum psp_status navi32_psp_dump_tracelog(struct amdgv_adapter *adapt,
	uint64_t buf_bus_addr,
	uint32_t buf_size,
	uint32_t *buf_used_size);

enum psp_status navi32_psp_set_snapshot_addr(struct amdgv_adapter *adapt,
	uint64_t buf_bus_addr,
	uint32_t buf_size);

enum psp_status navi32_psp_trigger_snapshot(struct amdgv_adapter *adapt,
	uint32_t vfid,
	uint32_t sections,
	uint32_t *buf_used_size);
#endif

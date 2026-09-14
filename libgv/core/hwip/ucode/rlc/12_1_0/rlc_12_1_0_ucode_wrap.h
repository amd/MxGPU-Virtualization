/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef RLC_12_1_0_UCODE_WRAP_H
#define RLC_12_1_0_UCODE_WRAP_H

/*
 * Runtime A0/B0 RLC ucode selection for gc_12_1_0. A0/B0 variant headers under
 * a0/ and b0/ share include guards and C symbol names, so each is compiled in
 * its own TU (rlc_12_1_0_ucode_a0.c / _b0.c) that boundary-renames the symbol
 * and re-exports the blob via a getter; rlc_12_1_0_ucode_get() dispatches by
 * adapt->rev_id. Only prototypes are declared here to keep the collision out of
 * including TUs. A structural FW change (renamed file/symbol, added/removed
 * sub-component) requires updating rlc_12_1_0_ucode_a0.c / _b0.c; a binary-only
 * refresh does not.
 */

#include <amdgv_basetypes.h>

struct amdgv_adapter;
enum amdgv_firmware_id;

/* A0 variant getters (rlc_12_1_0_ucode_a0.c). */
unsigned char *rlc_12_1_0_gpm_ucode_a0(uint32_t *size);
unsigned char *rlc_12_1_0_toc_data_a0(uint32_t *size);
unsigned char *rlc_12_1_0_restore_list_gpm_mem_a0(uint32_t *size);
unsigned char *rlc_12_1_0_restore_list_srm_mem_a0(uint32_t *size);
unsigned char *rlc_12_1_0_lx6_iram_ucode_a0(uint32_t *size);
unsigned char *rlc_12_1_0_lx6_dram_ucode_a0(uint32_t *size);
unsigned char *rlc_12_1_0_lx6_core1_iram_ucode_a0(uint32_t *size);
unsigned char *rlc_12_1_0_lx6_core1_dram_ucode_a0(uint32_t *size);

/* B0 variant getters (rlc_12_1_0_ucode_b0.c). */
unsigned char *rlc_12_1_0_gpm_ucode_b0(uint32_t *size);
unsigned char *rlc_12_1_0_toc_data_b0(uint32_t *size);
unsigned char *rlc_12_1_0_restore_list_gpm_mem_b0(uint32_t *size);
unsigned char *rlc_12_1_0_restore_list_srm_mem_b0(uint32_t *size);
unsigned char *rlc_12_1_0_lx6_iram_ucode_b0(uint32_t *size);
unsigned char *rlc_12_1_0_lx6_dram_ucode_b0(uint32_t *size);
unsigned char *rlc_12_1_0_lx6_core1_iram_ucode_b0(uint32_t *size);
unsigned char *rlc_12_1_0_lx6_core1_dram_ucode_b0(uint32_t *size);

/*
 * Select the RLC ucode blob for fw_id, picking the A0/B0 variant by
 * adapt->rev_id. Returns 0 on success (ptr/size filled), AMDGV_FAILURE on an
 * unknown fw_id (ptr/size left untouched).
 */
int rlc_12_1_0_ucode_get(struct amdgv_adapter *adapt, enum amdgv_firmware_id fw_id,
			 unsigned char **ptr, uint32_t *size);

/* Install the RLC accessor into adapt->ucode so callers reach it by pointer. */
void rlc_12_1_0_ucode_register(struct amdgv_adapter *adapt);

#endif /* RLC_12_1_0_UCODE_WRAP_H */

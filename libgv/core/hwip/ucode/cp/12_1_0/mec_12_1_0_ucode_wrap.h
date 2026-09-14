/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MEC_12_1_0_UCODE_WRAP_H
#define MEC_12_1_0_UCODE_WRAP_H

/*
 * Runtime A0/B0 MEC ucode selection for gc_12_1_0. A0/B0 variant headers under
 * a0/ and b0/ share include guards, C symbol names, and START_ADDR macro names,
 * so each is compiled in its own TU (mec_12_1_0_ucode_a0.c / _b0.c) that
 * boundary-renames the symbol and re-exports the blob and the forked START_ADDR
 * scalars via getters; mec_12_1_0_ucode_get() and _get_start_addr() dispatch by
 * adapt->rev_id. Only prototypes are declared here to keep the collision out of
 * including TUs. A structural FW change (renamed file/symbol, added/removed
 * sub-component) requires updating mec_12_1_0_ucode_a0.c / _b0.c; a binary-only
 * refresh does not.
 */

#include <amdgv_basetypes.h>

struct amdgv_adapter;
enum amdgv_firmware_id;

/* A0 variant getters (mec_12_1_0_ucode_a0.c). */
unsigned char *mec_12_1_0_ucode_a0(uint32_t *size);
unsigned char *mec_12_1_0_data_a0(uint32_t *size);
void mec_12_1_0_ucode_get_start_addr_a0(uint32_t *lo, uint32_t *hi);

/* B0 variant getters (mec_12_1_0_ucode_b0.c). */
unsigned char *mec_12_1_0_ucode_b0(uint32_t *size);
unsigned char *mec_12_1_0_data_b0(uint32_t *size);
void mec_12_1_0_ucode_get_start_addr_b0(uint32_t *lo, uint32_t *hi);

/*
 * Select the MEC ucode blob for fw_id, picking the A0/B0 variant by
 * adapt->rev_id. Returns 0 on success (ptr/size filled), AMDGV_FAILURE on an
 * unknown fw_id (ptr/size left untouched).
 */
int mec_12_1_0_ucode_get(struct amdgv_adapter *adapt, enum amdgv_firmware_id fw_id,
			 unsigned char **ptr, uint32_t *size);

/*
 * Fill the MEC ucode start address LO/HI scalars for the A0/B0 variant selected
 * by adapt->rev_id.
 */
void mec_12_1_0_ucode_get_start_addr(struct amdgv_adapter *adapt, uint32_t *lo, uint32_t *hi);

/* Install the MEC accessors into adapt->ucode so callers reach them by pointer. */
void mec_12_1_0_ucode_register(struct amdgv_adapter *adapt);

#endif /* MEC_12_1_0_UCODE_WRAP_H */

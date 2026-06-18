/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AMDGV_MEMRSV_H__
#define __AMDGV_MEMRSV_H__

enum region_id {
	REGION_ID__NO_RESERVED		= 0,
	REGION_ID__PRE_OS_DISP_FW	= 1,	/* reserved for UEFI GOP/DMUB FW */
	REGION_ID__UMF			= 2,	/* TMR/UMF reserved region */
	REGION_ID__DCC_META_DATA	= 3,
	REGION_ID__VM_PAGE_FAULT	= 4,	/* GPUVM page fault region */
	REGION_ID__G7_PSTATE_APERTURE1	= 5,	/* G7 Alternate Pstate Aperture 1 */
	REGION_ID__G7_PSTATE_APERTURE2	= 6,	/* G7 Alternate Pstate Aperture 2 */
	REGION_ID__G7_PSTATE_MIRROR	= 7,	/* G7 Alternate Pstate Mirror Region */
	REGION_ID__G7_TRAINING_DATA	= 8,	/* G7 Pstate Training Data */
	REGION_ID__SPECIFIC_PURPOSE	= 9,	/* Specific purpose memory region */
	REGION_ID__VBIOS_IMAGE		= 10,	/* VBIOS image */
	/* reserved */
	REGION_ID__MAX				= 16
};

/* Per-region entry info */
struct amdgv_mem_rsv_entry {
	uint64_t size;
	uint64_t start_addr;
};

/* Memory reservation info parsed from IP discovery */
struct amdgv_mem_rsv_info {
	uint32_t count;
	struct amdgv_mem_rsv_entry entries[REGION_ID__MAX];
};

/* Check if a region entry is filled (has valid size and start_addr) */
#define MEM_RSV_REGION_FILLED(adapt, region_id) \
	((adapt)->mem_rsv_info.entries[region_id].size != 0 && \
	 (adapt)->mem_rsv_info.entries[region_id].start_addr != 0)

#endif /* __AMDGV_MEMRSV_H__ */


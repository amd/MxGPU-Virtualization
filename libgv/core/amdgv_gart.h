/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _AMDGV_GART_H_
#define _AMDGV_GART_H_

#define GART_START	(0ULL)
#define GART_SIZE	(512 << 20) /* 512M */
#define AMDGV_PDE0_PAGE_SHIFT	33
#define AMDGV_PDE0_PAGE_SIZE	(1ULL << AMDGV_PDE0_PAGE_SHIFT) /* 8G */

#define AMDGV_PTE_VALID		(1ULL << 0)
#define AMDGV_PTE_SYSTEM	(1ULL << 1)
#define AMDGV_PTE_SNOOPED	(1ULL << 2)

/* VI only */
#define AMDGV_PTE_EXECUTABLE	(1ULL << 4)

#define AMDGV_PTE_READABLE	(1ULL << 5)
#define AMDGV_PTE_WRITEABLE	(1ULL << 6)

#define AMDGV_PTE_FRAG(x)	(((x) & 0x1fULL) << 7)

/* PDE is handled as PTE for gfx v9 */
#define AMDGV_PDE_PTE		(1ULL << 54)
/* PDE is handled as PTE for gfx v12 */
#define AMDGV_PDE_PTE_GFX12		(1ULL << 63)

#define AMDGV_PDE_PTE_FLAG(adapt)	\
	((adapt->ip_versions[GC_HWIP][0] >= IP_VERSION(12, 0, 0)) ? AMDGV_PDE_PTE_GFX12 : AMDGV_PDE_PTE)

/* PDE Block Fragment Size for GFX9 */
#define AMDGV_PDE_BFS(a)		((unsigned long long)(a) << 59)
/* PDE Block Fragment Size for GFX12 */
#define AMDGV_PDE_BFS_GFX12(a)		((unsigned long long)((a) & 0x1fULL) << 58)
#define AMDGV_PDE_BFS_FLAG(adapt, a)	\
	((adapt->ip_versions[GC_HWIP][0] >= IP_VERSION(12, 0, 0)) ? AMDGV_PDE_BFS_GFX12(a) : AMDGV_PDE_BFS(a))

typedef enum MTYPE {
	MTYPE_NC = 0x00000000,
	MTYPE_WC = 0x00000001,
	MTYPE_RW = 0x00000001,
	MTYPE_CC = 0x00000002,
	MTYPE_UC = 0x00000003,
} MTYPE;

void amdgv_gart_init_pdb0(struct amdgv_adapter *adapt);
void amdgv_gart_invalidate_tlb(struct amdgv_adapter *adapt);
void amdgv_gart_map(struct amdgv_adapter *adapt, uint64_t offset, int pages,
		    uint64_t dma_addr);

#endif

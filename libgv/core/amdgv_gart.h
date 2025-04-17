/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef _AMDGV_GART_H_
#define _AMDGV_GART_H_

#define GART_START	(0ULL)
#define GART_SIZE	(512 << 20) /* 512M */

#define AMDGV_PTE_VALID		(1ULL << 0)
#define AMDGV_PTE_SYSTEM	(1ULL << 1)
#define AMDGV_PTE_SNOOPED	(1ULL << 2)

/* RV+ */
#define AMDGV_PTE_TMZ		(1ULL << 3)

/* VI only */
#define AMDGV_PTE_EXECUTABLE	(1ULL << 4)

#define AMDGV_PTE_READABLE	(1ULL << 5)
#define AMDGV_PTE_WRITEABLE	(1ULL << 6)

#define AMDGV_PTE_FRAG(x)	((x & 0x1fULL) << 7)

/* TILED for VEGA10, reserved for older ASICs  */
#define AMDGV_PTE_PRT		(1ULL << 51)

/* PDE is handled as PTE for VEGA10 */
#define AMDGV_PDE_PTE		(1ULL << 54)

#define AMDGV_PTE_LOG		(1ULL << 55)

/* PTE is handled as PDE for VEGA10 (Translate Further) */
#define AMDGV_PTE_TF		(1ULL << 56)

/* MALL noalloc for sienna_cichlid, reserved for older ASICs  */
#define AMDGV_PTE_NOALLOC	(1ULL << 58)

/* PDE Block Fragment Size for VEGA10 */
#define AMDGV_PDE_BFS(a)	((uint64_t)a << 59)

/* For GFX9 */
#define AMDGV_PTE_MTYPE_GFX9(a)	((uint64_t)(a) << 57)

typedef enum MTYPE {
	MTYPE_NC = 0x00000000,
	MTYPE_WC = 0x00000001,
	MTYPE_RW = 0x00000001,
	MTYPE_CC = 0x00000002,
	MTYPE_UC = 0x00000003,
} MTYPE;

void amdgv_gart_init_pdb0(struct amdgv_adapter *adapt);
void amdgv_gart_map(struct amdgv_adapter *adapt, uint64_t offset, int pages,
		    uint64_t dma_addr);

#endif

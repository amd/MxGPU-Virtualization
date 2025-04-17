/*
 * Copyright (c) 2018-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_MISC_H
#define AMDGV_MISC_H

enum amdgv_timeout_section {
	TIMEOUT_PSP_REG,		/* PSP register */
	TIMEOUT_PSP_MEM,		/* PSP memory */
	TIMEOUT_SMU_REG,		/* SMU register */
	TIMEOUT_SMU_IND_REG,	/* SMU indirect register */
	TIMEOUT_RESET,			/* reset */
	TIMEOUT_STATUS_REG,		/* status registers */
	TIMEOUT_GRBM_STATUS,	/* GRBM_STATUS registers */
	TIMEOUT_CMD_RESP,		/* command response */
	TIMEOUT_CP_DMA,			/* for CP DMA per 64mb copy */
	TIMEOUT_READ_VBIOS,		/* read vbios */
	TIMEOUT_MANUAL_SWITCH,	/* manual switch */
	TIMEOUT_AUTO_SWITCH_MM,	/* auto switch */
	TIMEOUT_AUTO_SWITCH_GFX, /* GFX auto switch */
	TIMEOUT_GUEST_IDH_RESP,	/* guest idh response for FLR*/
	TIMEOUT_PCI_TRANS,		/* PCIE transaction */
	TIMEOUT_BACO_HW,		/* hw recovery in BACO */
	TIMEOUT_GUEST_IDH_RESP_GPU_RESET, /* guest idh response for GPU reset*/
	TIMEOUT_LSDMA,           /* for LSDMA per 64mb copy */
	TIMEOUT_RAS_BOOT_STATUS,
	TIMEOUT_DUMP_CU_DATA,
	TIMEOUT_SEC_LEN,
};

enum amdgv_dma_engine{
	AMDGV_DMA_ENGINE_CP_DMA = 0,
	AMDGV_DMA_ENGINE_LSDMA,
	AMDGV_DMA_ENGINE_NONE,
};

struct amdgv_misc {
	/* get or set HDP nonsurface MC base */
	int (*get_hdp_nonsurface_base)(struct amdgv_adapter *adapt, uint64_t *hdp_mc_addr);
	int (*set_hdp_nonsurface_base)(struct amdgv_adapter *adapt, uint64_t hdp_mc_addr);

	/* dma engine used */
	enum amdgv_dma_engine dma_engine;

	/* use CP_DMA or LSDMA to fill or copy data to memory */
	/* if (fill_mode), src contains pattern to fill */
	int (*dma_copy)(struct amdgv_adapter *adapt, uint32_t idx_vf, bool fill_mode,
			   uint64_t src, uint64_t dst, uint64_t size, uint64_t *size_copied);
	/* ASIC specified common timeout values */
	uint64_t timeouts[TIMEOUT_SEC_LEN];

	int (*load_dfc)(struct amdgv_adapter *adapt);
	int (*update_dummy_page_addr)(struct amdgv_adapter *adapt, uint32_t idx_vf);

	/* need to reprogram golden settings during gpu_init for some ASICs */
	void (*reprogram_golden_settings)(struct amdgv_adapter *adapt, void *idx_vf);

	/* get memory config */
	uint64_t (*get_memsize)(struct amdgv_adapter *adapt);

	/* clean scratch registers */
	void (*clean_scratch_registers)(struct amdgv_adapter *adapt, uint32_t idx_vf);
};

#define AMDGV_TIMEOUT(x) adapt->misc.timeouts[(x)]

int amdgv_misc_get_hdp_nonsurface_base(struct amdgv_adapter *adapt, uint64_t *hdp_mc_addr);
int amdgv_misc_set_hdp_nonsurface_base(struct amdgv_adapter *adapt, uint64_t hdp_mc_addr);
int amdgv_misc_clear_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint8_t pattern);
int amdgv_misc_load_dfc(struct amdgv_adapter *adapt);
void amdgv_misc_reprogram_golden_settings(struct amdgv_adapter *adapt, uint32_t idx_vf);
uint64_t amdgv_misc_get_memsize(struct amdgv_adapter *adapt);
void amdgv_misc_hdp_flush(struct amdgv_adapter *adapt);

int amdgv_misc_get_agp_cpu_base(struct amdgv_adapter *adapt, void **data);
int amdgv_misc_migrate_fb(struct amdgv_adapter *adapt,
	uint32_t idx_vf, uint32_t idx_fb_block, void *data, bool to_fb);
#endif

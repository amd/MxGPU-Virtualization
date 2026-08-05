/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_misc.h>

#include <asic_reg/LSDMA/lsdma_7_1_0_offset.h>
#include <asic_reg/LSDMA/lsdma_7_1_0_sh_mask.h>


#define LSDMA_PIO_DMA_MAX_SIZE	0x3FFFFFFL		/* 64MB - 1*/

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;


static int lsdma_v7_1_wait_for_lsdma_pio_cb(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;
	uint32_t dma_status, fifo_full;

	dma_status = RREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_STATUS));
	fifo_full = REG_GET_FIELD(dma_status, LSDMA_PIO_STATUS, PIO_FIFO_FULL);

	return fifo_full;
}

#define DMA_COPY_CONTEXT_REGS 1
static struct amdgv_reg_dump_info dma_copy_context_regs[DMA_COPY_CONTEXT_REGS] = {
	{
		.name = "LSDMA_PIO_STATUS",
		.hwip = LSDMA_HWIP,
		.seg = regLSDMA_PIO_STATUS_BASE_IDX,
		.logical_inst = 0,
		.offset_hwip = regLSDMA_PIO_STATUS,
		.access_method = AMDGV_REG_DUMP_ACCESS_MMIO,
	},
};

static int lsdma_v7_1_lsdma_copy(struct amdgv_adapter *adapt, uint32_t idx_vf, bool fill_mode,
				uint64_t src, uint64_t dst, uint64_t size, uint64_t *size_copied)
{
	uint32_t dma_cmd = 0;
	uint32_t dma_size, dma_temp;
	int wait_ret;
	struct amdgv_wait_for_cb_context cb_context = { 0 };

	dma_size = (size < LSDMA_PIO_DMA_MAX_SIZE) ? size : LSDMA_PIO_DMA_MAX_SIZE;

	cb_context.ctx = (void *)adapt;
	cb_context.type = AMDGV_WAIT_FOR_LSDMA_PIO;
	cb_context.ctx_ext = dma_copy_context_regs;
	cb_context.num_ctx_ext = DMA_COPY_CONTEXT_REGS;


	*size_copied = 0;
	while ((*size_copied) < size) {

		wait_ret = amdgv_wait_for(adapt, lsdma_v7_1_wait_for_lsdma_pio_cb, &cb_context,
				AMDGV_TIMEOUT(TIMEOUT_LSDMA), 0);

		if (!wait_ret) {
			if (fill_mode) {
				WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_SRC_ADDR_LO), 0);
				WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_SRC_ADDR_HI), 0);
				WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_CONSTFILL_DATA),
						(uint32_t)(src & 0xffffffff));
			} else {
				WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_SRC_ADDR_LO),
						(uint32_t)(src & 0xffffffff));
				WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_SRC_ADDR_HI),
						(uint32_t)((src >> 32) & 0xffffffff));
			}

			WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_DST_ADDR_LO),
					(uint32_t)(dst & 0xffffffff));
			WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_DST_ADDR_HI),
					(uint32_t)((dst >> 32) & 0xffffffff));

			/* Make sure we don't go past end of region */
			if (((*size_copied) + dma_size) > size)
				dma_size = size - (*size_copied);

			dma_cmd = dma_size << LSDMA_PIO_COMMAND__BYTE_COUNT__SHIFT;

			if (fill_mode)
				dma_cmd = dma_cmd | (1 << LSDMA_PIO_COMMAND__CONSTANT_FILL__SHIFT);
			/*
			 * NOTE: writing LSDMA_PIO_COMMAND initiates operation,
			 * so write it last!
			 */
			WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_COMMAND), dma_cmd);

			AMDGV_DEBUG4("dma_cmd=0x%x dma_size=0x%x src=0x%llx dst=0x%llx\n",
					dma_cmd, dma_size, src, dst);

			/* Advance to next block of FB region to fill/copy */
			*size_copied = (*size_copied) + dma_size;
			if (!fill_mode)
				src = src + dma_size;
			dst = dst + dma_size;
		} else {
			return AMDGV_FAILURE;
		}

	}

	/* wait_dma_pio_idle */
	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET_NAME(LSDMA, 0, regLSDMA_PIO_STATUS),
					LSDMA_PIO_STATUS__PIO_IDLE_MASK, 0,
					AMDGV_TIMEOUT(TIMEOUT_LSDMA), AMDGV_WAIT_CHECK_NE, 0);
	if (wait_ret) {
		dma_temp = RREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_STATUS));
		return AMDGV_FAILURE;
	}

	return 0;

}


static int lsdma_v7_1_sw_init(struct amdgv_adapter *adapt)
{
	adapt->misc.dma_copy = lsdma_v7_1_lsdma_copy;
	adapt->misc.dma_engine = AMDGV_DMA_ENGINE_LSDMA;

	return 0;
}

static int lsdma_v7_1_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->misc.dma_copy = NULL;

	return 0;
}

static int lsdma_v7_1_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int lsdma_v7_1_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func lsdma_v7_1_func = {
	.name = "lsdma_v7_1_func",
	.sw_init = lsdma_v7_1_sw_init,
	.sw_fini = lsdma_v7_1_sw_fini,
	.hw_init = lsdma_v7_1_hw_init,
	.hw_fini = lsdma_v7_1_hw_fini,
};

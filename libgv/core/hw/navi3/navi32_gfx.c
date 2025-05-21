/*
 * Copyright (C) 2022 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE
 */

#include <amdgv_device.h>
#include <amdgv.h>
#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include "navi32_gfx.h"

#define A_MINUS_B_UNTIL_ZERO(c, a, b) ((c) = ((a) > (b)) ? (a) - (b) : 0)

static const uint32_t this_block = AMDGV_GFX_BLOCK;

static void navi32_gfx_select_se_sh(struct amdgv_adapter *adapt, uint32_t se, uint32_t sh,
				   uint32_t instance)
{
	uint32_t data;

	if (instance == ~0)
		data = REG_SET_FIELD(0, GRBM_GFX_INDEX, INSTANCE_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(0, GRBM_GFX_INDEX, INSTANCE_INDEX, instance);

	if (se == ~0)
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SE_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SE_INDEX, se);

	if (sh == ~0)
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SA_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SA_INDEX, sh);

	WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_INDEX), data);
	AMDGV_DEBUG("GRBM_GFX_INDEX=0x%x\n", data);
}

static uint32_t navi32_gfx_get_wgp_active_bitmap(struct amdgv_adapter *adapt)
{
	uint32_t data, wgp_bitmask;

	data = RREG32(SOC15_REG_OFFSET(GC, 0, regCC_GC_SHADER_ARRAY_CONFIG));
	data |= RREG32(SOC15_REG_OFFSET(GC, 0, regGC_USER_SHADER_ARRAY_CONFIG));

	data = REG_GET_FIELD(data, CC_GC_SHADER_ARRAY_CONFIG, INACTIVE_WGPS);

	wgp_bitmask = (uint32_t)((1ULL << (adapt->config.gfx.max_cu_per_sh / 2)) - 1);

	return (~data) & wgp_bitmask;
}

static uint32_t navi32_gfx_get_cu_active_bitmap(struct amdgv_adapter *adapt)
{
	uint32_t wgp_idx, wgp_active_bitmap;
	uint32_t cu_bitmap_per_wgp, cu_active_bitmap;

	wgp_active_bitmap = navi32_gfx_get_wgp_active_bitmap(adapt);
	cu_active_bitmap = 0;

	for (wgp_idx = 0; wgp_idx < 16; wgp_idx++) {
		/* If there is one WGP active, 2 CU are active */
		cu_bitmap_per_wgp = 3 << (2 * wgp_idx);
		if (wgp_active_bitmap & (1 << wgp_idx))
			cu_active_bitmap |= cu_bitmap_per_wgp;
	}

	return cu_active_bitmap;
}

uint32_t navi32_gfx_cu_count(struct amdgv_adapter *adapt)
{
	uint32_t cu_count = 0;
	uint32_t i, j, k;
	uint32_t mask, bitmap;

	for (i = 0; i < adapt->config.gfx.max_shader_engines; i++) {
		for (j = 0; j < adapt->config.gfx.max_sh_per_se; j++) {
			mask = 1;
			navi32_gfx_select_se_sh(adapt, i, j, ~0);
			bitmap = navi32_gfx_get_cu_active_bitmap(adapt);

			for (k = 0; k < adapt->config.gfx.max_cu_per_sh; k++) {
				if (bitmap & mask)
					cu_count++;
				mask <<= 1;
			}
		}
	}

	navi32_gfx_select_se_sh(adapt, ~0, ~0, ~0);
	return cu_count;
}

uint32_t navi32_gfx_atc_ats_invalidate(struct amdgv_adapter *adapt)
{
	return 0;
}

void navi32_gfx_program_golden_settings(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* cp debug, it prevents page fault. */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_DEBUG));
	tmp = REG_SET_FIELD(tmp, CP_DEBUG, CPG_UTCL1_ERROR_HALT_DISABLE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_DEBUG), tmp);

	/* prevent SQ read timeout during CWSR */
	tmp = RREG32_SOC15_RLC(GC, 0, regGRBM_CNTL);
	tmp = REG_SET_FIELD(tmp, GRBM_CNTL, READ_TIMEOUT, 0xff);
	WREG32_SOC15_RLC(GC, 0, regGRBM_CNTL, tmp);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regTCP_CNTL));
	tmp = REG_SET_FIELD(tmp, TCP_CNTL, FORCE_ORDER_BETWEEN_READ_WRITE_TO_SAME_ADDRESS, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regTCP_CNTL), tmp);
}

static int navi32_wait_for_autoload_complete_cb(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;
	uint32_t rlc_status = 0;
	uint32_t cp_status = 0;
	uint32_t bootload_complete;
	uint32_t bootload_status = 0;

	rlc_status = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_STAT));
	cp_status = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_STAT));
	if (rlc_status == 0 && cp_status == 0) {
		/* then check RLC BOOTLOAD_COMPLETE */
		bootload_status =
			RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_RLCS_BOOTLOAD_STATUS));
		bootload_complete = (REG_GET_FIELD(
			bootload_status, RLC_RLCS_BOOTLOAD_STATUS, BOOTLOAD_COMPLETE));
		if (bootload_complete)
			return 0;
	}
	return AMDGV_FAILURE;
}

int navi32_gfx_check_rlc_autoload_complete(struct amdgv_adapter *adapt)
{
	uint32_t bootload_status = 0;
	uint32_t bootload_status1 = 0;
	uint32_t rlc_status = 0;
	uint32_t cp_status = 0;
	int wait_ret;

	/* Wait for IMU to exit GFXOFF then touch the RLC_STAT */
	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(GC, 0, regGFX_IMU_MSG_FLAGS), 0x6, 0x6,
										AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_AUTO);

	if (wait_ret) {
		AMDGV_INFO("PSP: Can't wait for IMU to exit GFXOFF,"
					"GFX_IMU_MSG_FLAGS=0x%x\n",
					RREG32(SOC15_REG_OFFSET(GC, 0, regGFX_IMU_MSG_FLAGS)));
		return AMDGV_FAILURE;
	}

	wait_ret = amdgv_wait_for(adapt, navi32_wait_for_autoload_complete_cb, (void *)adapt,
								AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_FLAG_FORCE_YIELD);
	bootload_status = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_RLCS_BOOTLOAD_STATUS));

	if (wait_ret) {
		bootload_status1 = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_RLCS_BOOTLOAD_ID_STATUS1));
		rlc_status = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_STAT));
		cp_status = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_STAT));
		AMDGV_ERROR("PSP: RLC Autoload Failed. TIMEOUT after %d (usec)"
				" RLC_RLCS_BOOTLOAD_STATUS=0x%x"
				" RLC_RLCS_BOOTLOAD_ID_STATUS1=0x%x"
				" RLC_STAT=0x%x"
				" CP_STAT=0x%x\n",
				AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), bootload_status, bootload_status1, rlc_status, cp_status);
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("PSP: RLC Autoload OK. RLC_RLCS_BOOTLOAD_STATUS=0x%x\n",
			bootload_status);

	return 0;
}

void navi32_gfx_halt_gpu_state(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* halt sdma0 engines - freeze micro engine first */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_FREEZE));
	tmp = REG_SET_FIELD(tmp, SDMA0_FREEZE, FREEZE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_FREEZE), tmp);

	/* wait for FROZEN bit to be set */
	oss_udelay(1000);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_F32_CNTL));
	tmp = REG_SET_FIELD(tmp, SDMA0_F32_CNTL, HALT, 1);
	tmp = REG_SET_FIELD(tmp, SDMA0_F32_CNTL, TH1_RESET, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_F32_CNTL), tmp);

	/* halt sdma1 engines - freeze micro engine first */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_FREEZE));
	tmp = REG_SET_FIELD(tmp, SDMA1_FREEZE, FREEZE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_FREEZE), tmp);

	/* wait for FROZEN bit to be set */
	oss_udelay(1000);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_F32_CNTL));
	tmp = REG_SET_FIELD(tmp, SDMA1_F32_CNTL, HALT, 1);
	tmp = REG_SET_FIELD(tmp, SDMA1_F32_CNTL, TH1_RESET, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_F32_CNTL), tmp);

	/* halt CP (CE, ME, PFP, MEC1, MEC2) */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, ME_HALT, 1);
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, PFP_HALT, 1);
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, CE_HALT, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL), tmp);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_MEC_CNTL, MEC_ME1_HALT, 1);
	tmp = REG_SET_FIELD(tmp, CP_MEC_CNTL, MEC_ME2_HALT, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_CNTL), tmp);
}

void navi32_gfx_unhalt_gpu_state(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* unhalt sdma0 engines */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_F32_CNTL));
	tmp = REG_SET_FIELD(tmp, SDMA0_F32_CNTL, HALT, 0);
	tmp = REG_SET_FIELD(tmp, SDMA0_F32_CNTL, TH1_RESET, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_F32_CNTL), tmp);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_FREEZE));
	tmp = REG_SET_FIELD(tmp, SDMA0_FREEZE, FREEZE, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_FREEZE), tmp);

	/* unhalt sdma1 engines */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_F32_CNTL));
	tmp = REG_SET_FIELD(tmp, SDMA1_F32_CNTL, HALT, 0);
	tmp = REG_SET_FIELD(tmp, SDMA1_F32_CNTL, TH1_RESET, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_F32_CNTL), tmp);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_FREEZE));
	tmp = REG_SET_FIELD(tmp, SDMA1_FREEZE, FREEZE, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_FREEZE), tmp);

	/* setup MES PRGRM_START regsiters */
	if (adapt->ucode.prepare_ucode_engine) {
		adapt->ucode.prepare_ucode_engine(adapt, AMDGV_FIRMWARE_ID__MES_THREAD1);
		adapt->ucode.prepare_ucode_engine(adapt, AMDGV_FIRMWARE_ID__RS64_MEC_UCODE);
		adapt->ucode.prepare_ucode_engine(adapt, AMDGV_FIRMWARE_ID__RS64_ME_UCODE);
		adapt->ucode.prepare_ucode_engine(adapt, AMDGV_FIRMWARE_ID__RS64_PFP_UCODE);
	}

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL));

	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, ME_HALT, 0);
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, PFP_HALT, 0);
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, CE_HALT, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL), tmp);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_RS64_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_HALT, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_RS64_CNTL), tmp);

	/*enable SRM */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_SRM_CNTL));
	tmp = REG_SET_FIELD(tmp, RLC_SRM_CNTL, SRM_ENABLE, 1);
	tmp = REG_SET_FIELD(tmp, RLC_SRM_CNTL, AUTO_INCR_ADDR, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_SRM_CNTL), tmp);
}

static int navi32_gfx_test_cp_ring(struct amdgv_adapter *adapt)
{
#ifdef TEST_CP_RING
	uint64_t ring_gpu_addr;
	uint32_t *ring_cpu_addr;
	uint32_t i, tmp, wptr;
	struct amdgv_memmgr_mem *mem;

	mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, 0x1000, 0x1000);
	if (!mem) {
		AMDGV_ERROR("TEST CP: Failed to allocate memory!\n");
		return AMDGV_FAILURE;
	}

	ring_gpu_addr = amdgv_memmgr_get_gpu_addr(mem);
	ring_cpu_addr = (uint32_t *)amdgv_memmgr_get_cpu_addr(mem);

	AMDGV_DEBUG("TEST CP: RB_BASE 0x%llx\n", ring_gpu_addr);

	/* test for cp */
	/* Initialize the ring buffer's read and write pointers */

	/* Set the write pointer delay */
	WREG32(SOC15_REG_OFFSET(GC, 0, mmCP_RB_WPTR_DELAY), 0);

	/* set the RB to use vmid 0 */
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB_VMID), 0);

	WREG32(SOC15_REG_OFFSET(GC, 0, mmGRBM_GFX_CNTL), 0x0);
	/* RB_BUFSZ = order_base_2(0x1000 / 8) = 8
	 * RB_BLKSZ= RB_BUFSZ-2
	 * RB_NO_UPDATE = 0
	 */
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB_CNTL), 0x8000608);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB0_WPTR), 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB0_RPTR), 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, mmCP_RB0_BUFSZ_MASK), 0x7ff);
	oss_msleep(5);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB0_BASE), lower_32_bits(ring_gpu_addr >> 8));
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB0_BASE_HI), upper_32_bits(ring_gpu_addr >> 8));
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB_ACTIVE), 1);

	wptr = 0x20;
	for (i = 0; i < wptr; ++i)
		ring_cpu_addr[i] = 0xFFFF1000;
	/*submit nop*/
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB0_WPTR), wptr);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB0_WPTR_HI), 0x0);
	oss_msleep(5);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB0_RPTR));

	AMDGV_DEBUG("TEST CP: RPTR 0x%x after submit 0x%x DW of NOP\n", tmp, wptr);
	AMDGV_DEBUG("TEST CP: instr_pntr => MEC1: 0x%08x, MEC2: 0x%08x\n",
		    RREG32(SOC15_REG_OFFSET(GC, 0, mmCP_MEC1_INSTR_PNTR)),
		    RREG32(SOC15_REG_OFFSET(GC, 0, mmCP_MEC2_INSTR_PNTR)));

	/*free memory*/
	amdgv_memmgr_free(mem);

	if (tmp != wptr) {
		AMDGV_ERROR("TEST CP: failed! CP_RB0_RPTR(readback=0x%x, "
			    "expected=0x%x\n",
			    tmp, wptr);
		return AMDGV_FAILURE;
	}
#endif
	return 0;
}

static void navi32_gfx_init_perf_counter(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	navi32_gfx_select_se_sh(adapt, ~0, ~0, ~0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PERFMON_CNTL), 0x0);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regSQ_PERFCOUNTER_CTRL));
	WREG32(SOC15_REG_OFFSET(GC, 0, regSQ_PERFCOUNTER_CTRL), tmp & 0x7f);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSQ_PERFCOUNTER_CTRL2), 0x1);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regSQG_PERFCOUNTER_CTRL));
	WREG32(SOC15_REG_OFFSET(GC, 0, regSQG_PERFCOUNTER_CTRL), tmp & 0x7f);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSQG_PERFCOUNTER_CTRL2), 0x1);

	WREG32(SOC15_REG_OFFSET(GC, 0, regSQ_PERFCOUNTER0_SELECT), 491);          // 491 - SP_PERF_SEL_PERF_MEM_RD_CNT : VGPR read cnt
	WREG32(SOC15_REG_OFFSET(GC, 0, regSQ_PERFCOUNTER2_SELECT), 492);          // 492 - SP_PERF_SEL_PERF_MEM_WR_CNT : VGPR write cnt
	WREG32(SOC15_REG_OFFSET(GC, 0, regTD_PERFCOUNTER0_SELECT), 1);            // TD busy counter
	WREG32(SOC15_REG_OFFSET(GC, 0, regTA_PERFCOUNTER0_SELECT), 15);           // TA busy counter
}


static void navi32_gfx_start_perf_counter(struct amdgv_adapter *adapt)
{
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PERFMON_CNTL), 0x0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PERFMON_CNTL), 0x1);
}

static void navi32_gfx_stop_and_sample_perf_counter(struct amdgv_adapter *adapt)
{
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PERFMON_CNTL), 0x402);
}

static uint64_t navi32_gfx_read_perf_counter(struct amdgv_adapter *adapt, uint32_t perf_counter_select)
{
	uint32_t i, j;
	uint64_t sum = 0;
	uint32_t reg_lo, reg_hi;
	uint32_t wgp_active_bitmap;
	uint32_t wgp_idx;

	switch (perf_counter_select) {
	case PERFCOUNTER_SQ0:   /* For SQ PERFCOUNTER{rep}_SELECT is for PERFCOUNTER{rep/2}_LO, see doc GFX11_PerfCtr_per_WGP.docx */
		reg_lo = SOC15_REG_OFFSET(GC, 0, regSQ_PERFCOUNTER0_LO);
		reg_hi = 0;
		break;
	case PERFCOUNTER_SQ2:
		reg_lo = SOC15_REG_OFFSET(GC, 0, regSQ_PERFCOUNTER1_LO);
		reg_hi = 0;
		break;
	case PERFCOUNTER_TD:
		reg_lo = SOC15_REG_OFFSET(GC, 0, regTD_PERFCOUNTER0_LO);
		reg_hi = SOC15_REG_OFFSET(GC, 0, regTD_PERFCOUNTER0_HI);
		break;
	case PERFCOUNTER_TA:
		reg_lo = SOC15_REG_OFFSET(GC, 0, regTA_PERFCOUNTER0_LO);
		reg_hi = SOC15_REG_OFFSET(GC, 0, regTA_PERFCOUNTER0_HI);
		break;
	default:
		return 0;
	}

	if (reg_lo != 0 && perf_counter_select <= PERFCOUNTER_PER_WGP) {
		/* per wgp or per sa counters */
		for (i = 0; i < adapt->config.gfx.max_shader_engines; i++) {
			for (j = 0; j < adapt->config.gfx.max_sh_per_se; j++) {
				navi32_gfx_select_se_sh(adapt, i, j, ~0);
				wgp_active_bitmap = navi32_gfx_get_wgp_active_bitmap(adapt);
				for (wgp_idx = 0; wgp_idx < 16; wgp_idx++) {
					if (wgp_active_bitmap & (1 << wgp_idx)) {
						navi32_gfx_select_se_sh(adapt, i, j, (wgp_idx << 2));
						sum += RREG32(reg_lo);
						if (reg_hi)
							sum += (((uint64_t)RREG32(reg_hi)) << 32);
					}
				}
			}
		}
		navi32_gfx_select_se_sh(adapt, ~0, ~0, ~0);
	} else
		sum = RREG32(reg_lo) + (((uint64_t)RREG32(reg_hi)) << 32);

	return sum;
}



static bool navi32_gfx_need_hang_detection(struct amdgv_adapter *adapt)
{
	/* test any SE busy */
	uint32_t regval = 0;
	regval |= RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS_SE0));
	regval |= RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS_SE1));
	regval |= RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS_SE2));
	regval &= ~0x00000106;      /* allow DB/CB CLEAN, allow SEDC busy */
	return regval;
}
static bool navi32_gfx_is_cu_exporting(struct amdgv_adapter *adapt)
{
	/* test SX*/
	uint32_t regval = 0;
	regval |= RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS_SE0));
	regval |= RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS_SE1));
	regval |= RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS_SE2));
	return regval & 0x04000000;
}

static bool navi32_gfx_no_se_page_fault(struct amdgv_adapter *adapt)
{
	uint32_t regval, cid = 0, wke = 0, pme = 0;
	regval = RREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_STATUS));
	cid = REG_GET_FIELD(regval, GCVM_L2_PROTECTION_FAULT_STATUS, CID);
	wke = REG_GET_FIELD(regval, GCVM_L2_PROTECTION_FAULT_STATUS, WALKER_ERROR);
	pme = REG_GET_FIELD(regval, GCVM_L2_PROTECTION_FAULT_STATUS, PERMISSION_FAULTS);

	/* wke == 0 && pme == 0 means no page fault, if there is page fault, we need check if this is from SE */
	return (wke == 0 && pme == 0) || (cid != 0xa && cid != 0xb && cid != 0xc && cid != 0xd);
}

static bool navi32_gfx_is_gpu_hang(struct amdgv_adapter *adapt)
{
	uint64_t counter_sum = 0;
	counter_sum += navi32_gfx_read_perf_counter(adapt, PERFCOUNTER_SQ0);
	counter_sum += navi32_gfx_read_perf_counter(adapt, PERFCOUNTER_SQ2);
	counter_sum += navi32_gfx_read_perf_counter(adapt, PERFCOUNTER_TA);
	counter_sum += navi32_gfx_read_perf_counter(adapt, PERFCOUNTER_TD);

	if (navi32_gfx_need_hang_detection(adapt)) { /* SE still busy */
		if (counter_sum == 0) { /* this means no monitored block has work load recorded */
			if (!navi32_gfx_is_cu_exporting(adapt)) /* cu is not exporting */
				return true;
			else if (!navi32_gfx_no_se_page_fault(adapt))  /* cu is exporting, but there is page fault*/
				return true;
		}
	}
	return false;
}

static int navi32_gfx_wait_detect_hang(struct amdgv_adapter *adapt, amdgv_wait_cb_t cb_func, void *cb_context, uint64_t timeout)
{
	int wait_ret = 0;
	uint32_t hang_detection_threshold = adapt->gfx.hang_detection_threshold_us;
	uint32_t hang_detection_duration = adapt->gfx.hang_detection_duration_us;
	uint32_t time_left = timeout;

	/* phase 1, wait some time for WS command */
	wait_ret = amdgv_wait_for(adapt, cb_func, cb_context, hang_detection_threshold,
		AMDGV_WAIT_FLAG_USLEEP | AMDGV_WAIT_FLAG_NO_WARNING);
	/* time_left = time_left - hang_detection_threshold */
	A_MINUS_B_UNTIL_ZERO(time_left, time_left, hang_detection_threshold);

	if (wait_ret && time_left == 0)
		goto out;

	/* phase 2, if command didn't return in a certain time, start hang detection */
	if (adapt->gfx.hang_detection_supported && navi32_gfx_need_hang_detection(adapt) && hang_detection_duration > 0) {
		navi32_gfx_start_perf_counter(adapt);
		wait_ret = amdgv_wait_for(adapt, cb_func, cb_context, hang_detection_duration,
			AMDGV_WAIT_FLAG_USLEEP | AMDGV_WAIT_FLAG_NO_WARNING);
		/* time_left = time_left - hang_detection_duration */
		A_MINUS_B_UNTIL_ZERO(time_left, time_left, hang_detection_duration);
		navi32_gfx_stop_and_sample_perf_counter(adapt);
		if (wait_ret && (navi32_gfx_is_gpu_hang(adapt) || time_left == 0)) {
			AMDGV_ERROR("Hang detection detected GFX hang!\n");
			goto out;
		}
	}

	/* phase 3, if busy or can't decide, wait for the reset of time until the hard limit */
	wait_ret = amdgv_wait_for(adapt, cb_func, cb_context, time_left,
		AMDGV_WAIT_FLAG_USLEEP);
	time_left = 0;
out:
	return wait_ret;
}

struct amdgv_gfx_funcs navi32_gfx_funcs = {
	.wait_detect_hang = navi32_gfx_wait_detect_hang
};


static int navi32_gfx_sw_init(struct amdgv_adapter *adapt)
{
	adapt->gfx.funcs = &navi32_gfx_funcs;
	return 0;
}

static int navi32_gfx_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_gfx_check_rlc_autoload_complete(adapt);
	if (ret)
		return AMDGV_FAILURE;

	if (adapt->ip_discovery.parse_gc_table(adapt))
		return AMDGV_FAILURE;

	navi32_gfx_unhalt_gpu_state(adapt);
	ret = navi32_gfx_test_cp_ring(adapt);

	if (ret && adapt->sched.dump_gpu_state)
		adapt->sched.dump_gpu_state(adapt);

	if (ret)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_INIT_FAIL, 0);

	if (adapt->gfx.hang_detection_supported)
		navi32_gfx_init_perf_counter(adapt);

	return ret;
}

static int navi32_gfx_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int navi32_gfx_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}


const struct amdgv_init_func navi32_gfx_func = {
	.name = "navi32_gfx_func",
	.sw_init = navi32_gfx_sw_init,
	.sw_fini = navi32_gfx_sw_fini,
	.hw_init = navi32_gfx_hw_init,
	.hw_fini = navi32_gfx_hw_fini,
};

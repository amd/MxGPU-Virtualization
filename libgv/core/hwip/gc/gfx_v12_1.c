/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv.h>
#include <amdgv_sched_internal.h>

#include "gfx_v12_1.h"
#include "amdgv_mes.h"
#include "asic_reg/GC/gc_12_1_0_offset.h"
#include "asic_reg/GC/gc_12_1_0_sh_mask.h"
#include "gfx_v12_1_doorbell.h"


static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static int gfx_v12_1_xcc_set_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t data;
	int ret;

	data = RLC_SAFE_MODE__CMD_MASK;
	data |= (1 << RLC_SAFE_MODE__MESSAGE__SHIFT);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE, data);

	ret = amdgv_wait_for_register(
		adapt, SOC15_REG_OFFSET_NAME(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE),
		RLC_SAFE_MODE__CMD_MASK, 0, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG),
		AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_YIELD);

	if (ret) {
		return AMDGV_FAILURE;
	}

	return 0;
}

static int gfx_v12_1_xcc_unset_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t data;

	data = RLC_SAFE_MODE__CMD_MASK;
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE, data);

	return 0;
}

static bool gfx_v12_1_is_rlc_enabled(struct amdgv_adapter *adapt)
{
	uint32_t rlc_setting;

	rlc_setting = RREG32_SOC15(GC, GET_INST(GC, 0), regRLC_CNTL);
	if (!(rlc_setting & RLC_CNTL__RLC_ENABLE_F32_MASK))
		return false;

	return true;
}

static const struct amdgv_rlc_funcs gfx_v12_1_rlc_funcs = {
	.set_safe_mode = gfx_v12_1_xcc_set_safe_mode,
	.unset_safe_mode = gfx_v12_1_xcc_unset_safe_mode,
	.is_rlc_enabled = gfx_v12_1_is_rlc_enabled,
};

static void gfx_v12_1_set_rlc_funcs(struct amdgv_adapter *adapt)
{
	adapt->gfx.rlc.funcs = &gfx_v12_1_rlc_funcs;
}

void gfx_v12_1_grbm_select(struct amdgv_adapter *adapt, uint32_t me,
			uint32_t pipe, uint32_t queue, uint32_t vmid, int xcc_id)
{
	uint32_t grbm_gfx_cntl = 0;

	grbm_gfx_cntl =
		REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, PIPEID, pipe);
	grbm_gfx_cntl =
		REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, MEID, me);
	grbm_gfx_cntl =
		REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, VMID, vmid);
	grbm_gfx_cntl =
		REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, QUEUEID, queue);

	WREG32_SOC15_RLC_SHADOW(GC, GET_INST(GC, xcc_id), regGRBM_GFX_CNTL, grbm_gfx_cntl);
}

static int gfx_v12_1_wait_for_autoload_complete_cb(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;
	uint32_t rlc_status = 0;
	uint32_t cp_status = 0;
	uint32_t bootload_complete;
	uint32_t bootload_status = 0;
	uint32_t xcd_id;

	bool xcd_pending = false;

	for (xcd_id = 0; xcd_id < adapt->mcp.gfx.num_xcc; xcd_id++) {
		rlc_status = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regRLC_STAT));
		cp_status = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regCP_STAT));
		if (rlc_status == 0 && cp_status == 0) {
			bootload_status = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regRLC_RLCS_BOOTLOAD_STATUS));
			bootload_complete = (REG_GET_FIELD(bootload_status, RLC_RLCS_BOOTLOAD_STATUS, BOOTLOAD_COMPLETE));
			if (!bootload_complete)
				xcd_pending = true;
		} else {
			xcd_pending = true;
		}
	}

	return (xcd_pending ? AMDGV_FAILURE : 0);
}

static int gfx_v12_1_gfx_check_rlc_autoload_complete(struct amdgv_adapter *adapt)
{
	int wait_ret;
	uint32_t xcd_id = 0;
	struct amdgv_wait_for_cb_context cb_context = { 0 };

	for (xcd_id = 0; xcd_id < adapt->mcp.gfx.num_xcc; xcd_id++) {
		/* Wait for IMU to exit GFXOFF then touch the RLC_STAT */
		wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET_NAME(GC, GET_INST(GC, xcd_id), regGFX_IMU_MSG_FLAGS),
						0x6, 0x6, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG),
						AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_AUTO);

		if (wait_ret) {
			AMDGV_INFO("Can't wait for IMU to exit GFXOFF. GFX_IMU_MSG_FLAGS=0x%x\n",
				RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regGFX_IMU_MSG_FLAGS)));
			return AMDGV_FAILURE;
		}
	}

	cb_context.ctx = (void *)adapt;
	cb_context.type = AMDGV_WAIT_FOR_RLC_AUTOLOAD_COMPLETE;
	wait_ret = amdgv_wait_for(adapt, gfx_v12_1_wait_for_autoload_complete_cb, &cb_context,
				  AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_FLAG_FORCE_YIELD);

	if (wait_ret) {
		for (xcd_id = 0; xcd_id < adapt->mcp.gfx.num_xcc; xcd_id++) {
			AMDGV_ERROR("RLC Autoload Failed. TIMEOUT after %d (usec) [%d] "
				    " RLC_RLCS_BOOTLOAD_STATUS=0x%x"
				    " RLC_RLCS_BOOTLOAD_ID_STATUS1=0x%x"
				    " RLC_STAT=0x%x"
				    " CP_STAT=0x%x\n",
				    xcd_id,
				    AMDGV_TIMEOUT(TIMEOUT_STATUS_REG),
				    RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regRLC_RLCS_BOOTLOAD_STATUS)),
				    RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regRLC_RLCS_BOOTLOAD_ID_STATUS1)),
				    RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regRLC_STAT)),
				    RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regCP_STAT)));
		}
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("RLC Autoload OK.\n");

	return 0;
}

int gfx_v12_1_prepare_engine(struct amdgv_adapter *adapt,
			  uint32_t ucode_id, uint32_t xcd_id)
{
	uint32_t tmp, pipe;
	uint64_t uc_start_addr = 0;
	int ret = 0;

	if (adapt->ucode.get_ucode_start_addr)
		ret = adapt->ucode.get_ucode_start_addr(adapt, ucode_id, &uc_start_addr);

	if (ret || !uc_start_addr) {
		AMDGV_ERROR("Failed to get ucode start address for firmware id %d\n", ucode_id);
		return AMDGV_FAILURE;
	}

	switch (ucode_id) {
	case AMDGV_FIRMWARE_ID__MES_THREAD1:
		tmp = RREG32_SOC15(GC, GET_INST(GC, xcd_id), regCP_MES_CNTL);
		tmp = REG_SET_FIELD(tmp, CP_MES_CNTL, MES_PIPE0_ACTIVE, 0);
		tmp = REG_SET_FIELD(tmp, CP_MES_CNTL, MES_PIPE1_ACTIVE, 0);
		tmp = REG_SET_FIELD(tmp, CP_MES_CNTL,
					MES_INVALIDATE_ICACHE, 1);
		tmp = REG_SET_FIELD(tmp, CP_MES_CNTL, MES_PIPE0_RESET, 1);
		tmp = REG_SET_FIELD(tmp, CP_MES_CNTL, MES_PIPE1_RESET, 1);
		tmp = REG_SET_FIELD(tmp, CP_MES_CNTL, MES_HALT, 1);
		WREG32_SOC15(GC, GET_INST(GC, xcd_id), regCP_MES_CNTL, tmp);

		oss_mutex_lock(adapt->srbm_mutex);
		gfx_v12_1_grbm_select(adapt, 3, 0, 0, 0, xcd_id);
		/* set ucode start address */
		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regCP_MES_PRGRM_CNTR_START),
				lower_32_bits(uc_start_addr));
		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regCP_MES_PRGRM_CNTR_START_HI),
				upper_32_bits(uc_start_addr));

		gfx_v12_1_grbm_select(adapt, 3, 1, 0, 0, xcd_id);
		/* set ucode start address */
		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regCP_MES_PRGRM_CNTR_START),
				lower_32_bits(uc_start_addr));
		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regCP_MES_PRGRM_CNTR_START_HI),
				upper_32_bits(uc_start_addr));

		gfx_v12_1_grbm_select(adapt, 0, 0, 0, 0, xcd_id);
		oss_mutex_unlock(adapt->srbm_mutex);

		oss_udelay(50);

		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
		/* MEC SET FOR 4 pipes */
		oss_mutex_lock(adapt->srbm_mutex);
		for (pipe = 0; pipe < 4; pipe++) {
			gfx_v12_1_grbm_select(adapt, 1, pipe, 0, 0, xcd_id);

			WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regCP_MEC_RS64_PRGRM_CNTR_START), lower_32_bits(uc_start_addr));
			WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcd_id), regCP_MEC_RS64_PRGRM_CNTR_START_HI), upper_32_bits(uc_start_addr));
		}

		gfx_v12_1_grbm_select(adapt, 0, 0, 0, 0, xcd_id);
		oss_mutex_unlock(adapt->srbm_mutex);

		/* reset mec pipe */
		tmp = RREG32_SOC15(GC, GET_INST(GC, xcd_id), regCP_MEC_RS64_CNTL);
		tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE0_RESET, 1);
		tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE1_RESET, 1);
		tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE2_RESET, 1);
		tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE3_RESET, 1);
		WREG32_SOC15(GC, GET_INST(GC, xcd_id), regCP_MEC_RS64_CNTL, tmp);

		oss_udelay(50);

		/* clear mec pipe reset */
		tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE0_RESET, 0);
		tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE1_RESET, 0);
		tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE2_RESET, 0);
		tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE3_RESET, 0);
		WREG32_SOC15(GC, GET_INST(GC, xcd_id), regCP_MEC_RS64_CNTL, tmp);

		break;
	default:
		AMDGV_INFO("Engine with ucode ID %d doesn't needs to prepare\n",
			ucode_id);
		break;
	}

	return 0;
}


static void gfx_v12_1_xcc_cp_compute_enable(struct amdgv_adapter *adapt,
						  bool enable, int xcc_id)
{
	uint32_t data;

	data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MEC_RS64_CNTL);
	data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_INVALIDATE_ICACHE,
					enable ? 0 : 1);
	data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE0_RESET,
					enable ? 0 : 1);
	data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE1_RESET,
					enable ? 0 : 1);
	data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE2_RESET,
					enable ? 0 : 1);
	data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE3_RESET,
					enable ? 0 : 1);
	data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE0_ACTIVE,
					enable ? 1 : 0);
	data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE1_ACTIVE,
					enable ? 1 : 0);
	data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE2_ACTIVE,
					enable ? 1 : 0);
	data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE3_ACTIVE,
					enable ? 1 : 0);
	data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_HALT,
					enable ? 0 : 1);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MEC_RS64_CNTL, data);

	oss_udelay(50);
}


static bool gfx_v12_1_is_gfx_off(struct amdgv_adapter *adapt)
{
	uint32_t i;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		if (0x6 != RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, i), regGFX_IMU_MSG_FLAGS))) {
			return true;
		}
	}

	return false;
}

static int gfx_v12_1_set_poison_interrupt_routing(struct amdgv_adapter *adapt, bool route_to_pf)
{
	uint32_t data;
	uint32_t i;

	/*
	 * Program RLC_CNTL.POISON_INTERRUPT_TO_PF for each XCC instance.
	 * RLC FW reads this field and drives Force_To_PF in the IH Cookie
	 * accordingly.
	 */
	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		data = RREG32_SOC15(GC, GET_INST(GC, i), regRLC_CNTL);
		data = REG_SET_FIELD(data, RLC_CNTL, POISON_INTERRUPT_TO_PF, route_to_pf ? 1 : 0);
		WREG32_SOC15(GC, GET_INST(GC, i), regRLC_CNTL, data);
	}

	return 0;
}

static uint32_t gfx_v12_1_get_num_xcc_in_xcp(struct amdgv_adapter *adapt)
{
	switch (adapt->mcp.accelerator_partition_mode) {
		case AMDGV_ACCELERATOR_PARTITION_MODE_SPX:
			return adapt->mcp.gfx.num_xcc;
		case AMDGV_ACCELERATOR_PARTITION_MODE_DPX:
			return adapt->mcp.gfx.num_xcc / 2;
		case AMDGV_ACCELERATOR_PARTITION_MODE_QPX:
			return adapt->mcp.gfx.num_xcc / 4;
		case AMDGV_ACCELERATOR_PARTITION_MODE_CPX:
			return adapt->mcp.gfx.num_xcc / 8;
		default:
			AMDGV_ERROR("Invalid accelerator partition mode\n");
			return 1;
	}
}

struct amdgv_gfx_funcs gfx_v12_1_funcs = {
	.check_rlc_autoload_complete = gfx_v12_1_gfx_check_rlc_autoload_complete,
	.is_gfx_off = gfx_v12_1_is_gfx_off,
	.get_num_xcc_in_xcp = gfx_v12_1_get_num_xcc_in_xcp,
	.set_clockgating_state = gfx_v12_1_set_clockgating_state,
	.set_poison_interrupt_routing = gfx_v12_1_set_poison_interrupt_routing,
};

static void gfx_v12_1_xcc_init_compute_vmid(struct amdgv_adapter *adapt, int xcc_id)
{
	int i;
	uint32_t sh_mem_bases;
	uint32_t data;

	/*
	 * Configure apertures:
	 * LDS:         0x20000000'00000000 - 0x20000001'00000000 (4GB)
	 * Scratch:     0x10000000'00000000 - 0x10000001'00000000 (4GB)
	 */

	sh_mem_bases = REG_SET_FIELD(0, SH_MEM_BASES, PRIVATE_BASE, SCRATCH_APP_BASE);
	sh_mem_bases = REG_SET_FIELD(sh_mem_bases, SH_MEM_BASES, SHARED_BASE, LDS_APP_BASE);

	oss_mutex_lock(adapt->srbm_mutex);
	for (i = FIRST_KFD_VMID; i < AMDGV_NUM_VMID; i++) {
		gfx_v12_1_grbm_select(adapt, 0, 0, 0, i, xcc_id);
		/* CP and shaders */
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regSH_MEM_CONFIG, DEFAULT_SH_MEM_CONFIG);
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regSH_MEM_BASES, sh_mem_bases);

		/* Enable trap for each kfd vmid. */
		data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regSPI_GDBG_PER_VMID_CNTL);
		data = REG_SET_FIELD(data, SPI_GDBG_PER_VMID_CNTL, TRAP_EN, 1);
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regSPI_GDBG_PER_VMID_CNTL, data);

		/* Disable VGPR deallocation instruction for each KFD vmid. */
		data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regSQ_DEBUG);
		data = REG_SET_FIELD(data, SQ_DEBUG, DISABLE_VGPR_DEALLOC, 1);
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regSQ_DEBUG, data);
	}
	gfx_v12_1_grbm_select(adapt, 0, 0, 0, 0, xcc_id);
	oss_mutex_unlock(adapt->srbm_mutex);
}

static void gfx_v12_1_xcc_constants_init(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t tmp;
	int i;

	oss_mutex_lock(adapt->srbm_mutex);
	for (i = 0; i < FIRST_KFD_VMID; i++) {
		gfx_v12_1_grbm_select(adapt, 0, 0, 0, i, xcc_id);
		WREG32_SOC15(GC, GET_INST(GC, xcc_id),
			     regSH_MEM_CONFIG, DEFAULT_SH_MEM_CONFIG);
		if (i != 0) {
			tmp = REG_SET_FIELD(0, SH_MEM_BASES, PRIVATE_BASE, SCRATCH_APP_BASE);
			tmp = REG_SET_FIELD(tmp, SH_MEM_BASES, SHARED_BASE, LDS_APP_BASE);
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regSH_MEM_BASES, tmp);
		}
	}
	gfx_v12_1_grbm_select(adapt, 0, 0, 0, 0, xcc_id);
	oss_mutex_unlock(adapt->srbm_mutex);

	gfx_v12_1_xcc_init_compute_vmid(adapt, xcc_id);
}

static void gfx_v12_1_constants_init(struct amdgv_adapter *adapt)
{
	int i;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++)
		gfx_v12_1_xcc_constants_init(adapt, i);
}

// TODO: This is not completed. Partly implement for WS cmd verfication
static int gfx_v12_1_sw_init(struct amdgv_adapter *adapt)
{
	adapt->gfx.funcs = &gfx_v12_1_funcs;

	gfx_v12_1_set_rlc_funcs(adapt);

	gfx_v12_1_doorbell_index_init(adapt);

	adapt->gfx.mec.num_mec = 2;
	adapt->gfx.mec.num_pipe_per_mec = 4;
	adapt->gfx.mec.num_queue_per_pipe = 8;

	return 0;
}

static int gfx_v12_1_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static void gfx_v12_1_enable_interrupt(struct amdgv_adapter *adapt, uint32_t xcc_id, bool enable)
{
	uint32_t tmp;
	int i;

	/* Enable CP interrupt for all hw queues */
	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_INT_CNTL_RING0);

	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0, CNTX_BUSY_INT_ENABLE,
			enable ? 1 : 0);
	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0, CNTX_EMPTY_INT_ENABLE,
			enable ? 1 : 0);
	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0, CMP_BUSY_INT_ENABLE,
			enable ? 1 : 0);
	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0, GFX_IDLE_INT_ENABLE,
			enable ? 1 : 0);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_INT_CNTL_RING0, tmp);

	oss_mutex_lock(adapt->srbm_mutex);
	for (i = 0; i < 2; i++) {
		gfx_v12_1_grbm_select(adapt, 3, i, 0, 0, xcc_id);
		tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCPC_INT_CNTL);
		tmp = REG_SET_FIELD(tmp, CPC_INT_CNTL, TIME_STAMP_INT_ENABLE, enable ? 1 : 0);
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCPC_INT_CNTL, tmp);
	}
	gfx_v12_1_grbm_select(adapt, 0, 0, 0, 0, xcc_id);
	oss_mutex_unlock(adapt->srbm_mutex);
}

static void gfx_v12_1_cp_set_doorbell_range(struct amdgv_adapter *adapt, uint32_t xcc_id)
{
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_RB_DOORBELL_RANGE_LOWER, 0);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_RB_DOORBELL_RANGE_UPPER, 0);

	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MEC_DOORBELL_RANGE_LOWER,
			((adapt->doorbell_index.kiq +
			  xcc_id * adapt->doorbell_index.xcc_doorbell_range) * 2) << 2);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MEC_DOORBELL_RANGE_UPPER,
			((adapt->doorbell_index.userqueue_end +
			  xcc_id * adapt->doorbell_index.xcc_doorbell_range) * 2) << 2);
}

static int gfx_v12_1_hw_init_xcc(struct amdgv_adapter *adapt, uint32_t xcc_id)
{
	int r = 0;

	r = gfx_v12_1_prepare_engine(adapt, AMDGV_FIRMWARE_ID__RS64_MEC_UCODE, xcc_id);
	if (r) {
		AMDGV_ERROR("Failed to prepare engine for firmware id %d\n",
			    AMDGV_FIRMWARE_ID__RS64_MEC_UCODE);
		return r;
	}

	if (adapt->enable_mes_kiq) {
		gfx_v12_1_cp_set_doorbell_range(adapt, xcc_id);

		gfx_v12_1_xcc_cp_compute_enable(adapt, true, xcc_id);

		gfx_v12_1_enable_interrupt(adapt, xcc_id, true);

		r = amdgv_mes_kiq_hw_init(adapt, xcc_id);
		if (r) {
			AMDGV_ERROR("Failed to init MES KIQ on xcc %d\n", xcc_id);
			gfx_v12_1_enable_interrupt(adapt, xcc_id, false);
			gfx_v12_1_xcc_cp_compute_enable(adapt, false, xcc_id);
		}
	} else {
		// KIQ must be enabled on gfx12.1
		AMDGV_ERROR("MES KIQ is not enabled on xcc %d\n", xcc_id);
		return AMDGV_FAILURE;
	}

	return r;
}

static int gfx_v12_1_hw_init(struct amdgv_adapter *adapt)
{
	int i;

	gfx_v12_1_constants_init(adapt);

	if (amdgv_nbio_gc_doorbell_init(adapt))
		return AMDGV_FAILURE;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++)
		if (gfx_v12_1_hw_init_xcc(adapt, i))
			return AMDGV_FAILURE;

	return 0;
}

static int gfx_v12_1_hw_fini_xcc(struct amdgv_adapter *adapt, uint32_t xcc_id)
{
	gfx_v12_1_enable_interrupt(adapt, xcc_id, false);

	if (adapt->enable_mes_kiq) {
		if (amdgv_mes_kiq_hw_fini(adapt, xcc_id))
			AMDGV_ERROR("MES KIQ hw fini failed on xcc %d, "
				    "proceeding with compute disable\n", xcc_id);
	}

	gfx_v12_1_xcc_cp_compute_enable(adapt, false, xcc_id);

	return 0;
}

static int gfx_v12_1_hw_fini(struct amdgv_adapter *adapt)
{
	int i;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++)
		gfx_v12_1_hw_fini_xcc(adapt, i);

	return 0;
}

struct amdgv_init_func gfx_v12_1_func = {
	.name = "gfx_v12_1_func",
	.sw_init = gfx_v12_1_sw_init,
	.sw_fini = gfx_v12_1_sw_fini,
	.hw_init = gfx_v12_1_hw_init,
	.hw_fini = gfx_v12_1_hw_fini,
};

int gfx_v12_1_hw_resume(struct amdgv_adapter *adapt, uint32_t xcc_mask)
{
	uint32_t xcc_id;
	int r = 0;

	for_each_id (xcc_id, xcc_mask) {
		if (gfx_v12_1_hw_init_xcc(adapt, xcc_id)) {
			AMDGV_ERROR("Failed to resume MES KIQ on xcc %d\n", xcc_id);
			r = AMDGV_FAILURE;
		}
	}

	return r;
}

int gfx_v12_1_hw_suspend(struct amdgv_adapter *adapt, uint32_t xcc_mask)
{
	uint32_t xcc_id;

	for_each_id (xcc_id, xcc_mask)
		gfx_v12_1_hw_fini_xcc(adapt, xcc_id);

	return 0;
}

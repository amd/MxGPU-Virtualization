/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include "amdgv_nbio.h"
#include "amdgv_nps.h"

#include "asic_reg/NBIO/nbio_6_3_2_offset.h"
#include "asic_reg/NBIO/nbio_6_3_2_sh_mask.h"

#include "atombios/atomfirmware.h"
#include "smu/smuio_v15_0_8.h"
#include "vcn/vcn_v5_0_2.h"
#include "psp/psp_v15_0_8.h"
#include "smu/smu_v15_0_8_internal.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

#define BUILD_NUM_MAX_LENGTH 8
#define BUILD_DATE_LENGTH    17

static int nbio_v6_3_2_vbios_special_version_check(struct amdgv_adapter *adapt, uint8_t *img)
{
	int i, j, k;
	/* Build_num is defined as 8 bytes length. When the build_num is lower than 8
	 * bytes, there will be 0x00 added in the end until 8 bytes is used, which is
	 * not part of the anchor.
	 */
	int build_num_length;
	uint8_t anchor[] = { 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00 };
	uint8_t part_info[128] = { '\0' };
	uint8_t build_date[18] = { '\0' };

	/* search for the first 2k vbios space*/
	for (i = 0; i < 2048 - sizeof(anchor); i++) {
		for (j = 0; j < sizeof(anchor) && ((i + j) < 2048); j++) {
			if (anchor[j] != img[i + j])
				break;
		}

		if (j == sizeof(anchor)) {
			AMDGV_INFO("found anchor at %d\n", i);

			for (k = 0; k < 128; k++) {
				if (img[i + j + k] == '\\' || img[i + j + k] == 0x0)
					break;
			}

			for (build_num_length = 0; build_num_length < BUILD_NUM_MAX_LENGTH;
			     build_num_length++) {
				if (img[i - BUILD_NUM_MAX_LENGTH + build_num_length] == 0x0)
					break;
			}

		if (k < 128) {
			/* Save build_num to adapter structure */
			oss_memset(adapt->vbios.build_num, 0,
				   sizeof(adapt->vbios.build_num));
			oss_memcpy(adapt->vbios.build_num, img + i - BUILD_NUM_MAX_LENGTH,
				   build_num_length);
			AMDGV_INFO("build num: %s\n", adapt->vbios.build_num);

			oss_memcpy(part_info, img + i + j, k);
			AMDGV_INFO("part info: %s\n", part_info);
			oss_memcpy(build_date, img + 0x50, BUILD_DATE_LENGTH);
			AMDGV_INFO("build date: %s\n", build_date);
			return 0;
		}
		}
	}

	return AMDGV_FAILURE;
}

static void nbio_v6_3_2_ih_doorbell_range(struct amdgv_adapter *adapt, bool use_doorbell,
				   int doorbell_index)
{
	uint32_t ih_doorbell_range = 0;
	uint32_t ih_doorbell_range1 = 0;

	if (!use_doorbell)
		return;

	ih_doorbell_range = REG_SET_FIELD(ih_doorbell_range, GDC_S2A0_S2A_DOORBELL_ENTRY_1_CTRL,
					  S2A_DOORBELL_PORT1_RANGE_OFFSET, doorbell_index);
	ih_doorbell_range = REG_SET_FIELD(ih_doorbell_range, GDC_S2A0_S2A_DOORBELL_ENTRY_1_CTRL,
					  S2A_DOORBELL_PORT1_RANGE_SIZE, 0x8);

	ih_doorbell_range = REG_SET_FIELD(ih_doorbell_range, GDC_S2A0_S2A_DOORBELL_ENTRY_1_CTRL,
					  S2A_DOORBELL_PORT1_ENABLE, 1);
	ih_doorbell_range = REG_SET_FIELD(ih_doorbell_range, GDC_S2A0_S2A_DOORBELL_ENTRY_1_CTRL,
					  S2A_DOORBELL_PORT1_AWID, 0);
	ih_doorbell_range = REG_SET_FIELD(ih_doorbell_range, GDC_S2A0_S2A_DOORBELL_ENTRY_1_CTRL,
					  S2A_DOORBELL_PORT1_AWADDR_31_28_VALUE, 0);

	ih_doorbell_range1 = REG_SET_FIELD(ih_doorbell_range1, GDC_S2A0_S2A_DOORBELL_ENTRY_1_CTRL1,
					   S2A_DOORBELL_PORT1_TARGET_PORT_TYPE, 0x3);
	ih_doorbell_range1 = REG_SET_FIELD(ih_doorbell_range1, GDC_S2A0_S2A_DOORBELL_ENTRY_1_CTRL1,
					   S2A_DOORBELL_PORT1_TARGET_DIEID, 0x0);
	ih_doorbell_range1 = REG_SET_FIELD(ih_doorbell_range1, GDC_S2A0_S2A_DOORBELL_ENTRY_1_CTRL1,
					   S2A_DOORBELL_PORT1_TARGET_PORT_ID, 0x0);

	WREG32_SOC15(NBIO, 0, regGDC_S2A0_S2A_DOORBELL_ENTRY_1_CTRL, ih_doorbell_range);
	WREG32_SOC15(NBIO, 0, regGDC_S2A0_S2A_DOORBELL_ENTRY_1_CTRL1, ih_doorbell_range1);
}

static void nbio_v6_3_2_enable_doorbell_aperture(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t doorbell_aper;

	doorbell_aper = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_RCC_DOORBELL_APER_EN));
	doorbell_aper = REG_SET_FIELD(doorbell_aper, RCC_DEV0_EPF0_RCC_DOORBELL_APER_EN,
				      BIF_DOORBELL_APER_EN, enable ? 1 : 0);

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_RCC_DOORBELL_APER_EN), doorbell_aper);
}

static void nbio_v6_3_2_enable_ih_interrupt(struct amdgv_adapter *adapt, bool use_bus_addr,
					    uint32_t ih_base_addr)
{
	uint32_t interrupt_cntl;

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL2), ih_base_addr);

	interrupt_cntl = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL));
	interrupt_cntl = REG_SET_FIELD(interrupt_cntl, BIF_BX0_INTERRUPT_CNTL, IH_REQ_NONSNOOP_EN,
				       use_bus_addr ? 0 : 1);
	interrupt_cntl = REG_SET_FIELD(interrupt_cntl, BIF_BX0_INTERRUPT_CNTL, GEN_IH_INT_EN, 1);

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL), interrupt_cntl);
}

static void nbio_v6_3_2_gc_doorbell_init(struct amdgv_adapter *adapt)
{
	/* GC doorbell — values match upstream (0x30000007 / 0x3) */
	uint32_t doorbell_ctrl = 0;
	uint32_t doorbell_ctrl1 = 0;

	doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, GDC_S2A0_S2A_DOORBELL_ENTRY_0_CTRL,
				      S2A_DOORBELL_PORT0_ENABLE, 1);
	doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, GDC_S2A0_S2A_DOORBELL_ENTRY_0_CTRL,
				      S2A_DOORBELL_PORT0_AWID, 3);
	doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, GDC_S2A0_S2A_DOORBELL_ENTRY_0_CTRL,
				      S2A_DOORBELL_PORT0_AWADDR_31_28_VALUE, 3);
	WREG32_SOC15(NBIO, 0, regGDC_S2A0_S2A_DOORBELL_ENTRY_0_CTRL, doorbell_ctrl);

	doorbell_ctrl1 = REG_SET_FIELD(doorbell_ctrl1, GDC_S2A0_S2A_DOORBELL_ENTRY_0_CTRL1,
				       S2A_DOORBELL_PORT0_TARGET_PORT_TYPE, 3);
	WREG32_SOC15(NBIO, 0, regGDC_S2A0_S2A_DOORBELL_ENTRY_0_CTRL1, doorbell_ctrl1);
}

static uint32_t nbio_v6_3_2_get_memsize(struct amdgv_adapter *adapt)
{
    return RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_RCC_CONFIG_MEMSIZE));
}

static void nbio_v6_3_2_assign_sdma_doorbell(struct amdgv_adapter *adapt,
						int instance, bool use_doorbell,
						int doorbell_index, int doorbell_size)
{
	if (instance == 0) {
		uint32_t doorbell_range = 0;
		uint32_t doorbell_range1 = 0;

		if (use_doorbell) {
			doorbell_range = REG_SET_FIELD(doorbell_range,
								GDC_S2A0_S2A_DOORBELL_ENTRY_6_CTRL,
								S2A_DOORBELL_PORT6_ENABLE,
								0x1);
			doorbell_range = REG_SET_FIELD(doorbell_range,
								GDC_S2A0_S2A_DOORBELL_ENTRY_6_CTRL,
								S2A_DOORBELL_PORT6_AWID,
								0xe);
			doorbell_range = REG_SET_FIELD(doorbell_range,
								GDC_S2A0_S2A_DOORBELL_ENTRY_6_CTRL,
								S2A_DOORBELL_PORT6_RANGE_OFFSET,
								doorbell_index);
			doorbell_range = REG_SET_FIELD(doorbell_range,
								GDC_S2A0_S2A_DOORBELL_ENTRY_6_CTRL,
								S2A_DOORBELL_PORT6_RANGE_SIZE,
								doorbell_size);
			doorbell_range = REG_SET_FIELD(doorbell_range,
								GDC_S2A0_S2A_DOORBELL_ENTRY_6_CTRL,
								S2A_DOORBELL_PORT6_AWADDR_31_28_VALUE,
								0xe);
			doorbell_range1 = REG_SET_FIELD(doorbell_range1,
								GDC_S2A0_S2A_DOORBELL_ENTRY_6_CTRL1,
								S2A_DOORBELL_PORT6_TARGET_PORT_TYPE,
								0x3);
			doorbell_range1 = REG_SET_FIELD(doorbell_range1,
								GDC_S2A0_S2A_DOORBELL_ENTRY_6_CTRL1,
								S2A_DOORBELL_PORT6_TARGET_DIEID,
								0x0);
			doorbell_range1 = REG_SET_FIELD(doorbell_range1,
								GDC_S2A0_S2A_DOORBELL_ENTRY_6_CTRL1,
								S2A_DOORBELL_PORT6_TARGET_PORT_ID,
								0x0);
		}

		WREG32_SOC15(NBIO, 0, regGDC_S2A0_S2A_DOORBELL_ENTRY_6_CTRL, doorbell_range);
		WREG32_SOC15(NBIO, 0, regGDC_S2A0_S2A_DOORBELL_ENTRY_6_CTRL1, doorbell_range1);
	}
}

static uint64_t nbio_v6_3_2_encode_ext_smn_addressing(uint32_t ext_id)
{
	uint64_t ext_offset;
	int socket_id, die_id;

	/* local die routing for MID0 on local socket */
	if (ext_id == 0)
		return 0;

	die_id = ext_id & 0x3;
	socket_id = (ext_id >> 6) & 0xff;

	/* Initiated from host, accessing to non-MID0 is cross-die traffic */
	if (socket_id == 0)
		ext_offset = ((uint64_t)die_id << 34) | (1ULL << 32);
	else if (socket_id != 0 && die_id != 0)
		ext_offset = ((uint64_t)socket_id << 40) | ((uint64_t)die_id << 34) |
				(3ULL << 32);
	else
		ext_offset = ((uint64_t)socket_id << 40) | (1ULL << 33);

	return ext_offset;
}


/* Supported VF / memory-partition (NPS) / accelerator-partition combinations.
 * The various N-way CPX partitionings all map to the single CPX enum value.
 */
static const struct amdgv_nps_combination_cap_entry nps_cap_table[] = {
	{CHIP_IP_DISCOVERY, .vf_nps =
		{
			{
			.vf_num = 1,
			.combinations = {
				{	AMDGV_MEMORY_PARTITION_MODE_NPS1,
					AMDGV_ACCELERATOR_PARTITION_MODE_SPX
				},
				{
					AMDGV_MEMORY_PARTITION_MODE_NPS2,
					AMDGV_ACCELERATOR_PARTITION_MODE_CPX},
				}
			},
			{
			.vf_num = 2,
			.combinations = {
				{	AMDGV_MEMORY_PARTITION_MODE_NPS1,
					AMDGV_ACCELERATOR_PARTITION_MODE_DPX
				},
				{
					AMDGV_MEMORY_PARTITION_MODE_NPS2,
					AMDGV_ACCELERATOR_PARTITION_MODE_DPX},
				{
					AMDGV_MEMORY_PARTITION_MODE_NPS2,
					AMDGV_ACCELERATOR_PARTITION_MODE_CPX},
				}
			},
			{
			.vf_num = 4,
			.combinations = {
				{	AMDGV_MEMORY_PARTITION_MODE_NPS1,
					AMDGV_ACCELERATOR_PARTITION_MODE_QPX
				},
				{
					AMDGV_MEMORY_PARTITION_MODE_NPS2,
					AMDGV_ACCELERATOR_PARTITION_MODE_QPX},
				{
					AMDGV_MEMORY_PARTITION_MODE_NPS2,
					AMDGV_ACCELERATOR_PARTITION_MODE_CPX},
				}
			},
			{
			.vf_num = 6,
			.combinations = {
				{	AMDGV_MEMORY_PARTITION_MODE_NPS1,
					AMDGV_ACCELERATOR_PARTITION_MODE_CPX
				},
				{
					AMDGV_MEMORY_PARTITION_MODE_NPS2,
					AMDGV_ACCELERATOR_PARTITION_MODE_CPX},
				}
			},
			{
			.vf_num = 8,
			.combinations = {
				{	AMDGV_MEMORY_PARTITION_MODE_NPS1,
					AMDGV_ACCELERATOR_PARTITION_MODE_CPX
				},
				{
					AMDGV_MEMORY_PARTITION_MODE_NPS2,
					AMDGV_ACCELERATOR_PARTITION_MODE_CPX},
				}
			},
		}
	},
};

static int nbio_v6_3_2_get_asic_nps_caps(struct amdgv_adapter *adapt,
				 const struct amdgv_nps_compute_combination **combination)
{
	const struct amdgv_vf_nps_combination *vf_nps;
	int i, j;

	for (i = 0; i < ARRAY_SIZE(nps_cap_table); i++) {
		vf_nps = nps_cap_table[i].vf_nps;

		for (j = 0; j < AMDGV_VF_NPS_MAX_COMBINATIONS; j++) {
			if (vf_nps[j].vf_num == adapt->num_vf) {
				*combination = vf_nps[j].combinations;
				return 0;
			}
		}
	}

	AMDGV_ERROR("dev_id 0x%x with num_vf %d not found in NPS cap table!\n",
		    adapt->dev_id, adapt->num_vf);
	return AMDGV_FAILURE;
}

static int nbio_v6_3_2_get_supported_memory_partition_mode(struct amdgv_adapter *adapt,
						    enum amdgv_memory_partition_mode *supported_nps,
						    int *supported_nps_count)
{
	const struct amdgv_vf_nps_combination *vf_nps;
	const struct amdgv_nps_compute_combination *comb;
	int i, j, k;
	uint64_t cap = 0, mask = 0;
	uint32_t count = 0;

	for (i = 0; i < ARRAY_SIZE(nps_cap_table); i++) {
		vf_nps = nps_cap_table[i].vf_nps;

		for (j = 0; j < AMDGV_VF_NPS_MAX_COMBINATIONS; j++) {
			comb = vf_nps[j].combinations;
			for (k = 0; k < AMDGV_NPS_COMPUTE_MAX_COMBINATIONS; k++) {
				switch (comb[k].nps_mode) {
					case AMDGV_MEMORY_PARTITION_MODE_NPS1:
					case AMDGV_MEMORY_PARTITION_MODE_NPS2:
					case AMDGV_MEMORY_PARTITION_MODE_NPS4:
					case AMDGV_MEMORY_PARTITION_MODE_NPS8:
						cap = BIT64(comb[k].nps_mode);
						if ((mask & cap) == 0) {
							mask |= cap;
							supported_nps[count] = comb[k].nps_mode;
							count++;
						}
						break;
					default:
						continue;
				}
			}
		}
	}

	*supported_nps_count = count;
	return 0;
}

static int nbio_v6_3_2_get_nps_mode(struct amdgv_adapter *adapt,
				    enum amdgv_memory_partition_mode *memory_partition_mode)
{
	uint32_t mem_status;
	uint32_t mem_mode;

	mem_status = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_PARTITION_MEM_STATUS));
	mem_mode = REG_GET_FIELD(mem_status, BIF_BX_PF0_PARTITION_MEM_STATUS, NPS_MODE);

	switch (mem_mode) {
	case BIT(0):
		*memory_partition_mode = AMDGV_MEMORY_PARTITION_MODE_NPS1;
		break;
	case BIT(1):
		*memory_partition_mode = AMDGV_MEMORY_PARTITION_MODE_NPS2;
		break;
	case BIT(3):
		*memory_partition_mode = AMDGV_MEMORY_PARTITION_MODE_NPS4;
		break;
	case BIT(7):
		*memory_partition_mode = AMDGV_MEMORY_PARTITION_MODE_NPS8;
		break;
	default:
		AMDGV_ERROR("Unknown NPS mode\n");
		return AMDGV_FAILURE;
		break;
	}

	return 0;
}

static enum amdgv_accelerator_partition_mode
nbio_v6_3_2_get_accelerator_partition_mode(struct amdgv_adapter *adapt)
{
	uint32_t num_xcc_in_xcp;

	if (!adapt->gfx.funcs || !adapt->gfx.funcs->get_num_xcc_in_xcp)
		return 0;

	num_xcc_in_xcp = adapt->gfx.funcs->get_num_xcc_in_xcp(adapt);
	if (num_xcc_in_xcp > adapt->mcp.gfx.num_xcc)
		AMDGV_ERROR("Exceed the limit %d, num_xcc_in_xcp=%u\n", adapt->mcp.gfx.num_xcc,
			    num_xcc_in_xcp);

	switch (num_xcc_in_xcp) {
		case 1:
		case 2:
		case 4:
		case 8:
			return adapt->mcp.gfx.num_xcc / num_xcc_in_xcp;
		default:
			AMDGV_ERROR("Unknown num_xcc_in_xcp=%u\n", num_xcc_in_xcp);
			return 0;
	}
}

static enum amdgv_accelerator_partition_mode
	nbio_v6_3_2_get_accelerator_partition_mode_default_setting(
	struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode memory_partition_mode)
{
	switch (adapt->num_vf) {
	case 1:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
			return AMDGV_ACCELERATOR_PARTITION_MODE_SPX;
		case AMDGV_MEMORY_PARTITION_MODE_NPS2:
			return AMDGV_ACCELERATOR_PARTITION_MODE_CPX;
		default:
			break;
		}
		break;
	case 2:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
		case AMDGV_MEMORY_PARTITION_MODE_NPS2:
			return AMDGV_ACCELERATOR_PARTITION_MODE_DPX;
		default:
			break;
		}
		break;
	case 4:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
		case AMDGV_MEMORY_PARTITION_MODE_NPS2:
			return AMDGV_ACCELERATOR_PARTITION_MODE_QPX;
		default:
			break;
		}
		break;
	case 6:
	case 8:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
		case AMDGV_MEMORY_PARTITION_MODE_NPS2:
			return AMDGV_ACCELERATOR_PARTITION_MODE_CPX;
		default:
			break;
		}
		break;
	}

	AMDGV_ERROR("Unkown default accel partition mode setting for num_vf=%u\n", adapt->num_vf);
	return AMDGV_ACCELERATOR_PARTITION_MODE_UNKNOWN;
}

static bool nbio_v6_3_2_is_partition_mode_combination_supported(struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode memory_partition_mode,
	enum amdgv_accelerator_partition_mode accelerator_partition_mode)
{
	int i;
	const struct amdgv_nps_compute_combination *comb;

	if (nbio_v6_3_2_get_asic_nps_caps(adapt, &comb))
		return false;

	/* Check if the combination is supported in the capability table */
	for (i = 0; i < AMDGV_NPS_COMPUTE_MAX_COMBINATIONS; i++) {
		if (comb[i].nps_mode == memory_partition_mode &&
			comb[i].compute_mode == accelerator_partition_mode) {
			return true;
		}
	}

	return false;
}

const struct amdgv_nbio_funcs nbio_v6_3_2_funcs = {
	.ih_doorbell_range = nbio_v6_3_2_ih_doorbell_range,
	.enable_doorbell_aperture = nbio_v6_3_2_enable_doorbell_aperture,
	.enable_ih_interrupt = nbio_v6_3_2_enable_ih_interrupt,
	.gc_doorbell_init = nbio_v6_3_2_gc_doorbell_init,
	.get_memsize = 	nbio_v6_3_2_get_memsize,
	.sdma_doorbell_range = nbio_v6_3_2_assign_sdma_doorbell,
	.get_nps_mode = nbio_v6_3_2_get_nps_mode,
	.get_accel_partition_mode = nbio_v6_3_2_get_accelerator_partition_mode,
	.get_default_accel_partition_mode = nbio_v6_3_2_get_accelerator_partition_mode_default_setting,
	.is_partition_mode_supported = nbio_v6_3_2_is_partition_mode_combination_supported,
	.get_supported_memory_partition_mode = nbio_v6_3_2_get_supported_memory_partition_mode,
	.get_asic_nps_caps = nbio_v6_3_2_get_asic_nps_caps,
};

const struct amdgv_nbio_ras nbio_v6_3_2_ras_funcs = {
	.handle_ras_controller_intr_no_bifring = NULL,
	.handle_ras_err_event_athub_intr_no_bifring = NULL,
	.set_ras_err_event_athub_irq_state = NULL,
	.set_ras_controller_irq_state = NULL,
	.query_ras_error_count = NULL,
};

static void nbio_v6_3_2_enable_func_doorbell_access(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t vf0_en = 0;
	uint32_t vf1_en = 0;
	uint32_t vf2_en = 0;
	uint32_t vf3_en = 0;
	uint32_t vf4_en = 0;
	uint32_t vf5_en = 0;
	uint32_t vf6_en = 0;
	uint32_t vf7_en = 0;
	uint32_t pf_en  = GDC0_DOORBELL_ACCESS_EN_PF__DOORBELL_ACCESS_EN_PF_MASK;

	switch (adapt->num_vf) {
	case 1:
		vf0_en = ((0xff << GDC0_DOORBELL_ACCESS_EN_VF0__XCD_ACCESS_EN_VF0__SHIFT) |
				  (0x7f << GDC0_DOORBELL_ACCESS_EN_VF0__DOORBELL_ACCESS_EN_VF0__SHIFT));
		break;
	case 2:
		vf0_en = ((0x0f << GDC0_DOORBELL_ACCESS_EN_VF0__XCD_ACCESS_EN_VF0__SHIFT) |
				  (0x67 << GDC0_DOORBELL_ACCESS_EN_VF0__DOORBELL_ACCESS_EN_VF0__SHIFT));
		vf1_en = ((0xf0 << GDC0_DOORBELL_ACCESS_EN_VF1__XCD_ACCESS_EN_VF1__SHIFT) |
				  (0x79 << GDC0_DOORBELL_ACCESS_EN_VF1__DOORBELL_ACCESS_EN_VF1__SHIFT));
		break;
	case 4:
		vf0_en = ((0x03 << GDC0_DOORBELL_ACCESS_EN_VF0__XCD_ACCESS_EN_VF0__SHIFT) |
				  (0x63 << GDC0_DOORBELL_ACCESS_EN_VF0__DOORBELL_ACCESS_EN_VF0__SHIFT));
		vf1_en = ((0x0c << GDC0_DOORBELL_ACCESS_EN_VF1__XCD_ACCESS_EN_VF1__SHIFT) |
				  (0x65 << GDC0_DOORBELL_ACCESS_EN_VF1__DOORBELL_ACCESS_EN_VF1__SHIFT));
		vf2_en = ((0x30 << GDC0_DOORBELL_ACCESS_EN_VF2__XCD_ACCESS_EN_VF2__SHIFT) |
				  (0x69 << GDC0_DOORBELL_ACCESS_EN_VF2__DOORBELL_ACCESS_EN_VF2__SHIFT));
		vf3_en = ((0xc0 << GDC0_DOORBELL_ACCESS_EN_VF3__XCD_ACCESS_EN_VF3__SHIFT) |
				  (0x71 << GDC0_DOORBELL_ACCESS_EN_VF3__DOORBELL_ACCESS_EN_VF3__SHIFT));
		break;
	case 8:
		vf0_en = ((0x01 << GDC0_DOORBELL_ACCESS_EN_VF0__XCD_ACCESS_EN_VF0__SHIFT) |
				  (0x63 << GDC0_DOORBELL_ACCESS_EN_VF0__DOORBELL_ACCESS_EN_VF0__SHIFT));
		vf1_en = ((0x02 << GDC0_DOORBELL_ACCESS_EN_VF1__XCD_ACCESS_EN_VF1__SHIFT) |
				  (0x63 << GDC0_DOORBELL_ACCESS_EN_VF1__DOORBELL_ACCESS_EN_VF1__SHIFT));
		vf2_en = ((0x04 << GDC0_DOORBELL_ACCESS_EN_VF2__XCD_ACCESS_EN_VF2__SHIFT) |
				  (0x65 << GDC0_DOORBELL_ACCESS_EN_VF2__DOORBELL_ACCESS_EN_VF2__SHIFT));
		vf3_en = ((0x08 << GDC0_DOORBELL_ACCESS_EN_VF3__XCD_ACCESS_EN_VF3__SHIFT) |
				  (0x65 << GDC0_DOORBELL_ACCESS_EN_VF3__DOORBELL_ACCESS_EN_VF3__SHIFT));
		vf4_en = ((0x10 << GDC0_DOORBELL_ACCESS_EN_VF4__XCD_ACCESS_EN_VF4__SHIFT) |
				  (0x69 << GDC0_DOORBELL_ACCESS_EN_VF4__DOORBELL_ACCESS_EN_VF4__SHIFT));
		vf5_en = ((0x20 << GDC0_DOORBELL_ACCESS_EN_VF5__XCD_ACCESS_EN_VF5__SHIFT) |
				  (0x69 << GDC0_DOORBELL_ACCESS_EN_VF5__DOORBELL_ACCESS_EN_VF5__SHIFT));
		vf6_en = ((0x40 << GDC0_DOORBELL_ACCESS_EN_VF6__XCD_ACCESS_EN_VF6__SHIFT) |
				  (0x71 << GDC0_DOORBELL_ACCESS_EN_VF6__DOORBELL_ACCESS_EN_VF6__SHIFT));
		vf7_en = ((0x80 << GDC0_DOORBELL_ACCESS_EN_VF7__XCD_ACCESS_EN_VF7__SHIFT) |
				  (0x71 << GDC0_DOORBELL_ACCESS_EN_VF7__DOORBELL_ACCESS_EN_VF7__SHIFT));
		break;
	default:
		AMDGV_ERROR("Need to add support for Enabling Doorbell for num_vf=%d\n", adapt->num_vf);
	}

	// regGDC0_DOORBELL_ACCESS_EN_PF and regGDC0_DOORBELL_ACCESS_EN_VF0 has same address.
	// PF/VF1/VF3/VF5 uses bits 0:15, VF0/VF2/VF4/VF6 uses bits 16:31
	WREG32_SOC15(NBIO, 0, regGDC0_DOORBELL_ACCESS_EN_PF,  (enable ? ((vf0_en << 16) |  pf_en) : 0));
	WREG32_SOC15(NBIO, 0, regGDC1_DOORBELL_ACCESS_EN_PF,  (enable ? ((vf0_en << 16) |  pf_en) : 0));
	WREG32_SOC15(NBIO, 0, regGDC0_DOORBELL_ACCESS_EN_VF1, (enable ? ((vf2_en << 16) | vf1_en) : 0));
	WREG32_SOC15(NBIO, 0, regGDC1_DOORBELL_ACCESS_EN_VF1, (enable ? ((vf2_en << 16) | vf1_en) : 0));
	WREG32_SOC15(NBIO, 0, regGDC0_DOORBELL_ACCESS_EN_VF3, (enable ? ((vf4_en << 16) | vf3_en) : 0));
	WREG32_SOC15(NBIO, 0, regGDC1_DOORBELL_ACCESS_EN_VF3, (enable ? ((vf4_en << 16) | vf3_en) : 0));
	WREG32_SOC15(NBIO, 0, regGDC0_DOORBELL_ACCESS_EN_VF5, (enable ? ((vf6_en << 16) | vf5_en) : 0));
	WREG32_SOC15(NBIO, 0, regGDC1_DOORBELL_ACCESS_EN_VF5, (enable ? ((vf6_en << 16) | vf5_en) : 0));
	WREG32_SOC15(NBIO, 0, regGDC0_DOORBELL_ACCESS_EN_VF7, (enable ? (vf7_en) : 0));
	WREG32_SOC15(NBIO, 0, regGDC1_DOORBELL_ACCESS_EN_VF7, (enable ? (vf7_en) : 0));
}

static void nbio_v6_3_2_set_xcd_doorbell_fence(struct amdgv_adapter *adapt)
{
	WREG32_SOC15(NBIO, 0, regXCD_DOORBELL_FENCE_1,
		(0xff & ~(adapt->mcp.gfx.xcc_mask)) <<
		XCD_DOORBELL_FENCE_1__XCD_0_DOORBELL_DISABLE__SHIFT);
}

static int nbio_v6_3_2_enable_pci_atomic_request(struct amdgv_adapter *adapt)
{
	uint16_t val;
	int pos;

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID__PCIE);
	if (!pos) {
		AMDGV_ERROR("this device does not support capability: %x\n", PCI_CAP_ID__PCIE);
		return AMDGV_FAILURE;
	}

	oss_pci_read_config_word(adapt->dev, pos + PCIE_DEVICE_CONTROL2, &val);
	val |= (PCIE_DEVICE_CONTROL2__ATOMICOP_REQ);
	oss_pci_write_config_word(adapt->dev, pos + PCIE_DEVICE_CONTROL2, val);

	oss_pci_read_config_word(adapt->dev, pos + PCIE_DEVICE_CAP2, &val);
	adapt->pcie_atomic_ops_support_flags =
		val & (PCIE_DEVICE_CAP2__ATOMIC_COMP32 | PCIE_DEVICE_CAP2__ATOMIC_COMP64);

	AMDGV_INFO("Atomic Request Enabled\n");

	return 0;
}

static int nbio_v6_3_2_disable_pci_atomic_request(struct amdgv_adapter *adapt)
{
	uint16_t val;
	int pos;

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID_EXP);
	if (!pos) {
		AMDGV_ERROR("this device does not support capability: %x\n", PCI_CAP_ID__PCIE);
		return AMDGV_FAILURE;
	}

	oss_pci_read_config_word(adapt->dev, pos + PCI_EXP_DEVCTL2, &val);
	val &= ~(PCI_EXP_DEVCTL2_ATOMICOP_REQ);
	oss_pci_write_config_word(adapt->dev, pos + PCI_EXP_DEVCTL2, val);

	return 0;
}

static void nbio_v6_3_2_enable_vf_access_mmio_over_512k(struct amdgv_adapter *adapt)
{
	uint32_t val;

	val = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP9));
	val |= RCC_STRAP0_RCC_DEV0_EPF0_STRAP9__STRAP_NBIF_ROM_BAR_DIS_CHICKEN_DEV0_F0_MASK;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP9), val);
}

static void nbio_v6_3_2_disable_vf_flr(struct amdgv_adapter *adapt)
{
	uint32_t val;

	val = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4));
	val &= ~RCC_STRAP0_RCC_DEV0_EPF0_STRAP4__STRAP_FLR_EN_DEV0_F0_MASK;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4), val);
}

static void nbio_v6_3_2_assign_sdma_to_vf(struct amdgv_adapter *adapt)
{
	AMDGV_ERROR("NOT IMPLEMENTED!\n");
}

static void nbio_v6_3_2_assign_mmsch_doorbell(struct amdgv_adapter *adapt)
{
	vcn_v5_0_2_set_mmsch_doorbell_addr_base(adapt);
}

static void nbio_v6_3_2_clear_dummy_mode(struct amdgv_adapter *adapt)
{
	uint32_t val;

	val = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BACO_CNTL));

	if (val & (BIF_BX0_BACO_CNTL__BACO_DUMMY_EN_MASK | BIF_BX0_BACO_CNTL__BACO_EN_MASK)) {
		val &= ~(BIF_BX0_BACO_CNTL__BACO_DUMMY_EN_MASK |
				BIF_BX0_BACO_CNTL__BACO_EN_MASK);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BACO_CNTL), val);
	}
}

static int nbio_v6_3_2_vbios_read_rom_from_reg(struct amdgv_adapter *adapt,
					       uint8_t *bios, uint32_t length_bytes)
{
	struct amdgv_vbios *vbios = &adapt->vbios;
	int ret;

	if (vbios->ip_discovery_image && vbios->ip_discovery_image_size > 0) {
		AMDGV_INFO("Reading VBIOS from ATOMBIOS Table\n");
		oss_memcpy(bios, vbios->ip_discovery_image, length_bytes);
		ret = 0;
	} else {
		AMDGV_ERROR("No VBIOS image found\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static bool nbio_v6_3_2_vbios_need_post(struct amdgv_adapter *adapt)
{
	uint32_t val;

	val = RREG32_SOC15(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7);
	AMDGV_DEBUG("BIOS_SCRATCH_7 = 0x%08x\n", val);

	if (val & ATOM_ASIC_INIT_COMPLETE) {
		val &= ~ATOM_ASIC_INIT_COMPLETE;
		WREG32_SOC15(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7, val);

		AMDGV_INFO("ATOM_ASIC_POSTED\n");
		return false;
	}

	AMDGV_INFO("ATOM_ASIC_NEED_POST\n");
	return true;
}

static int nbio_v6_3_2_sw_init(struct amdgv_adapter *adapt)
{
	const char *name = "GC_V12_1_0";

	adapt->nbio.funcs = &nbio_v6_3_2_funcs;
	adapt->nbio.ras = &nbio_v6_3_2_ras_funcs;

	adapt->vbios.is_atom_fw = true;

	adapt->smuio.funcs = &smuio_v15_0_8_funcs;
	adapt->vbios.read_rom_from_reg = nbio_v6_3_2_vbios_read_rom_from_reg;
	adapt->vbios.special_version_check = nbio_v6_3_2_vbios_special_version_check;
	adapt->nbio.encode_ext_smn_addressing = nbio_v6_3_2_encode_ext_smn_addressing;

	/* Check if GPU is connected to CPU via XGMI (A+A mode) */
	if (adapt->smuio.funcs->is_host_gpu_xgmi_supported) {
		adapt->xgmi.connected_to_cpu =
			adapt->smuio.funcs->is_host_gpu_xgmi_supported(adapt);

		AMDGV_INFO("XGMI: connected_to_cpu = %s\n",
		   adapt->xgmi.connected_to_cpu ? "true" : "false");
	}

	adapt->flags |= AMDGV_FLAG_IH_REG_PSP_EN;
	adapt->flags |= AMDGV_FLAG_GC_REG_RLC_EN;

	oss_memcpy(adapt->config.name, name, oss_strlen(name));

	amdgv_vbios_atom_sw_init(adapt);

	return 0;
}

static int nbio_v6_3_2_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->nbio.funcs = NULL;
	adapt->nbio.ras = NULL;

	amdgv_vbios_atom_sw_fini(adapt);

	return 0;
}

static int nbio_v6_3_2_hw_init(struct amdgv_adapter *adapt)
{
	int ret;

	if (!in_whole_gpu_reset()) {
		if (amdgv_vbios_read_img(adapt))
			return AMDGV_FAILURE;
	}

	if (amdgv_vbios_atom_hw_init(adapt))
		goto fail;

	/* VBIOS POST will be set on driver reload.
	 * If set, trigger a reset, then continue reload. */
	if (!nbio_v6_3_2_vbios_need_post(adapt)) {
		if (psp_v15_0_8_wait_sos_loaded_status(adapt) && smu_v15_0_8_is_fw_alive(adapt)) {
			ret = amdgv_reset_hw_for_reload(adapt, false);
			if (ret)
				goto fail;
		}
	}

	if (amdgv_atomfirmware_post(adapt, VBIOS_POST_ASIC_INIT))
		goto fail;

	if (amdgv_atomfirmware_set_fw_usage_fb_guest(adapt))
		goto fail;

	if (amdgv_atomfirmware_get_vram_info(adapt))
		goto fail;

	/* Get XGMI info for A+A configurations */
	if (adapt->xgmi.connected_to_cpu) {
		ret = amdgv_mmhub_get_xgmi_info(adapt);
		if (ret) {
			AMDGV_ERROR("Failed to get XGMI info\n");
			goto fail;
		}
	}

	nbio_v6_3_2_clear_dummy_mode(adapt);
	nbio_v6_3_2_enable_func_doorbell_access(adapt, true);
	nbio_v6_3_2_set_xcd_doorbell_fence(adapt);
	nbio_v6_3_2_assign_mmsch_doorbell(adapt);
	nbio_v6_3_2_assign_sdma_to_vf(adapt);

	nbio_v6_3_2_enable_pci_atomic_request(adapt);
	nbio_v6_3_2_enable_vf_access_mmio_over_512k(adapt);
	nbio_v6_3_2_disable_vf_flr(adapt);

	return 0;

fail:
	amdgv_vbios_atom_hw_fini(adapt);

	return AMDGV_FAILURE;
}

static int nbio_v6_3_2_hw_fini(struct amdgv_adapter *adapt)
{
	amdgv_vbios_atom_hw_fini(adapt);

	if (in_whole_gpu_reset())
		return 0;

	nbio_v6_3_2_disable_pci_atomic_request(adapt);
	nbio_v6_3_2_enable_func_doorbell_access(adapt, false);

	if (adapt->vbios.image) {
		oss_free_memory(adapt->vbios.image);
		adapt->vbios.image = NULL;
	}

	if (adapt->vbios.guest_image) {
		oss_free_memory(adapt->vbios.guest_image);
		adapt->vbios.guest_image = NULL;
	}

	return 0;
}

const struct amdgv_init_func nbio_v6_3_2_func = {
	.name = "nbio_v6_3_2_func",
	.sw_init = nbio_v6_3_2_sw_init,
	.sw_fini = nbio_v6_3_2_sw_fini,
	.hw_init = nbio_v6_3_2_hw_init,
	.hw_fini = nbio_v6_3_2_hw_fini,
};

/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_ras.h>

#include "navi32_reg_inc.h"
#include "umc_v8_10.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

#define EEPROM_TABLE_VERSION_NV3	0x00021000

#define EEPROM_I2C_TARGET_ADDR_NAVI3        0xA8
#define EEPROM_I2C_CONTROLLER_PORT_NAVI3    1

#define U32_MAX ((unsigned int)~0U)

#define UMC_V8_10_NA_COL_2BITS_POWER_OF_2_NUM	 4

/* The C5 bit in NA  address */
#define UMC_V8_10_NA_C5_BIT	14

/* Map to swizzle mode address */
#define SWIZZLE_MODE_TMP_ADDR(na, ch_num, ch_idx) \
		((((na) >> 10) * (ch_num) + (ch_idx)) << 10)
#define SWIZZLE_MODE_ADDR_HI(addr, col_bit)  \
		(((addr) >> ((col_bit) + 2)) << ((col_bit) + 2))
#define SWIZZLE_MODE_ADDR_MID(na, col_bit) ((((na) >> 8) & 0x3) << (col_bit))
#define SWIZZLE_MODE_ADDR_LOW(addr, col_bit) \
		((((addr) >> 10) & ((0x1ULL << (col_bit - 8)) - 1)) << 8)
#define SWIZZLE_MODE_ADDR_LSB(na) ((na) & 0xFF)

struct channelnum_map_colbit {
	uint32_t channel_num;
	uint32_t col_bit;
};

const struct channelnum_map_colbit umc_v8_10_channelnum_map_colbit_table[] = {
	{24, 13},
	{20, 13},
	{16, 12},
	{14, 12},
	{12, 12},
	{10, 12},
	{6,  11},
};

const uint32_t umc_v8_10_channel_idx_tbl_nv32[]
					[UMC_V8_10_UMC_INSTANCE_NUM]
					[UMC_V8_10_CHANNEL_INSTANCE_NUM] = {
	{ {1,   5}, {7,   3} },
	{ {14, 15}, {13, 12} },
	{ {10, 11}, {9,   8} },
	{ {6,   2}, {0,   4} },
};

static inline uint32_t get_umc_v8_10_reg_offset(struct amdgv_adapter *adapt,
					    uint32_t node_inst,
					    uint32_t umc_inst,
					    uint32_t ch_inst)
{
	return adapt->umc.channel_offs * ch_inst + adapt->umc.inst_offs * umc_inst +
		adapt->umc.node_offs * node_inst;
}

static void umc_v8_10_loop_umc_channels(struct amdgv_adapter *adapt,
				umc_query_info query_info, void *ras_error_status)
{
	uint32_t node_inst       = 0;
	uint32_t umc_inst        = 0;
	uint32_t ch_inst         = 0;
	uint32_t eccinfo_table_idx   = 0;

	LOOP_UMC_NODE_INST_AND_CH(node_inst, umc_inst, ch_inst) {
		/* skip harvest umc node */
		if (!((1ULL << node_inst) & adapt->umc.active_mask))
			continue;

		eccinfo_table_idx = node_inst * adapt->umc.umc_inst_num *
				adapt->umc.channel_inst_num +
				umc_inst * adapt->umc.channel_inst_num +
				ch_inst;

		/* skip disabled chaneel*/
		if (!((1ULL << eccinfo_table_idx) & adapt->umc.channel_mask))
			continue;

		query_info(adapt, ras_error_status, node_inst, ch_inst, umc_inst);
	}
}

static void umc_v8_10_query_correctable_error_count(struct amdgv_adapter *adapt,
						uint32_t node_inst,	uint32_t umc_inst,
						uint32_t ch_inst, unsigned long *error_count)
{
	uint32_t ecc_err_cnt;
	uint32_t eccinfo_table_idx;

	eccinfo_table_idx = node_inst * adapt->umc.umc_inst_num *
				adapt->umc.channel_inst_num +
				umc_inst * adapt->umc.channel_inst_num +
				ch_inst;

	ecc_err_cnt = adapt->ecc.umc_ecc.ecc[eccinfo_table_idx].ce_count_lo_chip;
	/* increase error count */
	if (ecc_err_cnt)
		*error_count += ecc_err_cnt;
}

static void umc_v8_10_query_uncorrectable_error_count(struct amdgv_adapter *adapt,
						uint32_t node_inst, uint32_t umc_inst,
						uint32_t ch_inst, unsigned long *error_count)
{
	uint64_t mc_umc_status;
	uint32_t eccinfo_table_idx;

	eccinfo_table_idx = node_inst * adapt->umc.umc_inst_num *
				adapt->umc.channel_inst_num +
				umc_inst * adapt->umc.channel_inst_num +
				ch_inst;

	/* Check the MCUMC_STATUS. */
	mc_umc_status = adapt->ecc.umc_ecc.ecc[eccinfo_table_idx].mca_umc_status;
	if ((REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Val) == 1) &&
	    (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Deferred) == 1 ||
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UECC) == 1 ||
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, PCC) == 1 ||
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UC) == 1 ||
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, TCC) == 1))
		*error_count += 1;
}

static void umc_v8_10_query_ecc_error_count(struct amdgv_adapter *adapt,
						void *ras_error_status, uint32_t node_inst,
						uint32_t ch_inst, uint32_t umc_inst)
{
	struct ras_err_data *err_data = (struct ras_err_data *)ras_error_status;
	umc_v8_10_query_correctable_error_count(adapt,
					node_inst, umc_inst, ch_inst,
					&(err_data->ce_count));
	umc_v8_10_query_uncorrectable_error_count(adapt,
					node_inst, umc_inst, ch_inst,
					&(err_data->ue_count));
}

static void umc_v8_10_hw_query_uncorrectable_error_count(struct amdgv_adapter *adapt,
			void *ras_error_status, uint32_t node_inst, uint32_t umc_inst, uint32_t ch_inst)
{
	struct ras_err_data *err_data = (struct ras_err_data *)ras_error_status;
	uint64_t mc_umc_status;
	uint32_t mc_umc_status_addr;

	uint32_t umc_reg_offset =
		get_umc_v8_10_reg_offset(adapt, node_inst, umc_inst, ch_inst);

	mc_umc_status_addr = SOC15_REG_OFFSET(UMC, 0, regMCA_UMC_UMC0_MCUMC_STATUST0);
	mc_umc_status = RREG64_PCIE((mc_umc_status_addr + umc_reg_offset));
	if ((REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Val) == 1) &&
	    (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Deferred) == 1 ||
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UECC) == 1 ||
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, PCC) == 1 ||
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UC) == 1 ||
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, TCC) == 1))
		err_data->ue_count += 1;
}

static void umc_v8_10_hw_query_ras_error_count(struct amdgv_adapter *adapt,
						void *ras_error_status)
{
	umc_v8_10_loop_umc_channels(adapt,
		umc_v8_10_hw_query_uncorrectable_error_count, ras_error_status);
}

static void umc_v8_10_query_ras_error_count(struct amdgv_adapter *adapt,
						void *ras_error_status)
{
	umc_v8_10_loop_umc_channels(adapt,
		umc_v8_10_query_ecc_error_count, ras_error_status);
}

static uint32_t umc_v8_10_get_col_bit(uint32_t channel_num)
{
	uint32_t t = 0;

	for (t = 0; t < ARRAY_SIZE(umc_v8_10_channelnum_map_colbit_table); t++)
		if (channel_num == umc_v8_10_channelnum_map_colbit_table[t].channel_num)
			return umc_v8_10_channelnum_map_colbit_table[t].col_bit;

	/* Failed to get col_bit. */
	return U32_MAX;
}

/*
 * Mapping normal address to soc physical address in swizzle mode.
 */
static int umc_v8_10_swizzle_mode_na_to_pa(struct amdgv_adapter *adapt,
						uint32_t ch_idx,
						uint64_t na, uint64_t *soc_pa)
{
	uint32_t total_ch_num = adapt->umc.max_ras_err_cnt_per_query;
	uint32_t col_bit = umc_v8_10_get_col_bit(total_ch_num);
	uint64_t tmp_addr;

	if (col_bit == U32_MAX)
		return -1;

	tmp_addr = SWIZZLE_MODE_TMP_ADDR(na, total_ch_num, ch_idx);
	*soc_pa = SWIZZLE_MODE_ADDR_HI(tmp_addr, col_bit) |
		SWIZZLE_MODE_ADDR_MID(na, col_bit) |
		SWIZZLE_MODE_ADDR_LOW(tmp_addr, col_bit) |
		SWIZZLE_MODE_ADDR_LSB(na);

	return 0;
}

static int umc_v8_10_convert_error_address(struct amdgv_adapter *adapt,
						uint32_t logical_channel_index, uint64_t err_addr,
						uint64_t *retired_page, uint64_t mc_umc_status)
{
	uint32_t addr_lsb;
	int ret = 0;

	addr_lsb = REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, AddrLsb);
	err_addr &= ~((0x1ULL << addr_lsb) - 1);

	ret = umc_v8_10_swizzle_mode_na_to_pa(adapt, logical_channel_index,
					err_addr, retired_page);

	if (ret)
		return ret;

	return 0;
}

static void umc_v8_10_query_error_address(struct amdgv_adapter *adapt,
						void *ras_error_status, uint32_t node_inst,
						uint32_t ch_inst, uint32_t umc_inst)
{
	struct ras_err_data *err_data = (struct ras_err_data *)ras_error_status;
	uint64_t mc_umc_status, err_addr;
	uint64_t retired_page;
	uint32_t eccinfo_table_idx;
	uint32_t logical_channel_index;
	struct eeprom_table_record *err_rec;
	int ret = 0;

	if (!err_data->err_addr)
		return;

	eccinfo_table_idx = node_inst * adapt->umc.umc_inst_num *
				adapt->umc.channel_inst_num +
				umc_inst * adapt->umc.channel_inst_num +
				ch_inst;

	logical_channel_index = adapt->umc.channel_idx_tbl[eccinfo_table_idx];
	mc_umc_status = adapt->ecc.umc_ecc.ecc[eccinfo_table_idx].mca_umc_status;

	/* calculate error address if ue error is detected */
	if (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Val) == 1 &&
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, AddrV) == 1 &&
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UECC) == 1) {

		err_addr = adapt->ecc.umc_ecc.ecc[eccinfo_table_idx].mca_umc_addr;
		err_addr = REG_GET_FIELD(err_addr, MCA_UMC_UMC0_MCUMC_ADDRT0, ErrorAddr);

		/* PF only map the address read from MCA channel */
		ret = umc_v8_10_convert_error_address(adapt, logical_channel_index,
						err_addr, &retired_page, mc_umc_status);
		if (ret) {
			AMDGV_ERROR("Failed to map pa from umc na.\n");
			/* decrease uncorrectable error count
			 * due to map normal error address failed
			 */
			err_data->ue_count--;
			return;
		}
		err_rec = &err_data->err_addr[err_data->err_addr_cnt];
		err_rec->address = err_addr;
		/* page frame address is saved */
		err_rec->retired_page = retired_page >> AMDGV_GPU_PAGE_SHIFT;
		err_rec->cu = 0;
		err_rec->mem_channel = eccinfo_table_idx;
		err_rec->mcumc_id = umc_inst;
		err_rec->ts = amdgv_ras_eeprom_utc_to_eeprom_format(adapt, oss_get_utc_time_stamp());
		err_data->err_addr_cnt++;
	}
}

static void umc_v8_10_hw_query_error_address(struct amdgv_adapter *adapt,
						void *ras_error_status, uint32_t node_inst,
						uint32_t ch_inst, uint32_t umc_inst)
{
	struct ras_err_data *err_data = (struct ras_err_data *)ras_error_status;
	uint64_t mc_umc_status_addr;
	uint64_t mc_umc_status, err_addr;
	uint64_t mc_umc_addrt0;
	uint64_t retired_page;
	uint32_t eccinfo_table_idx, logical_channel_index;
	struct eeprom_table_record *err_rec;
	int ret = 0;

	uint32_t umc_reg_offset =
		get_umc_v8_10_reg_offset(adapt, node_inst, umc_inst, ch_inst);

	mc_umc_status_addr =
		SOC15_REG_OFFSET(UMC, 0, regMCA_UMC_UMC0_MCUMC_STATUST0);
	mc_umc_status = RREG64_PCIE((mc_umc_status_addr + umc_reg_offset));

	eccinfo_table_idx = node_inst * adapt->umc.umc_inst_num *
				adapt->umc.channel_inst_num +
				umc_inst * adapt->umc.channel_inst_num +
				ch_inst;

	logical_channel_index = adapt->umc.channel_idx_tbl[eccinfo_table_idx];

	if (!err_data->err_addr) {
		WREG64_PCIE((mc_umc_status_addr + umc_reg_offset), 0x0ULL);
		return;
	}

	/* calculate error address if ue error is detected */
	if (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Val) == 1 &&
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, AddrV) == 1 &&
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UECC) == 1) {

		mc_umc_addrt0 = SOC15_REG_OFFSET(UMC, 0, regMCA_UMC_UMC0_MCUMC_ADDRT0);
		err_addr = RREG64_PCIE((mc_umc_addrt0 + umc_reg_offset));
		err_addr = REG_GET_FIELD(err_addr, MCA_UMC_UMC0_MCUMC_ADDRT0, ErrorAddr);

		/* PF only map the address read from MCA channel */
		ret = umc_v8_10_convert_error_address(adapt, logical_channel_index,
						err_addr, &retired_page, mc_umc_status);
		if (ret) {
			err_data->ue_count--;
			return;
		}
		err_rec = &err_data->err_addr[err_data->err_addr_cnt];
		err_rec->address = err_addr;
		/* page frame address is saved */
		err_rec->retired_page = retired_page >> AMDGV_GPU_PAGE_SHIFT;
		err_rec->cu = 0;
		err_rec->mem_channel = eccinfo_table_idx;
		err_rec->mcumc_id = umc_inst;
		err_rec->ts = amdgv_ras_eeprom_utc_to_eeprom_format(adapt, oss_get_utc_time_stamp());
		err_data->err_addr_cnt++;

		WREG64_PCIE((mc_umc_status_addr + umc_reg_offset), 0x0ULL);
	}
}


static void umc_v8_10_hw_query_ras_error_address (struct amdgv_adapter *adapt,
						void *ras_error_status)
{
	umc_v8_10_loop_umc_channels(adapt,
		umc_v8_10_hw_query_error_address, ras_error_status);
}

static void umc_v8_10_query_ras_error_address(struct amdgv_adapter *adapt,
						void *ras_error_status)
{
	umc_v8_10_loop_umc_channels(adapt,
		umc_v8_10_query_error_address, ras_error_status);
}

static void umc_v8_10_err_cnt_init_per_channel(struct amdgv_adapter *adapt,
						void *ras_error_status, uint32_t node_inst,
						uint32_t ch_inst, uint32_t umc_inst)
{
	uint32_t ecc_err_cnt_sel, ecc_err_cnt_sel_addr;
	uint32_t ecc_err_cnt_addr, umc_reg_offset;

	umc_reg_offset = get_umc_v8_10_reg_offset(adapt, node_inst, umc_inst, ch_inst);

	ecc_err_cnt_sel_addr =
		SOC15_REG_OFFSET(UMC, 0, regUMCCH0_0_GeccErrCntSel);
	ecc_err_cnt_addr =
		SOC15_REG_OFFSET(UMC, 0, regUMCCH0_0_GeccErrCnt);

	ecc_err_cnt_sel = RREG32_PCIE(ecc_err_cnt_sel_addr + umc_reg_offset);

	/* set ce error interrupt type to APIC based interrupt */
	ecc_err_cnt_sel = REG_SET_FIELD(ecc_err_cnt_sel, UMCCH0_0_GeccErrCntSel,
					GeccErrInt, 0x1);
	WREG32_PCIE(ecc_err_cnt_sel_addr + umc_reg_offset, ecc_err_cnt_sel);
	/* set error count to initial value */
	WREG32_PCIE(ecc_err_cnt_addr + umc_reg_offset, UMC_V8_10_CE_CNT_INIT);
}

static void umc_v8_10_err_cnt_init(struct amdgv_adapter *adapt)
{
		umc_v8_10_loop_umc_channels(adapt,
			umc_v8_10_err_cnt_init_per_channel, NULL);
}

static bool umc_v8_10_query_memchandis(struct amdgv_adapter *adapt,
					   uint32_t umc_reg_offset)
{
	uint32_t umccap;
	uint32_t umccap_addr;

	/* UMC Cap register */
	umccap_addr =
		SOC15_REG_OFFSET(UMC, 0, regUMCCH0_0_UmcCap);

	umccap = RREG64_PCIE(umccap_addr + umc_reg_offset);

	return REG_GET_FIELD(umccap, UMCCH0_0_UmcCap, MemChanDis);
}

static void umc_v8_10_query_ras_memchandis(struct amdgv_adapter *adapt)
{
	uint32_t node_inst       = 0;
	uint32_t umc_inst        = 0;
	uint32_t ch_inst         = 0;
	uint32_t umc_reg_offset  = 0;
	uint32_t eccinfo_table_idx   = 0;
	uint64_t disabled_channel_config = 0;
	uint32_t total_channels  = 0;

	/* reset umc channel disbaled info before query */
	adapt->umc.channel_mask = 0;
	adapt->umc.channel_dis_num = 0;

	LOOP_UMC_NODE_INST_AND_CH(node_inst, umc_inst, ch_inst) {
		eccinfo_table_idx = node_inst * adapt->umc.umc_inst_num *
			adapt->umc.channel_inst_num +
			umc_inst * adapt->umc.channel_inst_num +
			ch_inst;

			/* skip harvest umc node */
		if (!((uint32_t) (1 << node_inst) & adapt->umc.active_mask)) {
			disabled_channel_config |= 1ULL << eccinfo_table_idx;
			continue;
		}

		umc_reg_offset = get_umc_v8_10_reg_offset(adapt,
							node_inst,
							umc_inst,
							ch_inst);

		if (umc_v8_10_query_memchandis(adapt, umc_reg_offset)) {
			adapt->umc.channel_dis_num++;
			disabled_channel_config |= 1ULL << eccinfo_table_idx;
		}
	}

	/**
	 * calculate the total channel instance include
	 * harvest umc node and disabled channels
	 */
	total_channels = adapt->umc.node_inst_num *
					UMC_V8_10_UMC_INSTANCE_NUM *
					UMC_V8_10_CHANNEL_INSTANCE_NUM;
	adapt->umc.channel_mask = (((uint64_t)1 << total_channels) - 1) &
					~disabled_channel_config;
}

static void umc_v8_10_get_eeprom_i2c_params(struct amdgv_adapter *adapt,
					struct amdgv_ras_eeprom_control *control)
{
	control->i2c_address = EEPROM_I2C_TARGET_ADDR_NAVI3;
	control->i2c_port = EEPROM_I2C_CONTROLLER_PORT_NAVI3;
}

static bool umc_v8_10_query_ras_poison_mode(struct amdgv_adapter *adapt)
{
	/*
	 * Force return true, because UMCCH0_0_GeccCtrl
	 * is not accessible from host side
	 */
	return true;
}

static void umc_v8_10_set_eeprom_table_version(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_eeprom_control *control = &adapt->eeprom_control;

	control->tbl_hdr.version = EEPROM_TABLE_VERSION_NV3;
}

/*
 * Definition of safe region:
 * 1. It must be inside valid VF's GPA FB
 * 2. It must be outside the VBIOS and data exchange region
 * 3. It must be outside the VF's TMR region
 */
static int umc_v8_10_get_ras_vf_safe_range(struct amdgv_adapter *adapt,
	uint64_t *offset, uint64_t *size, uint32_t max_entry_num)
{
	uint32_t entry_idx = 0;
	uint32_t vf_idx;

	struct amdgv_ffbm_pte_block *pteb;
	struct amdgv_vf_device *entry;

	uint64_t fb_offset = 0;
	uint64_t fb_size   = 0;

	uint64_t reserved_offset = KBYTES_TO_BYTES(AMD_SRIOV_MSG_DATAEXCHANGE_OFFSET_KB_V1 + AMD_SRIOV_MSG_DATAEXCHANGE_SIZE_KB_V1);
	uint64_t tmr_offset      = KBYTES_TO_BYTES(AMD_SRIOV_MSG_DATAEXCHANGE_OFFSET_KB_V1);
	uint64_t tmr_size        = MBYTES_TO_BYTES(adapt->tmr_size);

	if (adapt->ffbm.enabled) {
		FFBM_LOCK_LIST;
		amdgv_list_for_each_entry(pteb, &adapt->ffbm.spa_list, struct amdgv_ffbm_pte_block, spa_list_node) {
			if (pteb->type == AMDGV_FFBM_MEM_TYPE_VF) {
				if (pteb->gpa == 0) {
					fb_offset = pteb->spa  + reserved_offset;
					fb_size   = pteb->size - reserved_offset;
				} else {
					fb_offset = pteb->spa;
					fb_size   = pteb->size;
				}

				if (entry_idx < max_entry_num) {
					offset[entry_idx] = fb_offset;
					size[entry_idx]   = fb_size;
				}
				entry_idx++;
			}
		}
		FFBM_UNLOCK_LIST;
	} else {
		for (vf_idx = 0; vf_idx < adapt->num_vf; vf_idx++) {
			entry = &adapt->array_vf[vf_idx];

			fb_offset = 0;
			fb_size   = 0;

			if (entry->configured && entry->res_mapped) {
				if (tmr_size) {
					fb_offset = entry->fb_offset_tmr;
					fb_size   = entry->fb_size_tmr;
				} else {
					fb_offset = entry->fb_offset;
					fb_size   = entry->fb_size;
				}
			}

			if (fb_size) {
				if (tmr_size) {
					/* if TMR exists, the safe region is discontinuous */
					if (entry_idx < max_entry_num) {
						offset[entry_idx] = fb_offset  + reserved_offset;
						size[entry_idx]   = tmr_offset - reserved_offset;
					}
					entry_idx++;

					if (entry_idx < max_entry_num) {
						offset[entry_idx] = fb_offset + tmr_offset + tmr_size;
						size[entry_idx]   = fb_size   - tmr_offset - tmr_size;
					}
					entry_idx++;
				} else {
					if (entry_idx < max_entry_num) {
						offset[entry_idx] = fb_offset + reserved_offset;
						size[entry_idx]   = fb_size   - reserved_offset;
					}
					entry_idx++;
				}
			}
		}
	}

	return entry_idx;
}


const struct amdgv_umc_funcs umc_v8_10_funcs = {
	.err_cnt_init = umc_v8_10_err_cnt_init,
	.query_ras_error_count = umc_v8_10_query_ras_error_count,
	.query_ras_error_address = umc_v8_10_query_ras_error_address,
	.hw_query_ras_error_address = umc_v8_10_hw_query_ras_error_address,
	.hw_query_ras_error_count = umc_v8_10_hw_query_ras_error_count,
	.get_eeprom_i2c_params = umc_v8_10_get_eeprom_i2c_params,
	.set_eeprom_table_version = umc_v8_10_set_eeprom_table_version,
	.query_ras_memchandis = umc_v8_10_query_ras_memchandis,
	.query_ras_poison_mode = umc_v8_10_query_ras_poison_mode,
	.get_ras_vf_safe_range = umc_v8_10_get_ras_vf_safe_range,
};

void umc_v8_10_set_umc_funcs(struct amdgv_adapter *adapt)
{
	adapt->umc.channel_inst_num = UMC_V8_10_CHANNEL_INSTANCE_NUM;
	adapt->umc.umc_inst_num = UMC_V8_10_UMC_INSTANCE_NUM;
	adapt->umc.inst_offs = UMC_V8_10_PER_INST_OFFSET;
	adapt->umc.channel_offs = UMC_V8_10_PER_CHANNEL_OFFSET;
	adapt->umc.node_offs = UMC_V8_10_PER_NODE_OFFSET;
	adapt->umc.channel_idx_tbl =
		&umc_v8_10_channel_idx_tbl_nv32[0][0][0];
	adapt->umc.funcs = &umc_v8_10_funcs;
	adapt->umc.supports_ras_eeprom = true;
	if (adapt->opt.use_legacy_eeprom_format)
		adapt->umc.use_legacy_eeprom_format = true;
	adapt->umc.reset_mode = AMDGV_RESET_MODE1;
}

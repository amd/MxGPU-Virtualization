/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "umc_v6_7.h"
#include "amdgv_ras.h"
#include "mi200.h"
#include "amdgv.h"
#include "amdgv_device.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

#define EEPROM_I2C_TARGET_ADDR_MI200    0xA0
#define EEPROM_I2C_CONTROLLER_PORT_MI200 0

/*
 * (addr / 256) * 8192, the higher 26 bits in ErrorAddr
 * is the index of 8KB block
 */
#define ADDR_OF_8KB_BLOCK(addr)		(((addr) & ~0xffULL) << 5)
/* channel index is the index of 256B block */
#define ADDR_OF_256B_BLOCK(channel_index)	((channel_index) << 8)
/* offset in 256B block */
#define OFFSET_IN_256B_BLOCK(addr)		((addr) & 0xffULL)

const uint32_t
	umc_v6_7_channel_idx_tbl_second[UMC_V6_7_UMC_INSTANCE_NUM][UMC_V6_7_CHANNEL_INSTANCE_NUM] = {
		{28, 20, 24, 16, 12, 4, 8, 0},
		{6, 30, 2, 26, 22, 14, 18, 10},
		{19, 11, 15, 7, 3, 27, 31, 23},
		{9, 1, 5, 29, 25, 17, 21, 13},
};
const uint32_t
	umc_v6_7_channel_idx_tbl_first[UMC_V6_7_UMC_INSTANCE_NUM][UMC_V6_7_CHANNEL_INSTANCE_NUM] = {
		{19, 11, 15, 7, 3, 27, 31, 23},
		{9, 1, 5, 29, 25, 17, 21, 13},
		{28, 20, 24, 16, 12, 4, 8, 0},
		{6, 30, 2, 26, 22, 14, 18, 10},
};

static void umc_v6_7_query_correctable_error_count(
	struct amdgv_adapter *adapt,
	uint32_t umc_inst, uint32_t ch_inst,
	unsigned long *error_count)
{

	uint32_t ecc_err_cnt;
	uint64_t mc_umc_status;
	uint32_t eccinfo_table_idx;

	eccinfo_table_idx = umc_inst * adapt->umc.channel_inst_num + ch_inst;
	ecc_err_cnt = adapt->ecc.umc_ecc.ecc[eccinfo_table_idx].ce_count_lo_chip;

	/* increase error count */
	if (ecc_err_cnt)
		*error_count += ecc_err_cnt;

	ecc_err_cnt = adapt->ecc.umc_ecc.ecc[eccinfo_table_idx].ce_count_hi_chip;
	/* increase error count  */
	if (ecc_err_cnt)
		*error_count += ecc_err_cnt;

	/* check for SRAM correctable error MCUMC_STATUS is a 64 bit
	 * register
	 */
	mc_umc_status = adapt->ecc.umc_ecc.ecc[eccinfo_table_idx].mca_umc_status;

	/* we only count prefetch errors */
	if (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, ErrorCodeExt) == 6 &&
		REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Val) == 1 &&
	    REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, CECC) == 1) {
		*error_count += 1;
	}
}

static void umc_v6_7_querry_uncorrectable_error_count(
	struct amdgv_adapter *adapt,
	uint32_t umc_inst, uint32_t ch_inst,
	unsigned long *error_count)
{
	uint64_t mc_umc_status;
	uint32_t eccinfo_table_idx;

	eccinfo_table_idx = umc_inst * adapt->umc.channel_inst_num + ch_inst;

	/* check the MCUMC_STATUS */
	mc_umc_status = adapt->ecc.umc_ecc.ecc[eccinfo_table_idx].mca_umc_status;
	if ((REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Val) == 1) &&
	    (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Deferred) == 1 ||
	     REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UECC) == 1 ||
	     REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, PCC) == 1 ||
	     REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UC) == 1 ||
	     REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, TCC) == 1)) {
		*error_count += 1;
	}
}

static void umc_v6_7_query_ras_error_count(struct amdgv_adapter *adapt,
	void *ras_error_status)
{
	struct ras_err_data *err_data = (struct ras_err_data *)ras_error_status;
	uint32_t umc_inst        = 0;
	uint32_t ch_inst         = 0;

	LOOP_UMC_INST_AND_CH(umc_inst, ch_inst) {
		umc_v6_7_query_correctable_error_count(adapt,
							umc_inst, ch_inst,
							&(err_data->ce_count));
		umc_v6_7_querry_uncorrectable_error_count(adapt,
							umc_inst, ch_inst,
							&(err_data->ue_count));
	}
}


static void umc_v6_7_query_error_address(
	struct amdgv_adapter *adapt,
	struct ras_err_data *err_data,
	uint32_t umc_inst, uint32_t ch_inst)
{

	uint64_t err_addr, retired_page;
	struct eeprom_table_record *err_rec;
	uint64_t mc_umc_status;
	uint32_t eccinfo_table_idx;
	uint32_t channel_index;

	/* skip error address process if -ENOMEM */
	if (!err_data->err_addr)
		return;

	eccinfo_table_idx = umc_inst * adapt->umc.channel_inst_num + ch_inst;
	channel_index = adapt->umc.channel_idx_tbl[eccinfo_table_idx];

	err_rec = &err_data->err_addr[err_data->err_addr_cnt];
	mc_umc_status = adapt->ecc.umc_ecc.ecc[eccinfo_table_idx].mca_umc_status;

	/* calculate error address if ue/ce error is detected */
	if (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, Val) == 1 &&
	    (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UECC) == 1 ||
	     REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, CECC) == 1)) {

		err_addr = adapt->ecc.umc_ecc.ecc[eccinfo_table_idx].mca_umc_addr;

		err_addr = REG_GET_FIELD(err_addr, MCA_UMC_UMC0_MCUMC_ADDRT0, ErrorAddr);

		/* translate umc channel address to soc pa */
		retired_page = ADDR_OF_8KB_BLOCK(err_addr) |
			       ADDR_OF_256B_BLOCK(channel_index) |
			       OFFSET_IN_256B_BLOCK(err_addr);

		/* The umc channel bits are not original values,
		 * they are hashed
		 */
		SET_CHANNEL_HASH(channel_index, retired_page);

		/* we only save ue error information currently, ce is skipped */
		if (REG_GET_FIELD(mc_umc_status, MCA_UMC_UMC0_MCUMC_STATUST0, UECC) == 1) {
			err_rec->address = err_addr;
			/* page frame address is saved */
			err_rec->retired_page = retired_page >> AMDGV_GPU_PAGE_SHIFT;
			err_rec->ts = oss_get_utc_time_stamp();
			err_rec->err_type = AMDGV_RAS_EEPROM_ERR_NON_RECOVERABLE;
			err_rec->cu = 0;
			err_rec->mem_channel = channel_index;
			err_rec->mcumc_id = umc_inst;

			err_data->err_addr_cnt++;
		}
	}
}

static void umc_v6_7_query_ras_error_address(struct amdgv_adapter *adapt,
	void *ras_error_status)
{
	struct ras_err_data *err_data = (struct ras_err_data *)ras_error_status;
	uint32_t umc_inst        = 0;
	uint32_t ch_inst         = 0;

	LOOP_UMC_INST_AND_CH(umc_inst, ch_inst) {
		umc_v6_7_query_error_address(adapt,
				err_data,
				umc_inst,
				ch_inst);
	}
}

static void umc_v6_7_get_eeprom_i2c_params(struct amdgv_adapter *adapt,
					struct amdgv_ras_eeprom_control *control)
{
	uint8_t i2c_addr = EEPROM_I2C_TARGET_ADDR_MI200;

	amdgv_atomfirmware_ras_rom_addr(adapt, &i2c_addr);
	control->i2c_address = i2c_addr;

	control->i2c_port = EEPROM_I2C_CONTROLLER_PORT_MI200;
}

static bool umc_v6_7_query_ras_poison_mode(struct amdgv_adapter *adapt)
{
	uint32_t ecc_ctrl_addr, ecc_ctrl;

	/* Enabling fatal error in umc instance0 channel0 will be
	 * considered as fatal error mode
	 */
	ecc_ctrl_addr =
		SOC15_REG_OFFSET(UMC, 0, regUMCCH0_0_EccCtrl);
	ecc_ctrl = RREG32_PCIE(ecc_ctrl_addr);

	return !REG_GET_FIELD(ecc_ctrl, UMCCH0_0_EccCtrl, UCFatalEn);
}

const struct amdgv_umc_funcs umc_v6_7_funcs = {
	.err_cnt_init = NULL,
	.query_ras_error_count = umc_v6_7_query_ras_error_count,
	.query_ras_error_address = umc_v6_7_query_ras_error_address,
	.query_ras_poison_mode = umc_v6_7_query_ras_poison_mode,
	.get_eeprom_i2c_params = umc_v6_7_get_eeprom_i2c_params,
};

void umc_v6_7_set_umc_funcs(struct amdgv_adapter *adapt)
{
	adapt->umc.max_ras_err_cnt_per_query =
			UMC_V6_7_TOTAL_CHANNEL_NUM;
	adapt->umc.channel_inst_num = UMC_V6_7_CHANNEL_INSTANCE_NUM;
	adapt->umc.umc_inst_num = UMC_V6_7_UMC_INSTANCE_NUM;
	adapt->umc.inst_offs = UMC_V6_7_PER_INST_OFFSET;
	adapt->umc.channel_offs = UMC_V6_7_PER_CHANNEL_OFFSET;


	if (adapt->smuio.funcs) {
		if (1 & adapt->smuio.funcs->get_die_id(adapt))
			adapt->umc.channel_idx_tbl =
				&umc_v6_7_channel_idx_tbl_first[0][0];
		else
			adapt->umc.channel_idx_tbl =
				&umc_v6_7_channel_idx_tbl_second[0][0];
	}

	adapt->umc.funcs = &umc_v6_7_funcs;
	adapt->umc.supports_ras_eeprom = true;
	adapt->umc.use_legacy_eeprom_format = true;
	adapt->umc.reset_mode = AMDGV_RESET_MODE1;
}

/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "mi200_ecc.h"
#include "umc_v6_7.h"
#include "gfx_v9_4_2.h"
#include "sdma_v4_4.h"
#include "nbio_v7_4.h"
#include "vcn_v2_6.h"
#include "amdgv_ras.h"
#include "amdgv_psp_gfx_if.h"
#include "../../AI/ucode/mi200/psp_ras_bin.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static int mi200_umc_update_uc_error_count(struct amdgv_adapter *adapt,
				uint32_t idx_vf)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_ecc_info)
		ret = adapt->pp.pp_funcs->get_ecc_info(adapt, &(adapt->ecc.umc_ecc));

	if (ret)
		return 0;

	return amdgv_umc_update_uc_error_count(adapt, idx_vf);
}

static int mi200_umc_update_error_count(struct amdgv_adapter *adapt,
				uint32_t idx_vf)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_ecc_info)
		ret = adapt->pp.pp_funcs->get_ecc_info(adapt, &(adapt->ecc.umc_ecc));

	if (ret)
		return 0;

	return amdgv_umc_update_error_count(adapt, idx_vf);
}

static int mi200_ecc_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	adapt->ecc.correctable_error_num = 0;
	adapt->ecc.uncorrectable_error_num = 0;
	adapt->ecc.toggle_ecc_mode = NULL;
	adapt->ecc.supported = 0;
	adapt->ecc.bad_page_detection_mode = 0;

	switch (adapt->opt.bad_page_detection_mode) {
	case AMDGV_BAD_PAGE_DETECTION_MODE1:
		adapt->ecc.bad_page_detection_mode |= (1 << AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS) |
											(1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA);
		break;
	case AMDGV_BAD_PAGE_DETECTION_MODE2:
		adapt->ecc.bad_page_detection_mode |= 1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA;
		break;
	default:
		adapt->ecc.bad_page_detection_mode = 0;
	}
	oss_atomic_set(adapt->in_ecc_recovery, 0);

	adapt->ecc.get_correctable_error_count = mi200_umc_update_error_count;
	adapt->ecc.get_uncorrectable_error_count = mi200_umc_update_uc_error_count;
	adapt->ecc.get_error_count = amdgv_ecc_get_error_count;

	/* init umc structure for specific ASIC */
	umc_v6_7_set_umc_funcs(adapt);
	gfx_v9_4_2_set_funcs(adapt);
	sdma_v4_4_set_ras_funcs(adapt);

	nbio_v7_4_set_ras_funcs(adapt);
	vcn_v2_6_set_ras_funcs(adapt);

	/* save umc error address in one query */
	adapt->umc.err_addr =
		oss_malloc(sizeof(struct eeprom_table_record) *
			adapt->umc.max_ras_err_cnt_per_query);
	if (!adapt->umc.err_addr)
		AMDGV_WARN("Failed to allocate ecc error address array!\n");

	if (amdgv_umc_ras_lock_init(adapt)) {
		ret = AMDGV_FAILURE;
		goto out;
	}

	if (amdgv_umc_sw_init(adapt)) {
		ret = AMDGV_FAILURE;
		goto out;
	}

	return ret;

out:
	amdgv_umc_ras_lock_fini(adapt);

	return ret;
}

static int mi200_ecc_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->log_level >= AMDGV_DEBUG_LEVEL) {
		AMDGV_DEBUG("UMC RAS (TOTAL ERROR): cce cnt %d, uce cnt %d\n",
			    adapt->ecc.correctable_error_num,
			    adapt->ecc.uncorrectable_error_num);
	}

	adapt->ecc.enabled = 0;
	adapt->ecc.correctable_error_num = 0;
	adapt->ecc.uncorrectable_error_num = 0;
	adapt->ecc.toggle_ecc_mode = NULL;
	adapt->ecc.get_correctable_error_count = NULL;
	adapt->ecc.get_uncorrectable_error_count = NULL;


	if (adapt->umc.err_addr)
		oss_free(adapt->umc.err_addr);

	amdgv_umc_ras_lock_fini(adapt);

	amdgv_umc_sw_fini(adapt);

	return 0;
}

static int mi200_ecc_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	if (!(adapt->ecc.supported & (1 << AMDGV_RAS_MEM_ECC_SUPPORT)) &&
			!(adapt->ecc.supported & (1 << AMDGV_RAS_SRAM_ECC_SUPPORT))) {
		AMDGV_WARN("RAS ECC is not supported\n");
		goto out;
	}

	/* RAS TA loading depends on the setting of function pointer of df/umc
	 * poison mode query, so we move it from psp hw init to here
	 */
	ret = amdgv_psp_ras_initialize(adapt, psp_ras_bin, sizeof(psp_ras_bin));
	if (ret != PSP_STATUS__SUCCESS) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_FW_INIT_FAIL, 0);
		ret = AMDGV_FAILURE;
		goto out;
	}

	if (adapt->ecc.supported & (1 << AMDGV_RAS_SRAM_ECC_SUPPORT)) {
		if (adapt->gfx.funcs &&
			adapt->gfx.funcs->err_cnt_init)
			adapt->gfx.funcs->err_cnt_init(adapt);

		if (adapt->sdma.funcs &&
			adapt->sdma.funcs->err_cnt_init)
			adapt->sdma.funcs->err_cnt_init(adapt);

		if (adapt->nbio.ras) {
			if (adapt->nbio.ras->set_ras_controller_irq_state)
				adapt->nbio.ras->set_ras_controller_irq_state(adapt, true);
			if (adapt->nbio.ras->set_ras_err_event_athub_irq_state)
				adapt->nbio.ras->set_ras_err_event_athub_irq_state(adapt, true);
		}
	}

	if (amdgv_ecc_is_support(adapt, AMDGV_RAS_BLOCK__UMC)) {
		if (adapt->umc.funcs &&
			adapt->umc.funcs->err_cnt_init)
			adapt->umc.funcs->err_cnt_init(adapt);

		/* init ras eeprom and load umc retired pages */
		ret = amdgv_umc_hw_init(adapt);
		if (ret) {
			AMDGV_ERROR("Failed to Init RAS ECC\n");
			return ret;
		}
	}

out:
	return ret;
}

static int mi200_ecc_hw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->ecc.supported & (1 << AMDGV_RAS_MEM_ECC_SUPPORT)) {
		adapt->ecc.supported &= ~(1 << AMDGV_RAS_MEM_ECC_SUPPORT);
		adapt->ecc.enabled &= ~(1 << AMDGV_RAS_BLOCK__UMC);
		amdgv_umc_hw_fini(adapt);
	}

	if (adapt->ecc.supported & (1 << AMDGV_RAS_SRAM_ECC_SUPPORT)) {
		adapt->ecc.supported &= ~(1 << AMDGV_RAS_SRAM_ECC_SUPPORT);
		adapt->ecc.enabled &= ~(1 << AMDGV_RAS_BLOCK__SDMA |
								1 << AMDGV_RAS_BLOCK__GFX);
	}

	return 0;
}

struct amdgv_init_func mi200_ecc_func = {
	.name = "mi200_ecc_func",
	.sw_init = mi200_ecc_sw_init,
	.sw_fini = mi200_ecc_sw_fini,
	.hw_init = mi200_ecc_hw_init,
	.hw_fini = mi200_ecc_hw_fini,
};

/*
 * Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv.h"
#include "amdgv_device.h"
#include "mi300.h"
#include "mi300_ecc.h"
#include "mi300_nbio.h"
#include "mi300_xgmi.h"
#include "umc_v12_0.h"
#include "gfx_v9_4_3.h"
#include "mmhub_v1_8.h"
#include "sdma_v4_4_2.h"
#include "mi300_mca.h"
#include "amdgv_ras.h"
#include "amdgv_psp_gfx_if.h"
#include "amdgv_mca.h"
#include "mi300_psp.h"
#include "amdgv_cper.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static int mi300_umc_update_uc_error_count(struct amdgv_adapter *adapt,
				uint32_t idx_vf)
{
	int ret = 0;

	ret = amdgv_mca_get_new_banks(adapt, AMDGV_MCA_ERROR_TYPE_UE);

	if (ret)
		return 0;

	return amdgv_umc_update_uc_error_count(adapt, idx_vf);
}

static int mi300_umc_update_error_count(struct amdgv_adapter *adapt,
				uint32_t idx_vf)
{
	int ret = 0;

	ret = amdgv_mca_get_new_banks(adapt, AMDGV_MCA_ERROR_TYPE_CE);

	if (ret)
		return 0;

	return amdgv_umc_update_error_count(adapt, idx_vf);
}

static int mi300_get_error_count(struct amdgv_adapter *adapt,
				 struct amdgv_smi_ras_query_if *info)
{
	int ret = 0;
	uint32_t ce_count = 0, de_count = 0, ue_count = 0;
	uint32_t ce_overflow = 0, de_overflow = 0, ue_overflow = 0;

	ret = amdgv_mca_get_new_banks(adapt, AMDGV_MCA_ERROR_TYPE_CE);
	if (ret)
		return ret;

	ret = amdgv_mca_get_new_banks(adapt, AMDGV_MCA_ERROR_TYPE_UE);
	if (ret)
		return ret;

	ret = amdgv_mca_count_cache_client_get(adapt, &ce_count, &ue_count,
					       &de_count, &ce_overflow, &ue_overflow,
					       &de_overflow, info->head.block,
					       AMDGV_PF_IDX);
	if (ret)
		return ret;

	info->ce_count = ce_count;
	info->ue_count = ue_count;
	info->de_count = de_count;

	return 0;
}

uint32_t mi300_get_ras_cap(struct amdgv_adapter *adapt)
{
	uint32_t ras_cap = 0, drv_supported_blocks = 0;

	/* Update line below whenever a new RAS block support is added in driver */
	drv_supported_blocks |= BIT(AMDGV_RAS_BLOCK__UMC) |
				BIT(AMDGV_RAS_BLOCK__GFX) |
				BIT(AMDGV_RAS_BLOCK__SDMA) |
				BIT(AMDGV_RAS_BLOCK__MMHUB) |
				BIT(AMDGV_RAS_BLOCK__XGMI_WAFL) |
				BIT(AMDGV_RAS_BLOCK__PCIE_BIF);

	/* Init caps with driver supported blocks */
	ras_cap = adapt->ecc.ras_cap = drv_supported_blocks;

	/* Init supported with poison propogation enabled */
	adapt->ecc.supported |= BIT(AMDGV_RAS_POISON_ECC_SUPPORT);

	adapt->ecc.ras_cap = RREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_127));
	AMDGV_INFO("ras_cap reported by PSP BL = 0x%x\n", adapt->ecc.ras_cap);

	if (adapt->ecc.ras_cap) {
		/* Update ras_cap based on RAS blocks supported by driver. */
		ras_cap = adapt->ecc.ras_cap & drv_supported_blocks;

		/* Update supported */
		if (!(adapt->ecc.ras_cap & AMDGV_RAS_POISON_PROPOGATION_MODE_BIT)) {
			adapt->ecc.supported &= ~BIT(AMDGV_RAS_POISON_ECC_SUPPORT);
		}
	} else {
		AMDGV_INFO("Unexpected ras_cap reported by PSP BL.\n");
	}

	AMDGV_INFO("ras_cap updated with driver support = 0x%x\n", ras_cap);
	AMDGV_INFO("Poison Propogation Mode : %s\n",
		((adapt->ecc.ras_cap & AMDGV_RAS_POISON_PROPOGATION_MODE_BIT) ? "Enabled":"Disabled"));

	return ras_cap;
}

/* Fixed pattern for smn addressing on different AIDs:
 *   bit[34]: indicate cross AID access
 *   bit[33:32]: indicate target AID id
 * AID id range is 0 ~ 3 as maximum AID number is 4.
 */
static uint64_t mi300_aqua_vanjaram_encode_ext_smn_addressing(int ext_id)
{
	uint64_t ext_offset;

	/* local routing and bit[34:32] will be zeros */
	if (ext_id == 0)
		return 0;

	/* Initiated from host, accessing to all non-zero aids are cross traffic */
	ext_offset = ((uint64_t)(ext_id & 0x3) << 32) | (1ULL << 34);

	return ext_offset;
}

static uint32_t mi300_ras_read_register_by_instance(struct amdgv_adapter *adapt, uint32_t base_addr,
						 uint32_t instance)
{
	uint64_t reg_addr = (base_addr << 2) +
		   mi300_aqua_vanjaram_encode_ext_smn_addressing(instance);

	return RREG32_PCIE_EXT((reg_addr >> 2));
}

#define mmMP0_SMN_C2PMSG_92	0x1609C
#define mmMP0_SMN_C2PMSG_126	0x160BE
#define mmSMUIO_MCM_CONFIG	0x5A08C

static void mi300_ras_boot_time_error_reporting(struct amdgv_adapter *adapt,
						 uint32_t instance)
{
	uint32_t socket_id, aid_id, hbm_id;
	uint32_t fw_status;
	uint32_t boot_error;

	/* The pattern for smn addressing in other SOC could be different from
	 * the one for aqua_vanjaram. We should revisit the code if the pattern
	 * is changed. In such case, replace the aqua_vanjaram implementation
	 * with more common helper */
	fw_status = mi300_ras_read_register_by_instance(adapt, mmMP0_SMN_C2PMSG_92, instance);
	boot_error = mi300_ras_read_register_by_instance(adapt, mmMP0_SMN_C2PMSG_126, instance);

	if (boot_error == AMDGV_RAS_BOOT_READ_ERR_VAL) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_BOOT_REG_READ_FAIL,
			AMDGV_ERROR_32_32(boot_error, fw_status));
		return;
	}

	socket_id = AMDGV_RAS_GPU_ERR_SOCKET_ID(boot_error);
	aid_id = AMDGV_RAS_GPU_ERR_AID_ID(boot_error);
	hbm_id = ((1 == AMDGV_RAS_GPU_ERR_HBM_ID(boot_error)) ? 0 : 1);

	/* Generic error message */
	if (!boot_error) {
		socket_id = mi300_ras_read_register_by_instance(adapt, mmSMUIO_MCM_CONFIG, 0);
		socket_id = REG_GET_FIELD(socket_id, SMUIO_MCM_CONFIG, SOCKET_ID);

		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_BOOT_FAIL,
			AMDGV_ERROR_16_16_32(socket_id, instance, fw_status));
		return;
	}


	if (AMDGV_RAS_GPU_ERR_UNKNOWN(boot_error))
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_BOOT_FAIL,
			AMDGV_ERROR_16_16_32(socket_id, aid_id, fw_status));

	if (AMDGV_RAS_GPU_ERR_MEM_TRAINING(boot_error))
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_MEM_TRAIN_FAIL,
			AMDGV_ERROR_16_16_32(socket_id, aid_id, hbm_id));

	if (AMDGV_RAS_GPU_ERR_FW_LOAD(boot_error))
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_FW_LOAD_FAIL,
			AMDGV_ERROR_32_32(socket_id, aid_id));

	if (AMDGV_RAS_GPU_ERR_WAFL_LINK_TRAINING(boot_error))
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_WAFL_LINK_TRAIN_FAIL,
			AMDGV_ERROR_32_32(socket_id, aid_id));

	if (AMDGV_RAS_GPU_ERR_XGMI_LINK_TRAINING(boot_error))
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_XGMI_LINK_TRAIN_FAIL,
			AMDGV_ERROR_32_32(socket_id, aid_id));

	if (AMDGV_RAS_GPU_ERR_USR_CP_LINK_TRAINING(boot_error))
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_USR_CP_LINK_TRAIN_FAIL,
			AMDGV_ERROR_32_32(socket_id, aid_id));

	if (AMDGV_RAS_GPU_ERR_USR_DP_LINK_TRAINING(boot_error))
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_USR_DP_LINK_TRAIN_FAIL,
			AMDGV_ERROR_32_32(socket_id, aid_id));

	if (AMDGV_RAS_GPU_ERR_HBM_MEM_TEST(boot_error))
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_HBM_MEM_TEST_FAIL,
			AMDGV_ERROR_16_16_32(socket_id, aid_id, hbm_id));

	if (AMDGV_RAS_GPU_ERR_HBM_BIST_TEST(boot_error))
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_HBM_BIST_TEST_FAIL,
			AMDGV_ERROR_16_16_32(socket_id, aid_id, hbm_id));
}

static int mi300_psp_wait_for_boot_complete_cb(void *context)
{
	uint32_t reg_data;
	uint32_t inst_id;

	void **context_array = (void **)context;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context_array[0];
	uint32_t *inst_pending_mask = (uint32_t *)context_array[1];
	uint32_t tmp_mask = *inst_pending_mask;

	for_each_id (inst_id, tmp_mask) {
		reg_data = mi300_ras_read_register_by_instance(adapt, mmMP0_SMN_C2PMSG_92, inst_id);
		if ((reg_data & AMDGV_RAS_BOOT_STATUS_MASK) == AMDGV_RAS_BOOT_STEADY_STATUS)
			*inst_pending_mask = *inst_pending_mask & ~BIT(inst_id);
	}

	return (*inst_pending_mask != 0);
}

void mi300_ras_query_boot_status(struct amdgv_adapter *adapt, uint32_t num_instances)
{
	uint32_t i;
	uint32_t wait_ret;
	uint32_t reg_data;
	uint32_t inst_pending_mask;
	void *context[2];

	inst_pending_mask = BIT(num_instances) - 1;
	context[0] = (void *)adapt;
	context[1] = (void *)&inst_pending_mask;

	wait_ret = amdgv_wait_for(adapt, mi300_psp_wait_for_boot_complete_cb,
				  (void *)context, AMDGV_TIMEOUT(TIMEOUT_RAS_BOOT_STATUS),
				  0);

	for (i = 0; i < num_instances; i++) {
		reg_data = mi300_ras_read_register_by_instance(adapt, mmMP0_SMN_C2PMSG_92, i);
		if ((reg_data & AMDGV_RAS_BOOT_STATUS_MASK) != AMDGV_RAS_BOOT_STEADY_STATUS)
			mi300_ras_boot_time_error_reporting(adapt, i);
	}
}

static void mi300_ecc_clear_pending_de_count(struct amdgv_adapter *adapt)
{
	adapt->ecc.pending_de_count = 0;
}

static bool mi300_ecc_is_poison_consumption_wgr(struct amdgv_adapter *adapt,
						uint32_t idx_vf,
						enum amdgv_ras_block block)
{
	if (!amdgv_ras_is_poison_mode_supported(adapt))
		return true;

	if ((adapt->flags & AMDGV_FLAG_VF_HANG_GPU_RESET))
		return true;

	if (idx_vf == AMDGV_PF_IDX)
		return true;

	if ((block != AMDGV_RAS_BLOCK__GFX) &&
	    (block != AMDGV_RAS_BLOCK__SDMA))
	    return true;

	return false;
}

static void mi300_ecc_start_poison_consumption_recovery(struct amdgv_adapter *adapt,
							uint32_t idx_vf,
							enum amdgv_ras_block block)
{
	if (amdgv_ras_eeprom_is_gpu_bad(adapt)) {
		amdgv_device_handle_bad_gpu(adapt);
		return;
	}
	if (mi300_ecc_is_poison_consumption_wgr(adapt, idx_vf, block))
		amdgv_sched_queue_event(adapt->xgmi.master_adapt ?
					adapt->xgmi.master_adapt : adapt,
					AMDGV_PF_IDX, AMDGV_EVENT_SCHED_FORCE_RESET_GPU, 0);
	else
		amdgv_sched_queue_event(adapt, idx_vf,
			AMDGV_EVENT_SCHED_FORCE_RESET_VF, AMDGV_SCHED_BLOCK_ALL);
}

static void mi300_ecc_find_poison(struct amdgv_adapter *adapt,
				  uint32_t idx_vf,
				  bool is_consumption)
{
	int i = 0, ret = 0;
	uint32_t start_count = adapt->ecc.pending_de_count;
	struct ras_err_data err_data = { 0 };

	amdgv_ecc_get_poison_stat(adapt);

	if (!amdgv_ecc_is_support(adapt, AMDGV_RAS_BLOCK__UMC))
		return;

	for (i = adapt->ecc.max_de_query_retry; i > 0; i--) {
		ret = amdgv_mca_get_new_banks(adapt, AMDGV_MCA_ERROR_TYPE_CE);
		if (ret || (start_count != adapt->ecc.pending_de_count))
			break;
		oss_msleep(1);
	}

	/* Trigger page retirement */
	if (!ret && is_consumption)
		amdgv_umc_process_ras_data_cb(adapt, &err_data, idx_vf);
}

static int mi300_ecc_poison_creation(struct amdgv_adapter *adapt,
				     struct amdgv_sched_event *event)
{
	mi300_ecc_find_poison(adapt, event->idx_vf, false);

	return 0;
}

static int mi300_ecc_poison_consumption(struct amdgv_adapter *adapt,
					struct amdgv_sched_event *event)
{
	mi300_ecc_find_poison(adapt, event->idx_vf, true);
	mi300_ecc_start_poison_consumption_recovery(adapt,
						    event->idx_vf,
						    event->data.poison.consumption.block);

	mi300_ecc_clear_pending_de_count(adapt);

	return 0;
}


static int mi300_ecc_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	adapt->ecc.enabled = 0;
	adapt->ecc.ras_cap = 0;
	adapt->ecc.correctable_error_num = 0;
	adapt->ecc.uncorrectable_error_num = 0;
	adapt->ecc.toggle_ecc_mode = NULL;
	adapt->ecc.supported = 0;
	adapt->ecc.bad_page_detection_mode = 0;
	oss_atomic_set(adapt->in_ecc_recovery, 0);

	switch (adapt->opt.bad_page_detection_mode) {
	case AMDGV_BAD_PAGE_DETECTION_MODE1:
		adapt->ecc.bad_page_detection_mode |= BIT(AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS) |
						      BIT(AMDGV_RAS_ECC_FLAG_IGNORE_RMA);
		break;
	case AMDGV_BAD_PAGE_DETECTION_MODE2:
		adapt->ecc.bad_page_detection_mode |= BIT(AMDGV_RAS_ECC_FLAG_IGNORE_RMA);
		break;
	default:
		adapt->ecc.bad_page_detection_mode = 0;
	}

	adapt->ecc.max_de_query_retry = MAX_IN_BAND_QUERY_ECC_CNT;

	if (!adapt->opt.use_legacy_eeprom_format &&
		adapt->opt.bad_page_record_threshold > 0 &&
		adapt->opt.bad_page_record_threshold <= MAX_BAD_PAGE_THRESHOLD)
		adapt->ecc.bad_page_record_threshold = adapt->opt.bad_page_record_threshold;
	else if (adapt->asic_type == CHIP_MI308X)
		adapt->ecc.bad_page_record_threshold = 256;
	else
		adapt->ecc.bad_page_record_threshold = 128;

	adapt->ecc.skip_row_rma = true;

	adapt->ecc.get_correctable_error_count = mi300_umc_update_error_count;
	adapt->ecc.get_uncorrectable_error_count = mi300_umc_update_uc_error_count;
	adapt->ecc.get_error_count = mi300_get_error_count;
	adapt->ecc.get_ras_cap = mi300_get_ras_cap;
	adapt->ecc.poison_consumption = mi300_ecc_poison_consumption;
	adapt->ecc.poison_creation = mi300_ecc_poison_creation;

	umc_v12_0_set_umc_funcs(adapt);
	nbio_v7_9_set_ras_funcs(adapt);

	gfx_v9_4_3_set_funcs(adapt);
	mmhub_v1_8_set_ras_funcs(adapt);

	sdma_v4_4_2_set_ras_funcs(adapt);

	mi300_xgmi_set_ras_funcs(adapt);

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

	if (amdgv_umc_recovery_sw_init(adapt)) {
		ret = AMDGV_FAILURE;
		goto out;
	}

	amdgv_cper_sw_init(adapt);
	mi300_mca_init(adapt);

	return ret;

out:
	amdgv_umc_ras_lock_fini(adapt);

	return ret;
}

static int mi300_ecc_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->log_level >= AMDGV_DEBUG_LEVEL) {
		AMDGV_DEBUG("UMC RAS (TOTAL ERROR): cce cnt %d, uce cnt %d\n",
			    adapt->ecc.correctable_error_num,
			    adapt->ecc.uncorrectable_error_num);
	}

	mi300_mca_fini(adapt);
	amdgv_cper_sw_fini(adapt);

	if (adapt->umc.err_addr)
		oss_free(adapt->umc.err_addr);

	amdgv_umc_ras_lock_fini(adapt);

	amdgv_umc_recovery_sw_fini(adapt);

	//Clear all ecc data
	oss_memset(&adapt->ecc, 0, sizeof(struct amdgv_ecc));

	return 0;
}

static int mi300_ecc_queue_err_query_post_init(struct amdgv_adapter *adapt)
{
	union amdgv_sched_event_data data;

	if (adapt->opt.skip_hw_init)
		return 0;

	data.mca_bank.error_type = AMDGV_MCA_ERROR_TYPE_CE;
	amdgv_sched_queue_event_ex_no_signal(adapt, AMDGV_PF_IDX,
					     AMDGV_EVENT_SCHED_UPDATE_MCA_BANKS,
					     AMDGV_SCHED_BLOCK_ALL,
					     data);

	data.mca_bank.error_type = AMDGV_MCA_ERROR_TYPE_UE;
	amdgv_sched_queue_event_ex_no_signal(adapt, AMDGV_PF_IDX,
					     AMDGV_EVENT_SCHED_UPDATE_MCA_BANKS,
					     AMDGV_SCHED_BLOCK_ALL,
					     data);

	return 0;
}

static int mi300_ecc_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct psp_ras_context *ras_context = &(adapt->psp.ras_context);

	amdgv_ecc_check_support(adapt);

	if (!(adapt->ecc.supported & BIT(AMDGV_RAS_MEM_ECC_SUPPORT)) &&
			!(adapt->ecc.supported & BIT(AMDGV_RAS_SRAM_ECC_SUPPORT))) {
		AMDGV_WARN("RAS ECC is not supported\n");
		goto out;
	}

	/* reload RAS TA for reset */
	if (adapt->reset.reset_state &&
		ras_context->ras_bin_buf &&
		ras_context->ras_bin_size) {
		ret = amdgv_psp_ras_initialize(adapt, ras_context->ras_bin_buf, ras_context->ras_bin_size);
		if (ret != PSP_STATUS__SUCCESS) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_INIT_FAIL, 0);
			ret = AMDGV_FAILURE;
			goto out;
		}
	}

	if (adapt->ecc.supported & BIT(AMDGV_RAS_SRAM_ECC_SUPPORT)) {
		if (adapt->nbio.ras) {
			if (adapt->nbio.ras->set_ras_controller_irq_state)
				adapt->nbio.ras->set_ras_controller_irq_state(adapt, true);
			if (adapt->nbio.ras->set_ras_err_event_athub_irq_state)
				adapt->nbio.ras->set_ras_err_event_athub_irq_state(adapt, true);
		}
	}

	if (amdgv_ecc_is_support(adapt, AMDGV_RAS_BLOCK__UMC)) {
		/* init ras eeprom and load umc retired pages */
		ret = amdgv_umc_recovery_hw_init(adapt);
		if (ret) {
			AMDGV_ERROR("Failed to Init RAS ECC\n");
			return ret;
		}
	}

	/* Queue a one-shot query after driver load. */
	mi300_ecc_queue_err_query_post_init(adapt);

out:
	return ret;
}

static int mi300_ecc_hw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->ecc.supported & BIT(AMDGV_RAS_MEM_ECC_SUPPORT)) {
		adapt->ecc.supported &= ~BIT(AMDGV_RAS_MEM_ECC_SUPPORT);
		adapt->ecc.enabled &= ~BIT(AMDGV_RAS_BLOCK__UMC);
		amdgv_umc_recovery_hw_fini(adapt);
	}

	if (adapt->ecc.supported & BIT(AMDGV_RAS_SRAM_ECC_SUPPORT)) {
		adapt->ecc.supported &= ~BIT(AMDGV_RAS_SRAM_ECC_SUPPORT);
		adapt->ecc.enabled &= ~BIT(AMDGV_RAS_BLOCK__SDMA) |
				       BIT(AMDGV_RAS_BLOCK__GFX);
	}

	return 0;
}

struct amdgv_init_func mi300_ecc_func = {
	.name = "mi300_ecc_func",
	.sw_init = mi300_ecc_sw_init,
	.sw_fini = mi300_ecc_sw_fini,
	.hw_init = mi300_ecc_hw_init,
	.hw_fini = mi300_ecc_hw_fini,
};

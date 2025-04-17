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
#include "amdgv_device.h"
#include "amdgv_notify.h"
#include "amdgv_ecc.h"
#include "amdgv_ras.h"
#include "amdgv_ras_eeprom.h"
#include "amdgv_psp_gfx_if.h"
#include "amdgv_mca.h"
#include "amdgv_umc.h"
#include "amdgv_xgmi.h"

#define AMDGV_ECC_CHECK_RAS_BLOCK_ERR(BLOCK, CE_ERR_NUM, UE_ERR_NUM, DE_ERR_NUM) do { \
	unsigned long corr_error = 0, uncorr_error = 0, deferred_error = 0; \
	struct amdgv_smi_ras_query_if info = {0}; \
	int ret = 0; \
	if (!(adapt->ecc.enabled & (1 << AMDGV_RAS_BLOCK__##BLOCK))) \
		break; \
	info.head.block = AMDGV_RAS_BLOCK__##BLOCK; \
	ret = amdgv_ecc_get_error_count(adapt, &info); \
	if (ret) { \
		AMDGV_ERROR("Unable to get %s error count.\n", #BLOCK); \
	} \
	corr_error = info.ce_count; \
	uncorr_error = info.ue_count; \
	deferred_error = info.de_count; \
	if ((corr_error > 0) && (!adapt->mca.enabled)) { \
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_##BLOCK##_CE_TOTAL, \
				(CE_ERR_NUM)); \
		amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ECC_CORR_ERROR, \
				  "%s ECC Correctable Error Detected.Count:%d", \
				  #BLOCK, corr_error); \
	} \
	if ((uncorr_error > 0) && (!adapt->mca.enabled)) { \
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_##BLOCK##_UE_TOTAL, \
				(UE_ERR_NUM)); \
		amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ECC_UNCORR_ERROR, \
				  "%s ECC UnCorrectable Error Detected.Count:%d", \
				  #BLOCK, uncorr_error); \
	} \
	if ((info.head.block == AMDGV_SMI_RAS_BLOCK__UMC) && \
	    (deferred_error > 0) && (!adapt->mca.enabled)) { \
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_UMC_DE_TOTAL, \
				(DE_ERR_NUM)); \
		amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ECC_DFCORR_ERROR, \
				  "%s ECC Deferred Error Detected.Count:%d", \
				  #BLOCK, deferred_error); \
	} \
} while (0)

#define AMDGV_ECC_RESET_RAS_BLOCK_ERR(BLOCK, CE_ERR_NUM, UE_ERR_NUM, RETURN_VALUE) do { \
	struct amdgv_smi_ras_query_if info = {0}; \
	int ret = 0; \
	if (!(adapt->ecc.enabled & (1 << AMDGV_RAS_BLOCK__##BLOCK))) \
	break; \
	info.head.block = AMDGV_RAS_BLOCK__##BLOCK; \
	ret = amdgv_ecc_get_error_count(adapt, &info); \
	if (ret) { \
		AMDGV_ERROR("Unable to reset %s error count.\n", #BLOCK); \
		RETURN_VALUE = ret; \
	} else { \
		CE_ERR_NUM = 0; \
		UE_ERR_NUM = 0; \
	} \
	ret = amdgv_mca_reset_block_error_count(adapt, AMDGV_RAS_BLOCK__##BLOCK); \
	if (ret) { \
		AMDGV_ERROR("Unable to reset %s error count through MCA interface.\n", #BLOCK); \
		RETURN_VALUE = ret; \
	} \
} while (0)

oss_atomic64_t  amdgv_ras_in_intr = {0};

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

const char *amdgv_ras_block_name[AMDGV_RAS_BLOCK__LAST] = {
	"UMC", "SDMA", "GFX", "MMHUB",
	"ATHUB", "PCIE_BIF", "HDP", "XGMI_WAFL",
	"DF", "SMN", "SEM", "MP0",
	"MP1", "FUSE", "MCA", "VCN",
	"JPEG", "IH", "MPIO"
};

int amdgv_ecc_is_support(struct amdgv_adapter *adapt,
		unsigned int block)
{
	if (block >= AMDGV_RAS_BLOCK__LAST)
		return 0;

	return adapt->ecc.enabled & (1 << block);
}

void amdgv_ecc_check_support(struct amdgv_adapter *adapt)
{
	/* move ecc judge in hw init since the atom fw table
	 * will only be initialized after smu table hw_init */
	if (amdgv_atomfirmware_mem_ecc_support(adapt)) {
		adapt->ecc.supported |= 1 << AMDGV_RAS_MEM_ECC_SUPPORT;
		adapt->ecc.enabled |= 1 << AMDGV_RAS_BLOCK__UMC;
		AMDGV_INFO("MEM ECC is active.\n");
	} else {
		AMDGV_INFO("MEM ECC is not presented.\n");
	}

	if (amdgv_atomfirmware_sram_ecc_support(adapt)) {
		adapt->ecc.supported |= 1 << AMDGV_RAS_SRAM_ECC_SUPPORT;
		adapt->ecc.enabled |= (1 << AMDGV_RAS_BLOCK__GFX | 1 << AMDGV_RAS_BLOCK__SDMA);
		AMDGV_INFO("SRAM ECC is active.\n");
	} else {
		AMDGV_INFO("SRAM ECC is not presented.\n");
	}

	/* Get ras_cap from PSP BL and update ecc.enabled */
	if (adapt->ecc.get_ras_cap) {
		adapt->ecc.enabled = adapt->ecc.get_ras_cap(adapt);
	}
}

int amdgv_ecc_clean_correctable_error_count(struct amdgv_adapter *adapt,
					    struct amdgv_smi_ras_query_if *info)
{
	struct ras_query_if *qif = (struct ras_query_if *)info;
	int ret = 0;
	int ce_count = 0;

	ret = adapt->gpumon.funcs->clean_correctable_error_count(adapt, &ce_count);

	qif->ce_count = ce_count;

	return ret;
}

int amdgv_ecc_get_correctable_error_count(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret;

	if (adapt->ecc.get_correctable_error_count) {
		ret = adapt->ecc.get_correctable_error_count(adapt, idx_vf);
		return ret;
	}

	return AMDGV_FAILURE;
}

int amdgv_ecc_get_uncorrectable_error_count(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret;

	if (adapt->ecc.get_uncorrectable_error_count) {
		ret = adapt->ecc.get_uncorrectable_error_count(adapt, idx_vf);
		return ret;
	}
	return AMDGV_FAILURE;
}

int amdgv_ecc_get_error_count(struct amdgv_adapter *adapt,
		struct amdgv_smi_ras_query_if *info)
{
	struct ras_err_data err_data = { 0 };
	struct ras_query_if *qif = (struct ras_query_if *)info;
	int ret = 0;

	qif->ce_count = qif->ue_count = qif->de_count = 0;

	if (!amdgv_ecc_is_support(adapt, qif->head.block))
		return AMDGV_FAILURE;

	switch (qif->head.block) {
	case AMDGV_RAS_BLOCK__UMC:
		if (!adapt->pp.pp_funcs || !adapt->umc.funcs) {
			if (adapt->gpumon.funcs->get_ecc_info)
				ret = adapt->gpumon.funcs->get_ecc_info(
					adapt, (int *)&(qif->ce_count),
					(int *)&(qif->ue_count));
		} else {
			if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_ecc_info) {
				ret = adapt->pp.pp_funcs->get_ecc_info(adapt,
								       &(adapt->ecc.umc_ecc));
				if (ret) {
					AMDGV_WARN(
						"Message SMU to get ecc info table failed.\n");
					return 0;
				}
			}

			amdgv_umc_process_ras_data_cb(adapt, &err_data, AMDGV_PF_IDX);
		}
		break;
	case AMDGV_RAS_BLOCK__GFX:
		if (adapt->gfx.funcs && adapt->gfx.funcs->query_ras_error_count)
			adapt->gfx.funcs->query_ras_error_count(adapt, &err_data);

		if (adapt->gfx.funcs && adapt->gfx.funcs->query_ras_error_status)
			adapt->gfx.funcs->query_ras_error_status(adapt);

		if (adapt->gfx.funcs && adapt->gfx.funcs->reset_ras_error_count)
			adapt->gfx.funcs->reset_ras_error_count(adapt);

		if (adapt->gfx.funcs && adapt->gfx.funcs->reset_ras_error_status)
			adapt->gfx.funcs->reset_ras_error_status(adapt);
		break;
	case AMDGV_RAS_BLOCK__SDMA:
		if (adapt->sdma.funcs && adapt->sdma.funcs->query_ras_error_count)
			adapt->sdma.funcs->query_ras_error_count(adapt, &err_data);

		if (adapt->sdma.funcs && adapt->sdma.funcs->reset_ras_error_count)
			adapt->sdma.funcs->reset_ras_error_count(adapt);
		break;
	case AMDGV_RAS_BLOCK__XGMI_WAFL:
		if (adapt->xgmi.funcs &&
			adapt->xgmi.funcs->query_ras_error_count)
			adapt->xgmi.funcs->query_ras_error_count(adapt, &err_data);

		if (adapt->xgmi.funcs &&
			adapt->xgmi.funcs->reset_ras_error_count)
			adapt->xgmi.funcs->reset_ras_error_count(adapt);
		break;
	case AMDGV_RAS_BLOCK__MMHUB:
		if (adapt->mmhub.funcs && adapt->mmhub.funcs->query_ras_error_count)
			adapt->mmhub.funcs->query_ras_error_count(adapt, &err_data);

		if (adapt->mmhub.funcs && adapt->mmhub.funcs->reset_ras_error_count)
			adapt->mmhub.funcs->reset_ras_error_count(adapt);
		break;
	default:
		break;
	}

	if (ret)
		return AMDGV_FAILURE;

	switch (qif->head.block) {
	case AMDGV_RAS_BLOCK__UMC:
		qif->ce_count = adapt->ecc.correctable_error_num;
		qif->ue_count = adapt->ecc.uncorrectable_error_num;
		qif->de_count = adapt->ecc.deferred_error_num;
		break;
	case AMDGV_RAS_BLOCK__GFX:
		if (err_data.ce_count) {
			adapt->ecc.gfx_correctable_error_num += err_data.ce_count;
			if (!adapt->mca.enabled)
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_GFX_CE, err_data.ce_count);
		}
		if (err_data.ue_count) {
			adapt->ecc.gfx_uncorrectable_error_num += err_data.ue_count;
			if (!adapt->mca.enabled)
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_GFX_UE, err_data.ue_count);
		}
		qif->ce_count = adapt->ecc.gfx_correctable_error_num;
		qif->ue_count = adapt->ecc.gfx_uncorrectable_error_num;
		break;
	case AMDGV_RAS_BLOCK__SDMA:
		if (err_data.ue_count) {
			adapt->ecc.sdma_uncorrectable_error_num += err_data.ue_count;
			if (!adapt->mca.enabled)
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_SDMA_UE, err_data.ue_count);
		}
		qif->ue_count = adapt->ecc.sdma_uncorrectable_error_num;
		break;
	case AMDGV_RAS_BLOCK__MMHUB:
		if (err_data.ce_count) {
			adapt->ecc.mmhub_correctable_error_num += err_data.ce_count;
			if (!adapt->mca.enabled)
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_MMHUB_CE, err_data.ce_count);
		}
		if (err_data.ue_count) {
			adapt->ecc.mmhub_uncorrectable_error_num += err_data.ue_count;
			if (!adapt->mca.enabled)
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_MMHUB_UE, err_data.ue_count);
		}
		qif->ce_count = adapt->ecc.mmhub_correctable_error_num;
		qif->ue_count = adapt->ecc.mmhub_uncorrectable_error_num;
		break;
	case AMDGV_RAS_BLOCK__XGMI_WAFL:
		if (err_data.ce_count) {
			adapt->ecc.xgmi_correctable_error_num += err_data.ce_count;
			if (!adapt->mca.enabled)
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_XGMI_WAFL_CE, err_data.ce_count);
		}
		if (err_data.ue_count) {
			adapt->ecc.xgmi_uncorrectable_error_num += err_data.ue_count;
			if (!adapt->mca.enabled)
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_XGMI_WAFL_UE, err_data.ue_count);
		}
		qif->ce_count = adapt->ecc.xgmi_correctable_error_num;
		qif->ue_count = adapt->ecc.xgmi_uncorrectable_error_num;
		break;
	default:
		AMDGV_WARN("Block unsupport ecc!\n");
		break;
	}

	return 0;
}

void amdgv_ecc_check_for_errors(struct amdgv_adapter *adapt, struct amdgv_sched_event *event)
{
	/* query poison consumption status */
	amdgv_ecc_get_poison_stat(adapt);

	/* This function is used to handle umc RAS interruption */
	if (amdgv_ecc_is_support(adapt, AMDGV_RAS_BLOCK__UMC)) {
		int corr_error = amdgv_ecc_get_correctable_error_count(adapt, event->idx_vf);
		int uncorr_error =
			amdgv_ecc_get_uncorrectable_error_count(adapt, event->idx_vf);

		if (corr_error > 0) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_CE,
					adapt->ecc.correctable_error_num);
			amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ECC_CORR_ERROR,
					  "ECC Correctable Error Detected.Count:%d",
					  corr_error);
		} else if (corr_error == 0)
			AMDGV_WARN("No ECC Correctable Error Detected.\n");
		else /* corr_error<0 */
			AMDGV_WARN("ECC not supported for this ASIC.\n");

		if (uncorr_error > 0) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_UCE,
					adapt->ecc.uncorrectable_error_num);
			amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ECC_UNCORR_ERROR,
					  "ECC UnCorrectable Error Detected.Count:%d",
					  uncorr_error);
		} else if (uncorr_error == 0)
			AMDGV_WARN("No ECC UnCorrectable Error Detected.\n");
		else /* uncorr_error<0 */
			AMDGV_WARN("ECC not supported for this ASIC.\n");
	} else
		AMDGV_WARN(
			"Counting correctable/uncorrectable errors not supported for this ASIC.\n");

	/* check if the ecc fault left this gpu as bad one */
	if (amdgv_ras_eeprom_is_gpu_bad(adapt) && event->id == AMDGV_EVENT_SCHED_RAS_UMC)
		amdgv_device_set_status(adapt, AMDGV_STATUS_HW_RMA);
}

void amdgv_ecc_query_ras_errors(struct amdgv_adapter *adapt)
{
	/* Query ecc info from in-band */
	amdgv_mca_get_new_banks(adapt, AMDGV_MCA_ERROR_TYPE_CE);
	amdgv_mca_get_new_banks(adapt, AMDGV_MCA_ERROR_TYPE_UE);

	if (adapt->ecc.supported & (1 << AMDGV_RAS_MEM_ECC_SUPPORT))
		AMDGV_ECC_CHECK_RAS_BLOCK_ERR(UMC, adapt->ecc.correctable_error_num,
			adapt->ecc.uncorrectable_error_num, adapt->ecc.deferred_error_num);

	if (adapt->ecc.supported & (1 << AMDGV_RAS_SRAM_ECC_SUPPORT)) {
		AMDGV_ECC_CHECK_RAS_BLOCK_ERR(GFX, adapt->ecc.gfx_correctable_error_num,
			adapt->ecc.gfx_uncorrectable_error_num, 0);
		AMDGV_ECC_CHECK_RAS_BLOCK_ERR(SDMA, adapt->ecc.sdma_correctable_error_num,
			adapt->ecc.sdma_uncorrectable_error_num, 0);
		AMDGV_ECC_CHECK_RAS_BLOCK_ERR(MMHUB, adapt->ecc.mmhub_correctable_error_num,
			adapt->ecc.mmhub_uncorrectable_error_num, 0);
		AMDGV_ECC_CHECK_RAS_BLOCK_ERR(XGMI_WAFL, adapt->ecc.xgmi_correctable_error_num,
			adapt->ecc.xgmi_uncorrectable_error_num, 0);
	}
}

void amdgv_ecc_check_global_ras_errors(struct amdgv_adapter *adapt)
{
	adapt->reset.reset_mode = adapt->umc.reset_mode;
	if (adapt->reset.reset_mode == AMDGV_RESET_MODE1)
		oss_atomic_set(adapt->in_ecc_recovery, 1);

	if (oss_atomic_cmpxchg(&amdgv_ras_in_intr, 0, 1))
		return;

	amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_FATAL_ERROR, 0);

	if (adapt->xgmi.master_adapt) {
		AMDGV_INFO("Forwarding reset event to master adapter:0x%x\n",
			   adapt->xgmi.master_adapt->bdf);
		adapt = adapt->xgmi.master_adapt;
	}
	amdgv_sched_queue_event(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_FORCE_RESET_GPU, 0);
}
int amdgv_ecc_enable_ras_feature(struct amdgv_adapter *adapt)
{
	union ta_ras_cmd_input *info = NULL;
	int ret = 0;

	info = oss_malloc(sizeof(union ta_ras_cmd_input));
	if (!info) {
		AMDGV_ERROR("Alloc ta_ras_cm_input failed.\n");
		return AMDGV_FAILURE;
	}
	oss_memset(info, 0, sizeof(union ta_ras_cmd_input));

	/* Only send ras enable cmd to gfx block currently */
	info->enable_features = (struct ta_ras_enable_features_input){
		.block_id = TA_RAS_BLOCK__GFX,
		.error_type = TA_RAS_ERROR__MULTI_UNCORRECTABLE,
	};

	if (amdgv_psp_ras_set_feature(adapt, info, true) != PSP_STATUS__SUCCESS)
		ret = AMDGV_FAILURE;

	if (info)
		oss_free(info);

	return ret;
}

int amdgv_ecc_disable_ras_feature(struct amdgv_adapter *adapt)
{
	union ta_ras_cmd_input *info = NULL;
	int ret = 0;

	info = oss_malloc(sizeof(union ta_ras_cmd_input));
	if (!info) {
		AMDGV_ERROR("Alloc ta_ras_cm_input failed.\n");
		return AMDGV_FAILURE;
	}
	oss_memset(info, 0, sizeof(union ta_ras_cmd_input));

	/* send ras disable cmd only to gfx block currently */
	info->disable_features = (struct ta_ras_disable_features_input){
		.block_id = TA_RAS_BLOCK__GFX,
		.error_type = TA_RAS_ERROR__MULTI_UNCORRECTABLE,
	};

	if (amdgv_psp_ras_set_feature(adapt, info, false) !=
	    PSP_STATUS__SUCCESS)
		ret = AMDGV_FAILURE;

	if (info)
		oss_free(info);

	return ret;
}

/* Init poison supported flag */
void amdgv_ras_poison_mode_init(struct amdgv_adapter *adapt)
{
	bool df_poison, umc_poison;

	if (!(adapt->df.funcs) || !(adapt->df.funcs->query_ras_poison_mode) ||
	    !(adapt->umc.funcs) || !(adapt->umc.funcs->query_ras_poison_mode))
		return;

	df_poison = adapt->df.funcs->query_ras_poison_mode(adapt);
	umc_poison = adapt->umc.funcs->query_ras_poison_mode(adapt);
	/* Only poison is set in both DF and UMC, we can support it */
	if (df_poison && umc_poison)
		adapt->ecc.supported |= (1 << AMDGV_RAS_POISON_ECC_SUPPORT);
	else if (df_poison != umc_poison)
		AMDGV_WARN("Poison setting is inconsistent in DF/UMC(%d:%d)!\n", df_poison,
			   umc_poison);
}

bool amdgv_ras_is_poison_mode_supported(struct amdgv_adapter *adapt)
{
	return (adapt->ecc.supported & (1 << AMDGV_RAS_POISON_ECC_SUPPORT));
}

void amdgv_ecc_get_poison_stat(struct amdgv_adapter *adapt)
{
	if (adapt->vcn.funcs && adapt->vcn.funcs->query_poison_status)
		adapt->vcn.funcs->query_poison_status(adapt);
}

enum amdgv_live_info_status amdgv_ecc_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ecc *ecc_info)
{
	if (adapt->ecc.enabled && adapt->ecc.eh_data) {
		ecc_info->correctable_error_num = adapt->ecc.correctable_error_num;
		ecc_info->uncorrectable_error_num = adapt->ecc.uncorrectable_error_num;

		amdgv_umc_release_bad_pages(adapt);
	}

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_ecc_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ecc *ecc_info)
{
	uint32_t ret;
	enum amdgv_memory_partition_mode nps_mode;

	amdgv_ecc_check_support(adapt);

	if (adapt->ecc.enabled && adapt->ecc.eh_data) {
		amdgv_ras_poison_mode_init(adapt);

		adapt->ecc.correctable_error_num = ecc_info->correctable_error_num;
		adapt->ecc.uncorrectable_error_num = ecc_info->uncorrectable_error_num;

		// read bad pages directly from eeprom
		ret = amdgv_ras_eeprom_init(adapt, &(adapt->eeprom_control));
		if (ret)
			goto free;

		if (adapt->nbio.ras &&
			 adapt->nbio.ras->get_curr_memory_partition_mode) {
			ret = adapt->nbio.ras->get_curr_memory_partition_mode(adapt, &nps_mode);
			if (ret) {
				AMDGV_ERROR("Failed to get current nps mode\n");
				goto release;
			}
			adapt->ecc.eh_data->nps_mode = nps_mode;
		}

		if (adapt->eeprom_control.num_recs) {
			ret = amdgv_umc_load_bad_pages(adapt);
			if (ret)
				goto free;
			ret = amdgv_umc_reserve_bad_pages(adapt);
			if (ret)
				goto release;
		}

	}

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;

release:
	if (ret)
		amdgv_umc_release_bad_pages(adapt);
free:
	if (ret)
		amdgv_ras_eeprom_fini(&adapt->eeprom_control);

	if (amdgv_ras_eeprom_is_gpu_bad(adapt))
		amdgv_device_handle_bad_gpu(adapt);

	return ret;
}

void amdgv_ras_get_error_type_name(uint32_t err_type, char *err_type_name)
{
	const char *err_name = NULL;

	if (!err_type_name)
		return;

	switch (err_type) {
	case AMDGV_RAS_ERROR__SINGLE_CORRECTABLE:
		err_name = "correctable";
		break;
	case AMDGV_RAS_ERROR__MULTI_UNCORRECTABLE:
		err_name = "uncorrectable";
		break;
	default:
		err_name = "unknown";
		break;
	}

	if (err_name)
		oss_memcpy(err_type_name, err_name, oss_strlen(err_name));

}

bool amdgv_ras_inst_get_memory_id_field(struct amdgv_adapter *adapt,
		const struct amdgv_ras_err_status_reg_entry *reg_entry,
		uint32_t instance,
		uint32_t *memory_id)
{
	uint32_t err_status_lo_data, err_status_lo_offset;

	if (!reg_entry)
			return false;

	err_status_lo_offset =
		AMDGV_RAS_REG_ENTRY_OFFSET(reg_entry->hwip, instance,
			reg_entry->seg_lo, reg_entry->reg_lo);
	err_status_lo_data = RREG32(err_status_lo_offset);

	if ((reg_entry->flags & AMDGV_RAS_ERR_STATUS_VALID) &&
	     !REG_GET_FIELD(err_status_lo_data, ERR_STATUS_LO, ERR_STATUS_VALID_FLAG))
		return false;

	*memory_id = REG_GET_FIELD(err_status_lo_data, ERR_STATUS_LO, MEMORY_ID);

	return true;
}

bool amdgv_ras_inst_get_err_cnt_field(struct amdgv_adapter *adapt,
		const struct amdgv_ras_err_status_reg_entry *reg_entry,
		uint32_t instance,
		unsigned long *err_cnt)
{
	uint32_t err_status_hi_data, err_status_hi_offset;

	if (!reg_entry)
		return false;

	err_status_hi_offset =
	AMDGV_RAS_REG_ENTRY_OFFSET(reg_entry->hwip, instance,
		reg_entry->seg_hi, reg_entry->reg_hi);
	err_status_hi_data = RREG32(err_status_hi_offset);

	if ((reg_entry->flags & AMDGV_RAS_ERR_INFO_VALID) &&
	     !REG_GET_FIELD(err_status_hi_data, ERR_STATUS_HI, ERR_INFO_VALID_FLAG))
		/* keep the check here in case we need to refer to the result later */
		AMDGV_DEBUG("Invalid err_info field\n");

	/* read err count */
	*err_cnt = REG_GET_FIELD(err_status_hi_data, ERR_STATUS, ERR_CNT);

	return true;
}

void amdgv_ras_inst_query_ras_error_count(struct amdgv_adapter *adapt,
		const struct amdgv_ras_err_status_reg_entry *reg_list,
		uint32_t reg_list_size,
		const struct amdgv_ras_memory_id_entry *mem_list,
		uint32_t mem_list_size,
		uint32_t instance,
		uint32_t err_type,
		unsigned long *err_count)
{
	uint32_t memory_id;
	unsigned long err_cnt;
	char err_type_name[16] = {0};
	uint32_t i, j;

	for (i = 0; i < reg_list_size; i++) {
		/* query memory_id from err_status_lo */
		if (!amdgv_ras_inst_get_memory_id_field(adapt, &reg_list[i],
				instance, &memory_id))
			continue;

		/* query err_cnt from err_status_hi */
		if (!amdgv_ras_inst_get_err_cnt_field(adapt, &reg_list[i],
				instance, &err_cnt) || !err_cnt)
			continue;

		*err_count += err_cnt;

		/* log the errors */
		amdgv_ras_get_error_type_name(err_type, err_type_name);
		if (!mem_list) {
			/* memory_list is not supported */
			AMDGV_INFO(
				"%ld %s hardware errors detected in %s, instance: %d, memory_id: %d\n",
				err_cnt, err_type_name,
				reg_list[i].block_name,
				instance, memory_id);
		} else {
			for (j = 0; j < mem_list_size; j++) {
				if (memory_id == mem_list[j].memory_id) {
					AMDGV_INFO(
						"%ld %s hardware errors detected in %s, instance: %d, memory block: %s\n",
						err_cnt, err_type_name,
						reg_list[i].block_name,
						instance, mem_list[j].name);
					break;
				}
			}
		}
	}
}

void amdgv_ras_inst_reset_ras_error_count(struct amdgv_adapter *adapt,
		const struct amdgv_ras_err_status_reg_entry *reg_list,
		uint32_t reg_list_size,
		uint32_t instance)
{
	uint32_t err_status_lo_offset, err_status_hi_offset;
	uint32_t i;

	for (i = 0; i < reg_list_size; i++) {
		err_status_lo_offset =
			AMDGV_RAS_REG_ENTRY_OFFSET(reg_list[i].hwip, instance,
				reg_list[i].seg_lo, reg_list[i].reg_lo);
		err_status_hi_offset =
			AMDGV_RAS_REG_ENTRY_OFFSET(reg_list[i].hwip, instance,
				reg_list[i].seg_hi, reg_list[i].reg_hi);
		WREG32(err_status_lo_offset, 0);
		WREG32(err_status_hi_offset, 0);
	}
}

int amdgv_ecc_reset_all_error_counts(struct amdgv_adapter *adapt)
{
	int last_ret = 0;

	if (adapt->ecc.supported & (1 << AMDGV_RAS_MEM_ECC_SUPPORT)) {
		AMDGV_ECC_RESET_RAS_BLOCK_ERR(UMC, adapt->ecc.correctable_error_num,
			adapt->ecc.uncorrectable_error_num, last_ret);

		adapt->ecc.deferred_error_num = 0;
	}

	if (adapt->ecc.supported & (1 << AMDGV_RAS_SRAM_ECC_SUPPORT)) {
		AMDGV_ECC_RESET_RAS_BLOCK_ERR(GFX, adapt->ecc.gfx_correctable_error_num,
			adapt->ecc.gfx_uncorrectable_error_num, last_ret);
		AMDGV_ECC_RESET_RAS_BLOCK_ERR(SDMA, adapt->ecc.sdma_correctable_error_num,
			adapt->ecc.sdma_uncorrectable_error_num, last_ret);
		AMDGV_ECC_RESET_RAS_BLOCK_ERR(MMHUB, adapt->ecc.mmhub_correctable_error_num,
			adapt->ecc.mmhub_uncorrectable_error_num, last_ret);
		AMDGV_ECC_RESET_RAS_BLOCK_ERR(XGMI_WAFL, adapt->ecc.xgmi_correctable_error_num,
			adapt->ecc.xgmi_uncorrectable_error_num, last_ret);
	}

	return last_ret;
}

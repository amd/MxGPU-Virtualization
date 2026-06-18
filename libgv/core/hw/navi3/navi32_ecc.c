/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gpumon.h>
#include <amdgv_ras.h>

#include "umc_v8_10.h"
#include "navi32_reg_inc.h"
#include "navi32_vcn.h"
#include "navi32_nbio.h"
#include "navi32_psp.h"
#include "amdgv_psp_gfx_if.h"
#include "navi32_powerplay.h"
#include "navi32_reset.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

#define NAVI32_MAX_EEPROM_RECORD_COUNT 32

static int navi32_umc_update_uc_error_count(struct amdgv_adapter *adapt,
						uint32_t idx_vf)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_ecc_info)
		ret = adapt->pp.pp_funcs->get_ecc_info(adapt, &(adapt->ecc.umc_ecc));

	if (ret)
		return 0;

	return amdgv_umc_update_uc_error_count(adapt, idx_vf);
}

static int navi32_umc_update_error_count(struct amdgv_adapter *adapt,
						uint32_t idx_vf)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_ecc_info)
		ret = adapt->pp.pp_funcs->get_ecc_info(adapt, &(adapt->ecc.umc_ecc));

	if (ret)
		return 0;

	return amdgv_umc_update_error_count(adapt, idx_vf);
}

static void navi32_handle_fatal_error_consumption(struct amdgv_adapter *adapt,
				struct ras_err_data *err_data)
{
	/* read eccinfo directly from bank registers since vram is inaccessible when DF_SYNC_FLOOD */
	if (adapt->umc.funcs && adapt->umc.funcs->hw_query_ras_error_count &&
		adapt->umc.max_ras_err_cnt_per_query) {
		adapt->umc.funcs->hw_query_ras_error_count(adapt, err_data);
	}

	if (adapt->umc.funcs && adapt->umc.funcs->hw_query_ras_error_address &&
		adapt->umc.max_ras_err_cnt_per_query) {
		err_data->err_addr = adapt->umc.err_addr;
		adapt->umc.funcs->hw_query_ras_error_address(adapt, err_data);
	}

	if (err_data->ce_count) {
		adapt->ecc.correctable_error_num += err_data->ce_count;
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_VF_CE, err_data->ce_count);
	}

	if (err_data->ue_count)
		adapt->ecc.uncorrectable_error_num += err_data->ue_count;

	/* Save the bad pages into SW cache. Driver will write to the
	 * EEPROM on hw_recovery */
	amdgv_umc_add_bad_pages(adapt, err_data->err_addr, err_data->err_addr_cnt, false);
	amdgv_umc_reserve_bad_pages(adapt);
}

static int navi32_poison_consumption(struct amdgv_adapter *adapt,
			 struct amdgv_sched_event *event)
{
	int ret = 0;
	struct ras_err_data err_data = { 0 };
	int idx_vf;
	uint64_t err_addr_gpu;
	uint32_t reg_data;
	uint32_t i;

	if (!adapt->ecc.fatal_error)
		amdgv_gpumon_ras_report(adapt, (int)PP_RAS_TYPE__POISON_COMSUMPTION);

	if (event->data.fed_data.src == AMDGV_FED_SRC_GC_RLC) {
		if (navi32_reset_grbm_soft_reset_stage_1(adapt, event->data.fed_data.data.gc_rlc_ip_bits.gl2c == 1))
			goto reset_gpu;
	} else if (event->data.fed_data.src == AMDGV_FED_SRC_DF_SYNC_FLOOD) {
		if (oss_atomic_read(adapt->in_ecc_recovery)) {
			/* MP1 fatal error WA: trigger PSP dram read to unhalt PSP during MP1 triggered sync flood
			 * should not do it after whole GPU reset */
			reg_data = RREG32_SOC15(MP0, 0, regMP0_SMN_C2PMSG_67);
			WREG32_SOC15(MP0, 0, regMP0_SMN_C2PMSG_67, reg_data + 0x10);
		}


		navi32_handle_fatal_error_consumption(adapt, &err_data);
		/* delay 1000ms as a WA for the mode1 reset for fatal error to be recovered back */
		oss_msleep(1000);

		goto reset_gpu;
	}

	if (adapt->pp.pp_funcs->get_ecc_info)
		ret = adapt->pp.pp_funcs->get_ecc_info(adapt, &(adapt->ecc.umc_ecc));
	if (ret)
		goto reset_gpu;

	/* page judgement and reserve */
	ret = amdgv_umc_process_ras_data_cb(adapt, &err_data, 0);

	if (adapt->ecc.eh_data->bp_replace_pending) {
		if (event->data.fed_data.src == AMDGV_FED_SRC_GC_RLC)
			navi32_reset_grbm_soft_reset_stage_2(adapt);
		goto reset_gpu;
	}

	if (err_data.err_addr_cnt == 0) {
		if (!adapt->ecc.fatal_error)
			goto exit;
		/* **MALL/parity** threshold checking and RMA or reset */
		goto reset_gpu;
	} else {
		/* some error found */
		if (amdgv_ras_eeprom_is_gpu_bad(adapt)) {
			goto reset_gpu;
		} else if (err_data.err_addr_cnt == 1 &&
			 adapt->bp_msg_type == AMDGV_BP_MSG_INVALID &&
			 adapt->ffbm.enabled) {
			/* get idx_vf from ffbm for flr */
			err_addr_gpu = amdgv_ffbm_spa_to_gpa(adapt, err_data.err_addr[0].retired_page << AMDGV_GPU_PAGE_SHIFT, &idx_vf);

			/* push into stack */
			oss_mutex_lock(adapt->ecc.unhandled_bps_lock);
			/* assume the unhandled bad page will not exceed MAX_UMC_CHANNEL_NUM */
			if (!(adapt->flags & AMDGV_FLAG_SKIP_BAD_PAGE_RETIREMENT)) {
				for (i = 0; i < err_data.err_addr_cnt && adapt->ecc.last_err_bps_cnt < MAX_UMC_CHANNEL_NUM; i++)
					adapt->ecc.last_err_bps[adapt->ecc.last_err_bps_cnt++] = err_data.err_addr[i];
			}
			oss_mutex_unlock(adapt->ecc.unhandled_bps_lock);

			if (event->data.fed_data.src == AMDGV_FED_SRC_GC_RLC)
				navi32_reset_grbm_soft_reset_stage_2(adapt);

			amdgv_sched_queue_event(adapt, idx_vf, AMDGV_EVENT_SCHED_FORCE_RESET_VF, AMDGV_SCHED_BLOCK_ALL);
		} else {
			/*
			 * not swapable ffbm vf page nor critial case
			 * (in PF fb and pf not used or multiple VF bps)
			 */
			if (event->data.fed_data.src == AMDGV_FED_SRC_GC_RLC)
				navi32_reset_grbm_soft_reset_stage_2(adapt);
			goto reset_gpu;
		}
	}
exit:
	return 0;

reset_gpu:
	if (amdgv_ras_eeprom_is_gpu_bad(adapt)) {
		amdgv_device_handle_bad_gpu(adapt);
	} else {
		adapt->reset.reset_mode = adapt->umc.reset_mode;
		amdgv_sched_queue_event(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_FORCE_RESET_GPU, 0);
	}

	return 0;
}

static int navi32_ecc_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	adapt->ecc.enabled = false;
	adapt->ecc.correctable_error_num = 0;
	adapt->ecc.uncorrectable_error_num = 0;
	adapt->ecc.toggle_ecc_mode = NULL;
	adapt->ecc.get_correctable_error_count = NULL;
	adapt->ecc.get_uncorrectable_error_count = NULL;

	adapt->ecc.get_correctable_error_count = navi32_umc_update_error_count;
	adapt->ecc.get_uncorrectable_error_count = navi32_umc_update_uc_error_count;
	adapt->ecc.get_error_count = amdgv_ecc_get_error_count;

	adapt->ecc.poison_consumption = navi32_poison_consumption;
	adapt->ecc.fatal_error = false;
	adapt->ecc.unhandled_bps_lock = oss_mutex_init();

	if (!adapt->opt.use_legacy_eeprom_format &&
		adapt->opt.bad_page_record_threshold > 0 &&
		adapt->opt.bad_page_record_threshold <= NAVI32_MAX_EEPROM_RECORD_COUNT)
		adapt->ecc.bad_page_record_threshold = adapt->opt.bad_page_record_threshold;
	else
		adapt->ecc.bad_page_record_threshold = NAVI32_MAX_EEPROM_RECORD_COUNT;

	AMDGV_INFO("Bad page record threshold is %d\n", adapt->ecc.bad_page_record_threshold);

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

	navi32_vcn_set_ras_funcs(adapt);
	navi32_nbio_set_ras_funcs(adapt);
	/* init umc structure for specific ASIC */
	umc_v8_10_set_umc_funcs(adapt);
	/* save umc error address in one query */
	adapt->umc.err_addr =
		oss_malloc(sizeof(struct eeprom_table_record) *
				UMC_V8_10_TOTAL_CHANNELS);
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

	return 0;

out:
	amdgv_umc_ras_lock_fini(adapt);

	return ret;
}

static int navi32_ecc_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->log_level >= AMDGV_DEBUG_LEVEL) {
		AMDGV_DEBUG("UMC RAS (TOTAL ERROR): cce cnt %d, uce cnt %d\n",
			    adapt->ecc.correctable_error_num,
			    adapt->ecc.uncorrectable_error_num);
	}

	adapt->ecc.enabled = false;
	adapt->ecc.correctable_error_num = 0;
	adapt->ecc.uncorrectable_error_num = 0;
	adapt->ecc.toggle_ecc_mode = NULL;
	adapt->ecc.get_correctable_error_count = NULL;
	adapt->ecc.get_uncorrectable_error_count = NULL;
	if (adapt->umc.err_addr)
		oss_free(adapt->umc.err_addr);
	oss_mutex_fini(adapt->ecc.unhandled_bps_lock);
	amdgv_umc_ras_lock_fini(adapt);

	amdgv_umc_sw_fini(adapt);

	return 0;
}

static int navi32_ecc_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	/* move ecc judge in hw init since the atom fw table
	 * will only be initialized after smu table hw_init */
	amdgv_ecc_check_support(adapt);

	/* skip RAS TA loading, but need to init related parameters */
	amdgv_ras_poison_mode_init(adapt);

	if (adapt->umc.funcs && adapt->umc.funcs->query_ras_memchandis)
		adapt->umc.funcs->query_ras_memchandis(adapt);

	if (!(adapt->ecc.supported & (1 << AMDGV_RAS_MEM_ECC_SUPPORT)) &&
			!(adapt->ecc.supported & (1 << AMDGV_RAS_SRAM_ECC_SUPPORT))) {
		AMDGV_WARN("RAS ECC is not supported\n");
		goto out;
	}

	/* on navi32 fatal error is supported, no sram ecc check here */
	if (adapt->nbio.ras) {
		if (adapt->nbio.ras->set_ras_controller_irq_state)
			adapt->nbio.ras->set_ras_controller_irq_state(adapt, true);
		if (adapt->nbio.ras->set_ras_err_event_athub_irq_state)
			adapt->nbio.ras->set_ras_err_event_athub_irq_state(adapt, true);
	}

	if (amdgv_ecc_is_support(adapt, AMDGV_RAS_BLOCK__UMC)) {
		if (adapt->umc.funcs &&
				adapt->umc.funcs->err_cnt_init)
			adapt->umc.funcs->err_cnt_init(adapt);

		adapt->umc.max_ras_err_cnt_per_query =
			UMC_V8_10_TOTAL_CHANNEL_NUM_NV32(adapt);

		/* init ras eeprom and load umc retired pages */
		ret = amdgv_umc_hw_init(adapt);
		if (ret)
			AMDGV_ERROR("Failed to Init RAS ECC\n");
	}

out:
	return ret;
}

static int navi32_ecc_hw_fini(struct amdgv_adapter *adapt)
{
	if (!adapt->ecc.enabled)
		return 0;
	amdgv_umc_hw_fini(adapt);

	return 0;
}

static int navi32_ecc_post_reset(struct amdgv_adapter *adapt)
{
	navi32_ecc_hw_fini(adapt);

	return 0;
}

static int navi32_ecc_import_live_data(struct amdgv_adapter *adapt, void *data)
{
	enum amdgv_live_info_status status = amdgv_ecc_import_live_data(adapt, (struct amdgv_live_info_ecc *)data);

	if (status)
		return status;

	if (adapt->ecc.enabled)
		adapt->ecc.bad_page_record_threshold = NAVI32_MAX_EEPROM_RECORD_COUNT;

	return status;
}

struct amdgv_init_func navi32_ecc_func = {
	.name = "navi32_ecc_func",
	.sw_init = navi32_ecc_sw_init,
	.sw_fini = navi32_ecc_sw_fini,
	.hw_init = navi32_ecc_hw_init,
	.hw_fini = navi32_ecc_hw_fini,
	.post_reset = navi32_ecc_post_reset,
};

struct amdgv_live_info_func navi32_ecc_live_info_func = {
	.live_info_op = AMDGV_LIVE_INFO_DATA__ECC,
	.import_live_data = navi32_ecc_import_live_data,
};

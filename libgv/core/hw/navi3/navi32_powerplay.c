/*
 * Copyright (C) 2021 - 2024 Advanced Micro Devices, Inc. All rights reserved.
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
#include <amdgv_vbios.h>
#include <atombios/atom.h>
#include <atombios/atomfirmware.h>
#include <amdgv_powerplay.h>
#include <amdgv_powerplay_swsmu.h>
#include "amdgv_umc.h"

//#include "navi32_reg_inc.h"
#include "navi32_reg_inc.h"
#include "navi32_psp.h"
#include "navi32_smu_driver_if.h"
#include "navi32_smu_ppsmc_wrapper.h"
#include "navi32_smu_pptable.h"
#include "navi32_powerplay_swsmu.h"
#include "navi32_fru.h"
#include "navi32_powerplay.h"

#include "navi32_reset.h"

static const uint32_t this_block = AMDGV_POWER_BLOCK;

#define AMD_MAX_USEC_TIMEOUT 200000 /* 200 ms */
#define NAVI32_VDD_GFX_BIT 0x0
#define NAVI32_VDD_SOC_BIT 0x1
#define NAVI32_VDD_MEM_BIT 0x3

#define NAVI32_UMC_CHANNEL_NUM  16

static int navi32_powerplay_send_msg_without_waiting(struct amdgv_adapter *adapt, uint16_t msg)
{
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_MSG_START, 0, regMP1_SMN_C2PMSG_66, msg);

	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_66), msg);

	/* Add the message to diagnosis data trace log */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_MSG_END, 0, regMP1_SMN_C2PMSG_66,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_66)));
	return 0;
}

static int navi32_powerplay_read_arg(struct amdgv_adapter *adapt, uint32_t *arg)
{
	*arg = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82));
	return 0;
}

static int navi32_powerplay_wait_for_response(struct amdgv_adapter *adapt,
uint32_t *val)
{
	int wait_ret;

	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90),
					   MP1_C2PMSG_90__CONTENT_MASK, 0,
					   AMDGV_TIMEOUT(TIMEOUT_SMU_REG), AMDGV_WAIT_CHECK_NE, 0);

	/* read as return value */
	*val = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90));

	/* Add the message to diagnosis data trace log */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_READ_RESP, wait_ret, regMP1_SMN_C2PMSG_90,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90)));

	/* timeout means wrong logic */
	if (wait_ret)
		return AMDGV_FAILURE;

	return 0;
}

int navi32_powerplay_send_msg(struct amdgv_adapter *adapt, uint16_t msg)
{
	int ret = 0;
	uint32_t resp = 0;

	oss_mutex_lock(adapt->pp.smu_lock);
	ret = navi32_powerplay_wait_for_response(adapt, &resp);

	if (ret == AMDGV_FAILURE) {
		if (navi32_powerplay_wait_idle(adapt, 0)) {
			oss_mutex_unlock(adapt->pp.smu_lock);
			AMDGV_WARN("SMU failed to go idle.\n");
			return AMDGV_FAILURE;
		}
	}

	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90), 0);

	navi32_powerplay_send_msg_without_waiting(adapt, msg);

	ret = navi32_powerplay_wait_for_response(adapt, &resp);
	oss_mutex_unlock(adapt->pp.smu_lock);

	if (ret == AMDGV_FAILURE || resp != PPSMC_Result_OK) {
		AMDGV_ERROR("Failed to send message 0x%x, response 0x%x\n", msg,
			resp);
		return AMDGV_FAILURE;
	}

	return 0;
}

int navi32_powerplay_send_msg_with_param(struct amdgv_adapter *adapt, uint16_t msg,
					uint32_t param)
{
	int ret = 0;
	uint32_t resp = 0;

	oss_mutex_lock(adapt->pp.smu_lock);
	ret = navi32_powerplay_wait_for_response(adapt, &resp);

	if (ret == AMDGV_FAILURE) {
		if (navi32_powerplay_wait_idle(adapt, 0)) {
			oss_mutex_unlock(adapt->pp.smu_lock);
			AMDGV_WARN("SMU failed to go idle.\n");
			return AMDGV_FAILURE;
		}
	}

	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_ARG_START, 0,
			regMP1_SMN_C2PMSG_82, param);

	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90), 0);

	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82), param);

	/* Add the message to diagnosis data trace log */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_ARG_END, 0, regMP1_SMN_C2PMSG_82,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82)));

	navi32_powerplay_send_msg_without_waiting(adapt, msg);

	ret = navi32_powerplay_wait_for_response(adapt,  &resp);
	oss_mutex_unlock(adapt->pp.smu_lock);

	if (ret == AMDGV_FAILURE || resp != PPSMC_Result_OK) {
		AMDGV_WARN("Failed to send message 0x%x, response 0x%x\n", msg,
			resp);
		return AMDGV_FAILURE;
	}

	return 0;
}

int navi32_powerplay_get_arg_with_param(struct amdgv_adapter *adapt, uint16_t msg,
				       uint32_t param, uint32_t *output)
{
	if (navi32_powerplay_send_msg_with_param(adapt, msg, param))
		return AMDGV_FAILURE;

	return navi32_powerplay_read_arg(adapt, output);
}

static int navi32_powerplay_initialize_dpm_context(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	smu->smu_dpm_context = oss_zalloc(sizeof(struct smu_13_0_dpm_context));
	if (smu->smu_dpm_context == NULL) {
		AMDGV_ERROR("Failed to alloc memory for smu dpm context\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_powerplay_destroy_dpm_context(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_13_0_dpm_context *dpm_ctxt = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	dpm_ctxt = (struct smu_13_0_dpm_context *)(smu->smu_dpm_context);

	if (dpm_ctxt)
		oss_free(dpm_ctxt);

	return 0;
}

static int navi32_powerplay_destroy_smc_table(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (!table_context) {
		AMDGV_ERROR("smu_table_context is NULL\n");
		return AMDGV_FAILURE;
	}

	if (table_context->watermark_table)
		oss_free(table_context->watermark_table);
	if (table_context->metrics_table)
		oss_free(table_context->metrics_table);
	if (table_context->config_table)
		oss_free(table_context->config_table);
	if (table_context->overdrive_table)
		oss_free(table_context->overdrive_table);
	if (table_context->i2c_table)
		oss_free(table_context->i2c_table);
	if (table_context->activity_monitor_table)
		oss_free(table_context->activity_monitor_table);
	if (table_context->ecc_info_table)
		oss_free(table_context->ecc_info_table);

	return ret;
}

static int navi32_smu_initialize_pptable(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);

	table_context = oss_zalloc(sizeof(struct smu_table_context));
	if (table_context == NULL) {
		AMDGV_ERROR("Failed to alloc memory for smu table ctxt\n");
		return AMDGV_FAILURE;
	}

	table_context->power_play_table_size = sizeof(struct smu_13_0_0_powerplay_table);
	table_context->power_play_table = oss_zalloc(table_context->power_play_table_size);

	if (table_context->power_play_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for pp tb\n");
		return AMDGV_FAILURE;
	}
	smu->smu_table_context = table_context;

	return ret;
}

static int navi32_powerplay_initialize_smc_tables(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (!table_context) {
		AMDGV_ERROR("smu_table_context is NULL\n");
		return AMDGV_FAILURE;
	}

	table_context->watermark_table = oss_zalloc(sizeof(Watermarks_t));
	if (table_context->watermark_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for watermark tb\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}
	table_context->metrics_table = oss_zalloc(sizeof(SmuMetrics_t));
	if (table_context->metrics_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for smu metric tb\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}
	table_context->config_table = oss_zalloc(sizeof(DriverSmuConfig_t));
	if (table_context->config_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for smu config tb\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}
	table_context->overdrive_table = oss_zalloc(sizeof(OverDriveTable_t));
	if (table_context->overdrive_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for od tb\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}
	table_context->i2c_table = oss_zalloc(sizeof(SwI2cRequest_t));
	if (table_context->i2c_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for i2c tb\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}
	table_context->activity_monitor_table =
		oss_zalloc(sizeof(DpmActivityMonitorCoeffInt_t));
	if (table_context->activity_monitor_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for smu act tb\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}
	table_context->ecc_info_table =
		oss_zalloc(sizeof(EccInfoTable_t));
	if (table_context->ecc_info_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for smu ecc info tb\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}

	// FB memory will be allocated in init_fb_allo func, fill in size here

	table_context->smc_pptable.alignment = SMU13_PAGE_SIZE;
	table_context->smc_pptable.size = sizeof(struct smu_13_0_0_powerplay_table);

	table_context->smc_watermark_table.alignment = SMU13_PAGE_SIZE;
	table_context->smc_watermark_table.size = sizeof(Watermarks_t);

	table_context->smc_metrics_table.alignment = SMU13_PAGE_SIZE;
	table_context->smc_metrics_table.size = sizeof(SmuMetrics_t);

	table_context->smc_driver_smu_config_table.alignment = SMU13_PAGE_SIZE;
	table_context->smc_driver_smu_config_table.size = sizeof(DriverSmuConfig_t);

	table_context->smc_overdrive_table.alignment = SMU13_PAGE_SIZE;
	table_context->smc_overdrive_table.size = sizeof(OverDriveTable_t);

	table_context->smc_i2c_table.alignment = SMU13_PAGE_SIZE;
	table_context->smc_i2c_table.size = sizeof(SwI2cRequest_t);

	table_context->smc_activity_monitor_table.alignment = SMU13_PAGE_SIZE;
	table_context->smc_activity_monitor_table.size = sizeof(DpmActivityMonitorCoeffInt_t);

	/* allocate space for tools table */
	table_context->smc_pm_status_log_table.alignment = SMU13_PAGE_SIZE;
	table_context->smc_pm_status_log_table.size = SMU13_TOOL_SIZE;

	table_context->smc_ecc_info_table.alignment = SMU13_PAGE_SIZE;
	table_context->smc_ecc_info_table.size = sizeof(EccInfoTable_t);

	ret = navi32_powerplay_initialize_dpm_context(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to allocate memory for dpm context!\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}

	goto out;

fail_free:
	navi32_powerplay_destroy_smc_table(adapt);

out:
	return ret;
}

static int navi32_smu_destroy_pptable(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (!table_context) {
		AMDGV_ERROR("smu_table_context is NULL\n");
		return AMDGV_FAILURE;
	}

	if (table_context && table_context->power_play_table)
		oss_free(table_context->power_play_table);
	if (table_context)
		oss_free(table_context);

	return ret;
}

static int navi32_powerplay_init_fb_allocations(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (!table_context) {
		AMDGV_ERROR("smu_table_context is NULL\n");
		return AMDGV_FAILURE;
	}

	/* allocate space for pptable */
	table_context->smc_pptable.mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf, table_context->smc_pptable.size,
					 table_context->smc_pptable.alignment, MEM_SMC_PPTABLE);
	if (!table_context->smc_pptable.mem) {
		navi32_powerplay_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for pp table!\n");
		return AMDGV_FAILURE;
	}
	/* allocate space for watermark table */
	table_context->smc_watermark_table.mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
					 table_context->smc_watermark_table.size,
					 table_context->smc_watermark_table.alignment,
					 MEM_SMC_WATERMARK_TABLE);
	if (!table_context->smc_watermark_table.mem) {
		navi32_powerplay_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for watermark table!\n");
		return AMDGV_FAILURE;
	}
	/* allocate space for metrics table */
	table_context->smc_metrics_table.mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
					 table_context->smc_metrics_table.size,
					 table_context->smc_metrics_table.alignment,
					 MEM_SMC_METRICS_TABLE);
	if (!table_context->smc_metrics_table.mem) {
		navi32_powerplay_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for metrics table!\n");
		return AMDGV_FAILURE;
	}
	/* allocate space for smu driver config table */
	table_context->smc_driver_smu_config_table.mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
					 table_context->smc_driver_smu_config_table.size,
					 table_context->smc_driver_smu_config_table.alignment,
					 MEM_SMU_CONFIG_TABLE);
	if (!table_context->smc_driver_smu_config_table.mem) {
		navi32_powerplay_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for smu config table!\n");
		return AMDGV_FAILURE;
	}
	/* allocate space for overdrive table */
	table_context->smc_overdrive_table.mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
					 table_context->smc_overdrive_table.size,
					 table_context->smc_overdrive_table.alignment,
					 MEM_SMC_OVERRIDE_TABLE);
	if (!table_context->smc_overdrive_table.mem) {
		navi32_powerplay_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for overdrive table!\n");
		return AMDGV_FAILURE;
	}
	/* allocate space for I2C command table */
	table_context->smc_i2c_table.mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf, table_context->smc_i2c_table.size,
					 table_context->smc_i2c_table.alignment, MEM_SMC_I2C_TABLE);
	if (!table_context->smc_i2c_table.mem) {
		navi32_powerplay_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for I2c table!\n");
		return AMDGV_FAILURE;
	}
	/* allocate space for activity monitor table */
	table_context->smc_activity_monitor_table.mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
					 table_context->smc_activity_monitor_table.size,
					 table_context->smc_activity_monitor_table.alignment,
					 MEM_SMC_ACTIVITY_TABLE);
	if (!table_context->smc_activity_monitor_table.mem) {
		navi32_powerplay_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for "
			    "activity monitor table!\n");
		return AMDGV_FAILURE;
	}
	/* allocate space for tools table */
	table_context->smc_pm_status_log_table.mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
					 table_context->smc_pm_status_log_table.size,
					 table_context->smc_pm_status_log_table.alignment,
					 MEM_SMC_PM_STATUS_TABLE);
	if (!table_context->smc_pm_status_log_table.mem) {
		navi32_powerplay_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for tools table!\n");
		return AMDGV_FAILURE;
	}
	/* allocate space for ecc info table */
	table_context->smc_ecc_info_table.mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
					 table_context->smc_ecc_info_table.size,
					 table_context->smc_ecc_info_table.alignment,
					 MEM_SMU_ECC_INFO_TABLE);
	if (!table_context->smc_ecc_info_table.mem) {
		navi32_powerplay_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for ecc info table!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_powerplay_release_fb_allocations(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (!table_context) {
		AMDGV_ERROR("smu_table_context is NULL\n");
		return AMDGV_FAILURE;
	}

	/* free space for pptable */
	if (table_context->smc_pptable.mem != NULL) {
		amdgv_memmgr_free(table_context->smc_pptable.mem);
		oss_memset(&table_context->smc_pptable, 0, sizeof(struct smu_local_memory));
	}
	/* free space for watermark table */
	if (table_context->smc_watermark_table.mem != NULL) {
		amdgv_memmgr_free(table_context->smc_watermark_table.mem);
		oss_memset(&table_context->smc_watermark_table, 0,
			   sizeof(struct smu_local_memory));
	}
	/* free space for metrics table */
	if (table_context->smc_metrics_table.mem != NULL) {
		amdgv_memmgr_free(table_context->smc_metrics_table.mem);
		oss_memset(&table_context->smc_metrics_table, 0,
			   sizeof(struct smu_local_memory));
	}
	/* free space for smu config table */
	if (table_context->smc_driver_smu_config_table.mem != NULL) {
		amdgv_memmgr_free(table_context->smc_driver_smu_config_table.mem);
		oss_memset(&table_context->smc_driver_smu_config_table, 0,
			   sizeof(struct smu_local_memory));
	}
	/* free space for overdrive table */
	if (table_context->smc_overdrive_table.mem != NULL) {
		amdgv_memmgr_free(table_context->smc_overdrive_table.mem);
		oss_memset(&table_context->smc_overdrive_table, 0,
			   sizeof(struct smu_local_memory));
	}

	/* free space for I2C command table */
	if (table_context->smc_i2c_table.mem != NULL) {
		amdgv_memmgr_free(table_context->smc_i2c_table.mem);
		oss_memset(&table_context->smc_i2c_table, 0, sizeof(struct smu_local_memory));
	}
	/* free space for activity monitor table */
	if (table_context->smc_activity_monitor_table.mem != NULL) {
		amdgv_memmgr_free(table_context->smc_activity_monitor_table.mem);
		oss_memset(&table_context->smc_activity_monitor_table, 0,
			   sizeof(struct smu_local_memory));
	}
	/* free space for tools table */
	if (table_context->smc_pm_status_log_table.mem != NULL) {
		amdgv_memmgr_free(table_context->smc_pm_status_log_table.mem);
		oss_memset(&table_context->smc_pm_status_log_table, 0,
			   sizeof(struct smu_local_memory));
	}
	/* free space for ecc info table */
	if (table_context->smc_ecc_info_table.mem != NULL) {
		amdgv_memmgr_free(table_context->smc_ecc_info_table.mem);
		oss_memset(&table_context->smc_ecc_info_table, 0,
			   sizeof(struct smu_local_memory));
	}

	return 0;
}

static int navi32_powerplay_smc_table_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_smu_initialize_pptable(adapt);

	if (ret) {
		AMDGV_ERROR("[navi32_smu_initialize_pptable] Failed!\n");
		return ret;
	}

	ret = navi32_powerplay_initialize_smc_tables(adapt);

	if (ret) {
		AMDGV_ERROR("[navi32_powerplay_initialize_smc_tables]Failed!\n");
		return ret;
	}

	ret = navi32_powerplay_init_fb_allocations(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to alloc tables in fb!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_powerplay_smc_table_sw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = (struct smu_context *)(adapt->pp.smu_backend);

	ret = navi32_powerplay_release_fb_allocations(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to free tables in fb!\n");
		return AMDGV_FAILURE;
	}

	ret = navi32_powerplay_destroy_smc_table(adapt);

	if (ret) {
		AMDGV_ERROR("[navi32_powerplay_destroy_smc_table] Failed!\n");
		return ret;
	}

	ret = navi32_powerplay_destroy_dpm_context(adapt);

	if (ret) {
		AMDGV_ERROR("[navi32_powerplay_destroy_dpm_context] Failed!\n");
		return ret;
	}

	ret = navi32_smu_destroy_pptable(adapt);

	if (ret) {
		AMDGV_ERROR("[navi32_smu_destroy_pptable] Failed!\n");
		return ret;
	}

	oss_free(smu);

	return ret;
}

int navi32_powerplay_get_fw_loaded_status(struct amdgv_adapter *adapt)
{
	/* could be mini-PMFW that responded.
	 * Check status of SMU main FW
	 * - use PPSMC_MSG_TestMessage to check SMU alive
	 * - the response will be the argument you pass + 1
	 * If SMU responds successfully, means SMU FW is loaded and ready
	 */
	if (navi32_powerplay_wait_idle(adapt, 0))
		return 0; /* SMU not ready */


	return 1; /* SMU main FW ready and responding */
}

static int navi32_powerplay_wait_for_fw_loaded(struct amdgv_adapter *adapt)
{
	if (navi32_powerplay_get_fw_loaded_status(adapt))
		return 0;
	return AMDGV_FAILURE;
}

static int navi32_powerplay_check_fw_status(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_powerplay_wait_for_fw_loaded(adapt);
	if (ret)
		AMDGV_ERROR("[navi32_powerplay_wait_for_fw_loaded] Failed!\n");
	else
		/* Fix to avoid gim wait too long and timeout before we send first
		 * smu message, since SMU does not init all the response registers to 1.
		 */
		WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90), 1);
	return ret;
}

static int navi32_powerplay_get_vbios_bootup_values(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int index = 0;
	uint16_t size = 0;
	uint8_t frev = 0;
	uint8_t crev = 0;
	struct atom_common_table_header *header;
	struct atom_firmware_info_v3_4 *v_3_4 = NULL;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_bios_boot_up_values *boot_values;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (!table_context) {
		AMDGV_ERROR("smu_table_context is NULL\n");
		return AMDGV_FAILURE;
	}

	boot_values = &table_context->boot_values;

	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
					    firmwareinfo);
	ret = smu_get_atom_data_table(adapt, index, &size, &frev, &crev, (uint8_t **)&header);

	if (ret) {
		AMDGV_ERROR("Failed to get table from VBIOS!\n");
		return ret;
	}
	if (header->format_revision != 3) {
		AMDGV_ERROR("unknowned atom_firmware_info_version for smu13!\n");
		return AMDGV_FAILURE;
	}

	if (header->format_revision == 3 && header->content_revision >= 4) {
		v_3_4 = (struct atom_firmware_info_v3_4 *)header;
		boot_values->revision = v_3_4->firmware_revision;
		boot_values->gfxclk = v_3_4->bootup_sclk_in10khz;
		boot_values->uclk = v_3_4->bootup_mclk_in10khz;
		boot_values->socclk = 0;
		boot_values->dcefclk = 0;
		boot_values->vddc = v_3_4->bootup_vddc_mv;
		boot_values->vddci = v_3_4->bootup_vddci_mv;
		boot_values->mvddc = v_3_4->bootup_mvddc_mv;
		boot_values->vdd_gfx = v_3_4->bootup_vddgfx_mv;
		boot_values->cooling_id = v_3_4->coolingsolution_id;
		boot_values->pp_table_id = v_3_4->pplib_pptable_id;
	}
	return 0;
}

static int navi32_powerplay_read_pptable_from_vbios(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (!table_context) {
		AMDGV_ERROR("smu_table_context is NULL\n");
		return AMDGV_FAILURE;
	}

	AMDGV_DEBUG("Reading Soft PPTable softPowerPlayTable9999_signed\n");

	oss_memcpy(table_context->power_play_table, (char *)softPowerPlayTable9999_signed,
				sizeof(struct smu_13_0_0_powerplay_table));

	table_context->power_play_table_size = sizeof(struct smu_13_0_0_powerplay_table);

	((struct smu_13_0_0_powerplay_table *)(table_context->power_play_table))->smc_pptable.SkuTable.DebugOverrides = 0x400;

	return 0;
}

static int navi32_powerplay_append_vbios_pptable(struct amdgv_adapter *adapt)
{
	int index = 0;
	uint16_t size = 0;
	uint8_t frev = 0;
	uint8_t crev = 0;
	uint16_t data_offset = 0;
	struct atom_context *ctx = adapt->vbios.atom_context;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_13_0_0_powerplay_table *powerplay_table = NULL;
	struct atom_smc_dpm_info_table *dpm = NULL;
	BoardTable_t *board_table = NULL;
	bool ret = 0;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (!table_context) {
		AMDGV_ERROR("smu_table_context is NULL\n");
		return AMDGV_FAILURE;
	}

	powerplay_table = (struct smu_13_0_0_powerplay_table *)table_context->power_play_table;

	if (!powerplay_table) {
		AMDGV_ERROR("table_context->power_play_table is NULL\n");
		return AMDGV_FAILURE;
	}

	board_table = &(powerplay_table->smc_pptable.BoardTable);

	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
					    smc_dpm_info);
	ret = amdgv_atom_parse_data_header(ctx, index, &size, &frev, &crev, &data_offset);
	if (ret)
		dpm = (struct atom_smc_dpm_info_table *)((uint8_t *)ctx->bios + data_offset);

	if (dpm != NULL) {
		oss_memcpy(board_table, &(dpm->BoardTable), sizeof(BoardTable_t));
	} else {
		AMDGV_ERROR("failed to get smc dpm info from vbios!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_powerplay_get_power_capacity(struct amdgv_adapter *adapt, int *val)
{
	uint32_t asic_default_power_limit = 0;
	int ret = 0;

	ret = navi32_powerplay_get_arg_with_param(adapt, SMU_13_0_MSG__GET_PPT_LIMIT,
						 POWER_SOURCE_AC << 16,
						 &asic_default_power_limit);
	if (ret)
		return ret;

	*val = asic_default_power_limit;

	return 0;
}

static int navi32_powerplay_get_dpm_level_count(struct amdgv_adapter *adapt, int *val)
{
	int ret = 0;
	uint32_t param;

	/* use the specify parameter 0xff to query dpm levle count from SMU */
	param = (uint32_t)(((PPCLK_GFXCLK & 0xffff) << 16) | 0xff);

	ret = navi32_powerplay_get_arg_with_param(adapt, SMU_13_0_MSG__GET_DPM_FREQ_BY_INDEX,
						 param, val);
	if (!ret)
		*val = *val & 0x7fffffff;

	return ret;
}

static int navi32_powerplay_check_pptable(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_13_0_0_powerplay_table *powerplay_table = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (!table_context) {
		AMDGV_ERROR("smu_table_context is NULL\n");
		return AMDGV_FAILURE;
	}

	powerplay_table = table_context->power_play_table;
	if (!powerplay_table) {
		AMDGV_ERROR("table_context->power_play_table is NULL\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("powerplay_table header format_revision=0x%x\n", powerplay_table->header.format_revision);
	AMDGV_INFO("powerplay_table table_size=0x%x\n", powerplay_table->table_size);
	AMDGV_INFO("powerplay_table golden_pp_id=0x%x\n", powerplay_table->golden_pp_id);
	AMDGV_INFO("powerplay_table table_revision=0x%x\n", powerplay_table->table_revision);
	AMDGV_INFO("PPTable Sku Version=0x%x\n", powerplay_table->smc_pptable.SkuTable.Version);
	AMDGV_INFO("PPTable Board Version=0x%x\n", powerplay_table->smc_pptable.BoardTable.Version);
	AMDGV_INFO("PPTable Debu Overrides=0x%x\n", powerplay_table->smc_pptable.SkuTable.DebugOverrides);

	if (SMU_13_0_0_TABLE_FORMAT_REVISION > powerplay_table->header.format_revision) {
		AMDGV_ERROR("Unsupported PP table format!\n");
		return 0;
	}
	if (powerplay_table->header.structuresize == 0) {
		AMDGV_ERROR("Invalid PP table!\n");
		return 0;
	}

	return 0;
}

static int navi32_powerplay_parse_default_dpm_tables(struct amdgv_adapter *adapt,
						     SkuTable_t *driver_sku_tbl,
						     struct smu_13_0_dpm_context *dpm_context)
{
	uint8_t num_clk_level[PPCLK_COUNT];
	struct smu_13_0_dpm_tables *cur_tb = NULL;
	cur_tb = &dpm_context->dpm_tables;

	if (/*def_tb == NULL ||*/ cur_tb == NULL) {
		AMDGV_ERROR("Unable to get default/current DPM table!\n");
		return AMDGV_FAILURE;
	}

	if (!driver_sku_tbl) {
			AMDGV_ERROR("Cannot get SKU table!\n");
			return AMDGV_FAILURE;
	}

	num_clk_level[PPCLK_SOCCLK] = driver_sku_tbl->DpmDescriptor[PPCLK_SOCCLK].NumDiscreteLevels;
	num_clk_level[PPCLK_GFXCLK] = driver_sku_tbl->DpmDescriptor[PPCLK_GFXCLK].NumDiscreteLevels;
	num_clk_level[PPCLK_UCLK] = driver_sku_tbl->DpmDescriptor[PPCLK_UCLK].NumDiscreteLevels;
	num_clk_level[PPCLK_VCLK_0] = driver_sku_tbl->DpmDescriptor[PPCLK_VCLK_0].NumDiscreteLevels;
	num_clk_level[PPCLK_DCLK_0] = driver_sku_tbl->DpmDescriptor[PPCLK_DCLK_0].NumDiscreteLevels;
	num_clk_level[PPCLK_DISPCLK] = driver_sku_tbl->DpmDescriptor[PPCLK_DISPCLK].NumDiscreteLevels;
	num_clk_level[PPCLK_DPPCLK] = driver_sku_tbl->DpmDescriptor[PPCLK_DPPCLK].NumDiscreteLevels;
	num_clk_level[PPCLK_DPREFCLK] = driver_sku_tbl->DpmDescriptor[PPCLK_DPREFCLK].NumDiscreteLevels;
	num_clk_level[PPCLK_DCFCLK] = driver_sku_tbl->DpmDescriptor[PPCLK_DCFCLK].NumDiscreteLevels;
	num_clk_level[PPCLK_DTBCLK] = driver_sku_tbl->DpmDescriptor[PPCLK_DTBCLK].NumDiscreteLevels;
	num_clk_level[PPCLK_FCLK] = driver_sku_tbl->DpmDescriptor[PPCLK_FCLK].NumDiscreteLevels;

	if (NUM_SOCCLK_DPM_LEVELS < num_clk_level[PPCLK_SOCCLK] ||
		NUM_GFXCLK_DPM_LEVELS < num_clk_level[PPCLK_GFXCLK] ||
		NUM_UCLK_DPM_LEVELS < num_clk_level[PPCLK_UCLK] ||
		NUM_VCLK_DPM_LEVELS < num_clk_level[PPCLK_VCLK_0] ||
		NUM_DCLK_DPM_LEVELS < num_clk_level[PPCLK_DCLK_0] ||
		NUM_DISPCLK_DPM_LEVELS < num_clk_level[PPCLK_DISPCLK] ||
		NUM_DPPCLK_DPM_LEVELS < num_clk_level[PPCLK_DPPCLK] ||
		NUM_DPREFCLK_DPM_LEVELS < num_clk_level[PPCLK_DPREFCLK] ||
		NUM_DCFCLK_DPM_LEVELS < num_clk_level[PPCLK_DCFCLK] ||
		NUM_DTBCLK_DPM_LEVELS < num_clk_level[PPCLK_DTBCLK] ||
		NUM_FCLK_DPM_LEVELS < num_clk_level[PPCLK_FCLK]) {
		AMDGV_ERROR("Get invalid value from DpmDescriptor!\n");
		return AMDGV_FAILURE;
	}

	if (num_clk_level[PPCLK_SOCCLK] == 0 ||
		num_clk_level[PPCLK_GFXCLK] == 0 ||
		num_clk_level[PPCLK_UCLK] == 0 ||
		num_clk_level[PPCLK_VCLK_0] == 0 ||
		num_clk_level[PPCLK_DCLK_0] == 0 ||
		num_clk_level[PPCLK_DISPCLK] == 0 ||
		num_clk_level[PPCLK_DPPCLK] == 0 ||
		num_clk_level[PPCLK_DPREFCLK] == 0 ||
		num_clk_level[PPCLK_DCFCLK] == 0 ||
		num_clk_level[PPCLK_DTBCLK] == 0 ||
		num_clk_level[PPCLK_FCLK] == 0) {
		AMDGV_ERROR("Get 0 from DpmDescriptor!\n");
		return AMDGV_FAILURE;
	}
	cur_tb->soc_table.min = driver_sku_tbl->FreqTableSocclk[0];
	cur_tb->soc_table.max =
		driver_sku_tbl->FreqTableSocclk[num_clk_level[PPCLK_SOCCLK] - 1];

	cur_tb->gfx_table.min = driver_sku_tbl->FreqTableGfx[0];
	cur_tb->gfx_table.max = driver_sku_tbl->FreqTableGfx[num_clk_level[PPCLK_GFXCLK] - 1];

	cur_tb->uclk_table.min = driver_sku_tbl->FreqTableUclk[0];
	cur_tb->uclk_table.max = driver_sku_tbl->FreqTableUclk[num_clk_level[PPCLK_UCLK] - 1];

	cur_tb->vclk_table.min = driver_sku_tbl->FreqTableVclk[0];
	cur_tb->vclk_table.max =
		driver_sku_tbl->FreqTableVclk[num_clk_level[PPCLK_VCLK_0] - 1];

	cur_tb->dclk_table.min = driver_sku_tbl->FreqTableDclk[0];
	cur_tb->dclk_table.max =
		driver_sku_tbl->FreqTableDclk[num_clk_level[PPCLK_DCLK_0] - 1];

	cur_tb->dispclk_table.min = driver_sku_tbl->FreqTableDispclk[0];
	cur_tb->dispclk_table.max =
		driver_sku_tbl->FreqTableDispclk[num_clk_level[PPCLK_DISPCLK] - 1];

	cur_tb->dppclk_table.min = driver_sku_tbl->FreqTableDppClk[0];
	cur_tb->dppclk_table.max =
		driver_sku_tbl->FreqTableDppClk[num_clk_level[PPCLK_DPPCLK] - 1];

	cur_tb->dprefclk_table.min = driver_sku_tbl->FreqTableDprefclk[0];
	cur_tb->dprefclk_table.max =
		driver_sku_tbl->FreqTableDprefclk[num_clk_level[PPCLK_DPREFCLK]-1];

	cur_tb->dcfclk_table.min = driver_sku_tbl->FreqTableDcfclk[0];
	cur_tb->dcfclk_table.max =
		driver_sku_tbl->FreqTableDcfclk[num_clk_level[PPCLK_DCFCLK]-1];

	cur_tb->dtbclk_table.min = driver_sku_tbl->FreqTableDtbclk[0];
	cur_tb->dtbclk_table.max =
		driver_sku_tbl->FreqTableDtbclk[num_clk_level[PPCLK_DTBCLK]-1];

	cur_tb->fclk_table.min = driver_sku_tbl->FreqTableFclk[0];
	cur_tb->fclk_table.max =
		driver_sku_tbl->FreqTableFclk[num_clk_level[PPCLK_FCLK]-1];

	return 0;
}

static int navi32_powerplay_set_default_dpm_tables(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_13_0_dpm_context *dpm_context = NULL;
	struct smu_13_0_0_powerplay_table *powerplay_table = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	if (!smu) {
		AMDGV_ERROR("smu backend is NULL\n");
		return AMDGV_FAILURE;
	}

	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (!table_context) {
		AMDGV_ERROR("smu_table_context is NULL\n");
		return AMDGV_FAILURE;
	}

	dpm_context = (struct smu_13_0_dpm_context *)smu->smu_dpm_context;
	if (!dpm_context) {
		AMDGV_ERROR("smu_dpm_context is NULL\n");
		return AMDGV_FAILURE;
	}

	powerplay_table =  (struct smu_13_0_0_powerplay_table *)table_context->power_play_table;
	if (!powerplay_table) {
		AMDGV_ERROR("powerplay_table is NULL\n");
		return AMDGV_FAILURE;
	}

	oss_memset(dpm_context, 0, sizeof(struct smu_13_0_dpm_context));

	return navi32_powerplay_parse_default_dpm_tables(adapt, &powerplay_table->smc_pptable.SkuTable, dpm_context);
}

static bool navi32_powerplay_check_clock_type(struct amdgv_adapter *adapt,
					     enum pp_clock_type clk)
{
	bool allowed = 0;
	switch (clk) {
	case PP_CLOCK_TYPE__GFX:
	case PP_CLOCK_TYPE__UCLK:
	case PP_CLOCK_TYPE__SOC:
	case PP_CLOCK_TYPE__VCLK:
	case PP_CLOCK_TYPE__DCLK:
	case PP_CLOCK_TYPE__DCLK_1:
	case PP_CLOCK_TYPE__VCLK_1:
	case PP_CLOCK_TYPE__DISPCLK:
	case PP_CLOCK_TYPE__DPPCLK:
	case PP_CLOCK_TYPE__DPREFCLK:
	case PP_CLOCK_TYPE__DCFCLK:
	case PP_CLOCK_TYPE__FCLK:
	case PP_CLOCK_TYPE__DTBCLK:
		allowed = true;
		break;
	default:
		allowed = false;
		break;
	}
	return allowed;
}

static void navi32_powerplay_get_curr_limit_by_type(struct amdgv_adapter *adapt,
						   enum pp_clock_type clk,
						   struct smu_13_0_dpm_tables *current_dpm_tables,
						   struct smu_13_0_dpm_table **selected_table)
{
	switch (clk) {
	case PP_CLOCK_TYPE__GFX:
		*selected_table = &current_dpm_tables->gfx_table;
		break;
	case PP_CLOCK_TYPE__SOC:
		*selected_table = &current_dpm_tables->soc_table;
		break;
	case PP_CLOCK_TYPE__UCLK:
		*selected_table = &current_dpm_tables->uclk_table;
		break;
	case PP_CLOCK_TYPE__DCLK_1:
	case PP_CLOCK_TYPE__DCLK:
		*selected_table = &current_dpm_tables->dclk_table;
		break;
	case PP_CLOCK_TYPE__VCLK_1:
	case PP_CLOCK_TYPE__VCLK:
		*selected_table = &current_dpm_tables->vclk_table;
		break;
	default:
		*selected_table = NULL;
		break;
	}

}

static uint32_t navi32_powerplay_convert_clk_type(struct amdgv_adapter *adapt,
						 enum pp_clock_type clk)
{
	uint32_t smu_clk = 0xFFFF;
	switch (clk) {
	case PP_CLOCK_TYPE__GFX:
		smu_clk = PPCLK_GFXCLK << 16;
		break;
	case PP_CLOCK_TYPE__UCLK:
		smu_clk = PPCLK_UCLK << 16;
		break;
	case PP_CLOCK_TYPE__DCLK:
		smu_clk = PPCLK_DCLK_0 << 16;
		break;
	case PP_CLOCK_TYPE__VCLK:
		smu_clk = PPCLK_VCLK_0 << 16;
		break;
	case PP_CLOCK_TYPE__DCLK_1:
		smu_clk = PPCLK_DCLK_1 << 16;
		break;
	case PP_CLOCK_TYPE__VCLK_1:
		smu_clk = PPCLK_VCLK_1 << 16;
		break;
	case PP_CLOCK_TYPE__DISPCLK:
		smu_clk = PPCLK_DISPCLK << 16;
		break;
	default:
		AMDGV_ERROR("Unable to find matching clock type!\n");
		break;
	}
	return smu_clk;
}

int navi32_powerplay_get_clock_limit(struct amdgv_adapter *adapt, enum pp_clock_type clk,
				    enum pp_clock_limit_type limit_type, uint32_t *freq)
{
	struct smu_context *smu = NULL;
	struct smu_13_0_dpm_context *dpm_context = NULL;
	struct smu_13_0_dpm_tables *current_dpm_tables = NULL;
	struct smu_13_0_dpm_table *selected_table = NULL;
	uint16_t msg = 0;
	uint32_t param = 0;
	uint32_t driver_freq = 0;
	int ret = 0;

	if (!navi32_powerplay_check_clock_type(adapt, clk))
		return AMDGV_FAILURE;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	dpm_context = (struct smu_13_0_dpm_context *)smu->smu_dpm_context;

	if (dpm_context == NULL) {
		AMDGV_ERROR("Unable to get DPM context!\n");
		return AMDGV_FAILURE;
	}

	current_dpm_tables = (struct smu_13_0_dpm_tables *) &dpm_context->dpm_tables;
	navi32_powerplay_get_curr_limit_by_type(adapt, clk, current_dpm_tables, &selected_table);

	if (selected_table == NULL) {
		AMDGV_ERROR("Unable to get current limit table!\n");
		return AMDGV_FAILURE;
	}

	switch (limit_type) {
	case PP_CLOCK_LIMIT_TYPE__SOFT_MAX:
		msg = SMU_13_0_MSG__GET_MAX_DPM_FREQ;
		param = navi32_powerplay_convert_clk_type(adapt, clk);
		driver_freq = selected_table->max;

		if (param != 0xFFFF)
			ret = navi32_powerplay_send_msg_with_param(adapt, msg, param);
		if (ret == 0)
			ret = navi32_powerplay_read_arg(adapt, freq);

		*freq = (driver_freq > *freq) ? driver_freq : *freq;
		break;
	case PP_CLOCK_LIMIT_TYPE__SOFT_MIN:
		msg = SMU_13_0_MSG__GET_MIN_DPM_FREQ;
		param = navi32_powerplay_convert_clk_type(adapt, clk);
		driver_freq = selected_table->min;

		if (param != 0xFFFF)
			ret = navi32_powerplay_get_arg_with_param(adapt, msg, param, freq);

		*freq = (driver_freq < *freq) ? driver_freq : *freq;
		break;
	default:
		AMDGV_ERROR("clock limit isn't supported\n");
		ret = AMDGV_FAILURE;
		break;
	}

	return ret;
}

static int navi32_powerplay_populate_smc_pptable(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_powerplay_set_default_dpm_tables(adapt);
	return ret;
}

static int navi32_powerplay_check_fw_version(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t driver_version = 0;

	ret = navi32_powerplay_send_msg(adapt, SMU_13_0_MSG__GET_DRIVER_IF_VERSION);

	if (ret == 0) {
		ret = navi32_powerplay_read_arg(adapt, &driver_version);
		if (ret == 0) {
			if (driver_version != adapt->pp.smu_fw_version) {
				AMDGV_ERROR("SMU driver version(0x%x) doesn't"
					    " match SW-defined version(0x%x)!\n",
					    driver_version, adapt->pp.smu_fw_version);
				ret = AMDGV_FAILURE;
			}
		}
	} else {
		AMDGV_ERROR("Failed to get F/W version!\n");
	}

	return ret;
}

static int navi32_powerplay_copy_table_from_smc(struct amdgv_adapter *adapt,
					       struct smu_local_memory *fb_memory,
					       void *system_memory, uint32_t table_id)
{
	int ret = AMDGV_FAILURE;
	uint32_t mask_low =
		(uint32_t)((amdgv_memmgr_get_gpu_addr(fb_memory->mem) & SMU_INTERNAL_LOW_MASK) >>
			   SMU_INTERNAL_LOW_SHIFT);
	uint32_t mask_high =
		(uint32_t)((amdgv_memmgr_get_gpu_addr(fb_memory->mem) & SMU_INTERNAL_HIGH_MASK) >>
			   SMU_INTERNAL_HIGH_SHIFT);
	void *cpu_memory = amdgv_memmgr_get_cpu_addr(fb_memory->mem);

	AMDGV_ASSERT(fb_memory->mem);
	AMDGV_ASSERT(fb_memory->size != 0);
	AMDGV_ASSERT(system_memory != NULL);

	if ((system_memory != NULL) && (cpu_memory != NULL)) {
		ret = navi32_powerplay_send_msg_with_param(
			adapt, SMU_13_0_MSG__SET_DRIVER_DRAM_ADDR_HIGH, mask_high);

		if (ret == 0) {
			ret = navi32_powerplay_send_msg_with_param(
				adapt, SMU_13_0_MSG__SET_DRIVER_DRAM_ADDR_LOW, mask_low);
		}
		if (ret == 0) {
			ret = navi32_powerplay_send_msg_with_param(
				adapt, SMU_13_0_MSG__TRANSFER_TABLE_SMU2_DRAM, table_id);
		}
		oss_memcpy(system_memory, cpu_memory, fb_memory->size);
	}
	return ret;
}

static int navi32_powerplay_copy_table_to_smc(struct amdgv_adapter *adapt,
					     struct smu_local_memory *fb_memory,
					     void *system_memory, uint32_t table_id)
{
	int ret = AMDGV_FAILURE;
	uint32_t mask_low =
		(uint32_t)((amdgv_memmgr_get_gpu_addr(fb_memory->mem) & SMU_INTERNAL_LOW_MASK) >>
			   SMU_INTERNAL_LOW_SHIFT);
	uint32_t mask_high =
		(uint32_t)((amdgv_memmgr_get_gpu_addr(fb_memory->mem) & SMU_INTERNAL_HIGH_MASK) >>
			   SMU_INTERNAL_HIGH_SHIFT);
	void *cpu_memory = amdgv_memmgr_get_cpu_addr(fb_memory->mem);

	AMDGV_ASSERT(fb_memory->mem);
	AMDGV_ASSERT(fb_memory->size != 0);
	AMDGV_ASSERT(system_memory != NULL);

	if ((system_memory != NULL) && (cpu_memory != NULL)) {
		oss_memcpy(cpu_memory, system_memory, fb_memory->size);
		ret = navi32_powerplay_send_msg_with_param(
			adapt, SMU_13_0_MSG__SET_DRIVER_DRAM_ADDR_HIGH, mask_high);

		if (ret == 0) {
			ret = navi32_powerplay_send_msg_with_param(
				adapt, SMU_13_0_MSG__SET_DRIVER_DRAM_ADDR_LOW, mask_low);
		}
		if (ret == 0) {
			ret = navi32_powerplay_send_msg_with_param(
				adapt, SMU_13_0_MSG__TRANSFER_TABLE_DRAM2_SMU, table_id);
		}
	}

	return ret;
}

static int navi32_powerplay_update_smc_metrics(struct amdgv_adapter *adapt, uint32_t msg_arg)
{
	int ret = AMDGV_FAILURE;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_local_memory fb_memory;
	void *system_memory = NULL;
	uint16_t table_id = msg_arg & 0xffff;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (table_context != NULL) {
		switch (table_id) {
		case TABLE_SMU_METRICS:
			oss_memcpy(&fb_memory, &table_context->smc_metrics_table,
				   sizeof(struct smu_local_memory));
			system_memory = table_context->metrics_table;
			ret = 0;
			break;
		case TABLE_ECCINFO:
			oss_memcpy(&fb_memory, &table_context->smc_ecc_info_table,
				   sizeof(struct smu_local_memory));
			system_memory = table_context->ecc_info_table;
			ret = 0;
			break;
		default:
			AMDGV_ERROR(" SMU13 received wrong table ID\n");
			break;
		}

		if (ret != AMDGV_FAILURE)
			ret = navi32_powerplay_copy_table_from_smc(adapt, &fb_memory,
								  system_memory, msg_arg);
	}

	return ret;
}

static int navi32_powerplay_get_svi3_voltage(struct amdgv_adapter *adapt, enum navi32_voltage_type type, uint32_t *volt)
{
	int ret;

	switch (type) {
	case VDD_GFX:
		ret = navi32_powerplay_get_arg_with_param(
				adapt, SMU_13_0_MSG__GET_SVI3_VOLTAGE, NAVI32_VDD_GFX_BIT, volt);
		break;
	case VDD_SOC:
		ret = navi32_powerplay_get_arg_with_param(
				adapt, SMU_13_0_MSG__GET_SVI3_VOLTAGE, NAVI32_VDD_SOC_BIT, volt);
		break;
	case VDD_MEM:
		ret = navi32_powerplay_get_arg_with_param(
				adapt, SMU_13_0_MSG__GET_SVI3_VOLTAGE, NAVI32_VDD_MEM_BIT, volt);
		break;
	default:
		AMDGV_ERROR("Wrong voltage type\n");
		return -1;
	}

	return ret;
}

static int navi32_powerplay_get_pp_metrics(struct amdgv_adapter *adapt,
					  struct amdgv_gpumon_metrics *metrics)
{
	int ret = 0;
	struct smu_table_context *table_context = NULL;
	struct smu_13_0_0_powerplay_table *powerplay_table = NULL;
	SmuMetrics_t *metrics_table;
	struct smu_context *smu = NULL;
	uint32_t gfx_volt = 0;
	uint32_t soc_volt = 0;
	uint32_t mem_volt = 0;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (!table_context)
		return AMDGV_FAILURE;

	amdgv_gpumon_init_metrics_buf(metrics);

	metrics_table = (SmuMetrics_t *)table_context->metrics_table;
	powerplay_table = table_context->power_play_table;

	ret = navi32_powerplay_update_smc_metrics(adapt, TABLE_SMU_METRICS);

	if (!ret) {
		/* Update metrics */
		metrics->clocks[AMDGV_PP_CLK_GFX].curr = metrics_table->AverageGfxclkFrequencyPostDs;

		metrics->clocks[AMDGV_PP_CLK_GFX].avg_preDs =
			metrics_table->AverageGfxclkFrequencyPreDs;
		metrics->clocks[AMDGV_PP_CLK_GFX].avg_postDs =
			metrics_table->AverageGfxclkFrequencyPostDs;
		metrics->clocks[AMDGV_PP_CLK_GFX].avg =
			metrics_table->AverageGfxclkFrequencyPostDs;

		metrics->clocks[AMDGV_PP_CLK_SOC].curr = metrics_table->CurrClock[PPCLK_SOCCLK];
		metrics->clocks[AMDGV_PP_CLK_MEM].curr = metrics_table->CurrClock[PPCLK_UCLK];
		metrics->clocks[AMDGV_PP_CLK_GFX].ds_disabled = -1;
		metrics->clocks[AMDGV_PP_CLK_SOC].ds_disabled = -1;
		metrics->clocks[AMDGV_PP_CLK_MEM].ds_disabled = -1;

		//Compensate for 2x multiplication by SMU on AverageMemclkFrequency
		metrics->clocks[AMDGV_PP_CLK_MEM].avg_preDs =
			metrics_table->AverageMemclkFrequencyPreDs / 2;
		metrics->clocks[AMDGV_PP_CLK_MEM].avg_postDs =
			metrics_table->AverageMemclkFrequencyPostDs / 2;
		metrics->clocks[AMDGV_PP_CLK_MEM].avg =
			metrics_table->AverageMemclkFrequencyPostDs / 2;

		metrics->clocks[AMDGV_PP_CLK_DCLK_0].curr = metrics_table->CurrClock[PPCLK_DCLK_0];
		metrics->clocks[AMDGV_PP_CLK_DCLK_0].avg_preDs = 0;
		metrics->clocks[AMDGV_PP_CLK_DCLK_0].avg_postDs = 0;
		metrics->clocks[AMDGV_PP_CLK_DCLK_0].avg =
			metrics_table->AverageDclk0Frequency;
		metrics->clocks[AMDGV_PP_CLK_DCLK_0].ds_disabled = -1;

		metrics->clocks[AMDGV_PP_CLK_DCLK_1].curr = metrics_table->CurrClock[PPCLK_DCLK_1];
		metrics->clocks[AMDGV_PP_CLK_DCLK_1].avg_preDs = 0;
		metrics->clocks[AMDGV_PP_CLK_DCLK_1].avg_postDs = 0;
		metrics->clocks[AMDGV_PP_CLK_DCLK_1].avg =
			metrics_table->AverageDclk1Frequency;
		metrics->clocks[AMDGV_PP_CLK_DCLK_1].ds_disabled = -1;

		metrics->clocks[AMDGV_PP_CLK_VCLK_0].curr = metrics_table->CurrClock[PPCLK_VCLK_0];
		metrics->clocks[AMDGV_PP_CLK_VCLK_0].avg_preDs = 0;
		metrics->clocks[AMDGV_PP_CLK_VCLK_0].avg_postDs = 0;
		metrics->clocks[AMDGV_PP_CLK_VCLK_0].avg =
			metrics_table->AverageVclk0Frequency;
		metrics->clocks[AMDGV_PP_CLK_VCLK_0].ds_disabled = -1;

		metrics->clocks[AMDGV_PP_CLK_VCLK_1].curr = metrics_table->CurrClock[PPCLK_VCLK_1];
		metrics->clocks[AMDGV_PP_CLK_VCLK_1].avg_preDs = 0;
		metrics->clocks[AMDGV_PP_CLK_VCLK_1].avg_postDs = 0;
		metrics->clocks[AMDGV_PP_CLK_VCLK_1].avg =
			metrics_table->AverageVclk1Frequency;
		metrics->clocks[AMDGV_PP_CLK_VCLK_1].ds_disabled = -1;

		// AMDGV_PP_CLK_VID should report the same value as AMDGV_PP_CLK_VCLK_0,
		// allows backwards compatibility with older ASICs
		metrics->clocks[AMDGV_PP_CLK_VID].curr = metrics_table->AverageVclk0Frequency;
		metrics->clocks[AMDGV_PP_CLK_VID].avg = metrics_table->AverageVclk0Frequency;

		metrics->mem_usage = metrics_table->AverageUclkActivity;
		metrics->gfx_usage = metrics_table->AverageGfxActivity;
		metrics->mm_usage = metrics_table->Vcn0ActivityPercentage;

		metrics->mm_ip_usage[0] = metrics_table->Vcn0ActivityPercentage;
		metrics->mm_ip_usage[1] = metrics_table->Vcn1ActivityPercentage;

		metrics->fan_speed = metrics_table->AvgFanRpm;
		metrics->temp_edge = metrics_table->AvgTemperature[TEMP_EDGE];
		metrics->temp_hotspot = metrics_table->AvgTemperature[TEMP_HOTSPOT];
		metrics->temp_mem = metrics_table->AvgTemperature[TEMP_MEM];
		metrics->temp_plx = metrics_table->AvgTemperature[TEMP_PLX];

		metrics->power = metrics_table->AverageSocketPower;
		metrics->energy = metrics_table->EnergyAccumulator;

		metrics->temp_edge_limit = powerplay_table->smc_pptable.SkuTable.TemperatureLimit[TEMP_EDGE];
		metrics->temp_hotspot_limit = powerplay_table->smc_pptable.SkuTable.TemperatureLimit[TEMP_HOTSPOT];
		metrics->temp_mem_limit = powerplay_table->smc_pptable.SkuTable.TemperatureLimit[TEMP_MEM];
		metrics->power_limit = powerplay_table->smc_pptable.SkuTable.SocketPowerLimitAc[PPT_THROTTLER_PPT0];

		metrics->pcie_rate = metrics_table->PcieRate;
		metrics->pcie_width = metrics_table->PcieWidth;
	}

	// vid/volt coversion is: volt = 0.25f + 0.005f * (vid - 1), unit is V
	// smi_cmd.c need divided by 100 to get mV
	if (!navi32_powerplay_get_svi3_voltage(adapt, VDD_GFX, &gfx_volt))
		metrics->volt_gfx = 25000 + 500 * (gfx_volt - 1);

	if (!navi32_powerplay_get_svi3_voltage(adapt, VDD_SOC, &soc_volt))
		metrics->volt_soc = 25000 + 500 * (soc_volt - 1);

	if (!navi32_powerplay_get_svi3_voltage(adapt, VDD_MEM, &mem_volt))
		metrics->volt_mem = 25000 + 500 * (mem_volt - 1);

	return ret;
}

static int navi32_powerplay_write_smc_table(struct amdgv_adapter *adapt, uint32_t table_id)
{
	int ret = AMDGV_FAILURE;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_local_memory fb_memory;
	void *system_memory = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (table_context != NULL) {
		switch (table_id) {
		case TABLE_COMBO_PPTABLE:
			/* Only allowed when SCPM is disabled */
			if (!adapt->scpm_enabled) {
				oss_memcpy(&fb_memory, &table_context->smc_pptable,
					sizeof(struct smu_local_memory));
				system_memory = (char *) table_context->power_play_table;
				ret = 0;
			}
			break;
		case TABLE_PMFW_PPTABLE:
			/* Only allowed when SCPM is disabled */
			if (!adapt->scpm_enabled) {
				oss_memcpy(&fb_memory, &table_context->smc_pptable,
					sizeof(struct smu_local_memory));
				system_memory = &(((struct smu_13_0_0_powerplay_table *) (table_context->power_play_table))->smc_pptable);
				ret = 0;
			}
			break;
		case TABLE_DRIVER_SMU_CONFIG:
			oss_memcpy(&fb_memory, &table_context->smc_driver_smu_config_table,
				   sizeof(struct smu_local_memory));
			system_memory = table_context->config_table;
			ret = 0;
			break;
		case TABLE_ACTIVITY_MONITOR_COEFF:
			oss_memcpy(&fb_memory, &table_context->smc_activity_monitor_table,
				   sizeof(struct smu_local_memory));
			system_memory = table_context->activity_monitor_table;
			ret = 0;
			break;
		default:
			AMDGV_ERROR(" SMU13 received wrong table ID\n");
			break;
		}

		if (ret != AMDGV_FAILURE)
			ret = navi32_powerplay_copy_table_to_smc(adapt, &fb_memory,
								system_memory, table_id);
	}

	return ret;
}

static int navi32_powerplay_read_smc_table(struct amdgv_adapter *adapt, uint32_t table_id)
{
	int ret = AMDGV_FAILURE;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_local_memory fb_memory;
	void *system_memory = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (table_context != NULL) {
		switch (table_id & 0xffff) {
		case TABLE_COMBO_PPTABLE:
			oss_memcpy(&fb_memory, &table_context->smc_pptable,
					sizeof(struct smu_local_memory));
			system_memory = table_context->power_play_table;
			ret = 0;
			break;
		case TABLE_ACTIVITY_MONITOR_COEFF:
			oss_memcpy(&fb_memory, &table_context->smc_activity_monitor_table,
					sizeof(struct smu_local_memory));
			system_memory = table_context->activity_monitor_table;
			ret = 0;
			break;
		case TABLE_DRIVER_SMU_CONFIG:
			oss_memcpy(&fb_memory, &table_context->smc_driver_smu_config_table,
					sizeof(struct smu_local_memory));
			system_memory = table_context->config_table;
			ret = 0;
			break;
		default:
			AMDGV_ERROR(" SMU13 received wrong table ID\n");
			break;
		}

		if (ret != AMDGV_FAILURE)
			ret = navi32_powerplay_copy_table_from_smc(adapt, &fb_memory,
								system_memory, table_id);
	}

	return ret;
}

static int navi32_powerplay_set_driver_smu_config(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_table_context *table_context = NULL;
	struct smu_context *smu = NULL;
	DriverSmuConfig_t *config;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (!table_context)
		return AMDGV_FAILURE;

	config = (DriverSmuConfig_t *)table_context->config_table;
	// default is 2ms
	config->GfxclkAverageLpfTau = 50; // 50ms
	config->UclkAverageLpfTau = 50; // 50ms
	config->GfxActivityLpfTau = 50; // 50ms
	config->UclkActivityLpfTau = 50; // 50ms
	config->SocketPowerLpfTau = 100; // 100ms

	ret = navi32_powerplay_write_smc_table(adapt, TABLE_DRIVER_SMU_CONFIG);
	if (ret) {
		AMDGV_ERROR("Failed to copy smu config table to SMU!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_powerplay_set_tool_table_location(struct amdgv_adapter *adapt)
{
	int ret = AMDGV_FAILURE;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	uint64_t address = 0;
	uint32_t address_low = 0;
	uint32_t address_high = 0;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if ((table_context != NULL) && (table_context->smc_pm_status_log_table.size != 0)) {
		address =
			amdgv_memmgr_get_gpu_addr(table_context->smc_pm_status_log_table.mem);
		if (address == ~0)
			return ret;

		address_low = (uint32_t)((address & SMU_INTERNAL_LOW_MASK) >> SMU_INTERNAL_LOW_SHIFT);
		address_high =
			(uint32_t)((address & SMU_INTERNAL_HIGH_MASK) >> SMU_INTERNAL_HIGH_SHIFT);

		ret = navi32_powerplay_send_msg_with_param(
			adapt, SMU_13_0_MSG__SET_TOOLS_DRAM_ADDR_HIGH, address_high);

		if (ret == 0) {
			ret = navi32_powerplay_send_msg_with_param(
				adapt, SMU_13_0_MSG__SET_TOOLS_DRAM_ADDR_LOW, address_low);
		}
	}

	return ret;
}


/* for NV32, we need different time constant for 1vf vs multivf,
 * multivf needs a bigger timeconstant to slow down sampling rate
 * 1vf requires a faster sampling rate to accomadate short burst workload
 * in RoCM
 */
static int navi32_powerplay_set_custom_workload(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_table_context *table_context = NULL;
	DpmActivityMonitorCoeffInt_t *coeff;
	struct smu_context *smu = NULL;
	//int i = 0;
	uint32_t workload_mask;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (!table_context)
		return AMDGV_FAILURE;

	coeff = (DpmActivityMonitorCoeffInt_t *)table_context->activity_monitor_table;

	ret = navi32_powerplay_read_smc_table(adapt, (WORKLOAD_PPLIB_CUSTOM_BIT << 16) |
							    TABLE_ACTIVITY_MONITOR_COEFF);
	if (ret) {
		AMDGV_ERROR("failed to read compute activity table!\n");
		return AMDGV_FAILURE;
	}

/*
	coeff->Gfx_ActiveHystLimit = 0;
	coeff->Gfx_IdleHystLimit = 2;
	coeff->Gfx_MinActiveFreqType = 0;
	coeff->Gfx_MinActiveFreq = 1200;
	coeff->Gfx_BoosterFreqType = 4;
	coeff->Gfx_BoosterFreq = 0;
	coeff->Gfx_PD_Data_limit_c = 10 << 16;
	coeff->Gfx_PD_Data_error_coeff = 0xFFFFF333; // -0.05
	coeff->Gfx_PD_Data_error_rate_coeff = 0xFFFF0000; // -1

	coeff->Fclk_ActiveHystLimit = 0;
	coeff->Fclk_IdleHystLimit = 0;
	coeff->Fclk_MinActiveFreqType = 3;
	coeff->Fclk_MinActiveFreq = 0;
	coeff->Fclk_BoosterFreqType = 3;
	coeff->Fclk_BoosterFreq = 0;
	coeff->Fclk_PD_Data_limit_c = 20 << 16;
	coeff->Fclk_PD_Data_error_coeff = 0xFFFFE666; // -0.1
	coeff->Fclk_PD_Data_error_rate_coeff = 0xFFFFE666; // -0.1
	coeff->Fclk_PD_Data_time_constant = 0;

	coeff->Mem_UpThreshold_Limit[0] = 20 << 16;
	coeff->Mem_UpThreshold_Limit[1] = 5 << 16;
	coeff->Mem_UpThreshold_Limit[2] = 15 << 16;
	coeff->Mem_UpThreshold_Limit[3] = 30 << 16;

	for (i = 0; i < NUM_UCLK_DPM_LEVELS; i++) {
		coeff->Mem_UpHystLimit[i] = 0;
		coeff->Mem_DownHystLimit[i] = 1000;
	}
*/
	coeff->Gfx_PD_Data_time_constant = (adapt->num_vf == 1) ? 0xa : 0x64;
	ret = navi32_powerplay_write_smc_table(adapt, TABLE_ACTIVITY_MONITOR_COEFF);

	if (ret) {
		AMDGV_ERROR("failed to write custom activity table!\n");
		return AMDGV_FAILURE;
	}

	workload_mask  = 1 << WORKLOAD_PPLIB_CUSTOM_BIT;
	workload_mask |= 1 << WORKLOAD_PPLIB_POWER_SAVING_BIT;

	ret = navi32_powerplay_send_msg_with_param(adapt, SMU_13_0_MSG__SET_WORKLOAD_MASK,
						  workload_mask);

	if (ret) {
		AMDGV_ERROR("failed to set custom workload policy!\n");
		return AMDGV_FAILURE;
	}

	return ret;
}


static int navi32_powerplay_get_enabled_smu_features(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t features_low = 0;
	uint32_t features_high = 0;
	struct smu_context *smu = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);

	ret = navi32_powerplay_send_msg(adapt, SMU_13_0_MSG__GET_ENABLED_SMU_FEATURES_LOW);

	if (ret == 0)
		ret = navi32_powerplay_read_arg(adapt, &features_low);

	if (ret == 0) {
		ret = navi32_powerplay_send_msg(adapt,
					       SMU_13_0_MSG__GET_ENABLED_SMU_FEATURES_HIGH);

		if (ret == 0)
			ret = navi32_powerplay_read_arg(adapt, &features_high);
	}
	if (ret == 0)
		smu->features =
			((uint64_t)features_high << SMU_INTERNAL_HIGH_SHIFT) | features_low;

	return ret;
}


static int navi32_powerplay_smc_table_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (adapt->scpm_enabled)
		navi32_powerplay_read_smc_table(adapt, TABLE_COMBO_PPTABLE);
	else
		navi32_powerplay_read_pptable_from_vbios(adapt);

	ret = navi32_powerplay_check_pptable(adapt);
	if (ret) {
		AMDGV_ERROR("Failed at PP table check!\n");
		return AMDGV_FAILURE;
	}

	ret = navi32_powerplay_populate_smc_pptable(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to populate smc PP table!\n");
		return AMDGV_FAILURE;
	}

	ret = navi32_powerplay_check_fw_version(adapt);
	if (ret) {
		AMDGV_ERROR("Firmware version check failed!\n");
		return AMDGV_FAILURE;
	}

	ret = navi32_powerplay_set_tool_table_location(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to set tool table location!\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("smc i2c cmd table, gpu addr:0x%llx base:0x%llx offs0:0x%llx offs:0x%llx len:0x%llx\n",
		 amdgv_memmgr_get_gpu_addr(table_context->smc_i2c_table.mem),
		 table_context->smc_i2c_table.mem->memmgr->mc_base,
		 table_context->smc_i2c_table.mem->memmgr->offset,
		 table_context->smc_i2c_table.mem->alloc_off,
		 table_context->smc_i2c_table.mem->len);
	AMDGV_INFO("smc i2c cmd table, mmdown:%d size:0x%llx\n",
		 table_context->smc_i2c_table.mem->memmgr->down,
		 table_context->smc_i2c_table.mem->memmgr->size);

	return 0;
}

int navi32_enter_baco(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t reg_data = 0;
	uint32_t baco_enable_bit;
	int wait_ret;

	reg_data = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BACO_CNTL));
	if (reg_data & BACO_CNTL__BACO_MODE_MASK) {
		AMDGV_DEBUG("already entered BACO\n");
		return 0;
	}

	/* check if fused for BACO */
	reg_data = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_BIF_STRAP0));
	if (!(reg_data & RCC_BIF_STRAP0__STRAP_PX_CAPABLE_MASK)) {
		AMDGV_ERROR("BACO not supported! SMU strapped Default-Deny\n");
		return AMDGV_FAILURE;
	}

	/* check if BACO support is enabled */
	ret = navi32_powerplay_send_msg(adapt, SMU_13_0_MSG__GET_ENABLED_SMU_FEATURES_LOW);
	if (ret) {
		AMDGV_ERROR("Unable to request SMU features enabled\n");
		return AMDGV_FAILURE;
	}
	ret = navi32_powerplay_read_arg(adapt, &reg_data);
	if (ret) {
		AMDGV_ERROR("Unable to get SMU features enabled\n");
		return AMDGV_FAILURE;
	}
	baco_enable_bit = (uint32_t)0x1 << FEATURE_BACO_BIT;
	if ((reg_data & baco_enable_bit) == 0) {
		/* re-enable BACO */
		reg_data |= baco_enable_bit;
		ret = navi32_powerplay_send_msg_with_param(
			adapt, SMU_13_0_MSG__ENABLE_SMU_FEATURES_LOW, reg_data);
		if (ret) {
			AMDGV_ERROR("Unable to re-enable SMU BACO support\n");
			return AMDGV_FAILURE;
		}
	}

	AMDGV_DEBUG("entering BACO...\n");
	ret = navi32_powerplay_send_msg_with_param(adapt, SMU_13_0_MSG__ENTER_BACO, 0);
	if (ret) {
		AMDGV_ERROR("failed to message SMU to enter BACO\n");
		return AMDGV_FAILURE;
	}

	/* allow time for SMU to complete entering BACO */
	oss_msleep(100);

	/* Wait for SMU to enter BACO */
	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BACO_CNTL),
					   BACO_CNTL__BACO_MODE_MASK, 0,
					   AMDGV_TIMEOUT(TIMEOUT_RESET), AMDGV_WAIT_CHECK_NE, 0);

	if (wait_ret) {
		AMDGV_ERROR("TIMEOUT waiting SMU to enter BACO\n");
		return AMDGV_FAILURE;
	}

	AMDGV_DEBUG("enter BACO completed\n");
	return 0;
}

int navi32_exit_baco(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t reg_data = 0;
	int wait_ret;

	reg_data = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BACO_CNTL));
	if (!(reg_data & BACO_CNTL__BACO_MODE_MASK)) {
		AMDGV_ERROR("Attempt to re-exit BACO\n");
		return AMDGV_FAILURE;
	}

	AMDGV_DEBUG("exiting BACO...\n");
	ret = navi32_powerplay_send_msg(adapt, SMU_13_0_MSG__EXIT_BACO);
	if (ret) {
		AMDGV_ERROR("failed to message SMU to exit BACO\n");
		return AMDGV_FAILURE;
	}

	/* allow time for all blocks to complete RESET */
	oss_msleep(100);

	/* Wait for SMU to exit BACO */
	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BACO_CNTL),
					   BACO_CNTL__BACO_MODE_MASK, 0,
					   AMDGV_TIMEOUT(TIMEOUT_RESET), AMDGV_WAIT_CHECK_EQ, 0);

	if (wait_ret) {
		AMDGV_ERROR("TIMEOUT waiting SMU to exit BACO\n");
		return AMDGV_FAILURE;
	} else {
		AMDGV_DEBUG("exit BACO completed\n");
	}

	return ret;
}

int navi32_mode1_reset(struct amdgv_adapter *adapt)
{

	/* send mode1 reset command to SMU (MP1) */
	AMDGV_DEBUG("sending mode1_reset command to SMU ...\n");

	/*
	 On Navi3, we are using debug port for mode 1 reset:
	 1. Clear the response register.
	 2. Clear argument register.
	 3. Write msg to the dbg msg port.
	 4. Wait for some time for the bootloader.
	*/
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_54), 0);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_53), 0);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_75), DEBUGSMC_MSG_Mode1Reset);

	/* allow time for all blocks to complete RESET */
	oss_msleep(500);
	AMDGV_DEBUG("mode1_reset completed\n");

	return 0;
}

int navi32_wait_mode1_reset_completion(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t resp = 0;

	/* note that, C2PMSG_54 is debug port, it is different from msg port */
	ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_54),
					   MP1_C2PMSG_54__CONTENT_MASK, 0,
					   AMDGV_TIMEOUT(TIMEOUT_SMU_REG), AMDGV_WAIT_CHECK_NE, 0);

	/* read as return value */
	resp = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_54));

	/* Add the message to diagnosis data trace log */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_READ_RESP, ret, regMP1_SMN_C2PMSG_54,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_54)));

	if (ret == AMDGV_FAILURE || resp != PPSMC_Result_OK) {
		AMDGV_ERROR("Failed to send message 0x%x C2PMSG_54, response 0x%x\n",
			DEBUGSMC_MSG_Mode1Reset, resp);
		return AMDGV_FAILURE;
	}

	return 0;
}

int navi32_mode2_reset(struct amdgv_adapter *adapt)
{
	AMDGV_WARN("navi3 does not support mode2 reset\n");
	return AMDGV_FAILURE;
}

int navi32_powerplay_wait_idle(struct amdgv_adapter *adapt, int timeout)
{
	uint32_t param = 0xff00011;
	uint32_t rd;

	/* use PPSMC_MSG_TestMessage to check SMU alive
	 * the response will be the argument you pass + 1
	 * If SMU responds successfully, means SMU is IDLE
	 */
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90), 0);

	/* Set param, and add parameters start/end to diagnosis data */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_ARG_START, 0,
			regMP1_SMN_C2PMSG_82, param);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82), param);
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_ARG_END, 0, regMP1_SMN_C2PMSG_82,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82)));

	/* Set msg, and add parameters start/end to diagnosis data */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_MSG_START, 0, regMP1_SMN_C2PMSG_66,
		PPSMC_MSG_TestMessage);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_66), PPSMC_MSG_TestMessage);
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_MSG_END, 0, regMP1_SMN_C2PMSG_66,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_66)));

	if (navi32_powerplay_wait_for_response(adapt, &rd)) {
		AMDGV_WARN("TIMEOUT waiting for SMU response (SMU busy)\n");
		return 1; /* This may not be error! (depends on caller) */
	}
	rd = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82));

	/* Add the message to diagnosis data trace log */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_READ_ARG, 0, regMP1_SMN_C2PMSG_82, rd);

	if (rd != (param + 1)) {
		AMDGV_WARN("SMU responded (0x%08x) but expected (0x%08x)\n", rd, (param + 1));
		return 2; /* This may not be error! (depends on caller) */
	}
	return 0;
}

static int navi32_powerplay_smc_table_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int navi32_powerplay_update_smc_table(
	struct amdgv_adapter *adapt, void *table_data, uint32_t table_id)
{
	int ret = AMDGV_FAILURE;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_local_memory fb_memory;
	void *system_memory = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (table_context != NULL) {
		switch (table_id) {
		case TABLE_I2C_COMMANDS:
			oss_memcpy(&fb_memory,
				&table_context->smc_i2c_table,
				sizeof(struct smu_local_memory));
			oss_memcpy(table_context->i2c_table, table_data,
					sizeof(SwI2cRequest_t));
			system_memory = table_context->i2c_table;

			ret = navi32_powerplay_copy_table_to_smc(adapt, &fb_memory,
					system_memory, table_id);
			break;
		default:
			AMDGV_ERROR(" SMU13 received wrong table ID\n");
			break;
		}
	}

	return ret;
}

static void navi32_powerplay_fill_eeprom_i2c_req(SwI2cRequest_t *req, bool write,
						uint8_t address, uint8_t i2c_port,
						uint8_t *data, uint32_t numbytes)
{
	uint32_t i;

	/* numbytes should not exceed MAX_SW_I2C_COMMANDS */
	req->I2CcontrollerPort = i2c_port;
	req->I2CSpeed          = I2C_SPEED_FAST_400K;
	req->SlaveAddress      = address;
	req->NumCmds           = numbytes;

	for (i = 0; i < numbytes; i++) {
		SwI2cCmd_t *cmd = &req->SwI2cCmds[i];

		/* First 2 bytes are always write for lower 2b EEPROM address */
		if (i < 2)
			cmd->CmdConfig = CMDCONFIG_READWRITE_MASK;
		else
			cmd->CmdConfig = write ? CMDCONFIG_READWRITE_MASK : 0;

		/* Add RESTART for read  after address filled */
		cmd->CmdConfig |= (i == 2 && !write) ? CMDCONFIG_RESTART_MASK : 0;

		/* Add STOP in the end */
		cmd->CmdConfig |= (i == (numbytes - 1)) ? CMDCONFIG_STOP_MASK : 0;

		/* Fill with data regardless if read or write to simplify code */
		cmd->ReadWriteData = data[i];
	}
}

static int navi32_powerplay_i2c_eeprom_read_data(struct amdgv_adapter *adapt,
					       uint8_t address, uint8_t i2c_port,
					       uint8_t *data, uint32_t numbytes)
{
	uint32_t  i, ret = 0;
	SwI2cRequest_t req;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_local_memory *table = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	table = &(table_context->smc_i2c_table);

	oss_memset(&req, 0, sizeof(req));
	navi32_powerplay_fill_eeprom_i2c_req(&req, false,
				address, i2c_port,
				data, numbytes);

	/* Now read data starting with that address */
	ret = navi32_powerplay_update_smc_table(adapt, &req, TABLE_I2C_COMMANDS);
	if (!ret) {
		SwI2cRequest_t *res =
			(SwI2cRequest_t *)amdgv_memmgr_get_cpu_addr(table->mem);

		if (res == NULL) {
			AMDGV_ERROR("Unable to get SwI2cRequest !\n");
			return AMDGV_FAILURE;
		}
		/* Assume SMU  fills res.SwI2cCmds[i].Data with read bytes */
		for (i = 0; i < numbytes; i++)
			data[i] = res->SwI2cCmds[i].ReadWriteData;

		AMDGV_DEBUG("i2c_eeprom_read_data, address = %x, bytes = %d\n",
				  (uint16_t)address, numbytes);
	} else
		AMDGV_WARN("i2c_eeprom_read_data - error occurred :%x\n", ret);

	return ret;
}

static int navi32_powerplay_i2c_eeprom_write_data(struct amdgv_adapter *adapt,
					       uint8_t address, uint8_t i2c_port,
					       uint8_t *data, uint32_t numbytes)
{
	uint32_t ret;
	SwI2cRequest_t req;

	oss_memset(&req, 0, sizeof(req));
	navi32_powerplay_fill_eeprom_i2c_req(&req, true,
				address, i2c_port,
				data, numbytes);

	ret = navi32_powerplay_update_smc_table(adapt, &req, TABLE_I2C_COMMANDS);
	if (!ret) {
		AMDGV_DEBUG("i2c_write(), address = %x, bytes = %d , data: \n",
					 (uint16_t)address, numbytes);
		/*
		 * According to EEPROM spec there is a MAX of 10 ms required for
		 * EEPROM to flush internal RX buffer after STOP was issued at the
		 * end of write transaction. During this time the EEPROM will not be
		 * responsive to any more commands - so wait a bit more.
		 */
		oss_msleep(10);

	} else
		AMDGV_WARN("i2c_write- error occurred :%x\n", ret);

	return ret;
}

/**
 * AT24CM02 and M24M02-R have a 256-byte write page size.
 * Write the maximum amount of data, without
 * crossing the device's page boundary, as per
 * starting at any location within the page,
 * so long as the page boundary isn't crossed
 * over (actually the page pointer rolls over).
 */
static int navi32_powerplay_protect_i2c_eeprom_data_write(
		struct amdgv_adapter *adapt, uint8_t addr,
		uint8_t port, uint32_t next_eeprom_addr,
		uint8_t *data_ptr, uint32_t write_size)
{
	uint32_t remain_size = 0;
	uint8_t data_chunk[MAX_SW_I2C_COMMANDS] = { 0 };
	int ret = 0;

	/* calculate remain size of a 256-byte write page */
	remain_size = 256 - (next_eeprom_addr & 0xff);
	if (remain_size < write_size) {
		/* separate write command if crossing eeprom page boundary */
		oss_memcpy(data_chunk + 2, data_ptr, remain_size);
		data_chunk[0] = ((next_eeprom_addr >> 8) & 0xff);
		data_chunk[1] = (next_eeprom_addr & 0xff);
		ret = navi32_powerplay_i2c_eeprom_write_data(adapt,
				addr, port, data_chunk, remain_size + 2);
		if (ret)
			return ret;

		oss_memcpy(data_chunk + 2, data_ptr + remain_size, write_size - remain_size);
		data_chunk[0] = (((next_eeprom_addr + remain_size) >> 8) & 0xff);
		data_chunk[1] = ((next_eeprom_addr + remain_size) & 0xff);
		ret = navi32_powerplay_i2c_eeprom_write_data(adapt,
				addr, port, data_chunk, write_size - remain_size + 2);
	} else {
		oss_memcpy(data_chunk + 2, data_ptr, write_size);
		data_chunk[0] = ((next_eeprom_addr >> 8) & 0xff);
		data_chunk[1] = (next_eeprom_addr & 0xff);
		ret = navi32_powerplay_i2c_eeprom_write_data(adapt,
				addr, port, data_chunk, write_size + 2);
	}

	return ret;
}

static int navi32_powerplay_i2c_eeprom_i2c_xfer(struct amdgv_adapter *adapt, uint8_t port,
			struct i2c_msg *msgs, int num)
{
	uint32_t j, ret, data_size, data_chunk_size, next_eeprom_addr = 0;
	uint8_t *data_ptr, data_chunk[MAX_SW_I2C_COMMANDS] = { 0 };
	int i;

	for (i = 0; i < num; i++) {
		/*
		 * SMU interface allows at most MAX_SW_I2C_COMMANDS bytes of data at
		 * once and hence the data needs to be spliced into chunks and sent each
		 * chunk separately
		 */
		data_size = msgs[i].len - 2;
		data_chunk_size = MAX_SW_I2C_COMMANDS - 2;
		next_eeprom_addr = (msgs[i].buf[0] << 8 & 0xff00) | (msgs[i].buf[1] & 0xff);
		data_ptr = msgs[i].buf + 2;

		for (j = 0; j < data_size / data_chunk_size; j++) {
			/* Insert the EEPROM dest addess, bits 0-15 */
			data_chunk[0] = ((next_eeprom_addr >> 8) & 0xff);
			data_chunk[1] = (next_eeprom_addr & 0xff);

			if (msgs[i].flags & I2C_M_RD) {
				ret = navi32_powerplay_i2c_eeprom_read_data(adapt,
						(uint8_t)msgs[i].addr, port,
						data_chunk, MAX_SW_I2C_COMMANDS);

				oss_memcpy(data_ptr, data_chunk + 2, data_chunk_size);
			} else {
				ret = navi32_powerplay_protect_i2c_eeprom_data_write(adapt,
									(uint8_t)msgs[i].addr, port,
									next_eeprom_addr, data_ptr,
									data_chunk_size);
			}

			if (ret) {
				num = AMDGV_FAILURE;
				goto fail;
			}

			next_eeprom_addr += data_chunk_size;
			data_ptr += data_chunk_size;
		}

		if (data_size % data_chunk_size) {
			data_chunk[0] = ((next_eeprom_addr >> 8) & 0xff);
			data_chunk[1] = (next_eeprom_addr & 0xff);

			if (msgs[i].flags & I2C_M_RD) {
				ret = navi32_powerplay_i2c_eeprom_read_data(adapt,
						(uint8_t)msgs[i].addr, port,
						data_chunk, (data_size % data_chunk_size) + 2);

				oss_memcpy(data_ptr, data_chunk + 2, data_size % data_chunk_size);
			} else {
				ret = navi32_powerplay_protect_i2c_eeprom_data_write(adapt,
									(uint8_t)msgs[i].addr, port,
									next_eeprom_addr, data_ptr,
									data_size % data_chunk_size);
			}

			if (ret) {
				num = AMDGV_FAILURE;
				goto fail;
			}
		}
	}

fail:
	return num;
}

static int navi32_powerplay_get_ecc_info(struct amdgv_adapter *adapt,
					struct umc_ecc_info *eccinfo)
{

	int i, ret = 0;
	struct smu_table_context *table_context = NULL;
	EccInfoTable_t *ecc_table;
	struct smu_context *smu = NULL;
	struct ecc_info_per_ch  *ecc_info_per_channel = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (!table_context)
		return AMDGV_FAILURE;

	ecc_table = (EccInfoTable_t *)table_context->ecc_info_table;

	ret = navi32_powerplay_update_smc_metrics(adapt, TABLE_ECCINFO);

	if (!ret) {
		for (i = 0; i < NAVI32_UMC_CHANNEL_NUM; i++) {
			ecc_info_per_channel = &(eccinfo->ecc[i]);
			ecc_info_per_channel->ce_count_lo_chip =
				ecc_table->EccInfo[i].ce_count_lo_chip;
			ecc_info_per_channel->ce_count_hi_chip =
				ecc_table->EccInfo[i].ce_count_hi_chip;
			ecc_info_per_channel->mca_umc_status =
				ecc_table->EccInfo[i].mca_umc_status;
			ecc_info_per_channel->mca_umc_addr =
				ecc_table->EccInfo[i].mca_umc_addr;
		}
	}

	return ret;
}

static int navi32_parse_smu_table_info(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_powerplay_get_vbios_bootup_values(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to get VBIOS bootup values!\n");
		return AMDGV_FAILURE;
	}

	ret = navi32_powerplay_read_pptable_from_vbios(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to get PP table from VBIOS!\n");
		return AMDGV_FAILURE;
	}

	ret = navi32_powerplay_append_vbios_pptable(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to append VBIOS PP table!\n");
		return AMDGV_FAILURE;
	}

	ret = navi32_powerplay_populate_smc_pptable(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to populate smc PP table!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_powerplay_send_hbm_bad_channel_flag(struct amdgv_adapter *adapt, uint32_t bad_channel_bitmap)
{
	int ret = 0;

	/* message SMU to update the bad channel info on SMBUS */
	ret = navi32_powerplay_send_msg_with_param(adapt,
			SMU_13_0_MSG__SET_BAD_HBM_CHANNEL_FLAG, bad_channel_bitmap);
	if (ret) {
		AMDGV_ERROR("failed to message SMU to update bad channel bitmap\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static int navi32_powerplay_send_hbm_bad_page_num(struct amdgv_adapter *adapt, uint32_t size)
{
	int ret = 0;

	/* message SMU to update the bad page number on SMBUS */
	ret = navi32_powerplay_send_msg_with_param(adapt,
			SMU_13_0_MSG__SET_NUM_BAD_HBM_PAGES_RETIRED, size);
	if (ret) {
		AMDGV_ERROR("failed to message SMU to update HBM bad pages number\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static int navi32_powerplay_disallow_gfxoff(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_powerplay_send_msg(adapt, SMU_13_0_MSG__DISALLOW_GFX_OFF);
	if (ret) {
		AMDGV_ERROR("Failed to Disallow GFXOFF\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static void navi32_powerplay_enable_disp_timer2(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* enable disp timer2 */
	tmp = RREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_IH_CONTROL));
	tmp = REG_SET_FIELD(tmp, PWR_IH_CONTROL, DISP_TIMER2_TRIGGER_MASK, 0);
	tmp = REG_SET_FIELD(tmp, PWR_IH_CONTROL, PWR_IH_CLK_GATE_EN, 0);
	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_IH_CONTROL), tmp);
}

int navi32_powerplay_enable_smu_features(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_powerplay_send_msg_with_param(
					adapt, SMU_13_0_MSG__ENABLE_ALL_SMU_FEATURES, 0);
	if (ret) {
		AMDGV_ERROR("Failed to Enable all SMU Features\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int navi32_power_on_vcn(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_powerplay_send_msg_with_param(
					adapt, SMU_13_0_MSG__POWER_UP_VCN,
					SMU_13_VCN0 | SMU_13_KEEP_UVDS_TILE_ON);
	if (ret) {
		AMDGV_ERROR("Failed to power on VCN0\n");
		return AMDGV_FAILURE;
	}

	ret = navi32_powerplay_send_msg_with_param(
					adapt, SMU_13_0_MSG__POWER_UP_VCN, SMU_13_VCN1);
	if (ret) {
		AMDGV_ERROR("Failed to power on VCN1\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int navi32_power_down_vcn(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_powerplay_send_msg_with_param(
					adapt, SMU_13_0_MSG__POWER_DOWN_VCN, SMU_13_VCN0);
	if (ret) {
		AMDGV_ERROR("Failed to power down VCN0\n");
		return AMDGV_FAILURE;
	}

	ret = navi32_powerplay_send_msg_with_param(
					adapt, SMU_13_0_MSG__POWER_DOWN_VCN, SMU_13_VCN1);
	if (ret) {
		AMDGV_ERROR("Failed to power down VCN1\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}



static int navi32_enter_power_saving(struct amdgv_adapter *adapt)
{
	uint32_t reg_data = 0;

	/* check if fused for BACO */
	reg_data = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_BIF_STRAP0));
	if (!(reg_data & RCC_BIF_STRAP0__STRAP_PX_CAPABLE_MASK)) {
		AMDGV_ERROR("BACO not supported! SMU strapped Default-Deny\n");
		return AMDGV_FAILURE;
	}

	reg_data = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BACO_CNTL));
	if (reg_data & BACO_CNTL__BACO_MODE_MASK) {
		AMDGV_WARN("already entered BACO Power Saving\n");
		return 0;
	}

	return navi32_reset_enter_power_saving(adapt);
}

static int navi32_exit_power_saving(struct amdgv_adapter *adapt)
{
	uint32_t reg_data = 0;

	/* check if fused for BACO */
	reg_data = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_BIF_STRAP0));
	if (!(reg_data & RCC_BIF_STRAP0__STRAP_PX_CAPABLE_MASK)) {
		AMDGV_ERROR("BACO not supported! SMU strapped Default-Deny\n");
		return AMDGV_FAILURE;
	}

	reg_data = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BACO_CNTL));
	if (!(reg_data & BACO_CNTL__BACO_MODE_MASK)) {
		AMDGV_ERROR("Attempt to re-exit BACO Power Saving\n");
		return AMDGV_FAILURE;
	}

	return navi32_reset_exit_power_saving(adapt);
}

static int navi32_query_power_saving_status(struct amdgv_adapter *adapt,
	uint32_t *status)
{
	int ret = 0;
	uint32_t reg_data = 0;

	*status = PP_POWERSAVING_STATUS_MAX;

	/* check if fused for BACO */
	reg_data = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_BIF_STRAP0));
	if (!(reg_data & RCC_BIF_STRAP0__STRAP_PX_CAPABLE_MASK)) {
		AMDGV_WARN("Powersaving BACO feature not supported \n");
		return AMDGV_FAILURE;
	}

	reg_data = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BACO_CNTL));
	if (reg_data & BACO_CNTL__BACO_MODE_MASK)
		*status = PP_POWERSAVING_STATUS_IN_BACO;
	else
		*status = PP_POWERSAVING_STATUS_BACO_EXIT;

	return ret;
}

static int navi32_prepare_unload(struct amdgv_adapter *adapt)
{
	if (navi32_powerplay_send_msg_with_param(adapt, SMU_13_0_MSG__PREPARE_MP1_FOR_UNLOAD, SRIOV_GC_RESET_VCN_ROUTER)) {
		AMDGV_WARN("Fail to send unload to SMU\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_pp_smu_is_pm_enabled(struct amdgv_adapter *adapt, bool *pm_enabled)
{
	struct smu_context *smu = (struct smu_context *)(adapt->pp.smu_backend);

	if (smu->features) {
		*pm_enabled = true;
	} else {
		*pm_enabled = false;
	}

	return 0;
}

static int navi32_smu_pp_handle_irq(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	int ret = 0;
	uint32_t val;
	uint32_t ctx_id;
	uint32_t vf_flr_intr_sts;
	uint32_t i;

	if (entry->client_id != IH_IV_CLIENTID_MP1 ||
		entry->src_id != IH_INTERRUPT_ID_TO_DRIVER)
		return 0;

	/* ack irq first */
	val = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL));
	val = REG_SET_FIELD(val, MP1_SMN_IH_SW_INT_CTRL, INT_ACK, 1);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL), val);

	ctx_id = entry->src_data[0];
	switch (ctx_id) {
	case IH_INTERRUPT_VFFLR_INT:
		/* avoid the possible race condition that some VM is just destroyed within the short
		 * window when host driver enables flr strap in navi32_reset_vf_flr()
		 */
		if (!(adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY))
			break;

		/* Navi 32 FLR interrupt format:
		 * ctx[0] = IH_INTERRUPT_VFFLR_INT
		 * ctx[1] = BIF_PF0_VF_FLR_INTR_STS
		 */
		vf_flr_intr_sts = entry->src_data[1];

		/* Only for KVM, only trigger FLR if VF is active */
		for_each_id(i, vf_flr_intr_sts) {
			if (i >= adapt->num_vf)
				break;

			if (adapt->sched.array_vf[i].state == AMDGV_SCHED_ACTIVE) {
				ret = amdgv_sched_queue_event(
						adapt, i, AMDGV_EVENT_SCHED_FORCE_RESET_VF, AMDGV_SCHED_BLOCK_ALL);

				if (ret) {
					AMDGV_ERROR("Failed to trigger VFFLR for VF %d\n", i);
					ret = AMDGV_FAILURE;
					break;
				}
			}

			amdgv_sched_clear_dirty_vf_fb(adapt, i);
			amdgv_live_info_prepare_reset(adapt);
		}
		break;
	default:
		AMDGV_ERROR("navi32 smu can't process this context id %d\n", ctx_id);
		break;
	}

	return ret;
}


const struct amdgv_pp_funcs navi32_amdgv_pp_funcs = {
	.get_smu_fw_loaded_status = navi32_powerplay_get_fw_loaded_status,
	.wait_smu_idle = navi32_powerplay_wait_idle,
	.enter_baco = navi32_enter_baco,
	.exit_baco = navi32_exit_baco,
	.mode1_reset = navi32_mode1_reset,
	.mode2_reset = navi32_mode2_reset,
	.append_vbios_pptable = navi32_powerplay_append_vbios_pptable,
	.get_pp_metrics = navi32_powerplay_get_pp_metrics,
	.get_ecc_info = navi32_powerplay_get_ecc_info,
	.get_clock_limit = navi32_powerplay_get_clock_limit,
	.get_power_capacity = navi32_powerplay_get_power_capacity,
	.get_dpm_capacity = navi32_powerplay_get_dpm_level_count,
	.i2c_eeprom_xfer = navi32_powerplay_i2c_eeprom_i2c_xfer,
	.get_fru_product_info = navi32_fru_get_product_info,
	.send_hbm_bad_pages_num = navi32_powerplay_send_hbm_bad_page_num,
	.send_hbm_bad_channel_flag = navi32_powerplay_send_hbm_bad_channel_flag,
	.parse_smu_table_info = navi32_parse_smu_table_info,
	.prepare_unload = navi32_prepare_unload,
	.is_pm_enabled = navi32_pp_smu_is_pm_enabled,
	.handle_smu_irq = navi32_smu_pp_handle_irq,
	.enter_power_saving = navi32_enter_power_saving,
	.exit_power_saving = navi32_exit_power_saving,
	.query_power_saving_status = navi32_query_power_saving_status,
	.set_workload_profile = navi32_powerplay_set_custom_workload,
};

static int navi32_powerplay_sw_init(struct amdgv_adapter *adapt)
{
	amdgv_ras_eeprom_version_init(adapt);
	return 0;
}

static int navi32_powerplay_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t workload_mask;

	ret = navi32_powerplay_check_fw_status(adapt);

	if (ret) {
		AMDGV_ERROR("[navi32_powerplay_check_fw_status] Failed!\n");
		return ret;
	}

	ret = navi32_powerplay_set_driver_smu_config(adapt);
	if (ret) {
		AMDGV_ERROR("[navi32_powerplay_set_driver_smu_config] Failed!\n");
		return ret;
	}

	/* set to custom workload policy to support tuned FCLK/UCLK/GFXCLK DPM settings */
	workload_mask  = 1 << WORKLOAD_PPLIB_CUSTOM_BIT;

	/* Setting bit to enable 35mv soc guradband,
	 *  extra margin is needed for ROCM MSA */
	workload_mask |= 1 << WORKLOAD_PPLIB_POWER_SAVING_BIT;

	ret = navi32_powerplay_send_msg_with_param(adapt, SMU_13_0_MSG__SET_WORKLOAD_MASK, workload_mask);
	if (ret) {
		AMDGV_ERROR("Set workload policy failed!\n");
		return ret;
	}

	navi32_powerplay_get_enabled_smu_features(adapt);
	AMDGV_DEBUG("SMU_FEATURES = 0x%x\n", ((struct smu_context *)(adapt->pp.smu_backend))->features);

	return 0;
}

static int navi32_powerplay_hw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_powerplay_smc_table_hw_fini(adapt);
	if (ret) {
		AMDGV_ERROR("[navi32_powerplay_smc_table_hw_fini] Failed!\n");
		return ret;
	}

	return 0;
}

static int navi32_powerplay_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int navi32_smu_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = NULL;

	smu = oss_zalloc(sizeof(struct smu_context));
	if (smu == NULL) {
		AMDGV_ERROR("Failed to alloc memory for smu context\n");
		return AMDGV_FAILURE;
	}
	adapt->pp.smu_backend = smu;
	adapt->pp.smu_fw_version = SMU13_DRIVER_IF_VERSION;
	adapt->pp.pp_funcs = &navi32_amdgv_pp_funcs;

	ret = navi32_powerplay_smc_table_sw_init(adapt);

	return ret;
}

static int navi32_smu_sw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_powerplay_smc_table_sw_fini(adapt);
	if (ret) {
		AMDGV_ERROR("[navi32_powerplay_smc_table_sw_fini] Failed!\n");
		return ret;
	}

	return ret;
}

int navi32_powerplay_notify_no_dal(struct amdgv_adapter *adapt)
{
	int ret = 0;

#ifdef PPSMC_MSG_DALNotPresent
	ret = navi32_powerplay_send_msg(adapt, SMU_13_0_MSG__DAL_NOT_PRESENT);
	if (ret)
		AMDGV_ERROR("Failed to Notify SMU about no DAL environment\n");
#endif

	return ret;
}

static int navi32_smu_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = navi32_powerplay_check_fw_status(adapt);
	if (ret) {
		AMDGV_ERROR("[navi32_powerplay_check_fw_status] Failed!\n");
		return ret;
	}

	ret = navi32_powerplay_smc_table_hw_init(adapt);
	if (ret) {
		AMDGV_ERROR("[navi32_powerplay_smc_table_hw_init] Failed!\n");
		return AMDGV_FAILURE;
	}

	/* Poll on IMU Start before start HW init */
	ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(GC, 0, regGFX_IMU_MSG_FLAGS), 0x1, 0x1,
				AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_AUTO);
	if (ret) {
		AMDGV_ERROR("GFX_IMU_MSG_FLAGS=0x%x\n",
					RREG32(SOC15_REG_OFFSET(GC, 0, regGFX_IMU_MSG_FLAGS)));
		return PSP_STATUS__ERROR_GENERIC;
	}


	ret = navi32_powerplay_enable_smu_features(adapt);
	if (ret)
		return AMDGV_FAILURE;

	// PMFW will power gating timer interrupts, need to enable disp timer2 after smu features enabled
	navi32_powerplay_enable_disp_timer2(adapt);

	ret = navi32_powerplay_disallow_gfxoff(adapt);
	if (ret)
		return AMDGV_FAILURE;


	ret = navi32_powerplay_notify_no_dal(adapt);
	if (ret) {
		AMDGV_ERROR("[navi32_powerplay_notify_no_dal] Failed!\n");
		return ret;
	}

	return 0;
}

static int navi32_smu_hw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;
	if (0) {
		ret = navi32_powerplay_smc_table_hw_fini(adapt);
		if (ret) {
			AMDGV_ERROR("[navi32_powerplay_smc_table_hw_fini] Failed!\n");
			return ret;
		}
	}

	return 0;
}

int navi32_powerplay_ras_report(struct amdgv_adapter *adapt, int ras_type)
{
	AMDGV_INFO("RAS type 0x%x is reported to OOB\n", ras_type);
	return navi32_powerplay_send_msg_with_param(adapt, PPSMC_MSG_RAS_Alarms, ras_type);
}

const struct amdgv_init_func navi32_powerplay_func = {
	.name = "navi32_powerplay_func",
	.sw_init = navi32_powerplay_sw_init,
	.sw_fini = navi32_powerplay_sw_fini,
	.hw_init = navi32_powerplay_hw_init,
	.hw_fini = navi32_powerplay_hw_fini,
};

const struct amdgv_init_func navi32_smu_func = {
	.name = "navi32_smu_func",
	.sw_init = navi32_smu_sw_init,
	.sw_fini = navi32_smu_sw_fini,
	.hw_init = navi32_smu_hw_init,
	.hw_fini = navi32_smu_hw_fini,
};

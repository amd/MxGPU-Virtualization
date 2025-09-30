/*
 * Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
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


#include <amdgv_device.h>
#include "amdgv_vbios.h"
#include "atombios/atom.h"
#include "atombios/atomfirmware.h"
#include "amdgv_powerplay_swsmu.h"

#include "mi200.h"
#include "mi200_pptable.h"
#include "mi200_ppsmc_wrapper.h"
#include "mi200_swsmu.h"
#include "mi200_powerplay.h"

static const uint32_t this_block = AMDGV_POWER_BLOCK;

#define MI200_ADD_METRICS_EXT_ENTRY(_metrics, _category, _name, _unit, _value) \
	do { \
		_metrics->metric[_metrics->num_metric].code = 0; \
		_metrics->metric[_metrics->num_metric].category = (_category); \
		_metrics->metric[_metrics->num_metric].name = (_name); \
		_metrics->metric[_metrics->num_metric].unit = (_unit); \
		_metrics->metric[_metrics->num_metric].val = (_value); \
		_metrics->metric[_metrics->num_metric].vf_mask = ~0; \
		_metrics->num_metric++; \
	} while (0)

static const uint8_t mi200_smu_throttler_event_map[] = {
	[THROTTLER_PROCHOT_BIT] = PP_THROTTLER_EVENT__PROCHOT,
	[THROTTLER_THERMAL_SOCKET_BIT] = PP_THROTTLER_EVENT__SOCKET,
	[THROTTLER_THERMAL_HBM_BIT] = PP_THROTTLER_EVENT__HBM,
	[THROTTLER_THERMAL_VR_BIT] = PP_THROTTLER_EVENT__VR,
};

static int mi200_smu_13_0_send_msg_without_waiting(struct amdgv_adapter *adapt,
	uint16_t msg)
{
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_MSG_START, 0, regMP1_SMN_C2PMSG_66, msg);

	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_66), msg);

	/* Add the message to diagnosis data trace log */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_MSG_END, 0, regMP1_SMN_C2PMSG_66,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_66)));
	return 0;
}

static int mi200_smu_13_0_read_arg(struct amdgv_adapter *adapt, uint32_t *arg)
{
	*arg = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82));
	return 0;
}

static int mi200_smu_13_0_wait_for_response(struct amdgv_adapter *adapt,
					    uint32_t *val)
{
	int wait_ret;

	wait_ret = amdgv_wait_for_register(
	    adapt, SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90),
	    MP1_SMN_C2PMSG_90__CONTENT_MASK, 0, AMDGV_TIMEOUT(TIMEOUT_SMU_REG),
	    AMDGV_WAIT_CHECK_NE, 0);

	/* read as return value */
	*val = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90));

	/* Add the message to diagnosis data trace log */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_READ_RESP, wait_ret,
		regMP1_SMN_C2PMSG_90, *val);

	/* timeout means wrong logic */
	if (wait_ret) {
		AMDGV_ERROR("SMU TIMEOUT!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi200_smu_13_0_send_test_msg(struct amdgv_adapter *adapt)
{
	uint32_t param;
	uint32_t resp;

	param = 0xff00011; /* any value */
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90), 0);

/* Set param, and add parameters start/end to the diagnosis data */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_ARG_START, 0,
		regMP1_SMN_C2PMSG_82, param);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82), param);
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_ARG_END, 0, regMP1_SMN_C2PMSG_82,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82)));

	/* Set msg, and add parameters start/end to the diagnosis data */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_MSG_START, 0, regMP1_SMN_C2PMSG_66,
		PPSMC_MSG_TestMessage);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_66), PPSMC_MSG_TestMessage);
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_MSG_END, 0, regMP1_SMN_C2PMSG_66,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_66)));

	if (mi200_smu_13_0_wait_for_response(adapt, &resp) == AMDGV_FAILURE) {
		AMDGV_ERROR("TIMEOUT waiting for SMU response\n");
		return AMDGV_FAILURE; /* SMU not ready */
	}
	/* the response will be the argument you pass + 1 */
	resp = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82));
	/* Add the message to diagnosis data trace log */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_READ_ARG, 0, regMP1_SMN_C2PMSG_82, resp);
	if (resp != (param + 1)) {
		AMDGV_ERROR("SMU responded (0x%08x) but expected (0x%08x)\n", resp, (param + 1));
		return AMDGV_FAILURE; /* SMU did not respond as expected */
	}

	return 0;
}

static int mi200_smu_13_0_send_msg(struct amdgv_adapter *adapt, uint16_t msg)
{
	int ret = 0;
	uint32_t resp = 0;

	if (mi200_smu_13_0_wait_for_response(adapt, &resp) != 0)
		if (mi200_smu_13_0_send_test_msg(adapt) != 0)
			return AMDGV_FAILURE;

	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90), 0);

	mi200_smu_13_0_send_msg_without_waiting(adapt, msg);

	ret = mi200_smu_13_0_wait_for_response(adapt, &resp);
	if (ret == AMDGV_FAILURE || resp != PPSMC_Result_OK) {
		AMDGV_ERROR("Failed to send message 0x%x, response 0x%x\n", msg,
			resp);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi200_smu_13_0_send_msg_with_param(struct amdgv_adapter *adapt,
	uint16_t msg, uint32_t param)
{
	int ret = 0;
	uint32_t resp = 0;

	if (mi200_smu_13_0_wait_for_response(adapt, &resp) != 0)
		if (mi200_smu_13_0_send_test_msg(adapt) != 0)
			return AMDGV_FAILURE;

	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_ARG_START, 0,
		regMP1_SMN_C2PMSG_82, param);

	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90), 0);

	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82), param);

	/* Add the message to diagnosis data trace log */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_ARG_END, 0, regMP1_SMN_C2PMSG_82,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82)));

	mi200_smu_13_0_send_msg_without_waiting(adapt, msg);

	ret = mi200_smu_13_0_wait_for_response(adapt, &resp);
	if (ret == AMDGV_FAILURE || resp != PPSMC_Result_OK) {
		AMDGV_ERROR("Failed to send message 0x%x, response 0x%x\n", msg,
			resp);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi200_smu_13_0_get_arg_with_param(struct amdgv_adapter *adapt,
		uint16_t msg, uint32_t param, uint32_t *output)
{
	int ret = mi200_smu_13_0_send_msg_with_param(adapt, msg, param);
	if (ret)
		return ret;

	return mi200_smu_13_0_read_arg(adapt, output);
}

static int mi200_smu_13_0_get_dpm_clock_value(struct amdgv_adapter *adapt, PPCLK_e clk,
					      uint16_t level, uint32_t *value)
{
	uint32_t param = ((uint32_t) (clk << 16)) | level;

	return mi200_smu_13_0_get_arg_with_param(adapt, SMU_13_0_MSG__GET_DPM_FREQ_BY_INDEX,
						 param, value);
}

static int mi200_smu_13_0_get_pcie_info(struct amdgv_adapter *adapt, uint8_t *pcie_link_speed, uint8_t *pcie_link_width)
{
	int pos;
	uint16_t val;

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID_EXP);
	if (!pos) {
		AMDGV_ERROR("this device does not support capability: %x\n", PCI_CAP_ID_EXP);
		return AMDGV_FAILURE;
	}

	oss_pci_read_config_word(adapt->dev, pos + PCI_EXP_LNKSTA, &val);
	*pcie_link_width = (val & PCI_EXP_LNKSTA_NLW) >> 4;
	*pcie_link_speed = val & PCI_EXP_LNKSTA_CLS;

	return 0;
}

static int mi200_smu_13_0_get_dpm_level_count(struct amdgv_adapter *adapt, PPCLK_e clk,
					      uint32_t *count)
{
	return mi200_smu_13_0_get_dpm_clock_value(adapt, clk, 0xff, count);
}

static int mi200_smu_13_0_get_dpm_level_range(struct amdgv_adapter *adapt, PPCLK_e clk,
					      uint32_t *min, uint32_t *max)
{
	int ret;
	uint32_t level_count = 0;

	if (min) {
		ret = mi200_smu_13_0_get_dpm_clock_value(adapt, clk, 0, min);
		if (ret)
			return ret;
	}

	if (max) {
		ret = mi200_smu_13_0_get_dpm_level_count(adapt, clk, &level_count);
		if (ret)
			return ret;

		ret = mi200_smu_13_0_get_dpm_clock_value(adapt, clk, level_count - 1, max);
		if (ret)
			return ret;
	}

	return 0;
}

static int mi200_smu_13_0_get_power_capacity(struct amdgv_adapter *adapt,
					      int *val)
{
	return mi200_smu_13_0_get_arg_with_param(adapt,
						 SMU_13_0_MSG__GET_PPT_LIMIT,
						 0,
						 val);
}

static int mi200_smu_13_0_get_gfx_dpm_level_count(struct amdgv_adapter *adapt,
						  int *val)
{
	/*
	 * MI200 GFX doesn't support discrete DPM levels.
	 * There's 2 levels, Fmin and Fmax
	 */

	if (val)
		*val = 2;

	return 0;
}

static int mi200_smu_13_0_initialize_dpm_context(struct amdgv_adapter *adapt)
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

static int mi200_smu_13_0_destroy_dpm_context(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_13_0_dpm_context *dpm_ctxt = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	dpm_ctxt = (struct smu_13_0_dpm_context *)(smu->smu_dpm_context);

	if (dpm_ctxt)
		oss_free(dpm_ctxt);

	return 0;
}

static int mi200_smu_13_0_destroy_smc_table(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (table_context->driver_pptable)
		oss_free(table_context->driver_pptable);
	if (table_context->ppt_information)
		oss_free(table_context->ppt_information);
	if (table_context->metrics_table)
		oss_free(table_context->metrics_table);
	if (table_context->i2c_table)
		oss_free(table_context->i2c_table);
	if (table_context->ecc_info_table)
		oss_free(table_context->ecc_info_table);

	oss_memset(&table_context->smc_pptable,
		0, sizeof(struct smu_local_memory));
	oss_memset(&table_context->smc_metrics_table,
		0, sizeof(struct smu_local_memory));
	oss_memset(&table_context->smc_i2c_table,
		0, sizeof(struct smu_local_memory));
	oss_memset(&table_context->smc_pm_status_log_table,
		0, sizeof(struct smu_local_memory));
	oss_memset(&table_context->ecc_info_table,
		0, sizeof(struct smu_local_memory));

	return ret;
}

static int mi200_smu_initialize_pptable(struct amdgv_adapter *adapt)
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

	table_context->power_play_table = oss_zalloc(sizeof(struct smu_13_0_powerplay_table));
	if (table_context->power_play_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for pp tb\n");
		return AMDGV_FAILURE;
	}
	smu->smu_table_context = table_context;

	return ret;
}

static int mi200_smu_13_0_initialize_smc_tables(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	table_context->driver_pptable = oss_zalloc(sizeof(PPTable_t));
	if (table_context->driver_pptable == NULL) {
		AMDGV_ERROR("Failed to alloc memory for driver pp tb\n");
		return AMDGV_FAILURE;
	}
	table_context->ppt_information = oss_zalloc(
		sizeof(struct smu_13_0_ppt_information));
	if (table_context->ppt_information == NULL) {
		AMDGV_ERROR("Failed to alloc memory for ppt info\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}
	table_context->metrics_table = oss_zalloc(sizeof(SmuMetrics_t));
	if (table_context->metrics_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for smu metric tb\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}
	table_context->i2c_table = oss_zalloc(sizeof(SwI2cRequest_t));
	if (table_context->i2c_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for i2c tb\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}

	table_context->ecc_info_table =	oss_zalloc(sizeof(EccInfoTable_t));
	if (table_context->ecc_info_table == NULL) {
		AMDGV_ERROR("Failed to alloc memory for smu ecc info tb\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}

	/* FB memory will be allocated in init_fb_allo func, fill in size here */
	table_context->smc_pptable.alignment = PAGE_SIZE;
	table_context->smc_pptable.size = sizeof(PPTable_t);

	table_context->smc_metrics_table.alignment = PAGE_SIZE;
	table_context->smc_metrics_table.size = sizeof(SmuMetrics_t);

	table_context->smc_i2c_table.alignment = PAGE_SIZE;
	table_context->smc_i2c_table.size = sizeof(SwI2cRequest_t);

	/* allocate space for tools table */
	table_context->smc_pm_status_log_table.alignment = PAGE_SIZE;
	table_context->smc_pm_status_log_table.size = TOOL_SIZE;

	table_context->smc_ecc_info_table.alignment = PAGE_SIZE;
	table_context->smc_ecc_info_table.size = sizeof(EccInfoTable_t);

	ret = mi200_smu_13_0_initialize_dpm_context(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to allocate memory for dpm context!\n");
		ret = AMDGV_FAILURE;
		goto fail_free;
	}

	goto out;

fail_free:
	mi200_smu_13_0_destroy_smc_table(adapt);

out:
	return ret;
}


static int mi200_smu_destroy_pptable(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (table_context && table_context->power_play_table)
		oss_free(table_context->power_play_table);
	if (table_context)
		oss_free(table_context);

	return ret;
}

static int mi200_smu_13_0_init_fb_allocations(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	/* allocate space for pptable */
	table_context->smc_pptable.mem = amdgv_memmgr_alloc_align(
		&adapt->memmgr_pf,
		table_context->smc_pptable.size,
		table_context->smc_pptable.alignment, MEM_SMC_PPTABLE);
	if (!table_context->smc_pptable.mem) {
		mi200_smu_13_0_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for pp table!\n");
		return AMDGV_FAILURE;
	}

	/* allocate space for metrics table */
	table_context->smc_metrics_table.mem = amdgv_memmgr_alloc_align(
		&adapt->memmgr_pf,
		table_context->smc_metrics_table.size,
		table_context->smc_metrics_table.alignment,
		MEM_SMC_METRICS_TABLE);
	if (!table_context->smc_metrics_table.mem) {
		mi200_smu_13_0_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for metrics table!\n");
		return AMDGV_FAILURE;
	}

	/* allocate space for I2C command table */
	table_context->smc_i2c_table.mem = amdgv_memmgr_alloc_align(
		&adapt->memmgr_pf,
		table_context->smc_i2c_table.size,
		table_context->smc_i2c_table.alignment, MEM_SMC_I2C_TABLE);
	if (!table_context->smc_i2c_table.mem) {
		mi200_smu_13_0_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for I2c table!\n");
		return AMDGV_FAILURE;
	}

	/* allocate space for tools table */
	table_context->smc_pm_status_log_table.mem = amdgv_memmgr_alloc_align(
		&adapt->memmgr_pf,
		table_context->smc_pm_status_log_table.size,
		table_context->smc_pm_status_log_table.alignment,
		MEM_SMC_PM_STATUS_TABLE);
	if (!table_context->smc_pm_status_log_table.mem) {
		mi200_smu_13_0_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for tools table!\n");
		return AMDGV_FAILURE;
	}

	/* allocate space for ecc info table */
	table_context->smc_ecc_info_table.mem =	amdgv_memmgr_alloc_align(
		&adapt->memmgr_pf,
		table_context->smc_ecc_info_table.size,
		table_context->smc_ecc_info_table.alignment,
		MEM_SMU_ECC_INFO_TABLE);
	if (!table_context->smc_ecc_info_table.mem) {
		mi200_smu_13_0_destroy_smc_table(adapt);
		AMDGV_ERROR("Failed to allocate memory for ecc info table!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi200_smu_13_0_release_fb_allocations(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	/* free space for pptable */
	if (table_context->smc_pptable.mem != NULL)
		amdgv_memmgr_free(table_context->smc_pptable.mem);

	/* free space for metrics table */
	if (table_context->smc_metrics_table.mem != NULL)
		amdgv_memmgr_free(table_context->smc_metrics_table.mem);

	/* free space for I2C command table */
	if (table_context->smc_i2c_table.mem != NULL)
		amdgv_memmgr_free(table_context->smc_i2c_table.mem);

	/* free space for tools table */
	if (table_context->smc_pm_status_log_table.mem != NULL)
		amdgv_memmgr_free(table_context->smc_pm_status_log_table.mem);

	/* free space for ecc info table */
	if (table_context->smc_ecc_info_table.mem != NULL)
		amdgv_memmgr_free(table_context->smc_ecc_info_table.mem);

	return 0;
}


static int mi200_smu_13_0_smc_table_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = mi200_smu_initialize_pptable(adapt);

	if (ret) {
		AMDGV_ERROR("[mi200_smu_initialize_pptable] Failed!\n");
		return ret;
	}

	ret = mi200_smu_13_0_initialize_smc_tables(adapt);

	if (ret) {
		AMDGV_ERROR("[mi200_smu_13_0_initialize_smc_tables]Failed!\n");
		return ret;
	}

	ret = mi200_smu_13_0_init_fb_allocations(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to alloc tables in fb!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi200_smu_13_0_smc_table_sw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = (struct smu_context *)(adapt->pp.smu_backend);

	ret = mi200_smu_13_0_release_fb_allocations(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to free tables in fb!\n");
		return AMDGV_FAILURE;
	}

	ret = mi200_smu_13_0_destroy_smc_table(adapt);

	if (ret) {
		AMDGV_ERROR("[mi200_smu_13_0_destroy_smc_table] Failed!\n");
		return ret;
	}

	ret = mi200_smu_13_0_destroy_dpm_context(adapt);

	if (ret) {
		AMDGV_ERROR("[mi200_smu_13_0_destroy_dpm_context] Failed!\n");
		return ret;
	}

	ret = mi200_smu_destroy_pptable(adapt);

	if (ret) {
		AMDGV_ERROR("[mi200_smu_destroy_pptable] Failed!\n");
		return ret;
	}

	oss_free(smu);

	return ret;
}

int mi200_smu_13_0_get_fw_loaded_status(struct amdgv_adapter *adapt)
{
	uint32_t mp1_flags;
	uint32_t mp1_intr_en;

	WREG32(SOC15_REG_OFFSET(NBIO, 0, mmPCIE_INDEX2),
	       (MP1_Public | (smnMP1_FIRMWARE_FLAGS & 0xffffffff)));
	mp1_flags = RREG32(SOC15_REG_OFFSET(NBIO, 0, mmPCIE_DATA2));

	mp1_intr_en = REG_GET_FIELD(mp1_flags, MP1_FIRMWARE_FLAGS, INTERRUPTS_ENABLED);
	AMDGV_DEBUG("SMU (status=0x%x intr_en=%d)\n", mp1_flags, mp1_intr_en);

	if (!mp1_intr_en)
		return 0; /* MP1 not enabled */

	 /* Send a test message to check status of SMU main FW */
	if (mi200_smu_13_0_send_test_msg(adapt) != 0)
		return 0; /* SMU main FW is not ready or responding */
	else
		return 1; /* SMU main FW ready and responding */
}

static int mi200_smu_13_0_wait_for_fw_loaded(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int i = 0;

	for (i = 0; i < AMD_MAX_USEC_TIMEOUT; i++) {
		ret = mi200_smu_13_0_get_fw_loaded_status(adapt);
		if (ret)
			break;
		oss_udelay(1);
	}
	if (i == AMD_MAX_USEC_TIMEOUT)
		return AMDGV_FAILURE;

	return 0;

}

static int mi200_smu_13_0_check_fw_status(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = mi200_smu_13_0_wait_for_fw_loaded(adapt);
	if (ret)
		AMDGV_ERROR("[mi200_smu_13_0_wait_for_fw_loaded] Failed!\n");
	else
		/* to avoid gim wait too long and timeout before we
		 * send first smu message, since SMU does not init all the
		 * response registers to 1.
		 */
		WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90), 1);

	return ret;
}

static int mi200_smu_13_0_get_vbios_bootup_values(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int index = 0;
	uint16_t size = 0;
	uint8_t frev = 0;
	uint8_t crev = 0;
	struct atom_common_table_header *header;
	struct atom_firmware_info_v3_3 *v_3_3 = NULL;
	struct atom_firmware_info_v3_1 *v_3_1 = NULL;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_bios_boot_up_values *boot_values;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	boot_values = &table_context->boot_values;

	index = get_index_into_master_table(
		atom_master_list_of_data_tables_v2_1,
		firmwareinfo);
	ret = smu_get_atom_data_table(adapt,
			index, &size, &frev, &crev, (uint8_t **)&header);

	if (ret) {
		AMDGV_ERROR("Failed to get table from VBIOS!\n");
		return ret;
	}
	if (header->format_revision != 3) {
		AMDGV_ERROR(
			"unknowned atom_firmware_info_version for smu13!\n");
		return AMDGV_FAILURE;
	}

	if (header->format_revision == 3 && header->content_revision >= 3) {
		v_3_3 = (struct atom_firmware_info_v3_3 *)header;
		boot_values->revision = v_3_3->firmware_revision;
		boot_values->gfxclk = v_3_3->bootup_sclk_in10khz;
		boot_values->uclk = v_3_3->bootup_mclk_in10khz;
		boot_values->socclk = 0;
		boot_values->dcefclk = 0;
		boot_values->vddc = v_3_3->bootup_vddc_mv;
		boot_values->vddci = v_3_3->bootup_vddci_mv;
		boot_values->mvddc = v_3_3->bootup_mvddc_mv;
		boot_values->vdd_gfx = v_3_3->bootup_vddgfx_mv;
		boot_values->cooling_id = v_3_3->coolingsolution_id;
		boot_values->pp_table_id = v_3_3->pplib_pptable_id;
	} else {
		v_3_1 = (struct atom_firmware_info_v3_1 *)header;
		boot_values->revision = v_3_1->firmware_revision;
		boot_values->gfxclk = v_3_1->bootup_sclk_in10khz;
		boot_values->uclk = v_3_1->bootup_mclk_in10khz;
		boot_values->socclk = 0;
		boot_values->dcefclk = 0;
		boot_values->vddc = v_3_1->bootup_vddc_mv;
		boot_values->vddci = v_3_1->bootup_vddci_mv;
		boot_values->mvddc = v_3_1->bootup_mvddc_mv;
		boot_values->vdd_gfx = v_3_1->bootup_vddgfx_mv;
		boot_values->cooling_id = v_3_1->coolingsolution_id;
		boot_values->pp_table_id = 0;
	}
	return 0;
}

static int mi200_smu_v13_0_atom_get_smu_clockinfo(struct amdgv_adapter *adapt,
						  uint8_t clk_id,
						  uint8_t syspll_id,
						  uint32_t *clk_freq)
{
	struct atom_context *ctx = adapt->vbios.atom_context;
	struct atom_get_smu_clock_info_parameters_v3_1 input = {0};
	struct atom_get_smu_clock_info_output_parameters_v3_1 *output;
	int ret, index;

	input.clk_id = clk_id;
	input.syspll_id = syspll_id;
	input.command = GET_SMU_CLOCK_INFO_V3_1_GET_CLOCK_FREQ;
	index = get_index_into_master_table(atom_master_list_of_command_functions_v2_1,
					    getsmuclockinfo);

	ret = amdgv_atom_execute_table(ctx, index, (uint32_t *)&input);
	if (ret) {
		AMDGV_ERROR("amdgv_atom_execute_table() failed!\n");
		return AMDGV_FAILURE;
	}

	output = (struct atom_get_smu_clock_info_output_parameters_v3_1 *)&input;
	*clk_freq = le32_to_cpu(output->atom_smu_outputclkfreq.smu_clock_freq_hz) / 10000;

	return 0;
}

static int mi200_smu_13_0_read_pptable_from_vbios(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int index = 0;
	uint16_t size = 0;
	uint8_t frev = 0;
	uint8_t crev = 0;
	void *table = NULL;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	unsigned char *soft_tb = NULL;
	uint32_t soft_tb_size = 0;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	AMDGV_INFO("VBIOS PPLIB_PPTABLE_ID = %d\n",
		table_context->boot_values.pp_table_id);

	table_context->boot_values.pp_table_id = 0;
	AMDGV_INFO("overwrite pptable id to 0, to force using vbios's pptable\n");

	if (table_context->boot_values.pp_table_id > 0) {
		//use hard-coded internal pp table
		switch (table_context->boot_values.pp_table_id) {
		case 2524:
		default:
			soft_tb_size = sizeof(softPowerPlayTable2524);
			soft_tb = softPowerPlayTable2524;
			break;
		}
		table_context->power_play_table = oss_zalloc(
			soft_tb_size);
		if (table_context->power_play_table == NULL) {
			AMDGV_ERROR("Failed to alloc memory for soft pp tb\n");
			return AMDGV_FAILURE;
		}
		oss_memcpy(table_context->power_play_table,
			soft_tb, soft_tb_size);
		table_context->power_play_table_size = sizeof
			(soft_tb_size);
	} else {
		index = get_index_into_master_table(
			atom_master_list_of_data_tables_v2_1,
			powerplayinfo);
		ret = smu_get_atom_data_table(
			adapt, index, &size, &frev, &crev,
			(uint8_t **)&table);
		if (ret)
			return ret;

		if (size > sizeof(struct smu_13_0_powerplay_table)) {
			AMDGV_ERROR("PowerPlayTable size of %u exceed maximum %u\n", size,
				    sizeof(struct smu_13_0_powerplay_table));
			return AMDGV_FAILURE;
		}
		oss_memcpy(table_context->power_play_table, table, size);
		table_context->power_play_table_size = size;
	}

	return 0;
}

static int mi200_smu_13_0_get_clk_info_from_vbios(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_bios_boot_up_values *boot_values;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	boot_values = &table_context->boot_values;

	ret = mi200_smu_v13_0_atom_get_smu_clockinfo(adapt, SMU11_SYSPLL0_SOCCLK_ID,
						     0,  &boot_values->socclk);
	if (ret) {
		AMDGV_ERROR("get boot_values socclk failed!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi200_smu_13_0_append_vbios_pptable(struct amdgv_adapter *adapt)
{
	int index = 0;
	uint16_t size = 0;
	uint8_t frev = 0;
	uint8_t crev = 0;
	uint16_t data_offset = 0;
	struct atom_context *ctx = adapt->vbios.atom_context;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_13_0_powerplay_table *powerplay_table = NULL;
	struct atom_smc_dpm_info_v4_10 *dpm = NULL;
	PPTable_t *pptb = NULL;
	bool ret = 0;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	powerplay_table = table_context->power_play_table;
	pptb = &powerplay_table->smc_pptable;

	index = get_index_into_master_table(
		atom_master_list_of_data_tables_v2_1,
		smc_dpm_info);
	ret = amdgv_atom_parse_data_header(ctx, index, &size, &frev, &crev, &data_offset);
	if (ret)
		dpm = (struct atom_smc_dpm_info_v4_10 *)((uint8_t *)ctx->bios + data_offset);
	if (!dpm) {
		AMDGV_ERROR("failed to get smc dpm info from vbios!\n");
		return AMDGV_FAILURE;
	}

	if (frev == 4 && crev == 10) {
		oss_memcpy(&pptb->GfxMaxCurrent, &dpm->GfxMaxCurrent,
			   sizeof(*dpm) - offsetof(struct atom_smc_dpm_info_v4_10, GfxMaxCurrent));
	} else {
		AMDGV_ERROR("unsupport atom_smc_dpm_info version %d.%d\n", frev, crev);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi200_smu_13_0_check_pptable(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_13_0_powerplay_table *powerplay_table = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (table_context != NULL)
		powerplay_table = table_context->power_play_table;

	if (table_context != NULL && powerplay_table != NULL) {
		if (SMU_13_0_TABLE_FORMAT_REVISION >
		    powerplay_table->header.format_revision) {
			AMDGV_ERROR("Unsupported PP table format!\n");
			return AMDGV_FAILURE;
		}
		if (powerplay_table->header.structuresize == 0) {
			AMDGV_ERROR("Invalid PP table!\n");
			return AMDGV_FAILURE;
		}
	} else {
		AMDGV_ERROR("Unable to get PP table!\n");
		return AMDGV_FAILURE;
	}
	return 0;
}

static int mi200_smu_13_0_overwrite_pptable(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_13_0_powerplay_table *powerplay_table = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (table_context != NULL)
		powerplay_table = table_context->power_play_table;

	if (table_context != NULL && powerplay_table != NULL) {
		powerplay_table->smc_pptable.FeaturesToRun[0] &=
			adapt->pp.smu_features_mask[0];
		powerplay_table->smc_pptable.FeaturesToRun[1] &=
			adapt->pp.smu_features_mask[1];
	} else {
		AMDGV_ERROR("Unable to get PP table!\n");
		return AMDGV_FAILURE;
	}
	return 0;
}

static int mi200_smu_13_0_parse_pptable(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_13_0_powerplay_table *powerplay_table = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	if (table_context != NULL)
		powerplay_table = table_context->power_play_table;

	if (table_context != NULL &&
	    table_context->ppt_information != NULL &&
	    table_context->power_play_table != NULL &&
	    table_context->driver_pptable != NULL) {
		oss_memcpy(table_context->ppt_information,
			   &powerplay_table->platform_caps,
			   sizeof(struct smu_13_0_ppt_information));
		oss_memcpy(table_context->driver_pptable,
			   &powerplay_table->smc_pptable,
			   sizeof(PPTable_t));
	} else {
		AMDGV_ERROR("Unable to get PP table!\n");
		return AMDGV_FAILURE;
	}
	return 0;
}

static int mi200_smu_13_0_set_dpm_table(struct amdgv_adapter *adapt, PPCLK_e clk,
					struct smu_13_0_dpm_table *dpm_table)
{
	return mi200_smu_13_0_get_dpm_level_range(adapt, clk,
						  &dpm_table->min, &dpm_table->max);
}

static bool mi200_smu_13_0_clk_dpm_is_enabled(struct amdgv_adapter *adapt,
					      enum pp_clock_type clk)
{
	struct smu_context *smu = NULL;
	uint64_t clk_mask = ~0;

	smu = (struct smu_context *)(adapt->pp.smu_backend);

	switch (clk) {
	case PP_CLOCK_TYPE__GFX:
		clk_mask = FEATURE_DPM_GFXCLK_BIT << 1;
		break;
	case PP_CLOCK_TYPE__SOC:
		clk_mask = FEATURE_DPM_SOCCLK_BIT << 1;
		break;
	case PP_CLOCK_TYPE__UCLK:
		clk_mask = FEATURE_DPM_UCLK_BIT << 1;
		break;
	case PP_CLOCK_TYPE__FCLK:
		clk_mask = FEATURE_DPM_FCLK_BIT << 1;
		break;
	default:
		return true;
	}

	return clk_mask & smu->features;
}

static int mi200_smu_13_0_set_default_dpm_tables(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct smu_13_0_dpm_context *dpm_context = NULL;
	PPTable_t *driver_ppt = NULL;
	struct smu_13_0_dpm_tables *dpm_tb;
	struct smu_bios_boot_up_values *boot_values;
	int ret;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (table_context == NULL)
		return AMDGV_FAILURE;

	boot_values = &table_context->boot_values;
	dpm_context = (struct smu_13_0_dpm_context *)smu->smu_dpm_context;
	dpm_tb = &dpm_context->dpm_tables;

	if (!dpm_context || !table_context->driver_pptable) {
		AMDGV_ERROR("Unable to get PP/DPM table!\n");
		return AMDGV_FAILURE;
	}

	driver_ppt = table_context->driver_pptable;
	oss_memset(dpm_context, 0, sizeof(struct smu_13_0_dpm_context));

	if (mi200_smu_13_0_clk_dpm_is_enabled(adapt, PP_CLOCK_TYPE__SOC)) {
		ret = mi200_smu_13_0_set_dpm_table(adapt, PPCLK_SOCCLK, &dpm_tb->soc_table);
		if (ret)
			return ret;
	} else {
		dpm_tb->soc_table.min = boot_values->socclk / 100;
		dpm_tb->soc_table.max = boot_values->socclk / 100;
	}

	if (mi200_smu_13_0_clk_dpm_is_enabled(adapt, PP_CLOCK_TYPE__GFX)) {
		dpm_tb->gfx_table.min = driver_ppt->GfxclkFmin;
		dpm_tb->gfx_table.max = driver_ppt->GfxclkFmax;
	} else {
		dpm_tb->gfx_table.min = boot_values->gfxclk / 100;
		dpm_tb->gfx_table.max = boot_values->gfxclk / 100;
	}

	if (mi200_smu_13_0_clk_dpm_is_enabled(adapt, PP_CLOCK_TYPE__UCLK)) {
		ret = mi200_smu_13_0_set_dpm_table(adapt, PPCLK_UCLK, &dpm_tb->uclk_table);
		if (ret)
			return ret;
	} else {
		dpm_tb->uclk_table.min = boot_values->uclk / 100;
		dpm_tb->uclk_table.max = boot_values->uclk / 100;
	}

	return 0;

}

static bool mi200_smu_13_0_check_clock_type(struct amdgv_adapter *adapt,
					    enum pp_clock_type clk)
{
	bool allowed = 0;

	if (clk == PP_CLOCK_TYPE__GFX ||
	    clk == PP_CLOCK_TYPE__UCLK)
		allowed = true;
	else
		allowed = false;

	return allowed;
}

static void mi200_smu_13_0_get_clock_limit_by_type(
	struct amdgv_adapter *adapt,
	enum pp_clock_type clk,
	struct smu_13_0_dpm_tables *dpm_tb,
	struct smu_13_0_dpm_table **clock_limit)
{
	switch (clk) {
	case PP_CLOCK_TYPE__GFX:
		*clock_limit = &(dpm_tb->gfx_table);
		break;
	case PP_CLOCK_TYPE__VCLK:
		*clock_limit = &(dpm_tb->vclk_table);
		break;
	case PP_CLOCK_TYPE__DCLK:
		*clock_limit = &(dpm_tb->dclk_table);
		break;
	case PP_CLOCK_TYPE__SOC:
		*clock_limit = &(dpm_tb->soc_table);
		break;
	case PP_CLOCK_TYPE__UCLK:
		*clock_limit = &(dpm_tb->uclk_table);
		break;
	case PP_CLOCK_TYPE__FCLK:
		*clock_limit = &(dpm_tb->fclk_table);
		break;
	default:
		clock_limit = NULL;
		break;
	}
}

static uint32_t mi200_powerplay_convert_clk_type(
	struct amdgv_adapter *adapt,
	enum pp_clock_type clk)
{
	uint32_t smu_clk = 0xFFFF;

	switch (clk) {
	case PP_CLOCK_TYPE__GFX:
		smu_clk = PPCLK_GFXCLK << 16;
		break;
	case PP_CLOCK_TYPE__VCLK:
		smu_clk = PPCLK_VCLK << 16;
		break;
	case PP_CLOCK_TYPE__DCLK:
		smu_clk = PPCLK_DCLK << 16;
		break;
	case PP_CLOCK_TYPE__SOC:
		smu_clk = PPCLK_SOCCLK << 16;
		break;
	case PP_CLOCK_TYPE__UCLK:
		smu_clk = PPCLK_UCLK << 16;
		break;
	case PP_CLOCK_TYPE__FCLK:
		smu_clk = PPCLK_FCLK << 16;
		break;
	default:
		AMDGV_ERROR("Unable to find matching clock type!\n");
		break;
	}

	return smu_clk;
}

static int mi200_smu_13_0_get_clock_limit(struct amdgv_adapter *adapt,
	enum pp_clock_type clk,
	enum pp_clock_limit_type limit_type,
	uint32_t *freq)
{
	struct smu_context *smu = NULL;
	struct smu_13_0_dpm_context *dpm_context = NULL;
	struct smu_13_0_dpm_tables *dpm_tb = NULL;
	struct smu_13_0_dpm_table *clock_limit = NULL;
	uint32_t param = 0;
	int ret = 0;

	if (!mi200_smu_13_0_check_clock_type(adapt, clk))
		return AMDGV_FAILURE;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	dpm_context = (struct smu_13_0_dpm_context *)smu->smu_dpm_context;
	if (dpm_context == NULL) {
		AMDGV_ERROR("Unable to get DPM context!\n");
		return AMDGV_FAILURE;
	}

	dpm_tb = &dpm_context->dpm_tables;
	if (dpm_tb == NULL) {
		AMDGV_ERROR("Unable to get current limit table!\n");
		return AMDGV_FAILURE;
	}

	mi200_smu_13_0_get_clock_limit_by_type(adapt, clk, dpm_tb, &clock_limit);
	if (clock_limit == NULL) {
		AMDGV_ERROR("Unable to find matching clock type!\n");
		return AMDGV_FAILURE;
	}

	param = mi200_powerplay_convert_clk_type(adapt, clk);
	AMDGV_ASSERT(param != 0xFFFF);

	switch (limit_type) {
	case PP_CLOCK_LIMIT_TYPE__SOFT_MIN:
		if (!mi200_smu_13_0_clk_dpm_is_enabled(adapt, clk))
			*freq = clock_limit->min;
		else
			ret = mi200_smu_13_0_get_arg_with_param(adapt,
				SMU_13_0_MSG__GET_MIN_DPM_FREQ, param, freq);
		break;

	case PP_CLOCK_LIMIT_TYPE__SOFT_MAX:
		if (!mi200_smu_13_0_clk_dpm_is_enabled(adapt, clk))
			*freq = clock_limit->max;
		else
			ret = mi200_smu_13_0_get_arg_with_param(adapt,
				SMU_13_0_MSG__GET_MAX_DPM_FREQ, param, freq);
		break;

	default:
		AMDGV_ERROR("clock limit isn't supported\n");
		ret = AMDGV_FAILURE;
		break;
	}

	return ret;
}

static int mi200_smu_13_0_populate_smc_pptable(struct amdgv_adapter *adapt)
{
	return  mi200_smu_13_0_set_default_dpm_tables(adapt);
}

static int mi200_smu_13_0_check_fw_version(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t driver_version = 0;

	ret = mi200_smu_13_0_send_msg(adapt,
		SMU_13_0_MSG__GET_DRIVER_IF_VERSION);

	if (ret == 0) {
		ret = mi200_smu_13_0_read_arg(adapt, &driver_version);
		if (ret == 0) {
			if (driver_version != adapt->pp.smu_fw_version) {
				AMDGV_ERROR("SMU driver version(0x%x) doesn't" \
					" match SW-defined version(0x%x)!\n",
					driver_version,
					adapt->pp.smu_fw_version);
				ret = AMDGV_FAILURE;
			}
		}
	} else {
		AMDGV_ERROR("Failed to get F/W version!\n");
	}

	return ret;
}

static int mi200_smu_13_0_copy_table_from_smc(
	struct amdgv_adapter *adapt,
	struct smu_local_memory *fb_memory,
	void *system_memory,
	uint32_t table_id)
{
	int ret = AMDGV_FAILURE;
	uint32_t mask_low = (uint32_t)
		((amdgv_memmgr_get_gpu_addr(fb_memory->mem)
		& SMU_13_0_LOW_MASK) >> SMU_13_0_LOW_SHIFT);
	uint32_t mask_high = (uint32_t)
		((amdgv_memmgr_get_gpu_addr(fb_memory->mem)
		& SMU_13_0_HIGH_MASK) >> SMU_13_0_HIGH_SHIFT);
	void *cpu_memory = amdgv_memmgr_get_cpu_addr(fb_memory->mem);

	AMDGV_ASSERT(fb_memory->mem);
	AMDGV_ASSERT(fb_memory->size != 0);
	AMDGV_ASSERT(system_memory != NULL);

	if ((system_memory != NULL) &&
		(cpu_memory != NULL)) {
		ret = mi200_smu_13_0_send_msg_with_param(
			adapt,
			SMU_13_0_MSG__SET_DRIVER_DRAM_ADDR_HIGH,
			mask_high);

		if (ret == 0) {
			ret = mi200_smu_13_0_send_msg_with_param(adapt,
				SMU_13_0_MSG__SET_DRIVER_DRAM_ADDR_LOW,
				mask_low);
		}
		if (ret == 0) {
			ret = mi200_smu_13_0_send_msg_with_param(adapt,
				SMU_13_0_MSG__TRANSFER_TABLE_SMU2_DRAM,
				table_id);
		}
		oss_memcpy(system_memory, cpu_memory, fb_memory->size);
	}
	return ret;
}

static int mi200_smu_13_0_copy_table_to_smc(
	struct amdgv_adapter *adapt,
	struct smu_local_memory *fb_memory,
	void *system_memory,
	uint32_t table_id)
{

	int ret = AMDGV_FAILURE;
	uint32_t mask_low = (uint32_t)
		((amdgv_memmgr_get_gpu_addr(fb_memory->mem)
		& SMU_13_0_LOW_MASK) >> SMU_13_0_LOW_SHIFT);
	uint32_t mask_high = (uint32_t)
		((amdgv_memmgr_get_gpu_addr(fb_memory->mem)
		& SMU_13_0_HIGH_MASK) >> SMU_13_0_HIGH_SHIFT);
	void *cpu_memory = amdgv_memmgr_get_cpu_addr(fb_memory->mem);

	AMDGV_ASSERT(fb_memory->mem);
	AMDGV_ASSERT(fb_memory->size != 0);
	AMDGV_ASSERT(system_memory != NULL);

	if ((system_memory != NULL) &&
		(cpu_memory != NULL)) {
		oss_memcpy(cpu_memory,
			system_memory,
			fb_memory->size);
		ret = mi200_smu_13_0_send_msg_with_param(
			adapt,
			SMU_13_0_MSG__SET_DRIVER_DRAM_ADDR_HIGH,
			mask_high);

		if (ret == 0) {
			ret = mi200_smu_13_0_send_msg_with_param(adapt,
				SMU_13_0_MSG__SET_DRIVER_DRAM_ADDR_LOW,
				mask_low);
		}
		if (ret == 0) {
			ret = mi200_smu_13_0_send_msg_with_param(adapt,
				SMU_13_0_MSG__TRANSFER_TABLE_DRAM2_SMU,
				table_id);
		}
	}
	return ret;
}

static int mi200_smu_13_0_write_smc_table(
	struct amdgv_adapter *adapt,
	uint32_t table_id)
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
		case TABLE_PPTABLE:
			oss_memcpy(&fb_memory,
				&table_context->smc_pptable,
				sizeof(struct smu_local_memory));
			system_memory = table_context->driver_pptable;
			ret = 0;
			break;
		case TABLE_SMU_METRICS:
		case TABLE_DRIVER_SMU_CONFIG:
		case TABLE_AVFS_PSM_DEBUG:
		case TABLE_AVFS_FUSE_OVERRIDE:
		default:
			AMDGV_ERROR(" SMU13 received wrong table ID\n");
			break;
		}

		if (ret != AMDGV_FAILURE)
			ret = mi200_smu_13_0_copy_table_to_smc(
				adapt, &fb_memory, system_memory, table_id);
	}

	return ret;
}

static int mi200_smu_13_0_update_smc_metrics(struct amdgv_adapter *adapt, uint32_t msg_arg)
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
		ret = mi200_smu_13_0_copy_table_from_smc(adapt, &fb_memory,
								  system_memory, msg_arg);
	}

	return ret;
}

static int mi200_smu_13_0_get_pp_metrics(struct amdgv_adapter *adapt,
		struct amdgv_gpumon_metrics *metrics)
{
	int ret = 0;
	uint8_t pcie_link_speed, pcie_link_width;
	struct smu_table_context *table_context = NULL;
	SmuMetrics_t *metrics_table;
	struct smu_context *smu = NULL;
	PPTable_t *pp_table = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (!table_context)
		return AMDGV_FAILURE;

	amdgv_gpumon_init_metrics_buf(metrics);

	metrics_table = (SmuMetrics_t *) table_context->metrics_table;
	pp_table = (PPTable_t *)table_context->driver_pptable;

	ret = mi200_smu_13_0_update_smc_metrics(adapt, TABLE_SMU_METRICS);

	if (ret)
		return ret;

	ret = mi200_smu_13_0_get_pcie_info(adapt, &pcie_link_speed, &pcie_link_width);

	if (ret) {
		AMDGV_ERROR("Failed to get current PCIe info from PCI config space\n");
		return ret;
	}

	/* Update metrics */
	metrics->clocks[AMDGV_PP_CLK_GFX].curr =
		metrics_table->CurrClock[PPCLK_GFXCLK];
	metrics->clocks[AMDGV_PP_CLK_GFX].avg =
		metrics_table->AverageGfxclkFrequency;
	metrics->clocks[AMDGV_PP_CLK_SOC].curr =
		metrics_table->CurrClock[PPCLK_SOCCLK];
	metrics->clocks[AMDGV_PP_CLK_SOC].avg =
		metrics_table->AverageSocclkFrequency;
	/*
	* clocks[clk_id].curr can provide accurate
	*   output only when the dpm feature is enabled.
	* We can use Average_* for dpm disabled case,
	* such as AMDGV_PP_CLK_MEM.
	*/
	metrics->clocks[AMDGV_PP_CLK_MEM].curr =
		metrics_table->AverageUclkFrequency;
	metrics->clocks[AMDGV_PP_CLK_MEM].avg =
		metrics_table->AverageUclkFrequency;
	metrics->mem_usage =
		metrics_table->AverageUclkActivity;
	metrics->gfx_usage =
		metrics_table->AverageGfxActivity;
	metrics->volt_soc = 155000 - 625*
		metrics_table->CurrSocVoltageOffset;
	metrics->volt_gfx = 155000 - 625*
		metrics_table->CurrGfxVoltageOffset;
	metrics->volt_mem = 155000 - 625*
		metrics_table->CurrMemVidOffset;
	metrics->power =
		metrics_table->AverageSocketPower;
	metrics->temp_edge =
		metrics_table->TemperatureEdge;
	metrics->temp_hotspot =
		metrics_table->TemperatureHotspot;
	metrics->temp_mem =
		metrics_table->TemperatureHBM;
	metrics->temp_hotspot_limit =
		pp_table->ThotspotLimit;
	metrics->temp_mem_limit =
		pp_table->TmemLimit;
	metrics->serial =
		(uint64_t)metrics_table->PublicSerialNumUpper32 << 32 |
		metrics_table->PublicSerialNumLower32;
	metrics->pcie_rate = pcie_link_speed;
	metrics->pcie_width = pcie_link_width;

	return ret;
}

static int mi200_smu_13_0_set_tool_table_location(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = NULL;
	struct smu_table_context *table_context = NULL;
	struct amdgv_memmgr_mem *mem = NULL;
	uint32_t upper_mc_addr = 0;
	uint32_t lower_mc_addr = 0;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);
	mem = table_context->smc_pm_status_log_table.mem;
	upper_mc_addr = upper_32_bits(amdgv_memmgr_get_gpu_addr(mem));
	lower_mc_addr = lower_32_bits(amdgv_memmgr_get_gpu_addr(mem));

	ret = mi200_smu_13_0_send_msg_with_param(adapt,
		SMU_13_0_MSG__SET_TOOLS_DRAM_ADDR_HIGH,
		upper_mc_addr);
	if (ret)
		return AMDGV_FAILURE;

	ret = mi200_smu_13_0_send_msg_with_param(adapt,
		SMU_13_0_MSG__SET_TOOLS_DRAM_ADDR_LOW,
		lower_mc_addr);
	if (ret)
		return AMDGV_FAILURE;

	return 0;
}

static void mi200_smu_13_0_print_enabled_smu_features(struct amdgv_adapter *adapt, uint64_t features)
{
	if (features & (1 << FEATURE_DATA_CALCULATIONS)) {
		AMDGV_INFO("SMU Feature Enabled: DATA \n");
	}
	if (features & (1 << FEATURE_DPM_GFXCLK_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: GFX DPM \n");
	}
	if (features & (1 << FEATURE_DPM_XGMI_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: XGMI DPM \n");
	}
	if (features & (1 << FEATURE_DPM_UCLK_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: UCLK DPM \n");
	}
	if (features & (1 << FEATURE_DPM_SOCCLK_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: SOC CLK DPM \n");
	}
	if (features & (1 << FEATURE_DPM_FCLK_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: FCLK DPM \n");
	}
	if (features & (1 << FEATURE_DPM_LCLK_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: LCLK DPM \n");
	}
	if (features & (1 << FEATURE_DS_GFXCLK_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: GFX CLK DEEP SLEEP \n");
	}
	if (features & (1 << FEATURE_DS_SOCCLK_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: SOC CLK DEEP SLEEP \n");
	}
	if (features & (1 << FEATURE_DS_LCLK_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: LCLK DEEP SLEEP \n");
	}
	if (features & (1 << FEATURE_DS_FCLK_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: FCLK DEEP SLEEP \n");
	}
	if (features & (1 << FEATURE_DS_UCLK_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: UCLK DEEP SLEEP \n");
	}
	if (features & (1 << FEATURE_GFX_SS_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: GFX CLK SPREAD SPECTRUM \n");
	}
	if (features & (1 << FEATURE_DPM_VCN_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: VCN DPM \n");
	}
	if (features & (1 << FEATURE_RSMU_SMN_CG_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: RSMU SMN CLOCK GATING \n");
	}
	if (features & (1 << FEATURE_WAFL_CG_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: WAFL CLOCK GATING \n");
	}
	if (features & (1 << FEATURE_FUSE_CG_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: FUSE CLOCK GATING \n");
	}
	if (features & (1 << FEATURE_MP1_CG_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: MP1 CLOCK GATING \n");
	}
	if (features & (1 << FEATURE_SMUIO_CG_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: SMUIO CLOCK GATING \n");
	}
	if (features & (1 << FEATURE_THM_CG_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: THERMAL CLOCK GATING \n");
	}
	if (features & (1 << FEATURE_CLK_CG_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: CLK CLOCK GATING \n");
	}
	if (features & (1 << FEATURE_PPT_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: PACKAGE POWER TRACKING \n");
	}
	if (features & (1 << FEATURE_TDC_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: THERMAL DESIGN CONTROL \n");
	}
	if (features & (1 << FEATURE_APCC_PLUS_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: PEAK CURRENT CONTROL \n");
	}
	if (features & (1 << FEATURE_APCC_DFLL_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: APCC DIGITAL FREQUENCY LOCKED LOOP \n");
	}
	if (features & (1 << FEATURE_FW_CTF_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: CRITICAL TEMP FAULT \n");
	}
	if (features & (1 << FEATURE_THERMAL_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: THERMAL \n");
	}
	if (features & (1 << FEATURE_OUT_OF_BAND_MONITOR_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: OUT OF BAND MONITOR \n");
	}
	if (features & (1 << FEATURE_XGMI_PER_LINK_PWR_DWN)) {
		AMDGV_INFO("SMU Feature Enabled: PER LINK GMI PWR DOWN \n");
	}
	if (features & (1 << FEATURE_DF_CSTATE)) {
		AMDGV_INFO("SMU Feature Enabled: DF CSTATE \n");
	}
	if (features & (1 << FEATURE_EDC_BIT)) {
		AMDGV_INFO("SMU Feature Enabled: ELECTRICAL DESIGN CURRENT \n");
	}

}
static int mi200_smu_13_0_get_enabled_smu_features(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t features_low = 0;
	uint32_t features_high = 0;
	struct smu_context *smu = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);

	ret = mi200_smu_13_0_send_msg(adapt,
		SMU_13_0_MSG__GET_ENABLED_SMU_FEATURES_LOW);

	if (ret == 0)
		ret = mi200_smu_13_0_read_arg(adapt, &features_low);

	if (ret == 0) {
		ret = mi200_smu_13_0_send_msg(adapt,
			SMU_13_0_MSG__GET_ENABLED_SMU_FEATURES_HIGH);

		if (ret == 0)
			ret = mi200_smu_13_0_read_arg(adapt, &features_high);
	}
	if (ret == 0) {
		smu->features = ((uint64_t)features_high
			<< SMU_13_0_HIGH_SHIFT) | features_low;

		mi200_smu_13_0_print_enabled_smu_features(adapt, smu->features);
	}

	return ret;
}

static int mi200_smu_13_0_run_btc(struct amdgv_adapter *adapt)
{
	int ret;

	ret = mi200_smu_13_0_send_msg(adapt, PPSMC_MSG_RunDcBtc);
	if (ret) {
		AMDGV_ERROR("Failed to send RunDcBtc message to SMC\n");
		return ret;
	}

	/* the SMC firmware will ignore BoardPowerCalibration if board type is SCM */
	ret = mi200_smu_13_0_send_msg(adapt, PPSMC_MSG_BoardPowerCalibration);
	if (ret) {
		AMDGV_ERROR("Failed to send BoardPowerCalibration message to SMC\n");
		return ret;
	}

	return 0;
}

static int mi200_smu_13_0_system_features_control(
	struct amdgv_adapter *adapt, uint32_t enabled)
{
	int ret = 0;
	struct smu_context *smu = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);

	if (enabled == SMU_13_0_DISABLE) {
		AMDGV_INFO("SMU 13 is trying to disable all smu features\n");
		ret = mi200_smu_13_0_send_msg(adapt,
			SMU_13_0_MSG__DISABLE_ALL_SMU_FEATURES);

		if (ret == 0)
			smu->features = 0;
	} else {
		ret = mi200_smu_13_0_send_msg(adapt,
					      SMU_13_0_MSG__ENABLE_ALL_SMU_FEATURES);
		if (ret)
			return ret;

		ret = mi200_smu_13_0_get_enabled_smu_features(adapt);
		if (ret)
			return ret;

		ret = mi200_smu_13_0_run_btc(adapt);
		if (ret)
			return ret;
	}

	return ret;
}

static int mi200_smu_13_0_smc_table_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = mi200_smu_13_0_get_vbios_bootup_values(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to get VBIOS bootup values!\n");
		return AMDGV_FAILURE;
	}

	ret = mi200_smu_13_0_read_pptable_from_vbios(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to get PP table from VBIOS!\n");
		return AMDGV_FAILURE;
	}

	ret = mi200_smu_13_0_get_clk_info_from_vbios(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to get clk info from VBIOS!\n");
		return AMDGV_FAILURE;
	}

	ret = mi200_smu_13_0_append_vbios_pptable(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to append VBIOS PP table!\n");
		return AMDGV_FAILURE;
	}

	ret = mi200_smu_13_0_check_pptable(adapt);
	if (ret) {
		AMDGV_ERROR("Failed at PP table check!\n");
		return AMDGV_FAILURE;
	}

	ret = mi200_smu_13_0_overwrite_pptable(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to overwrite pp table!\n");
		return AMDGV_FAILURE;
	}

	ret = mi200_smu_13_0_parse_pptable(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to parse PP table!\n");
		return AMDGV_FAILURE;
	}

	ret = mi200_smu_13_0_check_fw_version(adapt);
	if (ret) {
		AMDGV_ERROR("Firmware version check failed!\n");
		return AMDGV_FAILURE;
	}

	ret = mi200_smu_13_0_write_smc_table(adapt, TABLE_PPTABLE);
	if (ret) {
		AMDGV_ERROR("Failed to copy PP table to SMU!\n");
		return AMDGV_FAILURE;
	}

	ret = mi200_smu_13_0_set_tool_table_location(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to set tool table location!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi200_smu_13_0_smc_table_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi200_smu_13_0_update_smc_table(
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
			ret = 0;
			break;

		default:
			AMDGV_ERROR(" SMU11 received wrong table ID\n");
			break;
		}

		if (ret != AMDGV_FAILURE)
			ret = mi200_smu_13_0_copy_table_to_smc(
				adapt, &fb_memory, system_memory, table_id);
	}

	return ret;
}

static void mi200_smu_13_0_fill_eeprom_i2c_req(SwI2cRequest_t *req, bool write,
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

static int mi200_smu_13_0_i2c_eeprom_read_data(struct amdgv_adapter *adapt,
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
	mi200_smu_13_0_fill_eeprom_i2c_req(&req, false,
				address, i2c_port,
				data, numbytes);

	/* Now read data starting with that address */
	ret = mi200_smu_13_0_update_smc_table(adapt, &req, TABLE_I2C_COMMANDS);
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

		AMDGV_DEBUG("i2c_eeprom_read_data, address = %x, bytes = %d",
				  (uint16_t)address, numbytes);
	} else
		AMDGV_WARN("i2c_eeprom_read_data - error occurred :%x", ret);

	return ret;
}

static int mi200_smu_13_0_i2c_eeprom_write_data(struct amdgv_adapter *adapt,
					       uint8_t address, uint8_t i2c_port,
					       uint8_t *data, uint32_t numbytes)
{
	uint32_t ret;
	SwI2cRequest_t req;

	oss_memset(&req, 0, sizeof(req));
	mi200_smu_13_0_fill_eeprom_i2c_req(&req, true,
				address, i2c_port,
				data, numbytes);

	ret = mi200_smu_13_0_update_smc_table(adapt, &req, TABLE_I2C_COMMANDS);
	if (!ret) {
		AMDGV_DEBUG("i2c_write(), address = %x, bytes = %d , data: ",
					 (uint16_t)address, numbytes);
		/*
		 * According to EEPROM spec there is a MAX of 10 ms required for
		 * EEPROM to flush internal RX buffer after STOP was issued at the
		 * end of write transaction. During this time the EEPROM will not be
		 * responsive to any more commands - so wait a bit more.
		 */
		oss_msleep(10);

	} else
		AMDGV_WARN("i2c_write- error occurred :%x", ret);

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
static int mi200_smu_13_0_protect_i2c_eeprom_data_write(
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
		ret = mi200_smu_13_0_i2c_eeprom_write_data(adapt,
				addr, port, data_chunk, remain_size + 2);
		if (ret)
			return ret;

		oss_memcpy(data_chunk + 2, data_ptr + remain_size, write_size - remain_size);
		data_chunk[0] = (((next_eeprom_addr + remain_size) >> 8) & 0xff);
		data_chunk[1] = ((next_eeprom_addr + remain_size) & 0xff);
		ret = mi200_smu_13_0_i2c_eeprom_write_data(adapt,
				addr, port, data_chunk, write_size - remain_size + 2);
	} else {
		oss_memcpy(data_chunk + 2, data_ptr, write_size);
		data_chunk[0] = ((next_eeprom_addr >> 8) & 0xff);
		data_chunk[1] = (next_eeprom_addr & 0xff);
		ret = mi200_smu_13_0_i2c_eeprom_write_data(adapt,
				addr, port, data_chunk, write_size + 2);
	}

	return ret;
}
static int mi200_smu_13_0_i2c_eeprom_i2c_xfer(struct amdgv_adapter *adapt,
		uint8_t port, struct i2c_msg *msgs, int num)
{
	uint32_t  j, ret, data_size, data_chunk_size, next_eeprom_addr = 0;
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
				ret = mi200_smu_13_0_i2c_eeprom_read_data(adapt,
						(uint8_t)msgs[i].addr, port,
						data_chunk, MAX_SW_I2C_COMMANDS);

				oss_memcpy(data_ptr, data_chunk + 2, data_chunk_size);
			} else {
				ret = mi200_smu_13_0_protect_i2c_eeprom_data_write(adapt,
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
				ret = mi200_smu_13_0_i2c_eeprom_read_data(adapt,
						(uint8_t)msgs[i].addr, port,
						data_chunk, (data_size % data_chunk_size) + 2);

				oss_memcpy(data_ptr, data_chunk + 2, data_size % data_chunk_size);
			} else {
				ret = mi200_smu_13_0_protect_i2c_eeprom_data_write(adapt,
									(uint8_t)msgs[i].addr, port,
									next_eeprom_addr, data_ptr,
									data_chunk_size);
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

int mi200_mode1_reset(struct amdgv_adapter *adapt)
{
	uint32_t resp = 0;
	uint32_t fatal_err = 0, param;

	/* send mode1 reset command to SMU (MP1) */
	AMDGV_DEBUG("sending mode1_reset command to SMU ...\n");

	param = PPSMC_RESET_TYPE_DRIVER_MODE_1_RESET;

	if (oss_atomic_read(adapt->in_ecc_recovery))
		fatal_err = 1;

	param |= (fatal_err << 16);

	if (mi200_smu_13_0_wait_for_response(adapt, &resp) != 0)
		if (mi200_smu_13_0_send_test_msg(adapt) != 0)
			return AMDGV_FAILURE;

	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90), 0);

	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82), param);

	/* For mi200, after mode1_reset msg, all registers are blocked(ffffffff)
	 * until pci_cfg space is restored.Therefore we move MSG90 polling
	 * step	behind mi200_reset_restore_pf.
	 */
	mi200_smu_13_0_send_msg_without_waiting(adapt,
						PPSMC_MSG_GfxDriverReset);

	/* allow time for all blocks to complete RESET */
	oss_msleep(500);
	AMDGV_DEBUG("mode1_reset completed\n");

	return 0;
}

int mi200_wait_mode1_reset_completion(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t resp = 0;

	ret = mi200_smu_13_0_wait_for_response(adapt, &resp);
	if (ret == AMDGV_FAILURE || resp != PPSMC_Result_OK) {
		AMDGV_ERROR(
		    "Failed to send mode1 reset message 0x%x, response 0x%x\n",
		    PPSMC_MSG_GfxDriverReset, resp);
		return AMDGV_FAILURE;
	}

	/* clear smu mail box registers after mode1 reset,
	 * if not, it will cause mode1 reset fail next time.
	 */
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_66), 0);

	return 0;
}

static int mi200_smu_13_0_trigger_vf_flr(struct amdgv_adapter *adapt, uint32_t param)
{
	return mi200_smu_13_0_send_msg_with_param(adapt,
						  PPSMC_MSG_TriggerVFFLR,
						  param);
}

static int mi200_smu_13_0_get_ecc_info(struct amdgv_adapter *adapt,
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
	ret = mi200_smu_13_0_update_smc_metrics(adapt, TABLE_ECCINFO);

	if (!ret) {
		for (i = 0; i < MI200_UMC_CHANNEL_NUM; i++) {
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

static int mi200_smu_13_0_is_pm_enabled(struct amdgv_adapter *adapt, bool *pm_enabled)
{
	struct smu_context *smu = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);

	if (smu->features)
		*pm_enabled = true;
	else
		*pm_enabled = false;

	return 0;
}

static void mi200_pp_smu_metric_to_gpumon_ext(struct amdgv_adapter *adapt,
	SmuMetrics_t *metrics_table, PPTable_t *pp_table,
	struct amdgv_gpumon_metrics_ext *metrics_ext)
{
	metrics_ext->num_metric = 0;

	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__FREQUENCY,
		AMDGV_GPUMON_METRIC_EXT_NAME__CLK_GFX, AMDGV_GPUMON_METRIC_EXT_UNIT__MHZ, metrics_table->CurrClock[PPCLK_GFXCLK]);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__FREQUENCY,
		AMDGV_GPUMON_METRIC_EXT_NAME__CLK_SOC, AMDGV_GPUMON_METRIC_EXT_UNIT__MHZ, metrics_table->CurrClock[PPCLK_SOCCLK]);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__FREQUENCY,
		AMDGV_GPUMON_METRIC_EXT_NAME__CLK_MEM, AMDGV_GPUMON_METRIC_EXT_UNIT__MHZ, metrics_table->AverageUclkFrequency);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__ACTIVITY,
		AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_GFX, AMDGV_GPUMON_METRIC_EXT_UNIT__PERCENT, metrics_table->AverageGfxActivity);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__ACTIVITY,
		AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_MEM, AMDGV_GPUMON_METRIC_EXT_UNIT__PERCENT, metrics_table->AverageUclkActivity);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__POWER,
		AMDGV_GPUMON_METRIC_EXT_NAME__VOLT_GFX, AMDGV_GPUMON_METRIC_EXT_UNIT__MILLIVOLT, 155000 - 625 * metrics_table->CurrGfxVoltageOffset);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__POWER,
		AMDGV_GPUMON_METRIC_EXT_NAME__VOLT_SOC, AMDGV_GPUMON_METRIC_EXT_UNIT__MILLIVOLT, 155000 - 625 * metrics_table->CurrSocVoltageOffset);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__POWER,
		AMDGV_GPUMON_METRIC_EXT_NAME__VOLT_MEM, AMDGV_GPUMON_METRIC_EXT_UNIT__MILLIVOLT, 155000 - 625 * metrics_table->CurrMemVidOffset);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__POWER,
		AMDGV_GPUMON_METRIC_EXT_NAME__POWER_CURR, AMDGV_GPUMON_METRIC_EXT_UNIT__WATT, metrics_table->AverageSocketPower);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__TEMPERATURE,
		AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_HOTSPOT_CURR, AMDGV_GPUMON_METRIC_EXT_UNIT__CELSIUS, metrics_table->TemperatureHotspot);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__TEMPERATURE,
		AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_MEM_CURR, AMDGV_GPUMON_METRIC_EXT_UNIT__CELSIUS, metrics_table->TemperatureHBM);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__TEMPERATURE,
		AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_HOTSPOT_LIMIT, AMDGV_GPUMON_METRIC_EXT_UNIT__CELSIUS, pp_table->ThotspotLimit);
	MI200_ADD_METRICS_EXT_ENTRY(metrics_ext, AMDGV_GPUMON_METRIC_EXT_CATEGORY__TEMPERATURE,
		AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_MEM_LIMIT, AMDGV_GPUMON_METRIC_EXT_UNIT__CELSIUS, pp_table->TmemLimit);
}

static int mi200_pp_smu_get_metrics_ext(struct amdgv_adapter *adapt,
	struct amdgv_gpumon_metrics_ext *metrics_ext)
{
	int ret = 0;
	struct smu_table_context *table_context = NULL;
	SmuMetrics_t *metrics_table;
	PPTable_t *pp_table;
	struct smu_context *smu = NULL;

	smu = (struct smu_context *)(adapt->pp.smu_backend);
	table_context = (struct smu_table_context *)(smu->smu_table_context);

	if (!table_context) {
		AMDGV_ERROR("table_context is NULL\n");
		return AMDGV_FAILURE;
	}

	metrics_table = (SmuMetrics_t *) table_context->metrics_table;
	pp_table = (PPTable_t *)table_context->driver_pptable;	/* this does not needs to update */

	ret = mi200_smu_13_0_update_smc_metrics(adapt, TABLE_SMU_METRICS);

	if (ret) {
		AMDGV_ERROR("smc metrics update failed\n");
		return AMDGV_FAILURE;
	}

	mi200_pp_smu_metric_to_gpumon_ext(adapt, metrics_table, pp_table, metrics_ext);

	return ret;
}

static void mi200_smu_notify_throttler_error(struct amdgv_adapter *adapt,
					     uint32_t throttler_status)
{
	uint64_t throttler_event;

	throttler_event =
		smu_pp_throttler_event_convert(adapt, mi200_smu_throttler_event_map,
							 ARRAY_SIZE(mi200_smu_throttler_event_map),
							 (uint64_t)throttler_status);

	AMDGV_DEBUG("mi200 smu notify throttler status 0x%08x, throttler_event 0x%016llx\n",
		    throttler_status, throttler_event);
	amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_PP_THROTTLER_EVENT, throttler_event);
}

static int mi200_smu_pp_handle_irq(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	int ret = 0;
	uint32_t val, throttler_status;
	uint32_t ctx_id;
	uint32_t vf_flr_intr_sts;
	int i;
	uint64_t curr_time, throttle_delta;

	if (entry->client_id != IH_IV_CLIENTID_MP1 ||
	    entry->src_id != IH_INTERRUPT_ID_TO_DRIVER)
		return 0;

	val = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL));
	val = REG_SET_FIELD(val, MP1_SMN_IH_SW_INT_CTRL, INT_ACK, 1);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL), val);

	ctx_id = entry->src_data[0];
	switch (ctx_id) {
	case IH_INTERRUPT_VFFLR_INT:
		/* avoid the possible race condition that some VM is just destroyed within the short
		 * window when host driver enables flr strap in mi300_reset_vf_flr()
		 */
		if (!(adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY))
			break;

		/* MI200 FLR interrupt format:
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
	case IH_INTERRUPT_CONTEXT_ID_THERMAL_THROTTLING:
		curr_time = oss_get_time_stamp();
		throttle_delta = curr_time - adapt->pp.thermal_throttle_start_time;
		if (throttle_delta > adapt->opt.thermal_throttle_rate_limit) {
			adapt->pp.thermal_throttle_start_time = curr_time;
			throttler_status = entry->src_data[1];
			mi200_smu_notify_throttler_error(adapt, throttler_status);
		}
		break;
	default:
		AMDGV_ERROR("mi200 smu can't process this context id %d\n", ctx_id);
		break;
	}

	return 0;
}

const struct amdgv_pp_funcs mi200_amdgv_pp_funcs = {
	.i2c_eeprom_xfer = mi200_smu_13_0_i2c_eeprom_i2c_xfer,
	.get_metrics_ext = mi200_pp_smu_get_metrics_ext,
	.get_pp_metrics = mi200_smu_13_0_get_pp_metrics,
	.get_power_capacity = mi200_smu_13_0_get_power_capacity,
	.get_dpm_capacity = mi200_smu_13_0_get_gfx_dpm_level_count,
	.get_clock_limit = mi200_smu_13_0_get_clock_limit,
	.trigger_vf_flr = mi200_smu_13_0_trigger_vf_flr,
	.get_ecc_info = mi200_smu_13_0_get_ecc_info,
	.is_pm_enabled = mi200_smu_13_0_is_pm_enabled,
	.get_fru_product_info = mi200_fru_get_product_info,
	.handle_smu_irq = mi200_smu_pp_handle_irq,
};

int mi200_powerplay_sw_init(struct amdgv_adapter *adapt)
{
	adapt->pp.thermal_throttle_start_time = 0;
	return 0;
}

int mi200_powerplay_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = mi200_smu_13_0_check_fw_status(adapt);

	if (ret) {
		AMDGV_ERROR("[mi200_smu_13_0_check_fw_status] Failed!\n");
		return ret;
	}

	ret = mi200_smu_13_0_system_features_control(adapt, SMU_13_0_ENABLE);
	if (ret) {
		AMDGV_ERROR(
		"[mi200_smu_13_0_system_features_control] Failed!\n");
		return ret;
	}

	ret = mi200_smu_13_0_populate_smc_pptable(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to populate smc PP table!\n");
		return AMDGV_FAILURE;
	}

	ret = mi200_fru_get_product_info(adapt);
	if (ret) {
			AMDGV_ERROR("Failed to get product info\n");
			return AMDGV_FAILURE;
	}

	return 0;
}

int mi200_powerplay_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

int mi200_powerplay_hw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct amdgv_hive_info *hive = NULL;

	/* 	Need a lock to ensure only one GPU is performing this operation
		in MCM XGMI config as master and slave GPU on the same die can
		cause SMU to deadlock. Currently affects all XGMI config as we
		can't differentiate the two easily*/
	hive = amdgv_get_xgmi_hive(adapt);
	if (adapt->xgmi.phy_nodes_num > 1 && hive && hive->mcm_hive_lock) {
		oss_mutex_lock(hive->mcm_hive_lock);
		ret = mi200_smu_13_0_send_msg(adapt,
					SMU_13_0_MSG__DISABLE_ALL_SMU_FEATURES);
		oss_mutex_unlock(hive->mcm_hive_lock);
	} else {
		ret = mi200_smu_13_0_send_msg(adapt,
					SMU_13_0_MSG__DISABLE_ALL_SMU_FEATURES);
	}
	if (ret) {
		AMDGV_ERROR("Failed to disable all smu features!\n");
		return ret;
	}

	return 0;
}

static void mi200_smu_set_reset_quirks(struct amdgv_adapter *adapt)
{
	int ret;
	uint32_t smu_version;

	ret = mi200_smu_13_0_send_msg(adapt, PPSMC_MSG_GetSmuVersion);
	if (ret)
		return;

	mi200_smu_13_0_read_arg(adapt, &smu_version);
	if (smu_version >= 0x00443f65)
		adapt->flags |= AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY;
}

static int mi200_smu_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct smu_context *smu = NULL;
	uint64_t disable_features = 0;

	smu = oss_zalloc(sizeof(struct smu_context));
	if (smu == NULL) {
		AMDGV_ERROR("Failed to alloc memory for smu context\n");
		return AMDGV_FAILURE;
	}
	adapt->pp.smu_backend = smu;
	adapt->pp.smu_fw_version = SMU13_DRIVER_IF_VERSION;
	adapt->pp.pp_funcs = &mi200_amdgv_pp_funcs;

	/* disable deep sleep features */
	disable_features |=
		((1 << FEATURE_DS_GFXCLK_BIT) |
		(1 << FEATURE_DS_SOCCLK_BIT) |
		(1 << FEATURE_DS_UCLK_BIT) |
		(1 << FEATURE_DS_LCLK_BIT) |
		(1 << FEATURE_DS_FCLK_BIT) |
		(1 << FEATURE_DF_CSTATE));

	adapt->pp.smu_features_mask[0] = ~(lower_32_bits(disable_features));
	adapt->pp.smu_features_mask[1] = ~(upper_32_bits(disable_features));

	ret = mi200_smu_13_0_smc_table_sw_init(adapt);

	return ret;
}

static int mi200_smu_sw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = mi200_smu_13_0_smc_table_sw_fini(adapt);
	if (ret) {
		AMDGV_ERROR("[mi200_smu_13_0_smc_table_sw_fini] Failed!\n");
		return ret;
	}

	return ret;
}

static int mi200_smu_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = mi200_smu_13_0_check_fw_status(adapt);
	if (ret) {
		AMDGV_ERROR("[mi200_smu_13_0_check_fw_status] Failed!\n");
		return ret;
	}

	ret = mi200_smu_13_0_smc_table_hw_init(adapt);
	if (ret) {
		AMDGV_ERROR("[mi200_smu_13_0_smc_table_hw_init] Failed!\n");
		return ret;
	}

	mi200_smu_set_reset_quirks(adapt);

	return 0;
}

static int mi200_smu_hw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = mi200_smu_13_0_smc_table_hw_fini(adapt);
	if (ret) {
		AMDGV_ERROR("[mi200_smu_13_0_smc_table_hw_fini] Failed!\n");
		return ret;
	}

	return 0;
}

int mi200_fru_get_product_info(struct amdgv_adapter *adapt)
{
	char *manufacturer_name = "AMD";
	char *product_name = "AMD Instinct MI210";
	char *non_applicable_field = "N/A";

	oss_memcpy(adapt->product_info.manufacturer_name, manufacturer_name,
		oss_strlen(manufacturer_name) + 1);
	adapt->product_info.manufacturer_name[oss_strlen(manufacturer_name)] = 0;

	oss_memcpy(adapt->product_info.product_name, product_name,
		oss_strlen(product_name) + 1);
	adapt->product_info.product_name[oss_strlen(product_name)] = 0;

	oss_memcpy(adapt->product_info.model_number, non_applicable_field,
		oss_strlen(non_applicable_field) + 1);
	adapt->product_info.model_number[oss_strlen(non_applicable_field)] = 0;

	oss_memcpy(adapt->product_info.product_serial, non_applicable_field,
		oss_strlen(non_applicable_field) + 1);
	adapt->product_info.product_serial[oss_strlen(non_applicable_field)] = 0;

	oss_memcpy(adapt->product_info.fru_id, non_applicable_field,
		oss_strlen(non_applicable_field) + 1);
	adapt->product_info.fru_id[oss_strlen(non_applicable_field)] = 0;

	adapt->product_info.valid = true;
	adapt->product_info.visit = true;

	return 0;
}

const struct amdgv_init_func mi200_powerplay_func = {
	.name = "mi200_powerplay_func",
	.sw_init = mi200_powerplay_sw_init,
	.sw_fini = mi200_powerplay_sw_fini,
	.hw_init = mi200_powerplay_hw_init,
	.hw_fini = mi200_powerplay_hw_fini,
};

const struct amdgv_init_func mi200_smu_func = {
	.name = "mi200_smu_func",
	.sw_init = mi200_smu_sw_init,
	.sw_fini = mi200_smu_sw_fini,
	.hw_init = mi200_smu_hw_init,
	.hw_fini = mi200_smu_hw_fini,
};

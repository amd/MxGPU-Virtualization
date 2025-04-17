/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_powerplay_swsmu.h"
#include "amdgv_vbios.h"
#include "atombios/atom.h"
#include "atombios/atomfirmware.h"
#include <amdgv_device.h>

#include "mi300.h"
#include "mi300/NBIO/nbio_7_9_0_offset.h"
#include "mi300/NBIO/nbio_7_9_0_sh_mask.h"
#include "mi300_nbio.h"
#include "mi300_powerplay.h"
#include "mi300_smu_driver_if.h"
#include "mi300_smu_pmfw.h"
#include "mi300_smu_ppsmc.h"
#include "mi300_fru.h"
#include "mi300_psp.h"

static const uint32_t this_block = AMDGV_POWER_BLOCK;
#define adapt_to_smu(adapt)	      ((struct smu_context *)((adapt)->pp.smu_backend))
#define max(a, b)		      ((a) > (b) ? (a) : (b))
#define MI300_SMU_MAX_DPM_LEVEL_COUNT (4)

#define MI300_SMU_NUM_AID_CLK 4
#define MI300_SMU_NUM_XCC_CLK 8

#define ADD_DRV_METRICS_EXT_ENTRY(metric_code, encoding, addr, mask) \
	mi300_pp_smu_add_drv_metrics_ext_entry(adapt, drv_metrics_ext, metric_code, encoding, addr, mask)

/* no pptable in pmfw, add fake one */
typedef struct Mi300_PPTable {
  uint32_t MaxGfxclkFrequency;
  uint32_t MinGfxclkFrequency;
  uint32_t FclkFrequencyTable[4];
  uint32_t UclkFrequencyTable[4];
  uint32_t SocclkFrequencyTable[4];
  uint32_t VclkFrequencyTable[4];
  uint32_t DclkFrequencyTable[4];
  uint32_t LclkFrequencyTable[4];
  uint32_t MaxSocketPowerLimit;
  uint32_t HotSpotCtfTemp;
  uint32_t MemCtfTemp;
  uint32_t DefaultPowerLimit;
} PPTable_t;

struct mi300_smu_table_context {
	struct smu_local_memory driver_table_mem;
	struct smu_local_memory tool_table_mem;
	void *metrics_table;
	uint32_t metrics_table_size;
	void *pptable;
	uint32_t pptable_size;
	void *ecctable_array;
	uint32_t ecctable_array_size;
};

struct mi300_dpm_clock_level {
	uint32_t enabled;
	uint32_t value;
};

struct mi300_dpm_table_entry {
	uint32_t count;
	uint32_t min;
	uint32_t max;
	bool fine_grain;
	struct mi300_dpm_clock_level levels[MI300_SMU_MAX_DPM_LEVEL_COUNT];
};

enum mi300_dpm_clock_type {
	DPM_CLOCK_TYPE_GFXCLK = 0,
	DPM_CLOCK_TYPE_FCLK,
	DPM_CLOCK_TYPE_UCLK,
	DPM_CLOCK_TYPE_SOCCLK,
	DPM_CLOCK_TYPE_VCLK,
	DPM_CLOCK_TYPE_DCLK,
	DPM_CLOCK_TYPE_LCLK,
	DPM_CLOCK_TYPE_MAX,
};

struct mi300_smu_dpm_context {
	struct mi300_dpm_table_entry dpm_entry[DPM_CLOCK_TYPE_MAX];
	struct mi300_smu_dpm_policy_ctxt *dpm_policies;
};

enum mi300_metric_addr_encoding {
	UINT32_BIT0 = 0,
	UINT32_BIT1 = 1,
	UINT32_BIT2 = 2,
	UINT32_BIT3 = 3,
	UINT32_BIT4 = 4,
	UINT32_BIT5 = 5,
	UINT32_BIT6 = 6,
	UINT32_BIT7 = 7,
	UINT_32,
	UINT_64,
	Q10_32,
	Q10_32_DS,
	Q10_64,
	NOT_AVAILABLE
};

struct mi300_pp_drv_metrics_ext {
	uint32_t num_metric;
	struct amdgv_gpumon_metric_ext metric[AMDGV_GPUMON_MAX_NUM_METRICS_EXT];
	struct {
		void *addr;
		enum mi300_metric_addr_encoding encoding;
	} runtime_config[AMDGV_GPUMON_MAX_NUM_METRICS_EXT];
};

static int mi300_smu_wait_for_response(struct amdgv_adapter *adapt, uint32_t *val)
{
	int ret;
	uint32_t tmp;

	ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90),
				      MP1_SMN_C2PMSG_90__CONTENT_MASK, 0,
				      AMDGV_TIMEOUT(TIMEOUT_SMU_REG), AMDGV_WAIT_CHECK_NE, 0);

	tmp = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90));
	if (val)
		*val = tmp;

	/* Add the message to diagnosis data trace log */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_READ_RESP, ret, regMP1_SMN_C2PMSG_90, tmp);

	/* timeout means wrong logic */
	if (ret)
		return AMDGV_FAILURE;

	return 0;
}

static void mi300_smu_send_msg_nocheck(struct amdgv_adapter *adapt, uint32_t msg,
				       uint32_t param)
{
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90), 0);

	/* Set param, and add parameters start/end to the diagnosis data */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_ARG_START, 0, regMP1_SMN_C2PMSG_82,
				 param);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82), param);
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_ARG_END, 0, regMP1_SMN_C2PMSG_82,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82)));

	/* Set msg, and add parameters start/end to the diagnosis data */
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_MSG_START, 0, regMP1_SMN_C2PMSG_66, msg);
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_66), msg);
	AMDGV_DIAG_DATA_TRACE_LOG_SMU(AMDGV_DIAG_DATA_SMU_WRITE_MSG_END, 0, regMP1_SMN_C2PMSG_66,
		RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_66)));
}

uint32_t mi300_smu_read_arg(struct amdgv_adapter *adapt)
{
	return (uint32_t)RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_82));
}

static int mi300_smu_msg_allowed_in_sync_flood(struct amdgv_adapter *adapt, uint32_t msg)
{
	int ret = false;

	/* Ony ECC related Issues are guaranteed to be serviced by PMFW.
	   PMFW may hang or not respond to driver if any other message is sent.

	   Add this check as late as possible.
	*/
	if ((msg == PPSMC_MSG_QueryValidMcaCount)   ||
	    (msg == PPSMC_MSG_McaBankDumpDW) 	    ||
	    (msg == PPSMC_MSG_ClearMcaOnRead)       ||
	    (msg == PPSMC_MSG_QueryValidMcaCeCount) ||
	    (msg == PPSMC_MSG_McaBankCeDumpDW)      ||
	    (msg == PPSMC_MSG_GfxDriverReset)) {
		ret = true;
	}

	return ret;
}

int mi300_smu_send_msg_with_param(struct amdgv_adapter *adapt, uint32_t msg, uint32_t param,
				  uint32_t *arg)
{
	uint32_t resp;
	int ret = 0;

	if (oss_atomic_read(adapt->in_sync_flood) && !mi300_smu_msg_allowed_in_sync_flood(adapt, msg)) {
		AMDGV_ERROR("Skip msg:0x%x due to fatal error interrupt\n", msg);
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->pp.smu_lock);

	ret = mi300_smu_wait_for_response(adapt, &resp);
	if (ret) {
		AMDGV_ERROR(
			"smu is already hang before send msg:0x%x param:0x%08x resp value:%08x\n",
			msg, param, resp);
		ret = AMDGV_FAILURE;
		goto end;
	}

	mi300_smu_send_msg_nocheck(adapt, msg, param);

	ret = mi300_smu_wait_for_response(adapt, &resp);

	if (ret && (resp == 0)) {
		AMDGV_ERROR("Driver timed out waiting for msg:0x%x param:0x%08x. Resp value:%08x\n", msg, param, resp);
		ret = AMDGV_FAILURE;
		goto end;
	}

	if (ret && (resp != PPSMC_Result_OK)) {
		AMDGV_ERROR("smu responds with failure to msg:0x%x param:0x%08x. Resp value:%08x\n", msg, param, resp);
		ret = AMDGV_FAILURE;
		goto end;
	}

	if (arg) {
		*arg = mi300_smu_read_arg(adapt);
		AMDGV_DEBUG("smu send msg:%d param:0x%08x readback:0x%08x success\n", msg,
			    param, *arg);
	} else {
		AMDGV_DEBUG("smu send msg:%d param:0x%08x success\n", msg, param);
	}

end:
	oss_mutex_unlock(adapt->pp.smu_lock);

	return ret;
}

int mi300_smu_send_msg(struct amdgv_adapter *adapt, uint32_t msg, uint32_t *arg)
{
	return mi300_smu_send_msg_with_param(adapt, msg, 0, arg);
}

static int mi300_smu_send_test_msg(struct amdgv_adapter *adapt)
{
	uint32_t param, val;
	int ret;

	param = 0x22334455;

	ret = mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_TestMessage, param, &val);
	if (ret)
		return ret;

	if (val != (param + 1))
		return AMDGV_FAILURE;

	return 0;
}

int mi300_smu_get_version(struct amdgv_adapter *adapt, uint32_t *smu_version,
			  uint32_t *driver_if_version)
{
	int ret;

	if (!smu_version && !driver_if_version)
		return AMDGV_FAILURE;

	if (smu_version) {
		ret = mi300_smu_send_msg(adapt, PPSMC_MSG_GetSmuVersion, smu_version);
		if (ret)
			return ret;
	}

	if (driver_if_version) {
		ret = mi300_smu_send_msg(adapt, PPSMC_MSG_GetDriverIfVersion,
					 driver_if_version);
		if (ret)
			return ret;
	}

	return 0;
}

static int mi300_smu_record_version(struct amdgv_adapter *adapt)
{
	uint32_t smu_version;
	int ret;

	ret = mi300_smu_get_version(adapt, &smu_version, NULL);
	if (ret)
		return ret;

	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SMU] = smu_version;
	return 0;
}

static int mi300_smu_get_power_limit(struct amdgv_adapter *adapt, uint32_t *val)
{
	if (!val)
		return AMDGV_FAILURE;

	return mi300_smu_send_msg(adapt, PPSMC_MSG_GetPptLimit, val);
}

static int mi300_smu_check_fw_status(struct amdgv_adapter *adapt)
{
	uint32_t mp1_flags, mp1_intr_en = 0;
	int retries_max = 2, retries;

	for (retries = 0; retries < retries_max; retries++) {
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_PCIE_INDEX2),
		       (MP1_Public | (smnMP1_FIRMWARE_FLAGS & 0xffffffff)));
		mp1_flags = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_PCIE_DATA2));

		if (mp1_flags == 0xffffffff) {
			AMDGV_WARN("MP1_FIRMWARE_FLAGS read 0xffffffff, try again...\n");
			oss_msleep(100);
			continue;
		}

		mp1_intr_en = REG_GET_FIELD(mp1_flags, MP1_FIRMWARE_FLAGS, INTERRUPTS_ENABLED);
		break;
	}

	AMDGV_DEBUG("Read MP1 FW flags: 0x%x, retries: %d\n", mp1_flags, retries);

	return mp1_intr_en ? 0 : AMDGV_FAILURE;
}

bool mi300_smu_get_fw_loaded_status(struct amdgv_adapter *adapt)
{
	int ret;

	ret = mi300_smu_check_fw_status(adapt);
	if (ret)
		return false;

	ret = mi300_smu_send_test_msg(adapt);
	if (ret)
		return false;

	return true;
}

int mi300_gpu_mode1_reset(struct amdgv_adapter *adapt)
{
	uint32_t fatal_err = 0, param;

	/* send mode1 reset command to SMU (MP1) */
	AMDGV_DEBUG("sending mode1_reset command to SMU ...\n");

	param = PPSMC_RESET_TYPE_DRIVER_MODE_1_RESET;

	if (oss_atomic_read(adapt->in_ecc_recovery))
		fatal_err = 1;

	param |= (fatal_err << 16);

	/* After mode1_reset msg, all registers are blocked(ffffffff)
	 * until pci_cfg space is restored.Therefore we move MSG90 polling
	 * step	after we restore pf.
	 */
	mi300_smu_send_msg_nocheck(adapt, PPSMC_MSG_GfxDriverReset, param);

	/* allow time for all blocks to complete RESET */
	if (mi300_psp_wait_for_bootloader_steady(adapt) != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("mode1_reset timed out\n");
		return AMDGV_FAILURE;
	}

	oss_atomic_set(adapt->in_sync_flood, 0);

	return 0;
}

int mi300_wait_gpu_reset_completion(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t resp = 0;

	ret = mi300_smu_wait_for_response(adapt, &resp);
	if (ret == AMDGV_FAILURE || resp != PPSMC_Result_OK) {
		AMDGV_ERROR("Failed to send mode1 reset message 0x%x, response 0x%x\n",
		    PPSMC_MSG_GfxDriverReset, resp);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi300_smu_init_driver_smu_table(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	struct smu_local_memory *driver_mem = &table_context->driver_table_mem;
	struct smu_local_memory *tool_mem = &table_context->tool_table_mem;

	/* allocate space for driver table */
	driver_mem->mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, driver_mem->size,
							driver_mem->alignment, MEM_SMU_DRIVER_TABLE);
	if (!driver_mem->mem)
		return AMDGV_FAILURE;

	tool_mem->mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, tool_mem->size,
							tool_mem->alignment, MEM_SMU_TOOL_TABLE);
	if (!tool_mem->mem)
		return AMDGV_FAILURE;

	return 0;
}

static void mi300_smu_fini_driver_smu_table(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	struct smu_local_memory *driver_mem = &table_context->driver_table_mem;
	struct smu_local_memory *tool_mem = &table_context->tool_table_mem;

	if (driver_mem->mem) {
		amdgv_memmgr_free(driver_mem->mem);
		driver_mem->mem = NULL;
	}

	if (tool_mem->mem) {
		amdgv_memmgr_free(tool_mem->mem);
		tool_mem->mem = NULL;
	}
}

static int mi300_smu_table_context_setup(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	struct smu_local_memory *driver_mem = &table_context->driver_table_mem;
	struct smu_local_memory *tool_mem = &table_context->tool_table_mem;

	table_context->metrics_table_size = sizeof(MetricsTable_t);
	table_context->metrics_table = oss_zalloc(sizeof(MetricsTable_t));
	if (!table_context->metrics_table)
		return AMDGV_FAILURE;

	table_context->ecctable_array_size = sizeof(EccInfoTable_t) * adapt->mcp.num_aid;
	table_context->ecctable_array = oss_zalloc(table_context->ecctable_array_size);
	if (!table_context->ecctable_array)
		return AMDGV_FAILURE;

	driver_mem->alignment = PAGE_SIZE;
	driver_mem->size = max(driver_mem->size, table_context->metrics_table_size);
	driver_mem->size = max(driver_mem->size, table_context->ecctable_array_size);
	driver_mem->size = max(driver_mem->size, sizeof(SwI2cRequest_t));

	tool_mem->alignment = PAGE_SIZE;
	tool_mem->size = TOOL_SIZE;

	table_context->pptable_size = sizeof(PPTable_t);
	table_context->pptable = oss_zalloc(sizeof(PPTable_t));
	if (!table_context->pptable)
		return AMDGV_FAILURE;

	return 0;
}

static void mi300_smu_table_context_release(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;

	if (table_context->metrics_table) {
		oss_free(table_context->metrics_table);
		table_context->metrics_table = NULL;
	}

	if (table_context->ecctable_array) {
		oss_free(table_context->ecctable_array);
		table_context->ecctable_array = NULL;
	}

	if (table_context->pptable) {
		oss_free(table_context->pptable);
		table_context->pptable = NULL;
	}
}

static int mi300_smu_table_context_init(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context;
	int ret;

	table_context = oss_zalloc(sizeof(*table_context));
	if (!table_context)
		return AMDGV_FAILURE;

	smu->smu_table_context = table_context;

	ret = mi300_smu_table_context_setup(adapt);
	if (ret)
		goto err_free_table;

	ret = mi300_smu_init_driver_smu_table(adapt);
	if (ret)
		goto err_table_setup;

	return 0;

err_table_setup:
	mi300_smu_table_context_release(adapt);

err_free_table:
	oss_free(table_context);

	return ret;
}

static void mi300_smu_table_context_fini(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);

	mi300_smu_fini_driver_smu_table(adapt);

	mi300_smu_table_context_release(adapt);

	if (smu->smu_table_context) {
		oss_free(smu->smu_table_context);
		smu->smu_table_context = NULL;
	}
}

static bool mi300_smu_feature_is_enabled(struct amdgv_adapter *adapt, int fea_id)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	uint64_t features = smu->features;

	return !!(features & (1ULL << fea_id));
}

static int mi300_smu_select_policy_soc_pstate(struct amdgv_adapter *adapt,
						int policy)
{
	int ret, param;

	switch (policy) {
	case SOC_PSTATE_DEFAULT:
		param = 0;
		break;
	case SOC_PSTATE_0:
		param = 1;
		break;
	case SOC_PSTATE_1:
		param = 2;
		break;
	case SOC_PSTATE_2:
		param = 3;
		break;
	default:
		return AMDGV_FAILURE;
	}

	ret = mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_SelectPstatePolicy,
					      param, NULL);

	if (ret)
		AMDGV_ERROR("Select soc pstate policy %d failed!\n", policy);

	return ret;
}

static const struct mi300_smu_dpm_policy soc_pstate_policy = {
	.policy_type = AMDGV_PP_PM_POLICY_SOC_PSTATE,
	.policies = {
		{SOC_PSTATE_DEFAULT, "soc_pstate_default"},
		{SOC_PSTATE_0, "soc_pstate_0"},
		{SOC_PSTATE_1, "soc_pstate_1"},
		{SOC_PSTATE_2, "soc_pstate_2"}
	},
	.level_mask = (1 << SOC_PSTATE_DEFAULT) |
			(1 << SOC_PSTATE_0) | (1 << SOC_PSTATE_1) |
			(1 << SOC_PSTATE_2),
	.num_supported = SOC_PSTATE_COUNT,
	.set_policy = mi300_smu_select_policy_soc_pstate
};

static int mi300_smu_set_xgmi_plpd_mode(struct amdgv_adapter *adapt, int mode)
{
	uint32_t msg = 0, param = 0, version = 0;

	if (!mi300_smu_feature_is_enabled(adapt, FEATURE_XGMI_PER_LINK_PWR_DOWN)) {
		AMDGV_WARN("the feature FEATURE_XGMI_PER_LINK_PWR_DOWN feature isn't enabled, skip to set XGMI PLPD mode!\n");
		return 0;
	}

	/* NOTE: PLPD feature is enabled after PMFW 85.73.0 */
	if (adapt->pp.smu_fw_version < 0x00554900) {
		AMDGV_WARN("the current SMU PMFW (0x%08x) doesn't support XGMI PLPD feature\n", version);
		return 0;
	}

	/* NOTE: the PMFW is allowed set PLPD mode from different clients,
	 * so driver should be force DEFAULT mode when enable PLPD feature.
	 * */
	switch (mode) {
	case PP_XGMI_PLPD_MODE_ENABLE:
		msg = PPSMC_MSG_SelectPLPDMode;
		param = PPSMC_PLPD_MODE_DEFAULT;
		break;
	case PP_XGMI_PLPD_MODE_OPTIMIZED:
		msg = PPSMC_MSG_SelectPLPDMode;
		param = PPSMC_PLPD_MODE_OPTIMIZED;
		break;
	case PP_XGMI_PLPD_MODE_DISABLE:
		msg = PPSMC_MSG_GmiPwrDnControl;
		param = 0;
		break;
	default:
		return AMDGV_FAILURE;
	}

	return mi300_smu_send_msg_with_param(adapt, msg, param, NULL);
}

static const struct mi300_smu_dpm_policy plpd_policy = {
	.policy_type = AMDGV_PP_PM_POLICY_XGMI_PLPD,
	.policies = {
		{PLPD_DISALLOW, "plpd_disallow"},
		{PLPD_DEFAULT, "plpd_default"},
		{PLPD_OPTIMIZED, "plpd_optimized"}
	},
	.level_mask = (1 << PLPD_DISALLOW) |
					(1 << PLPD_DEFAULT) | (1 << PLPD_OPTIMIZED),
	.num_supported = PLPD_COUNT,
	.set_policy = mi300_smu_set_xgmi_plpd_mode
};

static int mi300_smu_dpm_context_setup(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_dpm_context *smu_dpm_context =
		(struct mi300_smu_dpm_context *)smu->smu_dpm_context;

	smu_dpm_context->dpm_policies =
		oss_zalloc(sizeof(struct mi300_smu_dpm_policy_ctxt));

	smu_dpm_context->dpm_policies->policies[0] = soc_pstate_policy;
	smu_dpm_context->dpm_policies->policies[0].current_level = SOC_PSTATE_DEFAULT;

	smu_dpm_context->dpm_policies->policies[1] = plpd_policy;
	smu_dpm_context->dpm_policies->policies[1].current_level = PLPD_DEFAULT;

	smu_dpm_context->dpm_policies->policy_mask |=
		(1 << AMDGV_PP_PM_POLICY_SOC_PSTATE) |
		(1 << AMDGV_PP_PM_POLICY_XGMI_PLPD);

	return 0;
}

static int mi300_smu_dpm_context_init(struct amdgv_adapter *adapt)
{
	int ret;
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_dpm_context *dpm_context;

	dpm_context = oss_zalloc(sizeof(*dpm_context));
	if (!dpm_context)
		return AMDGV_FAILURE;

	smu->smu_dpm_context = dpm_context;

	ret = mi300_smu_dpm_context_setup(adapt);

	return ret;
}

static void mi300_smu_dpm_context_fini(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_dpm_context *dpm_context = smu->smu_dpm_context;

	if (smu->smu_dpm_context) {
		if (dpm_context->dpm_policies) {
			oss_free(dpm_context->dpm_policies);
			dpm_context->dpm_policies = NULL;
		}
		oss_free(smu->smu_dpm_context);
		smu->smu_dpm_context = NULL;
	}
}

static int mi300_smu_context_init(struct amdgv_adapter *adapt)
{
	int ret;

	ret = mi300_smu_table_context_init(adapt);
	if (ret)
		return ret;

	ret = mi300_smu_dpm_context_init(adapt);
	if (ret)
		goto err_free_table_context;

	return 0;

err_free_table_context:
	mi300_smu_table_context_fini(adapt);

	return ret;
}

static void mi300_smu_context_fini(struct amdgv_adapter *adapt)
{
	mi300_smu_dpm_context_fini(adapt);
	mi300_smu_table_context_fini(adapt);
}

static const uint8_t mi300_smu_throttler_event_map[] = {
	[THROTTLER_PROCHOT_BIT] = PP_THROTTLER_EVENT__PROCHOT,
	[THROTTLER_THERMAL_SOCKET_BIT] = PP_THROTTLER_EVENT__SOCKET,
	[THROTTLER_THERMAL_HBM_BIT] = PP_THROTTLER_EVENT__HBM,
	[THROTTLER_THERMAL_VR_BIT] = PP_THROTTLER_EVENT__VR,
};

int mi300_smu_get_pm_policy(struct amdgv_adapter *adapt,
			enum amdgv_pp_pm_policy p_type,
			struct mi300_smu_dpm_policy **policy_int)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_dpm_context *dpm_ctxt =
					(struct mi300_smu_dpm_context *)(smu->smu_dpm_context);
	struct mi300_smu_dpm_policy_ctxt *policy_ctxt =
			dpm_ctxt->dpm_policies;
	bool policy_found = false;
	int ret = AMDGV_FAILURE;
	int i = 0;

	if (!policy_ctxt || !(policy_ctxt->policy_mask & BIT(p_type)))
		return ret;

	for_each_id(i, policy_ctxt->policy_mask) {
		if (policy_ctxt->policies[i].policy_type == p_type) {
			policy_found = true;
			*policy_int = &policy_ctxt->policies[i];
		}
	}

	if (!policy_found)
		return ret;

	return 0;
}

static int mi300_smu_set_pm_policy(struct amdgv_adapter *adapt,
			    struct mi300_smu_dpm_policy *dpm_policy,
			    int level)
{
	return dpm_policy->set_policy(adapt, level);
}

int mi300_smu_compare_and_set_pm_policy(struct amdgv_adapter *adapt,
				     enum amdgv_pp_pm_policy p_type,
				     int level)
{
	struct mi300_smu_dpm_policy *dpm_policy;
	int ret = AMDGV_FAILURE;

	if (mi300_smu_get_pm_policy(adapt, p_type, &dpm_policy))
		return ret;

	if (!(dpm_policy->level_mask & BIT(level)) || !dpm_policy->set_policy)
		return ret;

	if (dpm_policy->current_level == level)
		return 0;

	ret = mi300_smu_set_pm_policy(adapt, dpm_policy, level);
	if (!ret)
		dpm_policy->current_level = level;

	return ret;
}

static void mi300_smu_restore_pm_policy(struct amdgv_adapter *adapt,
					enum amdgv_pp_pm_policy p_type)
{
	struct mi300_smu_dpm_policy *policy;

	if (mi300_smu_get_pm_policy(adapt, p_type, &policy))
		return;
	if (mi300_smu_set_pm_policy(adapt, policy, policy->current_level))
		AMDGV_ERROR("Failed to restore PM Policy");

	return;
}

static void mi300_smu_restore_pm_policies(struct amdgv_adapter *adapt)
{
	enum amdgv_pp_pm_policy i = 0;

	for (i = 0; i < AMDGV_PP_PM_POLICY_NUM; i++)
		mi300_smu_restore_pm_policy(adapt, i);
}

/* Force apply policy. Do not update the SW cached state. */
int mi300_smu_error_inject_set_pm_policy(struct amdgv_adapter *adapt,
					 enum amdgv_pp_pm_policy p_type,
					 int level)
{
	struct mi300_smu_dpm_policy *dpm_policy;

	if (mi300_smu_get_pm_policy(adapt, p_type, &dpm_policy))
		return AMDGV_FAILURE;

	return mi300_smu_set_pm_policy(adapt, dpm_policy, level);
}

/* Force apply policy to last SW cached state */
void mi300_smu_error_inject_restore_pm_policy(struct amdgv_adapter *adapt,
					     enum amdgv_pp_pm_policy p_type)
{
	mi300_smu_restore_pm_policy(adapt, p_type);
}

static void mi300_smu_notify_throttler_error(struct amdgv_adapter *adapt,
					     uint32_t throttler_status)
{
	uint64_t throttler_event;

	throttler_event =
		smu_pp_throttler_event_convert(adapt, mi300_smu_throttler_event_map,
							 ARRAY_SIZE(mi300_smu_throttler_event_map),
							 (uint64_t)throttler_status);

	AMDGV_DEBUG("mi300 smu notify throttler status 0x%08x, throttler_event 0x%016llx\n",
		    throttler_status, throttler_event);
	amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_PP_THROTTLER_EVENT, throttler_event);
}

static int mi300_smu_pp_handle_irq(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	int ret = 0;
	uint32_t val, throttler_status;
	uint32_t ctx_id;
	uint32_t vf_flr_intr_sts;
	int i;
	union amdgv_sched_event_data data;

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
		 * window when host driver enables flr strap in mi300_reset_vf_flr()
		 */
		if (!(adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY))
			break;

		/* MI300 FLR interrupt format:
		 * ctx[0] = IH_INTERRUPT_VFFLR_INT
		 * ctx[1] = BIF_PF0_VF_FLR_INTR_STS
		 */
		vf_flr_intr_sts = entry->src_data[1];

		/* Only for KVM, only trigger FLR and clear FB if VF is active */
		for (i = 0; i < adapt->num_vf; i++) {
			if (vf_flr_intr_sts & (1 << i) &&
			    adapt->sched.array_vf[i].state == AMDGV_SCHED_ACTIVE) {
				ret = amdgv_sched_queue_event(
						adapt, i, AMDGV_EVENT_SCHED_FORCE_RESET_VF, AMDGV_SCHED_BLOCK_ALL);

				if (ret) {
					AMDGV_ERROR("Failed to trigger VFFLR for VF %d\n", i);
					ret = AMDGV_FAILURE;
					break;
				}

				/* Clear VF FB after triggering FLR */
				data.vf_fb_data.pattern = 0x0;
				data.vf_fb_data.flag = 1;

				ret = amdgv_sched_queue_event_ex(adapt, i,
						AMDGV_EVENT_SCHED_INIT_VF_FB, AMDGV_SCHED_BLOCK_ALL, data);
				if (ret) {
					AMDGV_ERROR("Failed to clear VF FB for VF %d\n", i);
					ret = AMDGV_FAILURE;
				}
				amdgv_live_info_prepare_reset(adapt);
			}
		}
		break;
	case IH_INTERRUPT_CONTEXT_ID_THERMAL_THROTTLING:
		throttler_status = entry->src_data[1];
		mi300_smu_notify_throttler_error(adapt, throttler_status);
		break;
	default:
		AMDGV_ERROR("mi300 smu can't process this context id %d\n", ctx_id);
		break;
	}

	return 0;
}

static int mi300_smu_irq_control(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t val;

	if (enable) {
		AMDGV_DEBUG("mi300 smu enable throttler interrupt\n");
		val = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT));
		val = REG_SET_FIELD(val, MP1_SMN_IH_SW_INT, ID, 0xFE);
		val = REG_SET_FIELD(val, MP1_SMN_IH_SW_INT, VALID, 0);
		WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT), val);

		val = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL));
		val = REG_SET_FIELD(val, MP1_SMN_IH_SW_INT_CTRL, INT_MASK, 0);
		WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL), val);
	} else {
		AMDGV_DEBUG("mi300 smu disable throttler interrupt\n");
		val = RREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL));
		val = REG_SET_FIELD(val, MP1_SMN_IH_SW_INT_CTRL, INT_MASK, 0);
		WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_IH_SW_INT_CTRL), val);
	}

	return 0;
}

int mi300_smu_mca_set_debug_mode(struct amdgv_adapter *adapt, bool enable)
{
	return mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_ClearMcaOnRead,
		enable ? 0 : ClearMcaOnRead_UE_FLAG_MASK | ClearMcaOnRead_CE_POLL_MASK, NULL);
}

static int mi300_smu_sw_init(struct amdgv_adapter *adapt)
{
	struct smu_context *smu;
	int ret;

	smu = oss_zalloc(sizeof(struct smu_context));
	if (!smu) {
		AMDGV_ERROR("Failed to alloc memory for smu context\n");
		return AMDGV_FAILURE;
	}

	adapt->pp.smu_backend = smu;

	ret = mi300_smu_context_init(adapt);
	if (ret)
		return ret;

	adapt->pp.drv_metrics_ext =
		oss_zalloc(sizeof(struct mi300_pp_drv_metrics_ext));

	adapt->pp.smu_lock = oss_mutex_init();
	if (adapt->pp.smu_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi300_smu_sw_fini(struct amdgv_adapter *adapt)
{
	mi300_smu_context_fini(adapt);
	if (adapt->pp.smu_backend) {
		oss_free(adapt->pp.smu_backend);
		adapt->pp.smu_backend = NULL;
	}

	if (adapt->pp.drv_metrics_ext) {
		oss_free(adapt->pp.drv_metrics_ext);
		adapt->pp.drv_metrics_ext = NULL;
	}

	if (adapt->pp.smu_lock) {
		oss_mutex_fini(adapt->pp.smu_lock);
		adapt->pp.smu_lock = OSS_INVALID_HANDLE;
	}

	return 0;
}

static int mi300_smu_set_driver_table_addr(struct amdgv_adapter *adapt, uint64_t addr)
{
	int ret;

	ret = mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_SetDriverDramAddrHigh, addr >> 32,
					    NULL);
	if (ret)
		return ret;

	ret = mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_SetDriverDramAddrLow,
					    addr & 0xffffffff, NULL);
	if (ret)
		return ret;

	return 0;
}

static int mi300_smu_set_tool_table_addr(struct amdgv_adapter *adapt, uint64_t addr)
{
	int ret;

	ret = mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_SetToolsDramAddrHigh, addr >> 32,
					    NULL);
	if (ret)
		return ret;

	ret = mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_SetToolsDramAddrLow,
					    addr & 0xffffffff, NULL);
	if (ret)
		return ret;

	return 0;
}

int mi300_smu_get_metrics_version(struct amdgv_adapter *adapt, uint32_t *version)
{
	if (!version)
		return AMDGV_FAILURE;

	return mi300_smu_send_msg(adapt, PPSMC_MSG_GetMetricsVersion, version);
}

static int mi300_smu_get_metrics_table(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	struct smu_local_memory *driver_mem = &table_context->driver_table_mem;
	uint32_t metrics_table_size = table_context->metrics_table_size;
	void *data = table_context->metrics_table;
	void *cpu_addr;
	int ret;

	ret = mi300_smu_send_msg(adapt, PPSMC_MSG_GetMetricsTable, NULL);
	if (ret)
		return ret;

	/* TODO: need invalidate hdp if enabled */
	cpu_addr = amdgv_memmgr_get_cpu_addr(driver_mem->mem);
	oss_memcpy(data, cpu_addr, metrics_table_size);

	return ret;
}

static int mi300_smu_set_table_address(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	struct smu_local_memory *driver_mem = &table_context->driver_table_mem;
	uint64_t addr;
	int ret;

	addr = amdgv_memmgr_get_gpu_addr(driver_mem->mem);
	ret = mi300_smu_set_driver_table_addr(adapt, addr);
	if (ret)
		return ret;

	return ret;
}

static int mi300_smu_get_enabled_features(struct amdgv_adapter *adapt, uint64_t *fea64)
{
	uint32_t feature[2];
	int ret;

	if (!fea64)
		return AMDGV_FAILURE;

	ret = mi300_smu_send_msg(adapt, PPSMC_MSG_GetEnabledSmuFeaturesHigh, &feature[1]);
	if (ret)
		return ret;

	ret = mi300_smu_send_msg(adapt, PPSMC_MSG_GetEnabledSmuFeaturesLow, &feature[0]);
	if (ret)
		return ret;

	*fea64 = (uint64_t)feature[1] << 32 | feature[0];

	return 0;
}

static const char *mi300_smu_get_feature_name(int id)
{
	switch (id) {
	case FEATURE_DATA_CALCULATION:
		return "FEATURE_DATA_CALCULATION";
	case FEATURE_DPM_CCLK:
		return "FEATURE_DPM_CCLK";
	case FEATURE_DPM_FCLK:
		return "FEATURE_DPM_FCLK";
	case FEATURE_DPM_GFXCLK:
		return "FEATURE_DPM_GFXCLK";
	case FEATURE_DPM_LCLK:
		return "FEATURE_DPM_LCLK";
	case FEATURE_DPM_SOCCLK:
		return "FEATURE_DPM_SOCCLK";
	case FEATURE_DPM_UCLK:
		return "FEATURE_DPM_UCLK";
	case FEATURE_DPM_VCN:
		return "FEATURE_DPM_VCN";
	case FEATURE_DPM_XGMI:
		return "FEATURE_DPM_XGMI";
	case FEATURE_DS_FCLK:
		return "FEATURE_DS_FCLK";
	case FEATURE_DS_GFXCLK:
		return "FEATURE_DS_GFXCLK";
	case FEATURE_DS_LCLK:
		return "FEATURE_DS_LCLK";
	case FEATURE_DS_MP0CLK:
		return "FEATURE_DS_MP0CLK";
	case FEATURE_DS_MP1CLK:
		return "FEATURE_DS_MP1CLK";
	case FEATURE_DS_MPIOCLK:
		return "FEATURE_DS_MPIOCLK";
	case FEATURE_DS_SOCCLK:
		return "FEATURE_DS_SOCCLK";
	case FEATURE_DS_VCN:
		return "FEATURE_DS_VCN";
	case FEATURE_APCC_DFLL:
		return "FEATURE_APCC_DFLL";
	case FEATURE_APCC_PLUS:
		return "FEATURE_APCC_PLUS";
	case FEATURE_DF_CSTATE:
		return "FEATURE_DF_CSTATE";
	case FEATURE_CC6:
		return "FEATURE_CC6";
	case FEATURE_PC6:
		return "FEATURE_PC6";
	case FEATURE_CPPC:
		return "FEATURE_CPPC";
	case FEATURE_PPT:
		return "FEATURE_PPT";
	case FEATURE_TDC:
		return "FEATURE_TDC";
	case FEATURE_THERMAL:
		return "FEATURE_THERMAL";
	case FEATURE_SOC_PCC:
		return "FEATURE_SOC_PCC";
	case FEATURE_CCD_PCC:
		return "FEATURE_CCD_PCC";
	case FEATURE_CCD_EDC:
		return "FEATURE_CCD_EDC";
	case FEATURE_PROCHOT:
		return "FEATURE_PROCHOT";
	case FEATURE_DVO_CCLK:
		return "FEATURE_DVO_CCLK";
	case FEATURE_FDD_AID_HBM:
		return "FEATURE_FDD_AID_HBM";
	case FEATURE_FDD_AID_SOC:
		return "FEATURE_FDD_AID_SOC";
	case FEATURE_FDD_XCD_EDC:
		return "FEATURE_FDD_XCD_EDC";
	case FEATURE_FDD_XCD_XVMIN:
		return "FEATURE_FDD_XCD_XVMIN";
	case FEATURE_FW_CTF:
		return "FEATURE_FW_CTF";
	case FEATURE_GFXOFF:
		return "FEATURE_GFXOFF";
	case FEATURE_SMU_CG:
		return "FEATURE_SMU_CG";
	case FEATURE_PSI7:
		return "FEATURE_PSI7";
	case FEATURE_CSTATE_BOOST:
		return "FEATURE_CSTATE_BOOST";
	case FEATURE_XGMI_PER_LINK_PWR_DOWN:
		return "FEATURE_XGMI_PER_LINK_PWR_DOWN";
	case FEATURE_CXL_QOS:
		return "FEATURE_CXL_QOS";
	case FEATURE_SOC_DC_RTC:
		return "FEATURE_SOC_DC_RTC";
	case FEATURE_GFX_DC_RTC:
		return "FEATURE_GFX_DC_RTC";
	case FEATURE_DVM_MIN_PSM:
		return "FEATURE_DVM_MIN_PSM";
	case FEATURE_PRC:
		return "FEATURE_PRC";
	default:
		break;
	}

	return "unknown feature";
}

static void mi300_smu_dump_feature_state(struct amdgv_adapter *adapt, uint64_t features)
{
	int i;

	AMDGV_DEBUG("SMU features: 0x%016llx\n", features);
	for (i = 0; i < 64; i++) {
		if (features & (1ULL << i)) {
			AMDGV_DEBUG("SMU %s (%d) Enabled\n", mi300_smu_get_feature_name(i), i);
		}
	}
}

static int mi300_smu_feature_control(struct amdgv_adapter *adapt, bool enable)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	int ret = 0;

	if (enable) {
		ret = mi300_smu_send_msg(adapt, PPSMC_MSG_EnableAllSmuFeatures, NULL);
		if (ret)
			return ret;

		ret = mi300_smu_get_enabled_features(adapt, &smu->features);
		if (ret)
			return ret;
		mi300_smu_dump_feature_state(adapt, smu->features);
	} else {
		if (!in_whole_gpu_reset()) {
			ret = mi300_smu_send_msg(adapt, PPSMC_MSG_PrepareForDriverUnload, NULL);
			if (ret)
				return ret;
		}

		smu->features = 0ULL;
	}

	return 0;
}

static int mi300_smu_set_tool_table_address(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	struct smu_local_memory *tool_mem = &table_context->tool_table_mem;
	uint64_t addr;
	int ret;

	addr = amdgv_memmgr_get_gpu_addr(tool_mem->mem);
	ret = mi300_smu_set_tool_table_addr(adapt, addr);
	if (ret)
		return ret;

	return ret;
}

static int mi300_smu_init_pptable(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	PPTable_t *pptable = (PPTable_t *)table_context->pptable;
	MetricsTable_t *metrics = table_context->metrics_table;
	int i, ret, retry = 100;

	uint32_t temp;

	/* metrics table may not update yet afetr driver send EnableAllSmuFeatures
	 * message to pmfw, add following check to avoid driver a empty data.
	 * */
	while (retry--) {
		ret = mi300_smu_get_metrics_table(adapt);
		if (ret)
			return ret;

		if (metrics->AccumulationCounter)
			break;

		oss_msleep(1);
	}

	if (!retry) {
		AMDGV_ERROR("mi300 smu get metrics table fail\n");
		return AMDGV_FAILURE;
	}

	pptable->MaxSocketPowerLimit = SMUQ10_ROUND(metrics->MaxSocketPowerLimit);

	pptable->MaxGfxclkFrequency = SMUQ10_ROUND(metrics->MaxGfxclkFrequency);
	pptable->MinGfxclkFrequency = SMUQ10_ROUND(metrics->MinGfxclkFrequency);

	for (i = 0; i < 4; i++) {
		pptable->FclkFrequencyTable[i] = SMUQ10_ROUND(metrics->FclkFrequencyTable[i]);
		pptable->UclkFrequencyTable[i] = SMUQ10_ROUND(metrics->UclkFrequencyTable[i]);
		pptable->SocclkFrequencyTable[i] = SMUQ10_ROUND(metrics->SocclkFrequencyTable[i]);
		pptable->VclkFrequencyTable[i] = SMUQ10_ROUND(metrics->VclkFrequencyTable[i]);
		pptable->DclkFrequencyTable[i] = SMUQ10_ROUND(metrics->DclkFrequencyTable[i]);
		pptable->LclkFrequencyTable[i] = SMUQ10_ROUND(metrics->LclkFrequencyTable[i]);
	}

	if ((adapt->pp.smu_fw_version >= 0x00554500 && adapt->asic_type == CHIP_MI300X) ||
		(adapt->asic_type == CHIP_MI308X)) {
		ret = mi300_smu_send_msg_with_param(adapt,
				PPSMC_MSG_GetCTFLimit, PPSMC_AID_THM_TYPE, &pptable->HotSpotCtfTemp);
		if (ret)
			return ret;

		ret = mi300_smu_send_msg_with_param(adapt,
				PPSMC_MSG_GetCTFLimit, PPSMC_XCD_THM_TYPE, &temp);
		if (ret)
			return ret;
		pptable->HotSpotCtfTemp = min(pptable->HotSpotCtfTemp, temp);

		ret = mi300_smu_send_msg_with_param(adapt,
				PPSMC_MSG_GetCTFLimit, PPSMC_HBM_THM_TYPE, &pptable->MemCtfTemp);
		if (ret)
			return ret;

		ret = mi300_smu_send_msg_with_param(adapt,
				PPSMC_MSG_GetPptLimit, 0, &pptable->DefaultPowerLimit);
		if (ret)
			return ret;
	} else {
		pptable->HotSpotCtfTemp = -1;
		pptable->MemCtfTemp = -1;
	}

	return 0;
}

static int mi300_smu_check_version(struct amdgv_adapter *adapt)
{
	uint32_t smu_version, driver_if_version;
	int ret;

	ret = mi300_smu_get_version(adapt, &smu_version, &driver_if_version);
	if (ret)
		return ret;

	AMDGV_INFO("SMU PMFW version:%08x, PMFW IF version:%08x, Driver IF version:%08x\n",
		   smu_version, driver_if_version, DRIVER_IF_MI300_VERSION);

	return 0;
}

static int mi300_smu_get_gfx_dpm_freq(struct amdgv_adapter *adapt, uint32_t *min,
				      uint32_t *max)
{
	int ret;

	if (min) {
		ret = mi300_smu_send_msg(adapt, PPSMC_MSG_GetMinGfxDpmFreq, min);
		if (ret)
			return ret;
	}

	if (max) {
		ret = mi300_smu_send_msg(adapt, PPSMC_MSG_GetMaxGfxDpmFreq, max);
		if (ret)
			return ret;
	}

	return 0;
}

static int mi300_smu_get_dpmfreq_by_index(struct amdgv_adapter *adapt, uint16_t clk_id,
					  uint16_t level, uint32_t *val)
{
	uint32_t param;

	if (clk_id >= PPCLK_COUNT || !val)
		return AMDGV_FAILURE;

	param = (uint32_t)clk_id << 16 | level;

	return mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_GetDpmFreqByIndex, param, val);
}

static int mi300_smu_get_dpmfreq_level_count(struct amdgv_adapter *adapt, uint16_t clk_id,
					     uint32_t *count)
{
	int ret;

	if (!count)
		return AMDGV_FAILURE;

	ret = mi300_smu_get_dpmfreq_by_index(adapt, clk_id, 0xff, count);
	if (!ret) {
		/* PMFW will return the max level index rather than the max level count */
		*count += 1;
	}

	return ret;
}

static int mi300_smu_set_gfx_dpm_table(struct amdgv_adapter *adapt,
				       struct mi300_dpm_table_entry *dpm_entry,
				       MetricsTable_t *metrics)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	PPTable_t *pptable = (PPTable_t *)table_context->pptable;
		uint32_t min, max;
	int ret;

	if (mi300_smu_feature_is_enabled(adapt, FEATURE_DPM_GFXCLK)) {
		ret = mi300_smu_get_gfx_dpm_freq(adapt, &min, &max);
		if (ret)
			return ret;
		dpm_entry->count = 2;
		dpm_entry->min = min;
		dpm_entry->max = max;
		dpm_entry->fine_grain = true;
		dpm_entry->levels[0].enabled = 1;
		dpm_entry->levels[0].value = min;
		dpm_entry->levels[1].enabled = 1;
		dpm_entry->levels[1].value = max;
	} else {
		dpm_entry->count = 1;
		dpm_entry->min = pptable->MinGfxclkFrequency;
		dpm_entry->max = pptable->MinGfxclkFrequency;
		dpm_entry->fine_grain = false;
		dpm_entry->levels[0].enabled = 1;
		dpm_entry->levels[0].value = dpm_entry->min;
	}

	return 0;
}

static int mi300_smu_set_other_dpm_table(struct amdgv_adapter *adapt, int clk_type,
					 struct mi300_dpm_table_entry *dpm_entry)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	PPTable_t *pptable = (PPTable_t *)table_context->pptable;
	uint32_t default_freq, dpm_levels, freq;
	uint16_t clk_id, fea_id;
	int i, ret;

	switch (clk_type) {
	case DPM_CLOCK_TYPE_FCLK:
		clk_id = PPCLK_FCLK;
		fea_id = FEATURE_DPM_FCLK;
		default_freq = pptable->FclkFrequencyTable[0];
		break;
	case DPM_CLOCK_TYPE_UCLK:
		clk_id = PPCLK_UCLK;
		fea_id = FEATURE_DPM_UCLK;
		default_freq = pptable->UclkFrequencyTable[0];
		break;
	case DPM_CLOCK_TYPE_SOCCLK:
		clk_id = PPCLK_SOCCLK;
		fea_id = FEATURE_DPM_SOCCLK;
		default_freq = pptable->SocclkFrequencyTable[0];
		break;
	case DPM_CLOCK_TYPE_VCLK:
		clk_id = PPCLK_VCLK;
		fea_id = FEATURE_DPM_VCN;
		default_freq = pptable->VclkFrequencyTable[0];
		break;
	case DPM_CLOCK_TYPE_DCLK:
		clk_id = PPCLK_DCLK;
		fea_id = FEATURE_DPM_VCN;
		default_freq = pptable->DclkFrequencyTable[0];
		break;
	case DPM_CLOCK_TYPE_LCLK:
		clk_id = PPCLK_LCLK;
		fea_id = FEATURE_DPM_LCLK;
		default_freq = pptable->LclkFrequencyTable[0];
		break;
	default:
		return AMDGV_FAILURE;
	}

	if (mi300_smu_feature_is_enabled(adapt, fea_id)) {
		ret = mi300_smu_get_dpmfreq_level_count(adapt, clk_id, &dpm_levels);
		if (ret)
			return ret;

		for (i = 0; i < dpm_levels; i++) {
			ret = mi300_smu_get_dpmfreq_by_index(adapt, clk_id, i, &freq);
			if (ret)
				return ret;
			dpm_entry->levels[i].value = freq;
			dpm_entry->levels[i].enabled = 1;
		}

		dpm_entry->count = dpm_levels;
		dpm_entry->min = dpm_entry->levels[0].value;
		dpm_entry->max = dpm_entry->levels[dpm_levels - 1].value;
	} else {
		dpm_entry->levels[0].value = default_freq;
		dpm_entry->levels[0].enabled = 1;
		dpm_entry->count = 1;
		dpm_entry->min = default_freq;
		dpm_entry->max = default_freq;
	}

	return 0;
}

static int mi300_smu_set_default_dpm_table(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_dpm_context *dpm_context =
		(struct mi300_smu_dpm_context *)smu->smu_dpm_context;
	struct mi300_dpm_table_entry *dpm_entry;
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	MetricsTable_t *metrics = (MetricsTable_t *)table_context->metrics_table;
	int clk_type, ret;

	ret = mi300_smu_get_metrics_table(adapt);
	if (ret)
		return ret;

	for (clk_type = 0; clk_type < DPM_CLOCK_TYPE_MAX; clk_type++) {
		dpm_entry = &dpm_context->dpm_entry[clk_type];
		switch (clk_type) {
		case DPM_CLOCK_TYPE_GFXCLK:
			ret = mi300_smu_set_gfx_dpm_table(adapt, dpm_entry, metrics);
			break;
		case DPM_CLOCK_TYPE_FCLK:
		case DPM_CLOCK_TYPE_UCLK:
		case DPM_CLOCK_TYPE_SOCCLK:
		case DPM_CLOCK_TYPE_VCLK:
		case DPM_CLOCK_TYPE_DCLK:
		case DPM_CLOCK_TYPE_LCLK:
			ret = mi300_smu_set_other_dpm_table(adapt, clk_type, dpm_entry);
			break;
		default:
			ret = AMDGV_FAILURE;
			break;
		}

		if (ret)
			return ret;
	}

	return ret;
}

static void mi300_pp_smu_clear_drv_metrics_ext(struct amdgv_adapter *adapt,
	struct mi300_pp_drv_metrics_ext *drv_metrics_ext)
{
	drv_metrics_ext->num_metric = 0;
	return;
}

static int mi300_pp_smu_add_drv_metrics_ext_entry(struct amdgv_adapter *adapt,
	struct mi300_pp_drv_metrics_ext *drv_metrics_ext,
	uint64_t metric_code,
	enum mi300_metric_addr_encoding addr_encoding,
	void *metric_addr,
	uint32_t vf_mask)
{
	uint32_t entry_idx = drv_metrics_ext->num_metric;

	if (entry_idx >= AMDGV_GPUMON_MAX_NUM_METRICS_EXT) {
		AMDGV_ERROR("Entry %d dropped. Chiplet Metrics table V1 cannot support more than %d entries\n",
			entry_idx, AMDGV_GPUMON_MAX_NUM_METRICS_EXT);
		return AMDGV_FAILURE;
	}

	drv_metrics_ext->metric[entry_idx].code = metric_code;
	drv_metrics_ext->metric[entry_idx].vf_mask = vf_mask;

	drv_metrics_ext->runtime_config[entry_idx].encoding = addr_encoding;
	drv_metrics_ext->runtime_config[entry_idx].addr = metric_addr;

	drv_metrics_ext->num_metric++;

	return 0;
}

static int mi300_pp_smu_init_drv_metrics_ext(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_pp_drv_metrics_ext *drv_metrics_ext =
		(struct mi300_pp_drv_metrics_ext *)adapt->pp.drv_metrics_ext;
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	MetricsTable_t *metrics_table = (MetricsTable_t *)table_context->metrics_table;
	struct mi300_smu_dpm_context *dpm_context = (struct mi300_smu_dpm_context *)smu->smu_dpm_context;
	uint32_t whole_gpu_vf_mask = ((adapt->num_vf - 1) | (1 << AMDGV_PF_IDX));
	uint32_t vf_mask = 0;
	uint32_t i = 0, uint_mask_bit;

	if (!drv_metrics_ext)
		return AMDGV_FAILURE;

	mi300_pp_smu_clear_drv_metrics_ext(adapt, drv_metrics_ext);

	/* FW Counter*/
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(ACC_COUNTER, 	METRIC_ACC_COUNTER,
				COUNTER, 	METRIC_EXT_FLAG(COUNTER)),
				UINT_32, 	(void *)&metrics_table->AccumulationCounter,
				whole_gpu_vf_mask);

	for (i = 0; i < MI300_SMU_NUM_XCC_CLK; i++) {
		vf_mask = amdgv_mcp_get_vf_mask_by_xcc(adapt, i);
		uint_mask_bit = i;
		/* GFX CLK */
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_GFX,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					Q10_32,		(void *)&metrics_table->GfxclkFrequency[i],
					vf_mask);
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_GFX,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_ACC) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					Q10_64,		(void *)&metrics_table->GfxclkFrequencyAcc[i],
					vf_mask);
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_GFX_MAX_LIMIT,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					UINT_32,	(void *)&dpm_context->dpm_entry[DPM_CLOCK_TYPE_GFXCLK].max,
					vf_mask);

		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_GFX_MIN_LIMIT,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					UINT_32,	(void *)&dpm_context->dpm_entry[DPM_CLOCK_TYPE_GFXCLK].min,
					vf_mask);

		/* Deep-sleep state */
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_GFX_DS_DISABLED,
					BOOL,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					Q10_32_DS,	(void *)&metrics_table->GfxclkFrequency[i],
					vf_mask);

		/* Clock locked */
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_GFX_LOCKED,
					BOOL,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					uint_mask_bit,	(void *)&metrics_table->GfxLockXCDMak,
					vf_mask);

		if ((adapt->pp.smu_fw_version >= 0x00556F00 && adapt->asic_type == CHIP_MI300X) ||
			(adapt->asic_type == CHIP_MI308X)) {
			/* Instant Usage */
			ADD_DRV_METRICS_EXT_ENTRY(
				METRIC_EXT_CODE(ACTIVITY, 	USAGE_GFX,
						PERCENT,	(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
						Q10_32,		(void *)&metrics_table->GfxBusy[i],
						vf_mask);
			/* Accumulated Usage */
			ADD_DRV_METRICS_EXT_ENTRY(
				METRIC_EXT_CODE(ACTIVITY, 	USAGE_GFX,
						PERCENT,	(METRIC_EXT_FLAG(DATA_FILTER_ACC) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
						Q10_64,		(void *)&metrics_table->GfxBusyAcc[i],
						vf_mask);
		}
	}

	/* UCLK */
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(FREQUENCY,		CLK_MEM,
				MHZ,			METRIC_EXT_FLAG(DATA_FILTER_INST)),
				Q10_32,			(void *)&metrics_table->UclkFrequency,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(FREQUENCY,		CLK_MEM_MAX_LIMIT,
				MHZ,			METRIC_EXT_FLAG(DATA_FILTER_INST)),
				UINT_32,		(void *)&dpm_context->dpm_entry[DPM_CLOCK_TYPE_UCLK].max,
				whole_gpu_vf_mask);

	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(FREQUENCY,		CLK_MEM_MIN_LIMIT,
				MHZ,			METRIC_EXT_FLAG(DATA_FILTER_INST)),
				UINT_32,		(void *)&dpm_context->dpm_entry[DPM_CLOCK_TYPE_UCLK].min,
				whole_gpu_vf_mask);

	ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_MEM_DS_DISABLED,
					BOOL,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					Q10_32_DS,	(void *)&metrics_table->UclkFrequency,
					vf_mask);

	for (i = 0; i < MI300_SMU_NUM_AID_CLK; i++) {
		vf_mask = amdgv_mcp_get_vf_mask_by_aid(adapt, i);
		/* SOC CLK */
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_SOC,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					Q10_32,		(void *)&metrics_table->SocclkFrequency[i],
					vf_mask);
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_SOC_MAX_LIMIT,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					UINT_32,	(void *)&dpm_context->dpm_entry[DPM_CLOCK_TYPE_SOCCLK].max,
					vf_mask);

		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_SOC_MIN_LIMIT,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					UINT_32,	(void *)&dpm_context->dpm_entry[DPM_CLOCK_TYPE_SOCCLK].min,
					vf_mask);

		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_SOC_DS_DISABLED,
					BOOL,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					Q10_32_DS,	(void *)&metrics_table->SocclkFrequency[i],
					vf_mask);

		/* VCLK */
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_VCLK,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					Q10_32,		(void *)&metrics_table->VclkFrequency[i],
					vf_mask);
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_VCLK_MAX_LIMIT,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					UINT_32,	(void *)&dpm_context->dpm_entry[DPM_CLOCK_TYPE_VCLK].max,
					vf_mask);

		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_VCLK_MIN_LIMIT,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					UINT_32,	(void *)&dpm_context->dpm_entry[DPM_CLOCK_TYPE_VCLK].min,
					vf_mask);

		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_VCLK_DS_DISABLED,
					BOOL,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					Q10_32_DS,	(void *)&metrics_table->VclkFrequency[i],
					vf_mask);

		/* DCLK */
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_DCLK,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					Q10_32,		(void *)&metrics_table->DclkFrequency[i],
					vf_mask);
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_DCLK_MAX_LIMIT,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					UINT_32,	(void *)&dpm_context->dpm_entry[DPM_CLOCK_TYPE_DCLK].max,
					vf_mask);

		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_DCLK_MIN_LIMIT,
					MHZ,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					UINT_32,	(void *)&dpm_context->dpm_entry[DPM_CLOCK_TYPE_DCLK].min,
					vf_mask);

		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(FREQUENCY,	CLK_DCLK_DS_DISABLED,
					BOOL,		(METRIC_EXT_FLAG(DATA_FILTER_INST) | METRIC_EXT_FLAG(CHIPLET_METRIC))),
					Q10_32_DS,	(void *)&metrics_table->DclkFrequency[i],
					vf_mask);

	}

	/* Instant Temperatures */
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(TEMPERATURE, 		TEMP_HOTSPOT_CURR,
				CELSIUS,		METRIC_EXT_FLAG(DATA_FILTER_INST)),
				Q10_32,			(void *)&metrics_table->MaxSocketTemperature,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(TEMPERATURE, 		TEMP_VR_CURR,
				CELSIUS,		METRIC_EXT_FLAG(DATA_FILTER_INST)),
				Q10_32,			(void *)&metrics_table->MaxVrTemperature,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(TEMPERATURE, 		TEMP_MEM_CURR,
				CELSIUS,		METRIC_EXT_FLAG(DATA_FILTER_INST)),
				Q10_32,			(void *)&metrics_table->MaxHbmTemperature,
				whole_gpu_vf_mask);

	/* Accumulated Temperatures */
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(TEMPERATURE, 		TEMP_HOTSPOT_CURR,
				CELSIUS,		METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_64,			(void *)&metrics_table->MaxSocketTemperatureAcc,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(TEMPERATURE, 		TEMP_VR_CURR,
				CELSIUS,		METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_64,			(void *)&metrics_table->MaxVrTemperatureAcc,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(TEMPERATURE, 		TEMP_MEM_CURR,
				CELSIUS,		METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_64,			(void *)&metrics_table->MaxHbmTemperatureAcc,
				whole_gpu_vf_mask);

	/* Power */
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(POWER, 			POWER_LIMIT,
				WATT,			METRIC_EXT_FLAG(DATA_FILTER_INST)),
				Q10_32,			(void *)&metrics_table->SocketPowerLimit,
				whole_gpu_vf_mask);

	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(POWER, 			POWER_CURR,
				WATT,			METRIC_EXT_FLAG(DATA_FILTER_INST)),
				Q10_32,			(void *)&metrics_table->SocketPower,
				whole_gpu_vf_mask);

	/* Energy */
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(ENERGY, 		ENERGY_SOCKET,
				JOULE,			METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_64,			(void *)&metrics_table->SocketEnergyAcc,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(ENERGY, 		ENERGY_XCD,
				JOULE,			METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_64,			(void *)&metrics_table->XcdEnergyAcc,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(ENERGY, 		ENERGY_AID,
				JOULE,			METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_64,			(void *)&metrics_table->AidEnergyAcc,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(ENERGY, 		ENERGY_MEM,
				JOULE,			METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_64,			(void *)&metrics_table->HbmEnergyAcc,
				whole_gpu_vf_mask);
	/* Instant Usage */
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(ACTIVITY, 		USAGE_GFX,
				PERCENT,		METRIC_EXT_FLAG(DATA_FILTER_INST)),
				Q10_32,			(void *)&metrics_table->SocketGfxBusy,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(ACTIVITY, 		USAGE_MEM,
				PERCENT,		METRIC_EXT_FLAG(DATA_FILTER_INST)),
				Q10_32,			(void *)&metrics_table->DramBandwidthUtilization,
				whole_gpu_vf_mask);
	/* Accumulated Usage */
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(ACTIVITY, 		USAGE_GFX,
				PERCENT,		METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_64,			(void *)&metrics_table->SocketGfxBusyAcc,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(ACTIVITY, 		USAGE_MEM,
				PERCENT,		METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_64,			(void *)&metrics_table->DramBandwidthUtilizationAcc,
				whole_gpu_vf_mask);
	/* DRAM Bandwidth */
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(ACTIVITY, 		DRAM_BANDWIDTH,
				GBPS,			METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_64,			(void *)&metrics_table->DramBandwidthAcc,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(ACTIVITY, 		MAX_DRAM_BANDWIDTH,
				GBPS,			METRIC_EXT_FLAG(DATA_FILTER_INST)),
				Q10_32,			(void *)&metrics_table->MaxDramBandwidth,
				whole_gpu_vf_mask);
	/* Accumulated PCIE Bandwidth */
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(PCIE, 			PCIE_BANDWIDTH,
				GBPS,			METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_64,			(void *)&metrics_table->PcieBandwidthAcc[0],
				whole_gpu_vf_mask);
	/* Throttling Counters */
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(THROTTLE, 		THROTTLE_PROCHOT_ACTIVE,
				BOOL,			METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_32,			(void *)&metrics_table->ProchotResidencyAcc,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(THROTTLE, 		THROTTLE_PPT_ACTIVE,
				BOOL,			METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_32,			(void *)&metrics_table->PptResidencyAcc,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(THROTTLE, 		THROTTLE_SOCKET_ACTIVE,
				BOOL,			METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_32,			(void *)&metrics_table->SocketThmResidencyAcc,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(THROTTLE, 		THROTTLE_VR_ACTIVE,
				BOOL,			METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_32,			(void *)&metrics_table->VrThmResidencyAcc,
				whole_gpu_vf_mask);
	ADD_DRV_METRICS_EXT_ENTRY(
		METRIC_EXT_CODE(THROTTLE, 		THROTTLE_MEM_ACTIVE,
				BOOL,			METRIC_EXT_FLAG(DATA_FILTER_ACC)),
				Q10_32,			(void *)&metrics_table->HbmThmResidencyAcc,
				whole_gpu_vf_mask);

	if ((adapt->pp.smu_fw_version >= 0x00555200 && adapt->asic_type == CHIP_MI300X) ||
		(adapt->asic_type == CHIP_MI308X)) {
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(PCIE, PCIE_BANDWIDTH,
				MBITPS, METRIC_EXT_FLAG(DATA_FILTER_INST)),
			Q10_32, (void *)&(metrics_table->PcieBandwidth[0]),
					whole_gpu_vf_mask);

		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(PCIE, PCIE_L0_TO_RECOVERY_COUNT,
				UINT, METRIC_EXT_FLAG(COUNTER)),
			UINT_32, (void *)&(metrics_table->PCIeL0ToRecoveryCountAcc),
					whole_gpu_vf_mask);
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(PCIE, PCIE_REPLAY_COUNT,
				UINT, METRIC_EXT_FLAG(COUNTER)),
			UINT_32, (void *)&(metrics_table->PCIenReplayAAcc),
					whole_gpu_vf_mask);
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(PCIE, PCIE_REPLAY_ROLLOVER_COUNT,
				UINT, METRIC_EXT_FLAG(COUNTER)),
			UINT_32, (void *)&(metrics_table->PCIenReplayARolloverCountAcc),
					whole_gpu_vf_mask);
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(PCIE, PCIE_NAK_SENT_COUNT,
				UINT, METRIC_EXT_FLAG(COUNTER)),
			UINT_32, (void *)&(metrics_table->PCIeNAKSentCountAcc),
					whole_gpu_vf_mask);
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(PCIE, PCIE_NAK_RECEIVED_COUNT,
				UINT, METRIC_EXT_FLAG(COUNTER)),
			UINT_32, (void *)&(metrics_table->PCIeNAKReceivedCountAcc),
					whole_gpu_vf_mask);
	}

	if ((adapt->pp.smu_fw_version >= 0x00555900 && adapt->asic_type == CHIP_MI300X) ||
		(adapt->asic_type == CHIP_MI308X)) {
		/* JPEG - 32 engines */
		for (i = 0; i < 32; i++) {
			ADD_DRV_METRICS_EXT_ENTRY(
				METRIC_EXT_CODE(ACTIVITY, 		USAGE_JPEG,
						PERCENT,		METRIC_EXT_FLAG(DATA_FILTER_INST)),
						Q10_32,			(void *)&(metrics_table->JpegBusy[i]),
						whole_gpu_vf_mask);
		}
		/* VCN - 4 engines */
		for (i = 0; i < 4; i++) {
			ADD_DRV_METRICS_EXT_ENTRY(
				METRIC_EXT_CODE(ACTIVITY, 		USAGE_VCN,
						PERCENT,		METRIC_EXT_FLAG(DATA_FILTER_INST)),
						Q10_32,			(void *)&(metrics_table->VcnBusy[i]),
						whole_gpu_vf_mask);
		}
	} else {
		/* JPEG - 32 engines */
		for (i = 0; i < 32; i++) {
			ADD_DRV_METRICS_EXT_ENTRY(
				METRIC_EXT_CODE(ACTIVITY, 		USAGE_JPEG,
						PERCENT,		METRIC_EXT_FLAG(DATA_FILTER_INST)),
						NOT_AVAILABLE,		NULL,
						whole_gpu_vf_mask);
		}
		/* VCN - 4 engines */
		for (i = 0; i < 4; i++) {
			ADD_DRV_METRICS_EXT_ENTRY(
				METRIC_EXT_CODE(ACTIVITY, 		USAGE_VCN,
						PERCENT,		METRIC_EXT_FLAG(DATA_FILTER_INST)),
						NOT_AVAILABLE,		NULL,
						whole_gpu_vf_mask);
		}
	}

	if ((adapt->pp.smu_fw_version >= 0x00556300 && adapt->asic_type == CHIP_MI300X) ||
		(adapt->asic_type == CHIP_MI308X)) {
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(PCIE, 		PCIE_LINK_SPEED,
					PCIE_GEN, 			METRIC_EXT_FLAG(DATA_FILTER_INST)),
					UINT_32, 	(void *)&(metrics_table->PCIeLinkSpeed),
					whole_gpu_vf_mask);
		ADD_DRV_METRICS_EXT_ENTRY(
			METRIC_EXT_CODE(PCIE, 		PCIE_LINK_WIDTH,
					PCIE_LANES, 		METRIC_EXT_FLAG(DATA_FILTER_INST)),
					UINT_32, 	(void *)&(metrics_table->PCIeLinkWidth),
					whole_gpu_vf_mask);
	}

	return 0;
};

static void mi300_smu_set_reset_quirks(struct amdgv_adapter *adapt)
{
	if ((adapt->asic_type == CHIP_MI308X &&
		(adapt->pp.smu_fw_version >= 0x05550800)) ||
		(adapt->asic_type == CHIP_MI300X && // MI325X
		(adapt->pp.smu_fw_version >= 0x07550100))) {
		adapt->flags |= AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY;
	}
}

static int mi300_smu_hw_init(struct amdgv_adapter *adapt)
{
	int ret;

	ret = mi300_smu_check_fw_status(adapt);
	if (ret)
		return ret;

	ret = mi300_smu_get_version(adapt, &adapt->pp.smu_fw_version, NULL);
	if (ret)
		return ret;

	mi300_smu_set_reset_quirks(adapt);

	ret = mi300_smu_check_version(adapt);
	if (ret)
		return ret;

	ret = mi300_smu_record_version(adapt);
	if (ret)
		return ret;

	ret = mi300_smu_set_table_address(adapt);
	if (ret)
		return ret;

	ret = mi300_smu_set_tool_table_address(adapt);
	if (ret)
		return ret;

	ret = mi300_smu_feature_control(adapt, true);
	if (ret)
		return ret;

	ret = mi300_smu_init_pptable(adapt);
	if (ret)
		return ret;

	ret = mi300_smu_irq_control(adapt, true);
	if (ret)
		return ret;

	/* Perform during HW init after VFs have been assigned to spatial
	 * partitions.
	*/
	ret = mi300_pp_smu_init_drv_metrics_ext(adapt);
	if (ret)
		return ret;

	return 0;
}

static int mi300_smu_hw_fini(struct amdgv_adapter *adapt)
{
	int ret;

	ret = mi300_smu_irq_control(adapt, false);
	if (ret)
		return ret;

	return mi300_smu_feature_control(adapt, false);
}

const struct amdgv_init_func mi300_smu_func = {
	.name = "mi300_smu_func",
	.sw_init = mi300_smu_sw_init,
	.sw_fini = mi300_smu_sw_fini,
	.hw_init = mi300_smu_hw_init,
	.hw_fini = mi300_smu_hw_fini,
};

static int mi300_smu_pp_get_power_capacity(struct amdgv_adapter *adapt, int *val)
{
	uint32_t tmp;
	int ret;

	if (!val)
		return AMDGV_FAILURE;

	ret = mi300_smu_get_power_limit(adapt, &tmp);
	if (ret)
		return ret;

	*val = (int)tmp;

	return 0;
}

static int mi300_smu_pp_set_power_capacity(struct amdgv_adapter *adapt, int val)
{
	return mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_SetPptLimit, val, NULL);
}

static int mi300_smu_pp_get_dpm_capacity(struct amdgv_adapter *adapt, int *val)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_dpm_context *dpm_context =
		(struct mi300_smu_dpm_context *)smu->smu_dpm_context;

	if (!val)
		return AMDGV_FAILURE;

	*val = (int)dpm_context->dpm_entry[DPM_CLOCK_TYPE_GFXCLK].count;

	return 0;
}

static int mi300_smu_pp_get_clock_limit(struct amdgv_adapter *adapt, enum pp_clock_type clk,
					enum pp_clock_limit_type limit_type, uint32_t *freq)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_dpm_context *dpm_context =
		(struct mi300_smu_dpm_context *)smu->smu_dpm_context;
	struct mi300_dpm_table_entry *dpm_entry;

	if (!freq)
		return AMDGV_FAILURE;

	switch (clk) {
	case PP_CLOCK_TYPE__GFX:
		dpm_entry = &dpm_context->dpm_entry[DPM_CLOCK_TYPE_GFXCLK];
		break;
	case PP_CLOCK_TYPE__FCLK:
		dpm_entry = &dpm_context->dpm_entry[DPM_CLOCK_TYPE_FCLK];
		break;
	case PP_CLOCK_TYPE__UCLK:
		dpm_entry = &dpm_context->dpm_entry[DPM_CLOCK_TYPE_UCLK];
		break;
	case PP_CLOCK_TYPE__SOC:
		dpm_entry = &dpm_context->dpm_entry[DPM_CLOCK_TYPE_SOCCLK];
		break;
	case PP_CLOCK_TYPE__VCLK:
	case PP_CLOCK_TYPE__VCLK_1:
		dpm_entry = &dpm_context->dpm_entry[DPM_CLOCK_TYPE_VCLK];
		break;
	case PP_CLOCK_TYPE__DCLK:
	case PP_CLOCK_TYPE__DCLK_1:
		dpm_entry = &dpm_context->dpm_entry[DPM_CLOCK_TYPE_DCLK];
		break;
	default:
		return AMDGV_FAILURE;
	}

	switch (limit_type) {
	case PP_CLOCK_LIMIT_TYPE__SOFT_MIN:
		*freq = dpm_entry->min;
		break;
	case PP_CLOCK_LIMIT_TYPE__SOFT_MAX:
		*freq = dpm_entry->max;
		break;
	case PP_CLOCK_LIMIT_TYPE__COUNT:
		*freq = dpm_entry->count;
		break;
	default:
		break;
	}

	return 0;
}

/* Return the lowest shutdown temp */
static int mi300_smu_pp_get_shutdown_temperature(struct amdgv_adapter *adapt, int *val)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	PPTable_t *pptable = (PPTable_t *)table_context->pptable;

	*val = pptable->HotSpotCtfTemp;

	return 0;
}

static bool mi300_smu_pp_is_ds_disabled(uint32_t curr_clk)
{
	return curr_clk < MI300_GPUMON_DS_THRESHOLD ? false : true;
}

static int mi300_smu_pp_get_pp_metrics(struct amdgv_adapter *adapt,
				       struct amdgv_gpumon_metrics *metrics)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	MetricsTable_t *metrics_table = (MetricsTable_t *)table_context->metrics_table;
	PPTable_t *pptable = (PPTable_t *)table_context->pptable;
	/* use xcd0/aid0 value default */
	int xcd = 0, aid = 0;
	uint32_t val;
	int ret;

	if (!metrics)
		return AMDGV_FAILURE;

	amdgv_gpumon_init_metrics_buf(metrics);

	ret = mi300_smu_get_metrics_table(adapt);
	if (ret)
		return ret;

	/* Update metrics */
	metrics->temp_hotspot = SMUQ10_ROUND(metrics_table->MaxSocketTemperature);
	metrics->temp_mem = SMUQ10_ROUND(metrics_table->MaxHbmTemperature);

	metrics->clocks[AMDGV_PP_CLK_GFX].curr =
		SMUQ10_ROUND(metrics_table->GfxclkFrequency[xcd]);
	metrics->clocks[AMDGV_PP_CLK_GFX].avg =
		SMUQ10_ROUND(metrics_table->GfxclkFrequency[xcd]);
	metrics->clocks[AMDGV_PP_CLK_GFX].avg_postDs =
		SMUQ10_ROUND(metrics_table->GfxclkFrequency[xcd]);
	/* inverting the value of ds_disabled to get the correct
		value for deep sleep enabled in smi-lib. */
	metrics->clocks[AMDGV_PP_CLK_GFX].ds_disabled =
		!mi300_smu_pp_is_ds_disabled(metrics->clocks[AMDGV_PP_CLK_GFX].curr);

	metrics->clocks[AMDGV_PP_CLK_MEM].curr = SMUQ10_ROUND(metrics_table->UclkFrequency);
	metrics->clocks[AMDGV_PP_CLK_MEM].avg = SMUQ10_ROUND(metrics_table->UclkFrequency);
	metrics->clocks[AMDGV_PP_CLK_MEM].ds_disabled =
		!mi300_smu_pp_is_ds_disabled(metrics->clocks[AMDGV_PP_CLK_MEM].curr);

	metrics->clocks[AMDGV_PP_CLK_SOC].curr =
		SMUQ10_ROUND(metrics_table->SocclkFrequency[aid]);
	metrics->clocks[AMDGV_PP_CLK_SOC].avg =
		SMUQ10_ROUND(metrics_table->SocclkFrequency[aid]);
	metrics->clocks[AMDGV_PP_CLK_SOC].ds_disabled =
		!mi300_smu_pp_is_ds_disabled(metrics->clocks[AMDGV_PP_CLK_SOC].curr);

	metrics->clocks[AMDGV_PP_CLK_VCLK_0].curr =
		SMUQ10_ROUND(metrics_table->VclkFrequency[aid]);
	metrics->clocks[AMDGV_PP_CLK_VCLK_0].avg =
		SMUQ10_ROUND(metrics_table->VclkFrequency[aid]);
	metrics->clocks[AMDGV_PP_CLK_VCLK_0].ds_disabled =
		!mi300_smu_pp_is_ds_disabled(metrics->clocks[AMDGV_PP_CLK_VCLK_0].curr);
	metrics->clocks[AMDGV_PP_CLK_VCLK_1].curr =
		SMUQ10_ROUND(metrics_table->VclkFrequency[aid]);
	metrics->clocks[AMDGV_PP_CLK_VCLK_1].avg =
		SMUQ10_ROUND(metrics_table->VclkFrequency[aid]);
	metrics->clocks[AMDGV_PP_CLK_VCLK_1].ds_disabled =
		!mi300_smu_pp_is_ds_disabled(metrics->clocks[AMDGV_PP_CLK_VCLK_1].curr);

	metrics->clocks[AMDGV_PP_CLK_DCLK_0].curr =
		SMUQ10_ROUND(metrics_table->DclkFrequency[aid]);
	metrics->clocks[AMDGV_PP_CLK_DCLK_0].avg =
		SMUQ10_ROUND(metrics_table->DclkFrequency[aid]);
	metrics->clocks[AMDGV_PP_CLK_DCLK_0].ds_disabled =
		!mi300_smu_pp_is_ds_disabled(metrics->clocks[AMDGV_PP_CLK_DCLK_0].curr);
	metrics->clocks[AMDGV_PP_CLK_DCLK_1].curr =
		SMUQ10_ROUND(metrics_table->DclkFrequency[aid]);
	metrics->clocks[AMDGV_PP_CLK_DCLK_1].avg =
		SMUQ10_ROUND(metrics_table->DclkFrequency[aid]);
	metrics->clocks[AMDGV_PP_CLK_DCLK_1].ds_disabled =
		!mi300_smu_pp_is_ds_disabled(metrics->clocks[AMDGV_PP_CLK_DCLK_1].curr);

	metrics->gfx_usage = SMUQ10_ROUND(metrics_table->SocketGfxBusy);
	metrics->mem_usage = SMUQ10_ROUND(metrics_table->DramBandwidthUtilization);
	metrics->power = SMUQ10_ROUND(metrics_table->SocketPower);
	metrics->power_limit = SMUQ10_ROUND(metrics_table->SocketPowerLimit);
	metrics->energy = SMUQ16_TO_UINT(metrics_table->SocketEnergyAcc);

	ret = mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_ReadThrottlerLimit,
					    PPSMC_THROTTLING_LIMIT_TYPE_HBM, &val);
	if (ret)
		return ret;

	metrics->temp_mem_limit = val;

	ret = mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_ReadThrottlerLimit,
					    PPSMC_THROTTLING_LIMIT_TYPE_SOCKET, &val);
	if (ret)
		return ret;

	metrics->temp_hotspot_limit = val;

	if ((adapt->pp.smu_fw_version >= 0x00556300 && adapt->asic_type == CHIP_MI300X) ||
		(adapt->asic_type == CHIP_MI308X)) {
		metrics->pcie_rate = metrics_table->PCIeLinkSpeed;
		metrics->pcie_width = metrics_table->PCIeLinkWidth;
	} else {
		metrics->pcie_rate = mi300_nbio_pcie_curr_link_speed(adapt);
		metrics->pcie_width = mi300_nbio_pcie_curr_link_width(adapt);
	}

	/* Return AID0 for now */
	metrics->serial = metrics_table->PublicSerialNumber_AID[0];

	metrics->temp_mem_shutdown = pptable->MemCtfTemp;
	metrics->temp_hotspot_shutdown = pptable->HotSpotCtfTemp;

	if ((adapt->pp.smu_fw_version >= 0x00555200 && adapt->asic_type == CHIP_MI300X) ||
		(adapt->asic_type == CHIP_MI308X)) {
		metrics->pcie_bandwidth = SMUQ10_ROUND(metrics_table->PcieBandwidth[0]);
		metrics->pcie_l0_to_recovery_count = metrics_table->PCIeL0ToRecoveryCountAcc;
		metrics->pcie_replay_count = metrics_table->PCIenReplayAAcc;
		metrics->pcie_replay_roll_over_count = metrics_table->PCIenReplayARolloverCountAcc;
		metrics->pcie_nak_sent_count = metrics_table->PCIeNAKSentCountAcc;
		metrics->pcie_nak_received_count = metrics_table->PCIeNAKReceivedCountAcc;
	}

	return 0;
}

static int mi300_smu_pp_is_clock_locked(struct amdgv_adapter *adapt,
					enum AMDGV_PP_CLK_DOMAIN clk_domain, uint8_t *clk_locked)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	MetricsTable_t *metrics_table = (MetricsTable_t *)table_context->metrics_table;
	int ret;

	ret = mi300_smu_get_metrics_table(adapt);
	if (ret)
		return AMDGV_FAILURE;

	*clk_locked = 0;

	switch (clk_domain) {
	case AMDGV_PP_CLK_GFX:
		if (metrics_table->GfxLockXCDMak) {
			*clk_locked = 1;
		}
		break;
	default:
		ret = AMDGV_FAILURE;
		break;
	}

	return ret;
}

static int mi300_pp_smu_get_max_configurable_power_limit(struct amdgv_adapter *adapt,
				int *power_limit)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	PPTable_t *pptable = (PPTable_t *)table_context->pptable;

	*power_limit = pptable->MaxSocketPowerLimit;

	return 0;
}

static int mi300_pp_smu_get_default_power_limit(struct amdgv_adapter *adapt,
				int *default_power)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	PPTable_t *pptable = (PPTable_t *)table_context->pptable;

	*default_power = pptable->DefaultPowerLimit;

	return 0;
}

static int mi300_pp_smu_get_metric_ext(struct amdgv_adapter *adapt,
	uint64_t *val,
	enum mi300_metric_addr_encoding addr_encoding,
	void *metric_addr)
{
	if (!val || !metric_addr) {
		AMDGV_ERROR("Invalid chiplet metric entry\n");
		return AMDGV_FAILURE;
	}

	switch (addr_encoding) {
	case UINT32_BIT0:
	case UINT32_BIT1:
	case UINT32_BIT2:
	case UINT32_BIT3:
	case UINT32_BIT4:
	case UINT32_BIT5:
	case UINT32_BIT6:
	case UINT32_BIT7:
		/* 0 - 7 bitshift */
		if (((*(uint32_t *)metric_addr) >> addr_encoding) & 1ULL) {
			*(bool *)val = true;
		} else {
			*(bool *)val = false;
		}
		break;
	case Q10_32:
		*(uint32_t *)val = SMUQ10_ROUND(*(uint32_t *)metric_addr);
		break;
	case Q10_32_DS:
		*(bool *)val = mi300_smu_pp_is_ds_disabled(SMUQ10_ROUND(*(uint32_t *)metric_addr));
		break;
	case Q10_64:
		*(uint64_t *)val = SMUQ10_ROUND(*(uint64_t *)metric_addr);
		break;
	case UINT_32:
		*(uint32_t *)val = *(uint32_t *)metric_addr;
		break;
	case UINT_64:
		*(uint64_t *)val = *(uint64_t *)metric_addr;
		break;
	case NOT_AVAILABLE:
		*(uint64_t *)val = -1;
		break;
	default:
		AMDGV_ERROR("Unknown metric encoding type!\n");
		*(uint32_t *)val = *(uint32_t *)metric_addr;
		break;
	}

	return 0;
}

static int mi300_pp_smu_update_drv_metrics_ext(struct amdgv_adapter *adapt,
	struct mi300_pp_drv_metrics_ext *drv_metrics_ext)
{
	uint32_t i = 0;
	for (i = 0; i < drv_metrics_ext->num_metric; i++) {
		mi300_pp_smu_get_metric_ext(adapt,
			&drv_metrics_ext->metric[i].val,
			drv_metrics_ext->runtime_config[i].encoding,
			drv_metrics_ext->runtime_config[i].addr);
	}

	return 0;
}

static int mi300_pp_smu_copy_drv_metric_ext_to_user(struct amdgv_adapter *adapt,
	struct mi300_pp_drv_metrics_ext *drv_metrics_ext,
	struct amdgv_gpumon_metrics_ext *metrics_ext)
{
	/* Always assume usermode allocated enough space for all entries */
	oss_memcpy(&metrics_ext->metric, &drv_metrics_ext->metric,
		sizeof(struct amdgv_gpumon_metric_ext) * drv_metrics_ext->num_metric);

	metrics_ext->num_metric = drv_metrics_ext->num_metric;

	return 0;
}

static int mi300_pp_smu_get_metrics_ext(struct amdgv_adapter *adapt,
				 struct amdgv_gpumon_metrics_ext *metrics_ext)
{
	struct mi300_pp_drv_metrics_ext *drv_metrics_ext =
		(struct mi300_pp_drv_metrics_ext *)adapt->pp.drv_metrics_ext;
	int ret;

	if (!drv_metrics_ext)
		return AMDGV_FAILURE;

	ret = mi300_smu_get_metrics_table(adapt);
	if (ret)
		return ret;

	ret = mi300_pp_smu_update_drv_metrics_ext(adapt, drv_metrics_ext);
	if (ret)
		return ret;

	ret = mi300_pp_smu_copy_drv_metric_ext_to_user(adapt,
		drv_metrics_ext, metrics_ext);
	if (ret)
		return ret;

	return ret;
}

static int mi300_pp_smu_get_num_metrics_ext_entries(struct amdgv_adapter *adapt,
				 uint32_t *entries)
{
	struct mi300_pp_drv_metrics_ext *drv_metrics_ext =
		(struct mi300_pp_drv_metrics_ext *)adapt->pp.drv_metrics_ext;

	if (!drv_metrics_ext)
		return AMDGV_FAILURE;

	*entries = drv_metrics_ext->num_metric;

	return 0;
}

static int mi300_pp_smu_is_pm_enabled(struct amdgv_adapter *adapt, bool *pm_enabled)
{
	struct smu_context *smu = adapt_to_smu(adapt);

	if (smu->features)
		*pm_enabled = true;
	else
		*pm_enabled = false;

	return 0;
}

int mi300_smu_trigger_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret = mi300_smu_send_msg_with_param(adapt,
						PPSMC_MSG_TriggerVFFLR,
						(1 << idx_vf),
						NULL);
	if (ret)
		AMDGV_ERROR("Trigger VF FLR failed\n");

	return ret;
}

int mi300_smu_trigger_mode_3_reset(struct amdgv_adapter *adapt, uint32_t xcc_mask)
{
	int ret = mi300_smu_send_msg_with_param(adapt,
						PPSMC_MSG_GfxDriverReset,
						((xcc_mask << 8) | PPSMC_RESET_TYPE_DRIVER_MODE_3_RESET),
						NULL);
	if (ret)
		AMDGV_ERROR("Trigger mode 3 reset failed\n");

	return ret;
}

int mi300_smu_gfx_flr_recovery(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret = mi300_smu_send_msg_with_param(adapt,
			PPSMC_MSG_GfxDriverResetRecovery, (1 << idx_vf), NULL);

	if (ret)
		AMDGV_ERROR("PMFW reset recovery failed\n");

	return ret;
}

#define MI300_SMU_XCC_MASK_ARG (1 << 31)

int mi300_smu_gfx_mode_3_recovery(struct amdgv_adapter *adapt, uint32_t xcc_mask)
{
	int ret = mi300_smu_send_msg_with_param(adapt,
			PPSMC_MSG_GfxDriverResetRecovery,
			MI300_SMU_XCC_MASK_ARG | xcc_mask,
			NULL);

	if (ret)
		AMDGV_ERROR("PMFW reset recovery failed\n");

	return ret;
}

static const uint8_t phy_port_to_aid_link_idx[] = {
	[0] = 0, //AID0-cake0, N/A data - used for PCIE
	[1] = 2, //AID1-cake0
	[2] = 3, //AID1-cake1
	[3] = 1, //AID0-cake1
	[4] = 6, //AID3-cake0
	[5] = 7, //AID3-cake1
	[6] = 5, //AID2-cake0
	[7] = 4, //AID2-cake1
};

static int mi300_smu_pp_get_link_metrics(struct amdgv_adapter *adapt,
			struct amdgv_gpumon_link_metrics *link_metrics)
{
	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	MetricsTable_t *metrics_table = (MetricsTable_t *)table_context->metrics_table;
	struct amdgv_xgmi_psp_link_info *psp_link_info;
	uint32_t i = 0, j = 0, port_id = 0;
	struct amdgv_hive_info *hive;
	struct amdgv_adapter *tmp_adapt = NULL;

	if (mi300_smu_get_metrics_table(adapt))
		return AMDGV_FAILURE;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive)
		return AMDGV_FAILURE;

	psp_link_info = &adapt->xgmi.link_info;

	for (j = 0; j < psp_link_info->num_links; j++) {
		amdgv_list_for_each_entry(tmp_adapt, &hive->adapt_list,
			struct amdgv_adapter, xgmi.head) {
			if (tmp_adapt->xgmi.node_id != psp_link_info->link[j].dest_node_id)
				continue;

			link_metrics->links[i].link_type = AMDGV_GPUMON_LINK_TYPE_XGMI3;
			link_metrics->links[i].bdf = tmp_adapt->bdf;

			port_id = phy_port_to_aid_link_idx[psp_link_info->link[j].src_port_id];
			link_metrics->links[i].read =
				SMUQ10_TO_UINT(metrics_table->XgmiReadDataSizeAcc[port_id]);
			link_metrics->links[i].write =
				SMUQ10_TO_UINT(metrics_table->XgmiWriteDataSizeAcc[port_id]);
			link_metrics->links[i].width = 16;
			link_metrics->links[i].speed = 32;
			i++;
		}
	}

	link_metrics->num_links = i;

	return 0;
}

static void mi300_smu_fill_eeprom_i2c_req(SwI2cRequest_t *req, bool write, uint8_t address,
					  uint8_t i2c_port, uint8_t *data, uint32_t numbytes)
{
	int i;

	/* numbytes should not exceed MAX_SW_I2C_COMMANDS */
	req->I2CcontrollerPort = i2c_port;
	req->I2CSpeed = I2C_SPEED_FAST_400K;
	req->SlaveAddress = address;
	req->NumCmds = numbytes;

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

static int mi300_smu_request_i2c_transaction(struct amdgv_adapter *adapt,
					     SwI2cRequest_t *in_req, SwI2cRequest_t *out_req)
{
	void *cpu_addr;
	int ret;

	struct smu_context *smu = adapt_to_smu(adapt);
	struct mi300_smu_table_context *table_context =
		(struct mi300_smu_table_context *)smu->smu_table_context;
	struct smu_local_memory *driver_mem = &table_context->driver_table_mem;
	if (!(driver_mem && driver_mem->mem)) {
		AMDGV_ERROR("smu_table_context->driver_table_mem is not initialized");
		return AMDGV_FAILURE;
	}
	cpu_addr = amdgv_memmgr_get_cpu_addr(driver_mem->mem);
	ret = AMDGV_FAILURE;

	if (cpu_addr != NULL) {
	if (in_req)
		oss_memcpy(cpu_addr, in_req, sizeof(*in_req));

	ret = mi300_smu_send_msg(adapt, PPSMC_MSG_RequestI2cTransaction, NULL);
	if (ret)
		return ret;

	if (out_req)
		oss_memcpy(out_req, cpu_addr, sizeof(*out_req));
	}

	return ret;
}

static int mi300_smu_i2c_eeprom_read_data(struct amdgv_adapter *adapt, uint8_t address,
					  uint8_t i2c_port, uint8_t *data, uint32_t numbytes)
{
	SwI2cRequest_t req;
	int i, ret = 0;
	int retry_count = 1;

	oss_memset(&req, 0, sizeof(req));
	mi300_smu_fill_eeprom_i2c_req(&req, false, address, i2c_port, data, numbytes);

	/* Now read data starting with that address */
	while (retry_count <= 5) {
		ret = mi300_smu_request_i2c_transaction(adapt, &req, &req);

		if (ret == 0)
			break;

		retry_count++;
		oss_msleep(200);
	}

	if ((ret == 0) && (retry_count != 1))
		AMDGV_INFO("i2c_eeprom_read_data - success @ :%dth try\n", retry_count);

	if (ret) {
		AMDGV_WARN("i2c_eeprom_read_data - error occurred :%x\n", ret);
		return ret;
	}

	/* Assume SMU fills req.SwI2cCmds[i].Data with read bytes */
	for (i = 0; i < numbytes; i++)
		data[i] = req.SwI2cCmds[i].ReadWriteData;

	AMDGV_DEBUG("i2c_eeprom_read_data, address = %x, bytes = %d\n", (uint16_t)address,
		    numbytes);

	return ret;
}

static int mi300_smu_i2c_eeprom_write_data(struct amdgv_adapter *adapt, uint8_t address,
					   uint8_t i2c_port, uint8_t *data, uint32_t numbytes)
{
	SwI2cRequest_t req;
	int ret = 0;
	int retry_count = 1;

	oss_memset(&req, 0, sizeof(req));
	mi300_smu_fill_eeprom_i2c_req(&req, true, address, i2c_port, data, numbytes);

	while (retry_count <= 5) {
		ret = mi300_smu_request_i2c_transaction(adapt, &req, NULL);

		if (ret == 0)
			break;

		retry_count++;
		oss_msleep(200);
	}

	if ((ret == 0) && (retry_count != 1))
		AMDGV_INFO("i2c_eeprom_write_data - success @ :%dth try\n", retry_count);

	if (ret) {
		AMDGV_WARN("i2c_write- error occurred :%x\n", ret);
		return ret;
	}

	AMDGV_DEBUG("i2c_write(), address = %x, bytes = %d , data: \n", (uint16_t)address,
		    numbytes);
	/*
	 * According to EEPROM spec there is a MAX of 10 ms required for
	 * EEPROM to flush internal RX buffer after STOP was issued at the
	 * end of write transaction. During this time the EEPROM will not be
	 * responsive to any more commands - so wait a bit more.
	 */
	oss_msleep(10);

	return ret;
}

#define MI300_SMU_I2C_MAX_CHUNK_SIZE 256
#define MI300_SMU_I2C_ADDR_SIZE 2

static inline int mi300_smu_pp_i2c_page_allowed_xfer_len(uint32_t addr)
{
	return (MI300_SMU_I2C_MAX_CHUNK_SIZE - (addr % MI300_SMU_I2C_MAX_CHUNK_SIZE));
}

static int mi300_smu_pp_i2c_eeprom_i2c_xfer(struct amdgv_adapter *adapt, uint8_t port,
					    struct i2c_msg *msgs, int num)
{
	uint32_t data_size, data_chunk_size, next_eeprom_addr = 0;
	uint8_t *data_ptr, data_chunk[MI300_SMU_I2C_MAX_CHUNK_SIZE + MI300_SMU_I2C_ADDR_SIZE] = { 0 };
	int i, ret;

	for (i = 0; i < num; i++) {
		/*
		 * SMU interface allows at most MAX_SW_I2C_COMMANDS bytes of data within 256byte page alignment.
		 * Hence the data needs to be spliced into chunks and sent each
		 * chunk separately
		 */
		data_size = msgs[i].len - MI300_SMU_I2C_ADDR_SIZE;
		next_eeprom_addr = (msgs[i].buf[0] << 8 & 0xff00) | (msgs[i].buf[1] & 0xff);
		data_ptr = msgs[i].buf + MI300_SMU_I2C_ADDR_SIZE;
		while (data_size) {
			data_chunk_size = min(mi300_smu_pp_i2c_page_allowed_xfer_len(next_eeprom_addr),
								MAX_SW_I2C_COMMANDS - MI300_SMU_I2C_ADDR_SIZE);
			data_chunk_size = min(data_size, data_chunk_size);

			/* Insert the EEPROM dest addess, bits 0-15 */
			data_chunk[0] = ((next_eeprom_addr >> 8) & 0xff);
			data_chunk[1] = (next_eeprom_addr & 0xff);

			if (msgs[i].flags & I2C_M_RD) {
				ret = mi300_smu_i2c_eeprom_read_data(adapt,
										(uint8_t)msgs[i].addr, port,
										data_chunk, data_chunk_size + MI300_SMU_I2C_ADDR_SIZE);
				oss_memcpy(data_ptr, data_chunk + MI300_SMU_I2C_ADDR_SIZE, data_chunk_size);
			} else {
				oss_memcpy(data_chunk + MI300_SMU_I2C_ADDR_SIZE, data_ptr, data_chunk_size);
				ret = mi300_smu_i2c_eeprom_write_data(adapt,
										(uint8_t)msgs[i].addr, port,
										data_chunk, data_chunk_size + MI300_SMU_I2C_ADDR_SIZE);
			}

			if (ret) {
				num = AMDGV_FAILURE;
				goto fail;
			}

			next_eeprom_addr += data_chunk_size;
			data_size -= data_chunk_size;
			data_ptr += data_chunk_size;
		}
	}

fail:
	return num;
}

static int mi300_smu_send_hbm_bad_pages_num(struct amdgv_adapter *adapt, uint32_t size)
{
	return mi300_smu_send_msg_with_param(adapt, PPSMC_MSG_SetNumBadHbmPagesRetired, size, NULL);
}

static int mi300_smu_set_df_cstate(struct amdgv_adapter *adapt,
				     enum pp_df_cstate state)
{
	return mi300_smu_send_msg_with_param(adapt,
					       PPSMC_MSG_DFCstateControl,
					       state,
					       NULL);
}

static int mi300_parse_smu_table_info(struct amdgv_adapter *adapt)
{
	mi300_smu_init_pptable(adapt);
	mi300_smu_set_default_dpm_table(adapt);



	return 0;
}

static int mi300_send_rma_reason(struct amdgv_adapter *adapt, enum pp_rma_reason rma_reason)
{
	int ret;

	if (adapt->pp.smu_fw_version < 0x00555A00 && adapt->asic_type == CHIP_MI300X)
		return 0;

	switch (rma_reason) {
	case PP_RMA_BAD_PAGE_THRESHOLD:
		ret = mi300_smu_send_msg(adapt, PPSMC_MSG_RmaDueToBadPageThreshold, NULL);
		break;

	default:
		ret = AMDGV_FAILURE;
		AMDGV_ERROR("RMA Reason unsupported by PMFW\n");
		break;
	}

	return ret;
}

static const struct amdgv_pp_funcs mi300_amdgv_pp_funcs = {
	.handle_smu_irq = mi300_smu_pp_handle_irq,
	.i2c_eeprom_xfer = mi300_smu_pp_i2c_eeprom_i2c_xfer,
	.get_power_capacity = mi300_smu_pp_get_power_capacity,
	.set_power_capacity = mi300_smu_pp_set_power_capacity,
	.get_dpm_capacity = mi300_smu_pp_get_dpm_capacity,
	.get_clock_limit = mi300_smu_pp_get_clock_limit,
	.get_pp_metrics = mi300_smu_pp_get_pp_metrics,
	.is_clock_locked = mi300_smu_pp_is_clock_locked,
	.get_max_configurable_power_limit = mi300_pp_smu_get_max_configurable_power_limit,
	.get_default_power_limit = mi300_pp_smu_get_default_power_limit,
	.get_metrics_ext = mi300_pp_smu_get_metrics_ext,
	.get_num_metrics_ext_entries = mi300_pp_smu_get_num_metrics_ext_entries,
	.parse_smu_table_info = mi300_parse_smu_table_info,
	.is_pm_enabled = mi300_pp_smu_is_pm_enabled,
	.trigger_vf_flr = mi300_smu_trigger_vf_flr,
	.get_link_metrics = mi300_smu_pp_get_link_metrics,
	.get_shutdown_temperature = mi300_smu_pp_get_shutdown_temperature,
	.get_fru_product_info = mi300_fru_get_product_info,
	.send_hbm_bad_pages_num = mi300_smu_send_hbm_bad_pages_num,
	.set_df_cstate = mi300_smu_set_df_cstate,
	.send_rma_reason = mi300_send_rma_reason,
};

static int mi300_powerplay_sw_init(struct amdgv_adapter *adapt)
{
	adapt->pp.pp_funcs = &mi300_amdgv_pp_funcs;

	return 0;
}

static int mi300_powerplay_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_powerplay_hw_init(struct amdgv_adapter *adapt)
{
	int ret;

	ret = mi300_smu_check_fw_status(adapt);
	if (ret)
		return ret;

	/* TODO: NOTE: after enable gpuiov, smu resp reg will be auto cleard,
	 * Setting it to 1 indicates that the last SMU message was sent normally.
	 */
	WREG32(SOC15_REG_OFFSET(MP1, 0, regMP1_SMN_C2PMSG_90), 1);

	ret = mi300_smu_send_test_msg(adapt);
	if (ret)
		return ret;

	ret = mi300_smu_set_default_dpm_table(adapt);
	if (ret)
		return ret;

	ret = mi300_smu_mca_set_debug_mode(adapt, is_debug_mode_ras_smu());
	if (ret)
		return ret;

	ret = mi300_smu_set_xgmi_plpd_mode(adapt, PP_XGMI_PLPD_MODE_ENABLE);
	if (ret)
		return ret;

	if (in_whole_gpu_reset())
		mi300_smu_restore_pm_policies(adapt);

	/* Always query on init for CPER generation */
	if (!adapt->product_info.visit)
		mi300_fru_get_product_info(adapt);


	return 0;
}

static int mi300_powerplay_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mi300_powerplay_func = {
	.name = "mi300_powerplay_func",
	.sw_init = mi300_powerplay_sw_init,
	.sw_fini = mi300_powerplay_sw_fini,
	.hw_init = mi300_powerplay_hw_init,
	.hw_fini = mi300_powerplay_hw_fini,
};

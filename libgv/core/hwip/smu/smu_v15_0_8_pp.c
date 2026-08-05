/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include "smu_v15_0_8_internal.h"
#include "smu_v15_0_8_pp.h"

static const uint32_t this_block = AMDGV_POWER_BLOCK;

#define ADD_DRV_METRICS_ENTRY(drv_metric_code, vf_mask, res_instance, encoding, addr)		\
	smu_v15_0_8_pp_add_drv_metrics_entry(adapt, drv_metrics,				\
					  metric_code[drv_metric_code],				\
					  encoding, (void *)addr, vf_mask, res_instance)

static const uint8_t smu_v15_0_8_pp_throttler_event_map[] = {
	[THROTTLER_PROCHOT_BIT] = AMDGV_PP_THROTTLER_EVENT__PROCHOT,
	[THROTTLER_THERMAL_SOCKET_BIT] = AMDGV_PP_THROTTLER_EVENT__SOCKET,
	[THROTTLER_THERMAL_HBM_BIT] = AMDGV_PP_THROTTLER_EVENT__HBM,
	[THROTTLER_THERMAL_VR_BIT] = AMDGV_PP_THROTTLER_EVENT__VR,
};

static int smu_v15_0_8_pp_metrics_init(struct amdgv_adapter *adapt);
static int smu_v15_0_8_pp_save_product_info(struct amdgv_adapter *adapt);

static int smu_v15_0_8_pp_add_drv_metrics_entry(struct amdgv_adapter *adapt,
						struct drv_metrics *drv_metrics_ext,
						uint64_t metric_code,
						enum metric_addr_encoding addr_encoding,
						void *metric_addr, uint32_t vf_mask,
						uint32_t res_instance)
{
	uint32_t idx = drv_metrics_ext->num_metric;

	if (idx >= MAX_NUM_METRICS) {
		AMDGV_ERROR("Entry %d dropped. "
			    "drv_metrics table cannot support more than %d entries\n",
			    idx, MAX_NUM_METRICS);
		return AMDGV_FAILURE;
	}

	drv_metrics_ext->metric[idx].code = metric_code;
	drv_metrics_ext->metric[idx].vf_mask = vf_mask;
	drv_metrics_ext->metric[idx].res_instance = res_instance;

	drv_metrics_ext->runtime_config[idx].encoding = addr_encoding;
	drv_metrics_ext->runtime_config[idx].addr = metric_addr;

	drv_metrics_ext->num_metric++;

	return 0;
}

static int smu_v15_0_8_pp_get_fw_table(struct amdgv_adapter *adapt, enum smu_table_type type,
				       bool force)
{
	struct smu_context *smu = ADAPT_TO_SMU(adapt);
	struct smu_v15_0_8_table_context *table_context = SMU_TO_TABLE_CONTEXT(smu);
	struct smu_local_memory *driver_xchg_mem = &table_context->driver_xchg_mem;
	void *src = amdgv_memmgr_get_cpu_addr(driver_xchg_mem->mem);
	int ret = 0;
	struct smu_15_0_8_msg msg = { 0 };
	struct smu_table *entry;

	if (type >= SMU_TABLE__NUM)
		return AMDGV_FAILURE;

	entry = &table_context->tables[type];

	if (force || (amdgv_after_time(entry->timestamp + adapt->pp.metrics_cache_expire_us))) {
		msg.id = entry->msg;
		ret = smu_v15_0_8_send_msg(adapt, &msg);
		if (ret)
			return ret;

		oss_memcpy(entry->table, src, entry->size);
		entry->timestamp = oss_get_time_stamp();
	}

	return ret;
}

static int smu_v15_0_8_pp_gpu_mode2_reset(struct amdgv_adapter *adapt)
{
	struct smu_15_0_8_msg msg = { 0 };
	int ret;

	msg.id = PPSMC_MSG_GfxDriverReset;
	msg.in_arg[0] = PPSMC_RESET_TYPE_DRIVER_MODE_2_RESET;

	ret = smu_v15_0_8_send_msg(adapt, &msg);
	if (ret)
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_PP_MODE2_RESET_FAIL, 0);

	oss_atomic_set(adapt->in_sync_flood, 0);

	return ret;
}

static int smu_v15_0_8_pp_trigger_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct smu_15_0_8_msg msg = { 0 };

	msg.id = PPSMC_MSG_TriggerVFFLR;
	msg.in_arg[0] = BIT(idx_vf);

	return smu_v15_0_8_send_msg(adapt, &msg);
}

static int smu_v15_0_8_pp_gpu_mode0_reset(struct amdgv_adapter *adapt)
{
	struct smu_15_0_8_msg msg = { 0 };
	int ret;

	msg.id = PPSMC_MSG_GfxDriverReset;
	msg.in_arg[0] = PPSMC_RESET_TYPE_DRIVER_MODE_0_RESET;

	ret = smu_v15_0_8_send_msg(adapt, &msg);
	if (ret)
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_PP_MODE0_RESET_FAIL, 0);

	oss_atomic_set(adapt->in_sync_flood, 0);

	return ret;
}

static int smu_v15_0_8_pp_reset_vf_arbiters(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	AMDGV_WARN("VF arbiter reset unsupported by PMFW!\n");

	return 0;
}

static void smu_v15_0_8_pp_notify_throttler_error(struct amdgv_adapter *adapt,
					     uint32_t throttler_status)
{
	uint64_t throttler_event;

	throttler_event =
		smu_pp_throttler_event_convert(adapt, smu_v15_0_8_pp_throttler_event_map,
					       ARRAY_SIZE(smu_v15_0_8_pp_throttler_event_map),
					       (uint64_t)throttler_status);

	amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_PP_THROTTLER_EVENT, throttler_event);
}

static int smu_v15_0_8_pp_handle_irq(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	int ret = 0;
	uint32_t vf_flr_intr_sts;
	uint32_t i;
	uint64_t curr_time, throttle_delta;

	if (entry->client_id != IH_IV_CLIENTID_MP1 || entry->src_id != IH_INTERRUPT_ID_TO_DRIVER)
		return ret;

	smu_v15_0_8_ack_irq(adapt);

	switch (entry->src_data[0]) {
	case IH_INTERRUPT_VFFLR_INT:
		/* avoid the possible race condition that some VM is just destroyed within the short
		 * window when host driver enables flr strap in mi300_reset_vf_flr() */
		if (!(adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY))
			break;

		vf_flr_intr_sts = entry->src_data[1];

		/* only trigger FLR if VF is active */
		for_each_id(i, vf_flr_intr_sts) {
			if (i >= adapt->num_vf)
				break;

			if (adapt->sched.array_vf[i].state != AMDGV_SCHED_ACTIVE)
				continue;

			ret = amdgv_sched_queue_event(adapt, i, AMDGV_EVENT_SCHED_FORCE_RESET_VF,
						      AMDGV_SCHED_BLOCK_ALL);
			if (ret)
				break;

			amdgv_sched_clear_dirty_vf_fb(adapt, i);
			amdgv_live_info_prepare_reset(adapt);
		}
		break;
	case IH_INTERRUPT_CONTEXT_ID_THERMAL_THROTTLING:
		curr_time = oss_get_time_stamp();
		throttle_delta = curr_time - adapt->pp.thermal_throttle_start_time;
		if (throttle_delta > adapt->opt.thermal_throttle_rate_limit) {
			adapt->pp.thermal_throttle_start_time = curr_time;
			smu_v15_0_8_pp_notify_throttler_error(adapt, entry->src_data[1]);
		}
		break;
	default:
		AMDGV_WARN("Unknown interrupt id %d\n", entry->src_data[0]);
		break;
	}

	return ret;
}

static int smu_v15_0_8_pp_table_info(struct amdgv_adapter *adapt)
{
	/* PPTable contents are stored inside static metrics table */
	smu_v15_0_8_pp_metrics_init(adapt);

	return 0;
}

static int smu_v15_0_8_pp_get_num_metrics_ext_entries(struct amdgv_adapter *adapt,
						      uint32_t *entries)
{
	struct drv_metrics *drv_gpu_metrics = adapt_to_pp_metrics(adapt, AMDGV_PP_METRIC__GPU);
	struct drv_metrics *drv_sys_metrics = adapt_to_pp_metrics(adapt, AMDGV_PP_METRIC__SYSTEM);

	*entries = drv_gpu_metrics->num_metric + drv_sys_metrics->num_metric;

	return 0;
}

static int smu_v15_0_8_pp_get_num_static_metrics_ext_entries(struct amdgv_adapter *adapt,
							     uint32_t *entries)
{
	struct drv_metrics *drv_metrics = adapt_to_pp_metrics(adapt, AMDGV_PP_METRIC__GPU_STATIC);

	*entries = drv_metrics->num_metric;

	return 0;
}

static int smu_v15_0_8_pp_get_metric(struct amdgv_adapter *adapt,
				     uint64_t *val,
				     enum metric_addr_encoding addr_encoding,
				     void *metric_addr)
{
	if (!val || !metric_addr) {
		AMDGV_ERROR("Invalid metric entry\n");
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
		/* @TODO: Check DS limits on SMU_15 */
		*(bool *)val = false;
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
	case INT_16:
		*(int16_t *)val = *(int16_t *)metric_addr;
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

static int smu_v15_0_8_pp_update_drv_metrics(struct amdgv_adapter *adapt,
					     struct drv_metrics *drv_metrics)
{
	uint32_t i = 0;

	for (i = 0; i < drv_metrics->num_metric; i++)
		smu_v15_0_8_pp_get_metric(adapt, &drv_metrics->metric[i].val,
					  drv_metrics->runtime_config[i].encoding,
					  drv_metrics->runtime_config[i].addr);

	return 0;
}

static int smu_v15_0_8_pp_append_drv_metric_to_user(struct amdgv_adapter *adapt,
	struct drv_metrics *drv_metrics, struct amdgv_gpumon_metrics_ext *metrics_ext)
{
	/* Always assume usermode allocated enough space for all entries */
	if (metrics_ext->num_metric + drv_metrics->num_metric >= AMDGV_GPUMON_MAX_NUM_METRICS_EXT)
		return AMDGV_FAILURE;

	oss_memcpy(&(metrics_ext->metric[metrics_ext->num_metric]), &drv_metrics->metric,
		   sizeof(struct amdgv_gpumon_metric_ext) * drv_metrics->num_metric);

	metrics_ext->num_metric += drv_metrics->num_metric;

	return 0;
}

static int smu_v15_0_8_pp_get_metrics(struct amdgv_adapter *adapt,
				      struct amdgv_gpumon_metrics_ext *metrics_ext)
{
	struct drv_metrics *drv_gpu_metrics = adapt_to_pp_metrics(adapt, AMDGV_PP_METRIC__GPU);
	struct drv_metrics *drv_sys_metrics = adapt_to_pp_metrics(adapt, AMDGV_PP_METRIC__SYSTEM);

	metrics_ext->num_metric = 0;

	if (!drv_gpu_metrics->num_metric)
		if (smu_v15_0_8_pp_metrics_init(adapt))
			return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_get_fw_table(adapt, SMU_TABLE__METRICS, false))
		return AMDGV_FAILURE;
	if (smu_v15_0_8_pp_get_fw_table(adapt, SMU_TABLE__SYSTEM_METRICS, false))
		return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_update_drv_metrics(adapt, drv_gpu_metrics))
		return AMDGV_FAILURE;
	if (smu_v15_0_8_pp_update_drv_metrics(adapt, drv_sys_metrics))
		return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_append_drv_metric_to_user(adapt, drv_gpu_metrics, metrics_ext))
		return AMDGV_FAILURE;
	if (smu_v15_0_8_pp_append_drv_metric_to_user(adapt, drv_sys_metrics, metrics_ext))
		return AMDGV_FAILURE;

	return 0;
}

static int smu_v15_0_8_pp_get_static_metrics(struct amdgv_adapter *adapt,
					     struct amdgv_gpumon_metrics_ext *static_metrics_ext)
{
	struct drv_metrics *drv_metrics = adapt_to_pp_metrics(adapt, AMDGV_PP_METRIC__GPU_STATIC);

	static_metrics_ext->num_metric = 0;

	if (!drv_metrics->num_metric)
		if (smu_v15_0_8_pp_metrics_init(adapt))
			return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_get_fw_table(adapt, SMU_TABLE__STATIC_METRICS, false))
		return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_update_drv_metrics(adapt, drv_metrics))
		return AMDGV_FAILURE;


	if (smu_v15_0_8_pp_append_drv_metric_to_user(adapt, drv_metrics, static_metrics_ext))
		return AMDGV_FAILURE;

	return 0;
}

static bool smu_v15_0_8_pp_cap_supported(struct amdgv_adapter *adapt, int cap)
{
	struct smu_context *smu = ADAPT_TO_SMU(adapt);

	return (smu->supported_caps & SMU_CAPS(cap)) != 0;
}

static int smu_v15_0_8_pp_get_npm_info(struct amdgv_adapter *adapt,
				       struct amdgv_gpumon_npm_info *npm_info)
{
	SystemMetricsTable_t *metrics = ADAPT_TO_SMU_TABLE(adapt, SMU_TABLE__SYSTEM_METRICS);

	if (smu_v15_0_8_pp_get_fw_table(adapt, SMU_TABLE__SYSTEM_METRICS, true))
		return AMDGV_FAILURE;

	if (metrics->NodePower) {
		npm_info->npm_status = AMDGPUMON_NPM_ENABLED;
		npm_info->npm_limit = SMUQ10_ROUND(metrics->NodePowerLimit);
	} else {
		npm_info->npm_status = AMDGPUMON_NPM_DISABLED;
		npm_info->npm_limit = 0;
	}

	return 0;
}

static int smu_v15_0_8_pp_is_pm_enabled(struct amdgv_adapter *adapt, bool *pm_enabled)
{
	struct smu_context *smu = ADAPT_TO_SMU(adapt);

	return smu->features ? true : false;
}

static int smu_v15_0_8_pp_set_power_capacity(struct amdgv_adapter *adapt, int val)
{
	struct smu_15_0_8_msg msg = { 0 };

	msg.id = PPSMC_MSG_SetPptLimit;
	msg.in_arg[0] = val;

	return smu_v15_0_8_send_msg(adapt, &msg);
}
static int smu_v15_0_8_pp_set_df_cstate(struct amdgv_adapter *adapt, enum pp_df_cstate state)
{
	struct smu_15_0_8_msg msg = { 0 };

	msg.id = PPSMC_MSG_SetPptLimit;
	msg.in_arg[0] = state;

	return smu_v15_0_8_send_msg(adapt, &msg);
}

static int smu_v15_0_8_pp_get_clock_limit(struct amdgv_adapter *adapt, enum pp_clock_type clk,
					enum pp_clock_limit_type limit_type, uint32_t *freq)
{
	StaticMetricsTable_t *metrics = ADAPT_TO_SMU_TABLE(adapt, SMU_TABLE__STATIC_METRICS);

	if (!freq)
		return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_get_fw_table(adapt, SMU_TABLE__STATIC_METRICS, false))
		return AMDGV_FAILURE;

	switch (clk) {
	case PP_CLOCK_TYPE__GFX:
		if (limit_type == PP_CLOCK_LIMIT_TYPE__SOFT_MAX)
			*freq = metrics->MaxGfxclkFrequency;
		else if (limit_type == PP_CLOCK_LIMIT_TYPE__SOFT_MIN)
			*freq = metrics->MinGfxclkFrequency;
		else
			return AMDGV_FAILURE;
		break;
	case PP_CLOCK_TYPE__FCLK:
		if (limit_type == PP_CLOCK_LIMIT_TYPE__SOFT_MAX)
			*freq = metrics->MaxFclkFrequency;
		else if (limit_type == PP_CLOCK_LIMIT_TYPE__SOFT_MIN)
			*freq = metrics->MinFclkFrequency;
		else
			return AMDGV_FAILURE;
		break;
	case PP_CLOCK_TYPE__UCLK:
		if (limit_type == PP_CLOCK_LIMIT_TYPE__SOFT_MAX)
			*freq = metrics->UclkFrequencyTable[3];
		else if (limit_type == PP_CLOCK_LIMIT_TYPE__SOFT_MIN)
			*freq = metrics->UclkFrequencyTable[0];
		else
			return AMDGV_FAILURE;
		break;
	default:
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	}

	return 0;
}

const struct amdgv_pp_funcs smu_v15_0_8_pp_funcs = {
	.gpu_mode2_reset			= smu_v15_0_8_pp_gpu_mode2_reset,
	.trigger_vf_flr				= smu_v15_0_8_pp_trigger_vf_flr,
	.gpu_mode0_reset			= smu_v15_0_8_pp_gpu_mode0_reset,
	.reset_vf_arbiters			= smu_v15_0_8_pp_reset_vf_arbiters,
	.handle_smu_irq				= smu_v15_0_8_pp_handle_irq,
	.parse_smu_table_info			= smu_v15_0_8_pp_table_info,
	.init_drv_metrics_ext			= smu_v15_0_8_pp_metrics_init,
	.get_num_metrics_ext_entries		= smu_v15_0_8_pp_get_num_metrics_ext_entries,
	.get_num_static_metrics_ext_entries	= smu_v15_0_8_pp_get_num_static_metrics_ext_entries,
	.get_metrics_ext			= smu_v15_0_8_pp_get_metrics,
	.get_static_metrics_ext			= smu_v15_0_8_pp_get_static_metrics,
	.get_smu_cap_supported			= smu_v15_0_8_pp_cap_supported,
	.get_fru_product_info			= smu_v15_0_8_pp_save_product_info,
	.get_npm_info				= smu_v15_0_8_pp_get_npm_info,
	.is_pm_enabled				= smu_v15_0_8_pp_is_pm_enabled,
	.set_power_capacity			= smu_v15_0_8_pp_set_power_capacity,
	.set_df_cstate				= smu_v15_0_8_pp_set_df_cstate,
	.get_clock_limit			= smu_v15_0_8_pp_get_clock_limit,

	/* RAS Features */
	.send_hbm_bad_pages_num = NULL,
	.send_rma_reason = NULL,
	.get_valid_mca_bank_count = NULL,
	.read_mca_bank_reg32 = NULL,
	.smu_error_inject_set_pm_policy = NULL,
	.smu_error_inject_restore_pm_policy = NULL,

	/* Not applicable for SMU_15 */
	.get_link_metrics = NULL,
	.gpu_mode1_reset = NULL,
	.get_pp_metrics = NULL,
	.get_dpm_capacity = NULL,
	.is_clock_locked = NULL,
	.i2c_eeprom_xfer = NULL,
	.get_shutdown_temperature = NULL,
	.get_power_capacity = NULL,			/* Available in static metrics */
	.get_max_configurable_power_limit = NULL,	/* Available in static metrics */
	.smu_get_pm_policy = NULL,
	.smu_compare_and_set_pm_policy = NULL,
};

static void smu_v15_0_8_pp_fw_tables_fini(struct amdgv_adapter *adapt)
{
	uint32_t i;
	struct smu_context *smu = ADAPT_TO_SMU(adapt);
	struct smu_v15_0_8_table_context *table_context = SMU_TO_TABLE_CONTEXT(smu);
	struct smu_local_memory *driver_xchg_mem = &table_context->driver_xchg_mem;
	struct smu_local_memory *tool_xchg_mem = &table_context->tool_xchg_mem;

	if (driver_xchg_mem->mem) {
		amdgv_memmgr_free(driver_xchg_mem->mem);
		driver_xchg_mem->mem = NULL;
	}

	if (tool_xchg_mem->mem) {
		amdgv_memmgr_free(tool_xchg_mem->mem);
		tool_xchg_mem->mem = NULL;
	}

	for (i = 0; i < SMU_TABLE__NUM; i++) {
		if (table_context->tables[i].table) {
			oss_free(table_context->tables[i].table);
			table_context->tables[i].table = NULL;
		}
	}

	if (smu->smu_table_context) {
		oss_free(smu->smu_table_context);
		smu->smu_table_context = NULL;
	}
}

static void smu_v15_0_8_pp_fw_context_fini(struct amdgv_adapter *adapt)
{
	smu_v15_0_8_pp_fw_tables_fini(adapt);

	return;
}

static void smu_v15_0_8_pp_metrics_free(struct amdgv_adapter *adapt)
{
	uint32_t i;

	for (i = 0; i < AMDGV_PP_METRIC__NUM; i++) {
		if (adapt->pp.metrics[i]) {
			oss_free(adapt->pp.metrics[i]);
			adapt->pp.metrics[i] = NULL;
		}
	}
}

static int smu_v15_0_8_pp_fw_tables_init(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = ADAPT_TO_SMU(adapt);
	struct smu_v15_0_8_table_context *table_context;
	struct smu_local_memory *driver_xchg_mem;
	struct smu_local_memory *tool_xchg_mem;
	struct smu_table *entry = NULL;

	smu->smu_table_context = oss_zalloc(sizeof(struct smu_v15_0_8_table_context));
	if (!smu->smu_table_context)
		return AMDGV_FAILURE;

	table_context = SMU_TO_TABLE_CONTEXT(smu);
	driver_xchg_mem = &table_context->driver_xchg_mem;
	tool_xchg_mem = &table_context->tool_xchg_mem;

	entry = &table_context->tables[SMU_TABLE__METRICS];
	entry->size = sizeof(MetricsTable_t);
	entry->table = oss_zalloc(sizeof(MetricsTable_t));
	entry->msg = PPSMC_MSG_GetMetricsTable;
	driver_xchg_mem->size = max(driver_xchg_mem->size, entry->size);
	if (!entry->table)
		goto fail;

	entry = &table_context->tables[SMU_TABLE__STATIC_METRICS];
	entry->size = sizeof(StaticMetricsTable_t);
	entry->table = oss_zalloc(sizeof(StaticMetricsTable_t));
	entry->msg = PPSMC_MSG_GetStaticMetricsTable;
	driver_xchg_mem->size = max(driver_xchg_mem->size, entry->size);
	if (!entry->table)
		goto fail;

	entry = &table_context->tables[SMU_TABLE__SYSTEM_METRICS];
	entry->size = sizeof(SystemMetricsTable_t);
	entry->table = oss_zalloc(sizeof(SystemMetricsTable_t));
	driver_xchg_mem->size = max(driver_xchg_mem->size, entry->size);
	entry->msg = PPSMC_MSG_GetSystemMetricsTable;
	if (!entry->table)
		goto fail;

	driver_xchg_mem->alignment = PAGE_SIZE;
	tool_xchg_mem->alignment = PAGE_SIZE;
	tool_xchg_mem->size = TOOL_SIZE;

	/* allocate space for driver table */
	driver_xchg_mem->mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
							driver_xchg_mem->size,
							driver_xchg_mem->alignment,
							MEM_SMU_DRIVER_TABLE);
	if (!driver_xchg_mem->mem)
		goto fail;

	tool_xchg_mem->mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
						      tool_xchg_mem->size,
						      tool_xchg_mem->alignment,
						      MEM_SMU_TOOL_TABLE);
	if (!driver_xchg_mem->mem)
		goto fail;

	return 0;

fail:
	smu_v15_0_8_pp_fw_tables_fini(adapt);

	return AMDGV_FAILURE;
}

static int smu_v15_0_8_pp_fw_context_init(struct amdgv_adapter *adapt)
{
	if (smu_v15_0_8_pp_fw_tables_init(adapt))
		return AMDGV_FAILURE;

	return 0;
}

static int smu_v15_0_8_pp_metrics_alloc(struct amdgv_adapter *adapt)
{
	adapt->pp.metrics[AMDGV_PP_METRIC__GPU] = oss_zalloc(sizeof(struct drv_metrics));
	if (!adapt->pp.metrics[AMDGV_PP_METRIC__GPU]) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct drv_metrics));
		goto fail;
	}

	adapt->pp.metrics[AMDGV_PP_METRIC__GPU_STATIC] = oss_zalloc(sizeof(struct drv_metrics));
	if (!adapt->pp.metrics[AMDGV_PP_METRIC__GPU_STATIC]) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct drv_metrics));
		goto fail;
	}

	adapt->pp.metrics[AMDGV_PP_METRIC__SYSTEM] = oss_zalloc(sizeof(struct drv_metrics));
	if (!adapt->pp.metrics[AMDGV_PP_METRIC__SYSTEM]) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct drv_metrics));
		goto fail;
	}

	return 0;

fail:
	smu_v15_0_8_pp_metrics_free(adapt);

	return AMDGV_FAILURE;
}

static int smu_v15_0_8_pp_sw_init(struct amdgv_adapter *adapt)
{
	adapt->pp.pp_funcs = &smu_v15_0_8_pp_funcs;
	adapt->pp.thermal_throttle_start_time = 0;
	adapt->pp.metrics_cache_expire_us = PP_METRICS_CACHE_EXPIRY_US;

	adapt->pp.smu_backend = oss_zalloc(sizeof(struct smu_context));
	if (!adapt->pp.smu_backend) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct smu_context));
		return AMDGV_FAILURE;
	}

	if (smu_v15_0_8_pp_fw_context_init(adapt))
		return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_metrics_alloc(adapt))
		return AMDGV_FAILURE;

	return 0;
}

static int smu_v15_0_8_pp_sw_fini(struct amdgv_adapter *adapt)
{
	smu_v15_0_8_pp_metrics_free(adapt);
	smu_v15_0_8_pp_fw_context_fini(adapt);

	if (adapt->pp.smu_backend) {
		oss_free(adapt->pp.smu_backend);
		adapt->pp.smu_backend = NULL;
	}

	adapt->pp.pp_funcs = NULL;

	return 0;
}

static int smu_v15_0_8_pp_set_driver_table_addr(struct amdgv_adapter *adapt, uint64_t addr)
{
	struct smu_15_0_8_msg msg = { 0 };

	msg.id = PPSMC_MSG_SetDriverDramAddr;
	msg.in_arg[0] = upper_32_bits(addr);
	msg.in_arg[1] = lower_32_bits(addr);

	if (smu_v15_0_8_send_msg(adapt, &msg))
		return AMDGV_FAILURE;

	return 0;
}

static int smu_v15_0_8_pp_set_tool_table_addr(struct amdgv_adapter *adapt, uint64_t addr)
{
	struct smu_15_0_8_msg msg = { 0 };

	msg.id = PPSMC_MSG_SetToolsDramAddr;
	msg.in_arg[0] = upper_32_bits(addr);
	msg.in_arg[1] = lower_32_bits(addr);

	if (smu_v15_0_8_send_msg(adapt, &msg))
		return AMDGV_FAILURE;

	return 0;
}

static int smu_v15_0_8_pp_set_table_address(struct amdgv_adapter *adapt)
{
	struct smu_v15_0_8_table_context *table_context = SMU_TO_TABLE_CONTEXT(ADAPT_TO_SMU(adapt));
	uint64_t addr;

	addr = amdgv_memmgr_get_gpu_addr(table_context->driver_xchg_mem.mem);
	if (smu_v15_0_8_pp_set_driver_table_addr(adapt, addr))
		return AMDGV_FAILURE;

	addr = amdgv_memmgr_get_gpu_addr(table_context->tool_xchg_mem.mem);
	if (smu_v15_0_8_pp_set_tool_table_addr(adapt, addr))
		return AMDGV_FAILURE;

	return 0;
}

static void smu_v15_0_8_pp_init_supported_caps(struct amdgv_adapter *adapt)
{
	struct smu_context *smu = ADAPT_TO_SMU(adapt);

	smu->supported_caps = 0;
	/* Add PMFW/Driver version compatability checks here */
}

static int smu_v15_0_8_pp_save_product_info(struct amdgv_adapter *adapt)
{
	StaticMetricsTable_t *static_metrics = ADAPT_TO_SMU_TABLE(adapt, SMU_TABLE__STATIC_METRICS);

	oss_memset(&adapt->product_info, 0, sizeof(adapt->product_info));

	oss_memcpy(adapt->product_info.manufacturer_name,
		   static_metrics->ProductInfo.ManufacturerName,
		   min(sizeof(adapt->product_info.manufacturer_name),
		       sizeof(static_metrics->ProductInfo.ManufacturerName)));

	oss_memcpy(adapt->product_info.product_name,
		   static_metrics->ProductInfo.Name,
		   min(sizeof(adapt->product_info.product_name),
		       sizeof(static_metrics->ProductInfo.Name)));

	oss_memcpy(adapt->product_info.model_number,
		   static_metrics->ProductInfo.ModelNumber,
		   min(sizeof(adapt->product_info.model_number),
		       sizeof(static_metrics->ProductInfo.ModelNumber)));

	oss_memcpy(adapt->product_info.product_serial,
		   static_metrics->ProductInfo.Serial,
		   min(sizeof(adapt->product_info.product_serial),
		       sizeof(static_metrics->ProductInfo.Serial)));

	oss_memcpy(adapt->product_info.fru_id,
		   static_metrics->ProductInfo.FruId,
		   min(sizeof(adapt->product_info.fru_id),
		       sizeof(static_metrics->ProductInfo.FruId)));

	adapt->product_info.valid = true;
	adapt->product_info.visit = true;

	return 0;
}

static void smu_v15_0_8_pp_clear_drv_metrics(struct amdgv_adapter *adapt,
					     struct drv_metrics *drv_metrics)
{
	drv_metrics->num_metric = 0;
}

static void smu_v15_0_8_pp_drv_gpu_xcp_metrics_init(struct amdgv_adapter *adapt,
						   struct drv_metrics *drv_metrics,
						   MetricsTable_t *metrics,
						   uint32_t xcc_id)
{
	uint32_t vf_mask = amdgv_mcp_get_vf_mask_by_xcc(adapt, xcc_id);
	uint32_t xcp_id = amdgv_mcp_get_xcp_by_xcc(adapt, xcc_id);

	ADD_DRV_METRICS_ENTRY(SMU_15_GFX_CLK_XCD,		vf_mask, xcp_id, Q10_32,	&metrics->GfxclkFrequency[xcc_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_GFX_CLK_XCD_ACC,		vf_mask, xcp_id, Q10_64,	&metrics->GfxclkFrequencyAcc[xcc_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_GFX_CLK_DS_XCD,		vf_mask, xcp_id, Q10_32_DS,	&metrics->GfxclkFrequency[xcc_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_USAGE_GFX_XCD,		vf_mask, xcp_id, Q10_32,	&metrics->GfxBusy[xcc_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_USAGE_GFX_XCD_ACC,		vf_mask, xcp_id, Q10_64,	&metrics->GfxBusyAcc[xcc_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROT_GFX_BEL_PPT_ACC,	vf_mask, xcp_id, Q10_64,	&metrics->GfxclkBelowHostLimitPptAcc[xcc_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROT_GFX_BEL_THM_ACC,	vf_mask, xcp_id, Q10_64,	&metrics->GfxclkBelowHostLimitThmAcc[xcc_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROT_GFX_BEL_TOT_ACC,	vf_mask, xcp_id, Q10_64,	&metrics->GfxclkBelowHostLimitTotalAcc[xcc_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROT_GFX_CLK_LOW_ACC,	vf_mask, xcp_id, Q10_64,	&metrics->GfxclkLowUtilizationAcc[xcc_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_TEMP_XCD,			vf_mask, xcp_id, UINT_32,	&metrics->XcdTemperature[xcc_id]);
}

static void smu_v15_0_8_pp_drv_gpu_aid_metrics_init(struct amdgv_adapter *adapt,
						    struct drv_metrics *drv_metrics,
						    MetricsTable_t *metrics,
						    uint32_t aid_id)
{
	uint32_t vf_mask = amdgv_mcp_get_vf_mask_by_aid(adapt, aid_id);

	ADD_DRV_METRICS_ENTRY(SMU_15_TEMP_AID,		vf_mask, aid_id, UINT_32,	&metrics->AidTemperature[aid_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_MEMCLK,		vf_mask, aid_id, Q10_32,	&metrics->UclkFrequency[aid_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_MEM_CLK_DS,	vf_mask, aid_id, Q10_32_DS,	&metrics->UclkFrequency[aid_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_FCLK,		vf_mask, aid_id, Q10_32,	&metrics->FclkFrequency[aid_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_FCLK_DS,		vf_mask, aid_id, Q10_32_DS,	&metrics->FclkFrequency[aid_id]);
}

static void smu_v15_0_8_pp_drv_gpu_mid_metrics_init(struct amdgv_adapter *adapt,
						    struct drv_metrics *drv_metrics,
						    MetricsTable_t *metrics,
						    uint32_t mid_id)
{
	/* Equal number of MID & AID in smu_15 */
	uint32_t vf_mask = amdgv_mcp_get_vf_mask_by_aid(adapt, mid_id);
	uint32_t i = 0;

	//@TODO: verify the encoding
	ADD_DRV_METRICS_ENTRY(SMU_15_TEMP_MID,		vf_mask, mid_id, UINT_32,	&metrics->MidTemperature[mid_id]);

	ADD_DRV_METRICS_ENTRY(SMU_15_SOC_CLK,		vf_mask, mid_id, Q10_32,	&metrics->SocclkFrequency[mid_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_SOC_CLK_DS,	vf_mask, mid_id, Q10_32_DS,	&metrics->SocclkFrequency[mid_id]);
	ADD_DRV_METRICS_ENTRY(SMU_15_LCLK,		vf_mask, i, Q10_32,		&metrics->LclkFrequency[i]);
	ADD_DRV_METRICS_ENTRY(SMU_15_LCLK_DS,		vf_mask, i, Q10_32_DS,		&metrics->LclkFrequency[i]);

	/* 2 per MID */
	i = mid_id * 2;
	ADD_DRV_METRICS_ENTRY(SMU_15_USAGE_VCN,		vf_mask, i, Q10_32,	&metrics->VcnBusy[i]);
	ADD_DRV_METRICS_ENTRY(SMU_15_VCLK,		vf_mask, i, Q10_32,	&metrics->VclkFrequency[i]);
	ADD_DRV_METRICS_ENTRY(SMU_15_VCLK_DS,		vf_mask, i, Q10_32_DS,	&metrics->VclkFrequency[i]);
	ADD_DRV_METRICS_ENTRY(SMU_15_DCLK,		vf_mask, i, Q10_32,	&metrics->DclkFrequency[i]);
	ADD_DRV_METRICS_ENTRY(SMU_15_DCLK_DS,		vf_mask, i, Q10_32_DS,	&metrics->DclkFrequency[i]);
	i++;
	ADD_DRV_METRICS_ENTRY(SMU_15_USAGE_VCN,		vf_mask, i, Q10_32,	&metrics->VcnBusy[i]);
	ADD_DRV_METRICS_ENTRY(SMU_15_VCLK,		vf_mask, i, Q10_32,	&metrics->VclkFrequency[i]);
	ADD_DRV_METRICS_ENTRY(SMU_15_VCLK_DS,		vf_mask, i, Q10_32_DS,	&metrics->VclkFrequency[i]);
	ADD_DRV_METRICS_ENTRY(SMU_15_DCLK,		vf_mask, i, Q10_32,	&metrics->DclkFrequency[i]);
	ADD_DRV_METRICS_ENTRY(SMU_15_DCLK_DS,		vf_mask, i, Q10_32_DS,	&metrics->DclkFrequency[i]);

	for (i = SMU_15_JPEG_PER_MID * mid_id; i < SMU_15_JPEG_PER_MID * mid_id + SMU_15_JPEG_PER_MID; i++)
		ADD_DRV_METRICS_ENTRY(SMU_15_USAGE_JPEG, vf_mask, (i / SMU_15_JPEG_PER_MID), Q10_32, &metrics->JpegBusy[i]);
}

static void smu_v15_0_8_pp_drv_gpu_hbm_metrics_init(struct amdgv_adapter *adapt,
					 	    struct drv_metrics *drv_metrics,
					 	    MetricsTable_t *metrics,
					 	    uint32_t idx)
{
	uint32_t vf_mask = amdgv_mcp_get_vf_mask_by_aid(adapt, idx / SMU_15_HBM_PER_AID);

	ADD_DRV_METRICS_ENTRY(SMU_15_TEMP_HBM, vf_mask, idx / SMU_15_HBM_PER_AID, UINT_32,
			      &metrics->HbmTemperature[idx]);
	ADD_DRV_METRICS_ENTRY(SMU_15_TEMP_HBM_ACC, vf_mask, idx / SMU_15_HBM_PER_AID, UINT_64,
			      &metrics->HbmTemperatureAcc[idx]);
}

static int smu_v15_0_8_pp_drv_gpu_metrics_init(struct amdgv_adapter *adapt)
{
	struct drv_metrics *drv_metrics;
	MetricsTable_t *metrics;
	uint32_t whole_gpu_vf_mask = (BIT(adapt->num_vf) - 1) | BIT(AMDGV_PF_IDX);
	uint32_t i = 0;

	metrics = ADAPT_TO_SMU_TABLE(adapt, SMU_TABLE__METRICS);
	drv_metrics = adapt_to_pp_metrics(adapt, AMDGV_PP_METRIC__GPU);

	smu_v15_0_8_pp_clear_drv_metrics(adapt, drv_metrics);

	ADD_DRV_METRICS_ENTRY(SMU_15_METRICS_COUNTER, whole_gpu_vf_mask, SMU_15_GPU_RES_ID,
			      UINT_32, &metrics->AccumulationCounter);

	for (i = 0; i < SMU_15_NUM_XCD; i++)
		smu_v15_0_8_pp_drv_gpu_xcp_metrics_init(adapt, drv_metrics, metrics, i);

	for (i = 0; i < SMU_15_NUM_AID; i++)
		smu_v15_0_8_pp_drv_gpu_aid_metrics_init(adapt, drv_metrics, metrics, i);

	for (i = 0; i < SMU_15_NUM_MID; i++)
		smu_v15_0_8_pp_drv_gpu_mid_metrics_init(adapt, drv_metrics, metrics, i);

	for (i = 0; i < SMU_15_SMU_NUM_HBM; i++)
		smu_v15_0_8_pp_drv_gpu_hbm_metrics_init(adapt, drv_metrics, metrics, i);

	ADD_DRV_METRICS_ENTRY(SMU_15_TEMP_HOTSPOT,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_32,		&metrics->MaxSocketTemperature);
	ADD_DRV_METRICS_ENTRY(SMU_15_TEMP_VR,				whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_32,		&metrics->MaxVrTemperature);
	ADD_DRV_METRICS_ENTRY(SMU_15_TEMP_HOTSPOT_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_64,		&metrics->MaxSocketTemperatureAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_TEMP_VR_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_64,		&metrics->MaxVrTemperatureAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_POWER_MAX,				whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_32,		&metrics->SocketPowerLimit);
	ADD_DRV_METRICS_ENTRY(SMU_15_POWER,				whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_32,		&metrics->SocketPower);
	ADD_DRV_METRICS_ENTRY(SMU_15_ENERGY_SOCKET_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_64,		&metrics->SocketEnergyAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_ENERGY_MEM_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_64,		&metrics->HbmEnergyAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_USAGE_GFX,				whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_32,		&metrics->SocketGfxBusy);
	ADD_DRV_METRICS_ENTRY(SMU_15_USAGE_MEM,				whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_32,		&metrics->DramBandwidthUtilization);
	ADD_DRV_METRICS_ENTRY(SMU_15_USAGE_GFX_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_64,		&metrics->SocketGfxBusyAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_USAGE_MEM_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_64,		&metrics->DramBandwidthUtilizationAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_DRAM_BANDWIDTH_ACC,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_64,		&metrics->DramBandwidthAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_DRAM_BANDWIDTH_MAX,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_32,		&metrics->MaxDramBandwidth);
	ADD_DRV_METRICS_ENTRY(SMU_15_PCIE_BANDWIDTH_ACC,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_64,		&metrics->PcieBandwidthAcc[0]);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROT_PROCHOT_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&metrics->ProchotResidencyAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROT_PPT_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&metrics->PptResidencyAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROT_SOCKET_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&metrics->SocketThmResidencyAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROT_VR_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&metrics->VrThmResidencyAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROT_MEM_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&metrics->HbmThmResidencyAcc);
	ADD_DRV_METRICS_ENTRY(SMU_15_PCIE_BANDWIDTH,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, Q10_32,		&(metrics->PcieBandwidth[0]));
	ADD_DRV_METRICS_ENTRY(SMU_15_PCIE_L0_TO_RECOVER_ACC,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&(metrics->PCIeL0ToRecoveryCountAcc));
	ADD_DRV_METRICS_ENTRY(SMU_15_PCIE_REPL_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&(metrics->PCIenReplayAAcc));
	ADD_DRV_METRICS_ENTRY(SMU_15_PCIE_REPL_ROLLOVER_ACC,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&(metrics->PCIenReplayARolloverCountAcc));
	ADD_DRV_METRICS_ENTRY(SMU_15_PCIE_NAK_SENT_ACC,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&(metrics->PCIeNAKSentCountAcc));
	ADD_DRV_METRICS_ENTRY(SMU_15_PCIE_NAK_RECEIVED_ACC,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&(metrics->PCIeNAKReceivedCountAcc));
	ADD_DRV_METRICS_ENTRY(SMU_15_PCIE_OTHER_END_RECOVERY_ACC,	whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&(metrics->PCIeOtherEndRecoveryAcc));
	ADD_DRV_METRICS_ENTRY(SMU_15_PCIE_LINK_SPEED,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&(metrics->PCIeLinkSpeed));
	ADD_DRV_METRICS_ENTRY(SMU_15_PCIE_LINK_WIDTH,			whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,		&(metrics->PCIeLinkWidth));

	return 0;
};

static int smu_v15_0_8_pp_drv_static_metrics_init(struct amdgv_adapter *adapt)
{
	uint32_t whole_gpu_vf_mask = (BIT(adapt->num_vf) - 1) | BIT(AMDGV_PF_IDX);
	struct drv_metrics *drv_metrics;
	StaticMetricsTable_t *metrics;

	metrics = ADAPT_TO_SMU_TABLE(adapt, SMU_TABLE__STATIC_METRICS);
	drv_metrics = adapt_to_pp_metrics(adapt, AMDGV_PP_METRIC__GPU_STATIC);

	smu_v15_0_8_pp_clear_drv_metrics(adapt, drv_metrics);

	ADD_DRV_METRICS_ENTRY(SMU_15_IN_TEL_VOLTAGE,	whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&(metrics->InputTelemetryVoltageInmV));
	ADD_DRV_METRICS_ENTRY(SMU_15_PLDM_VERSION,	whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&(metrics->pldmVersion[0]));
	ADD_DRV_METRICS_ENTRY(SMU_15_GFX_CLK_MAX,	whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->MaxGfxclkFrequency);
	ADD_DRV_METRICS_ENTRY(SMU_15_GFX_CLK_MIN,	whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->MinGfxclkFrequency);
	ADD_DRV_METRICS_ENTRY(SMU_15_FCLK_MAX,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->MaxFclkFrequency);
	ADD_DRV_METRICS_ENTRY(SMU_15_FCLK_MIN,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->MinFclkFrequency);
	ADD_DRV_METRICS_ENTRY(SMU_15_MEMCLK_MAX,	whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->UclkFrequencyTable[3]);
	ADD_DRV_METRICS_ENTRY(SMU_15_MEMCLK_MIN,	whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->UclkFrequencyTable[0]);
	ADD_DRV_METRICS_ENTRY(SMU_15_CTF_XCD,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->CTFLimit_XCD);
	ADD_DRV_METRICS_ENTRY(SMU_15_CTF_AID,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->CTFLimit_AID);
	ADD_DRV_METRICS_ENTRY(SMU_15_CTF_MID,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->CTFLimit_MID);
	ADD_DRV_METRICS_ENTRY(SMU_15_CTF_HBM,		whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->CTFLimit_HBM);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROTTLE_XCD,	whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->ThermalLimit_XCD);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROTTLE_AID,	whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->ThermalLimit_AID);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROTTLE_MID,	whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->ThermalLimit_MID);
	ADD_DRV_METRICS_ENTRY(SMU_15_THROTTLE_HBM,	whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_32,	&metrics->ThermalLimit_HBM);

	return 0;
}

static void smu_v15_0_8_pp_drv_system_metric_init(struct amdgv_adapter *adapt,
						  enum smu_15_metric_name name,
						  struct drv_metrics *drv_metrics,
						  void *addr)
{
	uint32_t whole_gpu_vf_mask = (BIT(adapt->num_vf) - 1) | BIT(AMDGV_PF_IDX);

	/* PMFW will indicate a metric is unavailable with '-1'.
	   Do a one time pass to identify and skip invalid metrics. */
	if ((*(int16_t *)addr) ==  (int16_t)(-1))
		return;

	ADD_DRV_METRICS_ENTRY(name, whole_gpu_vf_mask, SMU_15_GPU_RES_ID, INT_16, addr);
}

static int smu_v15_0_8_pp_drv_system_metrics_init(struct amdgv_adapter *adapt)
{
	uint32_t whole_gpu_vf_mask = (BIT(adapt->num_vf) - 1) | BIT(AMDGV_PF_IDX);
	struct drv_metrics *drv_metrics;
	SystemMetricsTable_t *metrics;

	metrics = ADAPT_TO_SMU_TABLE(adapt, SMU_TABLE__SYSTEM_METRICS);
	drv_metrics = adapt_to_pp_metrics(adapt, AMDGV_PP_METRIC__SYSTEM);

	smu_v15_0_8_pp_clear_drv_metrics(adapt, drv_metrics);

	ADD_DRV_METRICS_ENTRY(SMU_15_SYS_METRIC_ACC_COUNTER, whole_gpu_vf_mask, SMU_15_GPU_RES_ID, UINT_64, &metrics->AccumulationCounter);

	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_UBB_FPGA,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_UBB_FPGA]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_UBB_FRONT,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_UBB_FRONT]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_UBB_BACK,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_UBB_BACK]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_UBB_OAM7,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_UBB_OAM7]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_UBB_IBC,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_UBB_IBC]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_UBB_UFPGA,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_UBB_UFPGA]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_UBB_OAM1,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_UBB_OAM1]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_OAM_0_1_HSC,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_OAM_0_1_HSC]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_OAM_2_3_HSC,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_OAM_2_3_HSC]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_OAM_4_5_HSC,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_OAM_4_5_HSC]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_OAM_6_7_HSC,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_OAM_6_7_HSC]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_UBB_FPGA_0V72_VR,	drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_UBB_FPGA_0V72_VR]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_UBB_FPGA_3V3_VR,	drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_UBB_FPGA_3V3_VR]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_RETIMER_0_1_2_3_1V2_VR,	drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_RETIMER_0_1_2_3_1V2_VR]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_RETIMER_4_5_6_7_1V2_VR,	drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_RETIMER_4_5_6_7_1V2_VR]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_RETIMER_0_1_0V9_VR,	drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_RETIMER_0_1_0V9_VR]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_RETIMER_4_5_0V9_VR,	drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_RETIMER_4_5_0V9_VR]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_RETIMER_2_3_0V9_VR,	drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_RETIMER_2_3_0V9_VR]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_RETIMER_6_7_0V9_VR,	drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_RETIMER_6_7_0V9_VR]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_OAM_0_1_2_3_3V3_VR,	drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_OAM_0_1_2_3_3V3_VR]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_OAM_4_5_6_7_3V3_VR,	drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_OAM_4_5_6_7_3V3_VR]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_IBC_HSC,		drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_IBC_HSC]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_TEMP_IBC,			drv_metrics, &metrics->SystemTemperatures[SYSTEM_TEMP_IBC]);

	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_NODE_TEMP_RETIMER,			drv_metrics, &metrics->NodeTemperatures[NODE_TEMP_RETIMER]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_NODE_TEMP_IBC_TEMP,			drv_metrics, &metrics->NodeTemperatures[NODE_TEMP_IBC_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_NODE_TEMP_IBC_2_TEMP,		drv_metrics, &metrics->NodeTemperatures[NODE_TEMP_IBC_2_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_NODE_TEMP_VDD18_VR_TEMP,		drv_metrics, &metrics->NodeTemperatures[NODE_TEMP_VDD18_VR_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_NODE_TEMP_04_HBM_B_VR_TEMP,		drv_metrics, &metrics->NodeTemperatures[NODE_TEMP_04_HBM_B_VR_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_NODE_TEMP_04_HBM_D_VR_TEMP,		drv_metrics, &metrics->NodeTemperatures[NODE_TEMP_04_HBM_D_VR_TEMP]);

	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDCR_X0_TEMP,		drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDCR_X0_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDCR_X1_TEMP,		drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDCR_X1_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDIO_HBM_B_TEMP,		drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDIO_HBM_B_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDIO_HBM_D_TEMP,		drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDIO_HBM_D_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDIO_04_HBM_B_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDIO_04_HBM_B_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDIO_04_HBM_D_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDIO_04_HBM_D_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDCR_HBM_B_TEMP,		drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDCR_HBM_B_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDCR_HBM_D_TEMP,		drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDCR_HBM_D_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDCR_075_HBM_B_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDCR_075_HBM_B_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDCR_075_HBM_D_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDCR_075_HBM_D_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDIO_11_GTA_A_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDIO_11_GTA_A_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDIO_11_GTA_C_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDIO_11_GTA_C_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDAN_075_GTA_A_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDAN_075_GTA_A_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDAN_075_GTA_C_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDAN_075_GTA_C_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDCR_075_UCIE_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDCR_075_UCIE_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDIO_065_UCIEAA_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDIO_065_UCIEAA_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDIO_065_UCIEAM_A_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDIO_065_UCIEAM_A_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDIO_065_UCIEAM_C_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDIO_065_UCIEAM_C_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDCR_SOCIO_A_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDCR_SOCIO_A_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDCR_SOCIO_C_TEMP,	drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDCR_SOCIO_C_TEMP]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SVI_PLANE_VDDAN_075_TEMP,		drv_metrics, &metrics->VrTemperatures[SVI_PLANE_VDDAN_075_TEMP]);

	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_POWER_UBB_POWER,		drv_metrics, &metrics->SystemPower[SYSTEM_POWER_UBB_POWER]);
	smu_v15_0_8_pp_drv_system_metric_init(adapt, SMU_15_SYSTEM_POWER_UBB_POWER_THRESHOLD,	drv_metrics, &metrics->SystemPower[SYSTEM_POWER_UBB_POWER_THRESHOLD]);

	return 0;
}

static int smu_v15_0_8_pp_metrics_init(struct amdgv_adapter *adapt)
{
	if (smu_v15_0_8_pp_get_fw_table(adapt, SMU_TABLE__METRICS, true))
		return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_get_fw_table(adapt, SMU_TABLE__STATIC_METRICS, true))
		return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_get_fw_table(adapt, SMU_TABLE__SYSTEM_METRICS, true))
		return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_drv_gpu_metrics_init(adapt))
		return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_drv_static_metrics_init(adapt))
		return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_drv_system_metrics_init(adapt))
		return AMDGV_FAILURE;

	return 0;
};

static int smu_v15_0_8_pp_hw_init(struct amdgv_adapter *adapt)
{
	smu_v15_0_8_pp_init_supported_caps(adapt);

	if (smu_v15_0_8_pp_set_table_address(adapt))
		return AMDGV_FAILURE;

	if (smu_v15_0_8_pp_metrics_init(adapt))
		return AMDGV_FAILURE;

	if (!adapt->product_info.visit)
		smu_v15_0_8_pp_save_product_info(adapt);

	return 0;
}

static int smu_v15_0_8_pp_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func smu_v15_0_8_pp_func = {
	.name = "smu_v15_0_8_pp_func",
	.sw_init = smu_v15_0_8_pp_sw_init,
	.sw_fini = smu_v15_0_8_pp_sw_fini,
	.hw_init = smu_v15_0_8_pp_hw_init,
	.hw_fini = smu_v15_0_8_pp_hw_fini,
};

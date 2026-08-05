/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_ual.h"
#include "amdgv_psp_gfx_if.h"
#include "amdgv_sched.h"
#include "hwip/psp/psp_v15_0_8.h"

static const uint32_t this_block = AMDGV_UAL_BLOCK;

static int amdgv_ual_sw_init(struct amdgv_adapter *adapt)
{
	adapt->ual.asp_cmd_resp_size = AMDGV_UAL_CMD_RESP_SIZE;
	adapt->ual.asp_cmd_resp_mem =
		amdgv_memmgr_alloc(&adapt->memmgr_pf, adapt->ual.asp_cmd_resp_size, MEM_PSP_UAL);
	if (!adapt->ual.asp_cmd_resp_mem) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL, adapt->ual.asp_cmd_resp_size);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int amdgv_ual_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->ual.asp_cmd_resp_mem) {
		amdgv_memmgr_free(adapt->ual.asp_cmd_resp_mem);
		adapt->ual.asp_cmd_resp_mem = NULL;
	}
	adapt->ual.asp_cmd_resp_size = 0;

	return 0;
}

static int amdgv_ual_hw_init(struct amdgv_adapter *adapt)
{
	int ret;

	if (adapt->smuio.funcs && adapt->smuio.funcs->get_link_type) {
		adapt->ual.link_type = adapt->smuio.funcs->get_link_type(adapt);
		if (adapt->ual.link_type == AMDGV_UAL_NONE)
			return AMDGV_FAILURE;
	} else {
		return AMDGV_FAILURE;
	}

	/* Get UAL interface version from ASP */
	ret = amdgv_ual_get_interface_version(adapt, &adapt->ual.intf_ver);
	if (ret)
		return ret;

	/* Get UAL config from ASP and update adapt->ual context */
	ret = amdgv_ual_get_config(adapt, NULL);
	if (ret)
		return ret;

	return 0;
}

static int amdgv_ual_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func amdgv_ual_func = {
	.name = "amdgv_ual_func",
	.sw_init = amdgv_ual_sw_init,
	.sw_fini = amdgv_ual_sw_fini,
	.hw_init = amdgv_ual_hw_init,
	.hw_fini = amdgv_ual_hw_fini,
};

int amdgv_ual_set_accelerator_state(struct amdgv_adapter *adapt, enum amdgv_ual_accelerator_vpod_state new_state)
{
	if (!adapt)
		return -1;

	if (!AMDGV_UAL_ACCEL_STATE_IS_VALID(new_state))
		return -1;

	adapt->ual.node_info_v1.accel_state = new_state;
	return 0;
}

enum amdgv_ual_accelerator_vpod_state amdgv_ual_get_accelerator_state(struct amdgv_adapter *adapt)
{
	if (!adapt)
		return AMDGV_UAL_ACCEL_VPOD_STATE_ERROR;

	return adapt->ual.node_info_v1.accel_state;
}

bool amdgv_ual_is_supported(struct amdgv_adapter *adapt)
{
    if ( (adapt->ual.link_type == AMDGV_UALOE || adapt->ual.link_type == AMDGV_UALINK) && (adapt->num_vf == 1) )
        return true;
    else
        return false;
}

int amdgv_ual_get_interface_version(struct amdgv_adapter *adapt, uint32_t *version)
{
	int ret = AMDGV_FAILURE;

	ret = psp_v15_0_8_ual_get_interface_version(adapt, version);
	if (ret != PSP_STATUS__SUCCESS)
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	return ret;
}

static enum amdgv_ual_accelerator_vpod_state amdgv_ual_derive_accel_state(struct amdgv_adapter *adapt)
{
	/* Check for sentinel/initial values indicating unconfigured state */
	if (adapt->ual.node_info_v1.accelerator_id == 0xFFFFFFFF)
		return AMDGV_UAL_ACCEL_VPOD_STATE_UNCONFIGURED;

	/* Check vpod configuration for READY state */
	if (adapt->ual.topology_info_v1.vpod_id != 0xFFFFFFFF &&
	    adapt->ual.topology_info_v1.vpod_size != 0)
		return AMDGV_UAL_ACCEL_VPOD_STATE_READY;

	/* Check ppod configuration for CONFIGURED state */
	if (adapt->ual.node_info_v1.ppod_size != 0)
		return AMDGV_UAL_ACCEL_VPOD_STATE_CONFIGURED;

	return AMDGV_UAL_ACCEL_VPOD_STATE_UNCONFIGURED;
}

int amdgv_ual_get_config(struct amdgv_adapter *adapt, struct amdgv_gpumon_get_config_rsp_ual_v1 *config)
{
	int ret = AMDGV_FAILURE;
	struct psp_km_get_config_ual_v1 *asp_config;

	ret = psp_v15_0_8_ual_get_config(adapt, amdgv_memmgr_get_gpu_addr(adapt->ual.asp_cmd_resp_mem), adapt->ual.asp_cmd_resp_size);
	if (ret != PSP_STATUS__SUCCESS)
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	asp_config = (struct psp_km_get_config_ual_v1 *)amdgv_memmgr_get_cpu_addr(adapt->ual.asp_cmd_resp_mem);
	if (!asp_config)
		return AMDGV_FAILURE;

	/* Update adapt->ual context with config from ASP */
	adapt->ual.node_info_v1.accelerator_id = asp_config->accelerator_id;
	oss_memcpy(adapt->ual.node_info_v1.ppod_id, asp_config->ppod_id,
		sizeof(adapt->ual.node_info_v1.ppod_id));
	adapt->ual.node_info_v1.ppod_size = asp_config->ppod_size;
	adapt->ual.node_info_v1.bandwidth = asp_config->bandwidth;
	adapt->ual.node_info_v1.latency = asp_config->latency;

	adapt->ual.topology_info_v1.vpod_id = asp_config->vpod_id;
	adapt->ual.topology_info_v1.vpod_size = asp_config->vpod_size;
	oss_memcpy(adapt->ual.topology_info_v1.vpod_active_accelerators,
		asp_config->vpod_active_accelerators,
		sizeof(adapt->ual.topology_info_v1.vpod_active_accelerators));
	adapt->ual.topology_info_v1.addr_mode = asp_config->addr_mode;

	/* Derive accel_state from config */
	adapt->ual.node_info_v1.accel_state = amdgv_ual_derive_accel_state(adapt);

	/* Copy to output config if provided */
	if (config) {
		config->link_type = adapt->ual.link_type;
		config->accelerator_id = adapt->ual.node_info_v1.accelerator_id;
		oss_memcpy(config->ppod_id, adapt->ual.node_info_v1.ppod_id, sizeof(config->ppod_id));
		config->ppod_size = adapt->ual.node_info_v1.ppod_size;
		config->bandwidth = adapt->ual.node_info_v1.bandwidth;
		config->latency = adapt->ual.node_info_v1.latency;
		oss_memcpy(config->local_accelerators,
			adapt->ual.node_info_v1.local_accelerators,
			sizeof(config->local_accelerators));

		config->vpod_id = adapt->ual.topology_info_v1.vpod_id;
		config->vpod_size = adapt->ual.topology_info_v1.vpod_size;
		oss_memcpy(config->vpod_active_accelerators,
			adapt->ual.topology_info_v1.vpod_active_accelerators,
			sizeof(config->vpod_active_accelerators));
		config->addr_mode = adapt->ual.topology_info_v1.addr_mode;
		config->accel_state = adapt->ual.node_info_v1.accel_state;
	}

	return ret;
}

int amdgv_ual_set_ppod_config(struct amdgv_adapter *adapt, struct amdgv_gpumon_set_ppod_config_req_ual_v1 *config)
{
	int ret = AMDGV_FAILURE;

	ret = psp_v15_0_8_ual_set_ppod_config(adapt, config);
	if (ret != PSP_STATUS__SUCCESS)
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	/* Update adapt->ual context with ppod config */
	adapt->ual.node_info_v1.accelerator_id = config->accelerator_id;
	oss_memcpy(adapt->ual.node_info_v1.ppod_id, config->ppod_id,
		sizeof(adapt->ual.node_info_v1.ppod_id));
	adapt->ual.node_info_v1.ppod_size = config->ppod_size;
	adapt->ual.node_info_v1.bandwidth = config->bandwidth;
	adapt->ual.node_info_v1.latency = config->latency;
	oss_memcpy(adapt->ual.node_info_v1.local_accelerators,
		config->local_accelerators,
		sizeof(adapt->ual.node_info_v1.local_accelerators));

	/* Derive accel_state from updated config */
	adapt->ual.node_info_v1.accel_state = amdgv_ual_derive_accel_state(adapt);

	return ret;
}

int amdgv_ual_set_vpod_config(struct amdgv_adapter *adapt, struct amdgv_gpumon_set_vpod_config_req_ual_v1 *config)
{
	int ret = AMDGV_FAILURE;

	ret = psp_v15_0_8_ual_set_vpod_config(adapt, config);
	if (ret != PSP_STATUS__SUCCESS)
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	/* Update adapt->ual context with vpod config */
	adapt->ual.topology_info_v1.vpod_id = config->vpod_id;
	adapt->ual.topology_info_v1.vpod_size = config->vpod_size;
	oss_memcpy(adapt->ual.topology_info_v1.vpod_active_accelerators,
		config->vpod_active_accelerators,
		sizeof(adapt->ual.topology_info_v1.vpod_active_accelerators));
	adapt->ual.topology_info_v1.addr_mode = config->addr_mode;

	/* Derive accel_state from updated config */
	adapt->ual.node_info_v1.accel_state = amdgv_ual_derive_accel_state(adapt);

	return ret;
}

int amdgv_ual_set_station_config(struct amdgv_adapter *adapt, struct amdgv_gpumon_set_station_config_req_ual_v1 *config)
{
	int ret = AMDGV_FAILURE;

	ret = psp_v15_0_8_ual_set_station_config(adapt, config);
	if (ret != PSP_STATUS__SUCCESS)
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	return ret;
}

int amdgv_ual_pause(struct amdgv_adapter *adapt, bool send_completion)
{
	int ret = 0;

	/* World-switch to PF so host can perform UAL operations */
	ret = amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX,
			AMDGV_SCHED_BLOCK_ALL);

	if (send_completion)
		amdgv_ual_send_completion(adapt, PSP_GFX_INT_CTXT_UAL_CMD_PAUSE, ret);

	return ret;
}

int amdgv_ual_resume(struct amdgv_adapter *adapt, bool send_completion)
{
	int ret = 0;

	/* World-switch back to VF (UAL is single-VF only, so VF index 0) */
	ret = amdgv_sched_context_switch_to_vf(adapt, 0,
			AMDGV_SCHED_BLOCK_ALL);

	if (send_completion)
		amdgv_ual_send_completion(adapt, PSP_GFX_INT_CTXT_UAL_CMD_RESUME, ret);

	return ret;
}

int amdgv_ual_trigger_mode2(struct amdgv_adapter *adapt)
{
	int ret = AMDGV_FAILURE;

	return ret;
}

int amdgv_ual_send_completion(struct amdgv_adapter *adapt, uint32_t cmd_id, uint32_t status)
{
	int ret = AMDGV_FAILURE;

	ret = psp_v15_0_8_ual_send_completion(adapt, cmd_id, status);
	if (ret != PSP_STATUS__SUCCESS)
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	return ret;
}
/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv_gpumon.h"
#include "amdgv_gpumon_internal.h"
#include "amdgv_api_internal.h"
#include "amdgv_sched_internal.h"
#include "amdgv_vfmgr.h"
#include "amdgv_psp_gfx_if.h"
#include "atombios/atomfirmware.h"
#include "atombios/atom.h"
#include "atombios/atombios.h"

static const uint32_t this_block = AMDGV_API_BLOCK;

int amdgv_int_allocate_vf(struct amdgv_adapter *adapt, struct amdgv_vf_option *option)
{
	int opt_err = 0, ret;
	uint32_t i;

	/* First time only, remove all default VFs */
	if (!adapt->customized_vf_config_mode)
		amdgv_vfmgr_enter_customized_vf_mode(adapt, 1);

	// VF index is already set during GPUV live update restore
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) || (adapt->live_update_state != AMDGV_LIVE_UPDATE_RESTORE)) {
		/* search for free vf */
		for (i = 0; i < adapt->num_vf; i++) {
			if (is_unavail_vf(i))
				break;
		}
		if (i == adapt->num_vf) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPUMON_NO_AVAILABLE_SLOT, 0);
			opt_err = AMDGV_ERROR_GPUMON_NO_AVAILABLE_SLOT;
			goto unlock;
		}

		option->idx_vf = i;
	}

	/* for now check SMI option only */

	if (option->fb_size == 0) {
		amdgv_vfmgr_set_default_fb_size(adapt, option);
	}

	opt_err = amdgv_vf_option_valid(adapt, AMDGV_SET_VF_SMI_FULL_OPT, option);
	if (opt_err != 0)
		goto unlock;

	ret = amdgv_sched_park(adapt);
	if (ret) {
		opt_err = ret;
		goto unlock;
	}
	opt_err = amdgv_vfmgr_alloc_vf(adapt, option);

	ret = amdgv_sched_unpark(adapt);
	if ((opt_err == 0) && (ret != 0))
		opt_err = ret;

unlock:
	return opt_err;
}

int amdgv_int_free_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int tmp_ret, ret;

	ret = AMDGV_ERROR_GPUMON_INVALID_VF_INDEX;

	if (!AMDGV_IS_IDX_INVALID(idx_vf)) {
		if (is_unavail_vf(idx_vf))
			return 0;

		if (!is_avail_vf(idx_vf))
			return AMDGV_ERROR_GPUMON_VF_BUSY;

		tmp_ret = amdgv_sched_park(adapt);
		if (tmp_ret) {
			ret = tmp_ret;
			goto unlock;
		}

		ret = amdgv_vfmgr_free_vf(adapt, idx_vf);

		tmp_ret = amdgv_sched_unpark(adapt);
		if ((ret == 0) && (tmp_ret != 0))
			ret = tmp_ret;
	}
unlock:
	return ret;
}

int amdgv_int_set_vf_number(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	int opt_err;
	uint32_t i;

	/* Check if dynamic VF number change is supported on this platform */
	if (adapt->flags & AMDGV_FLAG_NO_DYNAMIC_VF_NUM) {
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	/* all VFs must be in avail or unavail state */
	for (i = 0; i < adapt->num_vf; i++) {
		if (!is_unavail_vf(i) && !is_avail_vf(i)) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPUMON_VF_BUSY, i);
			opt_err = AMDGV_ERROR_GPUMON_VF_BUSY;
			goto out;
		}
	}

	if (num_vf > adapt->max_num_vf || num_vf <= 0) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPUMON_INVALID_VF_NUM,
				AMDGV_ERROR_32_32(num_vf, adapt->max_num_vf));
		opt_err = AMDGV_ERROR_GPUMON_INVALID_VF_NUM;
		goto out;
	}

	opt_err = amdgv_vfmgr_set_vf_num(adapt, num_vf);

out:
	return opt_err;
}

int amdgv_int_stop_to_pf_helper(struct amdgv_adapter *adapt)
{
	/* after event handle, world switch would resume */
	amdgv_sched_stop_all(adapt);
	if (!amdgv_sched_world_context_all_states_ok(adapt))
		amdgv_sched_reset_vf_auto(adapt);

	/* switch to PF for all blocks */
	if (amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL)) {
		amdgv_sched_reset_vf_auto(adapt);
		return AMDGV_FAILURE;
	}

	/* stop disptimer2 if we're in live update SAVE*/
	if (adapt->live_update_state == AMDGV_LIVE_UPDATE_SAVE &&
	    adapt->irqmgr.ih_funcs->toggle_disp_timer2)
		adapt->irqmgr.ih_funcs->toggle_disp_timer2(adapt, false);

	return 0;
}

/* position of instance value in sub_block_index of
 * ta_ras_trigger_error_input, the sub block uses lower 12 bits
 */
#define AMDGV_RAS_INST_MASK 0xfffff000
#define AMDGV_RAS_INST_SHIFT 0xc

int amdgv_int_ras_trigger_error(struct amdgv_adapter *adapt, struct amdgv_smi_ras_error_inject_info *data)
{
	struct ta_ras_trigger_error_input ras_data;
	int ret = 0;

	if (data->vf_idx >= adapt->num_vf)
		data->vf_idx = AMDGV_PF_IDX;

	if (NEED_SWITCH_TO_PF(adapt))
		ret = amdgv_int_stop_to_pf_helper(adapt);

	if (!ret) {
		uint32_t vf_idx = 0;
		uint32_t instance_mask;
		uint32_t dev_mask;

		vf_idx = data->vf_idx;
		ras_data.value = data->method;
		ras_data.block_id = data->block_id;
		ras_data.inject_error_type = data->inject_error_type;
		ras_data.sub_block_index = data->sub_block_index;
		ras_data.address = data->address;
		instance_mask = data->mask;

		if (ras_data.block_id == TA_RAS_BLOCK__UMC)
			ras_data.address = amdgv_gpa_to_global_spa(adapt, data->address, vf_idx);

		switch (ras_data.block_id) {
		case TA_RAS_BLOCK__GFX:
			dev_mask = GET_MASK(GC, instance_mask);
			break;
		case TA_RAS_BLOCK__SDMA:
			dev_mask = GET_MASK(SDMA0, instance_mask);
			break;
		case TA_RAS_BLOCK__VCN:
		case TA_RAS_BLOCK__JPEG:
		case TA_RAS_BLOCK__MMSCH:
			dev_mask = GET_MASK(VCN, instance_mask);
			break;
		default:
			dev_mask = instance_mask;
			break;
		}

		/* reuse sub_block_index for backward compatibility */
		dev_mask <<= AMDGV_RAS_INST_SHIFT;
		dev_mask &= AMDGV_RAS_INST_MASK;
		ras_data.sub_block_index |= dev_mask;

		if (ras_data.block_id == TA_RAS_BLOCK__XGMI_WAFL)
			ret = amdgv_xgmi_inject_error(adapt, &ras_data);
		else
			if (amdgv_psp_ras_trigger_error(adapt, &ras_data) != PSP_STATUS__SUCCESS)
				ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_int_ras_ta_load(struct amdgv_adapter *adapt, struct amdgv_smi_cmd_ras_ta_load *data)
{
	int ret = 0;
	struct psp_ras_context *ras_context = &(adapt->psp.ras_context);

	if (amdgv_psp_ras_terminate(adapt) != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("Failed to unload ras ta!\n");
		return AMDGV_FAILURE;
	}

	ret = amdgv_psp_ras_initialize(adapt, (uint8_t *)data->in_data_addr, data->in_data_len);
	if (ret || !ras_context->ras_initialized) {
		AMDGV_ERROR("Failed to load ras ta! ras_initialized value is %s, ret value is %d\n",
			ras_context->ras_initialized ? "true" : "false", ret);
		return AMDGV_FAILURE;
	}

	data->out_ras_session_id = ras_context->ras_session_id;

	return ret;
}

int amdgv_int_ras_ta_unload(struct amdgv_adapter *adapt, struct amdgv_smi_cmd_ras_ta_unload *data)
{
	int ret;

	ret = amdgv_psp_ras_terminate(adapt);
	if (ret) {
		AMDGV_ERROR("amdgv_psp_ras_terminate failed! ret code %d\n", ret);
	}

	return ret;
}

int amdgv_int_alloc_dump_cu_resource_memory(struct amdgv_adapter *adapt, struct amdgv_dump_cu_resource_size *dump_cu_resource_size,
											struct amdgv_dump_cu_resource_memory *output_data)
{
	int ret = AMDGV_FAILURE;

	if (adapt->gfx.funcs && adapt->gfx.funcs->alloc_dump_cu_resource_memory) {
		ret = adapt->gfx.funcs->alloc_dump_cu_resource_memory(adapt, dump_cu_resource_size, output_data);
	}

	return ret;
}

int amdgv_int_dump_cu_data(struct amdgv_adapter *adapt)
{
	int ret = 0;

	if (adapt->gfx.funcs && adapt->gfx.funcs->dump_cu_data) {
		if (adapt->bp_mode == AMDGV_BP_MODE_DISABLE)
			ret = amdgv_int_stop_to_pf_helper(adapt);
		if (ret) {
			AMDGV_ERROR("switch to pf failed! ret code %d\n", ret);
			return AMDGV_FAILURE;
		}

		ret = adapt->gfx.funcs->dump_cu_data(adapt);
	} else {
		AMDGV_ERROR("dump CU data not supported\n");
		return AMDGV_FAILURE;
	}
	return ret;
}

int amdgv_int_set_dump_cu_info(struct amdgv_adapter *adapt,
			       enum AMDGV_CU_DATA_TYPE cu_dump_type, uint32_t xcc_id,
			       bool use_extra_ring)
{
	uint32_t num_xcc;
	switch (cu_dump_type) {
	case AMDGV_CU_DATA_TYPE__LDS:	/* Fallthrough */
	case AMDGV_CU_DATA_TYPE__SGPRs: /* Fallthrough */
	case AMDGV_CU_DATA_TYPE__VGPRs:
		break;
	default:
		AMDGV_ERROR("Invalid CU dump type %d\n", cu_dump_type);
		return AMDGV_FAILURE;
	}

	num_xcc = adapt->mcp.gfx.num_xcc ? adapt->mcp.gfx.num_xcc : 1;
	if (xcc_id >= num_xcc) {
		AMDGV_ERROR("Invalid XCC ID %d, max is %d\n", xcc_id, num_xcc - 1);
		return AMDGV_FAILURE;
	}

	adapt->gfx.cu_dump_data_info.cu_dump_type = cu_dump_type;
	adapt->gfx.cu_dump_data_info.xcc_id = xcc_id;
	adapt->gfx.cu_dump_data_info.use_extra_ring = use_extra_ring;

	return 0;
}

void amdgv_int_free_dump_cu_resource_memory(struct amdgv_adapter *adapt)
{
	adapt->gfx.funcs->free_dump_cu_resource_memory(adapt);
}

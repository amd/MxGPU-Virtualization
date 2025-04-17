/*
 * Copyright (c) 2020-2021 Advanced Micro Devices, Inc. All rights reserved.
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
	int i;

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
	int i;

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
	if (amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL) == 0)
		AMDGV_INFO("Switch to PF OK.\n");
	else {
		AMDGV_INFO("Switch to PF Fail.\n");
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

static uint64_t amdgv_gpa_to_spa(struct amdgv_adapter *adapt, uint64_t gpa, uint32_t vf_idx)
{
	uint64_t addr;
	/* VF gpa convert to spa address */
	if (vf_idx < adapt->num_vf) {
		addr = (adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size +
			MBYTES_TO_BYTES(adapt->array_vf[vf_idx].fb_offset) + gpa);
		AMDGV_INFO("phy_node_id:%d, node_segment_size:0x%llx, vf_idx:%d, fb_offset:0x%x, gpa:0x%llx\n",
			adapt->xgmi.phy_node_id, adapt->xgmi.node_segment_size, vf_idx,
			MBYTES_TO_BYTES(adapt->array_vf[vf_idx].fb_offset), gpa);
		return addr;
	}

	/* PF gpa convert to spa address */
	return (adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size + gpa);
}

int amdgv_int_ras_trigger_error(struct amdgv_adapter *adapt, struct amdgv_smi_ras_error_inject_info *data)
{
	struct ta_ras_trigger_error_input *ras_data =
		(struct ta_ras_trigger_error_input *)data;
	int ret = 0;

	if (NEED_SWITCH_TO_PF(adapt)) {
		ret = amdgv_int_stop_to_pf_helper(adapt);
	}

	if (!ret) {
		uint32_t vf_idx = 0;
		uint32_t instance_mask;
		uint32_t dev_mask;

		vf_idx = data->vf_idx;
		ras_data->value = data->method;
		instance_mask = data->mask;

		AMDGV_DEBUG("RAS: ffbm is %s, VF %u, value:0x%llx\n",
			adapt->ffbm.enabled ? "enabled" : "disabled", vf_idx, ras_data->value);

		if (ras_data->block_id == TA_RAS_BLOCK__UMC)  {
			if (adapt->ffbm.enabled && (vf_idx < adapt->num_vf))
				ras_data->address = amdgv_ffbm_gpa_to_spa(adapt, data->address, vf_idx);
			else
				ras_data->address =
					amdgv_gpa_to_spa(adapt, data->address, vf_idx);
		}

		switch (ras_data->block_id) {
		case TA_RAS_BLOCK__GFX:
			dev_mask = GET_MASK(GC, instance_mask);
			break;
		case TA_RAS_BLOCK__SDMA:
			dev_mask = GET_MASK(SDMA0, instance_mask);
			break;
		case TA_RAS_BLOCK__VCN:
		case TA_RAS_BLOCK__JPEG:
			dev_mask = GET_MASK(VCN, instance_mask);
			break;
		default:
			dev_mask = instance_mask;
			break;
		}

		/* reuse sub_block_index for backward compatibility */
		dev_mask <<= AMDGV_RAS_INST_SHIFT;
		dev_mask &= AMDGV_RAS_INST_MASK;
		ras_data->sub_block_index |= dev_mask;

		if (ras_data->block_id == TA_RAS_BLOCK__XGMI_WAFL)
				ret = amdgv_xgmi_inject_error(adapt, ras_data);
		else
			if (amdgv_psp_ras_trigger_error(adapt, ras_data) != PSP_STATUS__SUCCESS)
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


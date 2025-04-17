/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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
#include "amdgv_xgmi.h"
#include "mi300_xgmi.h"
#include "mi300_powerplay.h"
#include "amdgv_psp_gfx_if.h"
#include "mi300/GC/gc_9_4_3_offset.h"
#include "mi300/GC/gc_9_4_3_sh_mask.h"
#include "mi300/XGMI/xgmi_6_4_0_sh_mask.h"
#include "mi300.h"

#define smnPCS_XGMI3X16_PCS_ERROR_STATUS 0x11a0020c
#define smnPCS_XGMI3X16_PCS_ERROR_NONCORRECTABLE_MASK   0x11a00218

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static uint32_t mi300_node_group_size_map[AMDGV_XGMI_FB_SHARING_MODE_NUM] = {
	[AMDGV_XGMI_FB_SHARING_MODE_DEFAULT] = 8,

	[AMDGV_XGMI_FB_SHARING_MODE_1] = 1,
	[AMDGV_XGMI_FB_SHARING_MODE_2] = 2,
	[AMDGV_XGMI_FB_SHARING_MODE_4] = 4,
	[AMDGV_XGMI_FB_SHARING_MODE_8] = 8,
	[AMDGV_XGMI_FB_SHARING_MODE_UNKNOWN] = 1
};

enum amdgv_xgmi_fb_sharing_mode
mi300_get_largest_xgmi_fb_sharing_mode(struct amdgv_adapter *adapt, uint32_t num_phy_nodes)
{
	switch (num_phy_nodes) {
	case 1:
		return AMDGV_XGMI_FB_SHARING_MODE_1;
	case 2:
		return AMDGV_XGMI_FB_SHARING_MODE_2;
	case 4:
		return AMDGV_XGMI_FB_SHARING_MODE_4;
	case 8:
		return AMDGV_XGMI_FB_SHARING_MODE_8;
	default:
		return AMDGV_XGMI_FB_SHARING_MODE_1;
	}
}



static int mi300_xgmi_reset_hive_fb_sharing_config(struct amdgv_adapter *adapt,
			struct amdgv_hive_info *hive,
			enum amdgv_xgmi_fb_sharing_mode mode)
{
	struct amdgv_adapter *entry, *tmp;

	amdgv_list_for_each_entry_safe(entry, tmp, &hive->adapt_list,
			struct amdgv_adapter, xgmi.head) {
		entry->xgmi.fb_sharing_mode = mode;
	}

	amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_XGMI_FB_SHARING_SETTING_RESET, 0);
	return 0;
}


/*
 * It is possible a hive is only partially updated with a new FB sharing mode
 * before encountering an issue. In this scenario, reset the FB sharing mode
 * back to default and notify the user.
 */
int mi300_xgmi_sanitize_hive_fb_sharing_config(struct amdgv_adapter *adapt,
			struct amdgv_hive_info *hive)
{
	struct amdgv_adapter *entry, *tmp;

	amdgv_list_for_each_entry_safe(entry, tmp, &hive->adapt_list,
			struct amdgv_adapter, xgmi.head) {
		if (adapt->xgmi.fb_sharing_mode != entry->xgmi.fb_sharing_mode) {
			mi300_xgmi_reset_hive_fb_sharing_config(adapt, hive,
				 mi300_get_largest_xgmi_fb_sharing_mode(adapt, adapt->xgmi.phy_nodes_num));
			break;
		}
	}

	return 0;
}

static int mi300_xgmi_sanitize_node_fb_sharing_config(struct amdgv_adapter *adapt)
{
	enum amdgv_xgmi_fb_sharing_mode largest_mode;

	/* set fb_sharing_mode to be AMDGV_XGMI_FB_SHARING_MODE_1
	 * if under spartial partition senario
	 */
	if (adapt->num_vf > 1) {
		adapt->xgmi.fb_sharing_mode = AMDGV_XGMI_FB_SHARING_MODE_1;
	}

	largest_mode =
		mi300_get_largest_xgmi_fb_sharing_mode(adapt, adapt->xgmi.phy_nodes_num);

	switch (adapt->xgmi.fb_sharing_mode) {
	case AMDGV_XGMI_FB_SHARING_MODE_DEFAULT:
		adapt->xgmi.fb_sharing_mode = largest_mode;
		break;
	case AMDGV_XGMI_FB_SHARING_MODE_1:
	case AMDGV_XGMI_FB_SHARING_MODE_2:
	case AMDGV_XGMI_FB_SHARING_MODE_4:
	case AMDGV_XGMI_FB_SHARING_MODE_8:
		if (adapt->xgmi.fb_sharing_mode > largest_mode) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_XGMI_FB_SHARING_SETTING_ERROR, adapt->xgmi.fb_sharing_mode);
			adapt->xgmi.fb_sharing_mode = largest_mode;
		}
		break;
	case AMDGV_XGMI_FB_SHARING_MODE_CUSTOM:
		break;
	default:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_XGMI_FB_SHARING_SETTING_ERROR, adapt->xgmi.fb_sharing_mode);
		adapt->xgmi.fb_sharing_mode = largest_mode;
		break;
	}

	AMDGV_INFO("XGMI FB Sharing mode = %d\n", adapt->xgmi.fb_sharing_mode);
	return 0;
}

static int mi300_init_xgmi_hw_info(struct amdgv_adapter *adapt)
{
	uint32_t xgmi_lfb_cntl = 0;
	uint32_t max_region = 0;
	uint64_t node_seg_size;
	struct amdgv_xgmi *xgmi = &adapt->xgmi;

	xgmi_lfb_cntl = RREG32_SOC15(GC, GET_INST(GC, 0), regMC_VM_XGMI_LFB_CNTL);
	AMDGV_DEBUG("xgmi_lfb_cntl = 0x%x\n", xgmi_lfb_cntl);
	max_region = REG_GET_FIELD(xgmi_lfb_cntl, MC_VM_XGMI_LFB_CNTL, PF_MAX_REGION);

	if (max_region) {
		xgmi->phy_nodes_num = max_region + 1;

		if (xgmi->phy_nodes_num > 8)
			return -1;

		xgmi->phy_node_id =
			REG_GET_FIELD(xgmi_lfb_cntl, MC_VM_XGMI_LFB_CNTL, PF_LFB_REGION);
		if (xgmi->phy_node_id > 7)
			return -1;

		xgmi->socket_id = REG_GET_FIELD(RREG32_SOC15(SMUIO, 0, regSMUIO_MCM_CONFIG),
						SMUIO_MCM_CONFIG, SOCKET_ID);

		node_seg_size = RREG32_SOC15(GC, GET_INST(GC, 0), regMC_VM_XGMI_LFB_SIZE);
		xgmi->node_segment_size =
			REG_GET_FIELD(node_seg_size, MC_VM_XGMI_LFB_SIZE, PF_LFB_SIZE) << 24;

		AMDGV_INFO("xgmi: nodes_num = %d id = %d segment_size = 0x%llx\n",
			xgmi->phy_nodes_num, xgmi->phy_node_id,
			xgmi->node_segment_size);
	}

	return 0;
}

/**
 * mi300_get_fb_sharing_mode_mask - query auto mode equivalent custom sharing mask
 *
 * @adapt: calling adapt
 * @mode: mode to query on
 *
 * Return the custom mode sharing mask that is equivalent the mode specified.
 * ((1 << group_size) - 1) get bit mask block for the mode, then shift left to align with
 * sharing group position.
 */
static uint32_t mi300_get_fb_sharing_mode_mask(struct amdgv_adapter *adapt,
			enum amdgv_xgmi_fb_sharing_mode mode)
{
	uint32_t group_size;
	if (mode == AMDGV_XGMI_FB_SHARING_MODE_CUSTOM)
		return 0;
	if (mode == AMDGV_XGMI_FB_SHARING_MODE_DEFAULT)
		mode = mi300_get_largest_xgmi_fb_sharing_mode(adapt, adapt->xgmi.phy_nodes_num);

	group_size = mi300_node_group_size_map[mode];
	return (((1 << group_size) - 1) << (group_size * (adapt->xgmi.phy_node_id / group_size)));
}

static bool mi300_xgmi_is_fb_sharing_allowed(struct amdgv_adapter *adapt,
					     uint32_t src_phy_node_id,
					     uint32_t dest_phy_node_id,
					     enum amdgv_xgmi_fb_sharing_mode mode)
{
	uint32_t group_size = mi300_node_group_size_map[mode];

	/*
	 * before default mode is sanitized,
	 * group size should be 1 for multi-vf and 8 for 1 vf
	 * later sanitization should convert default mode to 1 for multi-vf and 8 for 1 vf
	 */
	if (mode == AMDGV_XGMI_FB_SHARING_MODE_DEFAULT && adapt->num_vf != 1)
		group_size = mi300_node_group_size_map[AMDGV_XGMI_FB_SHARING_MODE_1];

	if (mode > MI300_XGMI_MAX_SUPPORTED_MODE) {
		AMDGV_ERROR("MI300 does not support FB Sharing mode: %d", mode);
		return false;
	}

	if (mode == AMDGV_XGMI_FB_SHARING_MODE_CUSTOM) {
		struct amdgv_hive_info *hive;
		struct amdgv_adapter *next_adapt;
		hive = amdgv_get_xgmi_hive(adapt);
		if (!hive) {
			AMDGV_ERROR("Failed to get XGMI hive when checking sharing setting.\n");
			return false;
		}
		amdgv_list_for_each_entry(next_adapt, &hive->adapt_list,
					struct amdgv_adapter, xgmi.head) {
			if (next_adapt->xgmi.phy_node_id == src_phy_node_id) {
				return ((1 << dest_phy_node_id) & next_adapt->xgmi.custom_mode_sharing_mask);
			}
		}
	}

	if (group_size) {
		return ((src_phy_node_id / group_size) == (dest_phy_node_id / group_size));
	} else {
		/* Something went wrong, denominator can't be 0 */
		return false;
	}
}

static int mi300_xgmi_early_sw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_xgmi_early_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_xgmi_early_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	if (!in_whole_gpu_reset()) {
		ret = mi300_init_xgmi_hw_info(adapt);
		if (ret)
			return ret;
	}

	return ret;
}

static int mi300_xgmi_early_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}


static int mi300_xgmi_late_sw_init(struct amdgv_adapter *adapt)
{
	adapt->xgmi.is_fb_sharing_allowed = mi300_xgmi_is_fb_sharing_allowed;

	adapt->xgmi.get_fb_sharing_mode_mask = mi300_get_fb_sharing_mode_mask;

	return 0;
}

static int mi300_xgmi_late_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->xgmi.is_fb_sharing_allowed = NULL;

	adapt->xgmi.get_fb_sharing_mode_mask = NULL;

	return 0;
}

static int mi300_xgmi_late_hw_init(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;
	int ret = 0;
	struct amdgv_adapter *entry, *tmp;

	if (adapt->xgmi.phy_nodes_num > 1) {
		if (mi300_xgmi_sanitize_node_fb_sharing_config(adapt))
			return AMDGV_FAILURE;

		hive = amdgv_get_xgmi_hive(adapt);
		if (!hive) {
			AMDGV_ERROR("XGMI: node 0x%llx, can not match hive "
				    "0x%llx in the hive list.\n",
				    adapt->xgmi.node_id, adapt->xgmi.hive_id);
			return AMDGV_FAILURE;
		}

		if (!adapt->reset.in_xgmi_chain_reset) {
			/* Sync counter to get the last adapt that passed
			 * xgmi_sanitize_node_fb_sharing_config
			 *
			 * xgmi_sanitize_hive_fb_sharing_config requires all node has passed
			 * xgmi_sanitize_node_fb_sharing_config otherwize default mode is not
			 * converted to proper actual mode
			 */
			if (oss_atomic_inc_return(&hive->tb_drv_init.thread_count) == adapt->xgmi.phy_nodes_num) {
				/* reset counter after the last adapt is identified */
				while (oss_atomic_dec_return(&hive->tb_drv_init.thread_count) > 0)
					;
				if (amdgv_xgmi_is_hive_bad(adapt))
					amdgv_device_handle_bad_hive(adapt);
				/*
				* If the other adapters have their HW_INIT complete, then
				* wake up the event thread.
				*
				* If the other adapters are in the middle of HW_INIT, do not
				* wake up the event thread. The event thread will be signalled
				* unconditionally at the end of HW_INIT and this event will be processed.
				*/
				mi300_xgmi_sanitize_hive_fb_sharing_config(adapt, hive);
				AMDGV_DEBUG("Last initialized device in hive initiating XGMI Topology update.");
				amdgv_list_for_each_entry_safe(entry, tmp, &hive->adapt_list,
						struct amdgv_adapter, xgmi.head) {
					if (entry == adapt)
						continue;

					if (entry->status == AMDGV_STATUS_HW_INIT)
						amdgv_sched_queue_event(entry, AMDGV_PF_IDX,
							AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY, AMDGV_SCHED_BLOCK_ALL);
					else
						amdgv_sched_queue_event_no_signal(entry, AMDGV_PF_IDX,
							AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY, AMDGV_SCHED_BLOCK_ALL);
				}
			}
		}

		/* Simply refresh the topology settings in PSP, including in Reset */
		ret = amdgv_xgmi_update_topology(adapt);
	}

	return ret;
}

static int mi300_xgmi_late_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static void mi300_xgmi_query_ras_error_count(struct amdgv_adapter *adapt, void *ras_error_status)
{
	adapt->mca.funcs->pop_block_error_count(adapt,
						AMDGV_RAS_BLOCK__XGMI_WAFL,
						ras_error_status);
}
static int mi300_ras_error_inject_xgmi(struct amdgv_adapter *adapt,
				       struct ta_ras_trigger_error_input *block_info)
{
	int ret = 0;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->set_df_cstate)
		if (adapt->pp.pp_funcs->set_df_cstate(adapt, DF_CSTATE_DISALLOW))
			AMDGV_WARN("Failed to disallow df cstate");


	if (mi300_smu_error_inject_set_pm_policy(adapt,
			AMDGV_PP_PM_POLICY_XGMI_PLPD,
			PP_XGMI_PLPD_MODE_DISABLE))
		AMDGV_WARN("Failed to disallow XGMI power down");

	ret = amdgv_psp_ras_trigger_error(adapt, block_info);

	if (amdgv_ras_intr_triggered())
		return ret;

	/* This call may fail if Fatal error interrupt is detected.
	 * The policy will be restored as part of regular mode 1 reset sequence. */
	mi300_smu_error_inject_restore_pm_policy(adapt, AMDGV_PP_PM_POLICY_XGMI_PLPD);

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->set_df_cstate)
		if (adapt->pp.pp_funcs->set_df_cstate(adapt, DF_CSTATE_ALLOW))
			AMDGV_WARN("Failed to allow df cstate");

	return ret;
}

const struct amdgv_xgmi_funcs  mi300_xgmi_funcs = {
	.query_ras_error_count = mi300_xgmi_query_ras_error_count,
	.reset_ras_error_count = NULL,
	.ras_error_inject = mi300_ras_error_inject_xgmi,
};


void mi300_xgmi_set_ras_funcs(struct amdgv_adapter *adapt)
{
	adapt->xgmi.funcs = &mi300_xgmi_funcs;
}

const struct amdgv_init_func mi300_xgmi_early_func = {
	.name = "mi300_xgmi_func",
	.sw_init = mi300_xgmi_early_sw_init,
	.sw_fini = mi300_xgmi_early_sw_fini,
	.hw_init = mi300_xgmi_early_hw_init,
	.hw_fini = mi300_xgmi_early_hw_fini,
};

const struct amdgv_init_func mi300_xgmi_late_func = {
	.name = "mi300_xgmi_func",
	.sw_init = mi300_xgmi_late_sw_init,
	.sw_fini = mi300_xgmi_late_sw_fini,
	.hw_init = mi300_xgmi_late_hw_init,
	.hw_fini = mi300_xgmi_late_hw_fini,
};

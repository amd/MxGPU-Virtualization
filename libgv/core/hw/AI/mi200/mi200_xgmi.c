/*
 * Copyright (c) 2021-2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_xgmi.h>
#include <amdgv_psp_gfx_if.h>

#include "mi200_gpumon.h"
#include "mi200_xgmi.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static uint32_t mi200_node_group_size_map[AMDGV_XGMI_FB_SHARING_MODE_NUM] = {
	[AMDGV_XGMI_FB_SHARING_MODE_DEFAULT] = 4,
	[AMDGV_XGMI_FB_SHARING_MODE_1] = 1,
	[AMDGV_XGMI_FB_SHARING_MODE_2] = 2,
	[AMDGV_XGMI_FB_SHARING_MODE_4] = 4,
	[AMDGV_XGMI_FB_SHARING_MODE_UNKNOWN] = 1
};

static enum amdgv_xgmi_fb_sharing_mode
mi200_get_largest_xgmi_fb_sharing_mode(struct amdgv_adapter *adapt, uint32_t num_phy_nodes)
{
	switch (num_phy_nodes) {
	case 1:
		return AMDGV_XGMI_FB_SHARING_MODE_1;
	case 2:
		return AMDGV_XGMI_FB_SHARING_MODE_2;
	case 4:
		return AMDGV_XGMI_FB_SHARING_MODE_4;
	default:
		return AMDGV_XGMI_FB_SHARING_MODE_1;
	}
}

/**
 * mi200_get_fb_sharing_mode_mask - query auto mode equivalent custom sharing mask
 *
 * @adapt: calling adapt
 * @mode: mode to query on
 *
 * Return the custom mode sharing mask that is equivalent the mode specified.
 * ((1 << group_size) - 1) get bit mask block for the mode, then shift left to align with
 * sharing group position.
 */
static uint32_t mi200_get_fb_sharing_mode_mask(struct amdgv_adapter *adapt,
			enum amdgv_xgmi_fb_sharing_mode mode)
{
	uint32_t group_size;
	if (mode == AMDGV_XGMI_FB_SHARING_MODE_CUSTOM)
		return 0;
	if (mode == AMDGV_XGMI_FB_SHARING_MODE_DEFAULT)
		mode = mi200_get_largest_xgmi_fb_sharing_mode(adapt, adapt->xgmi.phy_nodes_num);

	group_size = mi200_node_group_size_map[mode];
	return (((1 << group_size) - 1) << (group_size * (adapt->xgmi.phy_node_id / group_size)));
}

static bool mi200_xgmi_is_fb_sharing_allowed(struct amdgv_adapter *adapt,
					     uint32_t src_phy_node_id,
					     uint32_t dest_phy_node_id,
					     enum amdgv_xgmi_fb_sharing_mode mode)
{
	uint32_t group_size = mi200_node_group_size_map[mode];

	/*
	 * before default mode is sanitized,
	 * group size should be 1 for multi-vf and 8 for 1 vf
	 * later sanitization should convert default mode to 1 for multi-vf and 8 for 1 vf
	 */
	if (mode == AMDGV_XGMI_FB_SHARING_MODE_DEFAULT && adapt->num_vf != 1)
		group_size = mi200_node_group_size_map[AMDGV_XGMI_FB_SHARING_MODE_1];

	if (mode > MI200_XGMI_MAX_SUPPORTED_MODE) {
		AMDGV_ERROR("mi200 does not support FB Sharing mode: %d", mode);
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


static int mi200_xgmi_sw_init(struct amdgv_adapter *adapt)
{
	adapt->xgmi.is_fb_sharing_allowed = mi200_xgmi_is_fb_sharing_allowed;

	adapt->xgmi.get_fb_sharing_mode_mask = mi200_get_fb_sharing_mode_mask;

	return 0;
}

static int mi200_xgmi_sw_fini(struct amdgv_adapter *adapt)
{

	return 0;
}

static int mi200_xgmi_hw_init(struct amdgv_adapter *adapt)
{

	return 0;
}

static int mi200_xgmi_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi200_xgmi_func = {
	.name = "mi200_xgmi_func",
	.sw_init = mi200_xgmi_sw_init,
	.sw_fini = mi200_xgmi_sw_fini,
	.hw_init = mi200_xgmi_hw_init,
	.hw_fini = mi200_xgmi_hw_fini,
};

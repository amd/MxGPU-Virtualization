/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI300_XGMI_H
#define MI300_XGMI_H

#define MI300_XGMI_MAX_SUPPORTED_MODE AMDGV_XGMI_FB_SHARING_MODE_CUSTOM

enum amdgv_xgmi_fb_sharing_mode
mi300_get_largest_xgmi_fb_sharing_mode(struct amdgv_adapter *adapt, uint32_t num_phy_nodes);

int mi300_init_xgmi_info(struct amdgv_adapter *adapt);
int mi300_xgmi_sanitize_hive_fb_sharing_config(struct amdgv_adapter *adapt,
			struct amdgv_hive_info *hive);
void mi300_xgmi_set_ras_funcs(struct amdgv_adapter *adapt);

#endif
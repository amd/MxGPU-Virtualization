/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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
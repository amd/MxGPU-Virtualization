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

#ifndef AMDGV_GFXHUB_V1_2_H
#define AMDGV_GFXHUB_V1_2_H

void gfxhub_v1_2_gart_enable(struct amdgv_adapter *adapt);
void gfxhub_v1_2_gart_fini(struct amdgv_adapter *adapt);
void gfxhub_v1_2_vmhub_hook(struct amdgv_adapter *adapt);
void gfxhub_v1_2_enable_system_context(struct amdgv_adapter *adapt);
uint64_t gfxhub_v1_2_get_mc_fb_offset(struct amdgv_adapter *adapt);
void gfxhub_v1_2_enable_xgmi(struct amdgv_adapter *adapt);
#endif

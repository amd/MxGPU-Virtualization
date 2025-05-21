/*
 * Copyright (C) 2022 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef NAVI32_GFX_H
#define NAVI32_GFX_H

uint32_t navi32_gfx_cu_count(struct amdgv_adapter *adapt);
uint32_t navi32_gfx_atc_ats_invalidate(struct amdgv_adapter *adapt);
void navi32_gfx_program_golden_settings(struct amdgv_adapter *adapt);
void navi32_gfx_halt_gpu_state(struct amdgv_adapter *adapt);
void navi32_gfx_unhalt_gpu_state(struct amdgv_adapter *adapt);
int navi32_gfx_check_rlc_autoload_complete(struct amdgv_adapter *adapt);

#endif

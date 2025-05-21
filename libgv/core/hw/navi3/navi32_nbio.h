/*
 * Copyright 2023 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef NAVI32_NBIO_H
#define NAVI32_NBIO_H
#include <navi3/NBIO/nbio_4_3_0_offset.h>

void navi32_nbio_set_ras_funcs(struct amdgv_adapter *adapt);
void navi32_nbio_get_vram_vendor(struct amdgv_adapter *adapt);
uint32_t navi32_nbio_get_total_vram_size(struct amdgv_adapter *adapt);
void navi32_hdp_flush(struct amdgv_adapter *adapt);
#define SOC15_REG_OFFSET_SMN_NBIO_BLOCK(idx_vf, blk, reg) \
    (idx_vf == 0 ? smn##blk##_DEV0_EPF0_VF0_##reg : \
    (idx_vf == 1 ? smn##blk##_DEV0_EPF0_VF1_##reg : \
    (idx_vf == 2 ? smn##blk##_DEV0_EPF0_VF2_##reg : \
    (idx_vf == 3 ? smn##blk##_DEV0_EPF0_VF3_##reg : \
    (idx_vf == 4 ? smn##blk##_DEV0_EPF0_VF4_##reg : \
    (idx_vf == 5 ? smn##blk##_DEV0_EPF0_VF5_##reg : \
    (idx_vf == 6 ? smn##blk##_DEV0_EPF0_VF6_##reg : \
    (idx_vf == 7 ? smn##blk##_DEV0_EPF0_VF7_##reg : \
    (idx_vf == 8 ? smn##blk##_DEV0_EPF0_VF8_##reg : \
    (idx_vf == 9 ? smn##blk##_DEV0_EPF0_VF9_##reg : \
    (idx_vf == 10 ? smn##blk##_DEV0_EPF0_VF10_##reg : \
    (idx_vf == 11 ? smn##blk##_DEV0_EPF0_VF11_##reg : \
    (idx_vf == 12 ? smn##blk##_DEV0_EPF0_VF12_##reg : \
    (idx_vf == 13 ? smn##blk##_DEV0_EPF0_VF13_##reg : \
    (idx_vf == 14 ? smn##blk##_DEV0_EPF0_VF14_##reg : \
    (idx_vf == 15 ? smn##blk##_DEV0_EPF0_VF15_##reg : \
    (cfg##blk##_DEV0_EPF0_0_##reg)))))))))))))))))
#endif

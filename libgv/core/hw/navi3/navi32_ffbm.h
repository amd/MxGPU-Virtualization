/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI3_FFBM_H
#define NAVI3_FFBM_H

#define FFBM_SET_REG(reg, val) WREG32(SOC15_REG_OFFSET(GC, 0, regGCUTCL2_##reg), (val)); WREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMUTCL2_##reg), (val));


#endif
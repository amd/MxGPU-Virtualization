/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI300_VCN_H
#define MI300_VCN_H

void mi300_vcn_get_mmsch_regid_instid(struct amdgv_adapter *adapt,
							uint32_t hw_sched_id, uint64_t *reg, bool control);

#endif

/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI350_VCN_H
#define MI350_VCN_H

void mi350_vcn_get_mmsch_regid_instid(struct amdgv_adapter *adapt,
							uint32_t hw_sched_id, uint64_t *reg, bool control);

/**
 * Set MMSCH doorbell address base for MI350
 * @adapt: AMDGPU adapter structure
 */
void mi350_vcn_set_mmsch_doorbell_addr_base(struct amdgv_adapter *adapt);

#endif

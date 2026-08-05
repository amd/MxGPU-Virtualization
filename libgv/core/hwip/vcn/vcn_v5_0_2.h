/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

 #ifndef _VCN_V5_0_2_H_
 #define _VCN_V5_0_2_H_

void vcn_v5_0_2_get_mmsch_regid_instid(struct amdgv_adapter *adapt, uint32_t hw_sched_id, uint64_t *reg, bool control);
void vcn_v5_0_2_set_mmsch_doorbell_addr_base(struct amdgv_adapter *adapt);

void vcn_v5_0_2_set_ras_funcs(struct amdgv_adapter *adapt);

 #endif // _VCN_V5_0_2_H_

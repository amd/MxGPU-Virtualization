/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI300_NBIO_H
#define MI300_NBIO_H

#include "amdgv_nps.h"

void mi300_nbio_enable_vf_access_mmio_over_512k(struct amdgv_adapter *adapt);
bool mi300_nbio_vbios_need_post(struct amdgv_adapter *adapt);
void mi300_nbio_assign_sdma_doorbell(struct amdgv_adapter *adapt, int instance,
				     int doorbell_index, int doorbell_size);
void mi300_nbio_assign_mmsch_doorbell(struct amdgv_adapter *adapt);
uint32_t mi300_nbio_get_config_memsize(struct amdgv_adapter *adapt);
uint32_t mi300_nbio_get_total_vram_size(struct amdgv_adapter *adapt);
void mi300_nbio_get_vram_vendor(struct amdgv_adapter *adapt);
void mi300_read_partition_data(struct amdgv_adapter *adapt);
void mi300_nbio_enable_pf_rrmt(struct amdgv_adapter *adapt);
void mi300_nbio_dump_sriov_state(struct amdgv_adapter *adapt);
void mi300_nbio_vfen_pre_enable_sriov(struct amdgv_adapter *adapt);
void mi300_nbio_vfen_post_enable_sriov(struct amdgv_adapter *adapt);
void mi300_nbio_dis_multicast_to_xcd1(struct amdgv_adapter *adapt);
void mi300_nbio_enable_doorbell_aperture(struct amdgv_adapter *adapt, bool enable);
void mi300_nbio_disable_vf_flr(struct amdgv_adapter *adapt);
void mi300_nbio_set_doorbell_fence(struct amdgv_adapter *adapt);
void mi300_nbio_assign_sdma_to_vf(struct amdgv_adapter *adapt);
void mi308_nbio_assign_sdma_to_vf(struct amdgv_adapter *adapt);
void mi300_nbio_assign_vcn_to_vf(struct amdgv_adapter *adapt);
void mi300_nbio_dump_partition_config(struct amdgv_adapter *adapt);
int mi300_nbio_pcie_curr_link_speed(struct amdgv_adapter *adapt);
int mi300_nbio_pcie_curr_link_width(struct amdgv_adapter *adapt);
int mi300_nbio_get_pcie_replay_count(struct amdgv_adapter *adapt);
void nbio_v7_9_set_ras_funcs(struct amdgv_adapter *adapt);
void mi300_hdp_flush(struct amdgv_adapter *adapt);
#endif

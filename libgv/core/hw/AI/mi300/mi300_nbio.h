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

#ifndef MI300_NBIO_H
#define MI300_NBIO_H

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
int mi300_nbio_get_curr_memory_partition_mode(
	struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode *memory_partition_mode);
uint32_t mi300_nbio_get_accelerator_partition_mode(struct amdgv_adapter *adapt);
uint32_t mi300_nbio_get_accelerator_partition_mode_default_setting(
	struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode memory_partition_mode);
bool mi300_nbio_is_accelerator_partition_mode_supported(
	struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode memory_partition_mode,
	uint32_t accelerator_partition_mode);
bool mi300_nbio_is_partition_mode_combination_supported(
	struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode memory_partition_mode,
	uint32_t accelerator_partition_mode);
int mi300_nbio_get_pcie_replay_count(struct amdgv_adapter *adapt);
void nbio_v7_9_set_ras_funcs(struct amdgv_adapter *adapt);
#endif

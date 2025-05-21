/*
 * Copyright 2022-2024 Advanced Micro Devices, Inc.
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

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_misc.h"
#include "amdgv_nbio.h"
#include "amdgv_gfx.h"
#include "amdgv_hsa.h"
#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include "navi32_gfx.h"
#include "navi32_psp.h"
#include "gfx_v11_0.h"
#include "navi32_gpuiov.h"

static const uint32_t this_block = AMDGV_GFX_BLOCK;

#define GFX11_MEC_HPD_SIZE	2048
#define GFX11_CU_ID_MAX 20
#define LDS_SIZE 65536

static const hsa_signal_t signal = { 0 };

static const kernel_descriptor_t gfx_v11_dump_lds_kernelobj = {
	.group_segment_fixed_size = 0x10000,
	.private_segment_fixed_size = 0x0,
	.kernarg_size = 0x0,
	.reserved1 = 0x0,
	.kernel_code_entry_byte_offset = 0x0,
	.reserved2 = {0x0, 0x0, 0x0, 0x0, 0x0},
	.compute_pgm_rsrc3 = 0x0,
	.compute_pgm_rsrc1 = 0x40ac0001,
	.compute_pgm_rsrc2 = 0x4,
	.enable_sgpr_private_segment_buffer = 0,
	.enable_sgpr_dispatch_ptr = 0,
	.enable_sgpr_queue_ptr = 0,
	.enable_sgpr_kernarg_segment_ptr = 1,
	.enable_sgpr_dispatch_id = 0,
	.enable_sgpr_flat_scratch_init = 0,
	.enable_sgpr_private_segment_size = 0,
	.reserved3 = 0,
	.enable_wavefront_size32 = 1,
	.uses_dynamic_stack = 0,
	.reserved4 = 0,
	.kernarg_preload_spec_length = 0,
	.kernarg_preload_spec_offset = 0,
	.reserved5 = 0x0
};

static const uint32_t gfx_v11_dump_lds_shader[] = {
	0xbfbd0000, 0xf4040280, 0xf8000000, 0xf4040300,
	0xf8000008, 0xbf89fc07, 0xb880f817, 0x9303ff00,
	0x00030012, 0xbe8500ff, 0x00100000, 0x96050503,
	0x9303ff00, 0x00010010, 0xbe8600ff, 0x00080000,
	0x96060603, 0x9303ff00, 0x0004000a, 0x84038103,
	0x9304ff00, 0x00010008, 0x8c030403, 0xbe8700ff,
	0x00004000, 0x96070703, 0x80030607, 0x80030503,
	0x8004030c, 0x8205800d, 0x84038203, 0x8002030a,
	0x8203800b, 0x7e020280, 0xbe810080, 0x7e040281,
	0x7e080204, 0x7e0a0205, 0xd7006a04, 0x00020900,
	0x400a0b01, 0xdc600000, 0x007c0204, 0xd7006a04,
	0x000208ff, 0x00000400, 0x400a0a80, 0x80018101,
	0xbf079001, 0xbfa2fff7, 0x30000082, 0xbe810080,
	0x7e080202, 0x7e0a0203, 0xd7006a04, 0x00020900,
	0x400a0b01, 0xd8d80000, 0x06000000, 0xbf89fc07,
	0xdc680000, 0x007c0604, 0xd7006a04, 0x000208ff,
	0x00001000, 0x400a0a80, 0x80018101, 0xbf079001,
	0xbfa2fff4, 0xbfb00000
};

static const kernel_descriptor_t gfx_v11_dump_sgpr_kernelobj = {
	.group_segment_fixed_size = 0x0,
	.private_segment_fixed_size = 0x0,
	.kernarg_size = 0x0,
	.reserved1 = 0x0,
	.kernel_code_entry_byte_offset = 0x0,
	.reserved2 = {0x0, 0x0, 0x0, 0x0, 0x0},
	.compute_pgm_rsrc3 = 0x0,
	.compute_pgm_rsrc1 = 0x40ac0001,
	.compute_pgm_rsrc2 = 0x84,
	.enable_sgpr_private_segment_buffer = 0,
	.enable_sgpr_dispatch_ptr = 0,
	.enable_sgpr_queue_ptr = 0,
	.enable_sgpr_kernarg_segment_ptr = 1,
	.enable_sgpr_dispatch_id = 0,
	.enable_sgpr_flat_scratch_init = 0,
	.enable_sgpr_private_segment_size = 0,
	.reserved3 = 0,
	.enable_wavefront_size32 = 1,
	.uses_dynamic_stack = 0,
	.reserved4 = 0,
	.kernarg_preload_spec_length = 0,
	.kernarg_preload_spec_offset = 0,
	.reserved5 = 0x0
};

static const uint32_t gfx_v11_dump_sgpr_shader[] = {
	0xd7610001, 0x00010000, 0xd7610001, 0x00010201, 0xd7610001, 0x00010402, 0xd7610001, 0x00010603,
	0xd7610001, 0x00010804, 0xd7610001, 0x00010a05, 0xd7610001, 0x00010c06, 0xd7610001, 0x00010e07,
	0xd7610001, 0x00011008, 0xd7610001, 0x00011209, 0xd7610001, 0x0001140a, 0xd7610001, 0x0001160b,
	0xd7610001, 0x0001180c, 0xd7610001, 0x00011a0d, 0xd7610001, 0x00011c0e, 0xd7610001, 0x00011e0f,
	0xf4040280, 0xf8000000, 0xf4040300, 0xf8000008, 0xbf89fc07, 0xb880f817, 0xbf89fc07, 0x9303ff00,
	0x00030012, 0xbe8500ff, 0x00004000, 0x96059005, 0x96050503, 0x9303ff00, 0x00010010, 0xbe8600ff,
	0x00002000, 0x96069006, 0x96060603, 0x9303ff00, 0x0004000a, 0x84038103, 0x9304ff00, 0x00010008,
	0x8c030403, 0xbe8700ff, 0x00000100, 0x96079007, 0x96070703, 0x9303ff00, 0x00010009, 0xbe8800ff,
	0x00000080, 0x96089008, 0x96080803, 0x9303ff00, 0x00050000, 0xbe8900ff, 0x00000080, 0x96090903,
	0x80030809, 0x80030703, 0x80030603, 0x80030503, 0x8004030c, 0x8205800d, 0x84038203, 0x8002030a,
	0x8203800b, 0xbe8a017e, 0xbefe0181, 0xd760000c, 0x00010100, 0xd760000d, 0x00010101, 0xd760000e,
	0x00010102, 0xd760000f, 0x00010103, 0xd7600009, 0x00010105, 0xd7610000, 0x00010002, 0xd7610001,
	0x00010003, 0xd7610002, 0x00010004, 0xd7610003, 0x00010005, 0x7e0a02ff, 0x02020202, 0xdc6a0000,
	0x007c0502, 0xb888f817, 0xbf89fc07, 0x7e0a0208, 0xdc6a0000, 0x007c0500, 0x7e0a0280, 0xdc6a0004,
	0x007c0500, 0xdc6a0008, 0x007c0500, 0xdc6a000c, 0x007c0500, 0x7e0a02ff, 0x01010101, 0xdc6a0004,
	0x007c0502, 0xd7600006, 0x00010901, 0x7e0a0206, 0xdc6a0010, 0x007c0500, 0xd7600006, 0x00010b01,
	0x7e0a0206, 0xdc6a0014, 0x007c0500, 0xd7600006, 0x00010d01, 0x7e0a0206, 0xdc6a0018, 0x007c0500,
	0xd7600006, 0x00010f01, 0x7e0a0206, 0xdc6a001c, 0x007c0500, 0x7e0a02ff, 0x01010101, 0xdc6a0008,
	0x007c0502, 0xd7600006, 0x00011101, 0x7e0a0206, 0xdc6a0020, 0x007c0500, 0xd7600006, 0x00011301,
	0x7e0a0206, 0xdc6a0024, 0x007c0500, 0xd7600006, 0x00011501, 0x7e0a0206, 0xdc6a0028, 0x007c0500,
	0xd7600006, 0x00011701, 0x7e0a0206, 0xdc6a002c, 0x007c0500, 0x7e0a02ff, 0x01010101, 0xdc6a000c,
	0x007c0502, 0xd7600006, 0x00011901, 0x7e0a0206, 0xdc6a0030, 0x007c0500, 0xd7600006, 0x00011b01,
	0x7e0a0206, 0xdc6a0034, 0x007c0500, 0xd7600006, 0x00011d01, 0x7e0a0206, 0xdc6a0038, 0x007c0500,
	0xd7600006, 0x00011f01, 0x7e0a0206, 0xdc6a003c, 0x007c0500, 0xbe880090, 0xbefd0008, 0xd7006a00,
	0x000200c0, 0x40020280, 0xd7006a02, 0x00020490, 0x40060680, 0x7e0a02ff, 0x01010101, 0xdc6a0000,
	0x007c0502, 0xdc6a0004, 0x007c0502, 0xdc6a0008, 0x007c0502, 0xdc6a000c, 0x007c0502, 0xbe864100,
	0x7e0a0206, 0xdc6a0000, 0x007c0500, 0x7e0a0207, 0xdc6a0004, 0x007c0500, 0xbe864102, 0x7e0a0206,
	0xdc6a0008, 0x007c0500, 0x7e0a0207, 0xdc6a000c, 0x007c0500, 0xbe864104, 0x7e0a0206, 0xdc6a0010,
	0x007c0500, 0x7e0a0207, 0xdc6a0014, 0x007c0500, 0xbe864106, 0x7e0a0206, 0xdc6a0018, 0x007c0500,
	0x7e0a0207, 0xdc6a001c, 0x007c0500, 0xbe864108, 0x7e0a0206, 0xdc6a0020, 0x007c0500, 0x7e0a0207,
	0xdc6a0024, 0x007c0500, 0xbe86410a, 0x7e0a0206, 0xdc6a0028, 0x007c0500, 0x7e0a0207, 0xdc6a002c,
	0x007c0500, 0xbe86410c, 0x7e0a0206, 0xdc6a0030, 0x007c0500, 0x7e0a0207, 0xdc6a0034, 0x007c0500,
	0xbe86410e, 0x7e0a0206, 0xdc6a0038, 0x007c0500, 0x7e0a0207, 0xdc6a003c, 0x007c0500, 0x80089008,
	0xbf06ff08, 0x00000070, 0xbfa1ffb3, 0x7e0a02ff, 0x04040404, 0xdc6a0010, 0x007c0502, 0xdc6a0014,
	0x007c0502, 0xdc6a0018, 0x007c0502, 0xdc6a001c, 0x007c0502, 0xbefe010a, 0xbfb00000, 0xbf800000,
	0xbf800000, 0xbf800000, 0xbf800000, 0xbf800000,
};

static const kernel_descriptor_t gfx_v11_dump_vgpr_kernelobj = {
	.group_segment_fixed_size = 0x0,
	.private_segment_fixed_size = 0x0,
	.kernarg_size = 0x0,
	.reserved1 = 0x0,
	.kernel_code_entry_byte_offset = 0x0,
	.reserved2 = {0x0, 0x0, 0x0, 0x0, 0x0},
	.compute_pgm_rsrc3 = 0x0,
	.compute_pgm_rsrc1 = 0x40ac000b,
	.compute_pgm_rsrc2 = 0x84,
	.enable_sgpr_private_segment_buffer = 0,
	.enable_sgpr_dispatch_ptr = 0,
	.enable_sgpr_queue_ptr = 0,
	.enable_sgpr_kernarg_segment_ptr = 1,
	.enable_sgpr_dispatch_id = 0,
	.enable_sgpr_flat_scratch_init = 0,
	.enable_sgpr_private_segment_size = 0,
	.reserved3 = 0,
	.enable_wavefront_size32 = 1,
	.uses_dynamic_stack = 0,
	.reserved4 = 0,
	.kernarg_preload_spec_length = 0,
	.kernarg_preload_spec_offset = 0,
	.reserved5 = 0x0
};

static const uint32_t gfx_v11_dump_vgpr_shader[] = {
	0xbfbd0000, 0xf4040280, 0xf8000000, 0xf4040300, 0xf8000008, 0xbf89fc07, 0xb880f817, 0xb881f805,
	0x9303ff00, 0x00030012, 0xbe8500ff, 0x00600000, 0x96050503, 0x9303ff00, 0x00010010, 0xbe8600ff,
	0x00300000, 0x96060603, 0x9303ff00, 0x0004000a, 0x84038103, 0x9304ff00, 0x00010008, 0x8c030403,
	0xbe8700ff, 0x00018000, 0x96070703, 0x9303ff00, 0x00010009, 0xbe8800ff, 0x0000c000, 0x96080803,
	0x9303ff01, 0x00090000, 0x84098203, 0x84098509, 0x80030809, 0x80030703, 0x80030603, 0x80030503,
	0x8004030c, 0x8205800d, 0x84038203, 0x8002030a, 0x8203800b, 0x30000084, 0xdb7c0000, 0x00000400,
	0x32000084, 0xd51b0007, 0x00013f00, 0x7e080204, 0x7e0a0205, 0xd7006a04, 0x00020907, 0x400a0a80,
	0x7e0c0282, 0xdc600000, 0x007c0604, 0x7e0c0281, 0xd7006a04, 0x000208a0, 0x400a0a80, 0xdc600000,
	0x007c0604, 0xd7006a04, 0x000208a0, 0x400a0a80, 0xdc600000, 0x007c0604, 0xd7006a04, 0x000208a0,
	0x400a0a80, 0xdc600000, 0x007c0604, 0xd7006a04, 0x000208a0, 0x400a0a80, 0xbe8100ff, 0x0000005c,
	0x7e0c02ff, 0x01010101, 0xdc600000, 0x007c0604, 0x80818101, 0xd7006a04, 0x000208a0, 0x400a0a80,
	0xbf068001, 0xbfa1fff6, 0x7e080202, 0x7e0a0203, 0x300e0e82, 0xd7006a04, 0x00020907, 0x400a0a80,
	0xdc680000, 0x007c0004, 0xd7006a04, 0x000208ff, 0x00000080, 0x400a0a80, 0xdc680000, 0x007c0104,
	0xd7006a04, 0x000208ff, 0x00000080, 0x400a0a80, 0xdc680000, 0x007c0204, 0xd7006a04, 0x000208ff,
	0x00000080, 0x400a0a80, 0xdc680000, 0x007c0304, 0xd7006a04, 0x000208ff, 0x00000080, 0x400a0a80,
	0xbf89fc07, 0x30000082, 0xd8d80000, 0x06000000, 0xbf89fc07, 0xdc680000, 0x007c0604, 0xd8d80004,
	0x06000000, 0xd7006a04, 0x000208ff, 0x00000080, 0x400a0a80, 0xbf89fc07, 0xdc680000, 0x007c0604,
	0xd8d80008, 0x06000000, 0xd7006a04, 0x000208ff, 0x00000080, 0x400a0a80, 0xbf89fc07, 0xdc680000,
	0x007c0604, 0xd8d8000c, 0x06000000, 0xd7006a04, 0x000208ff, 0x00000080, 0x400a0a80, 0xbf89fc07,
	0xdc680000, 0x007c0604, 0xd7006a04, 0x000208ff, 0x00000080, 0x400a0a80, 0xbefd0088, 0x7e0c8700,
	0xdc680000, 0x007c0604, 0xd7006a04, 0x000208ff, 0x00000080, 0x400a0a80, 0x7e0c8701, 0xdc680000,
	0x007c0604, 0xd7006a04, 0x000208ff, 0x00000080, 0x400a0a80, 0x7e0c8702, 0xdc680000, 0x007c0604,
	0xd7006a04, 0x000208ff, 0x00000080, 0x400a0a80, 0x7e0c8703, 0xdc680000, 0x007c0604, 0xd7006a04,
	0x000208ff, 0x00000080, 0x400a0a80, 0x807d847d, 0xbf06ff7d, 0x00000060, 0xbfa1ffe0, 0xbfb00000,
	0xbfbd0000, 0xf4000080, 0xf8000000, 0xbf89fc07, 0x7e080202, 0x7e0a0202, 0x7e0c0202, 0x7e0e0202,
	0x30020084, 0xbe8000ff, 0x00004000, 0xbe810080, 0xdb7c0000, 0x00000401, 0xd5250001, 0x00000101,
	0x80018101, 0xbf078401, 0xbfa2fff9, 0xbfb00000, 0xbf800000, 0xbf800000, 0xbf800000, 0xbf800000,
	0xbf800000,
};

static unsigned int order_base_2(unsigned int size_of_dwords)
{
	unsigned int i, size_of_log2 = 0;

	for (i = 0; i < 32; i++) {
		if (size_of_dwords == (1U << i)) {
			size_of_log2 = i;
			break;
		}
	}

	return size_of_log2;
}

static void gfx_v11_kiq_set_resources(struct amdgv_ring *kiq_ring,
		uint64_t queue_mask)
{
	amdgv_ring_write(kiq_ring, PACKET3(PACKET3_SET_RESOURCES, 6));
	amdgv_ring_write(kiq_ring, PACKET3_SET_RESOURCES_VMID_MASK(0) |
			PACKET3_SET_RESOURCES_UNMAP_LATENTY(0xa) | /* unmap_latency: 0xa (~ 1s) */
			PACKET3_SET_RESOURCES_QUEUE_TYPE(0));	/* vmid_mask:0 queue_type:0 (KIQ) */
	amdgv_ring_write(kiq_ring, lower_32_bits(queue_mask));	/* queue mask lo */
	amdgv_ring_write(kiq_ring, upper_32_bits(queue_mask));	/* queue mask hi */
	amdgv_ring_write(kiq_ring, 0);	/* gws mask lo */
	amdgv_ring_write(kiq_ring, 0);	/* gws mask hi */
	amdgv_ring_write(kiq_ring, 0);	/* oac mask */
	amdgv_ring_write(kiq_ring, 0);	/* gds heap base:0, gds heap size:0 */
}

static void gfx_v11_kiq_map_queues(struct amdgv_ring *kiq_ring,
		struct amdgv_ring *ring)
{
	uint64_t mqd_addr = ring->mqd_gpu_addr;
	uint64_t wptr_addr = ring->wptr_gpu_addr;
	uint32_t me = 0;
	uint32_t eng_sel = ring->funcs->type == AMDGV_RING_TYPE_GFX ? 4 : 0;

	amdgv_ring_write(kiq_ring, PACKET3(PACKET3_MAP_QUEUES, 5));
	/* Q_sel:0, vmid:0, vidmem: 1, engine:0, num_Q:1*/
	amdgv_ring_write(kiq_ring, /* Q_sel: 0, vmid: 0, engine: 0, num_Q: 1 */
			PACKET3_MAP_QUEUES_QUEUE_SEL(0) | /* Queue_Sel */
			PACKET3_MAP_QUEUES_VMID(0) | /* VMID */
			PACKET3_MAP_QUEUES_QUEUE(ring->queue) |
			PACKET3_MAP_QUEUES_PIPE(ring->pipe) |
			PACKET3_MAP_QUEUES_ME((me)) |
			PACKET3_MAP_QUEUES_QUEUE_TYPE(0) | /*queue_type: normal compute queue */
			PACKET3_MAP_QUEUES_ALLOC_FORMAT(0) | /* alloc format: all_on_one_pipe */
			PACKET3_MAP_QUEUES_ENGINE_SEL(eng_sel) |
			PACKET3_MAP_QUEUES_NUM_QUEUES(1)); /* num_queues: must be 1 */
	amdgv_ring_write(kiq_ring, PACKET3_MAP_QUEUES_DOORBELL_OFFSET(ring->doorbell_index));
	amdgv_ring_write(kiq_ring, lower_32_bits(mqd_addr));
	amdgv_ring_write(kiq_ring, upper_32_bits(mqd_addr));
	amdgv_ring_write(kiq_ring, lower_32_bits(wptr_addr));
	amdgv_ring_write(kiq_ring, upper_32_bits(wptr_addr));
}

static void gfx_v11_kiq_unmap_queues(struct amdgv_ring *kiq_ring,
		struct amdgv_ring *ring,
		enum amdgv_unmap_queues_action action,
		uint64_t gpu_addr, uint64_t seq)
{
	uint32_t eng_sel = ring->funcs->type == AMDGV_RING_TYPE_GFX ? 4 : 0;

	amdgv_ring_write(kiq_ring, PACKET3(PACKET3_UNMAP_QUEUES, 4));
	amdgv_ring_write(kiq_ring, /* Q_sel: 0, vmid: 0, engine: 0, num_Q: 1 */
			PACKET3_UNMAP_QUEUES_ACTION(action) |
			PACKET3_UNMAP_QUEUES_QUEUE_SEL(0) |
			PACKET3_UNMAP_QUEUES_ENGINE_SEL(eng_sel) |
			PACKET3_UNMAP_QUEUES_NUM_QUEUES(1));
	amdgv_ring_write(kiq_ring,
			PACKET3_UNMAP_QUEUES_DOORBELL_OFFSET0(ring->doorbell_index));

	if (action == PREEMPT_QUEUES_NO_UNMAP) {
		amdgv_ring_write(kiq_ring, lower_32_bits(gpu_addr));
		amdgv_ring_write(kiq_ring, upper_32_bits(gpu_addr));
		amdgv_ring_write(kiq_ring, seq);
	} else {
		amdgv_ring_write(kiq_ring, 0);
		amdgv_ring_write(kiq_ring, 0);
		amdgv_ring_write(kiq_ring, 0);
	}
}

static const struct kiq_pm4_funcs gfx_v11_kiq_pm4_funcs = {
	.kiq_set_resources = gfx_v11_kiq_set_resources,
	.kiq_map_queues = gfx_v11_kiq_map_queues,
	.kiq_unmap_queues = gfx_v11_kiq_unmap_queues,
	.set_resources_size = 8,
	.map_queues_size = 7,
	.unmap_queues_size = 6,
	.query_status_size = 7,
	.invalidate_tlbs_size = 2,
};

static void gfx_v11_set_kiq_pm4_funcs(struct amdgv_adapter *adapt)
{
	adapt->gfx.kiq[0].pmf = &gfx_v11_kiq_pm4_funcs;
}

#define DEFAULT_SH_MEM_BASES	(0x6000)
#define LDS_APP_BASE           0x1
#define SCRATCH_APP_BASE       0x2
static void gfx_v11_init_compute_vmid(struct amdgv_adapter *adapt)
{
	int i;
	uint32_t sh_mem_bases, sh_mem_config;
	uint32_t data;
	/*
	 * Configure apertures:
	 * LDS:         0x60000000'00000000 - 0x60000001'00000000 (4GB)
	 * Scratch:     0x60000001'00000000 - 0x60000002'00000000 (4GB)
	 * GPUVM:       0x60010000'00000000 - 0x60020000'00000000 (1TB)
	 */
	sh_mem_bases = (LDS_APP_BASE << SH_MEM_BASES__SHARED_BASE__SHIFT) |
		SCRATCH_APP_BASE;
	sh_mem_config = SH_MEM_ADDRESS_MODE_64 |
		SH_MEM_ALIGNMENT_MODE_UNALIGNED <<
		SH_MEM_CONFIG__ALIGNMENT_MODE__SHIFT;

	oss_mutex_lock(adapt->srbm_mutex);
	for (i = FIRST_KFD_VMID; i < AMDGV_NUM_VMID; i++) {
		navi32_grbm_select(adapt, 0, 0, 0, i);
		/* CP and shaders */
		WREG32_SOC15(GC, 0, regSH_MEM_CONFIG, sh_mem_config);
		WREG32_SOC15(GC, 0, regSH_MEM_BASES, sh_mem_bases);

		/* Enable trap for each kfd vmid. */
		data = RREG32_SOC15(GC, 0, regSPI_GDBG_PER_VMID_CNTL);
		data = REG_SET_FIELD(data, SPI_GDBG_PER_VMID_CNTL, TRAP_EN, 1);
		WREG32_SOC15(GC, 0, regSPI_GDBG_PER_VMID_CNTL, data);
	}
	navi32_grbm_select(adapt, 0, 0, 0, 0);
	oss_mutex_unlock(adapt->srbm_mutex);

	/* Initialize all compute VMIDs to have no GDS, GWS, or OA
	   acccess. These should be enabled by FW for target VMIDs. */
	for (i = FIRST_KFD_VMID; i < AMDGV_NUM_VMID; i++) {
		WREG32_SOC15_OFFSET(GC, 0, regGDS_VMID0_BASE, 2 * i, 0);
		WREG32_SOC15_OFFSET(GC, 0, regGDS_VMID0_SIZE, 2 * i, 0);
		WREG32_SOC15_OFFSET(GC, 0, regGDS_GWS_VMID0, i, 0);
		WREG32_SOC15_OFFSET(GC, 0, regGDS_OA_VMID0, i, 0);
	}
}

static void gfx_v11_init_gds_vmid(struct amdgv_adapter *adapt)
{
	int vmid;
	/*
	 * Initialize all compute and user-gfx VMIDs to have no GDS, GWS, or OA
	 * access. Compute VMIDs should be enabled by FW for target VMIDs,
	 * the driver can enable them for graphics. VMID0 should maintain
	 * access so that HWS firmware can save/restore entries.
	 */
	for (vmid = 1; vmid < 16; vmid++) {
		WREG32_SOC15_OFFSET(GC, 0, regGDS_VMID0_BASE, 2 * vmid, 0);
		WREG32_SOC15_OFFSET(GC, 0, regGDS_VMID0_SIZE, 2 * vmid, 0);
		WREG32_SOC15_OFFSET(GC, 0, regGDS_GWS_VMID0, vmid, 0);
		WREG32_SOC15_OFFSET(GC, 0, regGDS_OA_VMID0, vmid, 0);
	}
}

#define DEFAULT_SH_MEM_CONFIG \
	((SH_MEM_ADDRESS_MODE_64 << SH_MEM_CONFIG__ADDRESS_MODE__SHIFT) | \
	 (SH_MEM_ALIGNMENT_MODE_UNALIGNED << SH_MEM_CONFIG__ALIGNMENT_MODE__SHIFT) | \
	 (3 << SH_MEM_CONFIG__INITIAL_INST_PREFETCH__SHIFT))

static void gfx_v11_constants_init(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	int i;

	tmp = RREG32_SOC15_RLC(GC, 0, regGRBM_CNTL);
	tmp = REG_SET_FIELD(tmp, GRBM_CNTL, READ_TIMEOUT, 0xff);
	WREG32_SOC15_RLC(GC, 0, regGRBM_CNTL, tmp);

	/* XXX SH_MEM regs */
	/* where to put LDS, scratch, GPUVM in FSA64 space */
	oss_mutex_lock(adapt->srbm_mutex);
	for (i = 0; i < NUM_IDS; i++) {
		navi32_grbm_select(adapt, 0, 0, 0, i);
		/* CP and shaders */
		WREG32_SOC15(GC, 0, regSH_MEM_CONFIG, DEFAULT_SH_MEM_CONFIG);
		if (i != 0) {
			tmp = REG_SET_FIELD(0, SH_MEM_BASES, PRIVATE_BASE,
					(0x1000000000000000ULL >> 48));
			tmp = REG_SET_FIELD(tmp, SH_MEM_BASES, SHARED_BASE,
					(0x2000000000000000ULL >> 48));
			WREG32_SOC15(GC, 0, regSH_MEM_BASES, tmp);
		}
	}
	navi32_grbm_select(adapt, 0, 0, 0, 0);

	oss_mutex_unlock(adapt->srbm_mutex);

	gfx_v11_init_compute_vmid(adapt);
	gfx_v11_init_gds_vmid(adapt);
}

static int gfx_v11_gpu_early_init(struct amdgv_adapter *adapt)
{
	adapt->gfx.config.max_hw_contexts = 8;
	adapt->gfx.config.sc_prim_fifo_size_frontend = 0x20;
	adapt->gfx.config.sc_prim_fifo_size_backend = 0x100;
	adapt->gfx.config.sc_hiz_tile_fifo_size = 0;
	adapt->gfx.config.sc_earlyz_tile_fifo_size = 0x4C0;

	// CU setting for data dump
	adapt->gfx.cu_dump_data_info.cu_info.num_se = 8;
	adapt->gfx.cu_dump_data_info.cu_info.num_sa_per_se = 2;
	adapt->gfx.cu_dump_data_info.cu_info.num_cus_per_sa = 32;
	adapt->gfx.cu_dump_data_info.cu_info.num_lds_dwords_per_cu = 16384;
	adapt->gfx.cu_dump_data_info.cu_info.simd_per_cu = 2;
	adapt->gfx.cu_dump_data_info.cu_info.max_waves_per_simd = 16;
	adapt->gfx.cu_dump_data_info.cu_info.num_sgprs_per_simd = 0;
	adapt->gfx.cu_dump_data_info.cu_info.num_sgprs_per_wave_slot = 128;
	adapt->gfx.cu_dump_data_info.cu_info.num_vgprs_per_simd = 1536;
	adapt->gfx.cu_dump_data_info.cu_info.num_lanes_per_vgpr = 32;
	return 0;
}

static bool is_gfx_v11_paging_compute_queue(struct amdgv_adapter *adapt, uint32_t me, uint32_t pipe, uint32_t queue)
{
	bool result = false;

	if (adapt->gfx.mec.paging_me == me &&
	    adapt->gfx.mec.paging_pipe == pipe  &&
		adapt->gfx.mec.paging_queue == queue) {
		result = true;
	}
	return result;
}

static int gfx_v11_compute_ring_init(struct amdgv_adapter *adapt, int ring_id,
		int mec, int pipe, int queue)
{
	struct amdgv_ring *ring;
	unsigned int hw_prio;
	bool is_paging_queue = false;
	uint32_t frame_dword_size = 1024;
	uint32_t frame_number = 2;

	ring = &adapt->gfx.compute_ring[ring_id];

	/* mec0 is me1 */
	ring->me = mec + 1;
	ring->pipe = pipe;
	ring->queue = queue;
	if (is_gfx_v11_paging_compute_queue(adapt, ring->me, ring->pipe, ring->queue)) {
		is_paging_queue = true;
		adapt->gfx.compute_paging_queue_id = ring_id;
	}
	ring->ring_obj = NULL;
	ring->use_doorbell = true;
	ring->doorbell_index = (adapt->doorbell_index.mec_ring0 + ring_id) << 1;
	ring->eop_gpu_addr = adapt->gfx.mec.hpd_eop_gpu_addr
		+ (ring_id * GFX11_MEC_HPD_SIZE);
	oss_vsnprintf(ring->name, 12, "comp_%d.%d.%d", ring->me,
			ring->pipe, ring->queue);

	hw_prio = amdgv_gfx_is_high_priority_compute_queue(adapt, ring) ?
		AMDGV_RING_PRIO_2 : AMDGV_RING_PRIO_DEFAULT;
	if (is_paging_queue) {
		frame_dword_size = adapt->opt.paging_queue_frame_bytes_size / sizeof(uint32_t);
		frame_number = adapt->opt.paging_queue_frame_number;
	}
	return amdgv_ring_init(adapt, ring, frame_dword_size, frame_number, hw_prio, NULL, MEM_COMPUTE0_RING + ring_id);
}

static void gfx_v11_mec_fini(struct amdgv_adapter *adapt)
{
	amdgv_memmgr_free(adapt->gfx.mec.hpd_eop_obj);
}

static int gfx_v11_mec_init(struct amdgv_adapter *adapt)
{
	uint32_t mec_hpd_size;
	adapt->gfx.mec_queue_bitmap[0] = 0;

	/* take ownership of the relevant compute queues */
	amdgv_gfx_compute_queue_acquire(adapt);
	mec_hpd_size = adapt->gfx.num_compute_rings * GFX11_MEC_HPD_SIZE;

	if (mec_hpd_size) {
		adapt->gfx.mec.hpd_eop_obj =
			amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
					mec_hpd_size, PAGE_SIZE, MEM_GFX_EOP);
		if (!adapt->gfx.mec.hpd_eop_obj) {
			AMDGV_WARN("create HDP EOP bo failed\n");
			gfx_v11_mec_fini(adapt);
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

static int gfx_v11_mec_init_set(struct amdgv_adapter *adapt)
{
	uint32_t i, *hpd;
	uint32_t mec_hpd_size = adapt->gfx.num_compute_rings * GFX11_MEC_HPD_SIZE;

	if (mec_hpd_size) {
		adapt->gfx.mec.hpd_eop_gpu_addr =
			amdgv_memmgr_get_gpu_addr(adapt->gfx.mec.hpd_eop_obj);
		hpd =
			amdgv_memmgr_get_cpu_addr(adapt->gfx.mec.hpd_eop_obj);

		oss_memset(hpd, 0, mec_hpd_size);
		for (i = 0; i < adapt->gfx.num_compute_rings; i++) {
			adapt->gfx.compute_ring[i].eop_gpu_addr =
				adapt->gfx.mec.hpd_eop_gpu_addr + i * GFX11_MEC_HPD_SIZE;
		}
	}

	return 0;
}

static int gfx_v11_early_init(struct amdgv_adapter *adapt);

static int gfx_v11_sw_init_internal(struct amdgv_adapter *adapt)
{
	int r, ring_id;
	struct amdgv_kiq *kiq;
	uint32_t i, j, k;

	if (in_whole_gpu_reset())
		return 0;

	adapt->gfx.mec.num_mec = 1;
	adapt->gfx.mec.num_pipe_per_mec = 4;
	adapt->gfx.mec.num_queue_per_pipe = 4;
	adapt->gfx.mec.mec_hpd_size = GFX11_MEC_HPD_SIZE;

	adapt->gfx.mec.paging_me = 1;
	adapt->gfx.mec.paging_pipe = 1;
	adapt->gfx.mec.paging_queue = 0;

	gfx_v11_early_init(adapt);

	adapt->gfx.gfx_current_status = AMDGV_GFX_NORMAL_MODE;

	r = gfx_v11_mec_init(adapt);
	if (r) {
		AMDGV_ERROR("Failed to init MEC BOs!\n");
		return r;
	}

	/* set up the compute queues - allocate horizontally across pipes */
	ring_id = 0;
	for (i = 0; i < adapt->gfx.mec.num_mec; ++i) {
		for (j = 0; j < adapt->gfx.mec.num_queue_per_pipe; j++) {
			for (k = 0; k < adapt->gfx.mec.num_pipe_per_mec; k++) {
				if (!amdgv_gfx_is_mec_queue_enabled(adapt, 0,
							i, k, j))
					continue;

				r = gfx_v11_compute_ring_init(adapt,
						ring_id, i, k, j);
				if (r)
					return r;

				ring_id++;
			}
		}
	}

	r = amdgv_gfx_kiq_init(adapt, GFX11_MEC_HPD_SIZE, 0);
	if (r) {
		AMDGV_ERROR("Failed to init KIQ BOs!\n");
		return r;
	}

	kiq = &adapt->gfx.kiq[0];

	kiq->ring.me = 3;
	kiq->ring.pipe = 1;
	kiq->ring.queue = 0;
	r = amdgv_gfx_kiq_init_ring(adapt, &kiq->ring, 0);
	if (r)
		return r;

	/* create MQD for all compute queues as wel as KIQ for SRIOV case */
	r = amdgv_gfx_mqd_sw_init(adapt, sizeof(struct v11_compute_mqd), 0);
	if (r)
		return r;

	r = gfx_v11_gpu_early_init(adapt);
	if (r)
		return r;
	amdgv_gfx_check_pf_fb_size_for_cu_data_dump(adapt);

	return 0;
}

static int gfx_v11_hw_init_internal_set(struct amdgv_adapter *adapt)
{
	int r;
	uint32_t i;
	struct amdgv_ring *ring;

	r = gfx_v11_mec_init_set(adapt);
	if (r) {
		AMDGV_ERROR("Failed to init MEC set!\n");
		return r;
	}

	for (i = 0; i < adapt->gfx.num_compute_rings; i++) {
		ring = &adapt->gfx.compute_ring[i];
		ring->aql_enable = (i == adapt->gfx.compute_paging_queue_id) ? false : true;
		r = amdgv_ring_init_set(adapt, ring);
		if (r)
			return r;
	}

	r = amdgv_gfx_kiq_init_set(adapt, GFX11_MEC_HPD_SIZE, 0);
	if (r) {
		AMDGV_ERROR("Failed to init KIQ BOs!\n");
		return r;
	}

	ring = &adapt->gfx.kiq[0].ring;

	r = amdgv_ring_init_set(adapt, ring);
	if (r)
		return r;

	amdgv_gfx_mqd_init_set(adapt, 0);
	return 0;
}

static void gfx_v11_hw_init_clear_ring(struct amdgv_adapter *adapt)
{
	uint32_t i;
	struct amdgv_ring *ring;

	if (in_whole_gpu_reset()) {
		for (i = 0; i < adapt->gfx.num_compute_rings; i++) {
			ring = &adapt->gfx.compute_ring[i];
			// Clear the ring
			amdgv_ring_clear_ring(ring);
			ring->wptr = 0;
		}

		ring = &adapt->gfx.kiq[0].ring;
		// Clear the ring
		amdgv_ring_clear_ring(ring);
		ring->wptr = 0;
	}
}

static int gfx_v11_sw_fini_internal(struct amdgv_adapter *adapt)
{
	uint32_t i;

	for (i = 0; i < adapt->gfx.num_compute_rings; i++)
		amdgv_ring_fini(&adapt->gfx.compute_ring[i]);

	amdgv_gfx_mqd_sw_fini(adapt, 0);
	amdgv_gfx_kiq_free_ring(&adapt->gfx.kiq[0].ring);
	amdgv_gfx_kiq_fini(adapt, 0);

	gfx_v11_mec_fini(adapt);

	return 0;
}

static int gfx_v11_sw_init(struct amdgv_adapter *adapt)
{
	/* hang detection needs to know GFX block scheduler type, which
	 * initializes later than gfx.sw_init, so initialize hang detection in hw_init */
	if (adapt->gfx.funcs &&    /* gfx funcs implemented */
		(adapt->num_vf > 1 || adapt->flags & AMDGV_FLAG_USE_PF) &&  /* not single VF mode */
		(adapt->flags & AMDGV_FLAG_ENABLE_HANG_DETECTION) &&    /* shim requested enable hang detection */
		(adapt->gpuiov.ctrl_blocks[NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV].sched_mode > AMDGV_SCHED_MAX_HW_SCHED_MODE)) {
		adapt->gfx.hang_detection_supported = true;
		adapt->gfx.hang_detection_threshold_us = ((adapt->sched.get_asic_time_slice) ? \
				(adapt->sched.get_asic_time_slice(adapt, AMDGV_SCHED_BLOCK_GFX, adapt->sched.num_vf_per_gfx_sched)) : DEFAULT_GFX_TIME_SLICE);
		adapt->gfx.hang_detection_duration_us = 10000;
		AMDGV_INFO("Hang Detection Enabled\n");
	} else {
		adapt->gfx.hang_detection_supported = false;
		AMDGV_INFO("Hang Detection Disabled\n");
	}

	if (amdgv_gfx_cu_data_dump_thread_init(adapt)) {
		AMDGV_ERROR("CU data dump thread init failed\n");
		amdgv_gfx_cu_data_dump_thread_fini(adapt);
		return AMDGV_FAILURE;
	}

	if (adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE) {
		gfx_v11_early_init(adapt);
		gfx_v11_gpu_early_init(adapt);
		return 0;
	}
	return gfx_v11_sw_init_internal(adapt);
}

static int gfx_v11_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_gfx_cu_data_dump_thread_fini(adapt);

	if (!(adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)) {
		gfx_v11_sw_fini_internal(adapt);
	}
	return 0;
}

static void gfx_v11_enable_interrupt(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t tmp;
	struct amdgv_ring *ring = &adapt->gfx.compute_ring[adapt->gfx.compute_paging_queue_id];

	tmp = RREG32_SOC15(GC, 0, regCP_INT_CNTL_RING0);

	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0, CNTX_BUSY_INT_ENABLE,
			enable ? 1 : 0);
	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0, CNTX_EMPTY_INT_ENABLE,
			enable ? 1 : 0);
	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0, CMP_BUSY_INT_ENABLE,
			enable ? 1 : 0);
	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0, GFX_IDLE_INT_ENABLE,
			enable ? 1 : 0);
	WREG32_SOC15(GC, 0, regCP_INT_CNTL_RING0, tmp);

	navi32_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0);
	tmp = RREG32_SOC15(GC, 0, regCPC_INT_CNTL);
	tmp = REG_SET_FIELD(tmp, CPC_INT_CNTL, TIME_STAMP_INT_ENABLE, enable ? 1 : 0);
	WREG32_SOC15(GC, 0, regCPC_INT_CNTL, tmp);
	navi32_grbm_select(adapt, 0, 0, 0, 0);
}

/* KIQ functions */
static void gfx_v11_kiq_setting(struct amdgv_ring *ring)
{
	uint32_t tmp;
	struct amdgv_adapter *adapt = ring->adapt;

	/* tell RLC which is KIQ queue */
	tmp = RREG32_SOC15(GC, 0, regRLC_CP_SCHEDULERS);
	tmp &= 0xffffff00;
	tmp |= (ring->me << 5) | (ring->pipe << 3) | (ring->queue);
	WREG32_SOC15(GC, 0, regRLC_CP_SCHEDULERS, tmp);
	tmp |= 0x80;
	WREG32_SOC15(GC, 0, regRLC_CP_SCHEDULERS, tmp);
}

static void gfx_v11_0_cp_set_doorbell_range(struct amdgv_adapter *adapt)
{
	/* set graphics engine doorbell range */
	WREG32_SOC15(GC, 0, regCP_RB_DOORBELL_RANGE_LOWER,
			(adapt->doorbell_index.gfx_ring0 * 2) << 2);
	WREG32_SOC15(GC, 0, regCP_RB_DOORBELL_RANGE_UPPER,
			(adapt->doorbell_index.gfx_userqueue_end * 2) << 2);

	/* set compute engine doorbell range */
	WREG32_SOC15(GC, 0, regCP_MEC_DOORBELL_RANGE_LOWER,
			(adapt->doorbell_index.kiq * 2) << 2);
	WREG32_SOC15(GC, 0, regCP_MEC_DOORBELL_RANGE_UPPER,
			(adapt->doorbell_index.userqueue_end * 2) << 2);
}

static void gfx_v11_cp_compute_enable(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t data;
	if (adapt->rs64_enable) {
		data = RREG32_SOC15(GC, 0, regCP_MEC_RS64_CNTL);
		data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_INVALIDATE_ICACHE,
				enable ? 0 : 1);
		data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE0_RESET,
				enable ? 0 : 1);
		data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE1_RESET,
				enable ? 0 : 1);
		data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE2_RESET,
				enable ? 0 : 1);
		data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE3_RESET,
				enable ? 0 : 1);
		data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE0_ACTIVE,
				enable ? 1 : 0);
		data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE1_ACTIVE,
				enable ? 1 : 0);
		data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE2_ACTIVE,
				enable ? 1 : 0);
		data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_PIPE3_ACTIVE,
				enable ? 1 : 0);
		data = REG_SET_FIELD(data, CP_MEC_RS64_CNTL, MEC_HALT,
				enable ? 0 : 1);
		WREG32_SOC15(GC, 0, regCP_MEC_RS64_CNTL, data);
	} else {
		data = RREG32_SOC15(GC, 0, regCP_MEC_CNTL);

		if (enable) {
			data = REG_SET_FIELD(data, CP_MEC_CNTL, MEC_ME1_HALT, 0);
		} else {
			data = REG_SET_FIELD(data, CP_MEC_CNTL, MEC_ME1_HALT, 1);
			data = REG_SET_FIELD(data, CP_MEC_CNTL, MEC_ME2_HALT, 1);
		}
		WREG32_SOC15(GC, 0, regCP_MEC_CNTL, data);
	}

	oss_udelay(50);
}

static void gfx_v11_mqd_set_priority(struct amdgv_ring *ring,
		struct v11_compute_mqd *mqd)
{
	struct amdgv_adapter *adapt = ring->adapt;
	if (ring->funcs->type == AMDGV_RING_TYPE_COMPUTE) {
		if (amdgv_gfx_is_high_priority_compute_queue(adapt, ring)) {
			mqd->cp_hqd_pipe_priority = AMDGV_GFX_PIPE_PRIO_HIGH;
			mqd->cp_hqd_queue_priority = AMDGV_GFX_QUEUE_PRIORITY_MAXIMUM;
		}
	}
}

static void gfx_v11_ring_submit_frame(struct amdgv_ring *ring, uint8_t *frame_data)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint64_t dword_wptr = ring->wptr % ring->ring_size;
	uint32_t *data32 = (uint32_t *)frame_data;
	uint32_t i;

	for (i = 0; i < ring->max_dw; i++) {
		ring->ring[dword_wptr++] = data32[i];
	}
	ring->wptr += ring->max_dw;

	// Both ring->wptr and CP WPTR are DWORD index
	dword_wptr = ring->wptr;
	*((volatile uint64_t *)(ring->wptr_cpu_addr)) = dword_wptr;

	if (ring->use_doorbell) {
		WDOORBELL64(ring->doorbell_index, dword_wptr);
	} else {
		oss_mutex_lock(adapt->srbm_mutex);
		navi32_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0);
		WREG32_SOC15(GC, 0, regCP_HQD_PQ_WPTR_LO, (uint32_t)(dword_wptr));
		WREG32_SOC15(GC, 0, regCP_HQD_PQ_WPTR_HI, (uint32_t)(dword_wptr >> 32));
		navi32_grbm_select(adapt, 0, 0, 0, 0);
		oss_mutex_unlock(adapt->srbm_mutex);
	}
}

static int gfx_v11_mqd_init(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v11_compute_mqd *mqd = (struct v11_compute_mqd *)ring->mqd_ptr;
	uint64_t hqd_gpu_addr, wb_gpu_addr, eop_base_addr;
	uint32_t tmp;

	mqd->header = 0xC0310800;
	mqd->compute_pipelinestat_enable = 0x00000001;
	mqd->compute_static_thread_mgmt_se0 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se1 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se2 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se3 = 0xffffffff;
	mqd->compute_misc_reserved = 0x00000007;
	if (ring->aql_enable)
		mqd->cp_hqd_aql_control = 1 << CP_HQD_AQL_CONTROL__CONTROL0__SHIFT;

	eop_base_addr = ring->eop_gpu_addr >> 8;
	mqd->cp_hqd_eop_base_addr_lo = eop_base_addr;
	mqd->cp_hqd_eop_base_addr_hi = upper_32_bits(eop_base_addr);

	/* set the EOP size, register value is 2^(EOP_SIZE+1) dwords */
	tmp = RREG32_SOC15(GC, 0, regCP_HQD_EOP_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_HQD_EOP_CONTROL, EOP_SIZE,
			(order_base_2(GFX11_MEC_HPD_SIZE / 4) - 1));

	mqd->cp_hqd_eop_control = tmp;

	/* disable the queue if it's active */
	mqd->cp_hqd_dequeue_request = 0;
	mqd->cp_hqd_pq_rptr = 0;
	mqd->cp_hqd_pq_wptr_lo = 0;
	mqd->cp_hqd_pq_wptr_hi = 0;

	/* set the pointer to the MQD */
	mqd->cp_mqd_base_addr_lo = ring->mqd_gpu_addr & 0xfffffffc;
	mqd->cp_mqd_base_addr_hi = upper_32_bits(ring->mqd_gpu_addr);

	/* set MQD vmid to 0 */
	tmp = RREG32_SOC15(GC, 0, regCP_MQD_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_MQD_CONTROL, VMID, 0);
	mqd->cp_mqd_control = tmp;

	/* set the pointer to the HQD, this is similar CP_RB0_BASE/_HI */
	hqd_gpu_addr = ring->gpu_addr >> 8;
	mqd->cp_hqd_pq_base_lo = hqd_gpu_addr;
	mqd->cp_hqd_pq_base_hi = upper_32_bits(hqd_gpu_addr);

	/* set up the HQD, this is similar to CP_RB0_CNTL */
	tmp = RREG32_SOC15(GC, 0, regCP_HQD_PQ_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, QUEUE_SIZE,
			ring->log2_ring_size - 1);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, RPTR_BLOCK_SIZE,
			(order_base_2(AMDGV_GPU_PAGE_SIZE / 4) - 1));
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, UNORD_DISPATCH, 0);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, TUNNEL_DISPATCH, 0);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, PRIV_STATE, 1);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, KMD_QUEUE, 1);
	mqd->cp_hqd_pq_control = tmp;

	/* set the wb address whether it's enabled or not */
	wb_gpu_addr = ring->rptr_gpu_addr;
	mqd->cp_hqd_pq_rptr_report_addr_lo = wb_gpu_addr & 0xfffffffc;
	mqd->cp_hqd_pq_rptr_report_addr_hi =
		upper_32_bits(wb_gpu_addr) & 0xffff;

	/* only used if CP_PQ_WPTR_POLL_CNTL.CP_PQ_WPTR_POLL_CNTL__EN_MASK=1 */
	wb_gpu_addr = ring->wptr_gpu_addr;
	mqd->cp_hqd_pq_wptr_poll_addr_lo = wb_gpu_addr & 0xfffffffc;
	mqd->cp_hqd_pq_wptr_poll_addr_hi = upper_32_bits(wb_gpu_addr) & 0xffff;

	tmp = 0;
	/* enable the doorbell if requested */
	if (ring->use_doorbell) {
		tmp = RREG32_SOC15(GC, 0, regCP_HQD_PQ_DOORBELL_CONTROL);
		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
				DOORBELL_OFFSET, ring->doorbell_index);

		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
				DOORBELL_EN, 1);
		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
				DOORBELL_SOURCE, 0);
		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
				DOORBELL_HIT, 0);
	}

	mqd->cp_hqd_pq_doorbell_control = tmp;

	/* reset read and write pointers, similar to CP_RB0_WPTR/_RPTR */
	mqd->cp_hqd_pq_rptr = RREG32_SOC15(GC, 0, regCP_HQD_PQ_RPTR);

	/* set the vmid for the queue */
	mqd->cp_hqd_vmid = 0;

	tmp = RREG32_SOC15(GC, 0, regCP_HQD_PERSISTENT_STATE);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PERSISTENT_STATE, PRELOAD_SIZE, 0x55);
	mqd->cp_hqd_persistent_state = tmp;

	/* set MIN_IB_AVAIL_SIZE */
	tmp = RREG32_SOC15(GC, 0, regCP_HQD_IB_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_HQD_IB_CONTROL, MIN_IB_AVAIL_SIZE, 3);
	mqd->cp_hqd_ib_control = tmp;

	/* set static priority for a compute queue/ring */
	gfx_v11_mqd_set_priority(ring, mqd);

	if (ring->funcs->type == AMDGV_RING_TYPE_KIQ)
		mqd->cp_hqd_active = 1;

	return 0;
}

static int gfx_v11_kiq_init_register(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v11_compute_mqd *mqd = (struct v11_compute_mqd *)ring->mqd_ptr;
	int j;
	uint32_t tmp;

	/* disable wptr polling */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_PQ_WPTR_POLL_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_PQ_WPTR_POLL_CNTL, EN, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PQ_WPTR_POLL_CNTL), tmp);

	/* write the EOP addr */
	WREG32_SOC15(GC, 0, regCP_HQD_EOP_BASE_ADDR,
			mqd->cp_hqd_eop_base_addr_lo);
	WREG32_SOC15(GC, 0, regCP_HQD_EOP_BASE_ADDR_HI,
			mqd->cp_hqd_eop_base_addr_hi);

	/* set the EOP size, register value is 2^(EOP_SIZE+1) dwords */
	WREG32_SOC15(GC, 0, regCP_HQD_EOP_CONTROL,
			mqd->cp_hqd_eop_control);

	/* enable doorbell? */
	WREG32_SOC15(GC, 0, regCP_HQD_PQ_DOORBELL_CONTROL,
			mqd->cp_hqd_pq_doorbell_control);

	/* disable the queue if it's active */
	if (RREG32_SOC15(GC, 0, regCP_HQD_ACTIVE) & 1) {
		WREG32_SOC15(GC, 0, regCP_HQD_DEQUEUE_REQUEST, 1);
		for (j = 0; j < AMDGV_GFX_MAX_USEC_TIMEOUT; j++) {
			if (!(RREG32_SOC15(GC, 0, regCP_HQD_ACTIVE) & 1))
				break;
			oss_udelay(1);
		}
		WREG32_SOC15(GC, 0, regCP_HQD_DEQUEUE_REQUEST,
				mqd->cp_hqd_dequeue_request);
		WREG32_SOC15(GC, 0, regCP_HQD_PQ_RPTR,
				mqd->cp_hqd_pq_rptr);
		WREG32_SOC15(GC, 0, regCP_HQD_PQ_WPTR_LO,
				mqd->cp_hqd_pq_wptr_lo);
		WREG32_SOC15(GC, 0, regCP_HQD_PQ_WPTR_HI,
				mqd->cp_hqd_pq_wptr_hi);
	}

	/* set the pointer to the MQD */
	WREG32_SOC15(GC, 0, regCP_MQD_BASE_ADDR,
			mqd->cp_mqd_base_addr_lo);
	WREG32_SOC15(GC, 0, regCP_MQD_BASE_ADDR_HI,
			mqd->cp_mqd_base_addr_hi);

	/* set MQD vmid to 0 */
	WREG32_SOC15(GC, 0, regCP_MQD_CONTROL,
			mqd->cp_mqd_control);

	/* set the pointer to the HQD, this is similar CP_RB0_BASE/_HI */
	WREG32_SOC15(GC, 0, regCP_HQD_PQ_BASE,
			mqd->cp_hqd_pq_base_lo);
	WREG32_SOC15(GC, 0, regCP_HQD_PQ_BASE_HI,
			mqd->cp_hqd_pq_base_hi);

	/* set up the HQD, this is similar to CP_RB0_CNTL */
	WREG32_SOC15(GC, 0, regCP_HQD_PQ_CONTROL,
			mqd->cp_hqd_pq_control);

	/* set the wb address whether it's enabled or not */
	WREG32_SOC15(GC, 0, regCP_HQD_PQ_RPTR_REPORT_ADDR,
			mqd->cp_hqd_pq_rptr_report_addr_lo);
	WREG32_SOC15(GC, 0, regCP_HQD_PQ_RPTR_REPORT_ADDR_HI,
			mqd->cp_hqd_pq_rptr_report_addr_hi);

	/* only used if CP_PQ_WPTR_POLL_CNTL.CP_PQ_WPTR_POLL_CNTL__EN_MASK=1 */
	WREG32_SOC15(GC, 0, regCP_HQD_PQ_WPTR_POLL_ADDR,
			mqd->cp_hqd_pq_wptr_poll_addr_lo);
	WREG32_SOC15(GC, 0, regCP_HQD_PQ_WPTR_POLL_ADDR_HI,
			mqd->cp_hqd_pq_wptr_poll_addr_hi);

	/* enable the doorbell if requested */
	if (ring->use_doorbell) {
		WREG32_SOC15(GC, 0, regCP_MEC_DOORBELL_RANGE_LOWER,
				(adapt->doorbell_index.kiq * 2) << 2);
		WREG32_SOC15(GC, 0, regCP_MEC_DOORBELL_RANGE_UPPER,
				(adapt->doorbell_index.userqueue_end * 2) << 2);
	}

	WREG32_SOC15(GC, 0, regCP_HQD_PQ_DOORBELL_CONTROL,
			mqd->cp_hqd_pq_doorbell_control);

	/* reset read and write pointers, similar to CP_RB0_WPTR/_RPTR */
	WREG32_SOC15(GC, 0, regCP_HQD_PQ_WPTR_LO,
			mqd->cp_hqd_pq_wptr_lo);
	WREG32_SOC15(GC, 0, regCP_HQD_PQ_WPTR_HI,
			mqd->cp_hqd_pq_wptr_hi);

	/* set the vmid for the queue */
	WREG32_SOC15(GC, 0, regCP_HQD_VMID, mqd->cp_hqd_vmid);

	WREG32_SOC15(GC, 0, regCP_HQD_PERSISTENT_STATE,
			mqd->cp_hqd_persistent_state);

	/* activate the queue */
	WREG32_SOC15(GC, 0, regCP_HQD_ACTIVE,
			mqd->cp_hqd_active);

	if (ring->use_doorbell) {
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_PQ_STATUS));
		tmp = REG_SET_FIELD(tmp, CP_PQ_STATUS, DOORBELL_ENABLE, 1);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PQ_STATUS), tmp);
	}

	return 0;
}

static int gfx_v11_kiq_init_queue(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v11_compute_mqd *init_mqd;
	struct v11_compute_mqd *mqd =
		(struct  v11_compute_mqd *)amdgv_memmgr_get_cpu_addr(ring->mqd_obj);
	init_mqd = (struct v11_compute_mqd *)adapt->gfx.kiq[ring->xcc_id].mqd_backup;

	oss_mutex_lock(adapt->srbm_mutex);
	navi32_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0);
	if (init_mqd && init_mqd->cp_hqd_pq_control) {
		oss_memcpy(mqd, init_mqd, sizeof(struct v11_compute_mqd));
	} else {
		gfx_v11_mqd_init(ring);
		if (init_mqd) {
			// save the intial MQD setup
			oss_memcpy(init_mqd, mqd, sizeof(struct v11_compute_mqd));
		}
	}
	gfx_v11_kiq_init_register(ring);
	navi32_grbm_select(adapt, 0, 0, 0, 0);
	oss_mutex_unlock(adapt->srbm_mutex);

	// set ring buffer WRITE pointer to the same value from MQD
	ring->wptr = ((uint64_t)mqd->cp_hqd_pq_wptr_hi << 32) + mqd->cp_hqd_pq_wptr_lo;
	*((volatile uint64_t *)(ring->wptr_cpu_addr)) = ring->wptr;

	return 0;
}

static int gfx_v11_kcq_init_queue(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	int mqd_idx = ring - &adapt->gfx.compute_ring[0];
	struct v11_compute_mqd *init_mqd;
	struct v11_compute_mqd *mqd =
		(struct  v11_compute_mqd *)amdgv_memmgr_get_cpu_addr(ring->mqd_obj);
	init_mqd = (struct v11_compute_mqd *)adapt->gfx.mec.mqd_backup[mqd_idx];

	if (init_mqd && init_mqd->cp_hqd_pq_control) {
		oss_memcpy(mqd, init_mqd, sizeof(struct v11_compute_mqd));
	} else {
		oss_mutex_lock(adapt->srbm_mutex);
		navi32_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0);
		gfx_v11_mqd_init(ring);
		navi32_grbm_select(adapt, 0, 0, 0, 0);
		oss_mutex_unlock(adapt->srbm_mutex);
		if (init_mqd) {
			// save the intial MQD setup
			oss_memcpy(init_mqd, mqd, sizeof(struct v11_compute_mqd));
		}
	}

	// set ring buffer WRITE pointer to the same value from MQD
	ring->wptr = ((uint64_t)mqd->cp_hqd_pq_wptr_hi << 32) + mqd->cp_hqd_pq_wptr_lo;
	*((volatile uint64_t *)(ring->wptr_cpu_addr)) = ring->wptr;

	return 0;
}

static int gfx_v11_kiq_resume(struct amdgv_adapter *adapt)
{
	struct amdgv_ring *ring;

	ring = &adapt->gfx.kiq[0].ring;
	gfx_v11_kiq_setting(ring);
	gfx_v11_kiq_init_queue(ring);

	return amdgv_gfx_kiq_set_resources(adapt, 0);
}

static int gfx_v11_kcq_resume(struct amdgv_adapter *adapt)
{
	struct amdgv_ring *ring = NULL;
	int r = 0;
	uint32_t i;

	gfx_v11_cp_compute_enable(adapt, true);

	for (i = 0; i < adapt->gfx.num_compute_rings && !r; i++) {
		ring = &adapt->gfx.compute_ring[i];
		r = gfx_v11_kcq_init_queue(ring);
	}

	if (!r && (adapt->flags & AMDGV_FLAG_ENABLE_COMPUTE_PAGING))
		r = amdgv_gfx_map_kcq(adapt, 0, XCC_QUEUE_INDEX__PAGING);

	return r;
}

static void gfx_v11_mes_enable(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t data = 0;

	if (enable) {
		data = RREG32_SOC15(GC, 0, regCP_MES_CNTL);
		data = REG_SET_FIELD(0, CP_MES_CNTL, MES_PIPE0_ACTIVE, 1);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE1_ACTIVE, 1);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_CNTL), data);

		oss_udelay(50);
	} else {
		data = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_CNTL));
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE0_ACTIVE, 0);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE1_ACTIVE, 0);
		data = REG_SET_FIELD(data, CP_MES_CNTL,
				MES_INVALIDATE_ICACHE, 1);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE0_RESET, 1);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE1_RESET, 1);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_HALT, 1);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_CNTL), data);
	}
}

static int gfx_v11_cp_resume(struct amdgv_adapter *adapt)
{
	int r;

	gfx_v11_enable_interrupt(adapt, false);

	gfx_v11_0_cp_set_doorbell_range(adapt);

	gfx_v11_mes_enable(adapt, true);

	r = gfx_v11_kiq_resume(adapt);
	if (r)
		return r;

	r = gfx_v11_kcq_resume(adapt);

	gfx_v11_enable_interrupt(adapt, true);

	if (r)
		return r;
	return 0;
}

static int gfx_v11_hw_init(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)) {

		navi32_gfx_unhalt_gpu_state(adapt);
		gfx_v11_hw_init_internal_set(adapt);
		gfx_v11_hw_init_clear_ring(adapt);
		gfx_v11_constants_init(adapt);
		gfx_v11_cp_resume(adapt);

	}
	/* Don't stop libgv init even gfx init fails */
	return 0;
}

static void gfx_v11_wait_reg_mem(struct amdgv_ring *ring, int eng_sel,
		int mem_space, int opt, uint32_t addr0,
		uint32_t addr1, uint32_t ref, uint32_t mask,
		uint32_t inv)
{
	amdgv_ring_write(ring, PACKET3(PACKET3_WAIT_REG_MEM, 5));
	amdgv_ring_write(ring,
			/* memory (1) or register (0) */
			(WAIT_REG_MEM_MEM_SPACE(mem_space) |
			 WAIT_REG_MEM_OPERATION(opt) | /* wait */
			 WAIT_REG_MEM_FUNCTION(3) |  /* equal */
			 WAIT_REG_MEM_ENGINE(eng_sel)));

	amdgv_ring_write(ring, addr0);
	amdgv_ring_write(ring, addr1);
	amdgv_ring_write(ring, ref);
	amdgv_ring_write(ring, mask);
	amdgv_ring_write(ring, inv); /* poll interval */
}

static void gfx_v11_ring_emit_hdp_flush(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t ref_and_mask, reg_mem_engine;
	const struct nbio_hdp_flush_reg *nbio_hf_reg =
		adapt->nbio.hdp_flush_reg;

	if (ring->funcs->type == AMDGV_RING_TYPE_COMPUTE) {
		switch (ring->me) {
		case 1:
			ref_and_mask =
				nbio_hf_reg->ref_and_mask_cp2 << ring->pipe;
			break;
		case 2:
			ref_and_mask =
				nbio_hf_reg->ref_and_mask_cp6 << ring->pipe;
			break;
		default:
			return;
		}
		reg_mem_engine = 0;
	} else {
		ref_and_mask = nbio_hf_reg->ref_and_mask_cp0;
		reg_mem_engine = 1; /* pfp */
	}

	gfx_v11_wait_reg_mem(ring, reg_mem_engine, 0, 1,
			adapt->nbio.funcs->get_hdp_flush_req_offset(adapt),
			adapt->nbio.funcs->get_hdp_flush_done_offset(adapt),
			ref_and_mask, ref_and_mask, 0x20);
}

static void gfx_v11_ring_set_wptr_compute(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t data;
	/* XXX check if swapping is necessary on BE */
	if (ring->use_doorbell) {
		*((volatile uint64_t *)(ring->wptr_cpu_addr)) = ring->wptr;
		WDOORBELL64(ring->doorbell_index, ring->wptr);
	} else {
		*((volatile uint64_t *)(ring->wptr_cpu_addr)) = ring->wptr;
		navi32_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0);
		data = (uint32_t)(ring->wptr & 0xffffffff);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_HQD_PQ_WPTR_LO), data);
		data = (uint32_t)(ring->wptr >> 32);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_HQD_PQ_WPTR_HI), data);
		navi32_grbm_select(adapt, 0, 0, 0, 0);
	}
}

static void gfx_v11_ring_emit_ib_compute(struct amdgv_ring *ring,
		struct amdgv_ib *ib,
		uint32_t flags)
{
	unsigned int vmid = 0;
	uint32_t control = INDIRECT_BUFFER_VALID | ib->length_dw | (vmid << 24);

	amdgv_ring_write(ring, PACKET3(PACKET3_INDIRECT_BUFFER, 2));
	amdgv_ring_write(ring,
#ifdef __BIG_ENDIAN
			(2 << 0) |
#endif
			lower_32_bits(ib->gpu_addr));
	amdgv_ring_write(ring, upper_32_bits(ib->gpu_addr));
	amdgv_ring_write(ring, control);
}

static void gfx_v11_ring_emit_fence(struct amdgv_ring *ring, uint64_t addr,
				     uint64_t seq, unsigned flags)
{
	bool write64bit = flags & AMDGV_FENCE_FLAG_64BIT;
	bool int_sel = flags & AMDGV_FENCE_FLAG_INT;

	/* RELEASE_MEM - flush caches, send int */
	amdgv_ring_write(ring, PACKET3(PACKET3_RELEASE_MEM, 6));
	amdgv_ring_write(ring, (PACKET3_RELEASE_MEM_GCR_SEQ |
				 PACKET3_RELEASE_MEM_GCR_GL2_WB |
				 PACKET3_RELEASE_MEM_GCR_GL2_INV |
				 PACKET3_RELEASE_MEM_GCR_GL2_US |
				 PACKET3_RELEASE_MEM_GCR_GL1_INV |
				 PACKET3_RELEASE_MEM_GCR_GLV_INV |
				 PACKET3_RELEASE_MEM_GCR_GLM_INV |
				 PACKET3_RELEASE_MEM_GCR_GLM_WB |
				 PACKET3_RELEASE_MEM_CACHE_POLICY(3) |
				 PACKET3_RELEASE_MEM_EVENT_TYPE(CACHE_FLUSH_AND_INV_TS_EVENT) |
				 PACKET3_RELEASE_MEM_EVENT_INDEX(5)));
	amdgv_ring_write(ring, (PACKET3_RELEASE_MEM_DATA_SEL(write64bit ? 2 : 1) |
				 PACKET3_RELEASE_MEM_INT_SEL(int_sel ? 2 : 0)));

	/*
	 * the address should be Qword aligned if 64bit write, Dword
	 * aligned if only send 32bit data low (discard data high)
	 */
	amdgv_ring_write(ring, lower_32_bits(addr));
	amdgv_ring_write(ring, upper_32_bits(addr));
	amdgv_ring_write(ring, lower_32_bits(seq));
	amdgv_ring_write(ring, upper_32_bits(seq));
	amdgv_ring_write(ring, 0);
}

static void gfx_v11_ring_emit_wreg(struct amdgv_ring *ring, uint32_t reg,
		uint32_t val)
{
	uint32_t cmd = 0;

	switch (ring->funcs->type) {
	case AMDGV_RING_TYPE_GFX:
		cmd = WRITE_DATA_ENGINE_SEL(1) | WR_CONFIRM;
		break;
	case AMDGV_RING_TYPE_KIQ:
		cmd = (1 << 16); /* no inc addr */
		break;
	default:
		cmd = WR_CONFIRM;
		break;
	}
	amdgv_ring_write(ring, PACKET3(PACKET3_WRITE_DATA, 3));
	amdgv_ring_write(ring, cmd);
	amdgv_ring_write(ring, reg);
	amdgv_ring_write(ring, 0);
	amdgv_ring_write(ring, val);
}

static int gfx_v11_ring_test_ring(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t scratch = SOC15_REG_OFFSET(GC, 0, regSCRATCH_REG0);
	uint32_t tmp = 0;
	unsigned int i;
	int r;

	WREG32(scratch, 0xCAFEDEAD);
	r = amdgv_ring_alloc(ring, 5);
	if (r) {
		AMDGV_ERROR("amdgpu: cp failed to lock ring %d (%d).\n",
				ring->idx, r);
		return r;
	}

	gfx_v11_ring_emit_wreg(ring, scratch, 0xDEADBEEF);

	amdgv_ring_commit(ring);

	for (i = 0; i < AMDGV_GFX_MAX_USEC_TIMEOUT; i++) {
		tmp = RREG32(scratch);
		if (tmp == 0xDEADBEEF)
			break;
		oss_udelay(1);
	}

	if (i >= AMDGV_GFX_MAX_USEC_TIMEOUT)
		r = AMDGV_FAILURE;
	return r;
}

static void gfx_v11_set_aql_comp_ring_info(struct amdgv_adapter *adapt,
		struct amdgv_ring *aql_compute_ring, struct oss_aql_comp_rb_info *info)
{
	aql_compute_ring->adapt = adapt;

	aql_compute_ring->aql_enable = true;
	aql_compute_ring->use_doorbell = true;
	aql_compute_ring->wptr = *(info->wptr_poll_memory);
	aql_compute_ring->buf_mask = info->ring_dw_size - 1;
	aql_compute_ring->ring = info->ring_base;
	aql_compute_ring->ptr_mask =
			aql_compute_ring->funcs->support_64bit_ptrs ? 0xffffffffffffffff : aql_compute_ring->buf_mask;
	aql_compute_ring->wptr_cpu_addr = (volatile uint32_t *)(info->wptr_poll_memory);
	aql_compute_ring->doorbell_index = info->doorbell_offset_in_dword;
	aql_compute_ring->max_dw = AQL_COMP_RING_MAX_DWORD;
}

static int amdgv_wait_dump_cu_data_cb(void *context)
{
	bool *finish_dump = (bool *)context;
	return !(*finish_dump == true);
}

static int gfx_v11_dump_cu_data(struct amdgv_adapter *adapt, enum AMDGV_CU_DATA_TYPE type)
{
	int i;
	int r = 0;
	uint32_t wb_size;
	struct amdgv_memmgr_mem *kernelobj, *kernelarg, *host_out_data, *host_out_flag, *packet, *signal_obj;
	uint64_t *kernarg_addr;
	uint32_t *kernelobj_addr;
	hsa_kernel_dispatch_packet_t *packet_addr;
	hsa_signal_t signal;
	uint32_t *output;
	uint32_t alignment = 256; // 256-byte alignment
	uint32_t padding_kernelobj;
	uint32_t padding_shader;
	int kernelobj_size, shader_size;
	struct amdgv_ring *mec_ring = NULL;
	uint32_t group_segment_size;
	struct oss_aql_comp_rb_info *aql_comp_rb_info = NULL;

	if (adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE) {
		aql_comp_rb_info = (struct oss_aql_comp_rb_info *)(oss_alloc_memory(sizeof(struct oss_aql_comp_rb_info)));
		if (aql_comp_rb_info == NULL)
			return AMDGV_FAILURE;

		if (oss_map_queue(adapt->dev, true, OSS_COMPUTE_AQL_QUEUE, aql_comp_rb_info)) {
			gfx_v11_set_aql_comp_ring_info(adapt, &adapt->aql_compute_ring, aql_comp_rb_info);
			mec_ring = &(adapt->aql_compute_ring);
		} else {
			AMDGV_ERROR("Map queue failed.\n");
			r = AMDGV_FAILURE;
			goto clean6;
		}
	} else {
		amdgv_gfx_map_kcq(adapt, 0, XCC_QUEUE_INDEX__AQL);
		mec_ring = &(adapt->gfx.compute_ring[XCC_QUEUE_INDEX__AQL]);
	}
	wb_size = amdgv_gfx_calculate_cu_data_size(adapt, type);

	switch (type) {
	case AMDGV_CU_DATA_TYPE__LDS:
		AMDGV_INFO("== dump LDS ==\n");
		group_segment_size = 0x10000;
		kernelobj_size = sizeof(gfx_v11_dump_lds_kernelobj);
		shader_size = sizeof(gfx_v11_dump_lds_shader);
		break;
	case AMDGV_CU_DATA_TYPE__SGPRs:
		AMDGV_INFO("== dump SGPR ==\n");
		group_segment_size = 0;
		kernelobj_size = sizeof(gfx_v11_dump_sgpr_kernelobj);
		shader_size = sizeof(gfx_v11_dump_sgpr_shader);
		break;
	case AMDGV_CU_DATA_TYPE__VGPRs:
		AMDGV_INFO("== dump VGPR ==\n");
		group_segment_size = 0x10000;
		kernelobj_size = sizeof(gfx_v11_dump_vgpr_kernelobj);
		shader_size = sizeof(gfx_v11_dump_vgpr_shader);
		break;
	default:
		AMDGV_ERROR("unsupported type\n");
		goto unmap;
	}
	padding_kernelobj = (alignment - (kernelobj_size % alignment)) % alignment;
	padding_shader = (alignment - (shader_size % alignment)) % alignment;

	// Allocate the memory:
	// kernelarg: hold address of host_out_data and host_out_flag
	// host_out_data: hold dump data
	// host_out_flag: dump flag which indicates the valid data position
	// kernelobj: hsa kernel obj and shader
	// signal_obj: completion signal
	// packet: aql packet

	kernelarg = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, 256, 256, MEM_GFX_IB);
	if (!kernelarg) {
		AMDGV_WARN("failed to create kernelarg.\n");
		goto unmap;
	}
	host_out_data = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, wb_size, 256, MEM_GFX_IB);
	if (!host_out_data) {
		AMDGV_WARN("failed to create host_out_data.\n");
		goto clean5;
	}
	host_out_flag = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, wb_size/sizeof(uint32_t), 256, MEM_GFX_IB);
	if (!host_out_flag) {
		AMDGV_WARN("failed to create host_out_flag.\n");
		goto clean4;
	}
	kernelobj = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, padding_kernelobj +
			padding_shader + kernelobj_size + shader_size, 256, MEM_GFX_IB);
	if (!kernelobj) {
		AMDGV_WARN("failed to create kernelobj.\n");
		goto clean3;
	}
	signal_obj = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, 256, 256, MEM_GFX_IB);
	if (!signal_obj) {
		AMDGV_WARN("failed to create signal_obj.\n");
		goto clean2;
	}
	packet = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, sizeof(hsa_kernel_dispatch_packet_t), 256, MEM_GFX_IB);
	if (!packet) {
		AMDGV_WARN("failed to create packet.\n");
		goto clean1;
	}

	signal.handle = amdgv_memmgr_get_gpu_addr(signal_obj);
	kernelobj_addr = (uint32_t *)amdgv_memmgr_get_cpu_addr(kernelobj);
	oss_memset(kernelobj_addr, 0, padding_kernelobj + padding_shader + kernelobj_size + shader_size);

	switch (type) {
	case AMDGV_CU_DATA_TYPE__LDS:
		oss_memcpy(kernelobj_addr, &gfx_v11_dump_lds_kernelobj, kernelobj_size);
		oss_memcpy(kernelobj_addr + (padding_kernelobj + kernelobj_size)/sizeof(uint32_t), gfx_v11_dump_lds_shader, shader_size);
		break;
	case AMDGV_CU_DATA_TYPE__SGPRs:
		oss_memcpy(kernelobj_addr, &gfx_v11_dump_sgpr_kernelobj, kernelobj_size);
		oss_memcpy(kernelobj_addr + (padding_kernelobj + kernelobj_size)/sizeof(uint32_t), gfx_v11_dump_sgpr_shader, shader_size);
		break;
	case AMDGV_CU_DATA_TYPE__VGPRs:
		oss_memcpy(kernelobj_addr, &gfx_v11_dump_vgpr_kernelobj, kernelobj_size);
		oss_memcpy(kernelobj_addr + (padding_kernelobj + kernelobj_size)/sizeof(uint32_t), gfx_v11_dump_vgpr_shader, shader_size);
		break;
	default:
		AMDGV_ERROR("unsupported type\n");
		goto cleanall;
	}

	((kernel_descriptor_t *)kernelobj_addr)->kernel_code_entry_byte_offset = padding_kernelobj + kernelobj_size;

	//do some cleanup
	oss_memset((uint64_t *)amdgv_memmgr_get_cpu_addr(host_out_data), 2, wb_size);
	oss_memset((uint64_t *)amdgv_memmgr_get_cpu_addr(host_out_flag), 0, wb_size/sizeof(uint32_t));
	oss_memset((uint64_t *)amdgv_memmgr_get_cpu_addr(signal_obj), 0, 256);

	kernarg_addr = (uint64_t *)amdgv_memmgr_get_cpu_addr(kernelarg);
	kernarg_addr[0] = amdgv_memmgr_get_gpu_addr(host_out_data);
	kernarg_addr[1] = amdgv_memmgr_get_gpu_addr(host_out_flag);

	packet_addr = (hsa_kernel_dispatch_packet_t *)amdgv_memmgr_get_cpu_addr(packet);
	oss_memset(packet_addr, 0, sizeof(hsa_kernel_dispatch_packet_t));

	packet_addr->header |= HSA_PACKET_TYPE_KERNEL_DISPATCH << HSA_PACKET_HEADER_TYPE;
	packet_addr->header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
	packet_addr->header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
	packet_addr->setup = 1 << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
	packet_addr->workgroup_size_x = 1024;
	packet_addr->workgroup_size_y = 1;
	packet_addr->workgroup_size_z = 1;
	packet_addr->grid_size_x = 0xd800;
	packet_addr->grid_size_y = 1;
	packet_addr->grid_size_z = 1;
	packet_addr->private_segment_size = 0;
	packet_addr->group_segment_size = group_segment_size;
	packet_addr->kernel_object = amdgv_memmgr_get_gpu_addr(kernelobj);
	packet_addr->kernarg_address = (void *)(amdgv_memmgr_get_gpu_addr(kernelarg));
	packet_addr->completion_signal = signal;

	amdgv_ring_alloc(mec_ring, sizeof(hsa_kernel_dispatch_packet_t)/sizeof(uint32_t));
	for (i = 0; i < sizeof(hsa_kernel_dispatch_packet_t)/sizeof(uint32_t); i++) {
		amdgv_ring_write(mec_ring, ((uint32_t *)packet_addr)[i]);
	}

	output = amdgv_memmgr_get_cpu_addr(host_out_data);

	amdgv_ring_commit(mec_ring);
	oss_msleep(100);

	adapt->gfx.cu_dump_data_info.cu_dump_size = wb_size;
	adapt->gfx.cu_dump_data_info.cu_dump_type = type;
	adapt->gfx.cu_dump_data_info.cu_dump_finished = false;

	adapt->gfx.cu_dump_data_info.cu_data = (const char *)amdgv_memmgr_get_cpu_addr(host_out_data);
	adapt->gfx.cu_dump_data_info.cu_data_flags = (const char *)amdgv_memmgr_get_cpu_addr(host_out_flag);
	oss_signal_event(adapt->gfx.cu_dump_data_info.cu_dump_event); // signal event to CU dump worker thread

	// wait 70 seconds in maximum for the dump process and then free data
	r = amdgv_wait_for(adapt, amdgv_wait_dump_cu_data_cb,
				     (void *)&adapt->gfx.cu_dump_data_info.cu_dump_finished,
				     AMDGV_TIMEOUT(TIMEOUT_DUMP_CU_DATA), 0);

cleanall:
	amdgv_memmgr_free(packet);
clean1:
	amdgv_memmgr_free(signal_obj);
clean2:
	amdgv_memmgr_free(kernelobj);
clean3:
	amdgv_memmgr_free(host_out_flag);
	adapt->gfx.cu_dump_data_info.cu_data_flags = NULL;
clean4:
	amdgv_memmgr_free(host_out_data);
	adapt->gfx.cu_dump_data_info.cu_data = NULL;
clean5:
	amdgv_memmgr_free(kernelarg);
unmap:
	if (adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)
		oss_map_queue(adapt->dev, false, OSS_COMPUTE_AQL_QUEUE, NULL);
	else
		amdgv_gfx_unmap_kcq(adapt, 0, XCC_QUEUE_INDEX__AQL);
clean6:
	if (adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)
		oss_free_memory(aql_comp_rb_info);

	return r;
}

static const struct amdgv_ring_funcs gfx_v11_ring_funcs_compute = {
	.type = AMDGV_RING_TYPE_COMPUTE,
	.align_mask = 0xff,
	.aql_align_mask = 0xf,
	.nop = PACKET3(PACKET3_NOP, 0x3FFF),
	.aql_nop = 0x0,
	.support_64bit_ptrs = true,
	.vmhub = 0,
	.set_wptr = gfx_v11_ring_set_wptr_compute,
	.submit_frame = gfx_v11_ring_submit_frame,
	.emit_frame_size =
		20 + /* gfx_v11_ring_emit_gds_switch */
		7 + /* gfx_v11_ring_emit_hdp_flush */
		5 + /* hdp invalidate */
		7 + /* gfx_v11_ring_emit_pipeline_sync */
		SOC15_FLUSH_GPU_TLB_NUM_WREG * 5 +
		SOC15_FLUSH_GPU_TLB_NUM_REG_WAIT * 7 +
		2 + /* gfx_v11_ring_emit_vm_flush */
		/* gfx_v11_ring_emit_fence x3 for user fence,
		 * vm fence
		 */
		8 + 8 + 8 +
		7 + /* gfx_v11_emit_mem_sync */
		/* gfx_v11_emit_wave_limit for updating
		 * mmSPI_WCL_PIPE_PERCENT_GFX register
		 */
		5 +
		15, /* for updating 3 mmSPI_WCL_PIPE_PERCENT_CS registers */
	.emit_ib_size =	7, /* gfx_v11_ring_emit_ib_compute */
	.emit_ib = gfx_v11_ring_emit_ib_compute,
	.emit_fence = gfx_v11_ring_emit_fence,
	.emit_hdp_flush = gfx_v11_ring_emit_hdp_flush,
	.insert_nop = amdgv_ring_insert_nop,
	.emit_wreg = gfx_v11_ring_emit_wreg,
};

static const struct amdgv_ring_funcs gfx_v11_ring_funcs_kiq = {
	.type = AMDGV_RING_TYPE_KIQ,
	.align_mask = 0xff,
	.nop = PACKET3(PACKET3_NOP, 0x3FFF),
	.support_64bit_ptrs = true,
	.vmhub = 0,
	.set_wptr = gfx_v11_ring_set_wptr_compute,
	.emit_frame_size =
		20 + /* gfx_v11_ring_emit_gds_switch */
		7 + /* gfx_v11_ring_emit_hdp_flush */
		5 + /* hdp invalidate */
		7 + /* gfx_v11_ring_emit_pipeline_sync */
		SOC15_FLUSH_GPU_TLB_NUM_WREG * 5 +
		SOC15_FLUSH_GPU_TLB_NUM_REG_WAIT * 7 +
		2 + /* gfx_v11_ring_emit_vm_flush */
		/* gfx_v11_ring_emit_fence_kiq x3 for user fence,
		 * vm fence
		 */
		8 + 8 + 8,
	.emit_ib_size =	7, /* gfx_v11_ring_emit_ib_compute */
	.test_ring = gfx_v11_ring_test_ring,
	.insert_nop = amdgv_ring_insert_nop,
	.emit_wreg = gfx_v11_ring_emit_wreg,
};

static void gfx_v11_set_ring_funcs(struct amdgv_adapter *adapt)
{
	uint32_t i;

	adapt->gfx.kiq[0].ring.funcs = &gfx_v11_ring_funcs_kiq;
	adapt->aql_compute_ring.funcs = &gfx_v11_ring_funcs_compute;
	for (i = 0; i < adapt->gfx.num_compute_rings; i++)
		adapt->gfx.compute_ring[i].funcs = &gfx_v11_ring_funcs_compute;
}

static int gfx_v11_early_init(struct amdgv_adapter *adapt)
{
	adapt->gfx.num_gfx_rings = 0;
	adapt->gfx.num_compute_rings = AMDGV_MAX_COMPUTE_RINGS;

	gfx_v11_set_ring_funcs(adapt);
	gfx_v11_set_kiq_pm4_funcs(adapt);
	adapt->gfx.funcs->dump_cu_data = gfx_v11_dump_cu_data;

	return 0;
}

static int gfx_v11_hw_fini_internal(struct amdgv_adapter *adapt)
{
	/* DF freeze and kcq disable will fail */

	/* disable KCQ to avoid CPC touch memory not valid anymore */
	if (adapt->flags & AMDGV_FLAG_ENABLE_COMPUTE_PAGING) {
		amdgv_gfx_unmap_kcq(adapt, 0, XCC_QUEUE_INDEX__PAGING);
	}

	gfx_v11_cp_compute_enable(adapt, false);
	WREG32_SOC15(GC, 0, regRLC_CP_SCHEDULERS, 0);

	gfx_v11_mes_enable(adapt, false);

	return 0;
}

static int gfx_v11_hw_fini(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)) {
		gfx_v11_hw_fini_internal(adapt);
	}
	return 0;
}

struct amdgv_init_func gfx_v11_func = {
	.name = "gfx_v11_func",
	.is_engine = true,
	.sw_init = gfx_v11_sw_init,
	.sw_fini = gfx_v11_sw_fini,
	.hw_init = gfx_v11_hw_init,
	.hw_fini = gfx_v11_hw_fini,
	.hw_live_init = gfx_v11_hw_init_internal_set,
};

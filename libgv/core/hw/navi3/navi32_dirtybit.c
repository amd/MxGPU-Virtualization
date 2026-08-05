/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv.h>
#include "amdgv_ffbm.h"
#include "amdgv_ring.h"
#include "navi32_dirtybit.h"
#include "gfx_v11_0.h"
#include "amdgv_oss_wrapper.h"

#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include "sdma60_pkt_struct.h"
#include <navi3/MMHUB/mmhub_3_0_0_offset.h>
#include <navi3/MMHUB/mmhub_3_0_0_sh_mask.h>

static const int this_block = AMDGV_LIVE_MIGRATION_BLOCK;

/* GL2C instance index -> MAM instance for GCEA_SDP access (GRBM_GFX_INDEX). */
static const uint8_t navi32_gl2c_to_mam[16] = {
	0,  8,  1,  9,  2, 10,  3, 11,	/* GL2C 0-7 */
	4, 12,  5, 13,  6, 14,  7, 15,	/* GL2C 8-15 */
};

void navi32_select_mam_instance(struct amdgv_adapter *adapt, uint8_t mam_instance)
{
	uint32_t data = 0;

	if (mam_instance >= MAX_MAM_INSTANCES_NAVI32) {
		AMDGV_ERROR("Dirty Bit: Invalid MAM instance %d\n", mam_instance);
		return;
	}

	data = REG_SET_FIELD(0, GRBM_GFX_INDEX, INSTANCE_INDEX, mam_instance);

	WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_INDEX), data);
}

void navi32_dirtybit_gcea_sdp_control(struct amdgv_adapter *adapt, bool gcea_sdp_enable)
{
	uint32_t tcc_value;
	uint16_t gl2c_harvested_mask = 0;
	uint32_t sdp_value = 0;
	int i;

	/* Find out which instance of GL2C is harvested */
	/* nv32 has 16 instances of GL2C, so only get the TCC_DISABLE field */
	tcc_value = RREG32(SOC15_REG_OFFSET(GC, 0, regCGTS_TCC_DISABLE));
	gl2c_harvested_mask = REG_GET_FIELD(tcc_value, CGTS_TCC_DISABLE, TCC_DISABLE);
	AMDGV_DEBUG("GL2C harvested instance: %d\n", gl2c_harvested_mask);

	for (i = 0; i < 16; i++) {
		if (gl2c_harvested_mask & (1 << i)) {
			AMDGV_DEBUG("%s SDP for harvested GL2C instance %d GCEA instance %d\n", gcea_sdp_enable ? "Enable" : "Disable", i, navi32_gl2c_to_mam[i]);
			navi32_select_mam_instance(adapt, navi32_gl2c_to_mam[i]);
			sdp_value = RREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_SDP_ENABLE));
			if (REG_GET_FIELD(sdp_value, GCEA_SDP_ENABLE, ENABLE) != gcea_sdp_enable) {
				sdp_value = REG_SET_FIELD(sdp_value, GCEA_SDP_ENABLE, ENABLE, gcea_sdp_enable ? 1 : 0);
				WREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_SDP_ENABLE), sdp_value);
			}
		}
	}

	navi32_select_mam_instance(adapt, 0);
}

int navi32_dirtybit_control(struct amdgv_adapter *adapt, bool enable)
{
	uint8_t i;
	uint32_t gc_value;
	uint32_t mm_value;
	uint32_t value = enable ? 0 : 1;

	if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION) {
		for (i = 0; i < MAX_MAM_INSTANCES_NAVI32; i++) {
			navi32_select_mam_instance(adapt, i);

			gc_value = RREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_CTRL));
			gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_CTRL, MAM_DISABLE, value);

			WREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_CTRL), gc_value);
			gc_value = RREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_CTRL));

			AMDGV_DEBUG("MAM instance %d regGCEA_MAM_CTRL = 0x%x\n", i, gc_value);
		}

		/* set grbm_gfx_index back to 0 to ensure other broadcast write is not affected */
		navi32_select_mam_instance(adapt, 0);

		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_MAM_CTRL));
		mm_value = REG_SET_FIELD(mm_value, DAGB0_MAM_CTRL, MAM_DISABLE, value);

		WREG32(SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_MAM_CTRL), mm_value);
		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_MAM_CTRL));
		AMDGV_DEBUG("MAM regDAGB0_MAM_CTRL = 0x%x\n", mm_value);

	}
	return 0;
}

static int navi32_dirtybit_query_dirty_page_size(struct amdgv_adapter *adapt, uint32_t *dirty_page_size)
{
	*dirty_page_size = adapt->dirtybit.dirty_page_size;
	return 0;
}

/* driver query GC and/or MM for segment dirty status */
int navi32_is_segment_dirty(struct amdgv_adapter *adapt, uint64_t segment, bool dbit_preserve,
				enum NV32_DBIT_QUERY query_type, bool *is_dirty)
{
	uint32_t gc_value = 0;
	uint32_t mm_value = 0;
	uint32_t grbm_data = 0;
	int wait_ret;
	int ret = 0;

	segment >>= SHIFT_256K;

	/* query */
	if (query_type == NV32_DBIT_QUERY_GC_MM || query_type == NV32_DBIT_QUERY_GC) {
		/* wait for GC MAM 0 query ready */
		navi32_select_mam_instance(adapt, 0);

		/* a1. wait for a query ready */
		wait_ret = amdgv_wait_for_register(
			adapt, SOC15_REG_OFFSET_NAME(GC, 0, regGCEA_MAM_STATUS),
			REG_FIELD_MASK(GCEA_MAM_STATUS, DBIT_QUERY_RDY),
			REG_FIELD_MASK(GCEA_MAM_STATUS, DBIT_QUERY_RDY),
			AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ,
			AMDGV_WAIT_FLAG_FORCE_DELAY);

		if (wait_ret) {
			ret = AMDGV_FAILURE;
			goto out;
		}

		/* b1. query GC MAM instances - broadcast write to all instances */
		gc_value = RREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_DBIT_QUERY));
		/* segment is 256-k aligned memory segment physical address */
		gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_DBIT_QUERY, QUERY_ADDR, segment);
		gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_DBIT_QUERY, QUERY_EN, 1);

		if (dbit_preserve)
			gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_DBIT_QUERY, DBIT_PRESERVE, 1);
		else
			gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_DBIT_QUERY, DBIT_PRESERVE, 0);

		grbm_data = REG_SET_FIELD(grbm_data, GRBM_GFX_INDEX, INSTANCE_BROADCAST_WRITES, 1);
		grbm_data = REG_SET_FIELD(grbm_data, GRBM_GFX_INDEX, SE_BROADCAST_WRITES, 1);
		grbm_data = REG_SET_FIELD(grbm_data, GRBM_GFX_INDEX, SA_BROADCAST_WRITES, 1);
		WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_INDEX), grbm_data);

		WREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_DBIT_QUERY), gc_value);

		navi32_select_mam_instance(adapt, 0);
	}

	if (query_type == NV32_DBIT_QUERY_GC_MM || query_type == NV32_DBIT_QUERY_MM) {
		/* a2. wait MM MAM query ready */
		wait_ret = amdgv_wait_for_register(
			adapt, SOC15_REG_OFFSET_NAME(MMHUB, 0, regDAGB0_MAM_STATUS),
			REG_FIELD_MASK(DAGB0_MAM_STATUS, DBIT_QUERY_RDY),
			REG_FIELD_MASK(DAGB0_MAM_STATUS, DBIT_QUERY_RDY),
			AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ,
			AMDGV_WAIT_FLAG_FORCE_DELAY);

		if (wait_ret) {
			ret = AMDGV_FAILURE;
			goto out;
		}

		/* b2.query MM MAM */
		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_MAM_DBIT_QUERY));
		/* segment is 256-k aligned memory segment physical address */
		mm_value = REG_SET_FIELD(mm_value, DAGB0_MAM_DBIT_QUERY, QUERY_ADDR, segment);
		mm_value = REG_SET_FIELD(mm_value, DAGB0_MAM_DBIT_QUERY, QUERY_EN, 1);

		if (dbit_preserve)
			mm_value = REG_SET_FIELD(mm_value, DAGB0_MAM_DBIT_QUERY, DBIT_PRESERVE, 1);
		else
			mm_value = REG_SET_FIELD(mm_value, DAGB0_MAM_DBIT_QUERY, DBIT_PRESERVE, 0);

		WREG32(SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_MAM_DBIT_QUERY), mm_value);
	}

	/* c. read result */
	if (query_type == NV32_DBIT_QUERY_GC_MM || query_type == NV32_DBIT_QUERY_GC) {
		/* wait for GC MAM 0 query ready */
		navi32_select_mam_instance(adapt, 0);

		/* wait for a query ready */
		wait_ret = amdgv_wait_for_register(
			adapt, SOC15_REG_OFFSET_NAME(GC, 0, regGCEA_MAM_STATUS),
			REG_FIELD_MASK(GCEA_MAM_STATUS, DBIT_QUERY_RDY),
			REG_FIELD_MASK(GCEA_MAM_STATUS, DBIT_QUERY_RDY),
			AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ,
			AMDGV_WAIT_FLAG_FORCE_DELAY);

		if (wait_ret) {
			ret = AMDGV_FAILURE;
			goto out;
		}

		gc_value = RREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_STATUS));
		gc_value = REG_GET_FIELD(gc_value, GCEA_MAM_STATUS, DBIT_QUERY_DIRTY);
	}

	if (query_type == NV32_DBIT_QUERY_GC_MM || query_type == NV32_DBIT_QUERY_MM) {
		/* wait MM MAM query ready */
		wait_ret = amdgv_wait_for_register(
			adapt, SOC15_REG_OFFSET_NAME(MMHUB, 0, regDAGB0_MAM_STATUS),
			REG_FIELD_MASK(DAGB0_MAM_STATUS, DBIT_QUERY_RDY),
			REG_FIELD_MASK(DAGB0_MAM_STATUS, DBIT_QUERY_RDY),
			AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ,
			AMDGV_WAIT_FLAG_FORCE_DELAY);

		if (wait_ret) {
			ret = AMDGV_FAILURE;
			goto out;
		}

		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_MAM_STATUS));
		mm_value = REG_GET_FIELD(mm_value, DAGB0_MAM_STATUS, DBIT_QUERY_DIRTY);
	}

	*is_dirty = mm_value || gc_value;

out:
	return ret;
}

/* sdma dirty bit polling and update dbit */
static int navi32_dirtybit_sdma_poll_dbit(struct amdgv_adapter *adapt, struct nv32_poll_dbit_write_mem *data)
{
	/* sdma1 */
	struct amdgv_ring *ring = &adapt->sdma.sdma_ring[1];
	uint32_t seq = ++ring->fence_drv.sync_seq;
	int r = 0;
	PSDMA_PKT_FENCE pFence;
	uint64_t start;
	uint64_t end;
	/* patch frame */
	uint8_t *pbuffer = adapt->dirtybit.query_submission_frame;
	PSDMA_PKT_POLL_DBIT_WRITE_MEM p = (PSDMA_PKT_POLL_DBIT_WRITE_MEM)pbuffer;

	oss_memset(pbuffer, 0, ring->max_dw * 4);
	p->HEADER_UNION.op = 0x08;
	p->HEADER_UNION.sub_op = 0x02;
	p->HEADER_UNION.ea = 0x2;  /* GC_EA */
	p->HEADER_UNION.clear_dbit = data->clear_dbit ? 1 : 0;
	p->DST_ADDR_LO_UNION.DW_1_DATA = data->gc_destination_addr;
	p->DST_ADDR_HI_UNION.DW_2_DATA = data->gc_destination_addr >> 32;
	p->START_PAGE_UNION.addr_31_4 = data->query_addr >> SHIFT_64K;
	p->PAGE_NUM_UNION.DW_4_DATA = data->number_of_pages;
	pbuffer += sizeof(SDMA_PKT_POLL_DBIT_WRITE_MEM);

	pFence = (PSDMA_PKT_FENCE)pbuffer;
	pFence->HEADER_UNION.op = 0x05;
	pFence->HEADER_UNION.mtype = 3;
	pFence->ADDR_LO_UNION.addr_31_0 = ring->fence_drv.gpu_addr;
	pFence->ADDR_HI_UNION.addr_63_32 = ring->fence_drv.gpu_addr >> 32;
	pFence->DATA_UNION.data = seq;
	pbuffer += sizeof(SDMA_PKT_FENCE);

	/* submit */
	start = oss_get_time_stamp();
	ring->funcs->submit_frame(ring, adapt->dirtybit.query_submission_frame);

	/* poll fence */
	r = amdgv_fence_wait_polling(ring, seq, AMDGV_SDMA_DBIT_MAX_USEC_TIMEOUT);
	if (r < 1) {
		AMDGV_ERROR("Dirty Bit: SDMA poll dbit failed\n");
		r = AMDGV_FAILURE;
	} else {
		end = oss_get_time_stamp();
		r = 0;
		AMDGV_DEBUG("Dirty Bit: SDMA poll dbit took 0x%llx us\n", (uint64_t)(end - start));
	}
	return r;
}

static void copy_dirty_bits(uint32_t number_of_dirty_bits, uint8_t *dirty_bits,
			uint32_t dbit_buffer_bit_offset, uint8_t *dbit_buffer)
{
	uint32_t dirty_bytes = roundup(number_of_dirty_bits, 8) / 8;
	uint32_t byte_index = dbit_buffer_bit_offset >> 3;
	uint32_t shift = dbit_buffer_bit_offset % 8;
	uint32_t i;

	if (0 == shift)	{
		for (i = 0; i < dirty_bytes; i++)
			dbit_buffer[byte_index + i] = dirty_bits[i];
	} else {
		uint8_t carry = 0;
		uint8_t next_carry = 0;

		for (i = 0; i < dirty_bytes; i++) {
			next_carry = dirty_bits[i] >> (8 - shift);
			dbit_buffer[byte_index + i] |= ((dirty_bits[i] << shift) | carry);
			carry = next_carry;
		}
		if (carry)
			dbit_buffer[byte_index + dirty_bytes] |= carry;
	}
}

static int mmio_query_dirty_bits(struct amdgv_adapter *adapt, enum NV32_DBIT_QUERY dbit_query,
			uint32_t dbit_buffer_bit_offset, uint8_t *dbit_buffer,
			bool dbit_preserve, uint64_t dbit_query_spa, uint64_t dbit_query_size)
{
	static uint8_t bits[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
	uint64_t query_spa = dbit_query_spa;
	bool is_segment_dirty = false;
	uint32_t number_of_dirty_bits = (dbit_query_size >> SHIFT_1M);
	uint32_t i;

	for (i = 0; i < number_of_dirty_bits; i++) {
		if (navi32_is_segment_dirty(adapt, query_spa, dbit_preserve, dbit_query, &is_segment_dirty))
			return AMDGV_FAILURE;
		if (is_segment_dirty) {
			uint32_t byte_offset = dbit_buffer_bit_offset >> 3;
			uint32_t bit_offset = dbit_buffer_bit_offset % 8;

			dbit_buffer[byte_offset] |= bits[bit_offset];
		}
		query_spa += SEGMENT_SIZE_1M;
		++dbit_buffer_bit_offset;
	}
	return 0;
}

static int query_block_dirty_bits(struct amdgv_adapter *adapt, uint32_t dbit_buffer_bit_offset,
			uint8_t *dbit_buffer, bool dbit_preserve, uint64_t dbit_query_spa, uint64_t dbit_query_size)
{
	int ret = AMDGV_FAILURE;
	struct nv32_poll_dbit_write_mem sdma_data = {0};
	uint8_t *dbit_gc = (uint8_t *)amdgv_memmgr_get_cpu_addr(adapt->dirtybit.gc_dirty_bitplane);
	uint32_t number_of_dirty_bits = (dbit_query_size >> SHIFT_1M);
	uint32_t sdma_dbit_bytes = roundup(number_of_dirty_bits, 8) / 8;

	oss_memset(dbit_gc, 0, sdma_dbit_bytes);

	/* Query the contiguous segment starting at VF FB SPA + query_fb_offset */
	sdma_data.query_addr = dbit_query_spa + adapt->mc_fb_loc_addr;
	sdma_data.number_of_pages = number_of_dirty_bits;
	sdma_data.clear_dbit = dbit_preserve ? 0 : 1;
	sdma_data.gc_destination_addr = amdgv_memmgr_get_gpu_addr(adapt->dirtybit.gc_dirty_bitplane);

	if (!navi32_dirtybit_sdma_poll_dbit(adapt, &sdma_data))	{
		copy_dirty_bits(number_of_dirty_bits, dbit_gc, dbit_buffer_bit_offset, dbit_buffer);
		ret = mmio_query_dirty_bits(adapt, NV32_DBIT_QUERY_MM, dbit_buffer_bit_offset,
							dbit_buffer, dbit_preserve, dbit_query_spa, dbit_query_size);
	}
	return ret;
}

static int query_dirty_bits_through_ffbm_blocks(struct amdgv_adapter *adapt,
		struct amdgv_vf_ffbm_map_list *vf_ffbm_map_list, uint64_t query_offset,
		uint64_t query_end_offset, uint8_t *dbit_buffer, bool dbit_preserve)
{
	int ret = 0;
	uint64_t ffbm_offset = 0;
	uint64_t dbit_query_offset = query_offset;
	uint64_t dbit_query_spa;
	uint64_t dbit_query_size;
	uint64_t block_offset;
	uint64_t end_offset;
	uint32_t dbit_buffer_bit_offset;
	uint32_t i;

	for (i = 0; i < vf_ffbm_map_list->count; i++) {
		dbit_query_size = 0;
		end_offset = ffbm_offset + vf_ffbm_map_list->entry[i].size;
		if (ffbm_offset <= dbit_query_offset && dbit_query_offset < end_offset) {
			block_offset = dbit_query_offset - ffbm_offset;
			dbit_query_spa = vf_ffbm_map_list->entry[i].spa + block_offset;
			dbit_query_size = vf_ffbm_map_list->entry[i].size - block_offset;
			if (end_offset > query_end_offset)
				dbit_query_size -= (end_offset - query_end_offset);
		}
		if (dbit_query_size) {
			dbit_buffer_bit_offset = (uint32_t)((dbit_query_offset - query_offset) >> SHIFT_1M);
#ifdef PURE_DRIVER_QUERY_DBIT
			ret = mmio_query_dirty_bits(adapt, NV32_DBIT_QUERY_GC_MM, dbit_buffer_bit_offset,
							dbit_buffer, dbit_preserve, dbit_query_spa, dbit_query_size);
#else
			ret = query_block_dirty_bits(adapt, dbit_buffer_bit_offset, dbit_buffer, dbit_preserve,
						dbit_query_spa, dbit_query_size);
#endif
			if (ret)
				break;
		}
		ffbm_offset += vf_ffbm_map_list->entry[i].size;
		dbit_query_offset += dbit_query_size;

		if (dbit_query_offset >= query_end_offset)
			break;
	}
	return ret;
}

static int navi32_query_dirtybit_data(struct amdgv_adapter *adapt,
				struct amdgv_query_dirty_bit_data *data)
{
	struct amdgv_vf_device *vf_dev = &adapt->array_vf[data->idx_vf];
	int ret = 0;
	uint8_t *dbit_buffer = data->dbit_plane_data_buffer;
	uint64_t num_segments = 0;
	uint64_t num_bits = 0;
	uint64_t query_offset = 0;
	struct amdgv_vf_ffbm_map_list *vf_ffbm_map_list = NULL;
	uint64_t query_end_offset = data->query_fb_offset + data->query_size;
	uint64_t query_bytes_size;

	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION)) {
		AMDGV_ERROR("GPUV live migration not enabled\n");
		return AMDGV_FAILURE;
	}

	/* nv32 must have ffbm enabled */
	if (!adapt->ffbm.enabled) {
		AMDGV_ERROR("FFBM not enabled\n");
		return AMDGV_FAILURE;
	}

	if ((data->query_size == 0) ||
		((data->query_fb_offset + data->query_size) > MBYTES_TO_BYTES(vf_dev->fb_size_os)) ||
		(data->dbit_plane_data_buffer == NULL) ||
		(data->dbit_plane_data_size == 0)) {
		AMDGV_ERROR("Dirty Bit: Invalid query input\n");
		return AMDGV_FAILURE;
	}

	ret = amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL);
	if (ret) {
		/* TODO: defer on transient WS fail in mVF case */
		AMDGV_WARN("Failed to context switch to PF for dirtybit query\n");
		return AMDGV_FAILURE;
	}

	query_offset = rounddown(data->query_fb_offset, SEGMENT_SIZE_1M);
	query_end_offset = roundup(query_end_offset, SEGMENT_SIZE_1M);
	query_bytes_size = query_end_offset - query_offset;

	num_segments = query_bytes_size >> SHIFT_1M;
	num_bits = data->dbit_plane_data_size * 8;

	/* check if dbit_plane is big enough */
	if (num_segments > num_bits) {
		AMDGV_ERROR("Dirty Bit: dbit_plane is not big enough\n");
		return AMDGV_FAILURE;
	}

	/* get ffbm mapping list for VF */
	vf_ffbm_map_list = oss_malloc(sizeof(struct amdgv_vf_ffbm_map_list));

	if (!vf_ffbm_map_list) {
		amdgv_put_log(AMDGV_PF_IDX,
			AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			sizeof(struct amdgv_vf_ffbm_map_list));
		ret = AMDGV_FAILURE;
		goto fail;
	}

	amdgv_get_vf_fb_mapping_list(adapt, data->idx_vf, vf_ffbm_map_list, false);

	if (vf_ffbm_map_list->count == 0) {
		AMDGV_ERROR("Dirty Bit: No FFBM blocks for vf%d\n", data->idx_vf);
		ret = AMDGV_FAILURE;
		goto out;
	}

	oss_memset(dbit_buffer, 0, data->dbit_plane_data_size);
	ret = query_dirty_bits_through_ffbm_blocks(adapt, vf_ffbm_map_list,
			query_offset, query_end_offset, dbit_buffer, data->dbit_preserve);

out:
	oss_free(vf_ffbm_map_list);
fail:
	return ret;
}

static const struct amdgv_dirtybit_funcs navi32_db_funcs = {
	.control = navi32_dirtybit_control,
	.gcea_sdp_control = navi32_dirtybit_gcea_sdp_control,
    .query_dirty_page_size = navi32_dirtybit_query_dirty_page_size,
	.query_data = navi32_query_dirtybit_data,
};

int navi32_dirtybit_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	adapt->dirtybit.funcs = &navi32_db_funcs;

	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return ret;

	adapt->dirtybit.query_submission_frame = (uint8_t *)oss_malloc(adapt->sdma.sdma_ring[1].max_dw * 4);

	if (!adapt->dirtybit.query_submission_frame) {
		amdgv_put_log(AMDGV_PF_IDX,
			AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			(adapt->sdma.sdma_ring[1].max_dw * 4));
		ret = AMDGV_FAILURE;
		return ret;
	}

	if (amdgv_dirtybit_sw_init(adapt)) {
		oss_free(adapt->dirtybit.query_submission_frame);
		adapt->dirtybit.query_submission_frame = NULL;
		return AMDGV_FAILURE;
	}

	return ret;
}

int navi32_dirtybit_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->dirtybit.query_submission_frame != NULL)
		oss_free(adapt->dirtybit.query_submission_frame);

	amdgv_dirtybit_free_acc_bits_whole_fb(adapt);

	return 0;
}

void navi32_dirtybit_setup_sdma_hbm_page_size(struct amdgv_adapter *adapt)
{
	uint32_t sdma_hbm_value;

	/* setup HBM page size of 1MB for sdma1 */
	sdma_hbm_value = REG_SET_FIELD(0, SDMA1_HBM_PAGE_CONFIG, PAGE_SIZE_EXPONENT, 2);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_HBM_PAGE_CONFIG), sdma_hbm_value);
}

int navi32_dirtybit_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint8_t i;
	uint32_t gc_value;
	uint32_t mm_value;
	uint32_t total_usable_fb;
	uint32_t pf_fb_size;
	uint64_t bitplane_size = 0;

	pf_fb_size = adapt->array_vf[AMDGV_PF_IDX].fb_size;
	amdgv_gpuiov_get_usable_fb_size(adapt, &total_usable_fb);
	bitplane_size = (total_usable_fb - pf_fb_size) << (SHIFT_1M - SHIFT_256K);
	bitplane_size = roundup(bitplane_size, 8) / 8;

	if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION) {
		for (i = 0; i < MAX_MAM_INSTANCES_NAVI32; i++) {
			navi32_select_mam_instance(adapt, i);

			gc_value = RREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_CTRL2));
			gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_CTRL2, DBIT_PF_CLR_ONLY, 1);
			gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_CTRL2, DBIT_PF_RD_ONLY, 1);
			gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_CTRL2, DBIT_TRACK_SEGMENT, NAVI32_DBIT_TRACK_SEGMENT_1MB);

			WREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_CTRL2), gc_value);

			gc_value = RREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_CTRL2));

			AMDGV_DEBUG("MAM instance %d regGCEA_MAM_CTRL2 = 0x%x\n", i, gc_value);

		}

		/* set grbm_gfx_index back to 0 to ensure other broadcast write is not affected */
		navi32_select_mam_instance(adapt, 0);

		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_MAM_CTRL2));
		mm_value = REG_SET_FIELD(mm_value, DAGB0_MAM_CTRL2, DBIT_PF_CLR_ONLY, 1);
		mm_value = REG_SET_FIELD(mm_value, DAGB0_MAM_CTRL2, DBIT_PF_RD_ONLY, 1);
		mm_value = REG_SET_FIELD(mm_value, DAGB0_MAM_CTRL2, DBIT_TRACK_SEGMENT, NAVI32_DBIT_TRACK_SEGMENT_1MB);

		WREG32(SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_MAM_CTRL2), mm_value);
		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_MAM_CTRL2));
		AMDGV_DEBUG("MAM regDAGB0_MAM_CTRL2 = 0x%x\n", mm_value);

		navi32_dirtybit_setup_sdma_hbm_page_size(adapt);

		adapt->dirtybit.gc_dirty_bitplane = amdgv_memmgr_alloc(
			&adapt->memmgr_pf, bitplane_size, MEM_GC_DIRTY_BIT_PLANE);
		if (adapt->dirtybit.gc_dirty_bitplane == NULL)
			goto gc_bitplane_fail;

		adapt->dirtybit.mm_dirty_bitplane = amdgv_memmgr_alloc(
			&adapt->memmgr_pf, bitplane_size, MEM_MM_DIRTY_BIT_PLANE);
		if (adapt->dirtybit.mm_dirty_bitplane == NULL)
			goto mm_bitplane_fail;

		adapt->dirtybit.dirty_page_size = SEGMENT_SIZE_1M;

		ret = amdgv_dirtybit_control(adapt, true);
		if (ret)
			goto fail;
	}
	return ret;

fail:
	amdgv_memmgr_free(adapt->dirtybit.mm_dirty_bitplane);
	adapt->dirtybit.mm_dirty_bitplane = NULL;
mm_bitplane_fail:
	amdgv_memmgr_free(adapt->dirtybit.gc_dirty_bitplane);
	adapt->dirtybit.gc_dirty_bitplane = NULL;
gc_bitplane_fail:
	return AMDGV_FAILURE;
}

int navi32_dirtybit_hw_fini(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return 0;

	amdgv_dirtybit_hw_fini(adapt);

	if (adapt->dirtybit.gc_dirty_bitplane != NULL)
		amdgv_memmgr_free(adapt->dirtybit.gc_dirty_bitplane);

	if (adapt->dirtybit.mm_dirty_bitplane != NULL)
		amdgv_memmgr_free(adapt->dirtybit.mm_dirty_bitplane);

	return 0;
}

struct amdgv_init_func navi32_dirtybit_func = {
	.name = "navi32_dirtybit_func",
	.sw_init = navi32_dirtybit_sw_init,
	.sw_fini = navi32_dirtybit_sw_fini,
	.hw_init = navi32_dirtybit_hw_init,
	.hw_fini = navi32_dirtybit_hw_fini,
};


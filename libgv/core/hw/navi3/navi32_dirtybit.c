/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE.
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
	uint8_t i;
	uint32_t gc_value = 0;
	uint32_t mm_value = 0;
	int wait_ret;
	int ret = 0;

	segment >>= SHIFT_256K;

	/* query */
	if (query_type == NV32_DBIT_QUERY_GC_MM || query_type == NV32_DBIT_QUERY_GC) {
		/* wait for GC MAM 0 query ready */
		navi32_select_mam_instance(adapt, 0);

		/* wait for a query ready */
		wait_ret = amdgv_wait_for_register(
			adapt, SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_STATUS),
			REG_FIELD_MASK(GCEA_MAM_STATUS, DBIT_QUERY_RDY),
			REG_FIELD_MASK(GCEA_MAM_STATUS, DBIT_QUERY_RDY),
			AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ,
			AMDGV_WAIT_FLAG_FORCE_DELAY);

		if (wait_ret) {
			ret = AMDGV_FAILURE;
			goto out;
		}

		/* query GC MAM instances */
		for (i = 0; i < MAX_MAM_INSTANCES_NAVI32; i++) {
			navi32_select_mam_instance(adapt, i);
			gc_value = RREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_DBIT_QUERY));
			/* segment is 256-k aligned memory segment pysical address */
			gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_DBIT_QUERY, QUERY_ADDR, segment);
			gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_DBIT_QUERY, QUERY_EN, 1);

			if (dbit_preserve)
				gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_DBIT_QUERY, DBIT_PRESERVE, 1);
			else
				gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_DBIT_QUERY, DBIT_PRESERVE, 0);

			WREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_DBIT_QUERY), gc_value);
		}
	}

	if (query_type == NV32_DBIT_QUERY_GC_MM || query_type == NV32_DBIT_QUERY_MM) {
		/* wait MM MAM query ready */
		wait_ret = amdgv_wait_for_register(
			adapt, SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_MAM_STATUS),
			REG_FIELD_MASK(DAGB0_MAM_STATUS, DBIT_QUERY_RDY),
			REG_FIELD_MASK(DAGB0_MAM_STATUS, DBIT_QUERY_RDY),
			AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ,
			AMDGV_WAIT_FLAG_FORCE_DELAY);

		if (wait_ret) {
			ret = AMDGV_FAILURE;
			goto out;
		}

		/* query MM MAM */
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

	/* read result */
	if (query_type == NV32_DBIT_QUERY_GC_MM || query_type == NV32_DBIT_QUERY_GC) {
		/* wait for GC MAM 0 query ready */
		navi32_select_mam_instance(adapt, 0);

		/* wait for a query ready */
		wait_ret = amdgv_wait_for_register(
			adapt, SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_STATUS),
			REG_FIELD_MASK(GCEA_MAM_STATUS, DBIT_QUERY_RDY),
			REG_FIELD_MASK(GCEA_MAM_STATUS, DBIT_QUERY_RDY),
			AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ,
			AMDGV_WAIT_FLAG_FORCE_DELAY);

		if (wait_ret) {
			ret = AMDGV_FAILURE;
			goto out;
		}

		do {
			gc_value = RREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_STATUS));
		} while (!(REG_GET_FIELD(gc_value, GCEA_MAM_STATUS, DBIT_QUERY_RDY)));

		gc_value = RREG32(SOC15_REG_OFFSET(GC, 0, regGCEA_MAM_STATUS));
		gc_value = REG_GET_FIELD(gc_value, GCEA_MAM_STATUS, DBIT_QUERY_DIRTY);
	}

	if (query_type == NV32_DBIT_QUERY_GC_MM || query_type == NV32_DBIT_QUERY_MM) {
		/* wait MM MAM query ready */
		wait_ret = amdgv_wait_for_register(
			adapt, SOC15_REG_OFFSET(MMHUB, 0, regDAGB0_MAM_STATUS),
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

/* get the address output SPA that we want to query dirty bit */
static uint64_t navi32_get_query_ffbm_spa(struct ffbm_map_entry entry, uint64_t offset_input)
{
	uint64_t address_output = 0x0;
	uint64_t offset_temp = entry.gpa;

	/* Check if the input offset is in the range of the current block */
	if ((offset_input >= offset_temp)  &&
		(offset_input < offset_temp + entry.size)) {
		/* Calculate the spa using the spa base of the block and the offset */
		address_output = entry.spa + (offset_input - offset_temp);
	}

	return address_output;
}

/* manual driver query and update result dbit */
static int navi32_query_spa_update_dbit(struct amdgv_adapter *adapt, uint64_t query_spa, uint32_t temp_query_bits,
			uint32_t prev_leftover_bits, bool dbit_preserve, uint8_t *dbit_data_ptr, enum NV32_DBIT_QUERY query_type,
			uint32_t *leftover_bits_ret)
{
	uint8_t dbit_results = 0;
	uint8_t prev_leftover_dbit_results = *dbit_data_ptr;
	uint8_t *dbit_data_ptr_temp = dbit_data_ptr;
	uint32_t leftover_bits = (temp_query_bits + prev_leftover_bits) % 8;
	bool is_segment_dirty = false;
	int ret = 0;
	int j = 0;
	/* Query the segments in the current FFBM block*/
	/* start at one for easy 8 bit mod */
	for (j = prev_leftover_bits + 1; j <= temp_query_bits + prev_leftover_bits ; j++) {
		ret = navi32_is_segment_dirty(adapt, query_spa, dbit_preserve, query_type, &is_segment_dirty);

		if (ret)
			goto out;

		if (is_segment_dirty)
			dbit_results |= 0x80;

		if (j % 8 == 0) {

			if (j == 8 && prev_leftover_bits) {
				dbit_results = dbit_results | prev_leftover_dbit_results;
			}

			/* write into dbit plane */
			oss_memcpy(dbit_data_ptr_temp, &dbit_results, sizeof(uint8_t));
			dbit_data_ptr_temp++;
			dbit_results = 0;
		} else {
			dbit_results >>= 1;
		}

		query_spa += SEGMENT_SIZE_1M;
	}

	/* handle leftover dbit_results that is less than 8 bit */
	if (leftover_bits) {

		if (prev_leftover_bits && (temp_query_bits + prev_leftover_bits) < 8) {
			dbit_results >>= (8 - prev_leftover_bits - temp_query_bits - 1);
			dbit_results = dbit_results | prev_leftover_dbit_results;
		} else {
			/* copy last byte of dbit result*/
			dbit_results >>= (8 - leftover_bits - 1); /*  minus 1 cause the for loop will shift one more */
		}
		oss_memcpy(dbit_data_ptr_temp, &dbit_results, sizeof(uint8_t));
	}

	*leftover_bits_ret = leftover_bits;

out:
	return ret;
}

static int navi32_dirtybit_query_data_internal(struct amdgv_adapter *adapt,
				struct amdgv_query_dirty_bit_data *data, enum NV32_DBIT_QUERY query_type)
{
	int ret = 0;
	uint64_t offset_input = data->query_fb_offset;
	uint64_t query_spa = 0;
	uint64_t leftover_query_size = data->query_size;
	uint8_t *temp_dbit_data_ptr = data->dbit_plane_data_buffer;
	uint64_t temp_query_size = 0;
	uint32_t temp_query_bits = 0;
	uint32_t temp_query_bytes_used = 0;
	uint32_t prev_leftover_bits = 0;
	uint32_t i = 0;
	/* get ffbm mapping list for VF */
	struct amdgv_vf_ffbm_map_list *vf_ffbm_map_list = oss_malloc(sizeof(struct amdgv_vf_ffbm_map_list));

	if (!vf_ffbm_map_list) {
		amdgv_put_error(AMDGV_PF_IDX,
			AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			sizeof(struct amdgv_vf_ffbm_map_list));
		ret = AMDGV_FAILURE;
		goto fail;
	}

	amdgv_get_vf_fb_mapping_list(adapt, data->idx_vf, vf_ffbm_map_list, true);

	/* Find the valid FFBM map list */
	if (vf_ffbm_map_list->count != 0) {
		for (i = 0; i < vf_ffbm_map_list->count; i++) {
			/* Adjust offset_input to TMR block size */
			if (vf_ffbm_map_list->entry[i].gpa == MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET)) {
				offset_input += vf_ffbm_map_list->entry[i].size;
				continue;
			}

			/* Check if there's valid spa to query */
			query_spa = navi32_get_query_ffbm_spa(vf_ffbm_map_list->entry[i], offset_input);
			if (query_spa != 0x0) {
				temp_query_size = (leftover_query_size < vf_ffbm_map_list->entry[i].size) ?
									leftover_query_size : vf_ffbm_map_list->entry[i].size;
				temp_query_bits = temp_query_size >> SHIFT_1M;

				/* Query the segments in the current FFBM block */
				ret = navi32_query_spa_update_dbit(adapt, query_spa, temp_query_bits,
									prev_leftover_bits, data->dbit_preserve, temp_dbit_data_ptr,
									query_type, &prev_leftover_bits);

				if (ret)
					goto out;

				temp_query_bytes_used = temp_query_bits >> 3;
				temp_dbit_data_ptr += temp_query_bytes_used;
				leftover_query_size -= temp_query_size;
				offset_input += temp_query_size;
			}

			if (leftover_query_size == 0x0)
				break;
		}
		/* Handle corner case: The first 2MB reserved block is always clean.
		* The 2MB reserve block starts at the beginning of VF's gpa.
		* Check if the queried range includes the 2MB reserved block.
		*/
		if (data->query_fb_offset < MBYTES_TO_BYTES(2)) {
			uint8_t temp_dbit = 0;
			temp_dbit_data_ptr = data->dbit_plane_data_buffer;

			if (data->query_fb_offset == 0) {
				/* query range includes the whole 2MB, so first 2 bits are clean */
				temp_dbit = 0xFC;
			} else {
				/* query range includes the the second half of 2MB, so first bit is clean */
				temp_dbit = 0xFE;
			}

			*temp_dbit_data_ptr = *temp_dbit_data_ptr & temp_dbit;

		}
	} else {
		AMDGV_ERROR("Dirty Bit: No FFBM blocks for vf%d\n", data->idx_vf);
		ret = AMDGV_FAILURE;
	}

out:
	oss_free(vf_ffbm_map_list);
fail:
	return ret;
}

#ifdef PURE_DRIVER_QUERY_DBIT
/* driver dirty bit query */
static int navi32_dirtybit_query_data(struct amdgv_adapter *adapt,
				struct amdgv_query_dirty_bit_data *data)
{
	struct amdgv_vf_device *vf_dev = &adapt->array_vf[data->idx_vf];
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

	/* calculate how many segments to cover query size compare to the dbit_plane size */
	uint64_t num_segments = roundup(data->query_size, SEGMENT_SIZE_1M);
	num_segments >>= SHIFT_1M;

	uint64_t num_bits = data->dbit_plane_data_size * 8;
	/* query_addr_basis should be already page size aligned */
	uint64_t query_addr = SEGMENT_SIZE_1M_ALIGN(data->query_fb_offset);

	/* check if dbit_plane is big enough */
	if (num_segments > num_bits) {
		AMDGV_ERROR("Dirty Bit: dbit_plane is not big enough\n");
		return AMDGV_FAILURE;
	}

	return navi32_dirtybit_query_data_internal(adapt, data, NV32_DBIT_QUERY_GC_MM);
}

#endif

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
	r = amdgv_fence_wait_polling(ring, seq, AMDGV_GFX_MAX_USEC_TIMEOUT);
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

static int navi32_dirtybit_query_replaced_pages(struct amdgv_adapter *adapt,  struct amdgv_query_dirty_bit_data *data)
{
	struct amdgv_vf_device *vf_dev = &adapt->array_vf[data->idx_vf];
	struct ffbm_map_entry *cur_pteb;
	int ret = 0;
	uint8_t dbit_results = 0;
	uint64_t vf_spa = 0;
	uint8_t *dbit_data_ptr = data->dbit_plane_data_buffer;
	int64_t query_addr = SEGMENT_SIZE_1M_ALIGN(data->query_fb_offset);
	bool is_segment_dirty = false;
	uint32_t i;
	uint32_t j;
	uint64_t first_page_pos;
	uint32_t number_of_pages_to_overwrite;
	uint32_t bit_pos;
	uint32_t byte_pos;
	uint8_t dbit_mask;
	/* get ffbm mapping list for VF */
	struct amdgv_vf_ffbm_map_list *vf_ffbm_map_list = oss_malloc(sizeof(struct amdgv_vf_ffbm_map_list));

	if (!vf_ffbm_map_list) {
		amdgv_put_error(AMDGV_PF_IDX,
			AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			sizeof(struct amdgv_vf_ffbm_map_list));
		ret = AMDGV_FAILURE;
		goto fail;
	}

	amdgv_get_vf_fb_mapping_list(adapt, data->idx_vf, vf_ffbm_map_list, false);

	for (i = 1; i < vf_ffbm_map_list->count; i++) {
		/* skips first block as it is 2MB reserve block */
		if (vf_ffbm_map_list->entry[i].size > AMDGV_FFBM_PAGE_SIZE(adapt->ffbm.default_fragment)) {
			vf_spa = vf_ffbm_map_list->entry[i].spa - (vf_ffbm_map_list->entry[i].gpa - MBYTES_TO_BYTES(adapt->tmr_size));
			break;
		}
	}

	/* query and fill in any bad pages and update OS's buffer*/
	for (i = 1; i < vf_ffbm_map_list->count; i++) {
		/* skips first block as it is 2MB reserve block, which is handled */
		cur_pteb = &vf_ffbm_map_list->entry[i];
		if ((cur_pteb->spa > vf_spa + MBYTES_TO_BYTES(vf_dev->fb_size_os)) && //check that spa is in the reserved page range
			((cur_pteb->gpa - MBYTES_TO_BYTES(adapt->tmr_size)) < (query_addr + data->query_size)) &&
			((cur_pteb->gpa - MBYTES_TO_BYTES(adapt->tmr_size) + cur_pteb->size) > query_addr)) {

			for (j = 0; j < (cur_pteb->size >> SHIFT_1M) ; j++) { // should be 2MB per reserve page
				ret = navi32_is_segment_dirty(adapt, cur_pteb->spa + j * SEGMENT_SIZE_1M, data->dbit_preserve, NV32_DBIT_QUERY_GC_MM, &is_segment_dirty);

				if (ret)
					goto out;

				if (is_segment_dirty)
					dbit_results |= 0x80;

				dbit_results >>= 1;
			}

			dbit_results >>= (7 - j); // so that the result is in first 2 bits
			/* find the bits position in */
			first_page_pos = 0;
			number_of_pages_to_overwrite = cur_pteb->size >> SHIFT_1M;
			if ((cur_pteb->gpa - MBYTES_TO_BYTES(adapt->tmr_size)) < query_addr) {
				/* handle case where the first bit is out of range */
				first_page_pos = (SEGMENT_SIZE_1M + (cur_pteb->gpa - MBYTES_TO_BYTES(adapt->tmr_size))) - query_addr;
				/* discard the first bit of dbit_results */
				number_of_pages_to_overwrite -= 1;
				dbit_results >>= 1;
			} else {
				first_page_pos = ((cur_pteb->gpa - MBYTES_TO_BYTES(adapt->tmr_size)) - query_addr);
			}
			if ((cur_pteb->gpa - MBYTES_TO_BYTES(adapt->tmr_size) + cur_pteb->size) > (query_addr + data->query_size)) {
				/* handle case where the second bit is out of query range */
				dbit_results &= ~(0b10);
				number_of_pages_to_overwrite -= 1;
			}
			bit_pos = first_page_pos >> SHIFT_1M;
			byte_pos = bit_pos / 8;
			dbit_data_ptr = data->dbit_plane_data_buffer;
			dbit_data_ptr += byte_pos;
			dbit_mask = number_of_pages_to_overwrite == 1 ? 0x1 : 0x3;
			dbit_mask <<= (bit_pos % 8);
			*dbit_data_ptr &= ~dbit_mask;
			*dbit_data_ptr |= (dbit_results << (bit_pos % 8));

			/* corner case, the 2 bits are in different byte position */
			if ((bit_pos % 8) == 7 &&
				(number_of_pages_to_overwrite == 2) &&
				(byte_pos + 1 < data->dbit_plane_data_size)) {
				dbit_data_ptr++;
				/* clear out first bit */
				*dbit_data_ptr &= (uint8_t) ~0x1;
				*dbit_data_ptr |= (dbit_results >> 1);
			}
		}
	}

out:
	oss_free(vf_ffbm_map_list);

fail:
	return ret;
}

/* hybrid query: sdma poll from GC and driver query from MMhub*/
static int navi32_dirtybit_query_data_hybrid(struct amdgv_adapter *adapt,
				struct amdgv_query_dirty_bit_data *data)
{
	struct amdgv_vf_device *vf_dev = &adapt->array_vf[data->idx_vf];
	int ret = 0;
	uint8_t *dbit_data_ptr = data->dbit_plane_data_buffer;
	uint64_t num_segments = 0;
	uint64_t num_bits = 0;
	int64_t query_addr = 0;
	uint64_t vf_spa = 0;
	uint8_t *dbit_gc = NULL;
	struct nv32_poll_dbit_write_mem sdma_data = {0};
	uint32_t sdma_dbit_plane_size  = 0;
	uint32_t i;
	uint8_t temp_dbit = 0;
	struct amdgv_vf_ffbm_map_list *vf_ffbm_map_list = NULL;

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

	num_segments = roundup(data->query_size, SEGMENT_SIZE_1M);
	num_segments >>= SHIFT_1M;

	num_bits = data->dbit_plane_data_size * 8;
	query_addr = SEGMENT_SIZE_1M_ALIGN(data->query_fb_offset);

	/* check if dbit_plane is big enough */
	if (num_segments > num_bits) {
		AMDGV_ERROR("Dirty Bit: dbit_plane is not big enough\n");
		return AMDGV_FAILURE;
	}

	/* get ffbm mapping list for VF */
	vf_ffbm_map_list = oss_malloc(sizeof(struct amdgv_vf_ffbm_map_list));

	if (!vf_ffbm_map_list) {
		amdgv_put_error(AMDGV_PF_IDX,
			AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			sizeof(struct amdgv_vf_ffbm_map_list));
		ret = AMDGV_FAILURE;
		goto fail;
	}

	amdgv_get_vf_fb_mapping_list(adapt, data->idx_vf, vf_ffbm_map_list, false);

	if (vf_ffbm_map_list->count != 0) {
		/* Get a big ffbm pte block that is more than 2MB (non-replaced page) from the mapping list.
		* Use the SPA and GPA of this block to get the SPA of the VF FB start.
		*/
		for (i = 1; i < vf_ffbm_map_list->count; i++) {
			/* skips first block as it is 2MB reserve block */
			if (vf_ffbm_map_list->entry[i].size > AMDGV_FFBM_PAGE_SIZE(adapt->ffbm.default_fragment)) {
				vf_spa = vf_ffbm_map_list->entry[i].spa - (vf_ffbm_map_list->entry[i].gpa - MBYTES_TO_BYTES(adapt->tmr_size));
				break;
			}
		}
	} else {
		AMDGV_ERROR("Dirty Bit: No FFBM blocks for vf%d\n", data->idx_vf);
		ret = AMDGV_FAILURE;
		goto out;
	}

	/* Query the contiguous segment starting at VF FB SPA + query_fb_offset */
	sdma_data.query_addr = vf_spa + query_addr + adapt->mc_fb_loc_addr;
	sdma_data.number_of_pages = num_segments;

	sdma_data.clear_dbit = data->dbit_preserve ? 0 : 1;
	sdma_data.gc_destination_addr = amdgv_memmgr_get_gpu_addr(adapt->dirtybit.gc_dirty_bitplane);
	sdma_data.mm_destination_addr = amdgv_memmgr_get_gpu_addr(adapt->dirtybit.mm_dirty_bitplane);

	if (navi32_dirtybit_sdma_poll_dbit(adapt, &sdma_data)) {
		AMDGV_ERROR("Dirty Bit: SDMA poll dbit failed\n");
		ret = AMDGV_FAILURE;
		goto out;
	}

	/* Manual query from mmhub */
	if (navi32_dirtybit_query_data_internal(adapt, data, NV32_DBIT_QUERY_MM)) {
		AMDGV_ERROR("Dirty Bit: query mmhub dbit failed\n");
		ret = AMDGV_FAILURE;
		goto out;
	}

	/* OR the GC result and MM result (in dbit_data_ptr) into OS's buffer */
	dbit_gc = (uint8_t *)amdgv_memmgr_get_cpu_addr(adapt->dirtybit.gc_dirty_bitplane);
	sdma_dbit_plane_size = roundup(sdma_data.number_of_pages, 8) / 8;

	for (i = 1; i <= sdma_dbit_plane_size; i++) {
		*dbit_data_ptr = *dbit_gc | *dbit_data_ptr;
		dbit_data_ptr++;
		dbit_gc++;
	}

	/* clean up gc_dbit */
	dbit_gc = (uint8_t *)amdgv_memmgr_get_cpu_addr(adapt->dirtybit.gc_dirty_bitplane);
	oss_memset(dbit_gc, 0, sdma_dbit_plane_size);

	/* Handle corner case: The first 2MB reserved block is always clean.
	* The 2MB reserve block starts at the beginning of VF's gpa.
	* Check if the queried range includes the 2MB reserved block.
	*/
	if (data->query_fb_offset < MBYTES_TO_BYTES(2)) {
		temp_dbit = 0;
		dbit_data_ptr = data->dbit_plane_data_buffer;

		if (data->query_fb_offset == 0) {
			/* query range includes the whole 2MB, so first 2 bits are clean */
			temp_dbit = 0xFC;
		} else {
			/* query range includes the the second half of 2MB, so first bit is clean */
			temp_dbit = 0xFE;
		}

		*dbit_data_ptr = *dbit_data_ptr & temp_dbit;
	}

	/* query dbit for replaced pages */
	ret = navi32_dirtybit_query_replaced_pages(adapt, data);

out:
	oss_free(vf_ffbm_map_list);
fail:
	return ret;
}

static const struct amdgv_dirtybit_funcs navi32_db_funcs = {
	.control = navi32_dirtybit_control,
    .query_dirty_page_size = navi32_dirtybit_query_dirty_page_size,
	.query_data = navi32_dirtybit_query_data_hybrid,
};

int navi32_dirtybit_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	adapt->dirtybit.funcs = &navi32_db_funcs;

	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return ret;

	adapt->dirtybit.query_submission_frame = (uint8_t *)oss_malloc(adapt->sdma.sdma_ring[1].max_dw * 4);

	if (!adapt->dirtybit.query_submission_frame) {
		amdgv_put_error(AMDGV_PF_IDX,
			AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			(adapt->sdma.sdma_ring[1].max_dw * 4));
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int navi32_dirtybit_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->dirtybit.query_submission_frame != NULL)
		oss_free(adapt->dirtybit.query_submission_frame);
	return 0;
}

int navi32_dirtybit_hw_init(struct amdgv_adapter *adapt)
{
	uint8_t i;
	uint32_t gc_value;
	uint32_t mm_value;
	uint32_t sdma_hbm_value;
	uint32_t total_usable_fb;
	uint32_t pf_fb_size;
	uint64_t bitplane_size = 0;
	int ret = 0;

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

		/* setup HBM page size of 1MB for sdma1 */
		sdma_hbm_value = REG_SET_FIELD(0, SDMA1_HBM_PAGE_CONFIG, PAGE_SIZE_EXPONENT, 2);
		WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_HBM_PAGE_CONFIG), sdma_hbm_value);

		adapt->dirtybit.gc_dirty_bitplane = amdgv_memmgr_alloc(
			&adapt->memmgr_pf, bitplane_size, MEM_GC_DIRTY_BIT_PLANE);

		if (adapt->dirtybit.gc_dirty_bitplane == NULL)
			ret = AMDGV_FAILURE;

		adapt->dirtybit.dirty_page_size = SEGMENT_SIZE_1M;
	}
	return ret;
}

int navi32_dirtybit_hw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->dirtybit.gc_dirty_bitplane != NULL)
		amdgv_memmgr_free(adapt->dirtybit.gc_dirty_bitplane);

	return 0;
}

struct amdgv_init_func navi32_dirtybit_func = {
	.name = "navi32_dirtybit_func",
	.sw_init = navi32_dirtybit_sw_init,
	.sw_fini = navi32_dirtybit_sw_fini,
	.hw_init = navi32_dirtybit_hw_init,
	.hw_fini = navi32_dirtybit_hw_fini,
};


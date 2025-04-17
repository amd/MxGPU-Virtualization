/*
 * Copyright (c) 2017-2024 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_device.h"
#include "amdgv_ras.h"
#include "amdgv_ras_eeprom.h"
#include "amdgv_vfmgr.h"

#define AMDGV_UMC_ALIGNMENT	     512
#define AMDGV_UMC_ALIGN(data, align) (((data) + (align)-1) & ~(align - 1))

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

void amdgv_umc_badpages_count_read(struct amdgv_adapter *adapt, int *bp_cnt)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;

	if (!data)
		*bp_cnt = 0;
	else
		*bp_cnt = data->count;
}

int amdgv_umc_get_badpages_record(struct amdgv_adapter *adapt, uint32_t index, void *record)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	struct eeprom_table_record *bp_record = (struct eeprom_table_record *)record;
	int ret = 0;

	if (!data)
		return ret;

	if (data->count && index <= data->count)
		oss_memcpy(bp_record, &data->bps[index], sizeof(struct eeprom_table_record));

	if (index > data->count)
		ret = AMDGV_FAILURE;

	return ret;
}

/*
 * caller need free bps.
 */
int amdgv_umc_badpages_read(struct amdgv_adapter *adapt, void **bp, unsigned int *count)
{
	struct ras_badpage **bps = (struct ras_badpage **)bp;
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	int i = 0;
	int ret = 0;

	oss_mutex_lock(adapt->ecc.recovery_lock);
	if (!data || data->count == 0) {
		*bps = NULL;
		*count = 0;
		goto out;
	}

	*bps = oss_malloc(sizeof(struct ras_badpage) * data->count);
	if (!*bps) {
		*count = 0;
		ret = AMDGV_FAILURE;
		goto out;
	}

	for (; i < data->count; i++) {
		(*bps)[i] = (struct ras_badpage){
			.bp = data->bps[i].retired_page,
			.size = AMDGV_GPU_PAGE_SIZE,
			.flags = 0,
		};

		/*
		 * The definition of flags:
		 *
		 * 0: reserved, this gpu page is reserved and not able to use.
		 *
		 * 1: pending for reserve, this gpu page is marked as bad, will
		 * be reserved in next window of page_reserve.
		 *
		 * 2: unable to reserve. this gpu page can't be reserved due to
		 * some reasons.
		 */
		if (data->last_reserved <= i)
			(*bps)[i].flags = 1;
		else if (data->bps_mem[i] == NULL)
			(*bps)[i].flags = 2;
	}

	*count = data->count;
out:
	oss_mutex_unlock(adapt->ecc.recovery_lock);
	return ret;
}

static bool amdgv_umc_is_dup_record(struct amdgv_adapter *adapt,
			struct eeprom_table_record *record, bool from_eeprom)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	uint64_t pa_pfn;
	int i;

	if (from_eeprom) {
		if (adapt->umc.funcs && adapt->umc.funcs->eeprom_record_to_soc_pa) {
			adapt->umc.funcs->eeprom_record_to_soc_pa(adapt, record, &pa_pfn);
			record->retired_page = pa_pfn;
		}
		return false;
	}

	for (i = 0; i < data->rom_data.count; i++) {
		if (data->rom_data.bps[i].retired_page == record->retired_page)
			return true;
	}

	return false;
}

/* Cache the raw data loaded from eeprom and newly detected data
 * that needs to be saved to eeprom.
 * For the record type of base address, there is no need to extend
 * to bad page addresses.
 */
static int amdgv_umc_update_eeprom_rom_data(struct amdgv_adapter *adapt,
		struct eeprom_table_record *bps)
{
	struct eeprom_data_record *data = &adapt->ecc.eh_data->rom_data;

	/* grow bad page buffer if the pending total bad pages is greater than bad page buffer size */
	if (amdgv_umc_update_bp_buff(adapt, &data->bps, (uint32_t)(data->count + 1), &data->bps_cap)) {
		AMDGV_ERROR("Failed to update eeprom rom bad page buffer!\n");
		return AMDGV_FAILURE;
	}

	oss_memcpy(&data->bps[data->count], bps, sizeof(struct eeprom_table_record));
	data->count++;

	return 0;
}

/* For the record type of base address, the base address needs to
 * be extended to the bad page addresses before caching.
 */
static int amdgv_umc_update_eeprom_ram_data(struct amdgv_adapter *adapt,
				struct eeprom_table_record *bps)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	uint64_t page_pfn[16] = {0};
	bool expand_to_pages = false;
	int count, j;

	count = 1;
	/* Expand the bad pages in a row */
	if (adapt->umc.funcs && adapt->umc.funcs->eeprom_record_to_pages) {
		count = adapt->umc.funcs->eeprom_record_to_pages(adapt, bps, page_pfn, ARRAY_SIZE(page_pfn));
		if (count <= 0) {
			AMDGV_ERROR("Failed to transfer record to pages!\n");
			return AMDGV_FAILURE;
		}
		expand_to_pages = true;
	}

	/* grow bad page buffer if the pending total bad pages is greater than bad page buffer size */
	if (amdgv_umc_update_bp_buff(adapt, &data->bps, (uint32_t)(data->count + count), &data->bps_cap)) {
		AMDGV_ERROR("Failed to update eeprom ram bad page buffer!\n");
		return AMDGV_FAILURE;
	}

	for (j = 0; j < count; j++) {
		if (expand_to_pages)
			bps->retired_page = page_pfn[j];
		oss_memcpy(&data->bps[data->count], bps, sizeof(struct eeprom_table_record));
		data->count++;
	}

	amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_BAD_PAGE_APPEND,
		AMDGV_ERROR_32_32(count, data->count));

	return 0;
}

int amdgv_umc_add_bad_pages(struct amdgv_adapter *adapt,
				   struct eeprom_table_record *bps, int pages, bool from_eeprom)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	int ret = 0;
	int i = 0;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return 0;

	if (!bps || pages <= 0)
		return 0;

	if (!data)
		return 0;

	oss_mutex_lock(adapt->ecc.recovery_lock);

	for (i = 0; i < pages; i++) {
		if (data->count >= adapt->eeprom_control.max_record_num) {
			AMDGV_ERROR("Bad page record count exceeds the max limit! Dropping new bad pages\n");
			ret = AMDGV_FAILURE;
			goto out;
		}

		if (amdgv_umc_is_dup_record(adapt, &bps[i], from_eeprom))
			continue;

		ret = amdgv_umc_update_eeprom_rom_data(adapt, &bps[i]);
		if (ret)
			goto out;

		/* Since the base address translated from the legacy page
		   record in the eeprom is the same, only the first address
		   needs to be extended. */
		if (data->last_retired_pfn == bps[i].retired_page)
			continue;

		data->last_retired_pfn = bps[i].retired_page;

		ret = amdgv_umc_update_eeprom_ram_data(adapt, &bps[i]);
		if (ret)
			goto out;
	}

out:
	oss_mutex_unlock(adapt->ecc.recovery_lock);

	return ret;
}

/*
 * write error record array to eeprom, the function should be
 * protected by recovery_lock
 */
int amdgv_umc_save_bad_pages(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	struct amdgv_ras_eeprom_control *control;
	int save_count;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return 0;

	if (!data)
		return 0;

	control = &adapt->eeprom_control;
	save_count = data->rom_data.count - control->num_recs;
	/* only new entries are saved */
	if (save_count > 0) {
		if (amdgv_ras_eeprom_process_records(
			    adapt, control, &data->rom_data.bps[control->num_recs], true, save_count)) {
			AMDGV_ERROR("Failed to save EEPROM table data!\n");
			return AMDGV_FAILURE;
		}

		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_EEPROM_APPEND,
				AMDGV_ERROR_32_32(save_count, control->num_recs));

		if (adapt->pp.pp_funcs->send_hbm_bad_pages_num)
			adapt->pp.pp_funcs->send_hbm_bad_pages_num(adapt,
					adapt->eeprom_control.num_recs);
		if (adapt->pp.pp_funcs->send_hbm_bad_channel_flag &&
				(adapt->update_channel_flag == true)) {
			adapt->pp.pp_funcs->send_hbm_bad_channel_flag(adapt,
					adapt->eeprom_control.bad_channel_bitmap);
			adapt->update_channel_flag = false;
		}
	} else {
		/* Bad Pages may have been saved on different OS.
		 * Need to update the EEPROM header after reservations are performed so
		 * SRIOV specific RMAreasons are saved.
		 */
		if (amdgv_ras_eeprom_process_records(adapt, control, NULL, true, 0)) {
			AMDGV_ERROR("Failed to save EEPROM table data!\n");
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

int amdgv_umc_load_bad_pages(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct amdgv_ras_eeprom_control *control = &adapt->eeprom_control;
	struct eeprom_table_record *bps = NULL;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return 0;

	/* no bad page record, skip eeprom access */
	if (!control->num_recs)
		return ret;

	bps = oss_zalloc(control->num_recs * sizeof(*bps));
	if (!bps)
		return AMDGV_FAILURE;

	if (amdgv_ras_eeprom_process_records(adapt, control, bps, false, control->num_recs)) {
		AMDGV_ERROR("Failed to load EEPROM table records!\n");
		ret = AMDGV_FAILURE;
		goto out;
	}

	adapt->ecc.eh_data->last_retired_pfn = AMDGV_RAS_INV_MEM_PFN;
	if (amdgv_umc_add_bad_pages(adapt, bps, control->num_recs, true))
		ret = AMDGV_FAILURE;

out:
	oss_free(bps);
	return ret;
}

/**
 * amdgv_umc_reload_bp_from_rom
 *  - update ram data from rom data, normally when nps mode change
 *
 *  release pf reserved bp and reset ram data count
 *  reload ram data from rom data
 *  reserve bad page for pf managed memory
 **/
int amdgv_umc_reload_bp_from_rom(struct amdgv_adapter *adapt)
{
	struct eeprom_data_record *rom_data = &adapt->ecc.eh_data->rom_data;
	struct ras_err_handler_data *eh_data = adapt->ecc.eh_data;

	int i, ret = 0;

	/* release pf reserved bp */
	amdgv_umc_release_bad_pages(adapt);

	/**
	 * reset bps for fresh reload from rom data
	 * The allocated size of bps can be larger then
	 * actual used after reload.
	 */
	eh_data->count = 0;
	eh_data->last_retired_pfn = AMDGV_RAS_INV_MEM_PFN;
	amdgv_vfmgr_clean_bp_block_size(adapt);

	/* load from rom data */
	for (i = 0; i < rom_data->count; i++) {
		ret = amdgv_umc_update_eeprom_ram_data(adapt, &rom_data->bps[i]);
		if (ret)
			break;
	}

	if (ret)
		return ret;
	return amdgv_umc_reserve_bad_pages(adapt);
}

/*
 * check if an address belongs to bad page
 *
 * Note: this check is only for umc block
 * TODO: add check for the page in special area, such as VBIOS/CSA/TOC...
 */
bool amdgv_umc_check_bad_page(struct amdgv_adapter *adapt, uint64_t addr)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	int i;
	bool ret = false;
	uint64_t page_addr;

	oss_mutex_lock(adapt->ecc.recovery_lock);
	if (!data)
		goto out;

	page_addr = addr >> AMDGV_GPU_PAGE_SHIFT;
	for (i = 0; i < data->count; i++)
		if (page_addr == data->bps[i].retired_page) {
			AMDGV_ERROR("Address (0x%llx) found in an EEPROM entry as a retired page!\n", addr);
			ret = true;
			goto out;
		}

out:
	oss_mutex_unlock(adapt->ecc.recovery_lock);
	return ret;
}

static bool amdgv_umc_is_bp_in_range(uint64_t err_addr, uint64_t offset, uint64_t size)
{
	if ((err_addr >= offset) && (err_addr < offset + size))
		return true;
	else
		return false;
}

static uint32_t amdgv_umc_calc_retired_page_vf_slot(struct amdgv_adapter *adapt,
						    uint64_t err_addr)
{
	struct amdgv_vf_device *entry;
	uint32_t vf_slot = AMDGV_PF_IDX;
	uint32_t idx_vf;
	uint64_t fb_offset, fb_size;
	uint64_t err_addr_gpu;

	if (adapt->ffbm.enabled) {
		err_addr_gpu = amdgv_ffbm_spa_to_gpa(adapt, err_addr, &vf_slot);
	} else {
		for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
			entry = &adapt->array_vf[idx_vf];
			fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
			fb_size = MBYTES_TO_BYTES(entry->fb_size);

			if (amdgv_umc_is_bp_in_range(err_addr, fb_offset, fb_size)) {
				vf_slot = idx_vf;
				break;
			}
		}
	}

	return vf_slot;
}

static bool amdgv_umc_check_bp_in_critical_region(struct amdgv_adapter *adapt,
						  uint64_t err_addr, uint32_t idx_vf)
{
	struct amdgv_vf_device *entry;
	struct psp_local_memory *tmr_mem;
	struct amdgv_memmgr_mem *csa_fb_mem;
	uint64_t vf_fb_offset, vf_fb_size, vf_real_fb_size;
	uint64_t fb_offset, fb_size, total_avail_fb, fb_gpu_addr;
	uint32_t total_avail_fb_in_mb;
	uint32_t i;
	uint64_t err_addr_gpa;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return false;
	/*
	 * with FFBM enabled,
	 * skip PF and VF fb check and check other regions first
	 * (there is no change to non-managed region and TMR with FFBM)
	 * no critical region in VF fb, only PF fb need to be checked after
	 */
	if (!adapt->ffbm.enabled) {
		entry = &adapt->array_vf[idx_vf];
		vf_fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
		vf_fb_size = MBYTES_TO_BYTES(entry->fb_size);
		vf_real_fb_size = MBYTES_TO_BYTES(entry->real_fb_size);

		if (idx_vf == AMDGV_PF_IDX) {
			/* check pf memory region */
			if (amdgv_umc_is_bp_in_range(err_addr, vf_fb_offset, vf_fb_size)) {
				adapt->bp_msg_type = AMDGV_BP_MSG_IN_PF_FB;
				return (adapt->flags & AMDGV_FLAG_USE_PF);
			}

			if (amdgv_umc_is_bp_in_range(err_addr, adapt->memmgr_pf.offset, adapt->memmgr_pf.size)) {
				adapt->bp_msg_type = AMDGV_BP_MSG_IN_PF_FB;
				return (adapt->flags & AMDGV_FLAG_USE_PF);
			}

		} else {
			/* we may get multiple ECCs at the same time, and the ecc address and ecc vf may
			* be not in sequence. So check all the VF for critical range, if found, set
			* to RMA status */
			for (i = 0; i < adapt->num_vf; i++) {
				entry = &adapt->array_vf[i];
				vf_fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
				vf_fb_size = MBYTES_TO_BYTES(entry->fb_size);
				vf_real_fb_size = MBYTES_TO_BYTES(entry->real_fb_size);

				/* check bp whether in vbios and data exchange region */
				fb_offset = vf_fb_offset +
					    KBYTES_TO_BYTES(AMD_SRIOV_MSG_VBIOS_OFFSET);
				fb_size =
					KBYTES_TO_BYTES(AMD_SRIOV_MSG_DATAEXCHANGE_OFFSET_KB) +
					KBYTES_TO_BYTES(AMD_SRIOV_MSG_DATAEXCHANGE_SIZE_KB);

				if (amdgv_umc_is_bp_in_range(err_addr, fb_offset, fb_size)) {
					adapt->bp_msg_type =
						AMDGV_BP_MSG_IN_VF_CRITICAL_REGION;

					return true;
				}

				/* check bp whether in vf ip discovery data block region */
				fb_offset = vf_fb_offset + vf_real_fb_size -
					    AMDGV_IP_DISCOVERY_OFFSET;

				fb_size = AMDGV_IP_DISCOVERY_SIZE;
				if (amdgv_umc_is_bp_in_range(err_addr, fb_offset, fb_size)) {
					adapt->bp_msg_type =
						AMDGV_BP_MSG_IN_VF_CRITICAL_REGION;

					return true;
				}
			}
		}
	}

	amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_avail_fb_in_mb);
	total_avail_fb = (uint64_t)total_avail_fb_in_mb;
	total_avail_fb = MBYTES_TO_BYTES(total_avail_fb);

	/* check bp whether in tmr region */
	tmr_mem = &adapt->psp.tmr_context;
	if (tmr_mem->mem) {
		fb_gpu_addr = amdgv_memmgr_get_gpu_addr(tmr_mem->mem);
		fb_offset = fb_gpu_addr - adapt->memmgr_pf.mc_base;
		fb_size = amdgv_memmgr_get_size(tmr_mem->mem);
		if (amdgv_umc_is_bp_in_range(err_addr, fb_offset, fb_size)) {
			adapt->bp_msg_type = AMDGV_BP_MSG_IN_CRITICAL_REGION;

			return true;
		}
	}

	/* check bp whether in pf ip discovery data block region */
	fb_offset = total_avail_fb - AMDGV_IP_DISCOVERY_OFFSET;
	fb_size = AMDGV_IP_DISCOVERY_SIZE;
	if (amdgv_umc_is_bp_in_range(err_addr, fb_offset, fb_size)) {
		adapt->bp_msg_type = AMDGV_BP_MSG_IN_CRITICAL_REGION;

		return true;
	}

	/* check bp whether in csa region */
	csa_fb_mem = adapt->gpuiov.csa_fb_mem;
	if (csa_fb_mem) {
		fb_offset = total_avail_fb - amdgv_memmgr_get_offset(csa_fb_mem);
		fb_size = amdgv_memmgr_get_size(csa_fb_mem);
		if (amdgv_umc_is_bp_in_range(err_addr, fb_offset, fb_size)) {
			adapt->bp_msg_type = AMDGV_BP_MSG_IN_CRITICAL_REGION;

			return true;
		}
	} else
		AMDGV_ERROR("Private csa fb memory not allocated\n");

	/* check FFBM PF fb region */
	if (adapt->ffbm.enabled) {
		/* when idx_vf is PF, it can be TMR or PF fb
		 * and TMR has already been checked above
		 */
		err_addr_gpa = amdgv_ffbm_spa_to_gpa(adapt, err_addr, &idx_vf);
		if (idx_vf == AMDGV_PF_IDX && err_addr_gpa != AMDGV_FFBM_INVALID_ADDR) {
			adapt->bp_msg_type = AMDGV_BP_MSG_IN_PF_FB;
			/* return true if PF_FB is in use */
			return (adapt->flags & AMDGV_FLAG_USE_PF);
		}
	}

	return false;
}

static bool amdgv_umc_check_bp_in_same_mem_row(struct amdgv_adapter *adapt,
					       uint64_t err_addr_new)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	uint64_t err_addr, mem_row_idx;
	int i;
	bool ret = false;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return false;

	if (adapt->ecc.skip_row_rma)
		return ret;

	mem_row_idx = err_addr_new / AMDGV_GPU_MEM_ROW_SIZE;
	for (i = 0; i < data->last_reserved; i++) {
		err_addr = data->bps[i].retired_page << AMDGV_GPU_PAGE_SHIFT;
		if (mem_row_idx == (err_addr / AMDGV_GPU_MEM_ROW_SIZE))
			ret = true;
	}

	if (ret) {
		adapt->bp_msg_type = AMDGV_BP_MSG_IN_SAME_ROW;
	}

	return ret;
}

static void amdgv_umc_log_bp_errors(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;

	if (!data)
		return;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return;

	switch (adapt->bp_msg_type) {
	case AMDGV_BP_MSG_IN_PF_FB:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_IN_PF_FB,
				AMDGV_ERROR_32_32(data->last_reserved + 1, BAD_PAGE_RECORD_THRESHOLD));
		break;
	case AMDGV_BP_MSG_IN_CRITICAL_REGION:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_IN_CRI_REG,
				AMDGV_ERROR_32_32(data->last_reserved + 1, BAD_PAGE_RECORD_THRESHOLD));
		break;
	case AMDGV_BP_MSG_IN_VF_CRITICAL_REGION:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_IN_VF_CRI,
				AMDGV_ERROR_32_32(data->last_reserved + 1, BAD_PAGE_RECORD_THRESHOLD));
		break;
	case AMDGV_BP_MSG_IN_SAME_ROW:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_IN_SAME_ROW,
				AMDGV_ERROR_32_32(data->last_reserved + 1, BAD_PAGE_RECORD_THRESHOLD));
		break;
	case AMDGV_BP_MSG_RECORD_THRESHOLD_REACHED:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_REACH_THD, BAD_PAGE_RECORD_THRESHOLD);
		break;
	default:
		break;
	}

	return;
}

static bool amdgv_umc_check_bp_threshold(struct amdgv_adapter *adapt, struct ras_err_handler_data *data)
{
	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return false;

	/* RMA at the max count because new entries will be lost. */
	if (data->count >= BAD_PAGE_RECORD_THRESHOLD) {
		adapt->bp_msg_type = AMDGV_BP_MSG_RECORD_THRESHOLD_REACHED;
		return true;
	}

	return false;
}

/* called in gpu recovery/init */
int amdgv_umc_reserve_bad_pages(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	uint64_t err_addr_pf, err_addr_gpu, total_fb;
	int i, ret = 0;
	uint32_t idx_vf, total_fb_in_mb;
	int resv_count = 0;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return ret;

	if (!data)
		return ret;

	oss_mutex_lock(adapt->ecc.recovery_lock);

	if (amdgv_umc_check_bp_threshold(adapt, data)) {
		amdgv_umc_log_bp_errors(adapt);
		goto out;
	}

	resv_count = data->count <= BAD_PAGE_RECORD_THRESHOLD ? data->count : BAD_PAGE_RECORD_THRESHOLD;

	/* reserve vram at driver post stage. */
	for (i = data->last_reserved; i < resv_count; i++) {
		err_addr_pf = data->bps[i].retired_page << AMDGV_GPU_PAGE_SHIFT;
		amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_fb_in_mb);
		total_fb = (uint64_t)total_fb_in_mb;
		/* convert from MB to Byte */
		total_fb = MBYTES_TO_BYTES(total_fb);
		err_addr_gpu = total_fb - err_addr_pf;

		adapt->bp_msg_type = AMDGV_BP_MSG_INVALID;

		if (amdgv_umc_check_bp_in_same_mem_row(adapt, err_addr_pf)) {
			amdgv_umc_log_bp_errors(adapt);
			break;
		}

		idx_vf = amdgv_umc_calc_retired_page_vf_slot(adapt, err_addr_pf);
		if (amdgv_umc_check_bp_in_critical_region(adapt, err_addr_pf, idx_vf)) {
			amdgv_umc_log_bp_errors(adapt);
			break;
		}

		/* There are three cases of reserve error should be ignored:
		 * 1) a ras bad page has been allocated (used by someone);
		 * 2) a ras bad page has been reserved (duplicate error injection
		 *    for one page);
		 * 3) a ras bad page does not fall into memmgr_pf and memmgr_gpu (
		 *    falls into vf region) guest will handle page reserve;
		 */
		if ((err_addr_pf >= adapt->memmgr_pf.offset) &&
		    (err_addr_pf < adapt->memmgr_pf.offset + adapt->memmgr_pf.size)) {
			data->bps_mem[i] = amdgv_memmgr_alloc_align_at(
				&adapt->memmgr_pf, err_addr_pf, AMDGV_GPU_PAGE_SIZE,
				MEM_ECC_BAD_PAGE);
			if (!data->bps_mem[i])
				AMDGV_WARN("Reserve page at 0x%llx in memmgr_pf failed\n",
					   err_addr_pf);
		} else if ((err_addr_gpu >= adapt->memmgr_gpu.offset) &&
			   (err_addr_gpu <
			    adapt->memmgr_gpu.offset + adapt->memmgr_gpu.size)) {
			data->bps_mem[i] = amdgv_memmgr_alloc_align_at(
				&adapt->memmgr_gpu, err_addr_gpu, AMDGV_GPU_PAGE_SIZE,
				MEM_ECC_BAD_PAGE);
			if (!data->bps_mem[i])
				AMDGV_WARN("Reserve page at 0x%llx in memmgr_gpu failed\n",
					   err_addr_gpu);
		} else {
			data->bps_mem[i] = NULL;
		}

		data->last_reserved = i + 1;
	}

out:
	oss_mutex_unlock(adapt->ecc.recovery_lock);

	return ret;
}

/* write bad page record into share memory */
void amdgv_umc_notify_vf_bp_records(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	uint64_t err_addr;
	uint32_t bp_idx_vf;
	struct amdgv_vf_device *entry;
	int i, write_count;

	oss_mutex_lock(adapt->ecc.recovery_lock);
	if (!data) {
		oss_mutex_unlock(adapt->ecc.recovery_lock);
		return;
	}

	amdgv_vfmgr_clean_vf_bp_block_size(adapt, idx_vf);

	write_count = data->count <= BAD_PAGE_RECORD_THRESHOLD ?
					data->count : BAD_PAGE_RECORD_THRESHOLD;

	for (i = 0; i < write_count; i++) {
		err_addr = data->bps[i].retired_page << AMDGV_GPU_PAGE_SHIFT;

		bp_idx_vf = amdgv_umc_calc_retired_page_vf_slot(adapt, err_addr);
		if (!amdgv_umc_check_bp_in_critical_region(adapt, err_addr, idx_vf) &&
		    (idx_vf == bp_idx_vf)) {
			entry = &adapt->array_vf[idx_vf];
			entry->retired_page = data->bps[i].retired_page;

			if (amdgv_vfmgr_update_bp_message(adapt, idx_vf))
				AMDGV_WARN("update bp message failed\n");
		}
	}

	oss_mutex_unlock(adapt->ecc.recovery_lock);
}

/* called when driver unload */
int amdgv_umc_release_bad_pages(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	struct amdgv_memmgr_mem *mem = NULL;
	int i;

	oss_mutex_lock(adapt->ecc.recovery_lock);
	if (!data)
		goto out;

	for (i = data->last_reserved - 1; i >= 0; i--) {
		mem = data->bps_mem[i];
		amdgv_memmgr_free(mem);
		data->bps_mem[i] = NULL;
		data->last_reserved = i;
	}
out:
	oss_mutex_unlock(adapt->ecc.recovery_lock);
	return 0;
}

int amdgv_umc_recovery_sw_init(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data **data = &(adapt->ecc.eh_data);

	*data = oss_zalloc(sizeof(**data));
	if (!*data)
		goto error;

	adapt->ecc.eh_data->last_retired_pfn = AMDGV_RAS_INV_MEM_PFN;
	(*data)->rom_data.bps_cap = BAD_PAGE_RECORD_THRESHOLD;
	(*data)->rom_data.bps = oss_zalloc(AMDGV_UMC_ALIGN(
		(*data)->rom_data.bps_cap * sizeof(*(*data)->rom_data.bps), AMDGV_UMC_ALIGNMENT));

	(*data)->bps_cap = BAD_PAGE_RECORD_THRESHOLD;
	(*data)->bps = oss_zalloc(AMDGV_UMC_ALIGN(
		(*data)->bps_cap * sizeof(*(*data)->bps), AMDGV_UMC_ALIGNMENT));
	(*data)->bps_mem = oss_zalloc(AMDGV_UMC_ALIGN(
		BAD_PAGE_RECORD_THRESHOLD * sizeof(*(*data)->bps_mem), AMDGV_UMC_ALIGNMENT));

	if (!(*data)->bps || !(*data)->bps_mem)
		goto error;

	if (amdgv_ras_eeprom_sw_init(adapt, &(adapt->eeprom_control)))
		goto error;

	return 0;
error:
	AMDGV_ERROR("Failed to initialize ras recovery!\n");
	amdgv_umc_recovery_sw_fini(adapt);
	return AMDGV_FAILURE;
}

int amdgv_umc_recovery_hw_init(struct amdgv_adapter *adapt)
{
	int ret;
	enum amdgv_memory_partition_mode nps_mode;

	/* get fb available size in advance for possible usage in page retirement */
	if (!adapt->gpuiov.total_fb_avail) {
		if (adapt->gpuiov.funcs->get_config_info)
			adapt->gpuiov.funcs->get_config_info(adapt);
	}

	if (in_whole_gpu_reset()) {
		if (oss_atomic_read(adapt->in_ecc_recovery)) {
			if (adapt->gpumon.funcs->ras_report &&
				adapt->gpumon.funcs->ras_report(adapt, (int)PP_RAS_TYPE__FATAL_ERROR)) {
				AMDGV_ERROR("SMU is not responding, unable to report fatal error to SMBUS\n");
			}
		}
		/* Handle the scenario where eeprom has been corrupted after reset.
		 * If driver has already detected an RMA condition, it must mark
		 * GPU as bad in EEPROM again based on the driver cached status. */
		ret = amdgv_ras_eeprom_init(adapt, &(adapt->eeprom_control));
		if (ret)
			goto release;

		if (adapt->nbio.ras &&
			 adapt->nbio.ras->get_curr_memory_partition_mode) {
			ret = adapt->nbio.ras->get_curr_memory_partition_mode(adapt, &nps_mode);
			if (ret) {
				AMDGV_ERROR("Failed to get current nps mode\n");
				goto release;
			}
			if (nps_mode != adapt->ecc.eh_data->nps_mode) {
				AMDGV_INFO("Reload bad page on nps mode change.\n");
				ret = amdgv_umc_reload_bp_from_rom(adapt);
				if (ret) {
					/* TODO: dediacated error code ras routine */
					AMDGV_ERROR("Failed to reload bad pages\n");
					goto release;
				}
				adapt->ecc.eh_data->nps_mode = nps_mode;
			}
		}


		ret = amdgv_umc_save_bad_pages(adapt);
		if (ret)
			goto release;
	} else {
		ret = amdgv_ras_eeprom_init(adapt, &(adapt->eeprom_control));
		if (ret)
			goto free;
	}

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return 0;

release:
	if (ret)
		amdgv_umc_release_bad_pages(adapt);
free:
	if (ret)
		amdgv_ras_eeprom_fini(&adapt->eeprom_control);

	if (amdgv_ras_eeprom_is_gpu_bad(adapt))
		amdgv_device_handle_bad_gpu(adapt);

	return ret;
}

int amdgv_umc_recovery_sw_fini(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;

	/* recovery_init failed to init it, fini is useless */
	if (data) {

		data->count = 0;
		data->last_reserved = 0;
		data->rom_data.count = 0;

		if (data->rom_data.bps) {
			oss_memset(data->bps, 0,
				AMDGV_UMC_ALIGN(data->rom_data.bps_cap * sizeof(*data->rom_data.bps),
					AMDGV_UMC_ALIGNMENT));
			oss_free(data->rom_data.bps);
			data->rom_data.bps = NULL;
		}

		if (data->bps) {
			oss_memset(data->bps, 0,
				AMDGV_UMC_ALIGN(data->bps_cap * sizeof(*data->bps),
						AMDGV_UMC_ALIGNMENT));
			oss_free(data->bps);
			data->bps = NULL;
		}
		if (data->bps_mem) {
			oss_memset(data->bps_mem, 0,
				AMDGV_UMC_ALIGN(BAD_PAGE_RECORD_THRESHOLD * sizeof(*data->bps_mem),
						AMDGV_UMC_ALIGNMENT));
			oss_free(data->bps_mem);
			data->bps_mem = NULL;
		}
		oss_free(data);
		adapt->ecc.eh_data = NULL;
	}

	return 0;
}

int amdgv_umc_recovery_hw_fini(struct amdgv_adapter *adapt)
{
	amdgv_umc_release_bad_pages(adapt);
	amdgv_ras_eeprom_fini(&adapt->eeprom_control);

	return 0;
}

static int amdgv_umc_handle_bad_pages(struct amdgv_adapter *adapt,
		struct ras_err_data *err_data)
{
	int ret = AMDGV_RAS_SUCCESS;

	if (amdgv_umc_add_bad_pages(adapt, err_data->err_addr,
				err_data->err_addr_cnt, false)) {
		AMDGV_WARN("Failed to add ras bad page!\n");
		ret = AMDGV_FAILURE;
	} else if (amdgv_umc_reserve_bad_pages(adapt)) {
		AMDGV_WARN("Failed to reserve ras bad page\n");
		ret = AMDGV_FAILURE;
	} else if (amdgv_umc_save_bad_pages(adapt)) {
		AMDGV_WARN("Failed to save ras bad page\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

/* Retrieve bad pages from EEPROM table, check if they are in
 * critical region and reserve them.
 */
int amdgv_umc_retrieve_bad_pages(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum amdgv_memory_partition_mode nps_mode;

	if (!amdgv_ecc_is_support(adapt, AMDGV_RAS_BLOCK__UMC))
		return 0;

	// If umc is not init, return directly
	if (!adapt->ecc.eh_data)
		return 0;

	if (adapt->eeprom_control.num_recs) {
		ret = amdgv_umc_load_bad_pages(adapt);
		if (ret)
			goto free;
		ret = amdgv_umc_reserve_bad_pages(adapt);
		if (ret)
			goto release;

		/* nps mode is required if memory partitioning supported */
		if (adapt->nbio.ras &&
			adapt->nbio.ras->get_curr_memory_partition_mode) {
			ret = adapt->nbio.ras->get_curr_memory_partition_mode(adapt, &nps_mode);
			if (ret) {
				AMDGV_ERROR("Failed to get current nps mode\n");
				goto release;
			}
			adapt->ecc.eh_data->nps_mode = nps_mode;
		}
		/* This must be called in scenaro where
		 * bad pages were added from another OS/Driver.
		 * LibGV must re-calculate whether reservations
		 * are in a critical region, update the EEPROM
		 * header and RMA the GPU. */
		ret = amdgv_umc_save_bad_pages(adapt);
		if (ret)
			goto release;
	}

	if (adapt->ecc.eh_data->count) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_BAD_PAGE_ENTRIES_FOUND,
			AMDGV_ERROR_32_32(adapt->ecc.eh_data->count, BAD_PAGE_RECORD_THRESHOLD));
	}

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return 0;

release:
	if (ret)
		amdgv_umc_release_bad_pages(adapt);
free:
	if (ret)
		amdgv_ras_eeprom_fini(&adapt->eeprom_control);

	if (amdgv_ras_eeprom_is_gpu_bad(adapt))
		amdgv_device_handle_bad_gpu(adapt);

	return ret;
}

int amdgv_umc_process_ras_data_cb(struct amdgv_adapter *adapt, void *ras_error_status,
				  uint32_t idx_vf)
{
	struct ras_err_data *err_data = (struct ras_err_data *)ras_error_status;
	int ret = AMDGV_RAS_SUCCESS;

	oss_spin_lock_irq(adapt->ecc.query_err_lock);
	if (adapt->umc.funcs && adapt->umc.funcs->query_ras_error_count)
		adapt->umc.funcs->query_ras_error_count(adapt, ras_error_status);
	if (adapt->umc.funcs && adapt->umc.funcs->query_ras_error_address &&
	    adapt->umc.max_ras_err_cnt_per_query) {
		err_data->err_addr = adapt->umc.err_addr;
		/* umc query_ras_error_address is also responsible for clearing
		 * error status
		 */
		adapt->umc.funcs->query_ras_error_address(adapt, ras_error_status);
	}
	oss_spin_unlock_irq(adapt->ecc.query_err_lock);

	if (err_data->ce_count) {
		AMDGV_INFO("%ld new correctable hardware errors detected in  UMC block\n", err_data->ce_count);
		adapt->ecc.correctable_error_num += err_data->ce_count;
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_VF_CE, err_data->ce_count);
	}

	if (err_data->ue_count) {
		AMDGV_INFO("%ld new uncorrectable hardware errors detected in  UMC block\n", err_data->ue_count);
		adapt->ecc.uncorrectable_error_num += err_data->ue_count;
	}

	if (err_data->de_count) {
		AMDGV_INFO("%ld new deferred hardware errors detected in  UMC block\n", err_data->de_count);
		adapt->ecc.deferred_error_num += err_data->de_count;
	}

	if (err_data->err_addr_cnt)
		ret = amdgv_umc_handle_bad_pages(adapt, err_data);

	return ret;
}

void amdgv_umc_fill_error_record(struct amdgv_adapter *adapt,
		struct ras_err_data *err_data,
		uint64_t err_addr,
		uint64_t retired_page,
		uint32_t channel_index,
		uint32_t umc_inst)
{
	struct eeprom_table_record *err_rec = NULL;

	if (amdgv_umc_update_bp_buff(adapt, &adapt->umc.err_addr,
			(uint32_t)err_data->err_addr_cnt, &adapt->umc.max_ras_err_cnt_per_query)) {
		AMDGV_ERROR("Failed to update bad page buffer!\n");
		return;
	} else {
		/* sync temp bad page buffer to new location */
		err_data->err_addr = adapt->umc.err_addr;
	}

	if (err_data->err_addr_cnt >= adapt->eeprom_control.max_record_num) {
		AMDGV_ERROR("Exceed the max eeprom record size! Stop filling new error record\n");
		return;
	}

	err_rec = &err_data->err_addr[err_data->err_addr_cnt];
	err_rec->address = err_addr;
	/* page frame address is saved */
	err_rec->retired_page = retired_page >> AMDGV_GPU_PAGE_SHIFT;
	err_rec->ts = amdgv_utc_to_eeprom_format(adapt, oss_get_utc_time_stamp());
	err_rec->err_type = AMDGV_RAS_EEPROM_ERR_NON_RECOVERABLE;
	err_rec->cu = 0;
	err_rec->mem_channel = channel_index;
	err_rec->mcumc_id = umc_inst;

	err_data->err_addr_cnt++;
}

int amdgv_umc_update_uc_error_count(struct amdgv_adapter *adapt,
				uint32_t idx_vf)
{
	struct ras_err_data err_data = { 0 };

	if (!amdgv_ecc_is_support(adapt, AMDGV_RAS_BLOCK__UMC))
		return 0; /* ECC not enabled */

	if (amdgv_umc_process_ras_data_cb(adapt, &err_data, idx_vf))
		AMDGV_WARN(
			"Page retireing failed during updating uncorrectable error count\n");
	return adapt->ecc.uncorrectable_error_num;
}

int amdgv_umc_update_error_count(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct ras_err_data err_data = { 0 };

	if (!amdgv_ecc_is_support(adapt, AMDGV_RAS_BLOCK__UMC))
		return 0; /* ECC not enabled */

	if (amdgv_umc_process_ras_data_cb(adapt, &err_data, idx_vf))
		AMDGV_WARN("Page retireing failed during updating correctable error count\n");
	return adapt->ecc.correctable_error_num;
}

/* Clean all the content in RAS EEPROM and
 * also the cached bad page array.
 */
int amdgv_umc_clean_bad_page_records(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	struct amdgv_ras_eeprom_control *control = &adapt->eeprom_control;
	struct amdgv_memmgr_mem *mem = NULL;
	int i, ret = 0;

	ret = amdgv_ras_eeprom_reset_table(adapt, control);
	if (ret) {
		AMDGV_ERROR("Failed to reset eeprom table\n");
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(adapt->ecc.recovery_lock);
	adapt->ecc.eh_data->last_retired_pfn = AMDGV_RAS_INV_MEM_PFN;
	if (data && data->rom_data.bps && data->rom_data.count)
		data->rom_data.count = 0;

	if (data && data->count && data->bps && data->bps_mem) {
		data->count = 0;
		for (i = data->last_reserved - 1; i >= 0; i--) {
			mem = data->bps_mem[i];
			amdgv_memmgr_free(mem);
			data->bps_mem[i] = NULL;
			data->last_reserved = i;
		}
		/* clean bad page block size */
		amdgv_vfmgr_clean_bp_block_size(adapt);
	}

	if (adapt->pp.pp_funcs->send_hbm_bad_pages_num)
		ret = adapt->pp.pp_funcs->send_hbm_bad_pages_num(adapt,
				control->num_recs);
	adapt->eeprom_control.bad_channel_bitmap = 0;
	if (adapt->pp.pp_funcs->send_hbm_bad_channel_flag)
		ret = adapt->pp.pp_funcs->send_hbm_bad_channel_flag(
			adapt, control->bad_channel_bitmap);
	oss_mutex_unlock(adapt->ecc.recovery_lock);

	return ret;
}

int amdgv_umc_ras_lock_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	adapt->ecc.query_err_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_HIGHEST_RANK);
	if (adapt->ecc.query_err_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		ret = AMDGV_FAILURE;
		goto out;
	}

	adapt->ecc.recovery_lock = oss_mutex_init();
	if (adapt->ecc.recovery_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		ret = AMDGV_FAILURE;
		goto out;
	}

	adapt->eeprom_control.tbl_mutex = oss_mutex_init();
	if (adapt->eeprom_control.tbl_mutex == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		ret = AMDGV_FAILURE;
	}

out:
	return ret;
}

void amdgv_umc_ras_lock_fini(struct amdgv_adapter *adapt)
{
	if (adapt->ecc.query_err_lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(adapt->ecc.query_err_lock);
		adapt->ecc.query_err_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->ecc.recovery_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->ecc.recovery_lock);
		adapt->ecc.recovery_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->eeprom_control.tbl_mutex != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->eeprom_control.tbl_mutex);
		adapt->eeprom_control.tbl_mutex = OSS_INVALID_HANDLE;
	}
}

int amdgv_umc_loop_channels(struct amdgv_adapter *adapt, umc_func func, void *data)
{
	uint32_t node_inst       = 0;
	uint32_t umc_inst        = 0;
	uint32_t ch_inst         = 0;
	int ret = 0;

	if (adapt->umc.node_inst_num) {
		uint32_t umc = 0;

		for_each_id(umc, adapt->umc.active_mask) {
			node_inst = umc / adapt->umc.umc_inst_num;
			umc_inst = umc % adapt->umc.umc_inst_num;
			LOOP_UMC_CH_INST(ch_inst) {
				ret = func(adapt, node_inst, umc_inst, ch_inst, data);
				if (ret) {
					AMDGV_ERROR("Node %d umc %d ch %d func returns %d\n",
						node_inst, umc_inst, ch_inst, ret);
					return ret;
				}
			}
		}
	} else {
		LOOP_UMC_INST_AND_CH(umc_inst, ch_inst) {
			ret = func(adapt, 0, umc_inst, ch_inst, data);
			if (ret) {
				AMDGV_ERROR("Umc %d ch %d func returns %d\n",
					umc_inst, ch_inst, ret);
				return ret;
			}
		}
	}

	return 0;
}

/*
 * Return value is the number of safe regions.
 * However, only up to max_entry_num of entries are copied
 */
int amdgv_umc_get_ras_vf_safe_range(struct amdgv_adapter *adapt,
			uint64_t *offset, uint64_t *size, uint32_t max_entry_num)
{
	if (adapt->umc.funcs->get_ras_vf_safe_range)
		return adapt->umc.funcs->get_ras_vf_safe_range(adapt, offset, size, max_entry_num);

	return 0;
}

int amdgv_umc_soc_pa_to_bank(struct amdgv_adapter *adapt,
	uint64_t soc_pa, struct amdgv_umc_fb_bank_addr *bank_addr)
{
	if (!adapt->umc.funcs->soc_pa_to_bank) {
		AMDGV_ERROR("Unable to translate SOC PA to Bank Address\n");
		return AMDGV_FAILURE;
	}

	return adapt->umc.funcs->soc_pa_to_bank(adapt, soc_pa, bank_addr);
}

int amdgv_umc_bank_to_soc_pa(struct amdgv_adapter *adapt,
	struct amdgv_umc_fb_bank_addr bank_addr, uint64_t *soc_pa)
{
	if (!adapt->umc.funcs->bank_to_soc_pa) {
		AMDGV_ERROR("Unable to translate Bank to SOC PA Address\n");
		return AMDGV_FAILURE;
	}

	return adapt->umc.funcs->bank_to_soc_pa(adapt, bank_addr, soc_pa);
}

int amdgv_umc_local_gpa_to_spa(struct amdgv_adapter *adapt,
	uint64_t gpa, uint32_t idx_vf, uint64_t *spa)
{
	if (adapt->ffbm.enabled) {
		*spa = amdgv_ffbm_gpa_to_spa(adapt, gpa, idx_vf);
		if (*spa == AMDGV_FFBM_INVALID_ADDR)
			return AMDGV_FAILURE;
		else
			return 0;
	}

	/* VF gpa convert to spa address */
	if ((!adapt->array_vf[idx_vf].configured) || (idx_vf >= adapt->num_vf))
		return AMDGV_FAILURE;

	*spa = MBYTES_TO_BYTES(adapt->array_vf[idx_vf].fb_offset) + gpa;

	return 0;
}

int amdgv_umc_local_spa_to_gpa(struct amdgv_adapter *adapt, uint64_t spa,
		uint64_t *gpa, uint32_t *idx_vf)
{
	int i;
	struct amdgv_vf_device *entry;
	uint64_t fb_offset, fb_size;

	if (adapt->ffbm.enabled) {
		*gpa = amdgv_ffbm_spa_to_gpa(adapt, spa, idx_vf);
		if (*gpa == AMDGV_FFBM_INVALID_ADDR)
			return AMDGV_FAILURE;
		else
			return 0;
	}

	for (i = 0; i < adapt->num_vf; i++) {
		entry = &adapt->array_vf[i];

		if (!entry->configured)
			continue;

		fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
		fb_size = MBYTES_TO_BYTES(entry->fb_size);

		if ((spa >= fb_offset) && (spa < fb_offset + fb_size)) {
			*idx_vf = i;
			*gpa = spa - fb_offset;
			return 0;
		}
	}
	return AMDGV_FAILURE;
}

void *amdgv_umc_grow_bp_buff(void *buff, uint32_t *cap, uint64_t size)
{
	void *new_buff = NULL;
	uint32_t new_cap = 0;

	if (!buff || !cap)
		return NULL;

	new_cap = (*cap) << 1;
	new_buff = oss_malloc(AMDGV_UMC_ALIGN(new_cap * size, AMDGV_UMC_ALIGNMENT));
	if (!new_buff)
		return NULL;

	oss_memcpy(new_buff, buff, (*cap) * size);
	oss_free(buff);
	buff = NULL;
	*cap = new_cap;

	return new_buff;
}

int amdgv_umc_update_bp_buff(struct amdgv_adapter *adapt, struct eeprom_table_record **bp_buff,
		uint32_t pages, uint32_t *cap)
{
	struct eeprom_table_record *temp_buff = *bp_buff;

	if (!*bp_buff || !cap)
		return AMDGV_FAILURE;
	/* Increase the bad page buffer size to store more bad pages */
	while (pages >= *cap) {
		temp_buff = (struct eeprom_table_record *)amdgv_umc_grow_bp_buff((void *)temp_buff,
									cap, sizeof(struct eeprom_table_record));
		if (!temp_buff) {
			AMDGV_ERROR("Failed to grow bad page buffer!\n");
			return AMDGV_FAILURE;
		}
		*bp_buff = temp_buff;
	}

	return 0;
}

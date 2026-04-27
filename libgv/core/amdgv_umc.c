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
#include "amdgv_ras_eeprom_internal.h"
#include "amdgv_sched_internal.h"
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

	if (data->count && index <= (uint32_t)data->count)
		oss_memcpy(bp_record, &data->bps[index], sizeof(struct eeprom_table_record));

	if (index > (uint32_t)data->count)
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

/* Returns true if bad page record should be skipped (invalid or duplicate) */
static bool amdgv_umc_check_invalid_bp(struct amdgv_adapter *adapt,
			struct eeprom_table_record *record, bool from_eeprom)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	uint64_t pa_pfn, rec_pa;
	uint64_t err_addr_pf, total_fb;
	uint32_t total_fb_in_mb;
	int i;

	if (from_eeprom) {
		if (record->ts == 0) {
			AMDGV_WARN("Bad page record skipped: EEPROM record has zero timestamp\n");
			return true;
		}
		if (adapt->umc.funcs && adapt->umc.funcs->eeprom_record_to_soc_pa) {
			adapt->umc.funcs->eeprom_record_to_soc_pa(adapt, record, &pa_pfn);
			if (adapt->umc.eeprom_version == EEPROM_TABLE_VER_V3)
				record->retired_page =
					set_nps_to_pa(pa_pfn, get_nps_from_pa(record->retired_page));
			else
				record->retired_page = pa_pfn;
		}
	}

	/* Reject address outside GPU framebuffer */
	amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_fb_in_mb);
	if (total_fb_in_mb) {
		total_fb = MBYTES_TO_BYTES(total_fb_in_mb);
		err_addr_pf = set_nps_to_pa(record->retired_page,
				AMDGV_MEMORY_PARTITION_MODE_UNKNOWN) << AMDGV_GPU_PAGE_SHIFT;
		if (err_addr_pf >= total_fb) {
			AMDGV_WARN("Bad page record skipped: address 0x%llx outside framebuffer (total_fb 0x%llx)\n",
				   err_addr_pf, total_fb);
			return true;
		}
	}

	/* Duplicate check: only for PMFW-managed EEPROM  or runtime records.
	 * Compare normalized PAs so that nps-encoded record matches stored.
	 */
	if (!from_eeprom || adapt->umc.is_pmfw_managed_eeprom) {
		rec_pa = set_nps_to_pa(record->retired_page,
				AMDGV_MEMORY_PARTITION_MODE_UNKNOWN);
		for (i = 0; i < data->rom_data.count; i++) {
			if (set_nps_to_pa(data->rom_data.bps[i].retired_page,
					AMDGV_MEMORY_PARTITION_MODE_UNKNOWN) == rec_pa) {
				AMDGV_WARN("Duplicate bad page record: PA=0x%llx from %s\n",
					   record->retired_page, from_eeprom ? "EEPROM" : "runtime");
				return true;
			}
		}
	}

	return false;
}

/* Cache the raw data loaded from eeprom and newly detected data
 * that needs to be saved to eeprom.
 * For the record type of base address, there is no need to extend
 * to bad page addresses.
 */
static int amdgv_umc_update_eeprom_rom_data(struct amdgv_adapter *adapt,
		struct eeprom_table_record *bps, struct eeprom_data_record *data)
{
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
				struct eeprom_table_record *bps, struct ras_err_handler_data *data)
{
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
		/* Add page to sorted bad pages array */
		amdgv_umc_insert_sorted_bad_page(adapt, bps->retired_page, data);
	}

	amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_BAD_PAGE_APPEND,
		AMDGV_ERROR_32_32(count, data->count));

	return 0;
}

int amdgv_umc_add_bad_pages(struct amdgv_adapter *adapt,
				   struct eeprom_table_record *bps, int pages, bool from_eeprom)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	enum amdgv_memory_partition_mode nps = AMDGV_MEMORY_PARTITION_MODE_UNKNOWN;
	int ret = 0;
	int i = 0;

	if (adapt->ecc.bad_page_detection_mode & BIT(AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return 0;

	if (!bps || pages <= 0)
		return 0;

	if (!data)
		return 0;

	if (adapt->nbio.funcs &&
		adapt->nbio.funcs->get_nps_mode) {
		ret = adapt->nbio.funcs->get_nps_mode(adapt, &nps);
		if (ret) {
			AMDGV_ERROR("Failed to get current nps mode\n");
			return 0;
		}
	}

	oss_mutex_lock(adapt->ecc.recovery_lock);

	for (i = 0; i < pages; i++) {
		if (data->count >= (int)adapt->eeprom_control.max_record_num) {
			AMDGV_ERROR("Bad page record count exceeds the max limit! Dropping new bad pages\n");
			ret = AMDGV_FAILURE;
			goto out;
		}

		if (amdgv_umc_check_invalid_bp(adapt, &bps[i], from_eeprom))
			continue;

		if (adapt->umc.is_pmfw_managed_eeprom ||
				(!from_eeprom && (adapt->umc.eeprom_version == EEPROM_TABLE_VER_V3)))
			bps[i].retired_page = set_nps_to_pa(bps[i].retired_page, nps);

		ret = amdgv_umc_update_eeprom_rom_data(adapt, &bps[i], &data->rom_data);
		if (ret)
			goto out;

		/* Since the base address translated from the legacy page
		   record in the eeprom is the same, only the first address
		   needs to be extended. */
		if (set_nps_to_pa(data->last_retired_pfn, AMDGV_MEMORY_PARTITION_MODE_UNKNOWN) ==
				set_nps_to_pa(bps[i].retired_page, AMDGV_MEMORY_PARTITION_MODE_UNKNOWN))
			continue;

		data->last_retired_pfn = bps[i].retired_page;

		ret = amdgv_umc_update_eeprom_ram_data(adapt, &bps[i], data);
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

	if (adapt->ecc.bad_page_detection_mode & BIT(AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return 0;

	/*for pmfw managed eeprom, the bad pages are saved by pmfw,
	 * driver has no direct access to eeprom.
	 */
	if (adapt->umc.is_pmfw_managed_eeprom)
		return 0;

	if (!data)
		return 0;

	control = &adapt->eeprom_control;
	save_count = data->rom_data.count - data->rom_data.num_recs_synced;
	if (save_count > 0) {
		if (amdgv_ras_eeprom_process_records(adapt, control,
			    &data->rom_data.bps[data->rom_data.num_recs_synced], true, save_count)) {
			AMDGV_ERROR("Failed to save EEPROM table data!\n");
			return AMDGV_FAILURE;
		}
		data->rom_data.num_recs_synced = data->rom_data.count;

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

static int amdgv_umc_add_bad_pages_across_nps(struct amdgv_adapter *adapt,
				   struct eeprom_table_record *bps, int pages,
				   enum amdgv_memory_partition_mode nps,
				   struct ras_err_handler_data *data)
{
	int ret = 0;
	int i = 0;

	if (adapt->ecc.bad_page_detection_mode & BIT(AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return 0;

	if (!bps || pages <= 0)
		return 0;

	if (!data)
		return 0;

	oss_mutex_lock(adapt->ecc.recovery_lock);

	for (i = 0; i < pages; i++) {
		if (data->count >= (int)adapt->eeprom_control.max_record_num) {
			AMDGV_ERROR("Bad page record count exceeds the max limit! Dropping new bad pages\n");
			ret = AMDGV_FAILURE;
			goto out;
		}

		if (amdgv_umc_check_invalid_bp(adapt, &bps[i], true))
			continue;

		/* retired_page already converted in amdgv_umc_check_invalid_bp; apply caller's nps */
		bps[i].retired_page = set_nps_to_pa(bps[i].retired_page, nps);

		ret = amdgv_umc_update_eeprom_rom_data(adapt, &bps[i], &data->rom_data);
		if (ret)
			goto out;

		/* Since the base address translated from the legacy page
		   record in the eeprom is the same, only the first address
		   needs to be extended. */
		if (set_nps_to_pa(data->last_retired_pfn, AMDGV_MEMORY_PARTITION_MODE_UNKNOWN) ==
				set_nps_to_pa(bps[i].retired_page, AMDGV_MEMORY_PARTITION_MODE_UNKNOWN))
			continue;

		data->last_retired_pfn = bps[i].retired_page;

		/* Extending pages */
		oss_atomic_set(&adapt->ecc.in_cross_nps_handling, 1);
		ret = amdgv_umc_update_eeprom_ram_data(adapt, &bps[i], data);
		if (ret)
			goto out;
	}

out:
	oss_atomic_set(&adapt->ecc.in_cross_nps_handling, 0);
	oss_mutex_unlock(adapt->ecc.recovery_lock);

	return ret;
}

int amdgv_umc_load_bad_pages_across_nps(struct amdgv_adapter *adapt)
{
	int ret = 0, i;
	struct amdgv_ras_eeprom_control *control = &adapt->eeprom_control;
	struct eeprom_table_record *bps = adapt->ecc.init_bps;
	struct eeprom_table_record *temp_bps = NULL;
	struct ras_err_handler_data *data = adapt->ecc.eh_data_across_nps;
	enum amdgv_memory_partition_mode nps = 0;
	int supported_nps_count = adapt->ecc.supported_nps_count;

	if (adapt->ecc.bad_page_detection_mode & BIT(AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return ret;

	if (!bps)
		return AMDGV_FAILURE;

	temp_bps = oss_zalloc(control->num_recs * sizeof(*temp_bps));
	if (!temp_bps)
		return AMDGV_FAILURE;

	/* Get existing pmfw-eeprom bad pages records to minimizing pmfw accesss */
	if (amdgv_ras_eeprom_process_records(adapt, control, bps, false, control->num_recs)) {
			AMDGV_ERROR("Failed to load EEPROM table records!\n");
			ret = AMDGV_FAILURE;
			goto out;
		}

	for (i = 0; i < supported_nps_count; i++) {
		oss_memcpy(temp_bps, bps, control->num_recs * sizeof(*bps));

		/* Add bad pages masked with supported nps to data[i] */
		data[i].last_retired_pfn = AMDGV_RAS_INV_MEM_PFN;
		nps = adapt->ecc.supported_nps[i];
		if (amdgv_umc_add_bad_pages_across_nps(adapt, temp_bps, control->num_recs, nps, &data[i]))
			ret = AMDGV_FAILURE;
	}

	if (data) {
		for (i = 0; i < supported_nps_count; i++)
			data[i].rom_data.num_recs_synced = data[i].rom_data.count;
	}
out:
	oss_free(temp_bps);
	return ret;
}

int amdgv_umc_load_bad_pages(struct amdgv_adapter *adapt)
{
	int ret = 0, new_count, i;
	struct amdgv_ras_eeprom_control *control = &adapt->eeprom_control;
	struct eeprom_table_record *bps = NULL;
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	enum amdgv_memory_partition_mode nps;

	if (adapt->ecc.bad_page_detection_mode & BIT(AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return 0;

	/* no bad page record, skip eeprom access */
	if (!control->num_recs)
		return ret;

	bps = oss_zalloc(control->num_recs * sizeof(*bps));
	if (!bps)
		return AMDGV_FAILURE;

	if (adapt->umc.is_pmfw_managed_eeprom && adapt->ecc.init_bps_num_recs) {
		if (adapt->ecc.init_bps_num_recs == control->num_recs){
			if (!adapt->ecc.init_bps)
				return AMDGV_FAILURE;

			oss_memcpy(bps, adapt->ecc.init_bps, control->num_recs * sizeof(*bps));
		} else {
			AMDGV_ERROR("New bad page records come in during hw_init, exiting.\n");
			return AMDGV_FAILURE;
		}
	} else {
		if (amdgv_ras_eeprom_process_records(adapt, control, bps, false, control->num_recs)) {
			AMDGV_ERROR("Failed to load EEPROM table records!\n");
			ret = AMDGV_FAILURE;
			goto out;
		}
	}

	if (adapt->ecc.eh_data) {
		adapt->ecc.eh_data->last_retired_pfn = AMDGV_RAS_INV_MEM_PFN;
	}

	if (adapt->ecc.eh_data && adapt->ecc.eh_data->sorted_bps) {
		adapt->ecc.eh_data->sorted_bp_count = 0;
		/* Reset the count and zero out the memory */
		oss_memset(adapt->ecc.eh_data->sorted_bps, 0,
			  adapt->ecc.eh_data->sorted_bps_cap * sizeof(uint64_t));
	}
	if (amdgv_umc_add_bad_pages(adapt, bps, control->num_recs, true))
		ret = AMDGV_FAILURE;

	if (data)
		data->rom_data.num_recs_synced = data->rom_data.count;

	if (adapt->umc.eeprom_version >= EEPROM_TABLE_VER_V3) {
		if (control->tbl_hdr.version < EEPROM_TABLE_VER_V3) {
			if (adapt->nbio.funcs &&
				adapt->nbio.funcs->get_nps_mode) {
				ret = adapt->nbio.funcs->get_nps_mode(adapt, &nps);
				if (ret) {
						AMDGV_ERROR("Failed to get current nps mode\n");
						goto out;
				}
			}
			ret = amdgv_ras_eeprom_reset_table(adapt, control);
			if (ret) {
				AMDGV_ERROR("Failed to reset eeprom table\n");
				goto out;
			}

			oss_mutex_lock(adapt->ecc.recovery_lock);
			adapt->ecc.eh_data->last_retired_pfn = AMDGV_RAS_INV_MEM_PFN;
			if (data && data->rom_data.bps && data->rom_data.count) {
				data->rom_data.count = 0;
				data->rom_data.num_recs_synced = 0;
				new_count = data->count / 16;
				for (i = 0; i < new_count; i++) {
					ret = amdgv_umc_update_eeprom_rom_data(adapt, &(data->bps[i * 16]), &data->rom_data);
					if (ret)
						goto out;
					data->rom_data.bps[data->rom_data.count - 1].retired_page =
						set_nps_to_pa(data->rom_data.bps[data->rom_data.count - 1].retired_page, nps);
				}
				if (amdgv_ras_eeprom_process_records(
					adapt, control, &data->rom_data.bps[control->num_recs], true, new_count)) {
					AMDGV_ERROR("Failed to save EEPROM table data!\n");
					ret =  AMDGV_FAILURE;
					goto out;
				}
				data->rom_data.num_recs_synced = data->rom_data.count;
			}
			oss_mutex_unlock(adapt->ecc.recovery_lock);
		}
	}

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
	struct eeprom_data_record *rom_data;
	struct ras_err_handler_data *eh_data;

	int i, ret = 0;

	if (!adapt->ecc.eh_data) {
		AMDGV_ERROR("eh_data is null\n");
		return AMDGV_FAILURE;
	}

	rom_data = &adapt->ecc.eh_data->rom_data;
	eh_data = adapt->ecc.eh_data;

	/* release pf reserved bp */
	amdgv_umc_release_bad_pages(adapt);

	/* reset bps for fresh reload from rom data
	 * The allocated size of bps can be larger then
	 * actual used after reload.
	 */
	eh_data->count = 0;
	eh_data->last_retired_pfn = AMDGV_RAS_INV_MEM_PFN;

	if (adapt->ecc.eh_data && adapt->ecc.eh_data->sorted_bps) {
		adapt->ecc.eh_data->sorted_bp_count = 0;
		/* Reset the count and zero out the memory */
		oss_memset(adapt->ecc.eh_data->sorted_bps, 0,
			  adapt->ecc.eh_data->sorted_bps_cap * sizeof(uint64_t));
	}

	/* load from rom data */
	for (i = 0; i < rom_data->count; i++) {
		ret = amdgv_umc_update_eeprom_ram_data(adapt, &rom_data->bps[i], eh_data);
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
	uint64_t local_addr = addr;
	uint64_t xgmi_offset = 0;

	oss_mutex_lock(adapt->ecc.recovery_lock);
	if (!data)
		goto out;

	/* Translate the global err offset to local offset according to xgmi config */
	if (adapt->xgmi.phy_nodes_num > 1) {
		xgmi_offset = adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size;

		if (addr >= xgmi_offset)
			local_addr = addr - xgmi_offset;
	}

	page_addr = local_addr >> AMDGV_GPU_PAGE_SHIFT;
	for (i = 0; i < data->count; i++) {
		if (page_addr == data->bps[i].retired_page) {
			AMDGV_ERROR("Global address (0x%llx) found in an EEPROM entry as a retired page!\n", addr);
			ret = true;
			goto out;
		}
	}

out:
	oss_mutex_unlock(adapt->ecc.recovery_lock);
	return ret;
}

static bool amdgv_umc_is_bp_in_range(uint64_t err_addr, uint64_t offset, uint64_t size)
{
	uint64_t err_addr_begin, err_addr_end;

	err_addr_begin = err_addr & ~(AMDGV_GPU_PAGE_SIZE - 1);
	err_addr_end = err_addr_begin | (AMDGV_GPU_PAGE_SIZE - 1);
	if ((err_addr_begin >= offset) && (err_addr_end < offset + size))
		return true;
	else
		return false;
}

bool amdgv_umc_check_bad_pages_in_range(struct amdgv_adapter *adapt, uint64_t fb_offset, uint64_t size)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	int i;
	bool ret = false;
	uint64_t fb_end = fb_offset + size;
	uint64_t bad_page_addr;

	oss_mutex_lock(adapt->ecc.recovery_lock);
	if (!data || !data->sorted_bps)
		goto out;

	/* Iterate through all bad pages and check if any falls within the range */
	for (i = 0; i < data->sorted_bp_count; i++) {
		bad_page_addr = data->sorted_bps[i] << AMDGV_GPU_PAGE_SHIFT;
		if (amdgv_umc_is_bp_in_range(bad_page_addr, fb_offset, size)) {
			AMDGV_INFO("Bad page found in fb range [0x%llx-0x%llx] at address 0x%llx\n",
					fb_offset, fb_end, bad_page_addr);
			ret = true;
			goto out;
		}
	}

out:
	oss_mutex_unlock(adapt->ecc.recovery_lock);
	return ret;
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

static bool amdgv_umc_check_bp_in_critical_region(struct amdgv_adapter *adapt, uint64_t err_addr,
						  uint32_t idx_vf, bool log_err)
{
	struct amdgv_vf_device *entry;
	struct psp_local_memory *tmr_mem;
	struct amdgv_memmgr_mem *csa_fb_mem;
	uint64_t vf_fb_offset, vf_fb_size, vf_real_fb_size;
	uint64_t fb_offset, fb_size, total_avail_fb;
	uint32_t total_avail_fb_in_mb;
	uint32_t i;
	uint64_t err_addr_gpa;
	/* start offset of fb in mc */
	const uint64_t fb_mc_base = adapt->memmgr_pf.mc_base;

	if (adapt->ecc.bad_page_detection_mode & BIT(AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
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
			if (amdgv_umc_is_bp_in_range(err_addr, vf_fb_offset, vf_fb_size) &&
			    (adapt->flags & AMDGV_FLAG_USE_PF)) {
				if (log_err)
					adapt->bp_msg_type = AMDGV_BP_MSG_IN_PF_FB;
				return true;
			}

			if (amdgv_umc_is_bp_in_range(err_addr, adapt->memmgr_pf.offset, adapt->memmgr_pf.size) &&
			    (adapt->flags & AMDGV_FLAG_USE_PF)) {
				if (log_err)
					adapt->bp_msg_type = AMDGV_BP_MSG_IN_PF_FB;
				return true;
			}
		} else if (!adapt->umc.is_pmfw_managed_eeprom ||
			   (adapt->umc.is_pmfw_managed_eeprom && is_active_vf(idx_vf))) {
			/* we may get multiple ECCs at the same time, and the ecc address and ecc vf may
			* be not in sequence. So check all the VF for critical range, if found, set
			* to RMA status */
			for (i = 0; i < adapt->num_vf; i++) {
				entry = &adapt->array_vf[i];

				if (amdgv_vfmgr_check_bp_in_crit_region(adapt, idx_vf, err_addr)) {
					if (log_err)
						adapt->bp_msg_type = AMDGV_BP_MSG_IN_VF_CRITICAL_REGION;
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
		fb_offset = amdgv_memmgr_get_gpu_addr(tmr_mem->mem) - fb_mc_base;
		fb_size = amdgv_memmgr_get_size(tmr_mem->mem);
		if (amdgv_umc_is_bp_in_range(err_addr, fb_offset, fb_size)) {
			if (log_err)
				adapt->bp_msg_type = AMDGV_BP_MSG_IN_CRITICAL_REGION;

			return true;
		}
	}

	/* check bp whether in pf ip discovery data block region */
	fb_offset = total_avail_fb - AMDGV_IP_DISCOVERY_OFFSET;
	fb_size = AMDGV_IP_DISCOVERY_SIZE;
	if (amdgv_umc_is_bp_in_range(err_addr, fb_offset, fb_size)) {
		if (log_err)
			adapt->bp_msg_type = AMDGV_BP_MSG_IN_CRITICAL_REGION;

		return true;
	}

	/* check bp whether in csa region */
	csa_fb_mem = adapt->gpuiov.csa_fb_mem;
	if (adapt->ecc.ras_ecc_flags & AMDGV_ECC_FLAG__CSA_NON_CRITICAL) {
		AMDGV_DEBUG("skip csa critical region check.\n");
	} else {
		if (csa_fb_mem) {
			fb_offset = amdgv_memmgr_get_gpu_addr(csa_fb_mem) - fb_mc_base;
			fb_size = amdgv_memmgr_get_size(csa_fb_mem);
			if (amdgv_umc_is_bp_in_range(err_addr, fb_offset, fb_size)) {
				if (log_err)
					adapt->bp_msg_type = AMDGV_BP_MSG_IN_CRITICAL_REGION;

				return true;
			}
		} else
			AMDGV_ERROR("Private csa fb memory not allocated\n");
	}


	/* check FFBM PF fb region */
	if (adapt->ffbm.enabled) {
		/* when idx_vf is PF, it can be TMR or PF fb
		 * and TMR has already been checked above
		 */
		err_addr_gpa = amdgv_ffbm_spa_to_gpa(adapt, err_addr, &idx_vf);
		if (idx_vf == AMDGV_PF_IDX &&
			err_addr_gpa != AMDGV_FFBM_INVALID_ADDR && (adapt->flags & AMDGV_FLAG_USE_PF)) {
			if (log_err)
				adapt->bp_msg_type = AMDGV_BP_MSG_IN_PF_FB;
			/* return true if PF_FB is in use */
			return true;
		}
	}

	return false;
}

int amdgv_umc_vf_chk_critical_region(struct amdgv_adapter *adapt, uint64_t err_addr,
				     uint32_t idx_vf, uint32_t *hit)
{
	uint64_t page_pfns[32] = {0}, pa;
	uint32_t idx;
	int i, count;

	if (hit)
		*hit = 0;
	else
		return 0;

	err_addr = amdgv_gpa_to_local_spa(adapt, err_addr, idx_vf);

	if (adapt->umc.funcs->pages_in_a_row)
		count = adapt->umc.funcs->pages_in_a_row(adapt, err_addr,
							 page_pfns, ARRAY_SIZE(page_pfns));
	else
		return 0;

	if (count <= 0)
		return AMDGV_FAILURE;

	for (i = 0; i < count; i++) {
		pa = page_pfns[i] << AMDGV_GPU_PAGE_SHIFT;
		/* idx_vf is ignored as bp in critical region of any of vf slots is a concern */
		idx = amdgv_umc_calc_retired_page_vf_slot(adapt, pa);
		if (amdgv_umc_check_bp_in_critical_region(adapt, pa, idx, false)) {
			*hit = 1;
			break;
		}
	}

	return 0;
}

static void amdgv_umc_log_bp_errors(struct amdgv_adapter *adapt, uint32_t record_id)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;

	if (!data)
		return;

	if (adapt->ecc.bad_page_detection_mode & BIT(AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return;

	switch (adapt->bp_msg_type) {
	case AMDGV_BP_MSG_IN_PF_FB:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_IN_PF_FB,
				AMDGV_ERROR_32_32(record_id, BAD_PAGE_RECORD_THRESHOLD));
		break;
	case AMDGV_BP_MSG_IN_CRITICAL_REGION:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_IN_CRI_REG,
				AMDGV_ERROR_32_32(record_id, BAD_PAGE_RECORD_THRESHOLD));
		break;
	case AMDGV_BP_MSG_IN_VF_CRITICAL_REGION:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_IN_VF_CRI,
				AMDGV_ERROR_32_32(record_id, BAD_PAGE_RECORD_THRESHOLD));
		break;
	case AMDGV_BP_MSG_RECORD_THRESHOLD_REACHED:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_REACH_THD,
				BAD_PAGE_RECORD_THRESHOLD);
		break;
	default:
		break;
	}

	return;
}

static bool amdgv_umc_check_bp_threshold(struct amdgv_adapter *adapt, struct ras_err_handler_data *data)
{
	if (adapt->ecc.bad_page_detection_mode & BIT(AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return false;

	/* RMA at the max count because new entries will be lost. */
	if (data->count >= (int)BAD_PAGE_RECORD_THRESHOLD) {
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

	if (adapt->ecc.bad_page_detection_mode & BIT(AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return ret;

	if (!data)
		return ret;

	oss_mutex_lock(adapt->ecc.recovery_lock);

	if (amdgv_umc_check_bp_threshold(adapt, data)) {
		amdgv_umc_log_bp_errors(adapt, BAD_PAGE_RECORD_THRESHOLD);
		goto out;
	}

	resv_count = data->count <= (int)BAD_PAGE_RECORD_THRESHOLD ? data->count : BAD_PAGE_RECORD_THRESHOLD;

	/* reserve vram at driver post stage. */
	for (i = data->last_reserved; i < resv_count; i++) {
		err_addr_pf = data->bps[i].retired_page << AMDGV_GPU_PAGE_SHIFT;
		amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_fb_in_mb);
		total_fb = (uint64_t)total_fb_in_mb;
		/* convert from MB to Byte */
		total_fb = MBYTES_TO_BYTES(total_fb);
		err_addr_gpu = total_fb - err_addr_pf;

		idx_vf = amdgv_umc_calc_retired_page_vf_slot(adapt, err_addr_pf);
		if (amdgv_umc_check_bp_in_critical_region(adapt, err_addr_pf, idx_vf, true)) {
			/* if PMFW managed EEPROM and VF is not active, critical regions
			 * don't exist yet so we should not log anything here */
			if (adapt->umc.is_pmfw_managed_eeprom && is_active_vf(idx_vf)) {
				/* bad page discovered in critical region of active VF, we must
				 * set VF to conditionally available */
				amdgv_umc_log_bp_errors(adapt, i);
				amdgv_sched_queue_set_vf_cond_avail(adapt, idx_vf);
				adapt->array_vf[idx_vf].host_crit_region_caps =
					adapt->array_vf[idx_vf].host_crit_region_caps & ~BIT(0);
			}
			if (!adapt->umc.is_pmfw_managed_eeprom)
				amdgv_umc_log_bp_errors(adapt, i);

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
			if (!data->bps_mem[i]) {
				AMDGV_WARN("Reserve page at 0x%llx in memmgr_pf failed\n",
					   err_addr_pf);
				data->bp_replace_pending = true;
			}
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

int amdgv_umc_copy_bp_records_to_vf(struct amdgv_adapter *adapt,
				    uint32_t idx_vf,
				    uint32_t allowed_size,
				    uint32_t *write_size,
				    uint32_t *more)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	uint64_t err_addr;
	int i;

	*write_size = 0;
	*more = 0;

	if (!data)
		return 0;

	oss_mutex_lock(adapt->ecc.recovery_lock);

	for (i = 0; i < data->count; i++) {
		err_addr = data->bps[i].retired_page << AMDGV_GPU_PAGE_SHIFT;
		if (amdgv_umc_calc_retired_page_vf_slot(adapt, err_addr) != idx_vf ||
		    amdgv_umc_check_bp_in_critical_region(adapt, err_addr, idx_vf, true))
		    continue;

		if (*write_size > allowed_size) {
			*more = true;
			break;
		}

		if (!amdgv_vfmgr_copy_bp_entry_to_vf_fb(adapt, idx_vf,
							data->bps[i].retired_page,
							(*write_size) / sizeof(uint64_t),
							NULL))
			(*write_size) += sizeof(uint64_t);
	}

	oss_mutex_unlock(adapt->ecc.recovery_lock);

	return 0;
}

void amdgv_umc_check_and_handle_bp_in_crit_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	uint64_t err_addr;
	int i;

	/* GPU is already in bad state */
	if (adapt->bp_msg_type != AMDGV_BP_MSG_INVALID)
		return;

	oss_mutex_lock(adapt->ecc.recovery_lock);

	if (data) {
		for (i = 0; i < data->count; i++) {
			err_addr = data->bps[i].retired_page << AMDGV_GPU_PAGE_SHIFT;
			if (idx_vf != amdgv_umc_calc_retired_page_vf_slot(adapt, err_addr))
				continue;
			if (amdgv_umc_check_bp_in_critical_region(adapt, err_addr, idx_vf, true)) {
				amdgv_umc_log_bp_errors(adapt, i);
				break;
			}
		}
	}

	oss_mutex_unlock(adapt->ecc.recovery_lock);

	/* Commit the new status to EEPROM */
	if (adapt->bp_msg_type != AMDGV_BP_MSG_INVALID)
		amdgv_umc_save_bad_pages(adapt);

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

int amdgv_umc_across_nps_err_data_init(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data **data_across_nps = &(adapt->ecc.eh_data_across_nps);
	enum amdgv_memory_partition_mode supported_nps[AMDGV_MEMORY_PARTITION_MODE_MAX] = {0};
	int supported_nps_count = 0;
	int i, ret = 0;
	struct eeprom_table_record **init_bps = &(adapt->ecc.init_bps);

	/* Allocate driver buf for existing pmfw-eeprom bad pages records to minimizing pmfw access */
	*init_bps = oss_zalloc(adapt->eeprom_control.num_recs * sizeof(**init_bps));
	if (!*init_bps)
		goto error;
	adapt->ecc.init_bps_num_recs = adapt->eeprom_control.num_recs;

	/* get supported nps info/count */
	if (adapt->nbio.ras && adapt->nbio.ras->get_supported_memory_partition_mode) {
		ret = adapt->nbio.ras->get_supported_memory_partition_mode(adapt,
			supported_nps, &supported_nps_count);
		if (supported_nps_count == 0) {
			AMDGV_ERROR("No supported nps mode found.\n");
			return AMDGV_FAILURE;
		}
	}

	/* Allocate it to contain bad pages masking with all supported NPS respectively */
	*data_across_nps = oss_zalloc(supported_nps_count * sizeof(**data_across_nps));
	if (!*data_across_nps)
		goto error;

	for (i = 0; i < supported_nps_count; i++) {
		(*data_across_nps)[i].last_retired_pfn = AMDGV_RAS_INV_MEM_PFN;
		(*data_across_nps)[i].rom_data.bps_cap = BAD_PAGE_RECORD_THRESHOLD;
		(*data_across_nps)[i].rom_data.bps = oss_zalloc(AMDGV_UMC_ALIGN(
			(*data_across_nps)[i].rom_data.bps_cap * sizeof(*(*data_across_nps)[i].rom_data.bps), AMDGV_UMC_ALIGNMENT));

		(*data_across_nps)[i].bps_cap = BAD_PAGE_RECORD_THRESHOLD;
		(*data_across_nps)[i].bps = oss_zalloc(AMDGV_UMC_ALIGN(
			(*data_across_nps)[i].bps_cap * sizeof(*(*data_across_nps)[i].bps), AMDGV_UMC_ALIGNMENT));
		(*data_across_nps)[i].bps_mem = oss_zalloc(AMDGV_UMC_ALIGN(
			BAD_PAGE_RECORD_THRESHOLD * sizeof(*(*data_across_nps)[i].bps_mem), AMDGV_UMC_ALIGNMENT));

		(*data_across_nps)[i].bp_replace_pending = false;

		if (!(*data_across_nps)[i].rom_data.bps || !(*data_across_nps)[i].bps || !(*data_across_nps)[i].bps_mem)
			goto error;
	}

	adapt->ecc.eh_data_across_nps_initialized = true;
	for (i = 0; i < supported_nps_count; i++)
		adapt->ecc.supported_nps[i] = supported_nps[i];
	adapt->ecc.supported_nps_count = supported_nps_count;

	return 0;
error:
	AMDGV_ERROR("Failed to initialize ras data with all supported nps info!\n");
	amdgv_umc_across_nps_err_data_fini(adapt);
	return AMDGV_FAILURE;

}

int amdgv_umc_sw_init(struct amdgv_adapter *adapt)
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

	(*data)->bp_replace_pending = false;

	/* Initialize sorted bad pages array */
	if (amdgv_umc_init_sorted_bad_pages(adapt) != 0) {
		AMDGV_ERROR("Failed to initialize sorted bad pages array\n");
		goto error;
	}

	if (!(*data)->bps || !(*data)->bps_mem)
		goto error;

	return 0;
error:
	AMDGV_ERROR("Failed to initialize ras recovery!\n");
	amdgv_umc_sw_fini(adapt);
	return AMDGV_FAILURE;
}

int amdgv_umc_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
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

		if (adapt->nbio.funcs &&
			 adapt->nbio.funcs->get_nps_mode) {
			ret = adapt->nbio.funcs->get_nps_mode(adapt, &nps_mode);
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
			} else {
				ret = amdgv_umc_reserve_bad_pages(adapt);
				if (ret) {
					AMDGV_ERROR("Failed to reserve bad pages\n");
					goto release;
				}
			}
		}

		ret = amdgv_umc_save_bad_pages(adapt);
		if (ret)
			goto release;
	} else if (!adapt->umc.is_pmfw_managed_eeprom) {
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

	return ret;
}

int amdgv_umc_across_nps_err_data_fini(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data *data_across_nps = adapt->ecc.eh_data_across_nps;
	struct eeprom_table_record *init_bps = adapt->ecc.init_bps;
	int supported_nps_count = adapt->ecc.supported_nps_count;
	int i;

	if (!init_bps)
		return 0;

	/* Fini init_bps */
	oss_free(init_bps);
	adapt->ecc.init_bps = NULL;
	adapt->ecc.init_bps_num_recs = 0;

	if (!data_across_nps)
		return 0;

	/* get supported nps count */
	if (adapt->ecc.supported_nps_count != 0) {
		for (i = 0; i < supported_nps_count; i++) {
			struct ras_err_handler_data *data = &data_across_nps[i];

			data->count = 0;
			data->last_reserved = 0;
			data->rom_data.count = 0;
			data->rom_data.num_recs_synced = 0;

			if (data->rom_data.bps) {
				oss_memset(data->rom_data.bps, 0,
					AMDGV_UMC_ALIGN(data->rom_data.bps_cap * sizeof(*data->rom_data.bps),
						AMDGV_UMC_ALIGNMENT));
				oss_free(data->rom_data.bps);
				data->rom_data.bps = NULL;
				data->rom_data.bps_cap = 0;
			}

			if (data->bps) {
				oss_memset(data->bps, 0,
					AMDGV_UMC_ALIGN(data->bps_cap * sizeof(*data->bps),
						AMDGV_UMC_ALIGNMENT));
				oss_free(data->bps);
				data->bps = NULL;
				data->bps_cap = 0;
			}

			if (data->bps_mem) {
				oss_memset(data->bps_mem, 0,
					AMDGV_UMC_ALIGN(BAD_PAGE_RECORD_THRESHOLD * sizeof(*data->bps_mem),
						AMDGV_UMC_ALIGNMENT));
				oss_free(data->bps_mem);
				data->bps_mem = NULL;
			}
		}

		oss_free(data_across_nps);
		adapt->ecc.eh_data_across_nps = NULL;
		for (i = 0; i < supported_nps_count; i++)
			adapt->ecc.supported_nps[i] = 0;
		adapt->ecc.supported_nps_count = 0;
	}

	adapt->ecc.eh_data_across_nps_initialized = false;

	return 0;
}

int amdgv_umc_sw_fini(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;

	/* recovery_init failed to init it, fini is useless */
	if (data) {

		data->count = 0;
		data->last_reserved = 0;
		data->rom_data.count = 0;
		data->rom_data.num_recs_synced = 0;

		if (data->rom_data.bps) {
			oss_memset(data->rom_data.bps, 0,
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
		/* Cleanup sorted bad pages array */
		amdgv_umc_cleanup_sorted_bad_pages(adapt);
		oss_free(data);
		adapt->ecc.eh_data = NULL;
	}

	return 0;
}

int amdgv_umc_hw_fini(struct amdgv_adapter *adapt)
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

	if (adapt->umc.is_pmfw_managed_eeprom) {
		/* Get current bad page count*/
		if (adapt->pp.pmme_funcs &&
			adapt->pp.pmme_funcs->get_bad_page_count) {
			ret = adapt->pp.pmme_funcs->get_bad_page_count(adapt,
				&adapt->eeprom_control.num_recs);
			if (ret)
				return ret;
		}
	}
	if (adapt->eeprom_control.num_recs) {
		ret = amdgv_umc_load_bad_pages(adapt);
		if (ret)
			goto free;
		ret = amdgv_umc_reserve_bad_pages(adapt);
		if (ret)
			goto release;

		/* nps mode is required if memory partitioning supported */
		if (adapt->nbio.funcs &&
			adapt->nbio.funcs->get_nps_mode) {
			ret = adapt->nbio.funcs->get_nps_mode(adapt, &nps_mode);
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
	err_rec->ts = amdgv_ras_eeprom_utc_to_eeprom_format(adapt, oss_get_utc_time_stamp());
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
	if (adapt->ecc.eh_data) {
		adapt->ecc.eh_data->last_retired_pfn = AMDGV_RAS_INV_MEM_PFN;
	}

	if (data) {
		data->rom_data.bps_cap = BAD_PAGE_RECORD_THRESHOLD;
	}
	if (adapt->ecc.eh_data && adapt->ecc.eh_data->sorted_bps) {
		adapt->ecc.eh_data->sorted_bp_count = 0;
		/* Reset the count and zero out the memory */
		oss_memset(adapt->ecc.eh_data->sorted_bps, 0,
			  adapt->ecc.eh_data->sorted_bps_cap * sizeof(uint64_t));
	}
	if (data && data->rom_data.bps && data->rom_data.count) {
		data->rom_data.count = 0;
		data->rom_data.num_recs_synced = 0;
	}

	if (data && data->count && data->bps && data->bps_mem) {
		data->count = 0;
		for (i = data->last_reserved - 1; i >= 0; i--) {
			mem = data->bps_mem[i];
			amdgv_memmgr_free(mem);
			data->bps_mem[i] = NULL;
			data->last_reserved = i;
		}
	}

	if (adapt->pp.pp_funcs->send_hbm_bad_pages_num && !adapt->umc.is_pmfw_managed_eeprom)
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
	uint32_t i;
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
	struct eeprom_table_record *temp_buff;
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	uint32_t original_cap;

	if (!*bp_buff || !cap)
		return AMDGV_FAILURE;

	temp_buff = *bp_buff;
	original_cap = *cap;
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

	/* Update sorted bad pages buffer capacity */
	if (data && data->sorted_bps && *cap > original_cap &&
	    (bp_buff == &data->bps || bp_buff == &data->rom_data.bps)) {
		uint64_t *temp_sorted_buff;
		uint32_t new_sorted_cap = *cap;

		/* Grow the sorted bad pages buffer to match the main buffer capacity */
		temp_sorted_buff = (uint64_t *)amdgv_umc_grow_bp_buff((void *)data->sorted_bps,
								     &new_sorted_cap, sizeof(uint64_t));
		if (!temp_sorted_buff) {
			AMDGV_ERROR("Failed to grow sorted bad pages buffer!\n");
			return AMDGV_FAILURE;
		}

		data->sorted_bps = temp_sorted_buff;
		data->sorted_bps_cap = new_sorted_cap;

		AMDGV_DEBUG("Updated sorted bad pages buffer capacity from %d to %d\n",
			   original_cap, data->sorted_bps_cap);
	}

	return 0;
}

int amdgv_umc_set_eeprom_record(struct amdgv_adapter *adapt,
		struct eeprom_table_record *record, struct amdgv_ras_eeprom_bad_page_info *bp_info)
{
	int ret;

	if (adapt->umc.funcs && adapt->umc.funcs->set_eeprom_record) {
		ret = adapt->umc.funcs->set_eeprom_record(adapt, record, bp_info);
		if (ret) {
			AMDGV_ERROR("Failed to set eeprom record\n");
			return ret;
		}
	}

	return 0;
}

/**
 * amdgv_umc_replace_bad_pages - replace and reserve bad pages that is currently in-use in pf fb
 *
 * save info of memmgr mem that are affected by bp
 * release all these mem
 * reserve all bp
 * allocate new mem with saved mem info
 **/
int amdgv_umc_replace_bad_pages(struct amdgv_adapter *adapt)
{
	uint64_t err_addr;
	struct amdgv_memmgr_mem *old, *new, *prev;
	struct amdgv_memmgr *memmgr;
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	int i, ret = 0;
	int resv_count = data->count <= BAD_PAGE_RECORD_THRESHOLD ? data->count : BAD_PAGE_RECORD_THRESHOLD;

	struct amdgv_list_head list_tmp;
	struct amdgv_memmgr_mem *alloc, *alloc_tmp;

	memmgr = &adapt->memmgr_pf;

	if (amdgv_ras_eeprom_is_gpu_bad(adapt))
		goto exit;
	if (!resv_count)
		goto exit;

	if (!memmgr)
		goto exit;

	oss_mutex_lock(adapt->ecc.recovery_lock);


	AMDGV_INIT_LIST_HEAD(&list_tmp);

	for (i = 0; i < resv_count; i++) {
		err_addr = data->bps[i].retired_page << AMDGV_GPU_PAGE_SHIFT;
		/* look for bad page using memmgr offset and size*/
		if (amdgv_umc_is_bp_in_range(err_addr, memmgr->offset, memmgr->size)) {
			if (!data->bps_mem[i]) {
				oss_mutex_lock(memmgr->lock);
				/**
				 * for each mem that has overlap with bad address
				 * if the mem is bad page, save to eh data list
				 * for the mem at current tom, update tom to next highest
				 * move out the mem if it is not bad page
				 **/
				old = amdgv_memmgr_find_mem_at_offset(memmgr, err_addr, AMDGV_GPU_PAGE_SIZE);
				while (old) {
					if (MEM_ECC_BAD_PAGE == MEM_ID_GET_ID(old->id)) {
						data->bps_mem[i] = old;
						break;
					}
					/* update memmgr tom */
					if (old == amdgv_list_last_entry(&memmgr->allocs->node, struct amdgv_memmgr_mem, node)) {
						prev = amdgv_list_last_entry(&old->node, struct amdgv_memmgr_mem, node);
						memmgr->tom = prev->alloc_off;
					}
					/* move out mem that contain bad page */
					amdgv_list_move_entry(&old->node, &list_tmp);
					old = amdgv_memmgr_find_mem_at_offset(memmgr, err_addr, AMDGV_GPU_PAGE_SIZE);
				}
				oss_mutex_unlock(memmgr->lock);
				/* reserve bad page */
				if (!data->bps_mem[i])
					data->bps_mem[i] = amdgv_memmgr_alloc_align_at(
						&adapt->memmgr_pf, err_addr, AMDGV_GPU_PAGE_SIZE,
						MEM_ECC_BAD_PAGE);

				if (!data->bps_mem[i])
					AMDGV_WARN("No page reserved at 0x%lx in memmgr_pf \n",
						err_addr);
			}
		}
	}

	amdgv_list_for_each_entry_safe(alloc, alloc_tmp, &list_tmp, struct amdgv_memmgr_mem, node) {
		if (alloc->len == 0)
			continue;

		/**
		 * mem_id used does not affect replaced memory which always use the previous id
		 * use param mem type MEM_ECC_BAD_PAGE to always allocate new mem
		**/
		if (MEM_ECC_BAD_PAGE == MEM_ID_GET_ID(alloc->id))
			AMDGV_WARN("attempting to replace bad page type mem\n");
		new = amdgv_memmgr_alloc_and_replace(adapt, memmgr, alloc, MEM_ECC_BAD_PAGE);
		if (new) {
			/**
			 * now 'new' is swapped into list_tmp and holding 'alloc''s offset/size
			 * loop node 'alloc' is holding new allocated offset and size and is placed in memmgr alloc list
			**/

			/* release node in list_tmp */
			amdgv_list_del(&new->node);
			if (new->sys_mem.handle)
				oss_free_dma_mem(new->sys_mem.handle);
			oss_free(new);
		} else {
			/* no new mem allocation available, return failure */
			ret = AMDGV_FAILURE;
			break;
		}
	}

	if (!amdgv_list_empty(&list_tmp)) {
		amdgv_list_for_each_entry_safe(alloc, alloc_tmp, &list_tmp, struct amdgv_memmgr_mem, node) {
			amdgv_list_del(&alloc->node);
			if (alloc->sys_mem.handle)
				oss_free_dma_mem(alloc->sys_mem.handle);
			oss_free(alloc);
		}
	}

	oss_mutex_unlock(adapt->ecc.recovery_lock);
exit:
	adapt->ecc.eh_data->bp_replace_pending = false;
	return ret;
}

/* Initialize sorted bad pages array */
int amdgv_umc_init_sorted_bad_pages(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;

	if (!data)
		return AMDGV_FAILURE;

	/* Allocate sorted bad pages array with same capacity as regular bps */
	data->sorted_bps_cap = data->bps_cap;
	data->sorted_bps = oss_zalloc(data->sorted_bps_cap * sizeof(uint64_t));
	if (!data->sorted_bps) {
		AMDGV_ERROR("Failed to allocate sorted bad pages array\n");
		return AMDGV_FAILURE;
	}

	data->sorted_bp_count = 0;

	AMDGV_INFO("Initialized sorted bad pages array with capacity %d\n", data->sorted_bps_cap);

	return 0;
}

/* Cleanup sorted bad pages array */
void amdgv_umc_cleanup_sorted_bad_pages(struct amdgv_adapter *adapt)
{
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	/* Check if recovery lock is still valid before using it */

	if (!data)
		return;

	if (data->sorted_bps) {
		oss_free(data->sorted_bps);
		data->sorted_bps = NULL;
	}

	data->sorted_bp_count = 0;
	data->sorted_bps_cap = 0;
}

/* Insert a new bad page into the sorted array using binary search */
int amdgv_umc_insert_sorted_bad_page(struct amdgv_adapter *adapt, uint64_t page_addr,
										struct ras_err_handler_data *data)
{
	int left, right, mid;
	int insert_pos;
	int i;
	uint64_t fb_offset, mc_addr;

	if (!data || !data->sorted_bps)
		return AMDGV_FAILURE;

	/* Check if array is full */
	if (data->sorted_bp_count >= data->sorted_bps_cap) {
		AMDGV_ERROR("Sorted bad pages array is full\n");
		return AMDGV_FAILURE;
	}

	/* Convert FB page address to MC page address */
	fb_offset = page_addr << AMDGV_GPU_PAGE_SHIFT;
	mc_addr = fb_offset;

	/* Convert FB offset to MC address */
	mc_addr += adapt->mc_fb_loc_addr;
	if (adapt->xgmi.phy_nodes_num > 1)
			mc_addr += adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size;

	page_addr = mc_addr >> AMDGV_GPU_PAGE_SHIFT;

	/* Binary search to find insertion position */
	left = 0;
	right = data->sorted_bp_count - 1;
	insert_pos = data->sorted_bp_count; /* Default to end */

	while (left <= right) {
		mid = (left + right) / 2;
		if (data->sorted_bps[mid] == page_addr) {
			/* Page already exists, don't insert duplicate */
			AMDGV_DEBUG("Bad page 0x%llx already exists in sorted list\n", page_addr);
			return 0;
		} else if (data->sorted_bps[mid] < page_addr) {
			left = mid + 1;
		} else {
			insert_pos = mid;
			right = mid - 1;
		}
	}

	/* Shift elements to make room for new entry */
	for (i = data->sorted_bp_count; i > insert_pos; i--) {
		data->sorted_bps[i] = data->sorted_bps[i - 1];
	}

	/* Insert new page */
	data->sorted_bps[insert_pos] = page_addr;
	data->sorted_bp_count++;

	AMDGV_DEBUG("Inserted bad page 0x%llx at position %d, total count: %d\n",
			page_addr, insert_pos, data->sorted_bp_count);

	return 0;
}

/* Bubble sort the existing bp by offsets and remove duplicate if necessary */
static int amdgv_umc_sort_bp_offsets(uint64_t *bp_offsets, uint32_t num_bps)
{
	int i, j = 0;
	uint64_t temp;
	uint32_t unique_bp_idx;

	if (num_bps <= 1)
		return AMDGV_FAILURE;

	if (bp_offsets == NULL)
		return AMDGV_FAILURE;

	/* Sorting */
	for (i = 0; i < num_bps - 1; i++) {
		for (j = 0; j < num_bps - i - 1; j++) {
			if (bp_offsets[j] > bp_offsets[j + 1]) {
				temp = bp_offsets[j];
				bp_offsets[j] = bp_offsets[j + 1];
				bp_offsets[j + 1] = temp;
			}
		}
	}

	/* Remove duplicate */
	unique_bp_idx = 0;
	for (i = 1; i < num_bps; i++) {
		if (bp_offsets[i] != bp_offsets[unique_bp_idx]) {
			unique_bp_idx++;
			bp_offsets[unique_bp_idx] = bp_offsets[i];
		}
	}

	return (unique_bp_idx + 1);
}

/* This function fetches the raw bad page offsets.
 * There are 16 pages reserved per raw bad page offset.
 * The user is responsible for freeing bp_offsets.
 */
int amdgv_umc_fetch_and_sort_bps(struct amdgv_adapter *adapt, uint64_t **bp_offsets)
{
	uint64_t *tmp_bp_offsets = NULL;
	struct ras_err_handler_data *data = adapt->ecc.eh_data;
	int ret = 0, i = 0;

	if (bp_offsets == NULL)
		return AMDGV_FAILURE;

	tmp_bp_offsets = oss_zalloc(data->count * sizeof(uint64_t));
	if (!tmp_bp_offsets)
		return AMDGV_FAILURE;

	for (i = 0; i < data->count; i++)
		tmp_bp_offsets[i] = data->bps[i].retired_page << AMDGV_GPU_PAGE_SHIFT;

	amdgv_umc_sort_bp_offsets(tmp_bp_offsets, data->count);

	*bp_offsets = tmp_bp_offsets;

	return ret;
}

/* This function fetches the raw bad page offsets across supported nps mode.
 * There are 16 pages reserved per raw bad page offset.
 * The user is responsible for freeing bp_offsets.
 */
int amdgv_umc_fetch_and_sort_bps_across_nps(struct amdgv_adapter *adapt, uint64_t **bp_offsets,
												int *bp_count)
{
	uint64_t *tmp_bp_offsets = NULL;
	struct ras_err_handler_data *data_across_nps = adapt->ecc.eh_data_across_nps;
	int ret = 0;
	int i, j, k = 0;
	int total_bps_count = 0;

	if (bp_offsets == NULL)
		return AMDGV_FAILURE;

	for (i = 0; i < adapt->ecc.supported_nps_count; i++) {
		total_bps_count += data_across_nps[i].count;
	}

	tmp_bp_offsets = oss_zalloc(total_bps_count * sizeof(uint64_t));
	if (!tmp_bp_offsets)
		return AMDGV_FAILURE;

	for (i = 0; i < adapt->ecc.supported_nps_count; i++) {
		for (j = 0; j < data_across_nps[i].count; j++) {
			tmp_bp_offsets[k] = data_across_nps[i].bps[j].retired_page << AMDGV_GPU_PAGE_SHIFT;
			k++;
		}
	}

	total_bps_count = amdgv_umc_sort_bp_offsets(tmp_bp_offsets, total_bps_count);

	*bp_offsets = tmp_bp_offsets;
	*bp_count = total_bps_count;

	return ret;
}

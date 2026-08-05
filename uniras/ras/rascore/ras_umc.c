// SPDX-License-Identifier: MIT
/*
 * Copyright 2025 Advanced Micro Devices, Inc.
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
#include "ras.h"
#include "ras_umc.h"
#include "ras_umc_v12_0.h"
#include "ras_umc_v15_0.h"

#define MAX_ECC_NUM_PER_RETIREMENT  16

/* bad page timestamp format
 * yy[31:27] mm[26:23] day[22:17] hh[16:12] mm[11:6] ss[5:0]
 */
#define EEPROM_TIMESTAMP_MINUTE  6
#define EEPROM_TIMESTAMP_HOUR    12
#define EEPROM_TIMESTAMP_DAY     17
#define EEPROM_TIMESTAMP_MONTH   23
#define EEPROM_TIMESTAMP_YEAR    27

static uint64_t ras_umc_get_eeprom_timestamp(struct ras_core_context *ras_core)
{
	struct ras_time tm = {0};
	uint64_t utc_timestamp = 0;
	uint64_t eeprom_timestamp = 0;

	utc_timestamp = ras_core_get_utc_second_timestamp(ras_core);
	if (!utc_timestamp)
		return utc_timestamp;

	ras_core_convert_timestamp_to_time(ras_core, utc_timestamp, &tm);

	/* the year range is 2000 ~ 2031, set the year if not in the range */
	if (tm.tm_year < 2000)
		tm.tm_year = 2000;
	if (tm.tm_year > 2031)
		tm.tm_year = 2031;

	tm.tm_year -= 2000;

	eeprom_timestamp = tm.tm_sec + (tm.tm_min << EEPROM_TIMESTAMP_MINUTE)
				+ (tm.tm_hour << EEPROM_TIMESTAMP_HOUR)
				+ (tm.tm_mday << EEPROM_TIMESTAMP_DAY)
				+ (tm.tm_mon << EEPROM_TIMESTAMP_MONTH)
				+ (tm.tm_year << EEPROM_TIMESTAMP_YEAR);
	eeprom_timestamp &= 0xffffffff;

	return eeprom_timestamp;
}

static const struct ras_umc_ip_func *ras_umc_get_ip_func(
				struct ras_core_context *ras_core, uint32_t ip_version)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;

	switch (ip_version) {
	case IP_VERSION(12, 0, 0):
	case IP_VERSION(12, 5, 0):
		ras_umc->max_pages_per_row = 16;
		return &ras_umc_func_v12_0;
	case IP_VERSION(15, 0, 0):
		ras_umc->max_pages_per_row = 128;
		return &ras_umc_func_v15_0;
	default:
		RAS_DEV_ERR(ras_core->dev,
			"UMC ip version(0x%x) is not supported!\n", ip_version);
		break;
	}

	return NULL;
}

int ras_umc_ras_ta_translate_addr(struct ras_core_context *ras_core,
		struct umc_mca_addr *in, struct umc_phy_addr *out,
		uint32_t nps)
{
	struct ras_ta_query_address_input addr_in;
	struct ras_ta_query_address_output addr_out;
	int ret;

	if (!in)
		return -RAS_CORE_EINVAL;

	oss_memset(&addr_in, 0, sizeof(addr_in));
	oss_memset(&addr_out, 0, sizeof(addr_out));

	addr_in.ma.err_addr = in->err_addr;
	addr_in.ma.ch_inst = in->ch_inst;
	addr_in.ma.umc_inst = in->umc_inst;
	addr_in.ma.node_inst = in->node_inst;
	addr_in.ma.socket_id = in->socket_id;

	addr_in.addr_type = RAS_TA_MCA_TO_PA;

	ret = ras_psp_query_address(ras_core, &addr_in, &addr_out);
	if (ret) {
		RAS_DEV_WARN(ras_core->dev,
			"Failed to query RAS physical address for 0x%llx, ret:%d",
			in->err_addr, ret);
		return -RAS_CORE_EREMOTEIO;
	}

	if (out) {
		out->pa = addr_out.pa.pa;
		out->bank = addr_out.pa.bank;
		out->channel_idx = addr_out.pa.channel_idx;
	}

	return 0;
}

int ras_umc_psp_translate_addr(struct ras_core_context *ras_core,
		struct umc_mca_addr *in, struct umc_phy_addr *out,
		uint32_t nps)
{
	struct ras_psp_addr_trans_in psp_in = {0};
	struct ras_psp_addr_trans_out psp_out = {0};
	int	ret;

	psp_in.mca_addr = in->mca_addr;
	psp_in.ipid = in->ipid;
	psp_in.nps = nps;

	ret = ras_psp_translate_addr(ras_core, &psp_in, &psp_out);
	if (ret)
		return ret;

	out->pa = psp_out.row_pa;
	out->pa_flip_mask = psp_out.pa_flip_mask;

	return 0;
}

static int ras_umc_log_ecc(struct ras_core_context *ras_core,
		unsigned long idx, void *data)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	int ret;

	oss_mutex_lock(&ras_umc->tree_lock);
	ret = oss_radix_tree_insert(&ras_umc->root, idx, data);
	oss_mutex_unlock(&ras_umc->tree_lock);

	return ret;
}

int ras_umc_clear_logged_ecc(struct ras_core_context *ras_core)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	uint64_t buf[8] = {0};
	void  **slot;
	void *data;
	void *iter = buf;

	oss_mutex_lock(&ras_umc->tree_lock);
	oss_radix_tree_for_each_slot(slot, &ras_umc->root, iter, 0) {
		data = oss_radix_tree_delete_iter(&ras_umc->root, iter);
		oss_free(data);
	}
	oss_mutex_unlock(&ras_umc->tree_lock);

	return 0;
}

int ras_umc_alloc_row_pages(struct ras_core_context *ras_core,
		uint64_t **page_pfns, uint32_t *nr_page_pfns)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	uint64_t *address;
	uint64_t page_num;

	if (!page_pfns || !nr_page_pfns)
		return -RAS_CORE_EINVAL;

	if (!ras_umc->max_pages_per_row) {
		RAS_DEV_ERR(ras_core->dev, "max_pages_per_row was not initialized!\n");
		return -RAS_CORE_EPERM;
	}

	page_num = ras_umc->max_pages_per_row;

	address = ras_calloc(page_num, sizeof(*address));
	if (!address)
		return -RAS_CORE_ENOMEM;

	*page_pfns = address;
	*nr_page_pfns = page_num;

	return 0;
}

int ras_umc_free_row_pages(struct ras_core_context *ras_core,
		uint64_t *page_pfns)
{
	if (!page_pfns)
		return -RAS_CORE_EINVAL;

	oss_free(page_pfns);

	return 0;
}

static int ras_umc_expand_row_pages(struct ras_core_context *ras_core,
	struct eeprom_umc_record *record, uint64_t *page_pfns, uint32_t nr_page_pfns)
{
	uint64_t retired_addr = RAS_PFN_TO_ADDR(record->cur_nps_retired_row_pfn);
	uint64_t flip_mask = record->cur_nps_pa_flip_mask;
	uint64_t subset;
	uint64_t row_pa, addr;
	uint32_t count = 0;

	if (!retired_addr || !flip_mask || !page_pfns || !nr_page_pfns)
		return -RAS_CORE_ENOEXEC;

	row_pa = retired_addr & ~(flip_mask);

	if (count < nr_page_pfns)
		page_pfns[count++] = RAS_ADDR_TO_PFN(row_pa);

	subset = flip_mask;
	while (subset) {
		addr = row_pa ^ subset;

		if (count >= nr_page_pfns)
			break;

		page_pfns[count++] = RAS_ADDR_TO_PFN(addr);

		subset = (subset - 1) & flip_mask;
	};

	return count;
}

int ras_umc_convert_record_to_row_pages(struct ras_core_context *ras_core,
	struct eeprom_umc_record *record, uint64_t *page_pfns, uint32_t nr_page_pfns)
{
	uint32_t new_nps;
	int ret, count = 0;

	if (!page_pfns || !nr_page_pfns || !record ||
	    (record->cur_nps > UMC_MEMORY_PARTITION_MODE_NPS8))
		return -RAS_CORE_EINVAL;

	if (!record->cur_nps || !record->cur_nps_retired_row_pfn ||
	    !record->cur_nps_pa_flip_mask) {
		new_nps = record->cur_nps ?
			record->cur_nps : ras_core_get_curr_nps_mode(ras_core);
		ret = ras_umc_record_to_nps_record(ras_core, record, new_nps);
		if (ret)
			return ret;
	}

	count = ras_umc_expand_row_pages(ras_core, record, page_pfns, nr_page_pfns);
	if (count > 0)
		record->cur_nps_valid_page_num = count;

	return count;
}

static void ras_umc_reserve_row_pages(struct ras_core_context *ras_core,
	struct eeprom_umc_record *record, uint64_t *pages, uint32_t nr_pages)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	int i;

	if (!pages || !nr_pages ||
		(nr_pages > ras_umc->max_pages_per_row))
		return;

	/* Reserve memory */
	for (i = 0; i < nr_pages; i++)
		ras_core_event_notify(ras_core, ras_core_in_early_init(ras_core) ?
			RAS_EVENT_ID__EARLY_INIT_RESERVE_PAGE : RAS_EVENT_ID__RESERVE_BAD_PAGE,
			&pages[i]);
}

/* When gpu reset is ongoing, ecc logging operations will be pended.
 */
int ras_umc_log_bad_bank_pending(struct ras_core_context *ras_core, struct ras_bank_ecc *bank)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	struct ras_bank_ecc_node *ecc_node;

	ecc_node = oss_zalloc(sizeof(*ecc_node));
	if (!ecc_node)
		return -RAS_CORE_ENOMEM;

	oss_memcpy(&ecc_node->ecc, bank, sizeof(ecc_node->ecc));

	oss_mutex_lock(&ras_umc->pending_ecc_lock);
	oss_list_add_tail(&ecc_node->node, &ras_umc->pending_ecc_list);
	oss_mutex_unlock(&ras_umc->pending_ecc_lock);

	return 0;
}

/* After gpu reset is complete, re-log the pending error banks.
 */
int ras_umc_log_pending_bad_bank(struct ras_core_context *ras_core)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	struct ras_bank_ecc_node *ecc_node, *tmp;

	oss_mutex_lock(&ras_umc->pending_ecc_lock);
	oss_list_for_each_entry_safe(ecc_node, tmp,
		&ras_umc->pending_ecc_list, struct ras_bank_ecc_node, node) {
		if (!ras_umc_log_bad_bank(ras_core, &ecc_node->ecc)) {
			oss_list_del(&ecc_node->node);
			oss_free(ecc_node);
		}
	}
	oss_mutex_unlock(&ras_umc->pending_ecc_lock);

	return 0;
}

int ras_umc_log_bad_bank(struct ras_core_context *ras_core, struct ras_bank_ecc *bank)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	struct eeprom_umc_record umc_rec = {0};
	uint32_t c = 0;
	int ret;

	oss_mutex_lock(&ras_umc->bank_log_lock);
	ret = ras_umc_bank_to_umc_record(ras_core, bank, &umc_rec);
	if (ret)
		goto out;

	ret = ras_umc_add_bad_pages(ras_core, &umc_rec, 1, &c);
	if (ret) {
		RAS_DEV_ERR(ras_core->dev, "Failed to log bad bank! ret:%x\n", ret);
		goto out;
	}

	if (c)
		ret = ras_core_event_notify(ras_core,
				RAS_EVENT_ID__BAD_PAGE_DETECTED, NULL);

out:
	oss_mutex_unlock(&ras_umc->bank_log_lock);
	return ret;
}

static bool ras_umc_check_logged_record(struct ras_core_context *ras_core,
		struct eeprom_umc_record *record)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	void *res = NULL;

	oss_mutex_lock(&ras_umc->tree_lock);
	res = oss_radix_tree_lookup(&ras_umc->root, record->cur_nps_retired_row_pfn);
	oss_mutex_unlock(&ras_umc->tree_lock);

	return res ? true : false;
}

static bool ras_umc_check_retired_record(struct ras_core_context *ras_core,
				struct eeprom_umc_record *record)
{
	uint32_t nps = 0;
	int ret;

	nps = ras_core_get_curr_nps_mode(ras_core);
	ret = ras_umc_record_to_nps_record(ras_core, record, nps);
	if (ret) {
		RAS_DEV_ERR(ras_core->dev, "Failed to translate nps record! ret:%d\n", ret);
		return true;
	}

	if (ras_umc_check_logged_record(ras_core, record))
		return true;

	return false;
}

static int ras_umc_log_record(struct ras_core_context *ras_core,
			struct eeprom_umc_record *record)
{
	struct eeprom_umc_record *rec;
	int ret;

	rec = oss_zalloc(sizeof(*rec));
	if (!rec)
		return -RAS_CORE_ENOMEM;

	oss_memcpy(rec, record, sizeof(*rec));

	ret = ras_umc_log_ecc(ras_core, rec->cur_nps_retired_row_pfn, rec);
	if (ret)
		oss_free(rec);

	return ret;
}

/* alloc/realloc bps array */
static int ras_umc_realloc_err_data_space(struct ras_core_context *ras_core,
		struct eeprom_store_record *data, int pages)
{
	unsigned int old_space = data->count + data->space_left;
	unsigned int new_space = old_space + pages;
	unsigned int align_space = OSS_ALIGN(new_space, 512);
	void *bps = oss_zalloc(align_space * sizeof(*data->bps));

	if (!bps)
		return -RAS_CORE_ENOMEM;

	if (data->bps) {
		oss_memcpy(bps, data->bps,
				data->count * sizeof(*data->bps));
		oss_free(data->bps);
	}

	data->bps = bps;
	data->space_left += align_space - old_space;
	return 0;
}

static int ras_umc_update_eeprom_rom_data(struct ras_core_context *ras_core,
		struct eeprom_umc_record *bps)
{
	struct eeprom_store_record *data = &ras_core->ras_umc.umc_err_data.rom_data;

	if (!data->space_left &&
		ras_umc_realloc_err_data_space(ras_core, data, 256)) {
		return	-RAS_CORE_ENOMEM;
	}

	oss_memcpy(&data->bps[data->count], bps, sizeof(*data->bps));
	data->count++;
	data->space_left--;

	/* update bad channel bitmap */
	if (bps->mem_channel < OSS_BITS_PER_TYPE(data->umc_channel_bitmap))
		data->umc_channel_bitmap |= 0x1ULL << bps->mem_channel;

	return 0;
}

static int ras_umc_update_eeprom_ram_data(struct ras_core_context *ras_core,
		struct eeprom_umc_record *bps, uint64_t *page_pfns, uint32_t nr_page_pfns)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	struct eeprom_store_record *data = &ras_umc->umc_err_data.ram_data;
	int j;

	if (!bps || !page_pfns || !nr_page_pfns ||
		(nr_page_pfns > ras_umc->max_pages_per_row))
		return -RAS_CORE_EINVAL;

	if (!data->space_left &&
		ras_umc_realloc_err_data_space(ras_core, data, 256))
		return -RAS_CORE_ENOMEM;

	for (j = 0; j < nr_page_pfns; j++) {
		bps->cur_nps_retired_row_pfn = page_pfns[j];
		oss_memcpy(&data->bps[data->count], bps, sizeof(*data->bps));
		data->count++;
		data->space_left--;
	}

	/* update bad channel bitmap */
	if (bps->mem_channel < OSS_BITS_PER_TYPE(data->umc_channel_bitmap))
		data->umc_channel_bitmap |= 0x1ULL << bps->mem_channel;

	return 0;
}

void ras_umc_report_badpage_info(struct ras_core_context *ras_core)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	struct eeprom_store_record *data = &ras_umc->umc_err_data.ram_data;

	if (ras_umc->last_record_count != data->count) {
		ras_umc->last_record_count = data->count;
		ras_core_event_notify(ras_core, RAS_EVENT_ID__UPDATE_BAD_PAGE_NUM,
			&ras_umc->last_record_count);
	}

	if (ras_umc->last_channel_bitmap != data->umc_channel_bitmap) {
		ras_umc->last_channel_bitmap = data->umc_channel_bitmap;
		ras_core_event_notify(ras_core, RAS_EVENT_ID__UPDATE_BAD_CHANNEL_BITMAP,
			&ras_umc->last_channel_bitmap);
	}

	if (ras_core->is_rma)
		ras_core_event_notify(ras_core, RAS_EVENT_ID__DEVICE_RMA, NULL);
}

int ras_umc_add_bad_pages(struct ras_core_context *ras_core,
	struct eeprom_umc_record *bps, uint32_t bps_sz, uint32_t *valid_sz)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	uint64_t *page_pfns = NULL;
	uint32_t nr_page_pfns = 0;
	int nr_valid_pfns = 0;
	uint32_t i, c = 0;
	int ret = 0;

	if (!bps || !bps_sz || !valid_sz)
		return -RAS_CORE_EINVAL;

	ret = ras_umc_alloc_row_pages(ras_core, &page_pfns, &nr_page_pfns);
	if (ret)
		return ret;

	oss_mutex_lock(&ras_umc->umc_lock);
	for (i = 0; i < bps_sz; i++) {
		if (ras_umc_check_retired_record(ras_core, &bps[i]))
			continue;

		nr_valid_pfns = ras_umc_convert_record_to_row_pages(ras_core,
					&bps[i], page_pfns, nr_page_pfns);
		if (nr_valid_pfns < 0) {
			RAS_DEV_ERR(ras_core->dev,
				"Failed to lookup record bad pages! %d\n", nr_valid_pfns);
			ret = nr_valid_pfns;
			goto out;
		}

		ret = ras_umc_update_eeprom_rom_data(ras_core, &bps[i]);
		if (ret)
			goto out;

		ret = ras_umc_log_record(ras_core, &bps[i]);
		if (ret)
			goto out;

		ras_umc_reserve_row_pages(ras_core,
			&bps[i], page_pfns, nr_valid_pfns);

		ret = ras_umc_update_eeprom_ram_data(ras_core,
				&bps[i], page_pfns, nr_valid_pfns);
		if (ret)
			goto out;
		c++;
	}

	*valid_sz = c;

	if (c) {
		ras_eeprom_mgr_check_and_report_status(ras_core, true);
		if (!ras_core_in_early_init(ras_core))
			ras_umc_report_badpage_info(ras_core);
	}

out:
	oss_mutex_unlock(&ras_umc->umc_lock);
	ras_umc_free_row_pages(ras_core, page_pfns);
	return ret;
}

/*
 * read error record array in eeprom and reserve enough space for
 * storing new bad pages
 */
int ras_umc_load_bad_pages(struct ras_core_context *ras_core)
{
	struct eeprom_umc_record *bps;
	uint32_t c = 0;
	int ras_num_recs, ret;

	ras_num_recs = ras_eeprom_mgr_get_record_count(ras_core);
	/* no bad page record, skip eeprom access */
	if (ras_num_recs <= 0)
		return ras_num_recs;

	bps = ras_calloc(ras_num_recs, sizeof(*bps));
	if (!bps)
		return -RAS_CORE_ENOMEM;

	ret = ras_eeprom_mgr_get_records(ras_core, 0, bps, ras_num_recs);
	if (ret)
		RAS_DEV_ERR(ras_core->dev,
			"Failed to load EEPROM table records! ret:%d\n", ret);
	else
		ret = ras_umc_add_bad_pages(ras_core, bps, ras_num_recs, &c);

	oss_free(bps);
	return ret;
}

/*
 * write error record array to eeprom, the function should be
 * protected by recovery_lock
 * new_cnt: new added UE count, excluding reserved bad pages, can be NULL
 */
static int ras_umc_save_bad_pages(struct ras_core_context *ras_core)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	struct eeprom_store_record *data = &ras_umc->umc_err_data.rom_data;
	int eeprom_record_num;
	int save_count;
	int ret = -RAS_CORE_ENODATA;

	/* Not need to save bad pages when FW management EEPROM is enabled. */
	if (ras_eeprom_mgr_fw_record_enabled(ras_core))
		return 0;

	if (!data->bps)
		return -RAS_CORE_EINVAL;

	eeprom_record_num = ras_eeprom_mgr_get_record_count(ras_core);
	if (eeprom_record_num < 0)
		return eeprom_record_num;

	oss_mutex_lock(&ras_umc->umc_lock);
	save_count = data->count - eeprom_record_num;
	/* only new entries are saved */
	if (save_count > 0) {
		ret = ras_eeprom_mgr_append_records(ras_core,
				&data->bps[eeprom_record_num], save_count);
		if (ret) {
			RAS_DEV_ERR(ras_core->dev,
				"Failed to save EEPROM table data! ret:%d\n", ret);
			ret = -RAS_CORE_EIO;
			goto exit;
		}

		RAS_DEV_INFO(ras_core->dev, "Saved %d records to EEPROM table.\n", save_count);
	}

exit:
	oss_mutex_unlock(&ras_umc->umc_lock);
	return ret;
}

int ras_umc_handle_bad_pages(struct ras_core_context *ras_core, void *data)
{
	return ras_umc_save_bad_pages(ras_core);
}

int ras_umc_sw_init(struct ras_core_context *ras_core)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;

	oss_memset(ras_umc, 0, sizeof(*ras_umc));

	OSS_INIT_LIST_HEAD(&ras_umc->pending_ecc_list);

	oss_radix_tree_init(&ras_umc->root);

	oss_mutex_init_raw(&ras_umc->tree_lock);
	oss_mutex_init_raw(&ras_umc->pending_ecc_lock);
	oss_mutex_init_raw(&ras_umc->umc_lock);
	oss_mutex_init_raw(&ras_umc->bank_log_lock);

	ras_umc->umc_ip_version = ras_core->config->umc_ip_version;
	ras_umc->ip_func = ras_umc_get_ip_func(ras_core, ras_umc->umc_ip_version);
	if (!ras_umc->ip_func) {
		RAS_DEV_ERR(ras_core->dev, "Failed to get umc ip function!\n");
		return -RAS_CORE_EINVAL;
	}

	return 0;
}

int ras_umc_sw_fini(struct ras_core_context *ras_core)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	struct ras_umc_err_data *umc_err_data = &ras_umc->umc_err_data;
	struct ras_bank_ecc_node *ecc_node, *tmp;

	oss_mutex_destroy_raw(&ras_umc->umc_lock);
	oss_mutex_destroy_raw(&ras_umc->bank_log_lock);

	if (umc_err_data->rom_data.bps) {
		umc_err_data->rom_data.count = 0;
		oss_free(umc_err_data->rom_data.bps);
		umc_err_data->rom_data.bps = NULL;
		umc_err_data->rom_data.space_left = 0;
	}

	if (umc_err_data->ram_data.bps) {
		umc_err_data->ram_data.count = 0;
		oss_free(umc_err_data->ram_data.bps);
		umc_err_data->ram_data.bps = NULL;
		umc_err_data->ram_data.space_left = 0;
	}

	ras_umc_clear_logged_ecc(ras_core);

	oss_mutex_lock(&ras_umc->pending_ecc_lock);
	oss_list_for_each_entry_safe(ecc_node, tmp,
		&ras_umc->pending_ecc_list, struct ras_bank_ecc_node, node) {
		oss_list_del(&ecc_node->node);
		oss_free(ecc_node);
	}
	oss_mutex_unlock(&ras_umc->pending_ecc_lock);

	oss_mutex_destroy_raw(&ras_umc->tree_lock);
	oss_mutex_destroy_raw(&ras_umc->pending_ecc_lock);

	return 0;
}

int ras_umc_hw_init(struct ras_core_context *ras_core)
{
	int count = ras_umc_get_badpage_count(ras_core);

	/* For preloaded bad pages case, bad page info is
	 * deferred to report in hw init.
	 */
	if (count > 0)
		ras_umc_report_badpage_info(ras_core);

	return 0;
}

int ras_umc_hw_fini(struct ras_core_context *ras_core)
{
	return 0;
}

int ras_umc_clean_badpage_data(struct ras_core_context *ras_core)
{
	struct ras_umc_err_data *data = &ras_core->ras_umc.umc_err_data;

	oss_mutex_lock(&ras_core->ras_umc.umc_lock);

	oss_free(data->rom_data.bps);
	oss_free(data->ram_data.bps);

	oss_memset(data, 0, sizeof(*data));
	oss_mutex_unlock(&ras_core->ras_umc.umc_lock);

	return 0;
}

int ras_umc_fill_eeprom_record(struct ras_core_context *ras_core,
		uint64_t err_addr, uint32_t umc_inst, struct umc_phy_addr *cur_nps_addr,
		enum umc_memory_partition_mode cur_nps, struct eeprom_umc_record *record)
{
	struct eeprom_umc_record *err_rec = record;

	/* Set bad page pfn and nps mode */
	EEPROM_RECORD_SETUP_UMC_ADDR_AND_NPS(err_rec,
			RAS_ADDR_TO_PFN(cur_nps_addr->pa), cur_nps);

	err_rec->address = err_addr;
	err_rec->ts = ras_umc_get_eeprom_timestamp(ras_core);
	err_rec->err_type = RAS_EEPROM_ERR_NON_RECOVERABLE;
	err_rec->cu = 0;
	err_rec->mem_channel = cur_nps_addr->channel_idx;
	err_rec->mcumc_id = umc_inst;
	err_rec->cur_nps_retired_row_pfn = RAS_ADDR_TO_PFN(cur_nps_addr->pa);
	err_rec->cur_nps_pa_flip_mask = cur_nps_addr->pa_flip_mask;
	err_rec->cur_nps_bank = cur_nps_addr->bank;
	err_rec->cur_nps = cur_nps;
	return 0;
}

int ras_umc_get_saved_eeprom_count(struct ras_core_context *ras_core)
{
	struct ras_umc_err_data *err_data = &ras_core->ras_umc.umc_err_data;

	return err_data->rom_data.count;
}

int ras_umc_get_badpage_count(struct ras_core_context *ras_core)
{
	struct eeprom_store_record *data = &ras_core->ras_umc.umc_err_data.ram_data;

	return data->count;
}

int ras_umc_get_badpage_record(struct ras_core_context *ras_core, uint32_t index, void *record)
{
	struct eeprom_store_record *data = &ras_core->ras_umc.umc_err_data.ram_data;

	if (index >= data->count)
		return -RAS_CORE_EINVAL;

	oss_memcpy(record, &data->bps[index], sizeof(struct eeprom_umc_record));
	return 0;
}

bool ras_umc_check_retired_addr(struct ras_core_context *ras_core, uint64_t addr)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	struct eeprom_store_record *data = &ras_umc->umc_err_data.ram_data;
	uint64_t page_pfn = RAS_ADDR_TO_PFN(addr);
	int i, ret = false;

	oss_mutex_lock(&ras_umc->umc_lock);
	for (i = 0; i < data->count; i++) {
		if (data->bps[i].cur_nps_retired_row_pfn == page_pfn) {
			ret = true;
			break;
		}
	}
	oss_mutex_unlock(&ras_umc->umc_lock);

	return ret;
}

int ras_umc_translate_soc_pa_and_bank(struct ras_core_context *ras_core,
	uint64_t *soc_pa, struct umc_bank_addr *bank_addr, bool bank_to_pa)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	int ret = 0;

	if (bank_to_pa)
		ret = ras_umc->ip_func->bank_to_soc_pa(ras_core, *bank_addr, soc_pa);
	else
		ret = ras_umc->ip_func->soc_pa_to_bank(ras_core, *soc_pa, bank_addr);

	return ret;
}

int ras_umc_bank_to_umc_record(struct ras_core_context *ras_core,
		struct ras_bank_ecc *bank, struct eeprom_umc_record *record)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;
	int ret;

	if (!bank || !record || !ras_umc->ip_func ||
	    !ras_umc->ip_func->bank_to_eeprom_record)
		return -RAS_CORE_EINVAL;

	ret = ras_umc->ip_func->bank_to_eeprom_record(ras_core, bank, record);
	if (ret)
		return ret;

	record->ipid = bank->ipid;

	return 0;
}

int ras_umc_record_to_nps_record(struct ras_core_context *ras_core,
		struct eeprom_umc_record *record,  uint32_t nps)
{
	struct ras_umc *ras_umc = &ras_core->ras_umc;

	if (!record || !nps ||
		(nps >= UMC_MEMORY_PARTITION_MODE_UNKNOWN))
		return -RAS_CORE_EINVAL;

	/* Avoid redundant conversion for the same NPS mode */
	if ((record->cur_nps == nps) && record->cur_nps_retired_row_pfn &&
	    record->cur_nps_pa_flip_mask)
		return 0;

	if (!ras_umc->ip_func || !ras_umc->ip_func->eeprom_record_to_nps_record)
		return -RAS_CORE_EOPNOTSUPP;

	return ras_umc->ip_func->eeprom_record_to_nps_record(ras_core, record, nps);
}

int ras_umc_dump_fw_records(struct ras_core_context *ras_core)
{
	struct eeprom_umc_record rec;
	int eeprom_count, umc_count, new_count = 0;
	uint32_t c;
	int i, ret;

	eeprom_count = ras_eeprom_mgr_get_record_count(ras_core);
	/* no bad page record, skip eeprom access */
	if (eeprom_count <= 0)
		return eeprom_count;

	umc_count = ras_umc_get_saved_eeprom_count(ras_core);
	if (umc_count == eeprom_count) {
		return 0;
	} else if (umc_count > eeprom_count) {
		RAS_DEV_ERR(ras_core->dev, "Invalid error count: eeprom:%d, umc:%d\n",
			eeprom_count, umc_count);
		return 0;
	}

	for (i = umc_count; i < eeprom_count; i++) {
		oss_memset(&rec, 0, sizeof(rec));
		ret = ras_eeprom_mgr_get_records(ras_core, i, &rec, 1);
		if (ret)
			return 0;

		c = 0;
		ret = ras_umc_add_bad_pages(ras_core, &rec, 1, &c);
		if (!ret)
			new_count += c;
	}

	return new_count;
}

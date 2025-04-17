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

#include "amdgv_device.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_cper.h"
#include "amdgv_vfmgr.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

#define IS_LEAP_YEAR(x) ((x % 4 == 0 && x % 100 != 0) || x % 400 == 0)

#pragma pack(1)

static struct cper_guid MCE             = CPER_NOTIFY_MCE;
static struct cper_guid CMC             = CPER_NOTIFY_CMC;
static struct cper_guid BOOT            = BOOT_TYPE;


static struct cper_guid CRASHDUMP       = AMD_CRASHDUMP;
static struct cper_guid RUNTIME         = AMD_GPU_NONSTANDARD_ERROR;

#pragma pack()

static uint32_t seconds_per_day = 24 * 60 * 60;
static uint32_t seconds_per_hour = 60 * 60;
static uint32_t seconds_per_minute = 60;

static void __inc_entry_length(struct cper_hdr *hdr, uint32_t size)
{
	hdr->record_length += size;
}

static void amdgv_cper_fill_timestamp(struct cper_timestamp *ts,
				      uint64_t utc_timestamp)
{
	uint64_t year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
	uint32_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	uint64_t days = utc_timestamp / seconds_per_day;
	uint64_t remaining_seconds = utc_timestamp % seconds_per_day;

	/* utc_timestamp follows the Unix epoch */
	year = 1970;
	while (days >= 365) {
		if (IS_LEAP_YEAR(year)) {
			if (days < 366)
				break;
			days -= 366;
		} else {
			days -= 365;
		}
		year++;
	}

	days_in_month[1] += IS_LEAP_YEAR(year);

	month = 0;
	while (days >= days_in_month[month]) {
		days -= days_in_month[month];
		month++;
	}
	month++;
	day = days + 1;

	if (remaining_seconds) {
		hour = remaining_seconds / seconds_per_hour;
		minute = (remaining_seconds % seconds_per_hour) / seconds_per_minute;
		second = remaining_seconds % seconds_per_minute;
	}

	ts->seconds = second;
	ts->minutes = minute;
	ts->hours = hour;
	ts->day = day;
	ts->month = month;
	ts->century = (year / 100) + 1;
	ts->year = year % 100;
}

void amdgv_cper_entry_fill_hdr(struct amdgv_adapter *adapt,
			       struct cper_hdr *hdr,
			       enum amdgv_cper_type type,
			       enum cper_error_severity sev)
{
	uint32_t libgv_major_version = 0;
	uint32_t libgv_minor_version = 0;

	amdgv_get_version((int *)&libgv_major_version, (int *)&libgv_minor_version);

	hdr->signature[0]		= 'C';
	hdr->signature[1]		= 'P';
	hdr->signature[2]		= 'E';
	hdr->signature[3]		= 'R';
	hdr->revision			= CPER_HDR_REV_1;
	hdr->signature_end		= 0xFFFFFFFF;
	hdr->error_severity		= sev;

	hdr->valid_bits.platform_id	= 1;
	hdr->valid_bits.timestamp	= 1;

	oss_vsnprintf(hdr->record_id, 8, "%d:%X", adapt->xgmi.socket_id,
		      oss_atomic_inc_return(&adapt->cper.next_uid));
	amdgv_cper_fill_timestamp(&hdr->timestamp, oss_get_utc_time_stamp());
	oss_vsnprintf(hdr->creator_id, 16, "%s:%08X",
		      CPER_CREATOR_ID_AMDGV,
		      adapt->pp.smu_fw_version);
	oss_vsnprintf(hdr->platform_id, 16, "0x%04X:0x%04X",
		      adapt->vendor_id, adapt->dev_id);

	switch (type) {
	case AMDGV_CPER_TYPE_BOOT:
		hdr->notify_type = BOOT;
		break;
	case AMDGV_CPER_TYPE_FATAL:
	case AMDGV_CPER_TYPE_BP_THR: /* Fallthrough */
		hdr->notify_type = MCE;
		break;
	case AMDGV_CPER_TYPE_RUNTIME:
		if (sev == CPER_SEV_NON_FATAL_CORRECTED)
			hdr->notify_type = CMC;
		else
			hdr->notify_type = MCE;
		break;
	default:
		AMDGV_ERROR("Unknown CPER Type\n");
		break;
	}

	__inc_entry_length(hdr, HDR_LEN);
}

static int amdgv_cper_entry_fill_section_desc(struct amdgv_adapter *adapt,
					      struct cper_sec_desc *section_desc,
					      bool bp_threshold,
					      bool poison,
					      enum cper_error_severity sev,
					      struct cper_guid sec_type,
					      uint32_t section_length,
					      uint32_t section_offset)
{
	section_desc->revision_minor		= CPER_SEC_MINOR_REV_1;
	section_desc->revision_major		= CPER_SEC_MAJOR_REV_22;
	section_desc->sec_offset		= section_offset;
	section_desc->sec_length		= section_length;
	section_desc->valid_bits.fru_id		= 1;
	section_desc->valid_bits.fru_text	= 1;
	section_desc->flag_bits.primary		= 1;
	section_desc->severity			= sev;
	section_desc->sec_type			= sec_type;

	/* Named FRU ID in spec, but is really the Board Serial Number OOB. */
	oss_vsnprintf(section_desc->fru_text, 20, "OAM%d", adapt->xgmi.socket_id);
	oss_vsnprintf(section_desc->fru_id, 16, "%s", adapt->product_info.product_serial);

	if (bp_threshold)
		section_desc->flag_bits.exceed_err_threshold = 1;
	if (poison)
		section_desc->flag_bits.latent_err = 1;

	return 0;
}

int amdgv_cper_entry_fill_fatal_section(struct amdgv_adapter *adapt,
					struct cper_hdr *hdr,
					uint32_t idx,
					struct cper_sec_crashdump_reg_data reg_data)
{
	struct cper_sec_desc *section_desc;
	struct cper_sec_crashdump_fatal *section;

	section_desc = (struct cper_sec_desc *)((uint8_t *)hdr + SEC_DESC_OFFSET(idx));
	section = (struct cper_sec_crashdump_fatal *)((uint8_t *)hdr + FATAL_SEC_OFFSET(hdr->sec_cnt, idx));

	amdgv_cper_entry_fill_section_desc(adapt, section_desc, false, false,
					   CPER_SEV_FATAL, CRASHDUMP, FATAL_SEC_LEN,
					   FATAL_SEC_OFFSET(hdr->sec_cnt, idx));

	section->body.reg_ctx_type = CPER_CTX_TYPE_CRASH;
	section->body.reg_arr_size = sizeof(reg_data);
	section->body.data = reg_data;

	__inc_entry_length(hdr, SEC_DESC_LEN + FATAL_SEC_LEN);

	return 0;
}

int amdgv_cper_entry_fill_boot_section(struct amdgv_adapter *adapt,
				       struct cper_hdr *hdr,
				       uint32_t idx,
				       uint64_t *msg,
				       uint32_t msg_count)
{
	struct cper_sec_desc *section_desc;
	struct cper_sec_crashdump_boot *section;

	section_desc = (struct cper_sec_desc *)((uint8_t *)hdr + SEC_DESC_OFFSET(idx));
	section = (struct cper_sec_crashdump_boot *)((uint8_t *)hdr + BOOT_SEC_OFFSET(hdr->sec_cnt, idx));

	amdgv_cper_entry_fill_section_desc(adapt, section_desc, false, false,
					   CPER_SEV_FATAL, CRASHDUMP, BOOT_SEC_LEN,
					   BOOT_SEC_OFFSET(hdr->sec_cnt, idx));

	msg_count = min(msg_count, CPER_MAX_OAM_COUNT);

	section->body.reg_ctx_type = CPER_CTX_TYPE_BOOT;
	section->body.reg_arr_size = sizeof(section->body.msg);

	oss_memcpy(section->body.msg, msg, msg_count * sizeof(uint64_t));

	__inc_entry_length(hdr, SEC_DESC_LEN + BOOT_SEC_LEN);

	return 0;
}

int amdgv_cper_entry_fill_runtime_section(struct amdgv_adapter *adapt,
					  struct cper_hdr *hdr,
					  uint32_t idx,
					  enum cper_error_severity sev,
					  uint32_t *reg_dump,
					  uint32_t reg_count)
{
	struct cper_sec_desc *section_desc;
	struct cper_sec_nonstd_err *section;
	bool poison;

	poison = (sev == CPER_SEV_NON_FATAL_CORRECTED) ? false : true;
	section_desc = (struct cper_sec_desc *)((uint8_t *)hdr + SEC_DESC_OFFSET(idx));
	section = (struct cper_sec_nonstd_err *)((uint8_t *)hdr + NONSTD_SEC_OFFSET(hdr->sec_cnt, idx));

	amdgv_cper_entry_fill_section_desc(adapt, section_desc, false, poison,
					   sev, RUNTIME, NONSTD_SEC_LEN,
					   NONSTD_SEC_OFFSET(hdr->sec_cnt, idx));

	reg_count = min(reg_count, CPER_ACA_REG_COUNT);

	section->hdr.valid_bits.err_info_cnt = 1;
	section->hdr.valid_bits.err_context_cnt = 1;

	section->info.error_type = RUNTIME;
	section->info.ms_chk_bits.err_type_valid = 1;
	section->ctx.reg_ctx_type = CPER_CTX_TYPE_CRASH; /* 1 */
	section->ctx.reg_arr_size = sizeof(section->ctx.reg_dump);

	oss_memcpy(section->ctx.reg_dump, reg_dump, reg_count * sizeof(uint32_t));

	__inc_entry_length(hdr, SEC_DESC_LEN + NONSTD_SEC_LEN);

	return 0;
}

int amdgv_cper_entry_fill_bad_page_thr_section(struct amdgv_adapter *adapt,
					       struct cper_hdr *hdr,
					       uint32_t idx)
{
	struct cper_sec_desc *section_desc;
	struct cper_sec_nonstd_err *section;

	section_desc = (struct cper_sec_desc *)((uint8_t *)hdr + SEC_DESC_OFFSET(idx));
	section = (struct cper_sec_nonstd_err *)((uint8_t *)hdr + NONSTD_SEC_OFFSET(hdr->sec_cnt, idx));

	amdgv_cper_entry_fill_section_desc(adapt, section_desc, true, false,
					   CPER_SEV_FATAL, RUNTIME, NONSTD_SEC_LEN,
					   NONSTD_SEC_OFFSET(hdr->sec_cnt, idx));

	section->hdr.valid_bits.err_info_cnt = 1;
	section->hdr.valid_bits.err_context_cnt = 1;

	section->info.error_type = RUNTIME;
	section->info.ms_chk_bits.err_type_valid = 1;
	section->ctx.reg_ctx_type = CPER_CTX_TYPE_CRASH; /* 1 */
	section->ctx.reg_arr_size = sizeof(section->ctx.reg_dump);

	/* Hardcoded Reg dump for bad page threshold CPER */
	section->ctx.reg_dump[CPER_ACA_REG_CTL_LO]    = 0x1;
	section->ctx.reg_dump[CPER_ACA_REG_CTL_HI]    = 0x0;
	section->ctx.reg_dump[CPER_ACA_REG_STATUS_LO] = 0x137;
	section->ctx.reg_dump[CPER_ACA_REG_STATUS_HI] = 0xB0000000;
	section->ctx.reg_dump[CPER_ACA_REG_ADDR_LO]   = 0x0;
	section->ctx.reg_dump[CPER_ACA_REG_ADDR_HI]   = 0x0;
	section->ctx.reg_dump[CPER_ACA_REG_MISC0_LO]  = 0x0;
	section->ctx.reg_dump[CPER_ACA_REG_MISC0_HI]  = 0x0;
	section->ctx.reg_dump[CPER_ACA_REG_CONFIG_LO] = 0x2;
	section->ctx.reg_dump[CPER_ACA_REG_CONFIG_HI] = 0x1ff;
	section->ctx.reg_dump[CPER_ACA_REG_IPID_LO]   = 0x0;
	section->ctx.reg_dump[CPER_ACA_REG_IPID_HI]   = 0x96;
	section->ctx.reg_dump[CPER_ACA_REG_SYND_LO]   = 0x0;
	section->ctx.reg_dump[CPER_ACA_REG_SYND_HI]   = 0x0;

	__inc_entry_length(hdr, SEC_DESC_LEN + NONSTD_SEC_LEN);

	return 0;
}

int amdgv_cper_commit_entry(struct amdgv_adapter *adapt,
			    struct cper_hdr *hdr)
{
	uint32_t wr_idx = 0;

	if (!adapt->cper.enabled)
		return AMDGV_FAILURE;

	wr_idx = adapt->cper.wptr % CPER_MAX_COUNT;

	if (adapt->cper.ring[wr_idx])
		oss_free(adapt->cper.ring[wr_idx]);

	adapt->cper.ring[wr_idx] = hdr;
	adapt->cper.count++;
	adapt->cper.wptr++;

	return 0;
}

struct cper_hdr *amdgv_cper_alloc_entry(struct amdgv_adapter *adapt,
					enum amdgv_cper_type type,
					uint16_t section_count)
{
	struct cper_hdr *hdr = NULL;
	uint32_t size = 0;

	if (!adapt->cper.enabled)
		return NULL;

	size += HDR_LEN;
	size += (SEC_DESC_LEN * section_count);

	switch (type) {
	case AMDGV_CPER_TYPE_RUNTIME:
	case AMDGV_CPER_TYPE_BP_THR:
		size += (NONSTD_SEC_LEN * section_count);
		break;
	case AMDGV_CPER_TYPE_FATAL:
		size += (FATAL_SEC_LEN * section_count);
		break;
	case AMDGV_CPER_TYPE_BOOT:
		size += (BOOT_SEC_LEN * section_count);
		break;
	default:
		AMDGV_ERROR("Unknown CPER Type!\n");
		return 0;
	}

	hdr = oss_zalloc(size);
	if (!hdr) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL, size);
		return 0;
	}

	/* Save this early */
	hdr->sec_cnt = section_count;

	return hdr;
}

int amdgv_cper_sw_init(struct amdgv_adapter *adapt)
{
	if (adapt->opt.max_cper_count < 0) {
		adapt->cper.enabled = false;
		return 0;
	}

	if (adapt->opt.max_cper_count > 0 &&
	    adapt->opt.max_cper_count <= AMDGV_CPER_MAX_ALLOWED_COUNT)
		adapt->cper.max_count = (uint64_t)adapt->opt.max_cper_count;
	else
		adapt->cper.max_count = AMDGV_CPER_MAX_ALLOWED_COUNT;

	adapt->cper.enabled = true;

	return 0;
}

int amdgv_cper_sw_fini(struct amdgv_adapter *adapt)
{
	int i = 0;

	adapt->cper.enabled = 0;

	for (i = 0; i < CPER_MAX_COUNT; i++) {
		if (adapt->cper.ring[i]) {
			oss_free(adapt->cper.ring[i]);
			adapt->cper.ring[i] = NULL;
		}
	}

	adapt->cper.count = 0;
	adapt->cper.wptr = 0;

	return 0;
}

int amdgv_cper_get_count(struct amdgv_adapter *adapt,
			 uint64_t rptr, uint64_t *wptr,
			 uint64_t *avail_count,
			 uint64_t *size)
{
	uint64_t i = 0;

	*size = 0;

	*avail_count = adapt->cper.wptr - CPER_MOVE_TO_FIRST_VALID(rptr);
	*wptr = adapt->cper.wptr;

	for (i = CPER_MOVE_TO_FIRST_VALID(rptr); i < adapt->cper.wptr; i++)
		*size += ((struct cper_hdr *)adapt->cper.ring[i % CPER_MAX_COUNT])->record_length;

	return 0;
}

int amdgv_cper_get_entries(struct amdgv_adapter *adapt, uint64_t rptr,
			   void *buf, uint64_t buf_size,
			   uint64_t *write_count,
			   uint64_t *overflow_count,
			   uint64_t *left_size)
{
	uint32_t offset = 0;
	uint64_t i = 0;
	struct cper_hdr *hdr;

	*write_count = 0;
	*overflow_count = 0;
	*left_size = 0;

	*overflow_count = CPER_MOVE_TO_FIRST_VALID(rptr) - rptr;

	/* Fill user buffer */
	for (i = CPER_MOVE_TO_FIRST_VALID(rptr); i < adapt->cper.wptr; i++) {
		hdr = (struct cper_hdr *)adapt->cper.ring[i % CPER_MAX_COUNT];
		if (offset + hdr->record_length > buf_size)
			break;
		oss_memcpy(((char *)buf + offset), hdr, hdr->record_length);
		offset += hdr->record_length;
		(*write_count)++;
	}

	/* Report remaining buffer size */
	for (; i < adapt->cper.wptr; i++) {
		hdr = (struct cper_hdr *)adapt->cper.ring[i % CPER_MAX_COUNT];
		*left_size += hdr->record_length;
	}

	return 0;
}

struct cper_hdr *amdgv_cper_get_ring_entry(struct amdgv_adapter *adapt, uint64_t rptr)
{
	if (!adapt->cper.enabled)
		return NULL;

	return (struct cper_hdr *)adapt->cper.ring[rptr % CPER_MAX_COUNT];
}

enum amdgv_cper_type amdgv_cper_get_type(struct amdgv_adapter *adapt, struct cper_hdr *hdr, uint32_t idx)
{
	struct cper_sec_desc *section_desc = (struct cper_sec_desc *)((uint8_t *)hdr + SEC_DESC_OFFSET(idx));

	if (!oss_memcmp(&hdr->notify_type, &BOOT, sizeof(struct cper_guid)))
		return AMDGV_CPER_TYPE_BOOT;
	else if (!oss_memcmp(&section_desc->sec_type, &CRASHDUMP, sizeof(struct cper_guid)))
		return AMDGV_CPER_TYPE_FATAL;
	else if (!oss_memcmp(&section_desc->sec_type, &RUNTIME, sizeof(struct cper_guid)))
		return AMDGV_CPER_TYPE_RUNTIME;
	else
		return AMDGV_CPER_TYPE_MAX;
}

void amdgv_cper_get_crashdump(struct amdgv_adapter *adapt, struct cper_hdr *hdr, uint32_t idx,
				  struct cper_sec_crashdump_reg_data **reg_data)
{
	struct cper_sec_crashdump_fatal *section;

	section = (struct cper_sec_crashdump_fatal *)((uint8_t *)hdr + FATAL_SEC_OFFSET(hdr->sec_cnt, idx));
	*reg_data = &section->body.data;
}

void amdgv_cper_get_runtime_reg_dump(struct amdgv_adapter *adapt, struct cper_hdr *hdr, uint32_t idx,
				    uint32_t **reg_dump)
{
	struct cper_sec_nonstd_err *section;

	section = (struct cper_sec_nonstd_err *)((uint8_t *)hdr + NONSTD_SEC_OFFSET(hdr->sec_cnt, idx));
	*reg_dump = section->ctx.reg_dump;
}

static int amdgv_cper_patch_to_vf_hdr(struct amdgv_adapter *adapt, uint32_t idx_vf,
				      uint64_t fb_offset, struct cper_hdr *hdr,
				      uint32_t new_sec_count, uint32_t new_record_length,
				      uint32_t *checksum)
{
	int ret;
	uint32_t save_sec_count = hdr->sec_cnt;
	uint32_t save_record_length = hdr->record_length;

	/* Patch Info for VF */
	hdr->sec_cnt = new_sec_count;
	hdr->record_length = new_record_length;

	ret = amdgv_vfmgr_copy_and_calc_checksum_to_vf_fb(adapt, idx_vf,
							  fb_offset, (uint8_t *)(hdr),
							  HDR_LEN, checksum);

	/* restore */
	hdr->sec_cnt = save_sec_count;
	hdr->record_length = save_record_length;

	return ret;
}

static int amdgv_cper_patch_to_vf_desc(struct amdgv_adapter *adapt, uint32_t idx_vf, uint64_t fb_offset,
				       struct cper_sec_desc *desc, uint32_t new_sec_offset,
				       uint32_t *checksum)
{
	int ret;
	uint32_t save_sec_offset = desc->sec_offset;
	char save_fru_id[16] =  { 0 };
	oss_memcpy(save_fru_id, desc->fru_id, sizeof(save_fru_id));

	/* Patch info for VF */
	desc->valid_bits.fru_id = false;
	oss_memset(desc->fru_id, 0, sizeof(desc->fru_id));
	desc->sec_offset = new_sec_offset;

	ret = amdgv_vfmgr_copy_and_calc_checksum_to_vf_fb(adapt, idx_vf,
							  fb_offset, (uint8_t *)(desc),
							  SEC_DESC_LEN, checksum);

	/* Restore */
	desc->valid_bits.fru_id = true;
	oss_memcpy(desc->fru_id, save_fru_id, sizeof(save_fru_id));
	desc->sec_offset = save_sec_offset;

	return ret;
}

static int amdgv_cper_patch_to_vf_section(struct amdgv_adapter *adapt, uint32_t idx_vf,
					  uint64_t fb_offset, uint8_t *buf,
					  uint32_t size,  uint32_t *checksum)
{
	return amdgv_vfmgr_copy_and_calc_checksum_to_vf_fb(adapt, idx_vf,
							   fb_offset, buf,
							   size, checksum);
}

int amdgv_cper_patch_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			   struct cper_hdr *hdr, bool *allowed_sections,
			   uint32_t allowed_count, uint64_t *fb_offset,
			   uint32_t *checksum)
{
	int ret = 0;
	uint32_t write_count = 0;
	struct cper_sec_desc *sec_desc = NULL;
	uint64_t next_sec_offset;
	uint32_t i = 0;

	if (!allowed_count)
		return 0;

	next_sec_offset = HDR_LEN  + SEC_DESC_LEN * allowed_count;

	for (i = 0; i < hdr->sec_cnt; i++) {
		if (!allowed_sections[i])
			continue;

		sec_desc = (struct cper_sec_desc *)((uint8_t *)hdr + SEC_DESC_OFFSET(i));

		ret = amdgv_cper_patch_to_vf_section(adapt, idx_vf,
						     *fb_offset + next_sec_offset,
						     (uint8_t *)hdr + sec_desc->sec_offset,
						     sec_desc->sec_length, checksum);
		if (ret)
			break;

		ret = amdgv_cper_patch_to_vf_desc(adapt, idx_vf,
						  *fb_offset + HDR_LEN  + SEC_DESC_LEN * write_count,
						  sec_desc, next_sec_offset, checksum);
		if (ret)
			break;
		next_sec_offset += sec_desc->sec_length;
		write_count++;
	}

	if (!ret)
		ret = amdgv_cper_patch_to_vf_hdr(adapt, idx_vf, *fb_offset,
						 hdr, write_count, next_sec_offset,
						 checksum);

	if (!ret)
		(*fb_offset) += next_sec_offset;

	return ret;
}

/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_ras_eeprom.h"
#include "amdgv_ras_eeprom_internal.h"
#include "ras_eeprom_legacy.h"
#include "amdgv_powerplay.h"

typedef uint64_t __le64;
static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

int ras_eeprom_v2_1_process_records(struct amdgv_adapter *adapt,
				     struct amdgv_ras_eeprom_control *control,
				     struct eeprom_table_record *records, bool write, int num);

static int ras_eeprom_v2_1_format_i2c_msg(struct amdgv_adapter *adapt,
			struct amdgv_ras_eeprom_control *control,
			struct i2c_msg *msg, unsigned char *buff,
			uint16_t buff_len, uint32_t addr, bool write)
{
	/*
	 * Update bits 16,17 of EEPROM address in I2C address by setting them
	 * to bits 1,2 of Device address byte
	 *
	 * To check if we overflow page boundary
	 * https://www.st.com/resource/en/datasheet/m24m02-dr.pdf sec. 5.1.2
	 */
	msg->addr = control->i2c_address |
			((addr & EEPROM_ADDR_MSB_MASK) >> 15);
	msg->flags = write ? I2C_M_WR : I2C_M_RD;
	msg->len = buff_len; //EEPROM_ADDRESS_SIZE + EEPROM_TABLE_RECORD_SIZE;
	msg->buf = buff;

	/* Insert the EEPROM dest addess, bits 0-15 */
	buff[0] = ((addr >> 8) & 0xff);
	buff[1] = (addr & 0xff);

	return 0;
}

static inline int ras_eeprom_v2_1_i2c_allowed_xfer_len(uint32_t curr_tbl_offset)
{
	return (EEPROM_PAGE__SIZE_BYTES - (curr_tbl_offset % EEPROM_PAGE__SIZE_BYTES));
}

static void ras_eeprom_v2_1_encode_extra_info_to_buff(struct amdgv_ras_eeprom_control *control,
					       unsigned char *buff)
{
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 *egi_v2_1 = NULL;
	uint32_t *pp = (uint32_t *)buff;
	uint32_t tmp = 0;

	egi_v2_1 = &control->tbl_egi_v2_1;
	tmp = ((uint32_t)(egi_v2_1->rma_status) & 0xFF) |
		(((uint32_t)(egi_v2_1->gpu_healthy) << 8) & 0xFF00) |
		(((uint32_t)(egi_v2_1->ecc_page_threshold) << 16) & 0xFFFF0000);
	pp[0] = cpu_to_le32(tmp);
}

static void ras_eeprom_v2_1_decode_extra_info_from_buff(struct amdgv_ras_eeprom_control *control,
						 unsigned char *buff)
{
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 *egi_v2_1 = NULL;
	uint32_t *pp = (uint32_t *)buff;
	uint32_t tmp = 0;

	egi_v2_1 = &control->tbl_egi_v2_1;
	tmp = le32_to_cpu(pp[0]);
	egi_v2_1->rma_status = tmp & 0xFF;
	egi_v2_1->gpu_healthy = (tmp >> 8) & 0xFF;
	egi_v2_1->ecc_page_threshold = (tmp >> 16) & 0xFFFF;
}

static int ras_eeprom_v2_1_i2c_transfer_record(struct amdgv_adapter *adapt,
			struct amdgv_ras_eeprom_control *control,
			unsigned char *encoded_record, uint32_t encoded_record_len, bool write)
{
	unsigned char eeprom_buff[EEPROM_ADDRESS_SIZE + EEPROM_TABLE_RECORD_SIZE] = { 0 };
	struct i2c_msg msg = { 0 };
	int msg_num = 1;

	/* write record to EEPROM buff */
	if (write)
		oss_memcpy(eeprom_buff + EEPROM_ADDRESS_SIZE, encoded_record, encoded_record_len);

	ras_eeprom_v2_1_format_i2c_msg(adapt, control, &msg, eeprom_buff,
		sizeof(eeprom_buff), control->next_addr, write);

	if (msg_num != __smu_i2c_transfer(adapt, control, &msg, msg_num))
		return AMDGV_FAILURE;

	/* read record from EEPROM buff */
	if (!write)
		oss_memcpy(encoded_record, eeprom_buff + EEPROM_ADDRESS_SIZE, encoded_record_len);

	return 0;
}

static int ras_eeprom_v2_1_write_table_header(struct amdgv_adapter *adapt,
				 struct amdgv_ras_eeprom_control *control)
{
	unsigned char buff[EEPROM_ADDRESS_SIZE + EEPROM_TABLE_HEADER_SIZE] = { 0 };
	int ret = 0;
	struct i2c_msg msg = {
		.addr = control->i2c_address,
		.flags = 0,
		.len = EEPROM_ADDRESS_SIZE + EEPROM_TABLE_HEADER_SIZE,
		.buf = buff,
	};

	*(uint16_t *)buff = EEPROM_HDR_START;
	__encode_table_header_to_buff(&control->tbl_hdr, buff + EEPROM_ADDRESS_SIZE);

	ret = __smu_i2c_transfer(adapt, control, &msg, 1);
	if (ret < 1)
		AMDGV_ERROR("Failed to write EEPROM table header, ret:%d\n", ret);

	return ret;
}

/* write_reserved should be set to false if the EEPROM header has already been sanitized */
static int ras_eeprom_v2_1_write_table_header_ext(struct amdgv_adapter *adapt,
				   struct amdgv_ras_eeprom_control *control, bool write_reserved)
{
	unsigned char *buff = NULL;
	int ret = 0;
	uint32_t len = 0;
	struct i2c_msg msg = {
		.addr = control->i2c_address,
		.flags = 0,
	};

	if (write_reserved)
		len = EEPROM_ADDRESS_SIZE + EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE;
	else
		len = EEPROM_ADDRESS_SIZE + EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_SIZE;

	buff = (unsigned char *)oss_zalloc(len);
	if (!buff) {
		AMDGV_ERROR("Alloc memory to update extra info failed\n");
		return AMDGV_FAILURE;
	}
	msg.buf = buff;

	msg.len = len;
	buff[0] = ((EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_START >> 8) & 0xFF);
	buff[1] = (EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_START & 0xFF);

	ras_eeprom_v2_1_encode_extra_info_to_buff(control, buff + EEPROM_ADDRESS_SIZE);

	ret = __smu_i2c_transfer(adapt, control, &msg, 1);
	if (ret < 1)
		AMDGV_ERROR("Failed to write EEPROM extra gpu info, ret:%d\n", ret);

	oss_free(buff);

	return ret;
}

/* Encoded records have less bytes than libgv records. Trim them before calculating checksum */
static uint32_t ras_eeprom_v2_1_calc_recs_byte_sum(struct amdgv_ras_eeprom_control *control,
		struct eeprom_table_record *records, int num)
{
	int i, j;
	uint32_t tbl_sum = 0;
	unsigned char encoded_record[EEPROM_TABLE_RECORD_SIZE] = { 0 };

	/* Records checksum */
	for (i = 0; i < num; i++) {

		__encode_table_record_to_buff(control, &records[i], encoded_record);

		for (j = 0; j < EEPROM_TABLE_RECORD_SIZE; j++) {
			tbl_sum += *(((unsigned char *)encoded_record) + j);
		}
	}

	return tbl_sum;
}

static inline uint32_t ras_eeprom_v2_1_calc_tbl_byte_sum(struct amdgv_ras_eeprom_control *control,
					   struct eeprom_table_record *records, int num)
{
	return __calc_hdr_byte_sum(control) +
		__calc_extra_info_byte_sum(control) +
		ras_eeprom_v2_1_calc_recs_byte_sum(control, records, num);
}

/* 	Value chosen so that whole table adds up to 0 */
static void ras_eeprom_v2_1_update_tbl_checksum(struct amdgv_ras_eeprom_control *control,
				  struct eeprom_table_record *records, int num,
				  uint32_t old_hdr_byte_sum, uint32_t old_extra_info_byte_sum)
{
	control->tbl_byte_sum -= old_extra_info_byte_sum;
	control->tbl_byte_sum -= old_hdr_byte_sum;
	control->tbl_byte_sum += ras_eeprom_v2_1_calc_tbl_byte_sum(control, records, num);

	control->tbl_hdr.checksum = 256 - (control->tbl_byte_sum % 256);
}

static void ras_eeprom_v2_1_update_tbl_byte_sum(struct amdgv_adapter *adapt,
				    struct amdgv_ras_eeprom_control *control,
				    struct eeprom_table_record *records, int num)
{
	control->tbl_byte_sum = ras_eeprom_v2_1_calc_tbl_byte_sum(control, records, num);
}

static void ras_eeprom_v2_1_mark_gpu_healthy_status(struct amdgv_adapter *adapt,
	enum GPU_HEALTY_STATUS status)
{
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 *egi_v2_1 = &adapt->eeprom_control.tbl_egi_v2_1;

	egi_v2_1->rma_status = status;
	egi_v2_1->gpu_healthy = 0;
}

static enum GPU_HEALTY_STATUS ras_eeprom_v2_1_get_gpu_healthy_status(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 *egi_v2_1 = &adapt->eeprom_control.tbl_egi_v2_1;

	return egi_v2_1->rma_status;
}

static void ras_eeprom_v2_1_calc_gpu_healthy(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_eeprom_control *control = &adapt->eeprom_control;
	uint32_t num_recs = control->num_recs;
	uint32_t bad_page_threshold = BAD_PAGE_RECORD_THRESHOLD;
	uint32_t healthy_percent = 0;

	healthy_percent = ((bad_page_threshold - num_recs) * 100) /
			bad_page_threshold;
	control->tbl_egi_v2_1.gpu_healthy = (uint8_t)healthy_percent;
}

/* Do not use this function outside of the process_records call.
 * Doing so will corrupt the EEPROM checksum.
 */
static int ras_eeprom_v2_1_update_gpu_health(struct amdgv_adapter *adapt,
				struct amdgv_ras_eeprom_control *control)
{
	if (amdgv_ras_eeprom_is_gpu_bad(adapt))
		return 0;

	/* Check Driver conditions - Based on bad page reservations */
	switch (adapt->bp_msg_type) {
	case AMDGV_BP_MSG_IN_PF_FB:
		if (!(adapt->flags & AMDGV_FLAG_USE_PF))
			return 0;
		ras_eeprom_v2_1_mark_gpu_healthy_status(
			adapt, GPU_RETIRED__ECC_IN_CRITICAL_REGION);
		__mark_gpu_bad(adapt);
		return 0;
		break;
	case AMDGV_BP_MSG_IN_CRITICAL_REGION:
	case AMDGV_BP_MSG_IN_VF_CRITICAL_REGION:
		ras_eeprom_v2_1_mark_gpu_healthy_status(
			adapt, GPU_RETIRED__ECC_IN_CRITICAL_REGION);
		__mark_gpu_bad(adapt);
		return 0;
		break;
	case AMDGV_BP_MSG_IN_SAME_ROW:
		ras_eeprom_v2_1_mark_gpu_healthy_status(
			adapt, GPU_RETIRED__ECC_IN_SAME_MEMORY_ROW);
		__mark_gpu_bad(adapt);
		return 0;
		break;
	case AMDGV_BP_MSG_RECORD_THRESHOLD_REACHED:
		ras_eeprom_v2_1_mark_gpu_healthy_status(
				adapt, GPU_RETIRED__ECC_REACH_THRESHOLD);
		__mark_gpu_bad(adapt);
		return 0;
		break;
	default:
		ras_eeprom_v2_1_calc_gpu_healthy(adapt);
		break;
	}

	return 0;
}

static int ras_eeprom_v2_1_reset_table(struct amdgv_adapter *adapt,
				 struct amdgv_ras_eeprom_control *control)
{
	struct amdgv_ras_eeprom_table_header *hdr = &control->tbl_hdr;
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 *egi_v2_1 = &control->tbl_egi_v2_1;
	int ret = 0;

	oss_mutex_lock(control->tbl_mutex);

	hdr->header = EEPROM_TABLE_HDR_VAL;
	hdr->version = EEPROM_TABLE_VER_V2_1;
	hdr->first_rec_offset = EEPROM_RECORD_START_V2_1;
	hdr->tbl_size = EEPROM_TABLE_HEADER_SIZE + EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE;

	egi_v2_1->rma_status = GPU_HEALTHY_USABLE;
	egi_v2_1->gpu_healthy = 100;
	egi_v2_1->ecc_page_threshold = BAD_PAGE_RECORD_THRESHOLD;

	control->next_addr = EEPROM_RECORD_START_V2_1;
	control->tbl_byte_sum = 0;
	control->num_recs = 0;
	control->max_record_num = (EEPROM_SIZE_BYTES - EEPROM_TABLE_HEADER_SIZE -
			EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE) / EEPROM_TABLE_RECORD_SIZE;

	ras_eeprom_v2_1_update_tbl_checksum(control, NULL, 0, 0, 0);

	ret = ras_eeprom_v2_1_write_table_header(adapt, control);
	/* EEPROM Table Format V1 without extra gpu info */
	if (ret == 1)
		ret = ras_eeprom_v2_1_write_table_header_ext(adapt, control, true);

	oss_mutex_unlock(control->tbl_mutex);

	if (ret == 1) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_EEPROM_RESET, 0);
		ret = 0;
	} else {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_EEPROM_RESET_FAILED, 0);
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static int ras_eeprom_v2_1_parse_table_hdr_extra_info(struct amdgv_adapter *adapt,
						       struct i2c_msg *msg)
{
	unsigned char *buff = NULL;
	struct amdgv_ras_eeprom_control *control = &adapt->eeprom_control;
	int ret = 1;

	buff = (unsigned char *)oss_zalloc(EEPROM_ADDRESS_SIZE +
					   EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE);
	if (!buff) {
		AMDGV_ERROR("Alloc memory to read extra info failed\n");
		return AMDGV_FAILURE;
	}
	msg->buf = buff;

	msg->len = EEPROM_ADDRESS_SIZE + EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE;
	buff[0] = ((EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_START >> 8) & 0xFF);
	buff[1] = (EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_START & 0xFF);

	ret = __smu_i2c_transfer(adapt, control, msg, 1);
	if (ret < 1) {
		AMDGV_ERROR("Failed to read EEPROM extra gpu info, ret:%d\n",
			ret);
		goto out;
	}

	ras_eeprom_v2_1_decode_extra_info_from_buff(control, &buff[2]);

out:
	oss_free(buff);
	return ret;
}

static int ras_eeprom_v2_1_init_sw_control(struct amdgv_adapter *adapt, struct amdgv_ras_eeprom_control *control)
{
	int ret = 0;

	if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2_1) {
		control->num_recs = (control->tbl_hdr.tbl_size - EEPROM_TABLE_HEADER_SIZE - EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE) /
				EEPROM_TABLE_RECORD_SIZE;
		control->next_addr = EEPROM_RECORD_START_V2_1;
		control->max_record_num = (EEPROM_SIZE_BYTES - EEPROM_TABLE_HEADER_SIZE - EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE) /
				EEPROM_TABLE_RECORD_SIZE;
		control->tbl_byte_sum = __calc_hdr_byte_sum(control) + __calc_extra_info_byte_sum(control);
	} else if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2) {
		control->num_recs = (control->tbl_hdr.tbl_size - EEPROM_TABLE_HEADER_SIZE) /
				EEPROM_TABLE_RECORD_SIZE;
		control->next_addr = EEPROM_RECORD_START_V2;
		control->max_record_num = (EEPROM_SIZE_BYTES - EEPROM_TABLE_HEADER_SIZE) /
				EEPROM_TABLE_RECORD_SIZE;
		control->tbl_byte_sum = __calc_hdr_byte_sum(control);
	} else if (control->tbl_hdr.version == EEPROM_TABLE_VER_V1) {
		control->num_recs = (control->tbl_hdr.tbl_size - EEPROM_TABLE_HEADER_SIZE) /
				EEPROM_TABLE_RECORD_SIZE;
		control->next_addr = EEPROM_RECORD_START;
		control->max_record_num = (EEPROM_SIZE_BYTES - EEPROM_TABLE_HEADER_SIZE) /
				EEPROM_TABLE_RECORD_SIZE;
		control->tbl_byte_sum = __calc_hdr_byte_sum(control);
	} else {
		AMDGV_WARN("Invalid eeprom table version 0x%x\n", control->tbl_hdr.version);
		ret = AMDGV_FAILURE;
	}

	return ret;
}

/* reset EEPROM control read pointer back to the beginning */
static void ras_eeprom_v2_1_reset_read_control(struct amdgv_ras_eeprom_control *control)
{
	control->next_addr = EEPROM_RECORD_START_V2_1;
	control->tbl_byte_sum = ras_eeprom_v2_1_calc_tbl_byte_sum(control, NULL, 0);
}

/* Reset EEPROM control write count. Keeping the rest of header the same */
static void ras_eeprom_v2_1_reset_write_control(struct amdgv_ras_eeprom_control *control)
{
	control->num_recs = 0;
	control->tbl_hdr.tbl_size = EEPROM_TABLE_HEADER_SIZE + EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE;
	ras_eeprom_v2_1_reset_read_control(control);
}


/* Software back page threshold may be configured on different environemnts.
 * Check EEPROM records against current SW defined threhold
 * Following scenarios are possible:
 * 		1. SW threshold < EEPROM records ->  Un-RMA the GPU
 * 		2. SW threshold >= EEPROM records -> RMA the GPU
 */
static bool ras_eeprom_v2_1_need_fix_threshold(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control,
			  uint32_t num_recs)
{
	bool changed = false;
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 *egi_v2_1 = &control->tbl_egi_v2_1;
	enum GPU_HEALTY_STATUS status = ras_eeprom_v2_1_get_gpu_healthy_status(adapt);

	if (!(status == GPU_HEALTHY_USABLE) && !(status == GPU_RETIRED__ECC_REACH_THRESHOLD))
		return false;

	if (amdgv_ras_eeprom_is_gpu_bad(adapt))
		changed = (num_recs < BAD_PAGE_RECORD_THRESHOLD) ? true : false;
	else
		changed = (num_recs < BAD_PAGE_RECORD_THRESHOLD) ? false : true;

	if (egi_v2_1->ecc_page_threshold != BAD_PAGE_RECORD_THRESHOLD)
		changed = true;

	if (changed)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_THD_CHANGED,
			AMDGV_ERROR_32_32(egi_v2_1->ecc_page_threshold, BAD_PAGE_RECORD_THRESHOLD));

	return changed;
}

static int ras_eeprom_v2_1_fix_threshold(struct amdgv_adapter *adapt,
			struct amdgv_ras_eeprom_control *control,
			struct eeprom_table_record *bp_cache,
			uint32_t num_recs)
{
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 *egi_v2_1 = &control->tbl_egi_v2_1;
	uint32_t old_hdr_byte_sum = __calc_hdr_byte_sum(control);
	uint32_t old_extra_info_byte_sum = __calc_extra_info_byte_sum(control);

	ras_eeprom_v2_1_reset_read_control(control);

	egi_v2_1->ecc_page_threshold = BAD_PAGE_RECORD_THRESHOLD;
	if (num_recs >= egi_v2_1->ecc_page_threshold) {
		adapt->bp_msg_type = AMDGV_BP_MSG_RECORD_THRESHOLD_REACHED;
		__mark_gpu_bad(adapt);
		ras_eeprom_v2_1_mark_gpu_healthy_status(
					adapt, GPU_RETIRED__ECC_REACH_THRESHOLD);
	} else {
		adapt->bp_msg_type = AMDGV_BP_MSG_INVALID;
		adapt->eeprom_control.tbl_hdr.header = EEPROM_TABLE_HDR_VAL;
		ras_eeprom_v2_1_calc_gpu_healthy(adapt);
	}

	ras_eeprom_v2_1_update_tbl_checksum(control, bp_cache, num_recs,
					old_hdr_byte_sum,
					old_extra_info_byte_sum);

	ras_eeprom_v2_1_write_table_header(adapt, control);
	ras_eeprom_v2_1_write_table_header_ext(adapt, control, true);

	ras_eeprom_v2_1_reset_read_control(control);

	return 0;
}

static int ras_eeprom_v2_1_remove_dups(struct amdgv_adapter *adapt,
		struct eeprom_table_record *bps, int count)
{
	int i = 0;
	int j = 0;

	for (i = 0; i < count; i++) {
		for (j = i + 1; j < count; j++) {
			if (bps[i].retired_page == bps[j].retired_page) {
				oss_memcpy(&bps[j], &bps[j + 1], (count - 1 - j) * sizeof(*bps));
				oss_memset(&bps[count - 1], 0, sizeof(*bps));
				count--;
				j--;
			}
		}
	}

	return count;
}

static int ras_eeprom_v2_1_overwrite_table(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control,
			  struct eeprom_table_record *bp_cache,
			  uint32_t num_recs)
{
	ras_eeprom_v2_1_reset_write_control(control);

	if (ras_eeprom_v2_1_process_records(adapt, control, bp_cache, true, num_recs))
		return AMDGV_FAILURE;

	ras_eeprom_v2_1_reset_read_control(control);

	return 0;
}

/* Initial implementation of V2_1 would only read/write to first 4 bytes of extra header region.
 * However, all reserved 252bytes of this region must be written with 0s.
 * Unconditionally write to this region to fix any possible un-compliant EEPROMs.
 */
static int ras_eeprom_v2_1_fix_header_ext(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control,
			  struct eeprom_table_record *bp_cache,
			  uint32_t num_recs)
{
	ras_eeprom_v2_1_reset_read_control(control);

	ras_eeprom_v2_1_update_tbl_checksum(control, bp_cache, num_recs,
					__calc_hdr_byte_sum(control),
					__calc_extra_info_byte_sum(control));

	ras_eeprom_v2_1_write_table_header(adapt, control);
	ras_eeprom_v2_1_write_table_header_ext(adapt, control, true);

	ras_eeprom_v2_1_reset_read_control(control);

	return 0;
}

static int ras_eeprom_v2_1_fix_overwrite_table(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control,
			  struct eeprom_table_record *bp_cache, bool force_write)
{
	bool overwrite = false;
	bool fix_threshold = false;
	uint32_t new_count = 0;

	if (force_write)
		overwrite = true;

	new_count = ras_eeprom_v2_1_remove_dups(adapt, bp_cache, control->num_recs);
	if (new_count != control->num_recs) {
		overwrite = true;
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_DUP_ENTRIES,
			AMDGV_ERROR_32_32(control->num_recs, new_count));
	}
	fix_threshold = ras_eeprom_v2_1_need_fix_threshold(adapt, control, new_count);

	ras_eeprom_v2_1_fix_header_ext(adapt, control, bp_cache, new_count);

	if (!overwrite && !fix_threshold)
		return 0;

	if (overwrite)
		ras_eeprom_v2_1_overwrite_table(adapt, control, bp_cache, new_count);
	if (fix_threshold)
		ras_eeprom_v2_1_fix_threshold(adapt, control, bp_cache, new_count);

	return 0;
}

static bool ras_eeprom_v2_1_cache_and_validate_recs(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control,
			  struct eeprom_table_record *bp_cache, uint32_t num_recs)
{
	bool valid = true;

	if (ras_eeprom_v2_1_process_records(adapt, control, bp_cache, false, num_recs))
		valid = false;

	if (!__validate_tbl_checksum(adapt, control))
		valid = false;

	ras_eeprom_v2_1_reset_read_control(control);

	return valid;
}

static bool ras_eeprom_v2_1_cache_and_validate_legacy_recs(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control,
			  struct eeprom_table_record *bp_cache, uint32_t num_recs)
{
	bool valid = true;

	if (ras_eeprom_legacy_process_records(adapt, control, bp_cache, false, num_recs))
		valid = false;

	if (!__validate_tbl_checksum(adapt, control))
		valid = false;

	ras_eeprom_v2_1_reset_read_control(control);

	return valid;
}

/* Force overwrite is a W/A for mismatch between LibGV & Linux BM EEPROM formats
 * It should NOT be required for future EEPROM versions.
 */
static int ras_eeprom_v2_1_fix_and_upgrade_table(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control)
{
	int ret = AMDGV_FAILURE;
	bool force_overwrite = false;
	bool valid_v2_1_tbl = false;
	bool valid_legacy_tbl = false;
	struct eeprom_table_record *bp_cache = NULL;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return 0;

	if (control->num_recs) {
		bp_cache = oss_zalloc(control->num_recs * sizeof(struct eeprom_table_record));
		if (!bp_cache)
			return AMDGV_FAILURE;
	}

	valid_v2_1_tbl = ras_eeprom_v2_1_cache_and_validate_recs(adapt,
		control, bp_cache, control->num_recs);
	if (!valid_v2_1_tbl) {
		valid_legacy_tbl = ras_eeprom_v2_1_cache_and_validate_legacy_recs(adapt,
			control, bp_cache, control->num_recs);
	}

	if (valid_v2_1_tbl) {
		AMDGV_INFO("Valid EEPROM V2_1 table detected.\n");
		force_overwrite = false;
	} else if (valid_legacy_tbl) {
		AMDGV_INFO("Upgrading EEPROM table.\n");
		force_overwrite = true;
	} else {
		ret = AMDGV_FAILURE;
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_EEPROM_CHK_MISMATCH, 0);
		goto free_bp_cache;
	}

	ret = ras_eeprom_v2_1_fix_overwrite_table(adapt, control, bp_cache, force_overwrite);
	if (ret)
		goto free_bp_cache;

	ras_eeprom_v2_1_reset_read_control(control);

free_bp_cache:
	if (bp_cache)
		oss_free(bp_cache);

	return ret;
}


static int ras_eeprom_v2_1_parse_header(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control)
{
	int ret = 0;
	unsigned char buff[EEPROM_ADDRESS_SIZE + EEPROM_TABLE_HEADER_SIZE] = { 0 };
	struct amdgv_ras_eeprom_table_header *hdr = NULL;
	struct i2c_msg msg = {
		.addr = 0,
		.flags = I2C_M_RD,
		.len = EEPROM_ADDRESS_SIZE + EEPROM_TABLE_HEADER_SIZE,
		.buf = buff,
	};

	hdr = &control->tbl_hdr;
	if (!adapt->umc.supports_ras_eeprom)
		return 0;
	msg.addr = control->i2c_address;
	*(uint16_t *)buff = EEPROM_HDR_START;

	/* Read/Create table header from EEPROM address 0 */
	ret = __smu_i2c_transfer(adapt, control, &msg, 1);
	if (ret < 1) {
		AMDGV_ERROR("Failed to read EEPROM table header, ret:%d\n", ret);
		return AMDGV_FAILURE;
	}

	__decode_table_header_from_buff(hdr, &buff[2]);

	if (hdr->header != EEPROM_TABLE_HDR_VAL && hdr->header != EEPROM_TABLE_HDR_BAD) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_EEPROM_WRONG_HDR, hdr->header);
		return AMDGV_FAILURE;
	}

	if (hdr->version != EEPROM_TABLE_VER_V2_1 && hdr->version != EEPROM_TABLE_VER_V2) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_EEPROM_WRONG_VER,
			AMDGV_ERROR_32_32(hdr->version, EEPROM_TABLE_VER_V2_1));
		return AMDGV_FAILURE;
	}

	ret = ras_eeprom_v2_1_parse_table_hdr_extra_info(adapt, &msg);
	if (ret < 1)
		return AMDGV_FAILURE;

	return 0;
}

static int ras_eeprom_v2_1_init(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control)
{
	int ret = 0;

	if (!__get_eeprom_i2c_params(adapt, control))
		return AMDGV_FAILURE;

	ret = ras_eeprom_v2_1_parse_header(adapt, control);
	if (ret)
		goto reset_eeprom;

	if (!in_whole_gpu_reset()) {
		/* Note, if EEPROM control init failure, there is no chance we can reset it */
		ret = ras_eeprom_v2_1_init_sw_control(adapt, control);
		if (ret)
			return ret;

		ret = ras_eeprom_v2_1_fix_and_upgrade_table(adapt, control);
		if (ret)
			goto reset_eeprom;

		if (amdgv_ras_eeprom_is_gpu_bad(adapt))
			amdgv_device_handle_bad_gpu(adapt);
	}

	return ret;

reset_eeprom:
	AMDGV_INFO("Creating new EEPROM table\n");
	ret = ras_eeprom_v2_1_reset_table(adapt, control);

	return ret;
}

static int ras_eeprom_v2_1_entry_overflow(struct amdgv_ras_eeprom_control *control)
{
	return ((control->next_addr - control->tbl_hdr.first_rec_offset) /
		EEPROM_TABLE_RECORD_SIZE) > control->max_record_num;
}

static int ras_eeprom_v2_1_process_record(struct amdgv_adapter *adapt,
			struct amdgv_ras_eeprom_control *control,
			struct eeprom_table_record *record, bool write)
{
	unsigned char encoded_record[EEPROM_TABLE_RECORD_SIZE] = { 0 };

	if (write)
		__encode_table_record_to_buff(control, record, encoded_record);

	if (ras_eeprom_v2_1_i2c_transfer_record(adapt, control,
		encoded_record,
		sizeof(encoded_record), write)) {
		AMDGV_ERROR("Failed to %s EEPROM table record\n", write ? "write" : "read");
		return AMDGV_FAILURE;
	}

	if (!write)
		__decode_table_record_from_buff(control, record, encoded_record);

	control->next_addr += EEPROM_TABLE_RECORD_SIZE;
	__update_bad_channel_bitmap(adapt, control, record);

	return 0;
}


static int ras_eeprom_v2_1_update_table_header(struct amdgv_adapter *adapt,
						struct amdgv_ras_eeprom_control *control,
						struct eeprom_table_record *records,
						int num)
{
	uint32_t old_hdr_byte_sum = __calc_hdr_byte_sum(control);
	uint32_t old_extra_info_byte_sum = __calc_extra_info_byte_sum(control);

	control->num_recs += num;
	control->tbl_hdr.tbl_size += EEPROM_TABLE_RECORD_SIZE * num;

	ras_eeprom_v2_1_update_gpu_health(adapt, control);

	ras_eeprom_v2_1_update_tbl_checksum(control, records, num,
					old_hdr_byte_sum,
					old_extra_info_byte_sum);

	ras_eeprom_v2_1_write_table_header(adapt, control);
	ras_eeprom_v2_1_write_table_header_ext(adapt, control, false);

	return 0;
}

int ras_eeprom_v2_1_process_records(struct amdgv_adapter *adapt,
				     struct amdgv_ras_eeprom_control *control,
				     struct eeprom_table_record *records, bool write, int num)
{
	int i, ret = 0;


	if (!adapt->umc.supports_ras_eeprom)
		return 0;

	oss_mutex_lock(control->tbl_mutex);

	for (i = 0; i < num; i++) {

		if (ras_eeprom_v2_1_entry_overflow(control)) {
			AMDGV_WARN("Reached end of EEPROM. Ignore process request\n");
			break;
		}

		ret = ras_eeprom_v2_1_process_record(adapt, control, &records[i], write);
		if (ret)
			goto fail;
	}

	if (write)
		ras_eeprom_v2_1_update_table_header(adapt, control, records, i);
	else
		ras_eeprom_v2_1_update_tbl_byte_sum(adapt, control, records, i);


fail:
	oss_mutex_unlock(control->tbl_mutex);

	return ret;
}


static const struct amdgv_ras_eeprom_funcs ras_eeprom_v2_1_funcs = {
	.init = ras_eeprom_v2_1_init,
	.fini = NULL,
	.reset_table = ras_eeprom_v2_1_reset_table,
	.process_records = ras_eeprom_v2_1_process_records,
};

int ras_eeprom_v2_1_sw_init(struct amdgv_adapter *adapt)
{
	adapt->ras_eeprom.funcs = &ras_eeprom_v2_1_funcs;

	return 0;
}

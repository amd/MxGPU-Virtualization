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
#include "amdgv.h"

typedef uint64_t __le64;
static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static void __encode_eeprom_extra_info_to_buff(struct amdgv_ras_eeprom_control *control,
					       unsigned char *buff)
{
	struct amdgv_ras_eeprom_extra_gpu_info_v2 *egi_v2 = NULL;
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 *egi_v2_1 = NULL;
	uint32_t *pp = (uint32_t *)buff;
	uint32_t tmp = 0;

	if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2) {
		egi_v2 = &control->tbl_egi_v2;
		tmp = ((uint32_t)(egi_v2->ecc_page_threshold) & 0xFFFF) |
		      (((uint32_t)(egi_v2->gpu_healthy_status) << 16) & 0xFFFF0000);
		pp[0] = cpu_to_le32(egi_v2->header_version);
		pp[1] = cpu_to_le32(tmp);
	} else if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2_1) {
		egi_v2_1 = &control->tbl_egi_v2_1;
		tmp = ((uint32_t)(egi_v2_1->rma_status) & 0xFF) |
		      (((uint32_t)(egi_v2_1->gpu_healthy) << 8) & 0xFF00) |
		      (((uint32_t)(egi_v2_1->ecc_page_threshold) << 16) & 0xFFFF0000);
		pp[0] = cpu_to_le32(tmp);
	}
}

static void __decode_eeprom_extra_info_from_buff(struct amdgv_ras_eeprom_control *control,
						 unsigned char *buff)
{
	struct amdgv_ras_eeprom_extra_gpu_info_v2 *egi_v2 = NULL;
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 *egi_v2_1 = NULL;
	uint32_t *pp = (uint32_t *)buff;
	uint32_t tmp = 0;

	if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2) {
		egi_v2 = &control->tbl_egi_v2;
		egi_v2->header_version = le32_to_cpu(pp[0]);
		tmp = le32_to_cpu(pp[1]);
		egi_v2->ecc_page_threshold = tmp & 0xFFFF;
		egi_v2->gpu_healthy_status = (tmp >> 16) & 0xFFFF;
	} else if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2_1) {
		egi_v2_1 = &control->tbl_egi_v2_1;
		tmp = le32_to_cpu(pp[0]);
		egi_v2_1->rma_status = tmp & 0xFF;
		egi_v2_1->gpu_healthy = (tmp >> 8) & 0xFF;
		egi_v2_1->ecc_page_threshold = (tmp >> 16) & 0xFFFF;
	}
}

static int __update_table_header(struct amdgv_adapter *adapt,
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

static int __update_extra_gpu_info(struct amdgv_adapter *adapt,
				   struct amdgv_ras_eeprom_control *control)
{
	unsigned char *buff = NULL;
	int ret = 0;
	struct i2c_msg msg = {
		.addr = control->i2c_address,
		.flags = 0,
	};

	buff = (unsigned char *)oss_zalloc(EEPROM_ADDRESS_SIZE +
					   EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE);
	if (!buff) {
		AMDGV_ERROR("Alloc memory to update extra info failed\n");
		return AMDGV_FAILURE;
	}
	msg.buf = buff;

	if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2) {
		msg.len = EEPROM_ADDRESS_SIZE + EEPROM_TABLE_V2_EXTRA_GPU_INFO_SIZE;
		buff[0] = ((EEPROM_TABLE_V2_EXTRA_GPU_INFO_START >> 8) & 0xFF);
		buff[1] = (EEPROM_TABLE_V2_EXTRA_GPU_INFO_START & 0xFF);
	} else if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2_1) {
		msg.len = EEPROM_ADDRESS_SIZE + EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_SIZE;
		buff[0] = ((EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_START >> 8) & 0xFF);
		buff[1] = (EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_START & 0xFF);
	} else {
		AMDGV_WARN("EEPROM table version 0x%x is invalid to update extra gpu info\n",
			   control->tbl_hdr.version);
	}
	__encode_eeprom_extra_info_to_buff(control, buff + EEPROM_ADDRESS_SIZE);

	ret = __smu_i2c_transfer(adapt, control, &msg, 1);
	if (ret < 1)
		AMDGV_ERROR("Failed to write EEPROM extra gpu info, ret:%d\n", ret);

	oss_free(buff);

	return ret;
}

static inline uint32_t __calc_tbl_byte_sum(struct amdgv_ras_eeprom_control *control,
					   struct eeprom_table_record *records, int num)
{
	if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2_1)
		return __calc_hdr_byte_sum(control) +
		       __calc_extra_info_byte_sum(control) +
		       __calc_recs_byte_sum(records, num);
	else
		return __calc_hdr_byte_sum(control) +
		       __calc_recs_byte_sum(records, num);
}

/* Checksum = 256 -((sum of all table entries) mod 256) */
static void __update_tbl_checksum(struct amdgv_ras_eeprom_control *control,
				  struct eeprom_table_record *records, int num,
				  uint32_t old_hdr_byte_sum, uint32_t old_extra_info_byte_sum)
{
	/*
	 * This will update the table sum with new records.
	 *
	 * TODO: What happens when the EEPROM table is to be wrapped around
	 * and old records from start will get overridden.
	 */

	/* need to recalculate updated header byte sum */
	if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2_1)
		control->tbl_byte_sum -= old_extra_info_byte_sum;

	control->tbl_byte_sum -= old_hdr_byte_sum;
	control->tbl_byte_sum += __calc_tbl_byte_sum(control, records, num);

	control->tbl_hdr.checksum = 256 - (control->tbl_byte_sum % 256);
}

/* table sum mod 256 + checksum must equals 256 */
static bool __update_and_validate_tbl_checksum(struct amdgv_adapter *adapt,
				    struct amdgv_ras_eeprom_control *control,
				    struct eeprom_table_record *records, int num)
{
	control->tbl_byte_sum = __calc_tbl_byte_sum(control, records, num);
	return __validate_tbl_checksum(adapt, control);
}

/* Produce the worst case scenario given 256Byte page boundries */
static int amdgv_ras_eeprom_set_max_record_num(struct amdgv_adapter *adapt,
				struct amdgv_ras_eeprom_control *control)
{
	uint32_t num_record_pages = 0;
	uint32_t records_per_page = 0;

	if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2_1) {
		/* Pad Header to full 256 bytes*/
		num_record_pages = (EEPROM_SIZE_BYTES - EEPROM_PAGE__SIZE_BYTES -
			EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE) / EEPROM_PAGE__SIZE_BYTES;
		/* rounded down per page*/
		records_per_page = (EEPROM_PAGE__SIZE_BYTES / EEPROM_TABLE_RECORD_SIZE);
		control->max_record_num = (num_record_pages * records_per_page);
		return (num_record_pages * records_per_page);
	} else if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2) {
		control->max_record_num = EEPROM_TABLE_V2_MAX_ECC_PAGE_RECORD;
	} else {
		num_record_pages = (EEPROM_SIZE_BYTES - EEPROM_PAGE__SIZE_BYTES) / EEPROM_PAGE__SIZE_BYTES;
		records_per_page = (EEPROM_PAGE__SIZE_BYTES / EEPROM_TABLE_RECORD_SIZE);
		control->max_record_num = (num_record_pages * records_per_page);
	}

	return 0;
}

static int ras_eeprom_legacy_reset_table(struct amdgv_adapter *adapt,
				 struct amdgv_ras_eeprom_control *control)
{
	struct amdgv_ras_eeprom_table_header *hdr = &control->tbl_hdr;
	struct amdgv_ras_eeprom_extra_gpu_info_v2 *egi_v2 = &control->tbl_egi_v2;
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 *egi_v2_1 = &control->tbl_egi_v2_1;
	int ret = 0;

	oss_mutex_lock(control->tbl_mutex);

	hdr->header = EEPROM_TABLE_HDR_VAL;
	if (adapt->umc.funcs && adapt->umc.funcs->set_eeprom_table_version)
		adapt->umc.funcs->set_eeprom_table_version(adapt);
	else
		hdr->version = EEPROM_TABLE_VER_V2;

	switch (hdr->version) {
	case EEPROM_TABLE_VER_V1:
		hdr->first_rec_offset = EEPROM_RECORD_START;
		hdr->tbl_size = EEPROM_TABLE_HEADER_SIZE;
		control->next_addr = EEPROM_RECORD_START;
		break;
	case EEPROM_TABLE_VER_V2:
		hdr->first_rec_offset = EEPROM_RECORD_START_V2;
		hdr->tbl_size = EEPROM_TABLE_HEADER_SIZE;
		/* Reset eeprom table extra info */
		egi_v2->header_version = EEPROM_TABLE_VER_V2;
		egi_v2->ecc_page_threshold = BAD_PAGE_RECORD_THRESHOLD;
		egi_v2->gpu_healthy_status = GPU_HEALTHY_USABLE;
		control->next_addr = EEPROM_RECORD_START_V2;
		break;
	case EEPROM_TABLE_VER_V2_1:
		hdr->first_rec_offset = EEPROM_RECORD_START_V2_1;
		hdr->tbl_size = EEPROM_TABLE_HEADER_SIZE +
				EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE;
		/* Reset eeprom table extra info */
		egi_v2_1->rma_status = GPU_HEALTHY_USABLE;
		egi_v2_1->gpu_healthy = 100;
		egi_v2_1->ecc_page_threshold = BAD_PAGE_RECORD_THRESHOLD;
		control->next_addr = EEPROM_RECORD_START_V2_1;
		break;
	default:
		AMDGV_WARN("INVALID EEPROM table header version 0x%x\n",
			   hdr->version);
		break;
	}

	control->tbl_byte_sum = 0;
	control->num_recs = 0;

	__update_tbl_checksum(control, NULL, 0, 0, 0);

	ret = __update_table_header(adapt, control);
	/* EEPROM Table Format V1 without extra gpu info */
	if (ret == 1 && hdr->version > EEPROM_TABLE_VER_V1)
		ret = __update_extra_gpu_info(adapt, control);

	amdgv_ras_eeprom_set_max_record_num(adapt, control);

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

static int ras_eeprom_legacy_parse_table_hdr_extra_info(struct amdgv_adapter *adapt,
						       struct i2c_msg *msg)
{
	unsigned char *buff = NULL;
	struct amdgv_ras_eeprom_control *control = &adapt->eeprom_control;
	struct amdgv_ras_eeprom_table_header *hdr = &control->tbl_hdr;
	int ret = 1;

	buff = (unsigned char *)oss_zalloc(EEPROM_ADDRESS_SIZE +
					   EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE);
	if (!buff) {
		AMDGV_ERROR("Alloc memory to read extra info failed\n");
		return AMDGV_FAILURE;
	}
	msg->buf = buff;

	if (hdr->version == EEPROM_TABLE_VER_V2_1) {
		control->num_recs = (hdr->tbl_size -
					EEPROM_TABLE_HEADER_SIZE -
					EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE) /
					EEPROM_TABLE_RECORD_SIZE;
		control->tbl_byte_sum = __calc_hdr_byte_sum(control);
		control->next_addr = EEPROM_RECORD_START_V2_1;

		/* parse extra gpu info */
		msg->len = EEPROM_ADDRESS_SIZE + EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE;
		buff[0] = ((EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_START >> 8) & 0xFF);
		buff[1] = (EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_START & 0xFF);
		/* Read eeprom extra gpu info from EEPROM address */
		ret = __smu_i2c_transfer(adapt, control, msg, 1);
		if (ret < 1) {
			AMDGV_ERROR("Failed to read EEPROM extra gpu info, ret:%d\n",
				    ret);
			goto out;
		}

		__decode_eeprom_extra_info_from_buff(control, &buff[2]);
		control->tbl_byte_sum += __calc_extra_info_byte_sum(control);
	} else if (hdr->version == EEPROM_TABLE_VER_V2) {
		control->num_recs = (hdr->tbl_size -
				     EEPROM_TABLE_HEADER_SIZE) / EEPROM_TABLE_RECORD_SIZE;
		control->tbl_byte_sum = __calc_hdr_byte_sum(control);
		control->next_addr = EEPROM_RECORD_START_V2;

		/* parse extra gpu info */
		msg->len = EEPROM_ADDRESS_SIZE + EEPROM_TABLE_V2_EXTRA_GPU_INFO_SIZE;
		buff[0] = ((EEPROM_TABLE_V2_EXTRA_GPU_INFO_START >> 8) & 0xFF);
		buff[1] = (EEPROM_TABLE_V2_EXTRA_GPU_INFO_START & 0xFF);
		/* Read eeprom extra gpu info from EEPROM address */
		ret = __smu_i2c_transfer(adapt, control, msg, 1);
		if (ret < 1) {
			AMDGV_ERROR("Failed to read EEPROM extra gpu info, ret:%d\n",
				    ret);
			goto out;
		}

		__decode_eeprom_extra_info_from_buff(control, &buff[2]);
	} else if (hdr->version == EEPROM_TABLE_VER_V1) {
		control->num_recs = (hdr->tbl_size -
				     EEPROM_TABLE_HEADER_SIZE) / EEPROM_TABLE_RECORD_SIZE;
		control->tbl_byte_sum = __calc_hdr_byte_sum(control);
		control->next_addr = EEPROM_RECORD_START;
	} else {
		AMDGV_WARN("Invalid eeprom table version 0x%x\n",
			   hdr->version);
		ret = AMDGV_FAILURE;
	}

out:
	oss_free(buff);
	return ret;
}

static int ras_eeprom_legacy_init(struct amdgv_adapter *adapt,
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

	if (!__get_eeprom_i2c_params(adapt, control))
		return AMDGV_FAILURE;

	hdr = &control->tbl_hdr;
	if (!adapt->umc.supports_ras_eeprom)
		return 0;
	msg.addr = control->i2c_address;
	/* program address */
	*(uint16_t *)buff = EEPROM_HDR_START;

	/* Read/Create table header from EEPROM address 0 */
	ret = __smu_i2c_transfer(adapt, control, &msg, 1);
	if (ret < 1) {
		AMDGV_ERROR("Failed to read EEPROM table header, ret:%d\n", ret);
		return ret;
	}

	__decode_table_header_from_buff(hdr, &buff[2]);

	if (hdr->header != EEPROM_TABLE_HDR_VAL && hdr->header != EEPROM_TABLE_HDR_BAD) {
		AMDGV_ERROR("Invalid EEPROM Table detected.\n");
		goto reset_eeprom;
	}

	if (!in_whole_gpu_reset()) {
		ret = ras_eeprom_legacy_parse_table_hdr_extra_info(adapt, &msg);
		if (ret < 1)
			goto reset_eeprom;

		amdgv_ras_eeprom_set_max_record_num(adapt, control);
	}

	/* In reset state, the bad GPU state is handled by reset event */
	if (!amdgv_ras_eeprom_is_gpu_bad(adapt) || in_whole_gpu_reset()) {
		return 0;
	} else {
		amdgv_device_handle_bad_gpu(adapt);
		return 0;
	}

reset_eeprom:
	AMDGV_INFO("Creating new EEPROM table\n");
	ret = ras_eeprom_legacy_reset_table(adapt, control);

	return ret;
}

/*
 * When reaching end of EEPROM memory jump back to 0 record address
 * When next record access will go beyond EEPROM page boundary modify bits A17/A8
 * in I2C selector to go to next page
 */
static uint32_t __correct_eeprom_dest_address(struct amdgv_adapter *adapt,
					      uint32_t curr_address)
{
	uint32_t next_address = curr_address + EEPROM_TABLE_RECORD_SIZE;

	/* When all EEPROM memory used jump back to 0 address */
	if (next_address > EEPROM_SIZE_BYTES) {
		AMDGV_INFO("Reached end of EEPROM memory, jumping to 0 "
			   "and overriding old record\n");
		if (adapt->eeprom_control.tbl_hdr.version == EEPROM_TABLE_VER_V1)
			return EEPROM_RECORD_START;
		else if (adapt->eeprom_control.tbl_hdr.version == EEPROM_TABLE_VER_V2)
			return EEPROM_RECORD_START;
		else if (adapt->eeprom_control.tbl_hdr.version == EEPROM_TABLE_VER_V2_1)
			return EEPROM_RECORD_START_V2_1;
		else
			return EEPROM_RECORD_START_V2_1;
	}

	/*
	 * To check if we overflow page boundary compare next address with
	 * current and see if bits 17/8 of the EEPROM address will change
	 * If they do start from the next 256b page
	 *
	 * https://www.st.com/resource/en/datasheet/m24m02-dr.pdf sec. 5.1.2
	 */
	if ((curr_address & EEPROM_ADDR_MSB_MASK) != (next_address & EEPROM_ADDR_MSB_MASK)) {
		AMDGV_INFO("Reached end of EEPROM memory page, jumping to next: %lx\n",
			   (next_address & EEPROM_ADDR_MSB_MASK));

		return (next_address & EEPROM_ADDR_MSB_MASK);
	}

	return curr_address;
}

static void ras_eeprom_legacy_mark_gpu_healthy_status(struct amdgv_adapter *adapt, enum GPU_HEALTY_STATUS status)
{
	struct amdgv_ras_eeprom_extra_gpu_info_v2 *egi_v2 = &adapt->eeprom_control.tbl_egi_v2;
	struct amdgv_ras_eeprom_extra_gpu_info_v2_1 *egi_v2_1 = &adapt->eeprom_control.tbl_egi_v2_1;

	if (adapt->eeprom_control.tbl_hdr.version == EEPROM_TABLE_VER_V2_1) {
		egi_v2_1->rma_status = status;
		egi_v2_1->gpu_healthy = 0;
	} else if (adapt->eeprom_control.tbl_hdr.version == EEPROM_TABLE_VER_V2) {
		egi_v2->gpu_healthy_status = status;
	}
}

static void ras_eeprom_legacy_calc_gpu_healthy(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_eeprom_control *control = &adapt->eeprom_control;
	uint32_t num_recs = control->num_recs;
	uint32_t bad_page_threshold = BAD_PAGE_RECORD_THRESHOLD;
	uint32_t healthy_percent = 0;

	if (control->tbl_hdr.version == EEPROM_TABLE_VER_V2_1) {
		healthy_percent = ((bad_page_threshold - num_recs) * 100) /
				   bad_page_threshold;
		control->tbl_egi_v2_1.gpu_healthy = (uint8_t)healthy_percent;
	}
}


/* Do not use this function outside of the process_records call.
 * Doing so will corrupt the EEPROM checksum.
 */
static int ras_eeprom_legacy_update_gpu_health(struct amdgv_adapter *adapt,
				struct amdgv_ras_eeprom_control *control)
{
	if (amdgv_ras_eeprom_is_gpu_bad(adapt))
		return 0;

	/* Check Driver conditions - Based on bad page reservations */
	switch (adapt->bp_msg_type) {
	case AMDGV_BP_MSG_IN_PF_FB:
		if (!(adapt->flags & AMDGV_FLAG_USE_PF))
			return 0;
		ras_eeprom_legacy_mark_gpu_healthy_status(
			adapt, GPU_RETIRED__ECC_IN_CRITICAL_REGION);
		__mark_gpu_bad(adapt);
		return 0;
		break;
	case AMDGV_BP_MSG_IN_CRITICAL_REGION:
	case AMDGV_BP_MSG_IN_VF_CRITICAL_REGION:
		ras_eeprom_legacy_mark_gpu_healthy_status(
			adapt, GPU_RETIRED__ECC_IN_CRITICAL_REGION);
		__mark_gpu_bad(adapt);
		return 0;
		break;
	case AMDGV_BP_MSG_IN_SAME_ROW:
		ras_eeprom_legacy_mark_gpu_healthy_status(
			adapt, GPU_RETIRED__ECC_IN_SAME_MEMORY_ROW);
		__mark_gpu_bad(adapt);
		return 0;
		break;
	case AMDGV_BP_MSG_RECORD_THRESHOLD_REACHED:
		ras_eeprom_legacy_mark_gpu_healthy_status(
				adapt, GPU_RETIRED__ECC_REACH_THRESHOLD);
		__mark_gpu_bad(adapt);
		return 0;
		break;
	default:
		/* no action */
		break;
	}

	/* check EEPROM condition */
	if (control->num_recs >= control->max_record_num) {
		__mark_gpu_bad(adapt);
		ras_eeprom_legacy_mark_gpu_healthy_status(
				adapt, GPU_RETIRED__ECC_REACH_THRESHOLD);
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_EEPROM_REACH_THD, control->max_record_num);
	} else {
		ras_eeprom_legacy_calc_gpu_healthy(adapt);
	}

	return 0;
}

int ras_eeprom_legacy_process_records(struct amdgv_adapter *adapt,
				     struct amdgv_ras_eeprom_control *control,
				     struct eeprom_table_record *records, bool write, int num)
{
	int i, ret = 0;
	struct i2c_msg *msgs, *msg;
	unsigned char *buffs, *buff;
	struct eeprom_table_record *record;

	if (!adapt->umc.supports_ras_eeprom)
		return 0;

	if (!num)
		return 0;

	buffs = oss_zalloc(num * (EEPROM_ADDRESS_SIZE + EEPROM_TABLE_RECORD_SIZE));
	if (!buffs) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				num * (EEPROM_ADDRESS_SIZE + EEPROM_TABLE_RECORD_SIZE));
		return -1;
	}

	oss_mutex_lock(control->tbl_mutex);
	msgs = oss_zalloc(num * sizeof(*msgs));
	if (!msgs) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				num * sizeof(*msgs));
		ret = -1;
		goto free_buff;
	}

	/* In case of overflow just start from beginning to not lose newest records */
	if (write && (control->next_addr + EEPROM_TABLE_RECORD_SIZE * num > EEPROM_SIZE_BYTES))
		control->next_addr = EEPROM_RECORD_START;

	/*
	 * TODO Currently makes EEPROM writes for each record, this creates
	 * internal fragmentation. Optimized the code to do full page write of
	 * 256b
	 */
	for (i = 0; i < num; i++) {
		buff = &buffs[i * (EEPROM_ADDRESS_SIZE + EEPROM_TABLE_RECORD_SIZE)];
		record = &records[i];
		msg = &msgs[i];

		control->next_addr = __correct_eeprom_dest_address(adapt, control->next_addr);

		/*
		 * Update bits 16,17 of EEPROM address in I2C address by setting them
		 * to bits 1,2 of Device address byte
		 */
		msg->addr = control->i2c_address |
			    ((control->next_addr & EEPROM_ADDR_MSB_MASK) >> 15);
		msg->flags = write ? 0 : I2C_M_RD;
		msg->len = EEPROM_ADDRESS_SIZE + EEPROM_TABLE_RECORD_SIZE;
		msg->buf = buff;

		/* Insert the EEPROM dest addess, bits 0-15 */
		buff[0] = ((control->next_addr >> 8) & 0xff);
		buff[1] = (control->next_addr & 0xff);

		/* EEPROM table content is stored in LE format */
		if (write) {
			__encode_table_record_to_buff(control, record,
						      buff + EEPROM_ADDRESS_SIZE);

			/* update bad channel bitmap */
			if ((record->mem_channel < BITS_PER_TYPE(control->bad_channel_bitmap)) &&
				!(control->bad_channel_bitmap & (1 << record->mem_channel))) {
				control->bad_channel_bitmap |= 1 << record->mem_channel;
				adapt->update_channel_flag = true;
			}
		}

		/*
		 * The destination EEPROM address might need to be corrected to account
		 * for page or entire memory wrapping
		 */
		control->next_addr += EEPROM_TABLE_RECORD_SIZE;
	}

	ret = __smu_i2c_transfer(adapt, control, msgs, num);
	if (ret < 1) {
		AMDGV_ERROR("Failed to process EEPROM table records, ret:%d\n", ret);

		/* TODO Restore prev next EEPROM address ? */
		goto free_msgs;
	}

	if (!write) {
		for (i = 0; i < num; i++) {
			buff = &buffs[i * (EEPROM_ADDRESS_SIZE + EEPROM_TABLE_RECORD_SIZE)];
			record = &records[i];

			__decode_table_record_from_buff(control, record,
							buff + EEPROM_ADDRESS_SIZE);
			/* update bad channel bitmap */
			if ((record->mem_channel < BITS_PER_TYPE(control->bad_channel_bitmap)) &&
				!(control->bad_channel_bitmap & (1 << record->mem_channel))) {
				control->bad_channel_bitmap |= 1 << record->mem_channel;
				adapt->update_channel_flag = true;
			}
		}
	}

	if (write) {
		uint32_t old_hdr_byte_sum = __calc_hdr_byte_sum(control);
		uint32_t old_extra_info_byte_sum = __calc_extra_info_byte_sum(control);

		/*
		 * Update table header with size and CRC and account for table
		 * wrap around where the assumption is that we treat it as empty
		 * table
		 *
		 * TODO - Check the assumption is correct
		 */
		control->num_recs += num;
		control->tbl_hdr.tbl_size += EEPROM_TABLE_RECORD_SIZE * num;
		if (control->tbl_hdr.tbl_size > EEPROM_SIZE_BYTES)
			control->tbl_hdr.tbl_size =
				EEPROM_TABLE_HEADER_SIZE +
				(control->tbl_hdr.version == EEPROM_TABLE_VER_V2_1 ?
				EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE : 0) +
				control->num_recs * EEPROM_TABLE_RECORD_SIZE;

		ras_eeprom_legacy_update_gpu_health(adapt, control);

		if (amdgv_ras_eeprom_is_gpu_bad(adapt))
			__smu_send_rma_reason(adapt);

		__update_tbl_checksum(control, records, num,
				      old_hdr_byte_sum,
				      old_extra_info_byte_sum);

		__update_table_header(adapt, control);
		if (control->tbl_hdr.version > EEPROM_TABLE_VER_V1)
			__update_extra_gpu_info(adapt, control);
	} else if (!__update_and_validate_tbl_checksum(adapt, control, records, num)) {
		AMDGV_WARN("EEPROM Table checksum mismatch!\n");
	}

free_msgs:
	oss_free(msgs);

free_buff:
	oss_free(buffs);

	oss_mutex_unlock(control->tbl_mutex);

	return ret == num ? 0 : AMDGV_FAILURE;
}

static const struct amdgv_ras_eeprom_funcs ras_eeprom_legacy_funcs = {
	.init = ras_eeprom_legacy_init,
	.fini = NULL,
	.reset_table = ras_eeprom_legacy_reset_table,
	.process_records = ras_eeprom_legacy_process_records,
};

int ras_eeprom_legacy_sw_init(struct amdgv_adapter *adapt)
{
	adapt->ras_eeprom.funcs = &ras_eeprom_legacy_funcs;
	return 0;
}

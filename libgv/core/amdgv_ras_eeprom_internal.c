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

typedef uint64_t __le64;
static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

bool __get_eeprom_i2c_params(struct amdgv_adapter *adapt,
				    struct amdgv_ras_eeprom_control *control)
{
	if (!control)
		return false;

	if (adapt->umc.funcs && adapt->umc.funcs->get_eeprom_i2c_params) {
		adapt->umc.funcs->get_eeprom_i2c_params(adapt, control);
		return true;
	}

	return false;
}

/* RAS uses SMU GPIO port (port 1) */
int __smu_i2c_transfer(struct amdgv_adapter *adapt,
			      struct amdgv_ras_eeprom_control *control, struct i2c_msg *msgs,
			      int num)
{
	int ret = 0;

	if (adapt->pp.pp_funcs->i2c_eeprom_xfer) {
		oss_mutex_lock(adapt->smu_i2c_lock);
		ret = adapt->pp.pp_funcs->i2c_eeprom_xfer(adapt, control->i2c_port, msgs, num);
		oss_mutex_unlock(adapt->smu_i2c_lock);
	}

	return ret;
}

/* MI300 & NV32 have different interfaces */
void __smu_send_rma_reason(struct amdgv_adapter *adapt)
{
	if (adapt->pp.pp_funcs->send_rma_reason) {
		adapt->pp.pp_funcs->send_rma_reason(adapt, PP_RMA_BAD_PAGE_THRESHOLD);
	}

	if (adapt->gpumon.funcs->ras_report &&
	    adapt->gpumon.funcs->ras_report(adapt, (int)PP_RAS_TYPE__RMA)) {
		AMDGV_ERROR("SMU is not responding, unable to report RMA to SMBUS\n");
	}
}

void __encode_table_header_to_buff(struct amdgv_ras_eeprom_table_header *hdr,
					  unsigned char *buff)
{
	uint32_t *pp = (uint32_t *)buff;

	pp[0] = cpu_to_le32(hdr->header);
	pp[1] = cpu_to_le32(hdr->version);
	pp[2] = cpu_to_le32(hdr->first_rec_offset);
	pp[3] = cpu_to_le32(hdr->tbl_size);
	pp[4] = cpu_to_le32(hdr->checksum);
}

void __decode_table_header_from_buff(struct amdgv_ras_eeprom_table_header *hdr,
					    unsigned char *buff)
{
	uint32_t *pp = (uint32_t *)buff;

	hdr->header = le32_to_cpu(pp[0]);
	hdr->version = le32_to_cpu(pp[1]);
	hdr->first_rec_offset = le32_to_cpu(pp[2]);
	hdr->tbl_size = le32_to_cpu(pp[3]);
	hdr->checksum = le32_to_cpu(pp[4]);
}

void __encode_table_record_to_buff(struct amdgv_ras_eeprom_control *control,
					  struct eeprom_table_record *record,
					  unsigned char *buff)
{
	__le64 tmp = 0;
	int i = 0;

	/* Next are all record fields according to EEPROM page spec in LE foramt */
	buff[i++] = record->err_type;

	buff[i++] = record->bank;

	tmp = cpu_to_le64(record->ts);
	oss_memcpy(buff + i, &tmp, 8);
	i += 8;

	tmp = cpu_to_le64((record->offset & 0xffffffffffff));
	oss_memcpy(buff + i, &tmp, 6);
	i += 6;

	buff[i++] = record->mem_channel;
	buff[i++] = record->mcumc_id;

	tmp = cpu_to_le64((record->retired_page & 0xffffffffffff));
	oss_memcpy(buff + i, &tmp, 6);
}

void __decode_table_record_from_buff(struct amdgv_ras_eeprom_control *control,
					    struct eeprom_table_record *record,
					    unsigned char *buff)
{
	__le64 tmp = 0;
	int i = 0;

	/* Next are all record fields according to EEPROM page spec in LE foramt */
	record->err_type = buff[i++];

	record->bank = buff[i++];

	oss_memcpy(&tmp, buff + i, 8);
	record->ts = le64_to_cpu(tmp);
	i += 8;

	oss_memcpy(&tmp, buff + i, 6);
	record->offset = (le64_to_cpu(tmp) & 0xffffffffffff);
	i += 6;

	record->mem_channel = buff[i++];
	record->mcumc_id = buff[i++];

	oss_memcpy(&tmp, buff + i, 6);
	record->retired_page = (le64_to_cpu(tmp) & 0xffffffffffff);
}

uint32_t __calc_hdr_byte_sum(struct amdgv_ras_eeprom_control *control)
{
	int i;
	uint32_t tbl_sum = 0;

	/* Header checksum, skip checksum field in the calculation */
	for (i = 0; i < sizeof(control->tbl_hdr) - sizeof(control->tbl_hdr.checksum); i++)
		tbl_sum += *(((unsigned char *)&control->tbl_hdr) + i);

	return tbl_sum;
}

uint32_t __calc_extra_info_byte_sum(struct amdgv_ras_eeprom_control *control)
{
	int i;
	uint32_t extra_info_sum = 0;

	/* calculate table extra gpu info available field */
	for (i = 0; i < EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE; i++)
		extra_info_sum += *(((unsigned char *)&control->tbl_egi_v2_1) + i);

	return extra_info_sum;
}

uint32_t __calc_recs_byte_sum(struct eeprom_table_record *records, int num)
{
	int i, j;
	uint32_t tbl_sum = 0;

	/* Records checksum */
	for (i = 0; i < num; i++) {
		struct eeprom_table_record *record = &records[i];

		for (j = 0; j < sizeof(*record); j++) {
			tbl_sum += *(((unsigned char *)record) + j);
		}
	}

	return tbl_sum;
}

bool __validate_tbl_checksum(struct amdgv_adapter *adapt,
				    struct amdgv_ras_eeprom_control *control)
{
	if (control->tbl_hdr.checksum + (control->tbl_byte_sum % 256) != 256)
		return false;
	else
		return true;
}

void __mark_gpu_bad(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_eeprom_table_header *hdr = &adapt->eeprom_control.tbl_hdr;

	hdr->header = EEPROM_TABLE_HDR_BAD;

	// need to notify PF KMD about bad GPU
	if (adapt->flags & AMDGV_FLAG_USE_PF)
		amdgv_reset_notify_gpu_rma(adapt, AMDGV_PF_IDX);
}

void __update_bad_channel_bitmap(struct amdgv_adapter *adapt,
			struct amdgv_ras_eeprom_control *control,
			struct eeprom_table_record *record)
{
	if ((record->mem_channel < BITS_PER_TYPE(control->bad_channel_bitmap)) &&
		!(control->bad_channel_bitmap & (1 << record->mem_channel))) {
		control->bad_channel_bitmap |= 1 << record->mem_channel;
		adapt->update_channel_flag = true;
	}
}


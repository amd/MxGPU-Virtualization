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

#ifndef _AMDGV_RAS_EEPROM_INTERNAL_H
#define _AMDGV_RAS_EEPROM_INTERNAL_H

#include "amdgv_common_eeprom.h"

/*
 * The 2 macros bellow represent the actual size in bytes that
 * those entities occupy in the EEPROM memory.
 * EEPROM_TABLE_RECORD_SIZE is different than sizeof(eeprom_table_record) which
 * uses uint64 to store 6b fields such as retired_page.
 */
#define EEPROM_TABLE_HEADER_SIZE 20
#define EEPROM_TABLE_RECORD_SIZE 24

#define EEPROM_ADDRESS_SIZE 0x2

/**
 * EEPROM Table structrue V2.1
 * ---------------------------------
 * |                               |
 * |     EEPROM TABLE HEADER       |
 * |      ( size 20 Bytes )        |
 * |                               |
 * ---------------------------------
 * |                               |
 * |  EEPROM TABLE EXTRA GPU INFO  |
 * |            V2.1               |
 * | (available info size 4 Bytes) |
 * |  ( reserved size 252 Bytes )  |
 * |                               |
 * ---------------------------------
 * |                               |
 * |     BAD PAGE RECORD AREA      |
 * |                               |
 * ---------------------------------
 */

/**
 * EEPROM Table structure V2
 * ---------------------------------
 * |                               |
 * |     EEPROM TABLE HEADER       |
 * |      ( size 20 Bytes )        |
 * |                               |
 * ---------------------------------
 * |                               |
 * |    BAD PAGE RECORD AREA       |
 * | (size 100 * bad_page_records) |
 * |                               |
 * ---------------------------------
 * |                               |
 * |  EEPROM TABLE EXTRA GPU INFO  |
 * |              V2               |
 * | (available info size 8 Bytes) |
 * |  ( reserved size 248 Bytes )  |
 * |                               |
 * ---------------------------------
 */

/* Table hdr is 'AMDR' */
#define EEPROM_TABLE_HDR_VAL 0x414d4452
#define EEPROM_TABLE_VER_V1  0x00010000
#define EEPROM_TABLE_HDR_BAD 0x42414447

/* EEPROM V2 HEADER is hardcoded to 100 records */
#define EEPROM_TABLE_V2_MAX_ECC_PAGE_RECORD 100

/* Additional EEPROM Table header */
#define EEPROM_TABLE_V2_EXTRA_GPU_INFO_SIZE 8
#define EEPROM_TABLE_VER_V2                 0x00020000
#define EEPROM_TABLE_V2_EXTRA_GPU_INFO_START                                                 \
	(EEPROM_TABLE_HEADER_SIZE + EEPROM_TABLE_RECORD_SIZE * EEPROM_TABLE_V2_MAX_ECC_PAGE_RECORD)

/* Available info size (byte) in table extra info area */
#define EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_SIZE  4
#define EEPROM_TABLE_VER_V2_1                  0x00021000
#define EEPROM_TABLE_V2_1_EXTRA_GPU_INFO_START EEPROM_TABLE_HEADER_SIZE

/* There are 256Bytes size for EEPROM Table extra gpu info */
#define EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE 256

/* Assume 2 Mbit size */
#define EEPROM_SIZE_BYTES	256000
#define EEPROM_PAGE__SIZE_BYTES 256
#define EEPROM_HDR_START	0
#define EEPROM_RECORD_START     (EEPROM_HDR_START + EEPROM_TABLE_HEADER_SIZE)
#define EEPROM_RECORD_START_V2  (EEPROM_HDR_START + EEPROM_TABLE_HEADER_SIZE)
#define EEPROM_RECORD_START_V2_1                                                              \
	(EEPROM_HDR_START + EEPROM_TABLE_HEADER_SIZE + EEPROM_TABLE_TOTAL_EXTRA_INFO_SIZE)

#define EEPROM_ADDR_MSB_MASK 0x3ff00

/* bad page timestamp format
 * yy[31:27] mm[26:23] day[22:17]  hh[16:12] mm[11:6] ss[5:0]
 */
#define EEPROM_TIMESTAMP_MINUTE  6
#define EEPROM_TIMESTAMP_HOUR    12
#define EEPROM_TIMESTAMP_DAY     17
#define EEPROM_TIMESTAMP_MONTH   23
#define EEPROM_TIMESTAMP_YEAR    27
#define IS_LEAP_YEAR(x) ((x % 4 == 0 && x % 100 != 0) || x % 400 == 0)

#define cpu_to_le64(x) (x)
#define le64_to_cpu(x) (x)


int ras_eeprom_v2_1_sw_init(struct amdgv_adapter *adapt);
int ras_eeprom_legacy_sw_init(struct amdgv_adapter *adapt);


bool __get_eeprom_i2c_params(struct amdgv_adapter *adapt, struct amdgv_ras_eeprom_control *control);
int __smu_i2c_transfer(struct amdgv_adapter *adapt,
		struct amdgv_ras_eeprom_control *control, struct i2c_msg *msgs, int num);
void __smu_send_rma_reason(struct amdgv_adapter *adapt);

uint32_t __calc_hdr_byte_sum(struct amdgv_ras_eeprom_control *control);
uint32_t __calc_extra_info_byte_sum(struct amdgv_ras_eeprom_control *control);
uint32_t __calc_recs_byte_sum(struct eeprom_table_record *records, int num);
bool __validate_tbl_checksum(struct amdgv_adapter *adapt,
				    struct amdgv_ras_eeprom_control *control);

void __encode_table_header_to_buff(struct amdgv_ras_eeprom_table_header *hdr, unsigned char *buff);
void __decode_table_header_from_buff(struct amdgv_ras_eeprom_table_header *hdr, unsigned char *buff);
void __encode_table_record_to_buff(struct amdgv_ras_eeprom_control *control,
		struct eeprom_table_record *record, unsigned char *buff);
void __decode_table_record_from_buff(struct amdgv_ras_eeprom_control *control,
		struct eeprom_table_record *record, unsigned char *buff);

void __mark_gpu_bad(struct amdgv_adapter *adapt);
void __update_bad_channel_bitmap(struct amdgv_adapter *adapt,
		struct amdgv_ras_eeprom_control *control, struct eeprom_table_record *record);

#endif // _AMDGV_RAS_EEPROM_INTERNAL_H

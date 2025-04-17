/*
 * Copyright (c) 2019-2024 Advanced Micro Devices, Inc. All rights reserved.
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


uint64_t amdgv_utc_to_eeprom_format(struct amdgv_adapter *adapt, uint64_t utc_timestamp)
{
	uint64_t eeprom_timestamp = 0;
	uint64_t year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;

	int seconds_per_day = 24 * 60 * 60;
	int seconds_per_hour = 60 * 60;
	int seconds_per_minute = 60;

	int days = utc_timestamp / seconds_per_day;
	int remaining_seconds = utc_timestamp % seconds_per_day;

	int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

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

	/* the year range is 2000 ~ 2031, set the year if not in the range */
	if (year <= 2000)
		year = 2000;
	if (year >= 2031)
		year = 2031;

	year -= 2000;

	eeprom_timestamp = second + (minute << EEPROM_TIMESTAMP_MINUTE)
							+ (hour << EEPROM_TIMESTAMP_HOUR)
							+ (day << EEPROM_TIMESTAMP_DAY)
							+ (month << EEPROM_TIMESTAMP_MONTH)
							+ (year << EEPROM_TIMESTAMP_YEAR);
	eeprom_timestamp &= 0xffffffff;

	return eeprom_timestamp;
}


int amdgv_ras_eeprom_reset_table(struct amdgv_adapter *adapt,
				 struct amdgv_ras_eeprom_control *control)
{

	int ret = AMDGV_FAILURE;

	if (adapt->ras_eeprom.funcs->reset_table)
		ret = adapt->ras_eeprom.funcs->reset_table(adapt, control);
	else
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_EEPROM_RESET_FAILED, 0);

	return ret;
}

int amdgv_ras_eeprom_process_records(struct amdgv_adapter *adapt,
				     struct amdgv_ras_eeprom_control *control,
				     struct eeprom_table_record *records, bool write, int num)
{
	int ret = AMDGV_FAILURE;

	if (adapt->ras_eeprom.funcs->process_records)
		ret = adapt->ras_eeprom.funcs->process_records(adapt, control, records, write, num);
	else
		AMDGV_ERROR("Cannot process EEPROM Records.\n");

	return ret;
}

bool amdgv_ras_eeprom_is_gpu_bad(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_eeprom_table_header *hdr = &adapt->eeprom_control.tbl_hdr;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return false;

	return (hdr->header == EEPROM_TABLE_HDR_BAD);
}

int amdgv_ras_eeprom_sw_init(struct amdgv_adapter *adapt,
			     struct amdgv_ras_eeprom_control *control)
{
	int ret = AMDGV_FAILURE;

	if (adapt->umc.use_legacy_eeprom_format) {
		AMDGV_INFO("Using legacy EEPROM format.\n");
		ret = ras_eeprom_legacy_sw_init(adapt);
	} else {
		AMDGV_INFO("Using EEPROM format v2.1.\n");
		ret = ras_eeprom_v2_1_sw_init(adapt);
	}

	return ret;
}

int amdgv_ras_eeprom_init(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control)
{

	int ret = AMDGV_FAILURE;

	if (adapt->ras_eeprom.funcs->init)
		ret = adapt->ras_eeprom.funcs->init(adapt, control);
	else
		AMDGV_ERROR("Cannot init EEPROM control.\n");

	return ret;
}

void amdgv_ras_eeprom_fini(struct amdgv_ras_eeprom_control *control)
{
}


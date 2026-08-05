/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_ras_eeprom.h"
#include "amdgv_ras_eeprom_internal.h"

typedef uint64_t __le64;
static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

/**
 * Convert UTC timestamp to date/time components
 *
 * This unified function extracts date/time components from a UTC timestamp,
 * eliminating code duplication between EEPROM format conversion functions.
 *
 * @utc_timestamp: UTC timestamp in seconds since Unix epoch
 * @dt: Pointer to structure to store the extracted date/time components
 */
void amdgv_utc_to_datetime_components(struct amdgv_adapter *adapt, uint64_t utc_timestamp, struct utc_datetime *dt)
{
	const uint32_t seconds_per_day = 24 * 60 * 60;
	const uint32_t seconds_per_hour = 60 * 60;
	const uint32_t seconds_per_minute = 60;

	uint32_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	uint32_t days = utc_timestamp / seconds_per_day;
	uint32_t remaining_seconds = utc_timestamp % seconds_per_day;
	uint32_t days_in_year;

	/* Calculate year */
	dt->year = 1970;
	while (days >= 365) {
		days_in_year = IS_LEAP_YEAR(dt->year) ? 366 : 365;
		if (days < days_in_year)
			break;
		days -= days_in_year;
		dt->year++;
	}

	/* Calculate month and day */
	if (IS_LEAP_YEAR(dt->year)) {
		days_in_month[1] = 29;
	}

	dt->month = 1; // January
	while (dt->month <= 12 && days >= days_in_month[dt->month - 1]) {
		days -= days_in_month[dt->month - 1];
		dt->month++;
	}
	dt->day = days + 1;

	dt->hour = remaining_seconds / seconds_per_hour;
	dt->minute = (remaining_seconds % seconds_per_hour) / seconds_per_minute;
	dt->second = remaining_seconds % seconds_per_minute;

}

/**
 * Convert UTC timestamp to EEPROM format (legacy/v2/v3)
 *
 * @adapt: GPU adapter instance
 * @utc_timestamp: UTC timestamp in seconds since Unix epoch
 *
 * Returns: Formatted timestamp for EEPROM storage
 */
uint64_t amdgv_utc_to_eeprom_format(struct amdgv_adapter *adapt, uint64_t utc_timestamp)
{
	uint64_t eeprom_timestamp = 0;
	struct utc_datetime dt;

	/* Convert UTC timestamp to date/time components */
	amdgv_utc_to_datetime_components(adapt, utc_timestamp, &dt);

	if (dt.year <= 2000)
		dt.year = 2000;
	if (dt.year >= 2031)
		dt.year = 2031;

	dt.year -= 2000;

	eeprom_timestamp = dt.second + (dt.minute << EEPROM_TIMESTAMP_MINUTE)
			 + (dt.hour << EEPROM_TIMESTAMP_HOUR)
			 + (dt.day << EEPROM_TIMESTAMP_DAY)
			 + (dt.month << EEPROM_TIMESTAMP_MONTH)
			 + (dt.year << EEPROM_TIMESTAMP_YEAR);
	eeprom_timestamp &= 0xffffffff;

	return eeprom_timestamp;
}

uint64_t amdgv_ras_eeprom_utc_to_eeprom_format(struct amdgv_adapter *adapt,
				 uint64_t utc_timestamp)
{
	if (adapt->ras_eeprom.funcs && adapt->ras_eeprom.funcs->utc_to_eeprom_format)
		return adapt->ras_eeprom.funcs->utc_to_eeprom_format(adapt, utc_timestamp);
	else
		AMDGV_ERROR("Cannot convert UTC timestamp to EEPROM format.\n");

	return 0;

}

int amdgv_ras_eeprom_reset_table(struct amdgv_adapter *adapt,
				 struct amdgv_ras_eeprom_control *control)
{

	int ret = AMDGV_FAILURE;

	if (adapt->ras_eeprom.funcs && adapt->ras_eeprom.funcs->reset_table)
		ret = adapt->ras_eeprom.funcs->reset_table(adapt, control);
	else
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_ECC_EEPROM_RESET_FAILED, 0);

	return ret;
}

int amdgv_ras_eeprom_process_records(struct amdgv_adapter *adapt,
				     struct amdgv_ras_eeprom_control *control,
				     struct eeprom_table_record *records, bool write, int num)
{
	int ret = AMDGV_FAILURE;

	if (adapt->ras_eeprom.funcs && adapt->ras_eeprom.funcs->process_records)
		ret = adapt->ras_eeprom.funcs->process_records(adapt, control, records, write, num);
	else
		AMDGV_ERROR("Cannot process EEPROM Records.\n");

	return ret;
}

bool amdgv_ras_eeprom_is_header_bad(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_eeprom_table_header *hdr = &adapt->eeprom_control.tbl_hdr;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return false;

	return (hdr->header == EEPROM_TABLE_HDR_BAD);
}

bool amdgv_ras_eeprom_is_gpu_bad(struct amdgv_adapter *adapt)
{
#if defined(AMDGV_UNIRAS_SUPPORT)
	if (amdgv_uniras_enabled(adapt))
		return amdgv_ras_mgr_is_rma(adapt);
	else if (adapt->ras_eeprom.funcs && adapt->ras_eeprom.funcs->is_gpu_bad)
#else
	if (adapt->ras_eeprom.funcs && adapt->ras_eeprom.funcs->is_gpu_bad)
#endif
		return adapt->ras_eeprom.funcs->is_gpu_bad(adapt);
	else
		AMDGV_ERROR("Cannot check if GPU is bad.\n");
	return false;
}

int amdgv_ras_eeprom_version_init(struct amdgv_adapter *adapt)
{
	int ret = AMDGV_FAILURE;

	if (adapt->umc.use_legacy_eeprom_format) {
		AMDGV_INFO("Using legacy EEPROM format.\n");
		ret = ras_eeprom_legacy_sw_init(adapt);
	} else {
		if (adapt->pp.pmme_funcs &&
			adapt->pp.pmme_funcs->is_pmfw_managed_eeprom) {
			adapt->umc.is_pmfw_managed_eeprom = adapt->pp.pmme_funcs->is_pmfw_managed_eeprom(adapt);

			if (adapt->umc.is_pmfw_managed_eeprom) {
				/* For pmfw managed eeprom, the bad page threshold is controled by pmfw.
				 * Any changes to the bad page threshold must be done through ras policy update.
				 * Ignore the configured bad page threshold if set by user.
				 */
				if (adapt->opt.bad_page_record_threshold > 0)
					amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_ECC_EEPROM_CONFIG_BP_THD_NOT_SUPPORTED, 0);

				adapt->umc.eeprom_version = EEPROM_TABLE_VER_V4;
				AMDGV_INFO("Using EEPROM format v4.0.\n");
				ret = ras_eeprom_pmme_sw_init(adapt);
				return ret;
			}
		}

		if (adapt->umc.eeprom_version == EEPROM_TABLE_VER_V3)
			AMDGV_INFO("Using EEPROM format v3.0.\n");
		else
			AMDGV_INFO("Using EEPROM format v2.1.\n");
		ret = ras_eeprom_v2_1_sw_init(adapt);
	}

	return ret;
}

int amdgv_ras_eeprom_init(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control)
{

	int ret = AMDGV_FAILURE;

	if (adapt->ras_eeprom.funcs && adapt->ras_eeprom.funcs->init)
		ret = adapt->ras_eeprom.funcs->init(adapt, control);
	else
		AMDGV_ERROR("Cannot init EEPROM control.\n");

	return ret;
}

void amdgv_ras_eeprom_fini(struct amdgv_ras_eeprom_control *control)
{
}

int amdgv_ras_eeprom_export_live_update(struct amdgv_adapter *adapt, uint8_t *data)
{
	if (adapt->umc.use_legacy_eeprom_format || adapt->umc.is_pmfw_managed_eeprom)
		return 0;
	else
		return ras_eeprom_v2_1_export_live_data(adapt, data);
}
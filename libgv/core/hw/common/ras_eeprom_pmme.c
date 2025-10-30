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

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_ras_eeprom.h"
#include "amdgv_ras_eeprom_internal.h"
#include "amdgv_common_eeprom.h"
#include "amdgv_powerplay.h"
#include "hw/AI/mi300/mi350/mi350_smu_ppsmc.h"

#define NUM_BAD_PAGES_PER_RECORD 16
#define NUM_BAD_PAGES(num_rec) ((num_rec) * NUM_BAD_PAGES_PER_RECORD)

/*
 * Check RMA status of the GPU.
 *
 * @adapt: GPU adapter instance
 *
 * Returns: ture if rma status is set, false otherwise.
 */
static bool ras_eeprom_pmme_is_gpu_bad(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int rma_status = 0;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return false;

	if (adapt->pp.pmme_funcs && adapt->pp.pmme_funcs->get_rma_status) {
		ret = adapt->pp.pmme_funcs->get_rma_status(adapt, &rma_status);
		if (ret)
			return false;
		if (rma_status) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_REACH_THD, BAD_PAGE_RECORD_THRESHOLD);
			return true;
		} else
			return false;
	}

	return false;
}

/**
 * Convert UTC timestamp to EEPROM v4 format
 *
 * @adapt: GPU adapter instance
 * @utc_timestamp: UTC timestamp in seconds since Unix epoch
 *
 * Returns: Formatted timestamp for EEPROM v4 storage
 */
static uint64_t ras_eeprom_pmme_utc_to_eeprom_format(struct amdgv_adapter *adapt, uint64_t utc_timestamp)
{
	uint64_t eeprom_timestamp = 0;
	uint32_t eeprom_timestamp_hi = 0, eeprom_timestamp_lo = 0;
	struct utc_datetime dt;

	/* Convert UTC timestamp to date/time components */
	amdgv_utc_to_datetime_components(adapt, utc_timestamp, &dt);

	if (dt.year <= 2000)
		dt.year = 2000;
	dt.year -= 2000;

	eeprom_timestamp_lo = dt.second + (dt.minute << EEPROM_V4_TIMESTAMP_MINUTE)
			+ (dt.hour << EEPROM_V4_TIMESTAMP_HOUR);

	eeprom_timestamp_hi = dt.day + (dt.month << EEPROM_V4_TIMESTAMP_MONTH)
			+ (dt.year << EEPROM_V4_TIMESTAMP_YEAR);

	eeprom_timestamp = ((uint64_t)eeprom_timestamp_hi << 32) | eeprom_timestamp_lo;

	return eeprom_timestamp;
}

/*
 * Process EEPROM bad page records via PMFW.
 *
 * @adapt: GPU adapter instance
 * @control: RAS EEPROM control structure
 * @records: Array to store the retrieved records
 * @write: Write operation flag (currently not supported)
 * @num: Maximum number of records to process
 *
 * Returns: 0 on success, negative error code on failure
 */
static int ras_eeprom_pmme_process_records(struct amdgv_adapter *adapt,
			struct amdgv_ras_eeprom_control *control,
			struct eeprom_table_record *records,
			bool write,
			int num)
{
	int i, ret = 0;

	struct amdgv_ras_eeprom_bad_page_info bp_info = {0};

	if (write)
		return AMDGV_FAILURE; // Write operation not supported in this version

	if (!control || !records || num <= 0)
		return AMDGV_FAILURE;

	if (!adapt->umc.supports_ras_eeprom)
		return 0;

	oss_mutex_lock(control->tbl_mutex);

	for (i = 0; i < num; i++) {
		if (adapt->pp.pmme_funcs &&
			adapt->pp.pmme_funcs->get_bad_page_info_details) {
			ret = adapt->pp.pmme_funcs->get_bad_page_info_details(adapt,
						control->next_bp_record_idx,
						&bp_info);
			if (ret)
				goto out;

			ret = amdgv_umc_set_eeprom_record(adapt, &records[i], &bp_info);
			if (ret)
				goto out;
		}

		control->next_bp_record_idx++;
	}

out:
	oss_mutex_unlock(control->tbl_mutex);
	return ret;
}

/*
 * Initialize RAS EEPROM v4 functionality.
 *
 * @adapt: GPU adapter instance
 * @control: RAS EEPROM control structure to initialize
 *
 * Returns: 0 on success, negative error code on failure
 */
static int ras_eeprom_pmme_init(struct amdgv_adapter *adapt,
			struct amdgv_ras_eeprom_control *control)
{
	int ret = 0;

	if (!control)
		return AMDGV_FAILURE;

	if (!adapt->umc.supports_ras_eeprom)
		return 0;

	/*
	 * Check if PMME (PMFW Managed EEPROM) is ready before proceeding.
	 * PMME readiness is required for EEPROM operations; if not ready, attempt to query
	 * the status via PowerPlay interface. Initialization fails if PMME is not ready.
	 */
	if(!adapt->umc.is_pmme_ready) {
		if (adapt->pp.pmme_funcs &&
			adapt->pp.pmme_funcs->is_pmme_ready) {
			adapt->umc.is_pmme_ready = adapt->pp.pmme_funcs->is_pmme_ready(adapt);
			if (!adapt->umc.is_pmme_ready)
				return AMDGV_FAILURE;
		}
	}

	/* Initialize control structure fields */
	if (!in_whole_gpu_reset()) {
		control->next_bp_record_idx = 0;
		control->num_recs = 0;
	}

	if (adapt->pp.pmme_funcs &&
		adapt->pp.pmme_funcs->get_ras_table_version) {
		ret = adapt->pp.pmme_funcs->get_ras_table_version(adapt,
					&control->tbl_hdr.version);
		if (ret) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_EEPROM_VERSION_PARSE_FAILED, 0);
			return ret;
		}
	}

	if (adapt->pp.pmme_funcs &&
		adapt->pp.pmme_funcs->get_ras_policy_details) {
		ret = adapt->pp.pmme_funcs->get_ras_policy_details(adapt,
			&control->ras_policy_info);
		if (ret)
			return ret;

		adapt->ecc.bad_page_record_threshold = NUM_BAD_PAGES(control->ras_policy_info.dram_non_critical_region_threshold);
	}

	control->max_record_num = adapt->ecc.bad_page_record_threshold;

	return ret;
}

/*
 * Reset the RAS EEPROM table by erasing all stored records.
 *
 * @adapt: GPU adapter instance
 * @control: RAS EEPROM control structure
 *
 * Returns: 0 on success, negative error code on failure
 */
static int ras_eeprom_pmme_reset_table(struct amdgv_adapter *adapt,
			struct amdgv_ras_eeprom_control *control)
{
	int ret = 0, status = 0;

	if (!control)
		return AMDGV_FAILURE;

	oss_mutex_lock(control->tbl_mutex);

	if (adapt->pp.pmme_funcs &&
		adapt->pp.pmme_funcs->erase_ras_table) {
		ret = adapt->pp.pmme_funcs->erase_ras_table(adapt, &status);
		if (ret)
			goto unlock;

		if (status) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_EEPROM_RESET_FAILED, 0);
			ret = AMDGV_FAILURE;
			goto unlock;
		} else {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_ECC_EEPROM_RESET, 0);
		}
	}

	/* Reset control structure counters */
	control->num_recs = 0;
	control->next_bp_record_idx = 0;

unlock:
	oss_mutex_unlock(control->tbl_mutex);
	return ret;
}

/* RAS EEPROM v4 function table */
static const struct amdgv_ras_eeprom_funcs ras_eeprom_pmme_funcs = {
	.init                  = ras_eeprom_pmme_init,
	.fini                  = NULL,
	.reset_table           = ras_eeprom_pmme_reset_table,
	.process_records       = ras_eeprom_pmme_process_records,
	.utc_to_eeprom_format  = ras_eeprom_pmme_utc_to_eeprom_format,
	.is_gpu_bad            = ras_eeprom_pmme_is_gpu_bad,
};

/*
 * Initialize RAS EEPROM v4 software interface.
 *
 * @adapt: GPU adapter instance
 *
 * Returns: 0 on success, AMDGV_FAILURE on failure
 */
int ras_eeprom_pmme_sw_init(struct amdgv_adapter *adapt)
{
	adapt->ras_eeprom.funcs = &ras_eeprom_pmme_funcs;
	return 0;
}
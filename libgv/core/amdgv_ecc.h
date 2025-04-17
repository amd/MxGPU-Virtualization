/*
 * Copyright (c) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_ECC_H
#define AMDGV_ECC_H
#include "amdgv_ras.h"

#define MAX_UMC_CHANNEL_NUM 32
#define BAD_PAGE_RECORD_THRESHOLD ((adapt->ecc.bad_page_record_threshold) ? adapt->ecc.bad_page_record_threshold : 10)

#define MAX_IN_BAND_QUERY_ECC_CNT  100
#define MAX_BAD_PAGE_THRESHOLD 256

enum amdgv_ecc_bad_page_detection {
	AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS = 0,
	AMDGV_RAS_ECC_FLAG_IGNORE_RMA
};

enum amdgv_ecc_type_support {
	AMDGV_RAS_MEM_ECC_SUPPORT = 0,
	AMDGV_RAS_SRAM_ECC_SUPPORT,
	AMDGV_RAS_POISON_ECC_SUPPORT
};

enum amdgv_ecc_correction_schema_support {
	AMDGV_RAS_ECC_SUPPORT_PARITY = 0,
	AMDGV_RAS_ECC_SUPPORT_CORRECTABLE,
	AMDGV_RAS_ECC_SUPPORT_UNCORRECTABLE,
	AMDGV_RAS_ECC_SUPPORT_POISON,
	AMDGV_RAS_ECC_SUPPORT_MAX,
};

struct ecc_info_per_ch {
	uint16_t ce_count_lo_chip;
	uint16_t ce_count_hi_chip;
	uint64_t mca_umc_status;
	uint64_t mca_umc_addr;
};

struct umc_ecc_info {
	struct ecc_info_per_ch ecc[MAX_UMC_CHANNEL_NUM];
};

struct amdgv_ecc {
	uint32_t supported;
	uint32_t enabled;
	uint32_t ras_cap;

	/* record umc error count info */
	uint32_t correctable_error_num;
	uint32_t uncorrectable_error_num;
	uint32_t deferred_error_num;
	uint32_t max_de_query_retry;
	uint32_t uncorrectable_count_overflow;
	uint64_t uncorrectable_address;

	/* increment when deferred error is detected,
	 * but no action (i.e. poison consumption) has been taken yet */
	uint32_t pending_de_count;

	/* desired max bad page count */
	uint32_t bad_page_record_threshold;


	/* data poison fatal error flag */
	bool fatal_error;
	/* skip ROW RMA Condition flag */
	bool skip_row_rma;

	/* record gfx error count info */
	uint32_t gfx_correctable_error_num;
	uint32_t gfx_uncorrectable_error_num;

	/* record sdma error count info */
	uint32_t sdma_correctable_error_num;
	uint32_t sdma_uncorrectable_error_num;

	/* record mmhub error count info */
	uint32_t mmhub_correctable_error_num;
	uint32_t mmhub_uncorrectable_error_num;

	/* record xgmi error count info */
	uint32_t xgmi_correctable_error_num;
	uint32_t xgmi_uncorrectable_error_num;

	/* protect uncorrectable_error_num */
	spin_lock_t query_err_lock;
	/* protect eh_data */
	mutex_t recovery_lock;
	/* record ras bad page info */
	struct ras_err_handler_data *eh_data;
	struct umc_ecc_info umc_ecc;
	/* indicate bad page detection mode*/
	uint32_t bad_page_detection_mode;

	/* unhandled bad page stack, since FFBM invalidation needs
	 * to switch to PF, the FFBM page replacement will happen
	 * after the FLR, so record the address here, then after
	 * FLR FFBM will replace the page. Use stack to aviod multi-poison
	 * before a reset happens, normally this will have only one entry*/
	struct eeprom_table_record last_err_bps[MAX_UMC_CHANNEL_NUM];
	uint32_t last_err_bps_cnt;
	mutex_t unhandled_bps_lock;

	int (*toggle_ecc_mode)(struct amdgv_adapter *adapt);
	int (*get_correctable_error_count)(struct amdgv_adapter *adapt,
					uint32_t idx_vf);
	int (*get_uncorrectable_error_count)(struct amdgv_adapter *adapt,
					uint32_t idx_vf);
	int (*get_deferred_error_count)(struct amdgv_adapter *adapt,
					uint32_t idx_vf);
	int (*get_error_count)(struct amdgv_adapter *adapt, struct amdgv_smi_ras_query_if *info);
	uint32_t (*get_ras_cap)(struct amdgv_adapter *adapt);
	int (*poison_consumption)(struct amdgv_adapter *adapt, struct amdgv_sched_event *event);
	int (*poison_creation)(struct amdgv_adapter *adapt, struct amdgv_sched_event *event);
};

struct amdgv_ras_err_status_reg_entry;
struct amdgv_ras_memory_id_entry;

extern oss_atomic64_t amdgv_ras_in_intr;

static inline bool amdgv_ras_intr_triggered(void)
{
	return !!oss_atomic_read(&amdgv_ras_in_intr);
}

static inline void amdgv_ras_intr_cleared(void)
{
	oss_atomic_set(&amdgv_ras_in_intr, 0);
}

int amdgv_ecc_clean_correctable_error_count(struct amdgv_adapter *adapt,
			struct amdgv_smi_ras_query_if *info);
int amdgv_ecc_get_correctable_error_count(struct amdgv_adapter *adapt,
					uint32_t idx_vf);
int amdgv_ecc_get_uncorrectable_error_count(struct amdgv_adapter *adapt,
					uint32_t idx_vf);
int amdgv_ecc_get_deferred_error_count(struct amdgv_adapter *adapt,
					uint32_t idx_vf);
int amdgv_ecc_get_error_count(struct amdgv_adapter *adapt, struct amdgv_smi_ras_query_if *info);
void amdgv_ecc_check_for_errors(struct amdgv_adapter *adapt, struct amdgv_sched_event *event);
void amdgv_ecc_check_global_ras_errors(struct amdgv_adapter *adapt);
int amdgv_ecc_enable_ras_feature(struct amdgv_adapter *adapt);
int amdgv_ecc_disable_ras_feature(struct amdgv_adapter *adapt);
int amdgv_ecc_is_support(struct amdgv_adapter *adapt, unsigned int block);
void amdgv_ecc_check_support(struct amdgv_adapter *adapt);
void amdgv_ras_poison_mode_init(struct amdgv_adapter *adapt);
bool amdgv_ras_is_poison_mode_supported(struct amdgv_adapter *adapt);
void amdgv_ecc_get_poison_stat(struct amdgv_adapter *adapt);
void amdgv_ecc_query_ras_errors(struct amdgv_adapter *adapt);
void amdgv_ras_get_error_type_name(uint32_t err_type, char *err_type_name);
bool amdgv_ras_inst_get_memory_id_field(struct amdgv_adapter *adapt,
		const struct amdgv_ras_err_status_reg_entry *reg_entry,
		uint32_t instance,
		uint32_t *memory_id);
bool amdgv_ras_inst_get_err_cnt_field(struct amdgv_adapter *adapt,
		const struct amdgv_ras_err_status_reg_entry *reg_entry,
		uint32_t instance,
		unsigned long *err_cnt);
void amdgv_ras_inst_query_ras_error_count(struct amdgv_adapter *adapt,
		const struct amdgv_ras_err_status_reg_entry *reg_list,
		uint32_t reg_list_size,
		const struct amdgv_ras_memory_id_entry *mem_list,
		uint32_t mem_list_size,
		uint32_t instance,
		uint32_t err_type,
		unsigned long *err_count);
void amdgv_ras_inst_reset_ras_error_count(struct amdgv_adapter *adapt,
		const struct amdgv_ras_err_status_reg_entry *reg_list,
		uint32_t reg_list_size,
		uint32_t instance);
enum amdgv_live_info_status amdgv_ecc_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ecc *ecc_info);
enum amdgv_live_info_status amdgv_ecc_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ecc *ecc_info);
int amdgv_ecc_reset_all_error_counts(struct amdgv_adapter *adapt);
void amdgv_ecc_handle_umc_event(struct amdgv_adapter *adapt, uint32_t idx_vf);
#endif

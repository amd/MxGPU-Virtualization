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
#ifndef __AMDGV_UMC_H__
#define __AMDGV_UMC_H__
#include "amdgv_ras.h"

/* implement 64 bits REG operations via 32 bits interface */
#define RREG64_UMC(reg) (RREG32(reg) | ((uint64_t)RREG32((reg) + 1) << 32))
#define WREG64_UMC(reg, v)                                                                    \
	do {                                                                                  \
		WREG32((reg), lower_32_bits(v));                                              \
		WREG32((reg) + 1, upper_32_bits(v));                                          \
	} while (0)

#define LOOP_UMC_INST(umc_inst)	for ((umc_inst) = 0; (umc_inst) < adapt->umc.umc_inst_num; (umc_inst)++)
#define LOOP_UMC_CH_INST(ch_inst) for ((ch_inst) = 0; (ch_inst) < adapt->umc.channel_inst_num; (ch_inst)++)
#define LOOP_UMC_INST_AND_CH(umc_inst, ch_inst) LOOP_UMC_INST((umc_inst)) LOOP_UMC_CH_INST((ch_inst))

#define LOOP_UMC_NODE_INST(node_inst) \
	for ((node_inst) = 0; (node_inst) < adapt->umc.node_inst_num; (node_inst)++)

#define LOOP_UMC_NODE_INST_AND_CH(node_inst, umc_inst, ch_inst) \
	LOOP_UMC_NODE_INST((node_inst)) LOOP_UMC_INST_AND_CH((umc_inst), (ch_inst))

typedef void (*umc_query_info)(struct amdgv_adapter *adapt, void *data,
						uint32_t node_inst, uint32_t ch_inst, uint32_t umc_inst);

struct amdgv_umc_fb_bank_addr {
	uint32_t stack_id; /* SID */
	uint32_t bank_group;
	uint32_t bank;
	uint32_t row;
	uint32_t column;
	uint32_t channel;
	uint32_t subchannel; /* Also called Pseudochannel (PC) */
};


typedef int (*umc_func)(struct amdgv_adapter *adapt, uint32_t node_inst,
			uint32_t umc_inst, uint32_t ch_inst, void *data);

struct amdgv_umc_funcs {
	void (*err_cnt_init)(struct amdgv_adapter *adapt);
	int (*ras_late_init)(struct amdgv_adapter *adapt);
	void (*query_ras_error_count)(struct amdgv_adapter *adapt, void *ras_error_status);
	void (*query_ras_error_address)(struct amdgv_adapter *adapt, void *ras_error_status);
	void (*hw_query_ras_error_address)(struct amdgv_adapter *adapt, void *ras_error_status);
	void (*hw_query_ras_error_count)(struct amdgv_adapter *adapt, void *ras_error_status);
	void (*enable_umc_index_mode)(struct amdgv_adapter *adapt, uint32_t umc_instance);
	void (*disable_umc_index_mode)(struct amdgv_adapter *adapt);
	void (*init_registers)(struct amdgv_adapter *adapt);
	bool (*query_ras_poison_mode)(struct amdgv_adapter *adapt);
	void (*get_eeprom_i2c_params)(struct amdgv_adapter *adapt, struct amdgv_ras_eeprom_control *control);
	void (*set_eeprom_table_version)(struct amdgv_adapter *adapt);
	void (*query_ras_memchandis)(struct amdgv_adapter *adapt);
	int (*get_ras_vf_safe_range)(struct amdgv_adapter *adapt, uint64_t *offset, uint64_t *size, uint32_t max_entry_num);
	int (*bank_to_soc_pa)(struct amdgv_adapter *adapt, struct amdgv_umc_fb_bank_addr bank_addr, uint64_t *soc_pa);
	int (*soc_pa_to_bank)(struct amdgv_adapter *adapt, uint64_t soc_pa, struct amdgv_umc_fb_bank_addr *bank_addr);
	int (*eeprom_record_to_soc_pa)(struct amdgv_adapter *adapt, struct eeprom_table_record *record, uint64_t *pa_pfn);
	int (*eeprom_record_to_pages)(struct amdgv_adapter *adapt, struct eeprom_table_record *record, uint64_t *pages, uint32_t num);
};

struct amdgv_umc {
	/* max error count in one ras query call */
	uint32_t max_ras_err_cnt_per_query;
	/* number of mcd instance without harvest instance */
	uint8_t  num_umc;
	/* number of umc node instance with memory map register access */
	uint32_t node_inst_num;
	/* number of umc channel instance with memory map register access */
	uint32_t channel_inst_num;
	/* number of umc instance with memory map register access */
	uint32_t umc_inst_num;
	/* UMC regiser per node offset */
	uint32_t node_offs;
	/* UMC regiser per instance offset */
	uint32_t inst_offs;
	/* UMC regiser per channel offset */
	uint32_t channel_offs;
	/* point to the array saving error address */
	struct eeprom_table_record *err_addr;
	/* channel index table of interleaved memory */
	const uint32_t *channel_idx_tbl;
	struct ras_common_if *ras_if;

	const struct amdgv_umc_funcs *funcs;
	bool supports_ras_eeprom;
	uint32_t reset_mode;
	/* active mask for umc node instance */
	uint64_t active_mask;

	uint8_t channel_dis_num;
	/* channel mask for available channel instance */
	uint64_t channel_mask;

	bool use_legacy_eeprom_format;
};

struct ras_err_data;

void amdgv_umc_badpages_count_read(struct amdgv_adapter *adapt, int *bp_cnt);
int amdgv_umc_get_badpages_record(struct amdgv_adapter *adapt, uint32_t index, void *record);
int amdgv_umc_badpages_read(struct amdgv_adapter *adapt, void **bp, unsigned int *count);
bool amdgv_umc_check_bad_page(struct amdgv_adapter *adapt, uint64_t addr);
int amdgv_umc_load_bad_pages(struct amdgv_adapter *adapt);
int amdgv_umc_clean_bad_page_records(struct amdgv_adapter *adapt);
void amdgv_umc_notify_vf_bp_records(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_umc_update_fatal_error_record(struct amdgv_adapter *adapt);
void amdgv_umc_add_addr_to_vf_bad_block_region(struct amdgv_adapter *adapt,
					       uint64_t err_addr_pf);
int amdgv_umc_recovery_sw_init(struct amdgv_adapter *adapt);
int amdgv_umc_recovery_sw_fini(struct amdgv_adapter *adapt);
int amdgv_umc_recovery_hw_init(struct amdgv_adapter *adapt);
int amdgv_umc_recovery_hw_fini(struct amdgv_adapter *adapt);
int amdgv_umc_add_bad_pages(struct amdgv_adapter *adapt,
		struct eeprom_table_record *bps, int pages, bool from_eeprom);
int amdgv_umc_reserve_bad_pages(struct amdgv_adapter *adapt);
int amdgv_umc_save_bad_pages(struct amdgv_adapter *adapt);
int amdgv_umc_reload_bp_from_rom(struct amdgv_adapter *adapt);
void *amdgv_umc_grow_bp_buff(void *buff, uint32_t *cap, uint64_t size);
int amdgv_umc_update_bp_buff(struct amdgv_adapter *adapt, struct eeprom_table_record **bp_buff, uint32_t pages, uint32_t *cap);
int amdgv_umc_release_bad_pages(struct amdgv_adapter *adapt);
int amdgv_umc_retrieve_bad_pages(struct amdgv_adapter *adapt);
int amdgv_umc_process_ras_data_cb(struct amdgv_adapter *adapt,
			void *ras_error_status, uint32_t idx_vf);
int amdgv_umc_update_uc_error_count(struct amdgv_adapter *adapt,
			uint32_t idx_vf);
int amdgv_umc_update_error_count(struct amdgv_adapter *adapt,
			uint32_t idx_vf);
int amdgv_umc_update_df_error_count(struct amdgv_adapter *adapt,
			uint32_t idx_vf);
int amdgv_umc_ras_lock_init(struct amdgv_adapter *adapt);
void amdgv_umc_ras_lock_fini(struct amdgv_adapter *adapt);
int amdgv_umc_loop_channels(struct amdgv_adapter *adapt, umc_func func, void *data);
void amdgv_umc_fill_error_record(struct amdgv_adapter *adapt,
		struct ras_err_data *err_data,
		uint64_t err_addr,
		uint64_t retired_page,
		uint32_t channel_index,
		uint32_t umc_inst);

int amdgv_umc_soc_pa_to_bank(struct amdgv_adapter *adapt,
	uint64_t soc_pa, struct amdgv_umc_fb_bank_addr *bank_addr);
int amdgv_umc_bank_to_soc_pa(struct amdgv_adapter *adapt,
	struct amdgv_umc_fb_bank_addr bank_addr, uint64_t *soc_pa);
int amdgv_umc_get_ras_vf_safe_range(struct amdgv_adapter *adapt,
	uint64_t *offset, uint64_t *size, uint32_t max_entry_num);
int amdgv_umc_reserve_bad_pages(struct amdgv_adapter *adapt);
int amdgv_umc_local_gpa_to_spa(struct amdgv_adapter *adapt,
	uint64_t gpa, uint32_t idx_vf, uint64_t *spa);
int amdgv_umc_local_spa_to_gpa(struct amdgv_adapter *adapt, uint64_t spa,
	uint64_t *gpa, uint32_t *idx_vf);

#endif

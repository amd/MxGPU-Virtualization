/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

/*
 * save nps value to eeprom_table_record.retired_page[47:40],
 * the channel index flag above will be retired.
 */
#define UMC_NPS_SHIFT 40
#define UMC_NPS_MASK 0xffULL

static inline enum amdgv_memory_partition_mode get_nps_from_pa(uint64_t pa)
{
	return (enum amdgv_memory_partition_mode)((pa >> UMC_NPS_SHIFT) & UMC_NPS_MASK);
}

static inline uint64_t set_nps_to_pa(uint64_t pa, enum amdgv_memory_partition_mode nps)
{
	uint64_t nps_64 = (uint64_t)nps;
	pa &= ~(UMC_NPS_MASK << UMC_NPS_SHIFT);
	pa |= (nps_64 << UMC_NPS_SHIFT);
	return pa;
}
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
	int (*set_eeprom_record)(struct amdgv_adapter *adapt, struct eeprom_table_record *record, struct amdgv_ras_eeprom_bad_page_info *bp_info);
	int (*pages_in_a_row)(struct amdgv_adapter *adapt, uint64_t pa, uint64_t *pfns, int len);
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
	uint32_t eeprom_version;
	bool is_pmfw_managed_eeprom;
	bool is_pmme_ready;
};

struct ras_err_data;

void amdgv_umc_badpages_count_read(struct amdgv_adapter *adapt, int *bp_cnt);
int amdgv_umc_get_badpages_record(struct amdgv_adapter *adapt, uint32_t index, void *record);
int amdgv_umc_badpages_read(struct amdgv_adapter *adapt, void **bp, unsigned int *count);
bool amdgv_umc_check_bad_page(struct amdgv_adapter *adapt, uint64_t addr);
bool amdgv_umc_check_bad_pages_in_range(struct amdgv_adapter *adapt, uint64_t fb_offset, uint64_t size);
int amdgv_umc_load_bad_pages(struct amdgv_adapter *adapt);
int amdgv_umc_load_bad_pages_across_nps(struct amdgv_adapter *adapt);
int amdgv_umc_clean_bad_page_records(struct amdgv_adapter *adapt);
int amdgv_umc_copy_bp_records_to_vf(struct amdgv_adapter *adapt,
				    uint32_t idx_vf,
				    uint32_t allowed_size,
				    uint32_t *write_size,
				    uint32_t *more);
int amdgv_umc_update_fatal_error_record(struct amdgv_adapter *adapt);
void amdgv_umc_add_addr_to_vf_bad_block_region(struct amdgv_adapter *adapt,
					       uint64_t err_addr_pf);
int amdgv_umc_sw_init(struct amdgv_adapter *adapt);
int amdgv_umc_sw_fini(struct amdgv_adapter *adapt);
int amdgv_umc_hw_init(struct amdgv_adapter *adapt);
int amdgv_umc_hw_fini(struct amdgv_adapter *adapt);
int amdgv_umc_add_bad_pages(struct amdgv_adapter *adapt,
		struct eeprom_table_record *bps, int pages, bool from_eeprom);
int amdgv_umc_reserve_bad_pages(struct amdgv_adapter *adapt);
int amdgv_umc_save_bad_pages(struct amdgv_adapter *adapt);
void amdgv_umc_check_and_handle_bp_in_crit_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_umc_reload_bp_from_rom(struct amdgv_adapter *adapt);
int amdgv_umc_update_bp_buff(struct amdgv_adapter *adapt, struct eeprom_table_record **bp_buff, uint32_t pages, uint32_t *cap);
int amdgv_umc_release_bad_pages(struct amdgv_adapter *adapt);
int amdgv_umc_retrieve_bad_pages(struct amdgv_adapter *adapt);
int amdgv_umc_replace_bad_pages(struct amdgv_adapter *adapt);

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
uint32_t amdgv_umc_calc_retired_page_vf_slot(struct amdgv_adapter *adapt,
						    uint64_t err_addr);
void amdgv_umc_log_bp_errors(struct amdgv_adapter *adapt, uint32_t record_id);
bool amdgv_umc_check_bp_in_critical_region(struct amdgv_adapter *adapt,
						  uint64_t err_addr, uint32_t idx_vf, bool log_err);
int amdgv_umc_set_eeprom_record(struct amdgv_adapter *adapt,
	struct eeprom_table_record *record, struct amdgv_ras_eeprom_bad_page_info *bp_info);
int amdgv_umc_vf_chk_critical_region(struct amdgv_adapter *adapt, uint64_t err_addr,
				     uint32_t idx_vf, uint32_t *hit);

int amdgv_umc_across_nps_err_data_init(struct amdgv_adapter *adapt);
int amdgv_umc_across_nps_err_data_fini(struct amdgv_adapter *adapt);
int amdgv_umc_fetch_and_sort_bps(struct amdgv_adapter *adapt, uint64_t **bp_offsets);
int amdgv_umc_fetch_and_sort_bps_across_nps(struct amdgv_adapter *adapt, uint64_t **bp_offsets, int *bp_count);
int amdgv_umc_sort_bp_offsets(uint64_t *bp_offsets, uint32_t num_bps);
#endif

/* Sorted bad pages management functions */
int amdgv_umc_init_sorted_bad_pages(struct amdgv_adapter *adapt);
void amdgv_umc_cleanup_sorted_bad_pages(struct amdgv_adapter *adapt);
int amdgv_umc_insert_sorted_bad_page(struct amdgv_adapter *adapt, uint64_t page_addr,
					 struct ras_err_handler_data *data);

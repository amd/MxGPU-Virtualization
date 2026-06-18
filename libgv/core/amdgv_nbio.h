/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AMDGV_NBIO_H__
#define __AMDGV_NBIO_H__

#include "amdgv_nps.h"

struct nbio_hdp_flush_reg {
	uint32_t ref_and_mask_cp0;
	uint32_t ref_and_mask_cp1;
	uint32_t ref_and_mask_cp2;
	uint32_t ref_and_mask_cp3;
	uint32_t ref_and_mask_cp4;
	uint32_t ref_and_mask_cp5;
	uint32_t ref_and_mask_cp6;
	uint32_t ref_and_mask_cp7;
	uint32_t ref_and_mask_cp8;
	uint32_t ref_and_mask_cp9;
	uint32_t ref_and_mask_sdma0;
	uint32_t ref_and_mask_sdma1;
	uint32_t ref_and_mask_sdma2;
	uint32_t ref_and_mask_sdma3;
	uint32_t ref_and_mask_sdma4;
	uint32_t ref_and_mask_sdma5;
	uint32_t ref_and_mask_sdma6;
	uint32_t ref_and_mask_sdma7;
};

struct amdgv_nbio_funcs {
	const struct nbio_hdp_flush_reg *hdp_flush_reg;
	uint32_t (*get_hdp_flush_req_offset)(struct amdgv_adapter *adapt);
	uint32_t (*get_hdp_flush_done_offset)(struct amdgv_adapter *adapt);
	uint32_t (*get_pcie_index_offset)(struct amdgv_adapter *adapt);
	uint32_t (*get_pcie_data_offset)(struct amdgv_adapter *adapt);
	uint32_t (*get_pcie_port_index_offset)(struct amdgv_adapter *adapt);
	uint32_t (*get_pcie_port_data_offset)(struct amdgv_adapter *adapt);
	uint32_t (*get_rev_id)(struct amdgv_adapter *adapt);
	void (*mc_access_enable)(struct amdgv_adapter *adapt, bool enable);
	uint32_t (*get_memsize)(struct amdgv_adapter *adapt);
	void (*sdma_doorbell_range)(struct amdgv_adapter *adapt,
			int instance, bool use_doorbell,
			int doorbell_index, int doorbell_size);
	void (*vcn_doorbell_range)(struct amdgv_adapter *adapt,
				bool use_doorbell, int doorbell_index,
				int instance);
	void (*gc_doorbell_init)(struct amdgv_adapter *adapt);
	void (*enable_doorbell_aperture)(struct amdgv_adapter *adapt,
					 bool enable);
	void (*enable_doorbell_selfring_aperture)(struct amdgv_adapter *adapt,
						  bool enable);
	void (*ih_doorbell_range)(struct amdgv_adapter *adapt,
				  bool use_doorbell, int doorbell_index);
	void (*enable_ih_interrupt)(struct amdgv_adapter *adapt, bool use_bus_addr,
				    uint32_t ih_base_addr);
	void (*update_medium_grain_clock_gating)(struct amdgv_adapter *adapt,
						 bool enable);
	void (*update_medium_grain_light_sleep)(struct amdgv_adapter *adapt,
						bool enable);
	void (*get_clockgating_state)(struct amdgv_adapter *adapt,
				      uint64_t *flags);
	void (*ih_control)(struct amdgv_adapter *adapt);
	void (*init_registers)(struct amdgv_adapter *adapt);
	void (*remap_hdp_registers)(struct amdgv_adapter *adapt);
	void (*enable_aspm)(struct amdgv_adapter *adapt,
			    bool enable);
	void (*program_aspm)(struct amdgv_adapter *adapt);
	void (*apply_lc_spc_mode_wa)(struct amdgv_adapter *adapt);
	void (*apply_l1_link_width_reconfig_wa)(struct amdgv_adapter *adapt);
	void (*clear_doorbell_interrupt)(struct amdgv_adapter *adapt);
	uint32_t (*get_rom_offset)(struct amdgv_adapter *adapt);
	void (*hdp_flush)(struct amdgv_adapter *adapt);
	int (*get_nps_mode)(struct amdgv_adapter *adapt,
			enum amdgv_memory_partition_mode *nps_mode);
	bool (*is_partition_mode_supported)(struct amdgv_adapter *adapt,
			enum amdgv_memory_partition_mode memory_partition_mode,
			enum amdgv_accelerator_partition_mode accelerator_partition_mode);
	int (*get_supported_memory_partition_mode)(struct amdgv_adapter *adapt,
			enum amdgv_memory_partition_mode *supported_nps, int* supported_nps_count);
	enum amdgv_accelerator_partition_mode (*get_accel_partition_mode)(struct amdgv_adapter *adapt);
	enum amdgv_accelerator_partition_mode (*get_default_accel_partition_mode)(struct amdgv_adapter *adapt,
			enum amdgv_memory_partition_mode memory_partition_mode);
	int (*get_asic_nps_caps)(struct amdgv_adapter *adapt,
				 const struct amdgv_nps_compute_combination **combination);
};

struct amdgv_nbio_ras {
	void (*handle_ras_controller_intr_no_bifring)(struct amdgv_adapter *adapt);
	void (*handle_ras_err_event_athub_intr_no_bifring)(struct amdgv_adapter *adapt);
	int (*set_ras_controller_irq_state)(struct amdgv_adapter *adapt, bool state);
	int (*set_ras_err_event_athub_irq_state)(struct amdgv_adapter *adapt, bool state);
	void (*query_ras_error_count)(struct amdgv_adapter *adapt, void *ras_error_status);
};

struct amdgv_nbio {
	const struct nbio_hdp_flush_reg *hdp_flush_reg;
	const struct amdgv_nbio_funcs *funcs;
	const struct amdgv_nbio_ras	*ras;
	uint64_t (*encode_ext_smn_addressing)(uint32_t ext_id);
};

int amdgv_nbio_ih_doorbell_range(struct amdgv_adapter *adapt, bool use_doorbell,
				 int doorbell_index);
int amdgv_nbio_enable_doorbell_aperture(struct amdgv_adapter *adapt, bool enable);
int amdgv_nbio_enable_ih_interrupt(struct amdgv_adapter *adapt, bool use_bus_addr,
				   uint32_t ih_base_addr);
uint32_t amdgv_nbio_get_memsize(struct amdgv_adapter *adapt);
int amdgv_nbio_gc_doorbell_init(struct amdgv_adapter *adapt);
int amdgv_nbio_sdma_doorbell_range(struct amdgv_adapter *adapt, int instance, bool use_doorbell,
				   int doorbell_index, int doorbell_size);

int amdgv_nbio_get_nps_mode(struct amdgv_adapter *adapt,
			    enum amdgv_memory_partition_mode *memory_partition_mode);
bool amdgv_nbio_is_partition_mode_supported(struct amdgv_adapter *adapt,
		enum amdgv_memory_partition_mode memory_partition_mode,
		enum amdgv_accelerator_partition_mode accelerator_partition_mode);
int amdgv_nbio_get_supported_memory_partition_mode(struct amdgv_adapter *adapt,
						   enum amdgv_memory_partition_mode *supported_nps,
						   int* supported_nps_count);
enum amdgv_accelerator_partition_mode
amdgv_nbio_get_accel_partition_mode(struct amdgv_adapter *adapt);
enum amdgv_accelerator_partition_mode
amdgv_nbio_get_default_accel_partition_mode(struct amdgv_adapter *adapt,
					    enum amdgv_memory_partition_mode memory_partition_mode);
int amdgv_nbio_get_asic_nps_caps(struct amdgv_adapter *adapt,
				 const struct amdgv_nps_compute_combination **combination);
#endif


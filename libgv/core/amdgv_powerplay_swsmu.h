/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_POWERPLAY_SWSMU_H
#define AMDGV_POWERPLAY_SWSMU_H
#include "amdgv_device.h"

#define SMU_CAPS(x)		(1ULL << (x))
typedef enum {
	SMU_CAP_STATIC_METRICS,
	SMU_CAP_PLDM_VERSION,
	SMU_CAP_PCIE_METRICS,
	SMU_CAP_STATIC_PCIE_METRICS,
	SMU_CAP_JPEG_VCN_USAGE,
	SMU_CAP_PER_INST_METRICS,
	SMU_CAP_CTF_LIMIT,
	SMU_CAP_RMA_MSG,
	SMU_CAP_ACA_SYND,
	SMU_CAP_HST_LIMIT_METRICS,
	SUM_CAP_XGMI_PLPD,
	SMU_CAP_RESET_VF_ARBITERS,
	SMU_CAP_PTL,
	SMU_CAP_BOARD_METRICS,
	SMU_CAP_TEMP_INST_METRICS,
	SMU_CAP_LIVE_MIGRATION,
	SMU_CAP_ALL,
} SMU_CAPS_LIST_e;

struct smu_local_memory {
	uint64_t size;	    // in bytes
	uint32_t alignment; // in bytes
	union {
		struct {
			uint64_t mc_address;
			void    *virtual_address;
		};
		struct amdgv_memmgr_mem *mem;
	};
};

struct smu_bios_boot_up_values {
	uint32_t revision;
	uint32_t gfxclk;  // in 10KHz
	uint32_t uclk;	  // in 10KHz
	uint32_t socclk;  // in 10KHz
	uint32_t dcefclk; // in 10KHz
	uint16_t vddc;	  // in mV
	uint16_t vddci;	  // in mV
	uint16_t mvddc;	  // in mV
	uint16_t vdd_gfx; // in mV
	uint8_t	 cooling_id;
	uint32_t pp_table_id;
};

struct smu_table_context {
	void    *power_play_table;
	uint32_t power_play_table_size;
	/* Copy of VBIOS powerplayinfo table */
	void *ppt_information;
	/* Copy of VBIOS firmwareinfo table */
	struct smu_bios_boot_up_values boot_values;
	/* SMC tables */
	void                   *driver_pptable;
	void                   *watermark_table;
	void                   *metrics_table;
	void                   *config_table;
	void                   *overdrive_table;
	void                   *avfs_table;
	void                   *i2c_table;
	void                   *activity_monitor_table;
	void                   *ecc_info_table;
	struct smu_local_memory smc_pptable;
	struct smu_local_memory smc_watermark_table;
	struct smu_local_memory smc_avfs_table;
	struct smu_local_memory smc_avfs_psm_debug_table;
	struct smu_local_memory smc_avfs_fuse_override_table;
	struct smu_local_memory smc_pm_status_log_table;
	struct smu_local_memory smc_metrics_table;
	struct smu_local_memory smc_driver_smu_config_table;
	struct smu_local_memory smc_activity_monitor_table;
	struct smu_local_memory smc_overdrive_table;
	struct smu_local_memory smc_i2c_table;
	struct smu_local_memory smc_ecc_info_table;

	/* Debug pool */
	struct smu_local_memory memory_pool;
};

struct smu_context {
	uint64_t features;
	uint64_t supported_caps;
	void    *smu_table_context;
	void    *smu_dpm_context;
	void    *smu_overdrive_context;
	void    *smu_fan_context;
	void    *smu_power_context;
};

extern int smu_get_atom_data_table(struct amdgv_adapter *adapt, uint32_t table, uint16_t *size,
				   uint8_t *frev, uint8_t *crev, uint8_t **addr);
extern uint64_t smu_pp_throttler_event_convert(struct amdgv_adapter *adapt,
			const uint8_t *event_map, int count, uint64_t hw_status);

#endif

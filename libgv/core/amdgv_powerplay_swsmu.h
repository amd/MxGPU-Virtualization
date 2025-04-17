/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_POWERPLAY_SWSMU_H
#define AMDGV_POWERPLAY_SWSMU_H
#include "amdgv_device.h"

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

/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef navi32_POWERPLAY_H
#define navi32_POWERPLAY_H

#include "navi32_smu_driver_if.h"
#include <atombios/atomfirmware.h>

/* MP Apertures */
#define MP0_Public			0x03800000
#define MP0_SRAM			0x03900000
#define MP1_Public			0x03b00000
#define MP1_SRAM			0x03c00004

/* address block */
#define smnMP1_FIRMWARE_FLAGS		0x3010024
#define smnMP0_FW_INTF			0x30101c0
#define smnMP1_PUB_CTRL			0x3010b14

#define TEMP_RANGE_MIN			(0)
#define TEMP_RANGE_MAX			(80 * 1000)

#define SMU13_TOOL_SIZE			0x19000
#define SMU13_PAGE_SIZE			0x1000

#define MAX_DPM_LEVELS 16
#define MAX_PCIE_CONF 3

#define CTF_OFFSET_EDGE			5
#define CTF_OFFSET_HOTSPOT		5
#define CTF_OFFSET_MEM			5

#define SMU_13_0_DISABLE	  0
#define SMU_13_0_ENABLE		  1
#define SMU_13_0_ENABLE_BACO_ONLY 2


#define SMU_INTERNAL_HIGH_MASK          0xFFFFFFFF00000000
#define SMU_INTERNAL_LOW_MASK           0x00000000FFFFFFFF
#define SMU_INTERNAL_LOW_16_BIT_MASK    0xFFFF
#define SMU_INTERNAL_HIGH_SHIFT         32
#define SMU_INTERNAL_LOW_SHIFT          0
#define SMU_INTERNAL_REG_SIZE           4
#define SMU_INTERNAL_16_BIT             16
#define SMU_INTERNAL_24_BIT             24

#define SMU_13_KEEP_UVDS_TILE_ON (1 << 1)
#define SMU_13_VCN0 0
#define SMU_13_VCN1 (1 << 16)

// voltage type
enum navi32_voltage_type {
	VDD_GFX = 0,
	VDD_SOC,
	VDD_MEM
};

struct smu_13_0_max_sustainable_clocks {
	uint32_t display_clock;
	uint32_t phy_clock;
	uint32_t pixel_clock;
	uint32_t uclock;
	uint32_t dcef_clock;
	uint32_t soc_clock;
};

struct smu_13_0_dpm_clk_level {
	bool				enabled;
	uint32_t			value;
};

struct smu_13_0_dpm_table {
	uint32_t			min;        /* MHz */
	uint32_t			max;        /* MHz */
	uint32_t			count;
	bool				is_fine_grained;
	struct smu_13_0_dpm_clk_level	dpm_levels[MAX_DPM_LEVELS];
};

struct smu_13_0_pcie_table {
	uint8_t  pcie_gen[MAX_PCIE_CONF];
	uint8_t  pcie_lane[MAX_PCIE_CONF];
	uint16_t clk_freq[MAX_PCIE_CONF];
	uint32_t num_of_link_levels;
};

struct smu_13_0_dpm_tables {
	struct smu_13_0_dpm_table        gfx_table;
	struct smu_13_0_dpm_table        vclk_table;
	struct smu_13_0_dpm_table        dclk_table;
	struct smu_13_0_dpm_table        soc_table;
	struct smu_13_0_dpm_table        uclk_table;
	struct smu_13_0_dpm_table        dispclk_table;
	struct smu_13_0_dpm_table        dppclk_table;
	struct smu_13_0_dpm_table        dprefclk_table;
	struct smu_13_0_dpm_table        dcfclk_table;
	struct smu_13_0_dpm_table        dtbclk_table;
	struct smu_13_0_dpm_table        fclk_table;
};

struct smu_13_0_dpm_context {
	struct smu_13_0_dpm_tables  dpm_tables;
	uint32_t                    workload_policy_mask;
	uint32_t                    dcef_min_ds_clk;
};

enum smu_13_0_power_state {
	SMU_13_0_POWER_STATE__D0 = 0,
	SMU_13_0_POWER_STATE__D1,
	SMU_13_0_POWER_STATE__D3, /* Sleep*/
	SMU_13_0_POWER_STATE__D4, /* Hibernate*/
	SMU_13_0_POWER_STATE__D5, /* Power off*/
};

struct smu_13_0_power_context {
	uint32_t	power_source;
	uint8_t		in_power_limit_boost_mode;
	enum smu_13_0_power_state power_state;
};

enum smu_v13_0_baco_seq {
	BACO_SEQ_BACO = 0,
	BACO_SEQ_MSR,
	BACO_SEQ_BAMACO,
	BACO_SEQ_ULPS,
	BACO_SEQ_COUNT,
};

struct atom_smc_dpm_info_table {
	struct atom_common_table_header table_header;
	BoardTable_t BoardTable;
};

int navi32_powerplay_send_msg(struct amdgv_adapter *adapt, uint16_t msg);
int navi32_powerplay_send_msg_with_param(struct amdgv_adapter *adapt, uint16_t msg,
					       uint32_t param);
int navi32_powerplay_get_arg_with_param(struct amdgv_adapter *adapt, uint16_t msg,
					      uint32_t param, uint32_t *output);

int navi32_enter_baco(struct amdgv_adapter *adapt);
int navi32_exit_baco(struct amdgv_adapter *adapt);
int navi32_mode1_reset(struct amdgv_adapter *adapt);
int navi32_wait_mode1_reset_completion(struct amdgv_adapter *adapt);
int navi32_mode2_reset(struct amdgv_adapter *adapt);

int navi32_powerplay_get_fw_loaded_status(struct amdgv_adapter *adapt);
int navi32_powerplay_wait_idle(struct amdgv_adapter *adapt, int timeout);

int navi32_powerplay_get_clock_limit(struct amdgv_adapter *adapt, enum pp_clock_type clk,
					   enum pp_clock_limit_type limit_type,
					   uint32_t *freq);

int navi32_power_on_vcn(struct amdgv_adapter *adapt);
int navi32_power_down_vcn(struct amdgv_adapter *adapt);

int navi32_powerplay_enable_smu_features(struct amdgv_adapter *adapt);
int navi32_powerplay_notify_no_dal(struct amdgv_adapter *adapt);

int navi32_powerplay_ras_report(struct amdgv_adapter *adapt, int ras_type);

#endif

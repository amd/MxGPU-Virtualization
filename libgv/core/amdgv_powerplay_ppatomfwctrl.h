/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_POWERPLAY_PPATOMFWCTRL_H
#define AMDGV_POWERPLAY_PPATOMFWCTRL_H
#include "amdgv_device.h"

#define PP_ATOMFWCTRL_MAX_VOLTAGE_ENTRIES 32

typedef enum atom_smu9_syspll0_clock_id BIOS_CLKID;

struct pp_atomfwctrl_voltage_table_entry {
	uint16_t value;
	uint32_t smio_low;
};

struct pp_atomfwctrl_voltage_table {
	uint32_t				 count;
	uint32_t				 mask_low;
	uint32_t				 phase_delay;
	uint8_t					 psi0_enable;
	uint8_t					 psi1_enable;
	uint8_t					 max_vid_step;
	uint8_t					 telemetry_offset;
	uint8_t					 telemetry_slope;
	struct pp_atomfwctrl_voltage_table_entry entries[PP_ATOMFWCTRL_MAX_VOLTAGE_ENTRIES];
};

struct pp_atomfwctrl_bios_boot_up_values {
	uint32_t ulRevision;
	uint32_t ulGfxClk;
	uint32_t ulUClk;
	uint32_t ulSocClk;
	uint32_t ulDCEFClk;
	uint16_t usVddc;
	uint16_t usVddci;
	uint16_t usMvddc;
	uint16_t usVddGfx;
	uint8_t	 ucCoolingID;
};

#endif

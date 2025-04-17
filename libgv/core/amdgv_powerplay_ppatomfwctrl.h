/*
 * Copyright (c) 2017-2020 Advanced Micro Devices, Inc. All rights reserved.
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

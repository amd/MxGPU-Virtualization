/*
 * Copyright (c) 2019-2024 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef _AMDGV_RAS_EEPROM_H
#define _AMDGV_RAS_EEPROM_H

#include "amdgv_common_eeprom.h"

struct amdgv_ras_eeprom_funcs {
	int (*init)(struct amdgv_adapter *adapt, struct amdgv_ras_eeprom_control *control);
	void (*fini)(struct amdgv_ras_eeprom_control *control);
	int (*reset_table)(struct amdgv_adapter *adapt, struct amdgv_ras_eeprom_control *control);
	int (*process_records)(struct amdgv_adapter *adapt, struct amdgv_ras_eeprom_control *control,
			struct eeprom_table_record *records, bool write, int num);
};

struct amdgv_ras_eeprom {
	const struct amdgv_ras_eeprom_funcs *funcs;
};

int amdgv_ras_eeprom_sw_init(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control);
int amdgv_ras_eeprom_init(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control);
void amdgv_ras_eeprom_fini(struct amdgv_ras_eeprom_control *control);
int amdgv_ras_eeprom_reset_table(struct amdgv_adapter *adapt,
				 struct amdgv_ras_eeprom_control *control);

int amdgv_ras_eeprom_process_records(struct amdgv_adapter *adapt,
				     struct amdgv_ras_eeprom_control *control,
				     struct eeprom_table_record *records, bool write, int num);

bool amdgv_ras_eeprom_is_gpu_bad(struct amdgv_adapter *adapt);
uint64_t amdgv_utc_to_eeprom_format(struct amdgv_adapter *adapt, uint64_t utc_timestamp);

#endif // _AMDGV_RAS_EEPROM_H

/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef _AMDGV_RAS_EEPROM_LEGACY_H
#define _AMDGV_RAS_EEPROM_LEGACY_H

#include "amdgv_device.h"
#include "amdgv_common_eeprom.h"


/* RAS EEPROM V2_1 (and only V2_1!) supports upgrading from legacy format.
 * Expose this function to process the records using legacy method. */
int  ras_eeprom_legacy_process_records(struct amdgv_adapter *adapt,
			struct amdgv_ras_eeprom_control *control,
			struct eeprom_table_record *records,
			bool write,
			int num);

bool ras_eeprom_legacy_validate_tbl_checksum(struct amdgv_adapter *adapt,
			struct amdgv_ras_eeprom_control *control);

#endif // _AMDGV_RAS_EEPROM_LEGACY_H

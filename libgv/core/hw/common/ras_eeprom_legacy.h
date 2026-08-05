/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

#endif // _AMDGV_RAS_EEPROM_LEGACY_H

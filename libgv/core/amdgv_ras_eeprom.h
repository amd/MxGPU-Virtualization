/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _AMDGV_RAS_EEPROM_H
#define _AMDGV_RAS_EEPROM_H

#include "amdgv_common_eeprom.h"

struct utc_datetime {
	uint64_t year;
	uint64_t month;
	uint64_t day;
	uint64_t hour;
	uint64_t minute;
	uint64_t second;
};

struct amdgv_ras_eeprom_funcs {
	int (*init)(struct amdgv_adapter *adapt, struct amdgv_ras_eeprom_control *control);
	void (*fini)(struct amdgv_ras_eeprom_control *control);
	int (*reset_table)(struct amdgv_adapter *adapt, struct amdgv_ras_eeprom_control *control);
	int (*process_records)(struct amdgv_adapter *adapt, struct amdgv_ras_eeprom_control *control,
			struct eeprom_table_record *records, bool write, int num);
	uint64_t (*utc_to_eeprom_format)(struct amdgv_adapter *adapt, uint64_t utc_timestamp);
	bool (*is_gpu_bad)(struct amdgv_adapter *adapt);
};

struct amdgv_ras_eeprom {
	const struct amdgv_ras_eeprom_funcs *funcs;
};

int amdgv_ras_eeprom_version_init(struct amdgv_adapter *adapt);
int amdgv_ras_eeprom_init(struct amdgv_adapter *adapt,
			  struct amdgv_ras_eeprom_control *control);
void amdgv_ras_eeprom_fini(struct amdgv_ras_eeprom_control *control);
int amdgv_ras_eeprom_reset_table(struct amdgv_adapter *adapt,
				 struct amdgv_ras_eeprom_control *control);

int amdgv_ras_eeprom_process_records(struct amdgv_adapter *adapt,
				     struct amdgv_ras_eeprom_control *control,
				     struct eeprom_table_record *records, bool write, int num);
bool amdgv_ras_eeprom_is_header_bad(struct amdgv_adapter *adapt);
bool amdgv_ras_eeprom_is_gpu_bad(struct amdgv_adapter *adapt);
void amdgv_utc_to_datetime_components(struct amdgv_adapter *adapt, uint64_t utc_timestamp, struct utc_datetime *dt);
uint64_t amdgv_utc_to_eeprom_format(struct amdgv_adapter *adapt, uint64_t utc_timestamp);
uint64_t amdgv_ras_eeprom_utc_to_eeprom_format(struct amdgv_adapter *adapt,
					uint64_t utc_timestamp);
int amdgv_ras_eeprom_export_live_update(struct amdgv_adapter *adapt, uint8_t *data);

#endif // _AMDGV_RAS_EEPROM_H

/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_MARKETING_NAME_H
#define AMDGV_MARKETING_NAME_H

#include "amdgv_device.h"

struct amdgv_marketing_name_entry {
	uint32_t asic_type;
	uint32_t dev_id;
	uint32_t rev_id;
	const char *marketing_name;
};

const char *amdgv_get_marketing_name(uint32_t dev_id, uint32_t rev_id);

#endif
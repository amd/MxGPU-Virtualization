/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv.h>

#include "amdgv.h"
#include "amdgv_vfmgr.h"
#include "mi200_ip_discovery.h"

static int mi200_copy_ip_data_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return 0;
}

int mi200_ip_discovery_init(struct amdgv_adapter *adapt)
{
	adapt->ip_discovery.copy_to_vf = mi200_copy_ip_data_to_vf;

	return 0;
}

void mi200_ip_discovery_fini(struct amdgv_adapter *adapt)
{
	adapt->ip_discovery.copy_to_vf = NULL;
}

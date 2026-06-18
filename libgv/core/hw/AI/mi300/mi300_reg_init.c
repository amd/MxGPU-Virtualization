/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <ai.h>
#include "mi300.h"
#include "mi350/mi350_vcn.h"

void mi300_reg_base_init(struct amdgv_adapter *adapt)
{
	/* Removed hard-codeing of IP Base Addresses (which are now obtained via IP
	 * Discovery instead) */
}

static int mi300_doorbell_index_init(struct amdgv_adapter *adapt)
{
	uint32_t sdma_id;
	adapt->doorbell_index.kiq = 0;
	adapt->doorbell_index.mec_ring0 = 8;
	adapt->doorbell_index.userqueue_start = 16;
	adapt->doorbell_index.userqueue_end = 31;
	adapt->doorbell_index.xcc_doorbell_range = 32;
	adapt->doorbell_index.first_non_cp = 256;
	adapt->doorbell_index.last_non_cp = 488;
	adapt->doorbell_index.max_assignment = 488 << 1;
	adapt->doorbell_index.sdma_doorbell_range = 20;

	for (sdma_id = 0; sdma_id < 16; sdma_id++)
		adapt->doorbell_index.sdma_engine[sdma_id] =
				AMDGV_MI300_DOORBELL_sDMA_ENGINE0 + sdma_id * 10;

	adapt->doorbell_index.ih = AMDGV_MI300_DOORBELL_IH;

	return 0;
}

/* do nothing in these interfaces */
static int mi300_doorbell_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_doorbell_hw_init(struct amdgv_adapter *adapt)
{
	if (adapt->asic_type == CHIP_MI350X)
		mi350_vcn_set_mmsch_doorbell_addr_base(adapt);
	return 0;
}

static int mi300_doorbell_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi300_doorbell_func = {
	.name = "mi300_doorbell_func",
	.sw_init = mi300_doorbell_index_init,
	.sw_fini = mi300_doorbell_sw_fini,
	.hw_init = mi300_doorbell_hw_init,
	.hw_fini = mi300_doorbell_hw_fini,
};

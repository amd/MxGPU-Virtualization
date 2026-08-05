/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv.h>

#include "gfx_v12_1.h"
#include "asic_reg/GC/gc_12_1_0_offset.h"
#include "asic_reg/GC/gc_12_1_0_sh_mask.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static int gfx_v12_1_misc_hw_init(struct amdgv_adapter *adapt)
{
	if (adapt->flags & AMDGV_FLAG_RAS_POISON_INTERRUPT_TO_PF) {
		if (adapt->gfx.funcs && adapt->gfx.funcs->set_poison_interrupt_routing)
			adapt->gfx.funcs->set_poison_interrupt_routing(adapt, true);
	}

	return 0;
}

struct amdgv_init_func gfx_v12_1_misc_func = {
	.name = "gfx_v12_1_misc_func",
	.hw_init = gfx_v12_1_misc_hw_init,
};

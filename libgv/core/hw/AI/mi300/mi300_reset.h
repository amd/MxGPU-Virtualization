/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI300_RESET_H
#define MI300_RESET_H

struct engine_reg_info {
	uint32_t hwip;
	uint32_t offset;
	uint32_t seg;
	uint32_t reg_shift;
	uint32_t reg_mask;
	uint32_t clean_cond;
};

enum engine_reg {
	RLC_STAT,
	SDMA_STATUS_REG,
	SDMA_STATUS4_REG,
	GRBM_STATUS,
	NUM_ENGINES
};

void mi300_clear_dummy_mode_after_reset(struct amdgv_adapter *adapt);

#endif

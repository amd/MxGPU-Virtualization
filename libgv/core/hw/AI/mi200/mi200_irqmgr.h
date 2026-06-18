/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI200_IRQMGR_H
#define MI200_IRQMGR_H

#include <amdgv_device.h>

struct mi200_irqmgr_reset_state {
	uint32_t interrupt_cntl;
	uint32_t interrupt_cntl2;
	uint32_t ih_cntl;
	uint32_t ih_doorbell_range;
	uint32_t ih_rb_cntl;
	uint32_t doorbell_rptr;
	uint32_t doorbell_aper;
	uint32_t hdp_host_path_cntl;
	uint32_t rb_base_addr;
	uint32_t rb_base_addr_hi;
	uint32_t wptr_off_lo;
	uint32_t wptr_off_hi;
	uint32_t rptr;
	uint32_t wptr;
};

int mi200_irqmgr_restore_and_init(struct amdgv_adapter *adapt,
				   struct mi200_irqmgr_reset_state *reset_state);
void mi200_irqmgr_save_and_fini(struct amdgv_adapter *adapt,
				 struct mi200_irqmgr_reset_state *reset_state);
void mi200_irqmgr_golden_init(struct amdgv_adapter *adapt);

#endif

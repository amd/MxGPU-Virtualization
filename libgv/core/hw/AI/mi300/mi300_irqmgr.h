/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI300_IRQMGR_H
#define MI300_IRQMGR_H

#include <amdgv_device.h>

#define MI300_IH_IV_CLIENTID_SDMA0		0x08
#define MI300_IH_IV_CLIENTID_SDMA1      0x09
#define MI300_IH_IV_CLIENTID_SDMA2      0x01
#define MI300_IH_IV_CLIENTID_SDMA3      0x04

#define MI300_IH_IV_SRCID_SDMA_TRAP 	0xE0
#define MI300_IH_IV_SRCID_SDMA_CTXEMPTY	0xF3

#define MI300_IH_IV_CLIENTID_PWR		0x19
#define MI300_IH_IV_SRCID_DISP_TIMER 	0xFD
#define MI300_TIMER_MICROSEC_DELAY		10

#define MI300_IH_IV_CLIENTID_GRBM_CP	0x14
#define MI300_IH_IV_SRCID_CP_EOP_INTERRUPT 	0xB5

struct mi300_irqmgr_reset_state {
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

int mi300_irqmgr_restore_and_init(struct amdgv_adapter *adapt,
				  struct mi300_irqmgr_reset_state *reset_state);
void mi300_irqmgr_save_and_fini(struct amdgv_adapter *adapt,
				struct mi300_irqmgr_reset_state *reset_state);
void mi300_irqmgr_golden_init(struct amdgv_adapter *adapt);

#endif

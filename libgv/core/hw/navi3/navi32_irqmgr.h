/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI3_IRQMGR_H
#define NAVI3_IRQMGR_H

#include <amdgv_device.h>

#define NAVI3_IH_IV_CLIENTID_GFX		0x0A
#define NAVI3_IH_IV_SRCID_SDMA_TRAP 	0x31
#define NAVI3_IH_IV_SRCID_SDMA_CTXEMPTY	0x33

#define NAVI3_IH_IV_CLIENTID_PWR		0x19
#define NAVI3_IH_IV_SRCID_DISP_TIMER 	0xFD
#define NAVI3_TIMER_MICROSEC_DELAY		10

#define NAVI3_IH_IV_CLIENTID_GRBM_CP	0x14
#define NAVI3_IH_IV_SRCID_CP_EOP_INTERRUPT 	0xB5

struct navi32_irqmgr_reset_state {
	uint32_t interrupt_cntl;
	uint32_t interrupt_cntl2;
	uint32_t ih_cntl;
	uint32_t ih_rb_cntl;
	uint32_t ih_doorbell_rptr;
	uint32_t ih_doorbell_range;
	uint32_t doorbell_aper[AMDGV_MAX_VF_SLOT];
	uint32_t hdp_host_path_cntl;
	uint32_t rb_base_addr_lo;
	uint32_t rb_base_addr_hi;
	uint32_t wptr_off_lo;
	uint32_t wptr_off_hi;
	uint32_t rptr;
	uint32_t wptr;
};

#endif

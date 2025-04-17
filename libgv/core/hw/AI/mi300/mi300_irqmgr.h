/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

#ifndef MI300_IRQMGR_H
#define MI300_IRQMGR_H

#include <amdgv_device.h>

#define MI300_IH_IV_CLIENTID_SDMA0		0x08
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

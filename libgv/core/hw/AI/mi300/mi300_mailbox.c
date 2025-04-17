/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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

#include "mi300.h"
#include "mi300/NBIO/nbio_7_9_0_offset.h"
#include "mi300/NBIO/nbio_7_9_0_sh_mask.h"
#include <amdgv_device.h>

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

#define MI300_MAILBOX_DATA_LEN 4

static int mi300_mailbox_get_index(struct amdgv_adapter *adapt, uint32_t *idx_vf)
{
	*idx_vf = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_MAILBOX_INDEX));

	return 0;
}

static int mi300_mailbox_update_index(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t tmp;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_MAILBOX_INDEX));
	tmp = REG_SET_FIELD(tmp, BIF_BX0_MAILBOX_INDEX, MAILBOX_INDEX, idx_vf);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_MAILBOX_INDEX), tmp);

	return 0;
}

static int mi300_mailbox_rcv_msg(struct amdgv_adapter *adapt, uint32_t idx_vf, int offset,
				 uint32_t *msg_data)
{
	if (offset >= MI300_MAILBOX_DATA_LEN) {
		*msg_data = 0xfffffff;
		return AMDGV_FAILURE;
	}

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	*msg_data = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_RCV_DW0) +
			   offset);

	return 0;
}

static int mi300_mailbox_trn_msg(struct amdgv_adapter *adapt, uint32_t idx_vf, int offset,
				 uint32_t msg_data)
{
	if (offset >= MI300_MAILBOX_DATA_LEN)
		return AMDGV_FAILURE;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW0) + offset,
	       msg_data);

	return 0;
}

static int mi300_mailbox_trn_msg_valid(struct amdgv_adapter *adapt, uint32_t idx_vf,
				       bool valid)
{
	uint32_t offset;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	/* set MAILBOX_CONTROL.TRN_MSG_VALID = valid */
	offset = SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_CONTROL) * 4;
	oss_mm_write8((uint8_t *)(adapt->mmio) + offset, (valid ? 1 : 0));

	return 0;
}

static int mi300_mailbox_ack_msg(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t offset;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	/* set MAILBOX_CONTROL.RCV_MSG_ACK = 1 */
	offset = SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_CONTROL) * 4 + 1;
	oss_mm_write8((uint8_t *)(adapt->mmio) + offset, 2);

	return 0;
}

static int mi300_mailbox_rcv_peek_ack(struct amdgv_adapter *adapt)
{
	uint32_t offset;

	offset = SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_CONTROL) * 4;

	/* read TRN_MSG_ACK bit */
	return REG_GET_FIELD(oss_mm_read8((uint8_t *)(adapt->mmio) + offset),
			 BIF_BX_PF0_MAILBOX_CONTROL, TRN_MSG_ACK);
}

static int mi300_mailbox_reset(struct amdgv_adapter *adapt)
{
	uint32_t i, idx_vf;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		mi300_mailbox_update_index(adapt, idx_vf);

		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_CONTROL), 0);

		for (i = 0; i < MI300_MAILBOX_DATA_LEN; i++) {
			mi300_mailbox_trn_msg(adapt, idx_vf, i, 0);
		}
	}

	return 0;
}

static int mi300_mailbox_save_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t tmp;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_CONTROL));

	adapt->mailbox.state_vf[idx_vf].rcv_msg_acked =
		REG_GET_FIELD(tmp, BIF_BX_PF0_MAILBOX_CONTROL, RCV_MSG_ACK) ? true : false;

	adapt->mailbox.state_vf[idx_vf].saved_ack_count =
		adapt->mailbox.state_vf[idx_vf].rcv_ack_count;

	AMDGV_DEBUG("ack state of %s is %d\n", amdgv_idx_to_str(idx_vf),
		    adapt->mailbox.state_vf[idx_vf].rcv_msg_acked);

	adapt->mailbox.state_vf[idx_vf].msg_trn_dw0 =
		RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW0));

	adapt->mailbox.state_vf[idx_vf].msg_trn_dw1 =
		RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW1));

	adapt->mailbox.state_vf[idx_vf].msg_trn_dw2 =
		RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW2));

	adapt->mailbox.state_vf[idx_vf].msg_trn_dw3 =
		RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW3));

	AMDGV_DEBUG("saved msg_trn_dw of VF%d is %d, %d, %d, %d.\n", idx_vf,
		    adapt->mailbox.state_vf[idx_vf].msg_trn_dw0,
		    adapt->mailbox.state_vf[idx_vf].msg_trn_dw1,
		    adapt->mailbox.state_vf[idx_vf].msg_trn_dw2,
		    adapt->mailbox.state_vf[idx_vf].msg_trn_dw3);

	return 0;
}

static int mi300_mailbox_restore_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t offset;
	bool ack_count_changed = false;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	if (adapt->mailbox.state_vf[idx_vf].saved_ack_count !=
	    adapt->mailbox.state_vf[idx_vf].rcv_ack_count) {
		ack_count_changed = true;
		AMDGV_DEBUG("ack count changed\n");
	}

	if (ack_count_changed || adapt->mailbox.state_vf[idx_vf].rcv_msg_acked) {
		/* set MAILBOX_CONTROL.RCV_MSG_ACK = 1 */
		offset = SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_CONTROL) * 4 + 1;
		oss_mm_write8((uint8_t *)(adapt->mmio) + offset, 2);

		AMDGV_DEBUG("restored %s to acked state\n", amdgv_idx_to_str(idx_vf));
	}

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW0),
	       adapt->mailbox.state_vf[idx_vf].msg_trn_dw0);

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW1),
	       adapt->mailbox.state_vf[idx_vf].msg_trn_dw1);

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW2),
	       adapt->mailbox.state_vf[idx_vf].msg_trn_dw2);

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW3),
	       adapt->mailbox.state_vf[idx_vf].msg_trn_dw3);

	AMDGV_DEBUG("restored msg_trn_dw of VF%d is %d, %d, %d, %d. \n", idx_vf,
		    RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW0)),
		    RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW1)),
		    RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW2)),
		    RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW3)));

	return 0;
}

static const struct amdgv_mailbox_funcs mi300_mailbox_funcs = {
	.get_index = mi300_mailbox_get_index,
	.update_index = mi300_mailbox_update_index,
	.rcv_msg = mi300_mailbox_rcv_msg,
	.trn_msg = mi300_mailbox_trn_msg,
	.trn_msg_valid = mi300_mailbox_trn_msg_valid,
	.ack_msg = mi300_mailbox_ack_msg,
	.reset = mi300_mailbox_reset,
	.save_state = mi300_mailbox_save_state,
	.restore_state = mi300_mailbox_restore_state,
	.peek_ack = mi300_mailbox_rcv_peek_ack,
};

static int mi300_mailbox_sw_init(struct amdgv_adapter *adapt)
{
	adapt->mailbox.msg_buf_len = MI300_MAILBOX_DATA_LEN;

	return amdgv_mailbox_init(adapt, &mi300_mailbox_funcs);
}

static int mi300_mailbox_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_mailbox_fini(adapt);
	return 0;
}

static int mi300_mailbox_hw_init(struct amdgv_adapter *adapt)
{
	if (!adapt->opt.skip_hw_init)
		mi300_mailbox_reset(adapt);

	return 0;
}

static int mi300_mailbox_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mi300_mailbox_func = {
	.name = "mi300_mailbox_func",
	.sw_init = mi300_mailbox_sw_init,
	.sw_fini = mi300_mailbox_sw_fini,
	.hw_init = mi300_mailbox_hw_init,
	.hw_fini = mi300_mailbox_hw_fini,
};

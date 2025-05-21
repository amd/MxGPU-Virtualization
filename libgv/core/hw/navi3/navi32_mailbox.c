/*
 * Copyright (C) 2021  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <amdgv_device.h>

#include "navi32_reg_inc.h"
#include "navi32_nbio_mapper.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

#define NAVI10_MAILBOX_DATA_LEN 4

static int navi32_mailbox_get_index(struct amdgv_adapter *adapt, uint32_t *idx_vf)
{
	*idx_vf = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_MAILBOX_INDEX));

	return 0;
}

static int navi32_mailbox_update_index(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t tmp;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_MAILBOX_INDEX));
	tmp = REG_SET_FIELD(tmp, MAILBOX_INDEX, MAILBOX_INDEX, idx_vf);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_MAILBOX_INDEX), tmp);

	return 0;
}

static int navi32_mailbox_rcv_msg(struct amdgv_adapter *adapt, uint32_t idx_vf, int offset,
				 uint32_t *msg_data)
{
	if (offset >= NAVI10_MAILBOX_DATA_LEN) {
		*msg_data = 0xfffffff;
		return AMDGV_FAILURE;
	}

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		*msg_data = 0xfffffff;
		return AMDGV_FAILURE;
	}

	*msg_data = RREG32(
		SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, BIF_BX, MAILBOX_MSGBUF_RCV_DW0) +
		offset);

	if (offset == 0) {
		AMDGV_DEBUG("received MAILBOX_MSGBUF(%s) from %s\n",
			    amdgv_mailbox_rcv_idh_to_name(*msg_data),
			    amdgv_idx_to_str(idx_vf));
	}

	return 0;
}

static int navi32_mailbox_trn_msg(struct amdgv_adapter *adapt, uint32_t idx_vf, int offset,
				 uint32_t msg_data)
{
	if (offset >= NAVI10_MAILBOX_DATA_LEN)
		return AMDGV_FAILURE;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	WREG32(SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, BIF_BX, MAILBOX_MSGBUF_TRN_DW0) +
		       offset,
	       msg_data);

	if (offset == 0) {
		AMDGV_DEBUG("sent MAILBOX_MSGBUF(%s) to %s\n",
			    amdgv_mailbox_trn_idh_to_name(msg_data), amdgv_idx_to_str(idx_vf));
	}

	return 0;
}

static int navi32_mailbox_trn_msg_valid(struct amdgv_adapter *adapt, uint32_t idx_vf,
				       bool valid)
{
	uint32_t offset;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	/* set MAILBOX_CONTROL.TRN_MSG_VALID = valid */
	offset = SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, BIF_BX, MAILBOX_CONTROL) * 4;
	oss_mm_write8((uint8_t *)(adapt->mmio) + offset, (valid ? 1 : 0));
	if (valid) {
		AMDGV_DEBUG("sent MAILBOX_CONTROL(TRN_MSG_VALID) to %s\n",
			    amdgv_idx_to_str(idx_vf));
	}

	return 0;
}

static int navi32_mailbox_ack_msg(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t offset;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	/* set MAILBOX_CONTROL.RCV_MSG_ACK = 1 */
	offset = SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, BIF_BX, MAILBOX_CONTROL) * 4 + 1;
	oss_mm_write8((uint8_t *)(adapt->mmio) + offset, 2);

	AMDGV_DEBUG("sent MAILBOX_CONTROL(RCV_MSG_ACK) to %s\n", amdgv_idx_to_str(idx_vf));

	return 0;
}

static int navi32_mailbox_reset(struct amdgv_adapter *adapt)
{
	uint32_t i, idx_vf;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		navi32_mailbox_update_index(adapt, idx_vf);

		WREG32(SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, BIF_BX, MAILBOX_CONTROL),
		       0);

		for (i = 0; i < NAVI10_MAILBOX_DATA_LEN; i++)
			navi32_mailbox_trn_msg(adapt, idx_vf, i, 0);
	}

	return 0;
}

static int navi32_mailbox_save_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t tmp;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	tmp = RREG32(SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, BIF_BX, MAILBOX_CONTROL));

	adapt->mailbox.state_vf[idx_vf].rcv_msg_acked =
		REG_GET_FIELD(tmp, BIF_BX_PF_MAILBOX_CONTROL, RCV_MSG_ACK) ? true : false;

	adapt->mailbox.state_vf[idx_vf].saved_ack_count =
		adapt->mailbox.state_vf[idx_vf].rcv_ack_count;

	AMDGV_DEBUG("ack state of %s is %d\n", amdgv_idx_to_str(idx_vf),
		    adapt->mailbox.state_vf[idx_vf].rcv_msg_acked);

	return 0;
}

static int navi32_mailbox_restore_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t offset;
	bool ack_count_changed = false;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;

	if (adapt->mailbox.state_vf[idx_vf].saved_ack_count !=
	    adapt->mailbox.state_vf[idx_vf].rcv_ack_count) {
		ack_count_changed = true;
		AMDGV_DEBUG("ack count changed for %s\n", amdgv_idx_to_str(idx_vf));
	}

	if (ack_count_changed || adapt->mailbox.state_vf[idx_vf].rcv_msg_acked) {
		/* set MAILBOX_CONTROL.RCV_MSG_ACK = 1 */
		offset =
			SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, BIF_BX, MAILBOX_CONTROL) * 4 + 1;
		oss_mm_write8((uint8_t *)(adapt->mmio) + offset, 2);

		AMDGV_DEBUG("restored %s to acked state\n", amdgv_idx_to_str(idx_vf));
	}

	return 0;
}

static const struct amdgv_mailbox_funcs navi32_mailbox_funcs = {
	.get_index = navi32_mailbox_get_index,
	.update_index = navi32_mailbox_update_index,
	.rcv_msg = navi32_mailbox_rcv_msg,
	.trn_msg = navi32_mailbox_trn_msg,
	.trn_msg_valid = navi32_mailbox_trn_msg_valid,
	.ack_msg = navi32_mailbox_ack_msg,
	.reset = navi32_mailbox_reset,
	.save_state = navi32_mailbox_save_state,
	.restore_state = navi32_mailbox_restore_state,
};

static int navi32_mailbox_sw_init(struct amdgv_adapter *adapt)
{
	adapt->mailbox.msg_buf_len = NAVI10_MAILBOX_DATA_LEN;

	return amdgv_mailbox_init(adapt, &navi32_mailbox_funcs);
}

static int navi32_mailbox_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_mailbox_fini(adapt);
	return 0;
}

static int navi32_mailbox_hw_init(struct amdgv_adapter *adapt)
{
	navi32_mailbox_reset(adapt);

	return 0;
}

static int navi32_mailbox_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func navi32_mailbox_func = {
	.name = "navi32_mailbox_func",
	.sw_init = navi32_mailbox_sw_init,
	.sw_fini = navi32_mailbox_sw_fini,
	.hw_init = navi32_mailbox_hw_init,
	.hw_fini = navi32_mailbox_hw_fini,
};

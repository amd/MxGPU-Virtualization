/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI3_NBIO_MAPPER_H
#define NAVI3_NBIO_MAPPER_H

/*
 * To address NBIO inconsistent register naming
 * Remove them when they are defined/fixed in asic_reg files
 */
#define regBIF_BX_DEV0_EPF0_BIF_BME_STATUS                                (regBIF_BX_PF0_BIF_BME_STATUS)
#define regBIF_BX_DEV0_EPF0_BIF_BME_STATUS_BASE_IDX                       (regBIF_BX_PF0_BIF_BME_STATUS_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_BIF_ATOMIC_ERR_LOG                            (regBIF_BX_PF0_BIF_ATOMIC_ERR_LOG)
#define regBIF_BX_DEV0_EPF0_BIF_ATOMIC_ERR_LOG_BASE_IDX                   (regBIF_BX_PF0_BIF_ATOMIC_ERR_LOG_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_DOORBELL_SELFRING_GPA_APER_BASE_HIGH          (regBIF_BX_PF0_DOORBELL_SELFRING_GPA_APER_BASE_HIGH)
#define regBIF_BX_DEV0_EPF0_DOORBELL_SELFRING_GPA_APER_BASE_HIGH_BASE_IDX (regBIF_BX_PF0_DOORBELL_SELFRING_GPA_APER_BASE_HIGH_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_DOORBELL_SELFRING_GPA_APER_BASE_LOW           (regBIF_BX_PF0_DOORBELL_SELFRING_GPA_APER_BASE_LOW)
#define regBIF_BX_DEV0_EPF0_DOORBELL_SELFRING_GPA_APER_BASE_LOW_BASE_IDX  (regBIF_BX_PF0_DOORBELL_SELFRING_GPA_APER_BASE_LOW_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_DOORBELL_SELFRING_GPA_APER_CNTL               (regBIF_BX_PF0_DOORBELL_SELFRING_GPA_APER_CNTL)
#define regBIF_BX_DEV0_EPF0_DOORBELL_SELFRING_GPA_APER_CNTL_BASE_IDX      (regBIF_BX_PF0_DOORBELL_SELFRING_GPA_APER_CNTL_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_HDP_REG_COHERENCY_FLUSH_CNTL                  (regBIF_BX_PF0_HDP_REG_COHERENCY_FLUSH_CNTL)
#define regBIF_BX_DEV0_EPF0_HDP_REG_COHERENCY_FLUSH_CNTL_BASE_IDX         (regBIF_BX_PF0_HDP_REG_COHERENCY_FLUSH_CNTL_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_HDP_MEM_COHERENCY_FLUSH_CNTL                  (regBIF_BX_PF0_HDP_MEM_COHERENCY_FLUSH_CNTL)
#define regBIF_BX_DEV0_EPF0_HDP_MEM_COHERENCY_FLUSH_CNTL_BASE_IDX         (regBIF_BX_PF0_HDP_MEM_COHERENCY_FLUSH_CNTL_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_GPU_HDP_FLUSH_REQ                             (regBIF_BX_PF0_GPU_HDP_FLUSH_REQ)
#define regBIF_BX_DEV0_EPF0_GPU_HDP_FLUSH_REQ_BASE_IDX                    (regBIF_BX_PF0_GPU_HDP_FLUSH_REQ_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_GPU_HDP_FLUSH_DONE                            (regBIF_BX_PF0_GPU_HDP_FLUSH_DONE)
#define regBIF_BX_DEV0_EPF0_GPU_HDP_FLUSH_DONE_BASE_IDX                   (regBIF_BX_PF0_GPU_HDP_FLUSH_DONE_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_BIF_TRANS_PENDING                             (regBIF_BX_PF0_BIF_TRANS_PENDING)
#define regBIF_BX_DEV0_EPF0_BIF_TRANS_PENDING_BASE_IDX                    (regBIF_BX_PF0_BIF_TRANS_PENDING_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_NBIF_GFX_ADDR_LUT_BYPASS                      (regBIF_BX_PF0_NBIF_GFX_ADDR_LUT_BYPASS)
#define regBIF_BX_DEV0_EPF0_NBIF_GFX_ADDR_LUT_BYPASS_BASE_IDX             (regBIF_BX_PF0_NBIF_GFX_ADDR_LUT_BYPASS_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_TRN_DW0                        (regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW0)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_TRN_DW0_BASE_IDX               (regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW0_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_TRN_DW1                        (regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW1)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_TRN_DW1_BASE_IDX               (regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW1_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_TRN_DW2                        (regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW2)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_TRN_DW2_BASE_IDX               (regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW2_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_TRN_DW3                        (regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW3)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_TRN_DW3_BASE_IDX               (regBIF_BX_PF0_MAILBOX_MSGBUF_TRN_DW3_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_RCV_DW0                        (regBIF_BX_PF0_MAILBOX_MSGBUF_RCV_DW0)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_RCV_DW0_BASE_IDX               (regBIF_BX_PF0_MAILBOX_MSGBUF_RCV_DW0_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_RCV_DW1                        (regBIF_BX_PF0_MAILBOX_MSGBUF_RCV_DW1)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_RCV_DW1_BASE_IDX               (regBIF_BX_PF0_MAILBOX_MSGBUF_RCV_DW1_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_RCV_DW2                        (regBIF_BX_PF0_MAILBOX_MSGBUF_RCV_DW2)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_RCV_DW2_BASE_IDX               (regBIF_BX_PF0_MAILBOX_MSGBUF_RCV_DW2_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_RCV_DW3                        (regBIF_BX_PF0_MAILBOX_MSGBUF_RCV_DW3)
#define regBIF_BX_DEV0_EPF0_MAILBOX_MSGBUF_RCV_DW3_BASE_IDX               (regBIF_BX_PF0_MAILBOX_MSGBUF_RCV_DW3_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_MAILBOX_CONTROL                               (regBIF_BX_PF0_MAILBOX_CONTROL)
#define regBIF_BX_DEV0_EPF0_MAILBOX_CONTROL_BASE_IDX                      (regBIF_BX_PF0_MAILBOX_CONTROL_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_MAILBOX_INT_CNTL                              (regBIF_BX_PF0_MAILBOX_INT_CNTL)
#define regBIF_BX_DEV0_EPF0_MAILBOX_INT_CNTL_BASE_IDX                     (regBIF_BX_PF0_MAILBOX_INT_CNTL_BASE_IDX)
#define regBIF_BX_DEV0_EPF0_BIF_VMHV_MAILBOX                              (regBIF_BX_PF0_BIF_VMHV_MAILBOX)
#define regBIF_BX_DEV0_EPF0_BIF_VMHV_MAILBOX_BASE_IDX                     (regBIF_BX_PF0_BIF_VMHV_MAILBOX_BASE_IDX)

#define SOC15_REG_OFFSET_NBIO_BLOCK(ip, inst, idx_vf, blk, reg_name) \
	(idx_vf == 0 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF0_##reg_name) : \
	(idx_vf == 1 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF1_##reg_name) : \
	(idx_vf == 2 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF2_##reg_name) : \
	(idx_vf == 3 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF3_##reg_name) : \
	(idx_vf == 4 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF4_##reg_name) : \
	(idx_vf == 5 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF5_##reg_name) : \
	(idx_vf == 6 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF6_##reg_name) : \
	(idx_vf == 7 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF7_##reg_name) : \
	(idx_vf == 8 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF8_##reg_name) : \
	(idx_vf == 9 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF9_##reg_name) : \
	(idx_vf == 10 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF10_##reg_name) : \
	(idx_vf == 11 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF11_##reg_name) : \
	(idx_vf == 12 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF12_##reg_name) : \
	(idx_vf == 13 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF13_##reg_name) : \
	(idx_vf == 14 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF14_##reg_name) : \
	(idx_vf == 15 ? SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_VF15_##reg_name) : \
	(SOC15_REG_OFFSET(ip, inst, reg##blk##_DEV0_EPF0_##reg_name))))))))))))))))))

#define REG_SET_FIELD_NBIO_BLOCK(orig_val, idx_vf, blk, reg, field, field_val) ( \
	(idx_vf == 0 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF0_##reg, field, field_val) : \
	(idx_vf == 1 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF1_##reg, field, field_val) : \
	(idx_vf == 2 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF2_##reg, field, field_val) : \
	(idx_vf == 3 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF3_##reg, field, field_val) : \
	(idx_vf == 4 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF4_##reg, field, field_val) : \
	(idx_vf == 5 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF5_##reg, field, field_val) : \
	(idx_vf == 6 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF6_##reg, field, field_val) : \
	(idx_vf == 7 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF7_##reg, field, field_val) : \
	(idx_vf == 8 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF8_##reg, field, field_val) : \
	(idx_vf == 9 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF9_##reg, field, field_val) : \
	(idx_vf == 10 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF10_##reg, field, field_val) : \
	(idx_vf == 11 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF11_##reg, field, field_val) : \
	(idx_vf == 12 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF12_##reg, field, field_val) : \
	(idx_vf == 13 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF13_##reg, field, field_val) : \
	(idx_vf == 14 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF14_##reg, field, field_val) : \
	(idx_vf == 15 ? REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_VF15_##reg, field, field_val) : \
	(REG_SET_FIELD(orig_val, blk##_DEV0_EPF0_##reg, field, field_val)))))))))))))))))))

#endif

/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

 #ifndef GFX_V12_1_DOORBELL_H
 #define GFX_V12_1_DOORBELL_H

 typedef enum _AMDGV_GFX12_1_DOORBELL_ASSIGNMENT {
	/* Compute + GFX: 0~255 */
	AMDGV_GFX12_1_DOORBELL_KIQ                     = 0x000,
    AMDGV_GFX12_1_DOORBELL_HIQ                     = 0x001,
    AMDGV_GFX12_1_DOORBELL_DIQ                     = 0x002,
    AMDGV_GFX12_1_DOORBELL_MEC_RING0               = 0x003,
    AMDGV_GFX12_1_DOORBELL_MEC_RING1               = 0x004,
    AMDGV_GFX12_1_DOORBELL_MEC_RING2               = 0x005,
    AMDGV_GFX12_1_DOORBELL_MEC_RING3               = 0x006,
    AMDGV_GFX12_1_DOORBELL_MEC_RING4               = 0x007,
    AMDGV_GFX12_1_DOORBELL_MEC_RING5               = 0x008,
    AMDGV_GFX12_1_DOORBELL_MEC_RING6               = 0x009,
    AMDGV_GFX12_1_DOORBELL_MEC_RING7               = 0x00A,
    AMDGV_GFX12_1_DOORBELL_MES_RING0               = 0x00B,
    AMDGV_GFX12_1_DOORBELL_MES_RING1               = 0x00C,

    AMDGV_GFX12_1_DOORBELL_USERQUEUE_START         = 0x00D,
    AMDGV_GFX12_1_DOORBELL_USERQUEUE_END           = 0x01F,

    // Reserved
    AMDGV_GFX12_1_DOORBELL_INVALID                   = 0xFFFF
} AMDGV_GFX12_1_DOORBELL_ASSIGNMENT;

static int gfx_v12_1_doorbell_index_init(struct amdgv_adapter *adapt)
{
	adapt->doorbell_index.kiq = AMDGV_GFX12_1_DOORBELL_KIQ;
	adapt->doorbell_index.mec_ring0 = AMDGV_GFX12_1_DOORBELL_MEC_RING0;
	adapt->doorbell_index.mec_ring1 = AMDGV_GFX12_1_DOORBELL_MEC_RING1;
    adapt->doorbell_index.mec_ring2 = AMDGV_GFX12_1_DOORBELL_MEC_RING2;
    adapt->doorbell_index.mec_ring3 = AMDGV_GFX12_1_DOORBELL_MEC_RING3;
    adapt->doorbell_index.mec_ring4 = AMDGV_GFX12_1_DOORBELL_MEC_RING4;
    adapt->doorbell_index.mec_ring5 = AMDGV_GFX12_1_DOORBELL_MEC_RING5;
    adapt->doorbell_index.mec_ring6 = AMDGV_GFX12_1_DOORBELL_MEC_RING6;
    adapt->doorbell_index.mec_ring7 = AMDGV_GFX12_1_DOORBELL_MEC_RING7;
	adapt->doorbell_index.userqueue_start = AMDGV_GFX12_1_DOORBELL_USERQUEUE_START;
	adapt->doorbell_index.userqueue_end = AMDGV_GFX12_1_DOORBELL_USERQUEUE_END;
	adapt->doorbell_index.mes_ring0 = AMDGV_GFX12_1_DOORBELL_MES_RING0;
	adapt->doorbell_index.mes_ring1 = AMDGV_GFX12_1_DOORBELL_MES_RING1;

	adapt->doorbell_index.xcc_doorbell_range =
		adapt->doorbell_index.userqueue_end - adapt->doorbell_index.kiq + 1;

	return 0;
}


 #endif
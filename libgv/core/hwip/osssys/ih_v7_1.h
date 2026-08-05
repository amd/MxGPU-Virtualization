/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

 #ifndef IH_V7_1_H
 #define IH_V7_1_H

 typedef enum _AMDGV_IH_V7_DOORBELL_ASSIGNMENT {
	/* Compute: 0~255 */
	/* SDMA: 0x100 ~ 0x19F */
	AMDGV_IH_V7_DOORBELL_sDMA_ENGINE_START  = 0x100,
    AMDGV_IH_V7_DOORBELL_sDMA_ENGINE_END    = 0x19F,
	/* IH: 0x1A0 ~ 0x1AF */
	AMDGV_IH_V7_DOORBELL_IH = 0x1A0,
	/* MMSCH: 0x1B0 ~ 0x1C2 */
	AMDGV_IH_V7_DOORBELL_MMSCH0 = 0x1B0
} AMDGV_IH_V7_DOORBELL_ASSIGNMENT;

 #endif
/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __UMC_V6_7_H__
#define __UMC_V6_7_H__

#include "amdgv.h"
#include "amdgv_device.h"

/* HBM  Memory Channel Width */
#define UMC_V6_7_HBM_MEMORY_CHANNEL_WIDTH	128
/* number of umc channel instance with memory map register access */
#define UMC_V6_7_CHANNEL_INSTANCE_NUM		8
/* number of umc instance with memory map register access */
#define UMC_V6_7_UMC_INSTANCE_NUM		4
/* total channel instances in one umc block */
#define UMC_V6_7_TOTAL_CHANNEL_NUM (UMC_V6_7_CHANNEL_INSTANCE_NUM * UMC_V6_7_UMC_INSTANCE_NUM)
/* UMC regiser per channel offset */
#define UMC_V6_7_PER_CHANNEL_OFFSET		0x400
/* UMC regiser per instance offset */
#define UMC_V6_7_PER_INST_OFFSET		0x40000

/* EccErrCnt max value */
#define UMC_V6_7_CE_CNT_MAX		0xffff
/* umc ce interrupt threshold */
#define UMC_V6_7_CE_INT_THRESHOLD	0xffff
/* umc ce count initial value */
#define UMC_V6_7_CE_CNT_INIT (UMC_V6_7_CE_CNT_MAX - UMC_V6_7_CE_INT_THRESHOLD)
/* The CH4 bit in SOC physical address */
#define UMC_V6_7_PA_CH4_BIT	12

/* XOR bit 20, 25, 34 of PA into CH4 bit (bit 12 of PA),
 * hash bit is only effective when related setting is enabled
 */
#define CHANNEL_HASH(channel_idx, pa) (((channel_idx) >> 4) ^ \
			(((pa)  >> 20) & 0x1ULL) ^ \
			(((pa)  >> 25) & 0x1ULL) ^ \
			(((pa)  >> 34) & 0x1ULL))
#define SET_CHANNEL_HASH(channel_idx, pa) do { \
		(pa) &= ~(0x1ULL << UMC_V6_7_PA_CH4_BIT); \
		(pa) |= (CHANNEL_HASH(channel_idx, pa) << UMC_V6_7_PA_CH4_BIT); \
	} while (0)

extern const struct amdgv_umc_funcs umc_v6_7_funcs;
extern const uint32_t
	umc_v6_7_channel_idx_tbl_even_die[UMC_V6_7_UMC_INSTANCE_NUM][UMC_V6_7_CHANNEL_INSTANCE_NUM];
extern const uint32_t
	umc_v6_7_channel_idx_tbl_odd_die[UMC_V6_7_UMC_INSTANCE_NUM][UMC_V6_7_CHANNEL_INSTANCE_NUM];

void umc_v6_7_set_umc_funcs(struct amdgv_adapter *adapt);

#endif

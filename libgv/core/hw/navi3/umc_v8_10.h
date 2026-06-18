/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef UMC_V8_10_H
#define UMC_V8_10_H

#include <amdgv.h>
#include <amdgv_device.h>

/* number of umc nodes */
#define UMC_V8_10_NODE_INSTANCE_NUM_NV32	4
/* number of umc channel instance with memory map register access */
#define UMC_V8_10_CHANNEL_INSTANCE_NUM		2
/* number of umc instance with memory map register access */
#define UMC_V8_10_UMC_INSTANCE_NUM			2
/* total umc channels include harvest node and disabled channels */
#define UMC_V8_10_TOTAL_CHANNELS            \
	(UMC_V8_10_NODE_INSTANCE_NUM_NV32 * \
	 UMC_V8_10_CHANNEL_INSTANCE_NUM * \
	 UMC_V8_10_UMC_INSTANCE_NUM)
/* UMC regiser per channel offset */
#define UMC_V8_10_PER_CHANNEL_OFFSET		0x400
/* UMC regiser per instance offset */
#define UMC_V8_10_PER_INST_OFFSET		0x4000
/* UMC regiser per node offset */
#define UMC_V8_10_PER_NODE_OFFSET		0x800000
/* total available channel instances for all umc nodes */
#define UMC_V8_10_TOTAL_CHANNEL_NUM_NV32(adapt) \
	((UMC_V8_10_CHANNEL_INSTANCE_NUM * \
	 UMC_V8_10_UMC_INSTANCE_NUM * \
	 adapt->umc.num_umc) \
	 - (adapt)->umc.channel_dis_num)

/* EccErrCnt max value */
#define UMC_V8_10_CE_CNT_MAX			0xffff
/* umc ce interrupt threshold */
#define UMC_V8_10_CE_INT_THRESHOLD		0xffff
/* umc ce count initial value */
#define UMC_V8_10_CE_CNT_INIT \
	(UMC_V8_10_CE_CNT_MAX - UMC_V8_10_CE_INT_THRESHOLD)

extern const struct amdgv_umc_funcs umc_v8_10_funcs;

extern const uint32_t
	umc_v8_10_channel_idx_tbl_nv32[]
				[UMC_V8_10_UMC_INSTANCE_NUM]
				[UMC_V8_10_CHANNEL_INSTANCE_NUM];

void umc_v8_10_set_umc_funcs(struct amdgv_adapter *adapt);
#endif

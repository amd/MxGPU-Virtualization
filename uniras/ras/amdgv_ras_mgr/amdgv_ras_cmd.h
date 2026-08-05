/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE.
 */

#ifndef __AMDGV_RAS_CMD_H__
#define __AMDGV_RAS_CMD_H__
#include "../rascore/ras.h"

enum amdgv_ras_asic_type {
	AMDGV_RAS_CHIP_TONGA = 0,
	AMDGV_RAS_CHIP_VEGA10 = 1,
	AMDGV_RAS_CHIP_MI200 = 2,
	AMDGV_RAS_CHIP_NAVI10 = 3,
	AMDGV_RAS_CHIP_NAVI12,
	AMDGV_RAS_CHIP_NAVI21 = 5,
	AMDGV_RAS_CHIP_NAVI22,
	AMDGV_RAS_CHIP_NAVI31,
	AMDGV_RAS_CHIP_NAVI32,
	AMDGV_RAS_CHIP_MI300X,
	AMDGV_RAS_CHIP_MI300A,
	AMDGV_RAS_CHIP_MI308X,
	AMDGV_RAS_CHIP_UNKNOWN,
	AMDGV_RAS_CHIP_LAST,
};

enum amdgv_ras_cmd_id {
	RAS_CMD__AMDGV_BEGIN = RAS_CMD_ID_MXGPU_START,
	RAS_CMD__RAS_TA_LOAD,
	RAS_CMD__RAS_TA_UNLOAD,
	RAS_CMD__GET_FB_REGIONS,
	RAS_CMD__AMDGV_SUPPORTED_MAX = RAS_CMD_ID_MXGPU_END,
};

enum ras_ta_load_status {
	RAS_TA_STATUS_NO_CHANGE,
	RAS_TA_STATUS_UPGRADED,
	RAS_TA_STATUS_DOWNGRADED,
	RAS_TA_STATUS_LOADED
};

#pragma pack(push, 8)
struct ras_cmd_ras_ta_load_req {
	struct ras_cmd_dev_handle dev;
	uint32_t version;
	uint32_t data_len;
	uint64_t data_addr;
	uint32_t reserved[4];
};

struct ras_cmd_ras_ta_load_rsp {
	uint32_t version;
	uint32_t ta_status;
	uint64_t ras_session_id; // starts from 0
	uint32_t reserved[4];
};

struct ras_cmd_ras_ta_unload_req {
	struct ras_cmd_dev_handle dev;
	uint64_t ras_session_id;
	uint32_t reserved[4];
};


struct ras_cmd_fb_region_req {
	struct ras_cmd_dev_handle dev;
	uint32_t vf_idx;
	uint32_t reserved[5];
};

enum amdgv_region_type {
	AMDGV_REGION_PF_DATA_EXCHANGE = 0,
	AMDGV_REGION_PF_IP_DISCOVERY,
	AMDGV_REGION_TMR,
	AMDGV_REGION_CSA,
	AMDGV_REGION_VF_FB,
	AMDGV_REGION_VF_DATA_EXCHANGE,
	AMDGV_REGION_VF_IP_DISCOVERY,
};

struct ras_cmd_region_area {
	uint64_t start;
	uint64_t size;
	enum amdgv_region_type type;
	uint32_t reserved[5];
};

#define AMDGV_MAX_REGIONS  8
struct ras_cmd_fb_regions_rsp {
	uint32_t version;
	uint32_t reg_cnt;
	struct ras_cmd_region_area regions[AMDGV_MAX_REGIONS];
};

#pragma pack(pop)

int amdgv_ras_cmd_ioctl_handler(struct ras_core_context *ras_core,
		uint8_t *cmd_buf, uint32_t buf_size);

int amdgv_ras_cmd_handle_vf_cmd(struct ras_core_context *ras_core,
		uint32_t idx_vf, struct ras_cmd_ctx *cmd);
int amdgv_ras_submit_cmd(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data);
int amdgv_ras_cmd_add_device(struct ras_core_context *ras_core);
int amdgv_ras_cmd_remove_device(struct ras_core_context *ras_core);

struct auto_update_cmd *amdgv_ras_add_vf_cmd_to_auto_list(struct amdgv_adapter *adapt,
		uint32_t idx_vf, struct auto_update_cmd *rcmd, uint32_t mode);
int amdgv_ras_cmd_update_auto_list(struct amdgv_adapter *adapt);
int amdgv_ras_cmd_set_auto_list(struct amdgv_adapter *adapt, bool enable);
int amdgv_ras_cmd_clear_vf_auto_list(struct amdgv_adapter *adapt,
		uint32_t idx_vf);

#endif

/*
 * Copyright (c) 2017-2024 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef __GIM_LIVE_UPDATE_H__
#define __GIM_LIVE_UPDATE_H__


#include "gim.h"

enum gim_live_update_data_status {
	GIM_LIVE_UPDATE_DATA_OK = 0,
	GIM_LIVE_UPDATE_DATA_CORRUPTED = 1,
};

enum gim_live_update_type {
	GIM_LIVE_UPDATE_FILE,
	GIM_LIVE_UPDATE_MEM,
	GIM_LIVE_UPDATE_DISABLED,
};

struct gim_live_update_manager {
	enum gim_live_update_type update_type;

	uint32_t gpu_data_addr_lo;
	uint32_t gpu_data_addr_hi;
	uint32_t gpu_data_size;
	uint8_t  *gpu_data_ptr;
	uint8_t  *io_ptr;

	uint32_t gpu_num;

	bool is_valid_crc;
	bool is_valid_update;
	bool is_export;
};

struct amdgv_live_update_file_header *gim_live_update_get_file_header_ptr(struct gim_live_update_manager *mgr);
struct amdgv_gpu_data_v2 *gim_live_update_get_data_ptr(struct gim_live_update_manager *mgr, uint32_t gpu_index);
void gim_live_update_init_manager(struct gim_live_update_manager *mgr);
void gim_live_update_fini_manager(struct gim_live_update_manager *mgr);

enum gim_live_update_data_status gim_live_update_import_data(struct gim_live_update_manager *mgr, struct gim_dev_data *dev_data);
void gim_live_update_export_data(struct gim_live_update_manager *mgr, struct pci_dev *pdev, uint32_t *gim_gpu_id);

#endif

/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_device.h"
#include "amdgv_diag_data.h"
#include "amdgv_diag_data_host_drv.h"
#include "amdgv_diag_data_trace_log.h"
#include "amdgv_diag_data_gen_info.h"
#include "amdgv_sriovmsg.h"
#include "amdgv.h"
#include "amdgv_api.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_list.h"
#include "amdgv_device.h"
#include "amdgv_sched_internal.h"
#include "amdgv_guard.h"
#include "atombios/atom.h"

static const uint32_t this_block = AMDGV_MANAGEMENT_BLOCK;

/* Hosr Driver Init and fini functions */
int amdgv_diag_data_host_driver_init(struct amdgv_adapter *adapt)
{
	int i = 0;
	struct amdgv_diag_data_host_drv_blk *host_drv;
	uint64_t cur_addr = (uint64_t)adapt->diag_data.host_drv_buff.vaddr;

	if (cur_addr > cur_addr + AMDGV_DIAG_DATA_HOST_DRV_MEM_SIZE) {
		AMDGV_WARN("Host specific block memory overflow\n");
		return AMDGV_FAILURE;
	}

	/* Allocate host driver specific block */
	host_drv = (struct amdgv_diag_data_host_drv_blk *)
		AMDGV_DIAG_DATA_HOST_DRV_INTER_STRUCT_OFFSET;
	if (!host_drv) {
		AMDGV_WARN("Host specific block not initialized\n");
		return AMDGV_FAILURE;
	}

	/* Zero out the memory */
	oss_memset(adapt->diag_data.host_drv_buff.vaddr, 0,
		   AMDGV_DIAG_DATA_HOST_DRV_MEM_SIZE);

	/* Init TRACE_LOG logging */
	AMDGV_DIAG_DATA_FILL_MEM_BLK(host_drv->trace_log.mem_blk, cur_addr, HOST_TRACE_LOG);

	/* Init POST_VBIOS_LOG logging */
	AMDGV_DIAG_DATA_FILL_MEM_BLK(host_drv->vbios_post_log.mem_blk, cur_addr,
				HOST_POST_VBIOS_LOG);

	/* Init ERROR_DUMP logging */
	AMDGV_DIAG_DATA_FILL_MEM_BLK(host_drv->error_dump.mem_blk, cur_addr, HOST_ERROR_DUMP);

	/* fill in the trace log ring buffer entries */
	host_drv->trace_log.total_entries =
		host_drv->trace_log.mem_blk.size / sizeof(struct amdgv_diag_data_trace_log);
	host_drv->trace_log.feature_entries =
		host_drv->trace_log.total_entries / AMDGV_DIAG_DATA_TRACE_LOG_FEATURE_TOTAL;
	oss_memset(host_drv->trace_log.w_count, 0, sizeof(host_drv->trace_log.w_count));
	host_drv->trace_log.entry = host_drv->trace_log.mem_blk.vaddr;
	for (i = 0; i < AMDGV_DIAG_DATA_TRACE_LOG_FEATURE_TOTAL; i++) {
		host_drv->trace_log.feature_offset[i] =
			host_drv->trace_log.feature_entries * i;
	}

	/* fill in the vbios post log ring buffer entries */
	host_drv->vbios_post_log.total_entries =
		host_drv->vbios_post_log.mem_blk.size /
		sizeof(struct amdgv_diag_data_vbios_post_entry);
	host_drv->vbios_post_log.w_count = 0;
	host_drv->vbios_post_log.entry = host_drv->vbios_post_log.mem_blk.vaddr;
	host_drv->vbios_post_log.hdr = host_drv->vbios_post_log.mem_blk.vaddr;

	/* fill in the error dump log ring buffer entries */
	host_drv->error_dump.total_entries =
		host_drv->error_dump.mem_blk.size /
		sizeof(struct amdgv_diag_data_error_dump_entry);
	host_drv->error_dump.w_count = 0;
	host_drv->error_dump.entry = host_drv->error_dump.mem_blk.vaddr;

	return 0;
}

void amdgv_diag_data_host_driver_fini(struct amdgv_adapter *adapt)
{
	struct amdgv_diag_data_host_drv_blk *host_drv =
		(struct amdgv_diag_data_host_drv_blk *)
			AMDGV_DIAG_DATA_HOST_DRV_INTER_STRUCT_OFFSET;

	if (!host_drv)
		return;

	host_drv->trace_log.mem_blk.vaddr = NULL;
	host_drv->trace_log.entry = NULL;

	host_drv->error_dump.mem_blk.vaddr = NULL;

	host_drv->vbios_post_log.mem_blk.vaddr = NULL;
	host_drv->vbios_post_log.entry = NULL;
}

/* VBIOS init and fini functions */
void amdgv_diag_data_vbios_post_end(struct amdgv_adapter *adapt)
{
	struct amdgv_diag_data_host_drv_blk *host_drv =
		(struct amdgv_diag_data_host_drv_blk *)
			AMDGV_DIAG_DATA_HOST_DRV_INTER_STRUCT_OFFSET;
	if (!host_drv)
		return;

	if (host_drv->vbios_post_log.mem_blk.vaddr) {
		host_drv->vbios_post_log.w_count = 0;
	}
}

int amdgv_diag_data_vbios_post_start(struct amdgv_adapter *adapt, uint32_t post_type,
					uint32_t index, uint32_t ps, uint32_t ws, uint32_t len,
					uint32_t base, uint32_t code_loc)
{
	struct amdgv_diag_data_host_drv_blk *host_drv =
		(struct amdgv_diag_data_host_drv_blk *)
			AMDGV_DIAG_DATA_HOST_DRV_INTER_STRUCT_OFFSET;
	if (!host_drv)
		return AMDGV_FAILURE;

	host_drv->vbios_post_log.hdr->cmd_table_idx = index;
	host_drv->vbios_post_log.hdr->post_type = post_type;
	/* Lengths are in DWRODS */
	host_drv->vbios_post_log.hdr->ps_size = ps * 4;
	host_drv->vbios_post_log.hdr->ws_size = ws * 4;
	host_drv->vbios_post_log.hdr->len = len * 4;
	host_drv->vbios_post_log.hdr->base = base;
	host_drv->vbios_post_log.hdr->code_loc = code_loc;
	oss_memset(host_drv->vbios_post_log.hdr->reserved, 0, AMDGV_DIAG_DATA_VBIOS_HDR_RESERVED);

	/* Skip header */
	host_drv->vbios_post_log.w_count = AMDGV_DIAG_DATA_VPOST_POST_KEEP_ENTRIES;

	return 0;
}

int amdgv_diag_data_add_vbios_post_log(struct amdgv_adapter *adapt, uint32_t op,
					  uint32_t arg, uint64_t timestamp, uint32_t start_loc,
					  uint32_t end_loc, uint32_t write_val,
					  uint32_t write_addr, uint64_t read_val,
					  uint32_t read_addr)
{
	uint32_t index;
	int op_data_len;
	struct amdgv_diag_data_vbios_post_entry *entry;
	struct amdgv_diag_data_host_drv_blk *host_drv;
	struct amdgv_diag_data_vbios_post *vpost_log;

	host_drv = (struct amdgv_diag_data_host_drv_blk *)
		AMDGV_DIAG_DATA_HOST_DRV_INTER_STRUCT_OFFSET;
	if (!host_drv)
		return AMDGV_FAILURE;

	vpost_log = &host_drv->vbios_post_log;
	if (vpost_log->mem_blk.vaddr == NULL)
		return AMDGV_FAILURE;

	index = AMDGV_DIAG_DATA_RB_KEEP_INDEX(vpost_log->w_count, vpost_log->total_entries,
						 AMDGV_DIAG_DATA_VPOST_POST_KEEP_ENTRIES);

	entry = vpost_log->entry + index;

	/* Copy the data */
	entry->op = op;
	entry->arg = arg;
	entry->timestamp = timestamp;
	entry->val[AMDGV_DIAG_DATA_VBIOS_OP_WRITE].addr = write_addr;
	entry->val[AMDGV_DIAG_DATA_VBIOS_OP_WRITE].value = write_val;
	entry->val[AMDGV_DIAG_DATA_VBIOS_OP_READ].addr = read_addr;
	entry->val[AMDGV_DIAG_DATA_VBIOS_OP_READ].value = read_val;
	entry->start_loc = start_loc;
	entry->end_loc = end_loc;
	op_data_len = end_loc - start_loc;

	/* Update data contents, for the op */
	if (op_data_len > 0)
		oss_memcpy(entry->data, (uint8_t *)adapt->vbios.atom_context->bios + start_loc,
			   op_data_len > AMDGV_DIAG_DATA_VBIOS_OP_LENGTH ?
				   AMDGV_DIAG_DATA_VBIOS_OP_LENGTH :
				   op_data_len);

	/* If the entry is similar to previous do not record it */
	if (vpost_log->last_entry && entry->op == vpost_log->last_entry->op) {
		if (oss_memcmp(vpost_log->last_entry, entry,
			       sizeof(*entry) - sizeof(entry->timestamp)) == 0) {
			oss_memset(entry, 0, sizeof(*entry));
			return 0;
		}
	}

	vpost_log->w_count++;

	/*
	* If the w_count overflow to 0, we need change w_count as entry size to
	* indicate the ring buffer is full.
	*/
	if (vpost_log->w_count == 0) {
		vpost_log->w_count = vpost_log->total_entries;
	}

	/* Update the last entry */
	vpost_log->last_entry = entry;

	return 0;
}

static int amdgv_diag_data_host_driver_collect_gen_info(
	struct amdgv_adapter *adapt, struct amdgv_diag_data_file_info *file_data)
{
	int i;
	int j;
	int k;
	uint8_t *img;
	struct amdgv_memmgr_mem *csa_fb_mem;
	struct psp_local_memory *tmr_mem;
	uint64_t total_avail_fb;
	uint64_t fb_offset;
	uint64_t fb_real_size;
	uint32_t total_avail_fb_in_mb;
	int info_blk_size;
	struct amdgv_vf_device *func_entry;
	struct amdgv_diag_data_gen_info *info = NULL;
	uint8_t anchor[] = { 0x20, 0x20, 0x00, 0x20, 0x20, 0x20,
			     0x20, 0x20, 0x20, 0x20, 0x20, 0x00 };

	/* Block payload */
	info_blk_size = sizeof(struct amdgv_diag_data_gen_info);

	/* Generate the block header
	 * In addition to generating block header log
	 * the API will also check if the user buffer has
	 * enough space to hold block header and block data
	*/
	if (amdgv_diag_data_gen_blk_file_hdr(adapt, file_data,
						AMDGV_DIAG_DATA_BLOCK_ID_HOST_GENERAL_INFO, info,
						info_blk_size, 0) != 0) {
		AMDGV_WARN("Unable to generate block header for gen info\n");
		return AMDGV_FAILURE;
	}

	info = (struct amdgv_diag_data_gen_info *)((uint8_t *)file_data->buff +
						      file_data->cur_offset);
	oss_memset(info, 0, info_blk_size);

	/* Fill in the values for host driver information */

	/* Driver Information */
	/* Driver Information -- Version */
	amdgv_get_version((int *)&info->driver_info.version.major,
			  (int *)&info->driver_info.version.minor);

	/* Driver Information -- Config Info */
	if (adapt->flags & AMDGV_FLAG_SENSITIVE_EVENT_GUARD)
		info->driver_info.config_info.guard_enable = 1;
	else
		info->driver_info.config_info.guard_enable = 0;
	info->driver_info.config_info.skip_check_bad_gpu = adapt->opt.bad_page_detection_mode;

	info->driver_info.config_info.supp_field_flags =
		adapt->config.caps.supported_fields_flags;
	info->driver_info.config_info.sched_mode = adapt->opt.gfx_sched_mode;
	info->driver_info.config_info.log_level = adapt->log_level;
	info->driver_info.config_info.log_mask = adapt->log_mask;
	info->driver_info.config_info.allow_time_full_access =
		adapt->opt.allow_time_full_access;
	info->driver_info.config_info.allow_time_cmd_complete =
		adapt->opt.allow_time_cmd_complete;
	info->driver_info.config_info.fw_load_type = adapt->fw_load_type;
	info->driver_info.config_info.perf_mon_enable = adapt->opt.perf_mon_enable;

	/* GPU information */
	/* GPU Information -- Chip Info */
	info->gpu_info.chip_info.bdf = adapt->bdf;
	info->gpu_info.chip_info.domain = adapt->bdf >> 16;
	info->gpu_info.chip_info.vendor_id = adapt->vendor_id;
	info->gpu_info.chip_info.dev_id = adapt->dev_id;
	info->gpu_info.chip_info.rev_id = adapt->rev_id;
	info->gpu_info.chip_info.sub_sys_id = adapt->sub_sys_id;
	info->gpu_info.chip_info.sub_vnd_id = adapt->sub_vnd_id;
	/* cache product information if not visit */
	if (adapt->product_info.visit != true && adapt->pp.pp_funcs &&
	    adapt->pp.pp_funcs->get_fru_product_info &&
	    adapt->pp.pp_funcs->get_fru_product_info(adapt))
		AMDGV_WARN("Failed to get fru product info\n");
	if (adapt->product_info.valid) {
		oss_memcpy(info->gpu_info.chip_info.model_number,
			   adapt->product_info.model_number, STRLEN_NORMAL);
		oss_memcpy(info->gpu_info.chip_info.product_serial,
			   adapt->product_info.product_serial, STRLEN_NORMAL);
		oss_memcpy(info->gpu_info.chip_info.fru_id, adapt->product_info.fru_id,
			   STRLEN_NORMAL);
		oss_memcpy(info->gpu_info.chip_info.product_name,
			   adapt->product_info.product_name, STRLEN_NORMAL);
		oss_memcpy(info->gpu_info.chip_info.manufacturer_name,
			   adapt->product_info.manufacturer_name, STRLEN_NORMAL);
	}
	info->gpu_info.chip_info.vf_number = adapt->num_vf;

	if (amdgv_mcp_get_spatial_partition_mode(adapt,
		&info->gpu_info.chip_info.spatial_part_mode)) {
		info->gpu_info.chip_info.spatial_part_mode =
			SPATIAL_PARTITION_MODE__UNKNOWN;
	}

	if (amdgv_mcp_get_num_gfx_spatial_partitions(adapt,
		&info->gpu_info.chip_info.gfx_part_cnt)) {
		info->gpu_info.chip_info.gfx_part_cnt = 1;
	}

	if (amdgv_mcp_get_num_xcc_per_partition(adapt,
		&info->gpu_info.chip_info.num_xcc_per_partition)) {
		info->gpu_info.chip_info.num_xcc_per_partition = 1;
	}

	/* GPU Information -- XGMI info */
	info->gpu_info.xgmi_info.xgmi_node_id = adapt->xgmi.node_id;
	info->gpu_info.xgmi_info.xgmi_hive_id = adapt->xgmi.hive_id;
	info->gpu_info.xgmi_info.xgmi_node_segment_size = adapt->xgmi.node_segment_size;
	info->gpu_info.xgmi_info.xgmi_phy_node_id = adapt->xgmi.phy_node_id;
	info->gpu_info.xgmi_info.xgmi_phy_nodes_num = adapt->xgmi.phy_nodes_num;

	/* GPU Information -- resource map */
	info->gpu_info.resource_map.fb_pa = adapt->fb_pa;
	info->gpu_info.resource_map.fb_size = adapt->fb_size;
	info->gpu_info.resource_map.mmio_pa = adapt->mmio_pa;
	info->gpu_info.resource_map.mmio_size = adapt->mmio_size;
	info->gpu_info.resource_map.io_mem_pa = adapt->io_mem_pa;
	info->gpu_info.resource_map.io_mem_size = adapt->io_mem_size;
	info->gpu_info.resource_map.doorbell_pa = adapt->doorbell_pa;
	info->gpu_info.resource_map.doorbell_size = adapt->doorbell_size;

	/* GPU Information -- Vbios info */
	info->gpu_info.vbios_info.version = adapt->vbios_cache.version;
	img = adapt->vbios.image;
	if (img) {
		/* search for the first 1k vbios space*/
		for (i = 0; i < 1024 - sizeof(anchor); i++) {
			for (j = 0; j < sizeof(anchor) && ((i + j) < 1024); j++) {
				if (anchor[j] != img[i + j])
					break;
			}

			if (j == sizeof(anchor)) {
				for (k = 0; k < 128; k++) {
					if (img[i + j + k] == '\\')
						break;
				}

				if (k < 128) {
					oss_memcpy(info->gpu_info.vbios_info.build_num,
						   img + i - BUILD_NUM_LEN, BUILD_NUM_LEN);
					oss_memcpy(info->gpu_info.vbios_info.part_info,
						   img + i + j, k);
					oss_memcpy(info->gpu_info.vbios_info.build_date,
						   img + 0x50, BUILD_DATA_LEN);
				}
			}
		}
	}

	/* GPU Information --  Firmware information */
	for (i = 1; i < AMDGV_FIRMWARE_ID__MAX; i++) {
		bool support;
		if (adapt->psp.fw_id_support)
			support = adapt->psp.fw_id_support(i);
		else
			support = false;
		if (support) {
			info->gpu_info.fw_info[i - 1].version = adapt->psp.fw_info[i];
		} else {
			info->gpu_info.fw_info[i - 1].version = 0;
		}
		info->gpu_info.fw_info[i - 1].fw_id = i;
	}

	/* GPU Information -- PF information */
	info->gpu_info.pf_info.fb_pa =
		MBYTES_TO_BYTES(adapt->array_vf[AMDGV_PF_IDX].fb_offset);
	info->gpu_info.pf_info.fb_size =
		MBYTES_TO_BYTES(adapt->array_vf[AMDGV_PF_IDX].fb_size);
	amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_avail_fb_in_mb);
	total_avail_fb = (uint64_t)total_avail_fb_in_mb;
	total_avail_fb = MBYTES_TO_BYTES(total_avail_fb);
	info->gpu_info.pf_info.fb_total_avail = total_avail_fb;
	info->gpu_info.pf_info.fb_reservation_offset = adapt->opt.libgv_res_fb_offset;
	info->gpu_info.pf_info.fb_reservation_size = adapt->opt.libgv_res_fb_size;

	/* Framebuffer info */
	csa_fb_mem = adapt->gpuiov.csa_fb_mem;
	if (csa_fb_mem == NULL) {
		info->gpu_info.fb_info.csa_region.size = 0;
		info->gpu_info.fb_info.csa_region.offset = 0;
	} else {
		info->gpu_info.fb_info.csa_region.offset =
			total_avail_fb - amdgv_memmgr_get_offset(csa_fb_mem);
		info->gpu_info.fb_info.csa_region.size = amdgv_memmgr_get_size(csa_fb_mem);
	}
	tmr_mem = &adapt->psp.tmr_context;
	if (tmr_mem == NULL || tmr_mem->mem == NULL) {
		info->gpu_info.fb_info.tmr_region.size = 0;
		info->gpu_info.fb_info.tmr_region.offset = 0;
	} else {
		info->gpu_info.fb_info.tmr_region.offset =
			total_avail_fb - amdgv_memmgr_get_offset(tmr_mem->mem);
		info->gpu_info.fb_info.tmr_region.size = amdgv_memmgr_get_size(tmr_mem->mem);
	}
	for (i = 0; i < adapt->num_vf; i++) {
		info->gpu_info.fb_info.vf_region[i].valid_region = 1;
		fb_offset = MBYTES_TO_BYTES(adapt->array_vf[i].fb_offset);
		fb_real_size = MBYTES_TO_BYTES(adapt->array_vf[i].real_fb_size);
		info->gpu_info.fb_info.vf_region[i].func_dataexchange.size =
			KBYTES_TO_BYTES(AMDGV_VF_DATAEXCHANGE_SIZE);
		info->gpu_info.fb_info.vf_region[i].func_dataexchange.offset =
			fb_offset + KBYTES_TO_BYTES(AMD_SRIOV_MSG_DATAEXCHANGE_OFFSET_KB);
		info->gpu_info.fb_info.vf_region[i].func_ipd.size = AMDGV_IP_DISCOVERY_SIZE;
		info->gpu_info.fb_info.vf_region[i].func_ipd.offset =
			fb_offset + fb_real_size - AMDGV_IP_DISCOVERY_OFFSET;
	}

	/* Virtual Function Info */
	for (i = 0; i < adapt->num_vf; i++) {
		func_entry = &adapt->array_vf[i];

		info->gpu_info.vf_info[i].valid_vf = 1;
		info->gpu_info.vf_info[i].idx_vf = func_entry->idx_vf;
		info->gpu_info.vf_info[i].bdf = func_entry->bdf;
		info->gpu_info.vf_info[i].fb_offset = MBYTES_TO_BYTES(func_entry->fb_offset);
		info->gpu_info.vf_info[i].fb_size = MBYTES_TO_BYTES(func_entry->fb_size);
		info->gpu_info.vf_info[i].real_fb_size =
			MBYTES_TO_BYTES(adapt->array_vf[i].real_fb_size);
		info->gpu_info.vf_info[i].gfx_time_slice =
			func_entry->time_slice[AMDGV_SCHED_BLOCK_GFX];
		info->gpu_info.vf_info[i].gfx_time_divider = 0;
		oss_memcpy(info->gpu_info.vf_info[i].mm_bandwidth, func_entry->mm_bandwidth,
			   sizeof(info->gpu_info.vf_info[i].mm_bandwidth));

		for (j = 0; j < AMDGV_GUARD_EVENT_MAX; j++) {
			info->gpu_info.vf_info[i].guard[j].guard_type = j;
			if (func_entry->guard) {
				info->gpu_info.vf_info[i].guard[j].threshold =
					func_entry->guard->event[j].threshold;
				info->gpu_info.vf_info[i].guard[j].interval =
					func_entry->guard->event[j].interval;
			}
		}
	}

	/* Generate checksum */
	file_data->last_blk_hdr->checksum = amd_sriov_msg_checksum(info, info_blk_size, 0, 0);
	file_data->cur_offset += info_blk_size;

	AMDGV_INFO("Host driver info of size:%d added\n", info_blk_size);

	return 0;
}

/* Functions to add data in the diagnosis data log */
void amdgv_diag_data_add_error(struct amdgv_adapter *adapt)
{
	struct amdgv_error_ring_buffer *err_rb;
	struct amdgv_error_entry *r_entry;
	uint32_t wr_diff;
	uint32_t r_index;
	uint32_t w_index;
	uint32_t write_count;
	uint32_t read_count;

	struct amdgv_diag_data_error_dump_entry *w_entry;
	struct amdgv_diag_data_host_drv_blk *host_drv =
		(struct amdgv_diag_data_host_drv_blk *)
			AMDGV_DIAG_DATA_HOST_DRV_INTER_STRUCT_OFFSET;

	if (!host_drv || !host_drv->error_dump.mem_blk.vaddr)
		return;

	read_count = host_drv->error_dump.error_read_count;

	if (host_drv->error_dump.total_entries > host_drv->error_dump.w_count)
		w_index = host_drv->error_dump.w_count;
	else
		w_index = AMDGV_DIAG_DATA_RB_KEEP_INDEX(
			host_drv->error_dump.w_count, host_drv->error_dump.total_entries,
			AMDGV_DIAG_DATA_ERROR_DUMP_FIRST_KEEP);

	err_rb = adapt->error_ring_buffer;
	write_count = adapt->error_ring_buffer->write_count;

	/* log the errors to diagnosis data */
	while (write_count != read_count) {
		wr_diff = write_count - read_count;

		/* Shift the read_count to oldest entry if OF */
		if (wr_diff > AMDGV_ERROR_BUF_ENTRY_SIZE) {
			read_count = write_count - AMDGV_ERROR_BUF_ENTRY_SIZE + 1;
		}
		r_index = AMDGV_ERROR_INDEX(read_count);
		read_count++;
		r_entry = &err_rb->error_entry_buffer[r_index];
		w_entry = host_drv->error_dump.entry + w_index;

		w_entry->cpu_timestamp = r_entry->timestamp;
		w_entry->error_data = r_entry->error_data;
		w_entry->error_code = r_entry->error_code;
		w_entry->vf_idx = r_entry->vf_idx;
		if (adapt->diag_data.get_gpu_ref_timestamp &&
		    adapt->status == AMDGV_STATUS_HW_INIT)
			w_entry->gpu_timestamp =
				adapt->diag_data.get_gpu_ref_timestamp(adapt);

		host_drv->error_dump.w_count++;
		/*
		* If the w_count overflow to 0, we need change w_count as entry
		* size to indicate the ring buffer is full.
		*/
		if (host_drv->error_dump.w_count == 0) {
			host_drv->error_dump.w_count = host_drv->error_dump.total_entries;
		}
	}
	host_drv->error_dump.error_read_count = read_count;
}

int amdgv_diag_data_add_trace_log(struct amdgv_adapter *adapt, uint8_t feature,
				     uint16_t code, uint32_t ext_code)
{
	uint32_t index;
	uint32_t feature_idx;
	uint32_t gpu_timestamp = 0;
	uint64_t cpu_timestamp = 0;
	struct amdgv_diag_data_trace_log *entry;
	struct amdgv_diag_data_host_drv_blk *host_drv =
		(struct amdgv_diag_data_host_drv_blk *)
			AMDGV_DIAG_DATA_HOST_DRV_INTER_STRUCT_OFFSET;

	if (!host_drv || !host_drv->trace_log.mem_blk.vaddr)
		return AMDGV_FAILURE;

	/* If feature is 0 or greater than the max feature, return error */
	if (!feature || feature > AMDGV_DIAG_DATA_TRACE_LOG_FEATURE_TOTAL)
		return AMDGV_FAILURE;

	/* Get CPU timestamp */
	cpu_timestamp = oss_get_time_stamp();

	/* feature idx is 1 less than the feature since feature starts with 1 */
	feature_idx = feature - 1;

	if ((feature != AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_IRQ) &&
	    (feature != AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_SMU) &&
	    (feature != AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_PSP) &&
	    (feature != AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_MAILBOX)) {
		if (adapt->diag_data.get_gpu_ref_timestamp &&
		    adapt->status == AMDGV_STATUS_HW_INIT)
			gpu_timestamp = adapt->diag_data.get_gpu_ref_timestamp(adapt);
	}

	index = AMDGV_DIAG_DATA_RB_INDEX(host_drv->trace_log.w_count[feature_idx],
					    host_drv->trace_log.feature_entries);

	/* Increment the write index for feature ring buffer */
	host_drv->trace_log.w_count[feature_idx]++;

	/*
	 * If the w_count overflow to 0, we need change w_count as entry size to
	 * indicate the ring buffer is full.
	 */
	if (host_drv->trace_log.w_count[feature_idx] == 0) {
		host_drv->trace_log.w_count[feature_idx] = host_drv->trace_log.feature_entries;
	}

	/* Go to the feature section and then add index */
	entry = host_drv->trace_log.entry + host_drv->trace_log.feature_offset[feature_idx];
	entry += index;

	/* Fill in the entry contents */
	entry->client = AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_DRV_HOST;
	entry->feature = feature;
	entry->code = code;
	entry->ext_code = ext_code;
	entry->gpu_timestamp = gpu_timestamp;
	entry->cpu_timestamp = cpu_timestamp;

	return 0;
}

static int amdgv_diag_data_host_driver_collect_error_log(
	struct amdgv_adapter *adapt, struct amdgv_diag_data_file_info *file_data)
{
	uint32_t entry_size;
	struct amdgv_diag_data_host_drv_blk *host_drv =
		(struct amdgv_diag_data_host_drv_blk *)
			AMDGV_DIAG_DATA_HOST_DRV_INTER_STRUCT_OFFSET;

	if (!host_drv || host_drv->error_dump.w_count == 0 ||
	    host_drv->error_dump.total_entries == 0) {
		AMDGV_WARN("Empty buffer for host driver error dump\n");
		return AMDGV_FAILURE;
	}

	entry_size = sizeof(struct amdgv_diag_data_error_dump_entry);

	if (amdgv_diag_data_add_ring_buffer(
		    adapt, file_data, &host_drv->error_dump.mem_blk, entry_size,
		    host_drv->error_dump.total_entries, host_drv->error_dump.w_count,
		    AMDGV_DIAG_DATA_ERROR_DUMP_FIRST_KEEP, 0)) {
		AMDGV_WARN("Unable to copy ftrace ring buffer to memory\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("Error dump of size:%d added\n", host_drv->error_dump.mem_blk.used_size);

	return 0;
}

static int amdgv_diag_data_host_driver_collect_trace_log(
	struct amdgv_adapter *adapt, struct amdgv_diag_data_file_info *file_data)
{
	uint32_t used_size = 0;
	uint32_t feature_offset = 0;
	void *feature_vaddr;
	uint32_t trace_log_offset;
	uint32_t entry_size = sizeof(struct amdgv_diag_data_trace_log);
	struct amdgv_diag_data_host_drv_blk *host_drv =
		(struct amdgv_diag_data_host_drv_blk *)
			AMDGV_DIAG_DATA_HOST_DRV_INTER_STRUCT_OFFSET;
	int i = 0;

	if (!host_drv || host_drv->trace_log.mem_blk.vaddr == NULL ||
	    host_drv->trace_log.total_entries == 0) {
		AMDGV_WARN("Empty buffer for host driver trace log\n");
		return AMDGV_FAILURE;
	}

	/* Calculate the total used size of all the features */
	for (i = 0; i < AMDGV_DIAG_DATA_TRACE_LOG_FEATURE_TOTAL; i++) {
		if (host_drv->trace_log.w_count[i] <= host_drv->trace_log.feature_entries) {
			used_size += host_drv->trace_log.w_count[i] * entry_size;
		} else {
			used_size += host_drv->trace_log.feature_entries * entry_size;
		}
	}

	if (amdgv_diag_data_gen_blk_file_hdr(
		    adapt, file_data, host_drv->trace_log.mem_blk.block_id,
		    host_drv->trace_log.mem_blk.vaddr, used_size, 0) != 0) {
		AMDGV_WARN("Unable to copy ring buffer to memory\n");
		return AMDGV_FAILURE;
	}
	host_drv->trace_log.mem_blk.used_size = used_size;
	trace_log_offset = file_data->cur_offset;

	/* Copy all features contents to tracelog buffer */
	for (i = 0; i < AMDGV_DIAG_DATA_TRACE_LOG_FEATURE_TOTAL; i++) {
		/* Jump to the Feature specific ring buffer inside mem blk */
		feature_offset = entry_size * host_drv->trace_log.feature_entries * i;

		/* Continue to next feature, if this feature is empty */
		if (host_drv->trace_log.w_count[i] == 0)
			continue;

		/* Copy data to the file as ring buffer */
		feature_vaddr = (uint8_t *)host_drv->trace_log.mem_blk.vaddr + feature_offset;
		amdgv_diag_data_copy(adapt, file_data, entry_size,
					     host_drv->trace_log.feature_entries,
					     host_drv->trace_log.w_count[i], 0, feature_vaddr);
	}

	/* Calculate the checksum */
	file_data->last_blk_hdr->checksum =
		amd_sriov_msg_checksum((uint8_t *)file_data->buff + trace_log_offset,
				       host_drv->trace_log.mem_blk.used_size, 0, 0);

	AMDGV_INFO("Trace log of size:%d added\n", host_drv->trace_log.mem_blk.used_size);

	return 0;
}

static int amdgv_diag_data_host_driver_collect_vbios_post_log(
	struct amdgv_adapter *adapt, struct amdgv_diag_data_file_info *file_data)
{
	uint32_t entry_size;
	struct amdgv_diag_data_host_drv_blk *host_drv;
	struct amdgv_diag_data_vbios_post *vpost_log;

	host_drv = (struct amdgv_diag_data_host_drv_blk *)
		AMDGV_DIAG_DATA_HOST_DRV_INTER_STRUCT_OFFSET;
	if (!host_drv)
		return AMDGV_FAILURE;

	vpost_log = &host_drv->vbios_post_log;
	if (vpost_log->w_count == 0 || vpost_log->mem_blk.vaddr == NULL) {
		AMDGV_INFO("VBIOS Post Log not avilable\n");
		return AMDGV_FAILURE;
	}

	entry_size = sizeof(struct amdgv_diag_data_vbios_post_entry);

	if (amdgv_diag_data_add_ring_buffer(
		    adapt, file_data, &vpost_log->mem_blk, entry_size,
		    vpost_log->total_entries, vpost_log->w_count,
		    AMDGV_DIAG_DATA_VPOST_POST_KEEP_ENTRIES, 0)) {
		AMDGV_WARN("Unable to copy vbios post ring buffer to memory\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("VBPOST log of size:%d added\n", vpost_log->mem_blk.used_size);

	return 0;
}

static int amdgv_diag_data_host_driver_collect_ih_ring_log(
	struct amdgv_adapter *adapt, struct amdgv_diag_data_file_info *file_data)
{
	uint8_t *blk_data = NULL;
	uint32_t blk_data_len;
	struct oss_ih_rb_info info;
	struct amdgv_diag_data_ih_ring_hdr ih_hdr;
	void *ih_ring = NULL;

	oss_memset(&ih_hdr, 0, sizeof(struct amdgv_diag_data_ih_ring_hdr));
	if (!adapt->irqmgr.ih.ring) {
		if (oss_get_ih_rb_info(adapt->dev, 0, &info)) {
			ih_hdr.ring_size = info.rb_bytes_size;
			ih_hdr.rptr = info.rb_rptr;
			ih_hdr.rptr_offset = 0;
			ih_hdr.wptr_offset = 0;
			ih_hdr.doorbell_index = 0;
			ih_hdr.use_doorbell = 0;
			ih_hdr.use_bus_addr = info.rb_in_system;
			ih_hdr.rb_dma_addr = info.rb_in_system ? info.rb_address : 0;
			ih_hdr.gpu_addr = (!info.rb_in_system) ? info.rb_address : 0;

			ih_ring = info.rb;
		}
	} else {
		ih_hdr.ring_size = adapt->irqmgr.ih.ring_size;
		ih_hdr.ring_size_log2 = adapt->irqmgr.ih.ring_size_log2;
		ih_hdr.rptr = adapt->irqmgr.ih.rptr;
		ih_hdr.rptr_offset = adapt->irqmgr.ih.rptr_offs;
		ih_hdr.wptr_offset = adapt->irqmgr.ih.wptr_offs;
		ih_hdr.doorbell_index = adapt->irqmgr.ih.doorbell_index;
		ih_hdr.use_doorbell = adapt->irqmgr.ih.use_doorbell;
		ih_hdr.use_bus_addr = adapt->irqmgr.ih.use_bus_addr;
		ih_hdr.rb_dma_addr = adapt->irqmgr.ih.rb_dma_addr;
		ih_hdr.gpu_addr = adapt->irqmgr.ih.gpu_addr;

		ih_ring = (void *)(adapt->irqmgr.ih.ring);
	}
	if (!ih_hdr.ring_size) {
		AMDGV_WARN("IH ring not initialized\n");
		return 0;
	}

	/* Block payload is ring size + the ih header */
	blk_data_len = ih_hdr.ring_size + sizeof(struct amdgv_diag_data_ih_ring_hdr);

	if (blk_data_len > AMDGV_DIAG_DATA_HOST_IH_RING_DUMP_BLK_SIZE) {
		AMDGV_WARN("IH ring data excceded the limit\n");
		return AMDGV_FAILURE;
	}

	/* Generate the block header
	 * In addition to generating block header log
	 * the API will also check if the user buffer has
	 * enough space to hold block header and block data
	 */
	if (amdgv_diag_data_gen_blk_file_hdr(adapt, file_data,
						AMDGV_DIAG_DATA_BLOCK_ID_HOST_IH_RING_DUMP,
						blk_data, blk_data_len, 0) != 0) {
		AMDGV_WARN("Unable to generate block header for ring buffer\n");
		return AMDGV_FAILURE;
	}

	/* Block payload (ring header + ring contents) starts here */
	blk_data = (uint8_t *)file_data->buff + file_data->cur_offset;

	// Copy header to the log memory
	*((struct amdgv_diag_data_ih_ring_hdr *)blk_data) = ih_hdr;

	/* Copy the contents of FB to sys mem */
	oss_memcpy(blk_data + sizeof(struct amdgv_diag_data_ih_ring_hdr), ih_ring,
		   ih_hdr.ring_size);
	file_data->cur_offset += blk_data_len;

	/* Generate checksum */
	file_data->last_blk_hdr->checksum =
		amd_sriov_msg_checksum(blk_data, blk_data_len, 0, 0);

	AMDGV_INFO("IH RB log of size:%d added\n", blk_data_len);

	return 0;
}

int amdgv_diag_data_host_driver_collect(
	struct amdgv_adapter *adapt, struct amdgv_diag_data_file_info *file_data)
{
	if (amdgv_diag_data_host_driver_collect_gen_info(adapt, file_data) != 0)
		AMDGV_WARN("Host Driver general info collection failed\n");

	if (amdgv_diag_data_host_driver_collect_error_log(adapt, file_data) != 0)
		AMDGV_WARN("Host Driver error dump failed\n");

	if (amdgv_diag_data_host_driver_collect_trace_log(adapt, file_data) != 0)
		AMDGV_WARN("Host Driver collect trace log failed\n");

	if (amdgv_diag_data_host_driver_collect_vbios_post_log(adapt, file_data) != 0)
		AMDGV_WARN("VBIOS POST Log not available\n");

	if (amdgv_diag_data_host_driver_collect_ih_ring_log(adapt, file_data) != 0)
		AMDGV_WARN("IH RB Log not available\n");

	return 0;
}


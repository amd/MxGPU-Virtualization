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
#include "amdgv_diag_data_gen_info.h"
#include "amdgv_sriovmsg.h"
#include "amdgv.h"
#include "amdgv_api.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_list.h"
#include "amdgv_device.h"
#include "amdgv_sched_internal.h"
#include "amdgv_guard.h"

static const uint32_t this_block = AMDGV_MANAGEMENT_BLOCK;

struct amdgv_rd_cache_buf {
	struct amdgv_list_head head;
	mutex_t lock;
	bool init;
} amdgv_rd_cache_buf = { 0 };

static struct amdgv_diag_data_cache_buf *amdgv_diag_data_get_cache_buffer(
	uint32_t bdf, enum AMDGV_DIAG_DATA_LOG_COLLECT_TYPE type, bool creat_new)
{
	struct amdgv_diag_data_cache_buf *c_buf;

	if (!amdgv_rd_cache_buf.init)
		return NULL;

	if (type <= AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_START ||
	    type >= AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_END)
		return NULL;

	amdgv_list_for_each_entry(c_buf, &amdgv_rd_cache_buf.head,
				   struct amdgv_diag_data_cache_buf, head) {
		if (bdf == c_buf->bdf && type == c_buf->type)
			return c_buf;
	}

	c_buf = NULL;
	if (creat_new) {
		c_buf = oss_alloc_memory(sizeof(struct amdgv_diag_data_cache_buf));
		if (!c_buf)
			return NULL;
		c_buf->bdf = bdf;
		c_buf->type = type;
		c_buf->used_size = 0;
		AMDGV_INIT_LIST_HEAD(&c_buf->head);
		oss_mutex_lock(amdgv_rd_cache_buf.lock);
		amdgv_list_add_tail(&c_buf->head, &amdgv_rd_cache_buf.head);
		oss_mutex_unlock(amdgv_rd_cache_buf.lock);
	}
	return c_buf;
}

static void
amdgv_diag_data_free_cache_buffer(struct amdgv_diag_data_cache_buf *cache_buf)
{
	if (!amdgv_rd_cache_buf.init || !cache_buf)
		return;

	oss_mutex_lock(amdgv_rd_cache_buf.lock);
	amdgv_list_del(&cache_buf->head);
	oss_mutex_unlock(amdgv_rd_cache_buf.lock);
	oss_free_memory(cache_buf);
}

static void
amdgv_diag_data_block_version(struct amdgv_adapter *adapt,
				 struct amdgv_diag_data_block_header *block_header)
{
	bool support;

	switch (block_header->block_id) {
	case AMDGV_DIAG_DATA_BLOCK_ID_HOST_REGISTERS_DUMP:
	case AMDGV_DIAG_DATA_BLOCK_ID_HOST_TRACE_LOG:
		block_header->version = AMDGV_DIAG_DATA_VERSION;
#ifdef TRACE_LOG_MD5_0
		*(uint64_t *)(&block_header->md5[0]) = TRACE_LOG_MD5_0;
#endif
#ifdef TRACE_LOG_MD5_1
		*(uint64_t *)(&block_header->md5[8]) = TRACE_LOG_MD5_0;
#endif
		break;
	case AMDGV_DIAG_DATA_BLOCK_ID_HOST_GENERAL_INFO:
		block_header->version = AMDGV_RD_HOST_GEN_INFORMATION_VERSION;
#ifdef GEN_INFO_MD5_0
		*(uint64_t *)(&block_header->md5[0]) = GEN_INFO_MD5_0;
#endif
#ifdef GEN_INFO_MD5_1
		*(uint64_t *)(&block_header->md5[8]) = GEN_INFO_MD5_1;
#endif
		break;
	case AMDGV_DIAG_DATA_BLOCK_ID_HOST_ERROR_DUMP:
		block_header->version = AMDGV_RD_HOST_ERROR_DUMP_VERSION;
#ifdef ERROR_LOG_MD5_0
		*(uint64_t *)(&block_header->md5[0]) = ERROR_LOG_MD5_0;
#endif
#ifdef ERROR_LOG_MD5_1
		*(uint64_t *)(&block_header->md5[8]) = ERROR_LOG_MD5_1;
#endif
		break;
	case AMDGV_DIAG_DATA_BLOCK_ID_PSP_SNAPSHOT_DUMP:
	case AMDGV_DIAG_DATA_BLOCK_ID_PSP_TRACE_LOG_DUMP:
		support = adapt->psp.fw_id_support(AMDGV_FIRMWARE_ID__PSP_SYS);
		if (support)
			block_header->version = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SYS];
		break;
	case AMDGV_DIAG_DATA_BLOCK_ID_HOST_FTRACE_LOG:
	case AMDGV_DIAG_DATA_BLOCK_ID_HOST_IH_RING_DUMP:
	case AMDGV_DIAG_DATA_BLOCK_ID_HOST_POST_VBIOS_LOG:
	default:
		block_header->version = 0;
		break;
	}
}

int amdgv_diag_data_gen_blk_file_hdr(struct amdgv_adapter *adapt,
					struct amdgv_diag_data_file_info *file_data,
					uint32_t block_id, void *data, uint32_t data_len,
					uint32_t flag)
{
	uint32_t blk_hdr_size;
	struct amdgv_diag_data_block_header *block_header;

	/* Check if we have some data */
	if (!data_len) {
		AMDGV_WARN("No data available in debug memory block\n");
		return AMDGV_FAILURE;
	}

	blk_hdr_size = sizeof(struct amdgv_diag_data_block_header);

	/* Check for size overflow */
	if (data_len > (data_len + blk_hdr_size)) {
		AMDGV_WARN("Overflow detected in Used size of the block\n");
		return AMDGV_FAILURE;
	}

	/* Check remainder of provided memory big enough to hold this block */
	if ((data_len + blk_hdr_size) > (file_data->size - file_data->cur_offset)) {
		AMDGV_WARN("Not enough memory provided to copy block\n");
		return AMDGV_FAILURE;
	}

	/* Generate the block header */
	block_header = (struct amdgv_diag_data_block_header *)((uint8_t *)file_data->buff +
								  file_data->cur_offset);
	oss_memset(block_header, 0, blk_hdr_size);
	block_header->block_signature = AMDGV_DIAG_DATA_BLOCK_SIGNATURE;
	/* Block ID */
	block_header->block_id = block_id;
	/* Get the version and md5 based on block id */
	amdgv_diag_data_block_version(adapt, block_header);
	/* Update the block flags */
	block_header->blk_flags = flag;
	/* GPU and CPU timetamp */
	if (adapt->diag_data.get_gpu_ref_timestamp && adapt->status == AMDGV_STATUS_HW_INIT)
		block_header->timestamp = adapt->diag_data.get_gpu_ref_timestamp(adapt);
	block_header->cpu_timestamp = oss_get_time_stamp();
	/* Used size of the block */
	block_header->block_size = data_len;
	/* Next block offset */
	block_header->next_block_offset =
		file_data->cur_offset + block_header->block_size + blk_hdr_size;
	/* Checksum */
	if (block_header->checksum == 0 && data != NULL)
		block_header->checksum = amd_sriov_msg_checksum(data, data_len, 0, 0);

	/* Update the last block */
	file_data->last_blk_hdr = block_header;
	file_data->cur_offset += blk_hdr_size;

	return 0;
}

int amdgv_diag_data_host_collect_reg_dump(struct amdgv_adapter *adapt,
		struct amdgv_diag_data_dump_reg *dbg_regs, uint32_t regs_count,
		struct amdgv_diag_data_file_info *file_data)
{
	int i, j;
	uint32_t used_size = 0;
	uint32_t reg, value;
	uint32_t reg_first_inst = 0;
	struct amdgv_diag_data_reg_entry *reg_entry = NULL;
	int reg_entry_len = sizeof(struct amdgv_diag_data_reg_entry);
	void *reg_blk_data = NULL;

	if (!dbg_regs) {
		AMDGV_WARN("Empty list for host driver registers\n");
		return 0;
	}

	/* Generate the block header
	 * In addition to generating block header log
	 * the API will also check if the user buffer has
	 * enough space to hold block header and block data
	*/
	used_size = reg_entry_len * regs_count;
	if (amdgv_diag_data_gen_blk_file_hdr(adapt, file_data,
						AMDGV_DIAG_DATA_BLOCK_ID_HOST_REGISTERS_DUMP,
						reg_blk_data, used_size, 0) != 0) {
		AMDGV_WARN("Unable to generate block header for regs dump\n");
		return AMDGV_FAILURE;
	}

	/* Block data starts after header */
	reg_blk_data = (uint8_t *)file_data->buff + file_data->cur_offset;
	reg_entry = reg_blk_data;
	used_size = 0;
	for (i = 0; i < regs_count; i++) {
		for (j = 0; j <= dbg_regs[i].max_inst; j++) {

			/* If hwid is greater than HWIP_MAX means the offset is real address */
			if (dbg_regs[i].hwid < MAX_HWIP) {
				if (!adapt->reg_offset[dbg_regs[i].hwid][j])
					break;
				reg = adapt->reg_offset[dbg_regs[i].hwid][j][dbg_regs[i].seg] +
					dbg_regs[i].offset;
			} else {
				reg = dbg_regs[i].offset / 4;
			}
			if (dbg_regs[i].cross_aid) {
				if (!adapt->diag_data.read_cross_aid_reg)
					break;
				value = adapt->diag_data.read_cross_aid_reg(adapt, reg, j);
				reg <<= 2;
			} else {
				if ((dbg_regs[i].dir.reg_index == 0) &&
					(dbg_regs[i].dir.reg_data == 0)) {
					value = RREG32(reg);
					reg <<= 2;
				} else {
					reg <<= 2;
					WREG32(dbg_regs[i].dir.reg_index, reg);
					if (dbg_regs[i].dir.reg_index_hi != 0)
						WREG32(dbg_regs[i].dir.reg_index_hi, 0);
					value = RREG32(dbg_regs[i].dir.reg_data);
				}
			}
			if (j == 0)
				reg_first_inst = reg;

			reg_entry->reg_addr = reg_first_inst;
			reg_entry->reg_val = value;
			reg_entry->inst = j;
			reg_entry++;
			used_size += reg_entry_len;
		}
	}
	/* Generate checksum */
	file_data->last_blk_hdr->checksum =
		amd_sriov_msg_checksum(reg_blk_data, used_size, 0, 0);
	file_data->last_blk_hdr->version = AMDGV_DIAG_DATA_REG_DUMP_BLOCK_VERSION;
	file_data->last_blk_hdr->block_size = used_size;
	file_data->cur_offset += used_size;
	file_data->last_blk_hdr->next_block_offset = file_data->cur_offset;

	AMDGV_INFO("Register dump of size:%d added\n", used_size);

	return 0;
}

#ifndef EXCLUDE_FTRACE
void amdgv_diag_data_exclude_list(char **call_trace_exclude_list[],
				     uint32_t *call_trace_exclude_len)
{
	*call_trace_exclude_list = host_driver_call_trace_exclude_list;
	*call_trace_exclude_len = host_driver_call_trace_exclude_list_len;
}

static int amdgv_diag_data_host_driver_call_trace_log(
	struct amdgv_adapter *adapt, struct amdgv_diag_data_file_info *file_data)
{
	uint32_t used_size = 0;
	void *call_trace_data = NULL;
	uint32_t blk_hdr_size = sizeof(struct amdgv_diag_data_block_header);
	int buf_size = file_data->size - file_data->cur_offset;

	if (buf_size < blk_hdr_size) {
		AMDGV_WARN("Memory not enough to hold call_trace blk header\n");
		return AMDGV_FAILURE;
	}

	/* Leave room for header to be filled in later */
	call_trace_data = (uint8_t *)file_data->buff + (file_data->cur_offset + blk_hdr_size);
	buf_size -= blk_hdr_size;

	/* Get the call_trace data */
	used_size = oss_copy_call_trace_buffer(call_trace_data, buf_size);
	if (used_size == 0) {
		AMDGV_WARN("Can't get the call_trace buffer\n");
		return AMDGV_FAILURE;
	}

	/* Generate the block header
	 * In addition to generating header log
	 * the API will also check if the user buffer has
	 * enough space to hold block header and block data
	*/
	if (amdgv_diag_data_gen_blk_file_hdr(adapt, file_data,
						AMDGV_DIAG_DATA_BLOCK_ID_HOST_FTRACE_LOG,
						call_trace_data, used_size, 0) != 0) {
		AMDGV_WARN("Unable to generate block header for call_trace\n");
		return AMDGV_FAILURE;
	}

	/* Update the offset to reflect data which is already copied */
	file_data->cur_offset += used_size;

	AMDGV_INFO("call_trace log of size:%d added\n", used_size);
	return 0;
}
#endif /* #ifndef EXCLUDE_FTRACE */

int amdgv_diag_data_add_blk(struct amdgv_adapter *adapt,
				    struct amdgv_diag_data_mem_block *mem_blk,
				    struct amdgv_diag_data_file_info *file_data,
				    uint32_t used_size, uint32_t flag)
{
	/* Generate the header */
	if (amdgv_diag_data_gen_blk_file_hdr(adapt, file_data, mem_blk->block_id,
						mem_blk->vaddr, used_size, flag) != 0)
		return AMDGV_FAILURE;
	mem_blk->used_size = used_size;

	/* Copy block Data */
	oss_memcpy(((uint8_t *)file_data->buff) + file_data->cur_offset, mem_blk->vaddr,
		   mem_blk->used_size);
	file_data->cur_offset += mem_blk->used_size;

	return 0;
}

int amdgv_diag_data_add_blk_without_hdr(struct amdgv_adapter *adapt,
					struct amdgv_diag_data_mem_block *mem_blk,
					struct amdgv_diag_data_file_info *file_data,
					uint32_t used_size, bool gen_cksum)
{
	struct amdgv_diag_data_block_header *block_header;
	void *cksum_buff;

	/* Check if we have some data */
	if (!used_size) {
		AMDGV_WARN("No data available in debug memory block\n");
		return AMDGV_FAILURE;
	}

	/* Check remainder of provided memory big enough to hold this block */
	if (used_size > (file_data->size - file_data->cur_offset)) {
		AMDGV_WARN("Not enough memory provided to copy block data only\n");
		return AMDGV_FAILURE;
	}

	/* Copy block Data */
	oss_memcpy(((uint8_t *)file_data->buff) + file_data->cur_offset, mem_blk->vaddr,
		   used_size);
	file_data->cur_offset += used_size;

	/* Update block header of last block */
	block_header = file_data->last_blk_hdr;
	block_header->block_size += used_size;
	block_header->next_block_offset += used_size;

	/* Update checksum if indicated */
	if (gen_cksum) {
		cksum_buff = (((uint8_t *)file_data->buff) +
				(file_data->cur_offset - block_header->block_size));
		block_header->checksum = amd_sriov_msg_checksum(cksum_buff,
								block_header->block_size, 0, 0);
	}

	return 0;
}

void amdgv_diag_data_copy(struct amdgv_adapter *adapt,
				  struct amdgv_diag_data_file_info *file_data,
				  uint32_t entry_size, uint32_t log_entries, uint64_t w_count,
				  uint32_t keep_entries, void *blk_vaddr)
{
	uint8_t *dst_addr;
	uint32_t to_copy;
	uint32_t w_idx;

	if (blk_vaddr == NULL || file_data == NULL || file_data->buff == NULL)
		return;

	if (w_count == 0)
		return;

	/* Copy the contents as is if entries have not overflowed */
	if (w_count <= log_entries) {
		to_copy = w_count * entry_size;
		/* Copy block Data */
		oss_memcpy(((uint8_t *)file_data->buff) + file_data->cur_offset, blk_vaddr,
			   to_copy);
	} else {
		/* Get the write index */
		if (keep_entries) {
			w_idx = AMDGV_DIAG_DATA_RB_KEEP_INDEX(w_count, log_entries,
								 keep_entries);

			/* Copy first KEEP entries */
			to_copy = entry_size * keep_entries;
			oss_memcpy(((uint8_t *)file_data->buff) + file_data->cur_offset,
				   blk_vaddr, to_copy);
			file_data->cur_offset += to_copy;
		} else {
			w_idx = AMDGV_DIAG_DATA_RB_INDEX(w_count, log_entries);
		}

		/* Copy the oldest entries i.e, w_count to size */
		dst_addr = (uint8_t *)blk_vaddr + (w_idx * entry_size);
		to_copy = (log_entries - w_idx) * entry_size;
		oss_memcpy(((uint8_t *)file_data->buff) + file_data->cur_offset, dst_addr,
			   to_copy);
		file_data->cur_offset += to_copy;

		/* Copy rest i.e., between first entry/keep and wr_count */
		dst_addr = (uint8_t *)blk_vaddr + (keep_entries * entry_size);
		to_copy = (w_idx - keep_entries) * entry_size;
		oss_memcpy(((uint8_t *)file_data->buff) + file_data->cur_offset,
			   (void *)dst_addr, to_copy);
	}
	file_data->cur_offset += to_copy;
}

int amdgv_diag_data_add_ring_buffer(
	struct amdgv_adapter *adapt, struct amdgv_diag_data_file_info *file_data,
	struct amdgv_diag_data_mem_block *mem_blk, uint32_t entry_size,
	uint32_t log_entries, uint64_t w_count, uint32_t keep_entries, uint32_t flag)
{
	uint32_t used_size;

	if (file_data == NULL || file_data->buff == NULL)
		return AMDGV_FAILURE;

	if (w_count == 0)
		return 0;

	/* Update used size of the block */
	if (w_count <= log_entries)
		used_size = w_count * entry_size;
	else
		used_size = log_entries * entry_size;

	if (amdgv_diag_data_gen_blk_file_hdr(adapt, file_data, mem_blk->block_id,
						mem_blk->vaddr, used_size, flag) != 0) {
		AMDGV_WARN("Unable to copy ring buffer to memory\n");
		return AMDGV_FAILURE;
	}
	mem_blk->used_size = used_size;

	/* Copy data to the file */
	amdgv_diag_data_copy(adapt, file_data, entry_size, log_entries, w_count,
				     keep_entries, mem_blk->vaddr);

	return 0;
}

static void
amdgv_diag_data_update_cache_block_header(uint32_t bdf, uint32_t offset,
					     struct amdgv_diag_data_cache_buf *c_buf)
{
	uint8_t *blks = c_buf->buf;
	struct amdgv_diag_data_block_header *b_hdr;

	while (blks < ((uint8_t *)c_buf + c_buf->used_size)) {
		b_hdr = (struct amdgv_diag_data_block_header *)blks;
		if (b_hdr->block_signature == AMDGV_DIAG_DATA_BLOCK_SIGNATURE) {
			blks = (uint8_t *)c_buf + b_hdr->next_block_offset;
			b_hdr->next_block_offset += offset;
			b_hdr->blk_flags |= AMDGV_DIAG_DATA_COLLET_TIMING_FLAG(c_buf->type);
		} else {
			break;
		}
	}
}

static int
amdgv_diag_data_collect_cache(struct amdgv_adapter *adapt, void *buff,
					uint32_t size, uint32_t idx_vf,
					enum AMDGV_DIAG_DATA_LOG_COLLECT_TYPE collect_type)
{
	struct amdgv_diag_data_file_info file_data = { 0 };
	uint32_t world_switch_mask = 0;

	if (buff == NULL || size == 0)
		return 0;

	file_data.buff = (uint8_t *)buff;
	file_data.cur_offset = 0;
	file_data.size = size;
	file_data.collect_type = collect_type;
	file_data.idx_vf = idx_vf;

	/* Suspend Event process */
	if (!oss_is_current_running_thread(adapt->sched.event_thread)) {
		if (amdgv_sched_queue_event_process_suspend(adapt) != 0) {
			AMDGV_WARN("Unable to suspend event process\n");
			return 0;
		}
	}

	/* Stop World Switch if it is not already stopped */
	world_switch_mask = amdgv_sched_world_switch_list_active(adapt);
	if (world_switch_mask)
		amdgv_sched_stop_all(adapt);

	/* Call collect data for asic */
	if (adapt->diag_data.collect_data)
		adapt->diag_data.collect_data(adapt, &file_data);

#ifndef EXCLUDE_FTRACE
	/* Call collect data for call_trace */
	if (amdgv_diag_data_host_driver_call_trace_log(adapt, &file_data) != 0)
		AMDGV_WARN("Host Driver collect call_trace log failed\n");
#endif

	/* Call collect data for libgv */
	if (amdgv_diag_data_host_driver_collect(adapt, &file_data) != 0)
		AMDGV_WARN("Error in libgv debug data collection\n");

	if (file_data.cur_offset == 0)
		AMDGV_WARN("diagnosis data is empty\n");

	/* Restart WS if it was stopped by collect data */
	if (world_switch_mask)
		amdgv_sched_world_switch_sched_mask_start(adapt, world_switch_mask);

	/* Resume Event process */
	if (!oss_is_current_running_thread(adapt->sched.event_thread))
		amdgv_sched_queue_event_process_resume(adapt);

	return file_data.cur_offset;
}

int amdgv_diag_data_cache_dump(struct amdgv_adapter *adapt, uint32_t idx_vf,
					enum AMDGV_DIAG_DATA_LOG_COLLECT_TYPE type)
{
	struct amdgv_diag_data_cache_buf *c_buf;

	if (adapt->flags & AMDGV_FLAG_SKIP_DIAG_DATA) {
		AMDGV_INFO("skip diag_data_cache_dump when AMDGV_FLAG_SKIP_DIAG_DATA flag is set.\n");
		return 0;
	}

	if (adapt->ecc.fatal_error) {
		AMDGV_WARN("skip diag_data_cache_dump during fatal error.\n");
		return 0;
	}

	c_buf = amdgv_diag_data_get_cache_buffer(adapt->bdf, type, true);

	if (!c_buf)
		return 0;

	if (c_buf->used_size) {
		AMDGV_WARN("Cache logs hasn't been copied to user memory\n");
		return 0;
	}

	c_buf->dev_id = adapt->dev_id;
	c_buf->used_size = amdgv_diag_data_collect_cache(
		adapt, c_buf->buf, AMDGV_DIAG_DATA_CRIT_ERR_LOG_SIZE, idx_vf, type);

	if (!c_buf->used_size) {
		AMDGV_WARN("Unable to collect data for diagnosis data\n");
		amdgv_diag_data_free_cache_buffer(c_buf);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int amdgv_diag_data_copy_cache_logs(uint32_t bdf, void *buff, uint32_t size,
					      uint32_t *dev_id, uint32_t offset)
{
	struct amdgv_diag_data_cache_buf *c_buf;
	int type;
	int data_copied = offset;

	for (type = AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_START + 1;
	     type < AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_END; type++) {
		c_buf = amdgv_diag_data_get_cache_buffer(bdf, type, false);
		if (c_buf == NULL)
			continue;

		if ((data_copied + c_buf->used_size) < size) {
			/* Update block offsets and cache_index in blk hdr */
			amdgv_diag_data_update_cache_block_header(bdf, data_copied, c_buf);

			/* Copy data */
			oss_memcpy((uint8_t *)buff + data_copied, c_buf->buf,
				   c_buf->used_size);
			data_copied += c_buf->used_size;
			if (dev_id)
				*dev_id = c_buf->dev_id;
			amdgv_diag_data_free_cache_buffer(c_buf);
		}
	}

	/* Return the size of actual blocks data copied */
	return (data_copied - offset);
}

int amdgv_diag_data_collect(struct amdgv_adapter *adapt, uint32_t bdf, void *buff,
				    uint32_t size)
{
	uint32_t ret = 0;
	uint32_t file_hdr_size;
	uint32_t dev_id = 0;
	struct amdgv_diag_data_header file_hdr = { 0 };

	if (buff == NULL || size == 0)
		return 0;

	if (adapt == NULL && bdf == 0)
		return 0;

	/* Update the file header */
	file_hdr_size = sizeof(struct amdgv_diag_data_header);
	file_hdr.signature = AMDGV_DIAG_DATA_HEADER_SIGNATURE;
	file_hdr.version = AMDGV_DIAG_DATA_VERSION;
	file_hdr.datetime = oss_get_utc_time_stamp();
	file_hdr.next_block_offset = file_hdr_size;
	file_hdr.bus_id = bdf ? bdf : adapt->bdf;

	/* Get the cache debug logging if not started from error */
	file_hdr.size =
		amdgv_diag_data_copy_cache_logs(bdf, buff, size, &dev_id, file_hdr_size);
	file_hdr.asic_id = dev_id;

	/* Copy Activity buffer to cache logs only if gpu is initialized*/
	if (file_hdr.size == 0) {
		if (adapt) {
			AMDGV_INFO("Empty cache. Copy activity buf to cache\n");
			if (amdgv_diag_data_cache_dump(adapt, AMDGV_PF_IDX,
				    AMDGV_DIAG_DATA_LOG_COLLECT_ACTIVITY_BUFFER)) {
				AMDGV_WARN("Can't copy activity buf to cache\n");
			} else {
				file_hdr.size = amdgv_diag_data_copy_cache_logs(
					bdf, buff, size, &dev_id, file_hdr_size);
				file_hdr.asic_id = dev_id;
			}
		}
	}

	if (file_hdr.size) {
		/* Copy the header at start */
		oss_memcpy(buff, &file_hdr, file_hdr_size);
		ret = file_hdr.size + file_hdr_size;
	}
	return ret;
}

static int amdgv_diag_data_mem_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	/* Allocate the pool of memory */
	if (oss_alloc_dma_mem(adapt->dev, AMDGV_DIAG_DATA_ASIC_MEM_SIZE,
			      OSS_DMA_MEM_CACHEABLE, &adapt->diag_data.asic_sys_mem) != 0) {
		adapt->diag_data.asic_buff.vaddr = NULL;
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_DMA_MEM_FAIL,
				AMDGV_DIAG_DATA_ASIC_MEM_SIZE);
	} else {
		/* Assign asic buffer, 1MB of contig memory */
		adapt->diag_data.asic_buff.vaddr = adapt->diag_data.asic_sys_mem.va_ptr;
		adapt->diag_data.asic_buff.bus_addr =
			adapt->diag_data.asic_sys_mem.bus_addr;
	}

	/* Assign host driver buffer, 1M of the memory buffer */
	adapt->diag_data.host_drv_buff.bus_addr = 0;
	adapt->diag_data.host_drv_buff.vaddr =
		oss_alloc_memory(AMDGV_DIAG_DATA_HOST_DRV_MEM_SIZE);
	if (adapt->diag_data.host_drv_buff.vaddr == NULL) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				AMDGV_DIAG_DATA_HOST_DRV_MEM_SIZE);

		/* Return failure if both memory alloc fails */
		if (adapt->diag_data.asic_buff.vaddr == NULL)
			ret = AMDGV_FAILURE;
	}

	return ret;
}

static void amdgv_diag_data_mem_fini(struct amdgv_adapter *adapt)
{
	if (adapt->diag_data.asic_sys_mem.handle) {
		oss_free_dma_mem(adapt->diag_data.asic_sys_mem.handle);
		adapt->diag_data.asic_sys_mem.handle = NULL;
		adapt->diag_data.asic_buff.vaddr = NULL;
	}
	if (adapt->diag_data.host_drv_buff.vaddr) {
		oss_free_memory(adapt->diag_data.host_drv_buff.vaddr);
		adapt->diag_data.host_drv_buff.vaddr = NULL;
	}
}

/* Dummy CB for gpu time stamp, until the hw initializes */
static uint64_t amdgv_diag_data_get_gpu_ref_timestamp(struct amdgv_adapter *adapt)
{
	return 0;
}

int amdgv_diag_data_init(struct amdgv_adapter *adapt)
{
	adapt->diag_data.get_gpu_ref_timestamp = amdgv_diag_data_get_gpu_ref_timestamp;

	if (amdgv_diag_data_mem_init(adapt) != 0) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_DIAG_DATA_MEM_REQ_FAIL,
				AMDGV_DIAG_DATA_MEM_SIZE);
		return AMDGV_FAILURE;
	}

	/* Init the host driver resources */
	if (amdgv_diag_data_host_driver_init(adapt) != 0) {
		/* amdgv_device_internal_init calls amdgv_diag_data_fini */
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_DIAG_DATA_INIT_FAIL, 0);
		return AMDGV_FAILURE;
	}

	return 0;
}

void amdgv_diag_data_fini(struct amdgv_adapter *adapt)
{
	/* Clean all host driver related resources */
	amdgv_diag_data_host_driver_fini(adapt);
	amdgv_diag_data_mem_fini(adapt);
}

void amdgv_diag_data_cache_buf_init(void)
{
	AMDGV_INIT_LIST_HEAD(&amdgv_rd_cache_buf.head);
	amdgv_rd_cache_buf.lock = oss_mutex_init();
	if (amdgv_rd_cache_buf.lock)
		amdgv_rd_cache_buf.init = true;
	else
		amdgv_rd_cache_buf.init = false;
}

void amdgv_diag_data_cache_buf_fini(void)
{
	struct amdgv_diag_data_cache_buf *c_buf, *t_buf;

	if (!amdgv_rd_cache_buf.init)
		return;

	amdgv_list_for_each_entry_safe(c_buf, t_buf, &amdgv_rd_cache_buf.head,
					struct amdgv_diag_data_cache_buf, head) {
		amdgv_diag_data_free_cache_buffer(c_buf);
	}
	oss_mutex_fini(amdgv_rd_cache_buf.lock);
	amdgv_rd_cache_buf.init = false;
}


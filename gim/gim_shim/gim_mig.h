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
 * THE SOFTWARE
 */

#ifndef GIM_MIG_H
#define GIM_MIG_H

#if defined(SUPPORT_LIVE_MIGRATION)
#include <linux/vfio.h>
#include <linux/vfio_pci_core.h>

#include "gim_vfio_pci.h"

struct gim_mig_device;

#define GIM_MIG_COPY_MASK_CTX \
		(0x1 << AMDGV_MIGRATION_CONTENT_CTX)
#define GIM_MIG_COPY_MASK_VF_HW_STATIC_DATA \
		(0x1 << AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA)
#define GIM_MIG_COPY_MASK_VF_HW_DYNAMIC_DATA \
		(0x1 << AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA)
#define GIM_MIG_COPY_MASK_VF_FB_DATA \
		(0x1 << AMDGV_MIGRATION_CONTENT_VF_FB_DATA)
#define GIM_MIG_COPY_MASK_VF_FB_BITMAP \
		(0x1 << AMDGV_MIGRATION_CONTENT_VF_FB_BITMAP)

enum gim_mig_thread_state {
	/* Thread is actively copying data*/
	GIM_MIG_THREAD_RUNNING = 0,
	/* Thread is paused, waiting for data to become available*/
	GIM_MIG_THREAD_PAUSED = 1,
	/* Thread encountered an error and is in abnormal state, migration should fail */
	GIM_MIG_THREAD_ABNORMAL = 2,
};

struct gim_mig_msg_section {
	uint64_t valid:1; // 1 if this section is valid
	/* the granularity of the bitmap,
	 * valid when section is bitmap
	 * The size of one bit represents is 2^granularity * 4KB
	 */
	uint64_t granularity:4;
	uint64_t length:59;//length of the data
	uint64_t offset;//offset in the package
};

struct gim_mig_msg_header {
	uint32_t version; // version of the data package
	uint64_t length;//size of the msg and dirty fb
	uint64_t seq;//sequence number of the data package
	struct gim_mig_msg_section sec[AMDGV_MIGRATION_CONTENT_MAX];//sections in the data package
	uint32_t checksum;//checksum of the data package
};

struct gim_mig_file {
	struct file *filp; // data_fd used by Qemu to read/write data
	struct mutex lock; // protect the read/write operations of the filp
	bool enabled;
	uint64_t seq; // current sequence number of the data package/msg
	bool is_target; // false: on source, true: on target
	uint64_t total_bytes_fd; // how many bytes need to be copied in one loop, fd level
	ssize_t left_bytes_fd; // how many bytes left to be copied to/from ringbuffer in one loop, fd level
	uint64_t copied_size_thread; // how many bytes have been copied to/from ringbuffer, thread level
	uint32_t next_copy_mask; // mask indicating which section to copy
	uint32_t copied_mask; // mask indicating which section has been copied
	void *shadow_bitmap; // shadow bitmap of the FB
	uint64_t shadow_fb_len; //length of the dirty FB in shadow_bitmap
	struct mutex shadow_lock; // protect the shadow bitmap and shadow_fb_len

	struct {
		char *va_ptr; // virtual address of the msg buffer
		uint32_t size; // size of the msg buffer
		void *bitmap; // pointer to the dirty page bitmap in msg.va_ptr
		uint64_t bit_pos; // the processed position in bitmap
		/* if the len of the incoming buffer does not aligned to page size of the bitmap,
		 * @off represents the copied position in one page
		 */
		uint32_t off;
	} msg; // buffer for ctrl, FW static/dynamic data, CTX, bitmap

	struct {
		size_t rptr, wptr; // read/write pointer of the ring buffer
		uint64_t buf_mask; // mask of the ring buffer
		struct task_struct *thread; // thread to copy data from VRAM to ring buffer, or vice versa
		/* signal to copy_thread
		 * to copy data into ring buffer on source,
		 * to copy data from ring buffer on destination.
		 */
		wait_queue_head_t copy_thread_wq;
		enum gim_mig_thread_state state; // state of the thread
		uint32_t size; // size of the ring buffer
		void *va_ptr; // virtual address of the ring buffer
		uint64_t gpu_addr; // GPU address of the ring buffer
		bool enabled;
		struct oss_dma_mem_info *buf; // handle to the ring buffer
	} ring; // buffer ring to copy data
};

struct gim_mig_vf_ctx {
	struct gim_mig_device *gdev;
	enum vfio_device_mig_state curr_state; // current state of migration
	/*
	 * 1. hold the lock when accessing/changing the device state
	 * 2. Due to the resource is allocated during the device state changing,
	 *    the lock is also used to protect the resource allocation and deallocation
	 * 3. the control of which data section(in msg) to copy is also protected by this lock
	 */
	struct mutex mig_state_lock;
	struct gim_mig_file migf; // a structure on source/destination to manage data copy
	uint64_t fb_size;// FB size
	uint32_t fb_bitmap_bits; // number of bits for the VF FB
	uint32_t fb_bitmap_size; // size of the VF FB bitmap
	int vf_idx;
	struct amdgv_migration_ctx ctx;
	int vf_sched_state;
	bool inited;
};

struct gim_mig_device {
	struct gim_dev_data *pf_data;
	unsigned int mig_flags;
	struct gim_mig_vf_ctx vf_ctx[AMDGV_MAX_VF_NUM];
	uint64_t fw_s_size;// size of fw static data
	uint64_t fw_d_size;// size of fw dynamic data
	uint32_t fb_page_size;// page size of the fb
};

#define GIM_MIG_RING_BUF_SIZE (0x1 << 25)
#define GIM_MIG_FB_PAGE_SIZE (gdev->fb_page_size)
#define GIM_MIG_BITMAP_OFFSET \
		(sizeof(struct gim_mig_msg_header) + \
		 sizeof(struct amdgv_migration_ctx) + \
		 gdev->fw_d_size + gdev->fw_s_size)

#define GIM_MIG_VF_FB_SIZE(vf_idx) (gdev->vf_ctx[vf_idx].fb_size)
#define GIM_MIG_VF_FB_BITMAP_BITS(vf_idx) \
		(GIM_MIG_VF_FB_SIZE(vf_idx) / GIM_MIG_FB_PAGE_SIZE)
#define GIM_MIG_VF_FB_BITMAP_SIZE(vf_idx) \
		(roundup(GIM_MIG_VF_FB_BITMAP_BITS(vf_idx) >> 3, PAGE_SIZE))
/* Align the size of msg buffer to simplify the calculation when put it into ring buffer */
#define  GIM_MIG_VF_MSG_BUF_SIZE(vf_idx) \
		roundup(GIM_MIG_BITMAP_OFFSET + GIM_MIG_VF_FB_BITMAP_SIZE(vf_idx), \
			GIM_MIG_FB_PAGE_SIZE)

int gim_mig_init(struct pci_dev *pdev);
void gim_mig_fini(struct pci_dev *pdev);
int gim_get_mig_info(struct device *dev, struct gim_mig_info *mig_info);
#endif
#endif
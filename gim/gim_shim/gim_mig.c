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
#if defined(SUPPORT_LIVE_MIGRATION)
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/file.h>
#include <linux/anon_inodes.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include "gim.h"
#include "gim_vfio_pci.h"
#include "gim_debug.h"

extern struct gim_error_ring_buffer *gim_error_rb;

#define MIG_VERSION (1)

#define GIM_MIG_VF_MIGRATION_INVALID (-EINVAL)
#define GIM_MIG_VF_MIGRATION_PROCESSED (1)
#define GIM_MIG_VF_MIGRATION_SKIPPED (0)
#define GIM_MIG_VF_MIGRATION_FAILED (-EFAULT)

#define dev_to_pf_data(dev) (pci_get_drvdata(pci_physfn(to_pci_dev(dev))))
#define dev_to_gdev(dev) \
		(dev_to_pf_data(dev) != NULL ? \
		((struct gim_dev_data *)dev_to_pf_data(dev))->pf_mig_dev : NULL)

#define vdev_to_vf_idx(vdev) \
	({ \
		struct gim_mig_device *gdev = dev_to_gdev(vdev->dev); \
		gdev != NULL ? gim_get_vf_idx(to_pci_dev(vdev->dev), gdev->pf_data) : -1; \
	})

#define vdev_to_gdev(vdev) dev_to_gdev(vdev->dev)

#define gdev_to_vf_ctx(gdev, idx_vf) \
	({ \
		(gdev != NULL && (idx_vf >= 0 && idx_vf < AMDGV_MAX_VF_NUM)) ? \
		 &gdev->vf_ctx[idx_vf] : NULL; \
	})

#define vdev_to_vf_ctx(vdev) gdev_to_vf_ctx(vdev_to_gdev(vdev), vdev_to_vf_idx(vdev))

#define is_ring_empty(migf) ((READ_ONCE(migf->ring.rptr) & migf->ring.buf_mask) == \
				(READ_ONCE(migf->ring.wptr) & migf->ring.buf_mask))

#define gim_mig_get_fb_dirty_size(bitmap, total_bits, page_size) \
	(((uint64_t)page_size) * bitmap_weight((unsigned long *)(bitmap), (total_bits)))

static void gim_mig_do_finish_copy(struct gim_mig_vf_ctx *vf_ctx);
static ssize_t gim_mig_ringbuffer_data_copy_user(struct gim_mig_file *migf,
					char __user *buf, size_t len);

static int gim_mig_query_dirtybit_data(struct gim_mig_device *gdev,
					    int vf_idx,
					    uint64_t offset, uint64_t size,
					    void *bitmap_addr, uint64_t bitmap_size, bool dbit_preserve)
{
	struct amdgv_query_dirty_bit_data bm_info;

	bm_info.query_fb_offset = offset;
	bm_info.query_size = size;
	bm_info.dbit_plane_data_buffer = bitmap_addr;
	bm_info.dbit_plane_data_size = bitmap_size;
	bm_info.dbit_preserve = dbit_preserve;// clear the Dbit
	bm_info.idx_vf = vf_idx;

	if (amdgv_query_dirtybit_data(gdev->pf_data->adev, &bm_info))
		return -EFAULT;

	return 0;
}

static int gim_mig_update_shadow_dirtybit(struct gim_mig_file *migf, uint64_t *shadow_dirty_fb_len)
{
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	struct gim_mig_device *gdev = vf_ctx->gdev;
	int ret = 0;

	mutex_lock(&migf->shadow_lock);
	ret = gim_mig_query_dirtybit_data(gdev, vf_ctx->vf_idx,
					0, vf_ctx->fb_size,
					migf->shadow_bitmap,
					vf_ctx->fb_bitmap_size, false);
	if (!ret) {
		migf->shadow_fb_len = gim_mig_get_fb_dirty_size(migf->shadow_bitmap,
					vf_ctx->fb_bitmap_bits, GIM_MIG_FB_PAGE_SIZE);
		*shadow_dirty_fb_len = migf->shadow_fb_len;
	}

	mutex_unlock(&migf->shadow_lock);
	return ret;
}

static uint64_t gim_mig_get_dirtybit_from_shadow(struct gim_mig_file *migf)
{
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	struct gim_mig_device *gdev = vf_ctx->gdev;
	uint64_t dirty_fb_len = 0;

	mutex_lock(&migf->shadow_lock);
	memcpy(migf->msg.bitmap, migf->shadow_bitmap, GIM_MIG_VF_FB_BITMAP_SIZE(vf_ctx->vf_idx));
	dirty_fb_len  = migf->shadow_fb_len;
	memset(migf->shadow_bitmap, 0, GIM_MIG_VF_FB_BITMAP_SIZE(vf_ctx->vf_idx));
	migf->shadow_fb_len = 0;
	mutex_unlock(&migf->shadow_lock);

	return dirty_fb_len;
}

static uint32_t gim_mig_msg_checksum(struct gim_mig_msg_header *hdr)
{
	uint32_t checksum = 0;
	uint32_t cs_off = offsetof(struct gim_mig_msg_header, checksum);
	uint32_t *p = (uint32_t *)hdr;
	int i;

	for (i = 0; i < (sizeof(struct gim_mig_msg_header) / sizeof(uint32_t)); i++) {
		if (i == (cs_off / sizeof(uint32_t)))
			continue;
		checksum += p[i];
	}

	return checksum;
}

static bool gim_mig_msg_check(struct gim_mig_file *migf)
{
	struct gim_mig_msg_header *hdr = (struct gim_mig_msg_header *)migf->msg.va_ptr;

	if (hdr->version != MIG_VERSION) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
		return false;
	}
	if (hdr->checksum != gim_mig_msg_checksum(hdr)) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
		return false;
	}

	return true;
}

static bool gim_mig_ctx_check(struct gim_mig_vf_ctx *vf_ctx, struct amdgv_migration_ctx *ctx)
{
	bool ret = true;

	if (!amdgv_compare_mig_ctx(vf_ctx->gdev->pf_data->adev, vf_ctx->vf_idx, ctx)) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
		ret = false;
	}

	return ret;
}

static int gim_mig_vf_data_import(struct gim_mig_file *migf)
{
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	struct gim_mig_device *gdev = vf_ctx->gdev;
	struct gim_mig_msg_header *hdr = (struct gim_mig_msg_header *)migf->msg.va_ptr;
	struct gim_mig_msg_section *sec;
	uint64_t offset, length;

	if (!gim_mig_msg_check(migf)) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
		return -EINVAL;
	}

	sec = &hdr->sec[AMDGV_MIGRATION_CONTENT_CTX];
	if (sec->valid) {
		offset = sec->offset;
		length = sec->length;
		if (offset + length > migf->msg.size)
			return -EINVAL;

		if (!gim_mig_ctx_check(vf_ctx, (struct amdgv_migration_ctx *)((void *)hdr + offset)))
			return -EINVAL;
		if (amdgv_migration_import(gdev->pf_data->adev, vf_ctx->vf_idx, (void *)hdr + offset,
				       AMDGV_MIGRATION_IMPORT_PHASE1_PREPARE))
			return -EFAULT;
	}
	sec = &hdr->sec[AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA];
	if (sec->valid) {
		offset = sec->offset;
		length = sec->length;
		if (offset + length > migf->msg.size)
			return -EINVAL;

		if (amdgv_migration_import(gdev->pf_data->adev, vf_ctx->vf_idx, (void *)hdr + offset,
				       AMDGV_MIGRATION_IMPORT_PHASE2_STATIC_DATA))
			return -EFAULT;
	}
	sec = &hdr->sec[AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA];
	if (sec->valid) {
		offset = sec->offset;
		length = sec->length;
		if (offset + length > migf->msg.size)
			return -EINVAL;

		if (amdgv_migration_import(gdev->pf_data->adev, vf_ctx->vf_idx, (void *)hdr + offset,
				       AMDGV_MIGRATION_IMPORT_PHASE3_DYNAMIC_DATA))
			return -EFAULT;
	}
	sec = &hdr->sec[AMDGV_MIGRATION_CONTENT_VF_FB_BITMAP];
	if (sec->valid) {
		offset = sec->offset;
		length = sec->length;
		if (offset + length > migf->msg.size)
			return -EINVAL;

		if (GIM_MIG_FB_PAGE_SIZE != ((1 << sec->granularity) << PAGE_SHIFT)) {
			gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
			return -EINVAL;
		}
		migf->msg.bitmap = ((void *)hdr) + offset;
	}

	return 0;
}

static int gim_mig_vf_data_export(struct gim_mig_file *migf)
{
	uint32_t offset = 0;
	struct gim_mig_msg_section *sec;
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	struct gim_mig_device *gdev = vf_ctx->gdev;
	struct gim_mig_msg_header *hdr = (struct gim_mig_msg_header *)migf->msg.va_ptr;

	if (hdr == NULL)
		return -ENOMEM;

	if (migf->next_copy_mask == 0)
		return 0;

	offset = sizeof(struct gim_mig_msg_header);
	if (migf->next_copy_mask & GIM_MIG_COPY_MASK_CTX) {
		sec = &hdr->sec[AMDGV_MIGRATION_CONTENT_CTX];
		sec->length = sizeof(struct amdgv_migration_ctx);
		sec->offset = offset;

		if (amdgv_get_migration_ctx(gdev->pf_data->adev, vf_ctx->vf_idx,
					(struct amdgv_migration_ctx *)(((void *)hdr) + sec->offset)))
			return -EINVAL;

		sec->valid = 1;
		migf->copied_mask |= GIM_MIG_COPY_MASK_CTX;
		migf->next_copy_mask &= ~GIM_MIG_COPY_MASK_CTX;
	}

	offset += sizeof(struct amdgv_migration_ctx);
	if (migf->next_copy_mask & GIM_MIG_COPY_MASK_VF_HW_STATIC_DATA) {
		sec = &hdr->sec[AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA];
		sec->length = gdev->fw_s_size;
		sec->offset = offset;

		if (amdgv_migration_export(gdev->pf_data->adev, vf_ctx->vf_idx, ((void *)hdr) + sec->offset,
				       AMDGV_MIGRATION_EXPORT_PHASE1_STATIC_DATA))
			return -EFAULT;

		sec->valid = 1;
		migf->copied_mask |= GIM_MIG_COPY_MASK_VF_HW_STATIC_DATA;
		migf->next_copy_mask &= ~GIM_MIG_COPY_MASK_VF_HW_STATIC_DATA;
	}

	offset += gdev->fw_s_size;
	/* Qemu read the data without querying the size in STOP_COPY */
	if (migf->next_copy_mask & GIM_MIG_COPY_MASK_VF_HW_DYNAMIC_DATA) {
		sec = &hdr->sec[AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA];
		sec->length = gdev->fw_d_size;
		sec->offset = offset;

		if (amdgv_migration_export(gdev->pf_data->adev, vf_ctx->vf_idx, ((void *)hdr) + sec->offset,
				AMDGV_MIGRATION_EXPORT_PHASE2_DYNAMIC_DATA))
			return -EFAULT;

		sec->valid = 1;
		migf->copied_mask |= GIM_MIG_COPY_MASK_VF_HW_DYNAMIC_DATA;
		migf->next_copy_mask &= ~GIM_MIG_COPY_MASK_VF_HW_DYNAMIC_DATA;
	}

	offset += gdev->fw_d_size;
	if (migf->next_copy_mask & GIM_MIG_COPY_MASK_VF_FB_BITMAP) {
		hdr->length += gim_mig_get_dirtybit_from_shadow(migf);

		sec = &hdr->sec[AMDGV_MIGRATION_CONTENT_VF_FB_BITMAP];
		sec->length = GIM_MIG_VF_FB_BITMAP_SIZE(vf_ctx->vf_idx);
		/* the position of bitmap is fixed at the end of msg_buf */
		sec->offset = GIM_MIG_BITMAP_OFFSET;
		sec->granularity = ilog2(GIM_MIG_FB_PAGE_SIZE >> PAGE_SHIFT);

		sec->valid = 1;
		offset = sec->offset + sec->length;
		migf->copied_mask |= GIM_MIG_COPY_MASK_VF_FB_BITMAP;
		migf->next_copy_mask &= ~GIM_MIG_COPY_MASK_VF_FB_BITMAP;
	}

	migf->filp->f_pos = 0;

	hdr->seq = migf->seq++;
	hdr->version = MIG_VERSION;
	hdr->length += migf->msg.size;
	hdr->checksum = gim_mig_msg_checksum(hdr);

	return 0;
}

static int __gim_mig_validate_vf_sched_state(enum amdgv_sched_state current_state,
					 enum amdgv_sched_state new_state)
{
	/*
	 * if the vf is in ACTIVE or SUSPEND state or ACTIVE -> SUSPEND, migrate the vf.
	 * if the vf is in AVAIL or UNAVAL state, finish migration and skip VF data transfer.
	 * if the vf is in FULLACCESS state or state is changed unexpectedly, fail the migration.
	 */
	switch (current_state) {
	case AMDGV_SCHED_ACTIVE:
		if (new_state == AMDGV_SCHED_SUSPEND ||
			new_state == AMDGV_SCHED_ACTIVE)
			return GIM_MIG_VF_MIGRATION_PROCESSED;
		else
			return GIM_MIG_VF_MIGRATION_FAILED;
		break;
	case AMDGV_SCHED_SUSPEND:
		if (new_state == AMDGV_SCHED_SUSPEND)
			return GIM_MIG_VF_MIGRATION_PROCESSED;
		else
			return GIM_MIG_VF_MIGRATION_FAILED;
		break;
	case AMDGV_SCHED_AVAIL:
		if (new_state == AMDGV_SCHED_AVAIL)
			return GIM_MIG_VF_MIGRATION_SKIPPED;
		else
			return GIM_MIG_VF_MIGRATION_FAILED;
		break;
	case AMDGV_SCHED_UNAVAL:
		if (new_state == AMDGV_SCHED_UNAVAL)
			return GIM_MIG_VF_MIGRATION_SKIPPED;
		else
			return GIM_MIG_VF_MIGRATION_FAILED;
		break;
	default:
		return GIM_MIG_VF_MIGRATION_FAILED;
		break;
	}
}

/*
 * Validate the VF schedule state for migration.
 *
 * Return values:
 *  GIM_MIG_VF_MIGRATION_PROCESSED (value: 1): migrate the vf
 *  GIM_MIG_VF_MIGRATION_SKIPPED (value: 0): finish migration and skip VF data transfer.
 *  negetive (-EFAULT/-ENOMEM): abort migration
 *
 * Allowed state transitions during migration:
 *  - ACTIVE  -> ACTIVE/SUSPEND
 *  - SUSPEND -> SUSPEND
 *  - AVAIL   -> AVAIL   (skip)
 *  - UNAVAL  -> UNAVAL  (skip)
 */
static int gim_mig_validate_vf_sched_state(struct gim_mig_vf_ctx *vf_ctx)
{
	union amdgv_vf_info *info;
	amdgv_dev_t adev = vf_ctx->gdev->pf_data->adev;
	int validate_result = 0;
	int ret = 0;
	info = (union amdgv_vf_info *)gim_kzalloc(sizeof(union amdgv_vf_info), GFP_KERNEL);
	if (info == NULL) {
		ret = -ENOMEM;
		goto err_exit;
	}

	if (amdgv_get_vf_info(vf_ctx->gdev->pf_data->adev, vf_ctx->vf_idx,
			      AMDGV_GET_VF_SCHED_STATE, info)) {
		ret = -EFAULT;
		gim_kfree(info);
		goto err_exit;
	}

	if (vf_ctx->vf_sched_state == GIM_MIG_VF_MIGRATION_INVALID)
		vf_ctx->vf_sched_state = info->sched.state;

	validate_result = __gim_mig_validate_vf_sched_state(
		(enum amdgv_sched_state)vf_ctx->vf_sched_state, info->sched.state);
	vf_ctx->vf_sched_state = info->sched.state;

	gim_kfree(info);

	switch (validate_result) {
	case GIM_MIG_VF_MIGRATION_FAILED:
		ret = GIM_MIG_VF_MIGRATION_FAILED;
		goto err_exit;
	case GIM_MIG_VF_MIGRATION_SKIPPED:
		return 0;
	case GIM_MIG_VF_MIGRATION_PROCESSED:
		return 1;
	default:
		ret = -EFAULT;
		goto err_exit;
	}

err_exit:
	gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
	amdgv_migration_set_abort(adev, vf_ctx->vf_idx);
	return ret;
}

static int gim_mig_update_msg_copy_info(struct gim_mig_file *migf,
			uint64_t *initial_bytes, uint64_t *dirty_bytes,
			bool is_dyna, bool is_copy)
{
	int ret;
	uint64_t shadow_dirty_fb_len = 0;
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	amdgv_dev_t adev = vf_ctx->gdev->pf_data->adev;

	ret = gim_mig_validate_vf_sched_state(vf_ctx);
	if (ret <= 0)
		return ret;

	if (!migf->enabled) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
		amdgv_migration_set_abort(adev, vf_ctx->vf_idx);
		return -ENODEV;
	}

	if (!(migf->copied_mask & GIM_MIG_COPY_MASK_CTX))
		migf->next_copy_mask |= GIM_MIG_COPY_MASK_CTX;

	if (!(migf->copied_mask & GIM_MIG_COPY_MASK_VF_HW_STATIC_DATA))
		migf->next_copy_mask |= GIM_MIG_COPY_MASK_VF_HW_STATIC_DATA;

	if (is_dyna && !(migf->copied_mask & GIM_MIG_COPY_MASK_VF_HW_DYNAMIC_DATA))
		migf->next_copy_mask |= GIM_MIG_COPY_MASK_VF_HW_DYNAMIC_DATA;

	if (gim_mig_update_shadow_dirtybit(migf, &shadow_dirty_fb_len)) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
		migf->next_copy_mask = 0;
		amdgv_migration_set_abort(adev, vf_ctx->vf_idx);
		return -EFAULT;
	}

	if (shadow_dirty_fb_len > 0)
		migf->next_copy_mask |= GIM_MIG_COPY_MASK_VF_FB_BITMAP;


	if (initial_bytes && migf->next_copy_mask != 0)
		*initial_bytes = migf->msg.size;

	if (dirty_bytes && shadow_dirty_fb_len > 0)
		*dirty_bytes = shadow_dirty_fb_len;

	return 0;
}

static uint64_t gim_mig_get_remaining_bytes(struct gim_mig_file *migf, uint64_t *initial_bytes, uint64_t *dirty_bytes)
{
	if (migf->left_bytes_fd > 0) {
		if (migf->msg.size > migf->total_bytes_fd - migf->left_bytes_fd) {
			*initial_bytes = migf->msg.size - (migf->total_bytes_fd - migf->left_bytes_fd);
			*dirty_bytes = migf->left_bytes_fd - *initial_bytes;
		} else {
			*initial_bytes = 0;
			*dirty_bytes = migf->left_bytes_fd;
		}
	} else {
		*initial_bytes = 0;
		*dirty_bytes = 0;
	}

	return *initial_bytes + *dirty_bytes;
}

static int gim_mig_get_total_bytes(struct gim_mig_file *migf, void *buf, size_t len)
{
	int length_off = offsetof(struct gim_mig_msg_header, length) + sizeof(uint64_t);
	struct gim_mig_msg_header *hdr = (struct gim_mig_msg_header *)(buf);

	if (len >= length_off) {
		migf->total_bytes_fd = hdr->length;
		gim_dbg("total_bytes_fd=0x%llx\n", migf->total_bytes_fd);
		return 0;
	}

	return -EINVAL;
}

/*
 * Wait for a period of time and decrement the timeout.
 * @migf: migration file
 * @timeout: timeout in milliseconds
 *
 * Returns -EBUSY when timeout reaches 0, 0 otherwise.
 */
static int gim_mig_wait_with_timeout(struct gim_mig_file *migf, int *timeout)
{
	int ret = 0;
	ktime_t start, end;
	s64 delta_ms;

	if (*timeout <= 0)
		return -EBUSY;

	start = ktime_get();
	msleep(1);
	end = ktime_get();

	delta_ms = ktime_ms_delta(end, start);
	if (delta_ms <= 0)
		delta_ms = 1;

	*timeout -= (int)delta_ms;

	return ret;
}

static ssize_t gim_migf_save_read(struct file *filp, char __user *buf,
				  size_t len, loff_t *pos)
{
	struct gim_mig_file *migf = filp->private_data;
	ssize_t copied_size = 0;
	ssize_t ret = 0;
	int timeout = 10000; /* 10 seconds */
	bool pending = false;
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	amdgv_dev_t adev = vf_ctx->gdev->pf_data->adev;
	bool should_abort = false;

	if (pos)
		return -ESPIPE;

	if (migf->ring.state == GIM_MIG_THREAD_ABNORMAL)
		return -EIO;

	if (amdgv_migration_query_abort(adev, vf_ctx->vf_idx, &should_abort) || should_abort) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
		return -EFAULT;
	}

	/* the FB read position */
	pos = &filp->f_pos;

	mutex_lock(&migf->lock);
	if (migf->left_bytes_fd < 0) {
		ret = -EINVAL;
		goto exit;
	}

	if (!migf->enabled) {
		ret = -ENODEV;
		goto exit;
	}

retry:
	copied_size = gim_mig_ringbuffer_data_copy_user(migf, buf, len);
	if (copied_size < 0) {
		ret = -EFAULT;
		goto exit;
	}

	/* Only the first copy in one loop need extra signal to copy_thread */
	if (copied_size == 0) {
		if (migf->next_copy_mask != 0) {
			migf->ring.state = GIM_MIG_THREAD_RUNNING;
			wake_up(&migf->ring.copy_thread_wq);
			pending = true;

			ret = gim_mig_wait_with_timeout(migf, &timeout);
			if (ret < 0)
				goto exit;

			goto retry;
		}

		/* Even after next_copy_mask is cleared by copy_thread,
		 * continue waiting until a valid data copy is performed.
		 */
		if (pending) {
			ret = gim_mig_wait_with_timeout(migf, &timeout);
			if (ret < 0)
				goto exit;

			goto retry;
		}
	}

	if (copied_size > 0) {
		*pos += copied_size;
		migf->left_bytes_fd -= copied_size;
	}

	if (migf->left_bytes_fd > 0) {
		wake_up(&migf->ring.copy_thread_wq);
		if (copied_size == 0) {
			ret = gim_mig_wait_with_timeout(migf, &timeout);
			if (ret < 0)
				goto exit;

			goto retry;
		}
	}

	/* all data has been copied in current loop */
	if (migf->left_bytes_fd == 0)
		*pos = 0;

	ret = copied_size;
exit:
	mutex_unlock(&migf->lock);
	return ret;
}

static ssize_t gim_migf_resume_write(struct file *filp, const char __user *buf,
				     size_t len, loff_t *pos)
{
	struct gim_mig_file *migf = filp->private_data;
	ssize_t ret = 0;
	ssize_t sz = 0;
	ssize_t copied_size = 0;
	int timeout = 10000; /* 10 seconds */
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	amdgv_dev_t adev = vf_ctx->gdev->pf_data->adev;
	bool should_abort = false;

	if (pos)
		return -ESPIPE;

	if (migf->ring.state == GIM_MIG_THREAD_ABNORMAL)
		return -EIO;

	if (amdgv_migration_query_abort(adev, vf_ctx->vf_idx, &should_abort) || should_abort) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
		return -EFAULT;
	}

	pos = &filp->f_pos;

	mutex_lock(&migf->lock);
	if (!migf->enabled) {
		ret = -ENODEV;
		goto exit;
	}

	while (copied_size < len) {
		sz = gim_mig_ringbuffer_data_copy_user(migf, (char __user *)buf, len - copied_size);
		if (sz < 0) {
			ret = -EFAULT;
			goto exit;
		}

		if (sz == 0) {
			wake_up(&migf->ring.copy_thread_wq);

			ret = gim_mig_wait_with_timeout(migf, &timeout);
			if (ret < 0)
				goto exit;

			continue;
		}

		wake_up(&migf->ring.copy_thread_wq);
		copied_size += sz;
		migf->left_bytes_fd -= sz;
		*pos += sz;

		if (migf->left_bytes_fd == 0)
			*pos = 0;
	}

	ret = copied_size;
exit:
	mutex_unlock(&migf->lock);
	return ret;
}

static long gim_migf_save_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct gim_mig_file *migf = filp->private_data;
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	struct vfio_precopy_info info = {0};
	uint64_t initial_bytes = 0, dirty_bytes = 0;
	int ret;

	/* only precopy supported */
	if (cmd != VFIO_MIG_GET_PRECOPY_INFO)
		return -EINVAL;

	/* Don't do any copy if PRE_COPY is not supported */
	if (!(vf_ctx->gdev->mig_flags & VFIO_MIGRATION_PRE_COPY))
		return 0;

	if (copy_from_user(&info, (void __user *)arg, sizeof(info)))
		return -EFAULT;
	if (info.argsz != sizeof(info))
		return -EINVAL;

	mutex_lock(&vf_ctx->mig_state_lock);
	/* Qemu only query the size in PRECOPY */
	if (!migf->enabled ||
	    vf_ctx->curr_state != VFIO_DEVICE_STATE_PRE_COPY) {
		mutex_unlock(&vf_ctx->mig_state_lock);
		return -EINVAL;
	}

	gim_mig_get_remaining_bytes(migf, &initial_bytes, &dirty_bytes);
	info.initial_bytes = initial_bytes;
	info.dirty_bytes = dirty_bytes;

	initial_bytes = 0;
	dirty_bytes = 0;
	ret = gim_mig_update_msg_copy_info(migf, &initial_bytes, &dirty_bytes, false, false);
	if (ret < 0) {
		/* If a non-zero value is returned, QEMU will treat the data size as 100 GB */
		ret = 0;
		goto exit;
	}

	info.initial_bytes += initial_bytes;
	info.dirty_bytes += dirty_bytes;

	ret = copy_to_user((void __user *)arg, &info, sizeof(info)) ? -EFAULT : 0;
	filp->f_pos = 0;
exit:
	mutex_unlock(&vf_ctx->mig_state_lock);

	gim_info("initial_bytes=0x%llx, dirty_bytes=0x%llx, total=0x%llx\n",
		info.initial_bytes, info.dirty_bytes, info.initial_bytes + info.dirty_bytes);
	return ret;
}

static int gim_migf_release_file(struct inode *inode, struct file *filp)
{
	struct gim_mig_file *migf = filp->private_data;
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);

	amdgv_set_vf_migration_state(vf_ctx->gdev->pf_data->adev,
				vf_ctx->vf_idx, AMDGV_MIGRATION_VF_STATE_DEFAULT);
	vf_ctx->vf_sched_state = GIM_MIG_VF_MIGRATION_INVALID;


	if (migf->is_target) {
		/* On target, the running state means the migration success, VF can continue running,
		 * otherwise, migration failed and VF should be stopped.
		 */
		if (vf_ctx->curr_state != VFIO_DEVICE_STATE_RUNNING) {
			amdgv_stop_vf(vf_ctx->gdev->pf_data->adev, vf_ctx->vf_idx);
			vf_ctx->curr_state = VFIO_DEVICE_STATE_DEFAULT_GIM;
		}
	} else {
		/* On source, the stop state means the migration success, VF should be stopped.
		 * otherwise, migration failed and VF should be resumed and continue running.
		 */
		if (vf_ctx->curr_state == VFIO_DEVICE_STATE_STOP)
			amdgv_stop_vf(vf_ctx->gdev->pf_data->adev, vf_ctx->vf_idx);
		else {
			amdgv_resume_vf(vf_ctx->gdev->pf_data->adev, vf_ctx->vf_idx);
			vf_ctx->curr_state = VFIO_DEVICE_STATE_RUNNING;
		}
	}

	mutex_lock(&vf_ctx->mig_state_lock);
	gim_mig_do_finish_copy(vf_ctx);
	mutex_unlock(&vf_ctx->mig_state_lock);

	filp->f_pos = 0;
	return 0;
}

static const struct file_operations gim_mig_save_fops = {
	.owner = THIS_MODULE,
	.read = gim_migf_save_read,
	.unlocked_ioctl = gim_migf_save_ioctl,
	.compat_ioctl = compat_ptr_ioctl,
	.release = gim_migf_release_file,
	.llseek = noop_llseek,
};

static const struct file_operations gim_mig_resume_fops = {
	.owner = THIS_MODULE,
	.write = gim_migf_resume_write,
	.release = gim_migf_release_file,
	.llseek = noop_llseek,
};

static int gim_mig_get_file(struct gim_mig_file *migf)
{
	if (!migf->is_target)
		migf->filp = anon_inode_getfile("gim_migf_save",
					 &gim_mig_save_fops,
					 migf, O_RDONLY);
	else
		migf->filp = anon_inode_getfile("gim_migf_resume",
					 &gim_mig_resume_fops,
					 migf, O_WRONLY);
	if (IS_ERR(migf->filp))
		return PTR_ERR(migf->filp);

	stream_open(migf->filp->f_inode, migf->filp);

	return 0;
}

static int gim_mig_buffer_init(struct gim_mig_vf_ctx *vf_ctx)
{
	struct gim_mig_file *migf = &vf_ctx->migf;
	struct gim_mig_device *gdev = vf_ctx->gdev;

	migf->msg.size = GIM_MIG_VF_MSG_BUF_SIZE(vf_ctx->vf_idx);
	migf->msg.va_ptr = gim_vzalloc(migf->msg.size);
	if (migf->msg.va_ptr == NULL) {
		return -ENOMEM;
	}
	migf->msg.bitmap = migf->msg.va_ptr + GIM_MIG_BITMAP_OFFSET;

	migf->shadow_bitmap = gim_vzalloc(GIM_MIG_VF_FB_BITMAP_SIZE(vf_ctx->vf_idx));
	if (migf->shadow_bitmap == NULL) {
		gim_vfree(migf->msg.va_ptr);
		return -ENOMEM;
	}

	migf->ring.size = GIM_MIG_RING_BUF_SIZE;
	migf->ring.buf = amdgv_map_sysmem(gdev->pf_data->adev,
					migf->ring.size,
					&migf->ring.va_ptr,
					&migf->ring.gpu_addr);
	if (migf->ring.buf == NULL) {
		gim_vfree(migf->msg.va_ptr);
		gim_vfree(migf->shadow_bitmap);
		return -ENOMEM;
	}

	return 0;
}

static void gim_mig_buffer_fini(struct gim_mig_file *migf)
{
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	struct gim_mig_device *gdev = vf_ctx->gdev;

	amdgv_unmap_sysmem(gdev->pf_data->adev, migf->ring.buf);
	gim_vfree(migf->shadow_bitmap);
	gim_vfree(migf->msg.va_ptr);
	migf->ring.buf = NULL;
	migf->ring.va_ptr = NULL;
	migf->ring.gpu_addr = 0;
	migf->ring.size = 0;
	migf->msg.va_ptr = NULL;
	migf->msg.size = 0;
	migf->msg.bitmap = NULL;
	migf->shadow_fb_len = 0;
}

static inline void gim_mig_clear_thread_copy_info(struct gim_mig_file *migf)
{
	migf->msg.bit_pos = 0;
	migf->msg.off = 0;
	memset(migf->msg.va_ptr, 0, migf->msg.size);
}

/* copy the data from VF FB to ring buffer on source, or vice versa */
static ssize_t gim_mig_vf_dirty_fb_copy(struct gim_mig_file *migf, uint64_t gpu_addr, size_t len, bool *is_done)
{
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	struct gim_mig_device *gdev = vf_ctx->gdev;
	uint64_t bit_next, bit_cur;
	uint32_t bit_cnt = 0;
	ssize_t copied_size = 0, left_size = len, sz = 0;
	unsigned pg_shift = __ffs(GIM_MIG_FB_PAGE_SIZE);
	uint32_t bits_cnt = left_size >> pg_shift;

	while (left_size > 0) {
		bit_cnt = 0;
		bit_cur = find_next_bit((unsigned long *)migf->msg.bitmap, vf_ctx->fb_bitmap_bits, migf->msg.bit_pos);
		if (bit_cur >= vf_ctx->fb_bitmap_bits) {
			gim_mig_clear_thread_copy_info(migf);
			if (!migf->is_target && migf->next_copy_mask == 0)
				migf->ring.state = GIM_MIG_THREAD_PAUSED;

			*is_done = true;
			return copied_size;
		}

		bit_cnt++;
		bits_cnt--;
		bit_next = bit_cur + 1;
		while (bits_cnt > 0) {
			bit_next = find_next_bit((unsigned long *)migf->msg.bitmap, vf_ctx->fb_bitmap_bits, bit_next);
			if (bit_next >= vf_ctx->fb_bitmap_bits)
				break;

			if (bit_next == (bit_cur + bit_cnt) &&
			    ((bit_cnt + 1) << pg_shift) < left_size) {
				bit_cnt++;
				bit_next++;
				bits_cnt--;
				continue;
			}
			break;
		}

		sz = bit_cnt << pg_shift;
		sz = min_t(ssize_t, left_size, sz);

		if (amdgv_vf_fb_copy(gdev->pf_data->adev, vf_ctx->vf_idx,
				     (bit_cur << pg_shift) + migf->msg.off,
				     sz, gpu_addr + copied_size, migf->is_target, NULL)) {
			gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_IMPORT_FAIL, 0);
			amdgv_migration_set_abort(gdev->pf_data->adev, vf_ctx->vf_idx);
			return -EFAULT;
		}

		bit_cnt = (migf->msg.off + sz) >> pg_shift;
		migf->msg.off = (migf->msg.off + sz) & ((1 << pg_shift) - 1);
		bitmap_clear((unsigned long *)migf->msg.bitmap, bit_cur, bit_cnt);
		migf->msg.bit_pos = bit_cur + bit_cnt;

		left_size -= sz;
		copied_size += sz;

		bit_cur = find_next_bit((unsigned long *)migf->msg.bitmap, vf_ctx->fb_bitmap_bits, migf->msg.bit_pos);
		if (bit_cur >= vf_ctx->fb_bitmap_bits) {
			gim_mig_clear_thread_copy_info(migf);
			if (!migf->is_target && migf->next_copy_mask == 0)
				migf->ring.state = GIM_MIG_THREAD_PAUSED;

			*is_done = true;
			return copied_size;
		}
	}

	return copied_size;
}

/*copy the data from migf->msg to ring buffer on source, or vice versa*/
static ssize_t gim_mig_msg_copy(struct gim_mig_file *migf, void *ring_vaddr, size_t len)
{
	ssize_t size = min_t(ssize_t, len, migf->msg.size - migf->copied_size_thread);
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	struct gim_mig_device *gdev = vf_ctx->gdev;

	if (!migf->is_target && migf->copied_size_thread == 0) {
		if (gim_mig_vf_data_export(migf)) {
			amdgv_migration_set_abort(gdev->pf_data->adev, vf_ctx->vf_idx);
			return -EFAULT;
		}
	}

	if (migf->is_target)
		memcpy(migf->msg.va_ptr + migf->copied_size_thread, ring_vaddr, size);
	else
		memcpy(ring_vaddr, migf->msg.va_ptr + migf->copied_size_thread, size);

	if (migf->is_target && (migf->copied_size_thread + size == migf->msg.size)) {
		if (gim_mig_vf_data_import(migf)) {
			amdgv_migration_set_abort(gdev->pf_data->adev, vf_ctx->vf_idx);
			return -EBUSY;
		}
	}

	return size;
}
/* copy the data from internal to ring buffer on source, or vice versa */
static ssize_t gim_mig_ringbuffer_data_copy(struct gim_mig_file *migf,
			uint32_t ring_offset, size_t len)
{
	ssize_t copied_size = 0, sz = 0;
	bool is_done = false;

	if (migf->copied_size_thread < migf->msg.size) {
		sz = gim_mig_msg_copy(migf, migf->ring.va_ptr + ring_offset, len);
		if (sz < 0)
			return -EFAULT;

		copied_size += sz;
	}

	if (sz < len) {
		sz = gim_mig_vf_dirty_fb_copy(migf,
				migf->ring.gpu_addr + ring_offset + sz, len - sz, &is_done);
		if (sz < 0)
			return -EFAULT;

		copied_size += sz;
	}

	if (is_done)
		migf->copied_size_thread = 0;
	else
		migf->copied_size_thread += copied_size;

	return copied_size;
}

static bool gim_mig_thread_check_ringbuffer_space(struct gim_mig_file *migf,
					size_t *available_space, size_t *wptr)
{
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	struct gim_mig_device *gdev = vf_ctx->gdev;
	size_t _rptr, _wptr;

	if (migf->is_target) {
		_wptr = READ_ONCE(migf->ring.rptr);
		_rptr = READ_ONCE(migf->ring.wptr);
		*available_space = (_rptr + migf->ring.size - _wptr) & migf->ring.buf_mask;
	} else {
		_rptr = READ_ONCE(migf->ring.rptr);
		_wptr = READ_ONCE(migf->ring.wptr);
		*available_space = (migf->ring.size + _rptr - _wptr
				- GIM_MIG_FB_PAGE_SIZE) & migf->ring.buf_mask;
	}

	*wptr = _wptr;

	return *available_space >= GIM_MIG_FB_PAGE_SIZE;
}

static ssize_t gim_mig_thread_ringbuffer_data_copy(struct gim_mig_file *migf,
					size_t available_space, size_t wptr)
{
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	struct gim_mig_device *gdev = vf_ctx->gdev;
	size_t wrapped_space = 0;

	wrapped_space = available_space + wptr > migf->ring.size ?
				available_space + wptr - migf->ring.size : 0;
	available_space = available_space - wrapped_space;

	/* Only process the available_space, because the copied_size(returned by
	 * gim_mig_ringbuffer_data_copy) may be less than available_space, the data
	 * at available_space - copied_size will be missed if we process the wrapped
	 * space directly.
	 *
	 * The wrapped space will be processed in next call.
	 */
	if (available_space > 0)
		return gim_mig_ringbuffer_data_copy(migf, wptr, rounddown(available_space, GIM_MIG_FB_PAGE_SIZE));

	return 0;
}

static ssize_t gim_mig_user_check_ringbuffer_space(struct gim_mig_file *migf, size_t rptr)
{
	struct gim_mig_vf_ctx *vf_ctx = container_of(migf, struct gim_mig_vf_ctx, migf);
	struct gim_mig_device *gdev = vf_ctx->gdev;
	size_t wptr;
	ssize_t available_space = 0;

	if (migf->is_target)
		wptr = READ_ONCE(migf->ring.rptr);
	else
		wptr = READ_ONCE(migf->ring.wptr);

	if (!migf->is_target)
		available_space = (wptr + migf->ring.size - rptr) & migf->ring.buf_mask;
	else
		available_space = (migf->ring.size - GIM_MIG_FB_PAGE_SIZE +
				wptr - rptr) & migf->ring.buf_mask;

	return available_space;
}

/* copy the data from userspace to ringbuffer on target, or vice versa
 *
 * @migf: the gim_mig_file struct
 * @vaddr: pointer to rptr + ringbuffer base address
 * @buf: userspace buffer
 * @sz: the size of the data to copy
 *
 * @return: the size of the data copied
 */
static ssize_t gim_mig_user_ringbuffer_do_data_copy(struct gim_mig_file *migf,
			void *vaddr, char __user *buf, ssize_t sz)
{
	if (sz <= 0)
		return 0;

	if (migf->is_target) {
		if (copy_from_user(vaddr, buf, sz))
			return -EFAULT;
	} else {
		if (copy_to_user(buf, vaddr, sz))
			return -EFAULT;
	}

	return sz;
}

static ssize_t gim_mig_user_ringbuffer_data_copy(struct gim_mig_file *migf, size_t rptr,
					ssize_t available_space, char __user *buf, size_t len)
{
	ssize_t copied_size = 0, sz = 0;
	size_t wrapped_space = 0;

	wrapped_space = available_space + rptr > migf->ring.size ?
					available_space + rptr - migf->ring.size : 0;
	available_space = available_space - wrapped_space;

	sz = min_t(ssize_t, available_space, len);
	sz = gim_mig_user_ringbuffer_do_data_copy(migf, migf->ring.va_ptr + rptr, buf + copied_size, sz);
	if (sz < 0)
		return -EFAULT;

	copied_size += sz;
	if (migf->left_bytes_fd == 0) {
		gim_mig_get_total_bytes(migf, migf->ring.va_ptr + rptr, sz);
		migf->left_bytes_fd = migf->total_bytes_fd;
	}

	sz = min_t(ssize_t, wrapped_space, len - copied_size);
	sz = gim_mig_user_ringbuffer_do_data_copy(migf, migf->ring.va_ptr, buf + copied_size, sz);
	if (sz < 0)
		return -EFAULT;

	copied_size += sz;
	if (migf->left_bytes_fd == 0) {
		gim_mig_get_total_bytes(migf, migf->ring.va_ptr, sz);
		migf->left_bytes_fd = migf->total_bytes_fd;
	}

	return copied_size;
}

/* copy the data from ring buffer to userspace on source, or vice versa */
static ssize_t gim_mig_ringbuffer_data_copy_user(struct gim_mig_file *migf,
					char __user *buf, size_t len)
{
	size_t rptr, *rptr_ptr;
	ssize_t available_space = 0;
	ssize_t copied_size = 0;

	if (migf->is_target) {
		rptr = READ_ONCE(migf->ring.wptr);
		rptr_ptr = &migf->ring.wptr;
	} else {
		rptr = READ_ONCE(migf->ring.rptr);
		rptr_ptr = &migf->ring.rptr;
	}

	while (copied_size < len) {
		available_space = gim_mig_user_check_ringbuffer_space(migf, rptr);
		if (available_space == 0)
			break;

		copied_size = gim_mig_user_ringbuffer_data_copy(migf, rptr, available_space,
							buf + copied_size, len - copied_size);
		if (copied_size < 0)
			return -EFAULT;

		rptr += copied_size;
		rptr &= migf->ring.buf_mask;
	}

	WRITE_ONCE(*rptr_ptr, rptr);

	return copied_size;
}

/* a thread to copy data from internal to ringbuffer on source, or vice versa */
static int gim_mig_data_copy_thread(void *context)
{
	struct gim_mig_file *migf = (struct gim_mig_file *)context;
	size_t wptr, *wptr_ptr;
	ssize_t copied_size = 0, available_space = 0;

	sched_set_fifo_low(current);

	if (migf->is_target)
		wptr_ptr = &migf->ring.rptr;
	else
		wptr_ptr = &migf->ring.wptr;

	while (!kthread_should_stop()) {
		wait_event_interruptible(migf->ring.copy_thread_wq,
					 kthread_should_stop() ||
					 (migf->ring.state == GIM_MIG_THREAD_RUNNING &&
					  gim_mig_thread_check_ringbuffer_space(migf, &available_space, &wptr)));

		if (kthread_should_stop())
			break;

		copied_size = gim_mig_thread_ringbuffer_data_copy(migf, available_space, wptr);
		if (copied_size < 0) {
			migf->ring.state = GIM_MIG_THREAD_ABNORMAL;
			gim_put_error(migf->is_target ?
					AMDGV_ERROR_DRIVER_MIGRATION_IMPORT_FAIL :
					AMDGV_ERROR_DRIVER_MIGRATION_EXPORT_FAIL, 0);
			/*don't exit the thread incase calltrace when release resource */
			continue;
		}

		if (copied_size > 0) {
			wptr += copied_size;
			wptr &= migf->ring.buf_mask;
			WRITE_ONCE(*wptr_ptr, wptr);
		}
	}
	return 0;
}

static int gim_mig_data_copy_thread_init(struct gim_mig_file *migf)
{
	if (migf->ring.size == 0)
		return -EINVAL;

	migf->ring.rptr = 0;
	migf->ring.wptr = 0;
	migf->ring.buf_mask = migf->ring.size - 1;
	init_waitqueue_head(&migf->ring.copy_thread_wq);
	if (migf->is_target)
		migf->ring.state = GIM_MIG_THREAD_RUNNING;
	else
		migf->ring.state = GIM_MIG_THREAD_PAUSED;

	migf->ring.thread = kthread_run(gim_mig_data_copy_thread, migf, "live_migration_data_copy_thread");
	if (IS_ERR(migf->ring.thread)) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_THREAD_INIT_FAIL, 0);
		return PTR_ERR(migf->ring.thread);
	}

	gim_info("thread to copy data on %s fb started\n", migf->is_target ? "target" : "source");
	return 0;
}

static void gim_mig_data_copy_thread_fini(struct gim_mig_file *migf)
{
	migf->ring.state = GIM_MIG_THREAD_PAUSED;
	kthread_stop(migf->ring.thread);
	migf->seq = 0;
	migf->enabled = false;
	migf->ring.thread = NULL;
	migf->ring.rptr = 0;
	migf->ring.wptr = 0;
	migf->ring.buf_mask = 0;
}

static int gim_mig_setup_copy(struct gim_mig_vf_ctx *vf_ctx, bool is_target)
{
	struct gim_mig_file *migf = &vf_ctx->migf;
	int ret;

	if (migf->enabled)
		return 0;

	migf->is_target = is_target;

	ret = gim_mig_buffer_init(vf_ctx);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_SETUP_ENV_FAIL, 0);
		return ret;
	}

	ret = gim_mig_data_copy_thread_init(migf);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_SETUP_ENV_FAIL, 0);
		goto thread_error;
	}

	ret = gim_mig_get_file(migf);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_SETUP_ENV_FAIL, 0);
		goto file_error;
	}

	migf->seq = 0;
	migf->enabled = true;
	mutex_init(&migf->lock);
	mutex_init(&migf->shadow_lock);

	return 0;

file_error:
	gim_mig_data_copy_thread_fini(migf);
thread_error:
	gim_mig_buffer_fini(migf);
	return ret;
}

static void gim_mig_do_finish_copy(struct gim_mig_vf_ctx *vf_ctx)
{
	struct gim_mig_file *migf = &vf_ctx->migf;

	if (migf->enabled) {
		migf->enabled = false;
		migf->seq = 0;
		migf->next_copy_mask = 0;
		migf->copied_mask = 0;
		migf->left_bytes_fd = 0;
		migf->total_bytes_fd = 0;
		migf->copied_size_thread = 0;
		gim_mig_clear_thread_copy_info(migf);
		mutex_destroy(&migf->lock);
		mutex_destroy(&migf->shadow_lock);
		gim_mig_data_copy_thread_fini(migf);
		gim_mig_buffer_fini(migf);
		gim_info("Live migration is finished\n");
	}
}

static void gim_mig_finish_copy(struct gim_mig_vf_ctx *vf_ctx)
{
	return;
}

static int gim_mig_stop_copy_device(struct gim_mig_vf_ctx *vf_ctx, enum vfio_device_mig_state state)
{
	gim_mig_update_msg_copy_info(&vf_ctx->migf, NULL, NULL, true, true);

	return 0;
}

static int gim_mig_load_state(struct gim_mig_vf_ctx *vf_ctx)
{
	struct gim_mig_file *migf = &vf_ctx->migf;
	int timeout = 1000;

	while ((!is_ring_empty(migf)) && (--timeout > 0))
		gim_mig_wait_with_timeout(migf, &timeout);

	if (timeout <= 0) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
		return -EFAULT;
	}

	return 0;
}

static int gim_mig_stop_device(struct gim_mig_vf_ctx *vf_ctx)
{
	return amdgv_suspend_vf(vf_ctx->gdev->pf_data->adev, vf_ctx->vf_idx);
}

static int gim_mig_run_device(struct gim_mig_vf_ctx *vf_ctx)
{
	return amdgv_resume_vf(vf_ctx->gdev->pf_data->adev, vf_ctx->vf_idx);
}

/* RUNNING --> STOP/PRE_COPY/RUNNING_P2P */
static struct file *gim_mig_handle_state_running(struct gim_mig_vf_ctx *vf_ctx,
						enum vfio_device_mig_state new_state)
{
	int ret;

	if (new_state == VFIO_DEVICE_STATE_STOP) {
		ret = gim_mig_stop_device(vf_ctx);
		if (ret)
			return ERR_PTR(ret);
		return 0;
	} else if (new_state == VFIO_DEVICE_STATE_PRE_COPY) {
		ret = amdgv_set_vf_migration_state(vf_ctx->gdev->pf_data->adev, vf_ctx->vf_idx,
						   AMDGV_MIGRATION_VF_STATE_PRE_COPY);
		if (ret < 0)
			return ERR_PTR(ret);

		ret = gim_mig_setup_copy(vf_ctx, false);
		if (ret)
			return ERR_PTR(ret);

		return vf_ctx->migf.filp;
	} else if (new_state == VFIO_DEVICE_STATE_RUNNING_P2P) {
		return 0;
	}
	return ERR_PTR(-EINVAL);
}

/* PRE_COPY --> RUNNING/STOP_COPY/PRE_COPY_P2P */
static struct file *gim_mig_handle_state_pre_copy(struct gim_mig_vf_ctx *vf_ctx,
						 enum vfio_device_mig_state new_state)
{
	int ret;

	if (new_state == VFIO_DEVICE_STATE_RUNNING) {
		gim_mig_finish_copy(vf_ctx);
		return 0;
	} else if (new_state == VFIO_DEVICE_STATE_STOP_COPY) {
		ret = amdgv_set_vf_migration_state(vf_ctx->gdev->pf_data->adev, vf_ctx->vf_idx,
						   AMDGV_MIGRATION_VF_STATE_STOP_COPY);
		if (ret < 0)
			return ERR_PTR(ret);

		ret = gim_mig_stop_device(vf_ctx);
		if (ret)
			return ERR_PTR(-EBUSY);
		ret = gim_mig_stop_copy_device(vf_ctx, new_state);
		if (ret)
			return ERR_PTR(-EBUSY);
		return 0;
	} else if (new_state == VFIO_DEVICE_STATE_PRE_COPY_P2P) {
		return 0;
	}
	return ERR_PTR(-EINVAL);
}

/* STOP_COPY --> STOP */
static struct file *gim_mig_handle_state_stop_copy(struct gim_mig_vf_ctx *vf_ctx,
						  enum vfio_device_mig_state new_state)
{
	if (new_state == VFIO_DEVICE_STATE_STOP) {
		gim_mig_finish_copy(vf_ctx);
		return 0;
	} else
		return ERR_PTR(-EINVAL);
}

/* STOP --> RESUMING/STOP_COPY/RUNNING/RUNNING_P2P */
static struct file *gim_mig_handle_state_stop(struct gim_mig_vf_ctx *vf_ctx,
					     enum vfio_device_mig_state new_state)
{
	int ret;
	if (new_state == VFIO_DEVICE_STATE_RESUMING) {
		ret = gim_mig_setup_copy(vf_ctx, true);
		if (ret)
			return ERR_PTR(ret);

		return vf_ctx->migf.filp;
	} else if (new_state == VFIO_DEVICE_STATE_STOP_COPY) {
		ret = amdgv_set_vf_migration_state(vf_ctx->gdev->pf_data->adev, vf_ctx->vf_idx,
						   AMDGV_MIGRATION_VF_STATE_STOP_COPY);
		if (ret < 0)
			return ERR_PTR(ret);

		ret = gim_mig_stop_copy_device(vf_ctx, new_state);
		if (ret)
			return ERR_PTR(ret);
		return 0;
	} else if (new_state == VFIO_DEVICE_STATE_RUNNING ||
		   new_state == VFIO_DEVICE_STATE_RUNNING_P2P) {
		ret = gim_mig_run_device(vf_ctx);
		if (ret)
			return ERR_PTR(-EBUSY);
		return 0;
	}
	return ERR_PTR(-EINVAL);
}

/* RESUMING --> STOP */
static struct file *gim_mig_handle_state_resuming(struct gim_mig_vf_ctx *vf_ctx,
						 enum vfio_device_mig_state new_state)
{
	int ret;

	if (new_state == VFIO_DEVICE_STATE_STOP) {
		ret = gim_mig_load_state(vf_ctx);
		if (ret)
			return ERR_PTR(-EINVAL);

		gim_mig_finish_copy(vf_ctx);
		return 0;
	} else
		return ERR_PTR(-EINVAL);
}

/* PRE_COPY_P2P --> STOP_COPY/PRE_COPY */
static struct file *gim_mig_handle_state_pre_copy_p2p(struct gim_mig_vf_ctx *vf_ctx,
						     enum vfio_device_mig_state new_state)
{
	int ret;
	if (new_state == VFIO_DEVICE_STATE_STOP_COPY) {
		ret = amdgv_set_vf_migration_state(vf_ctx->gdev->pf_data->adev, vf_ctx->vf_idx,
						   AMDGV_MIGRATION_VF_STATE_STOP_COPY);
		if (ret < 0)
			return ERR_PTR(ret);

		ret = gim_mig_stop_device(vf_ctx);
		if (ret)
			return ERR_PTR(-EBUSY);
		ret = gim_mig_stop_copy_device(vf_ctx, new_state);
		if (ret)
			return ERR_PTR(-EBUSY);
		return 0;
	} else if (new_state == VFIO_DEVICE_STATE_PRE_COPY) {
		return 0;
	} else if (new_state == VFIO_DEVICE_STATE_RUNNING_P2P) {
		gim_mig_finish_copy(vf_ctx);
		return 0;
	}

	return ERR_PTR(-EINVAL);
}

/* RUNNING_P2P --> STOP/RUNNING */
static struct file *gim_mig_handle_state_running_p2p(struct gim_mig_vf_ctx *vf_ctx,
						    enum vfio_device_mig_state new_state)
{
	int ret;
	if (new_state == VFIO_DEVICE_STATE_STOP) {
		ret = gim_mig_stop_device(vf_ctx);
		if (ret)
			return ERR_PTR(ret);
		return 0;
	} else if (new_state == VFIO_DEVICE_STATE_RUNNING) {
		return 0;
	}

	return ERR_PTR(-EINVAL);
}

static struct file *gim_mig_set_device_state(struct gim_mig_vf_ctx *vf_ctx,
					 enum vfio_device_mig_state new_state)
{
	/* This FSM supports PRE_COPY, not support P2P */
	switch (vf_ctx->curr_state) {
	case VFIO_DEVICE_STATE_RUNNING:
		return gim_mig_handle_state_running(vf_ctx, new_state);
	case VFIO_DEVICE_STATE_PRE_COPY:
		return gim_mig_handle_state_pre_copy(vf_ctx, new_state);
	case VFIO_DEVICE_STATE_STOP_COPY:
		return gim_mig_handle_state_stop_copy(vf_ctx, new_state);
	case VFIO_DEVICE_STATE_STOP:
		return gim_mig_handle_state_stop(vf_ctx, new_state);
	case VFIO_DEVICE_STATE_RESUMING:
		return gim_mig_handle_state_resuming(vf_ctx, new_state);
	case VFIO_DEVICE_STATE_PRE_COPY_P2P:
		return gim_mig_handle_state_pre_copy_p2p(vf_ctx, new_state);
	case VFIO_DEVICE_STATE_RUNNING_P2P:
		return gim_mig_handle_state_running_p2p(vf_ctx, new_state);
	default:
		return ERR_PTR(-EINVAL);
	}
	return NULL;
}

static const char *vfio_mig_state_name(enum vfio_device_mig_state state)
{
	switch (state) {
	case VFIO_DEVICE_STATE_ERROR:
		return "ERROR";
	case VFIO_DEVICE_STATE_STOP:
		return "STOP";
	case VFIO_DEVICE_STATE_RUNNING:
		return "RUNNING";
	case VFIO_DEVICE_STATE_STOP_COPY:
		return "STOP_COPY";
	case VFIO_DEVICE_STATE_RESUMING:
		return "RESUMING";
	case VFIO_DEVICE_STATE_RUNNING_P2P:
		return "RUNNING_P2P";
	case VFIO_DEVICE_STATE_PRE_COPY:
		return "PRE_COPY";
	case VFIO_DEVICE_STATE_PRE_COPY_P2P:
		return "PRE_COPY_P2P";
	case VFIO_DEVICE_STATE_DEFAULT_GIM:
		return "GIM_DEFAULT";
	default:
		return "UNKNOWN";
	}
}

static struct file *
gim_mig_set_state(struct vfio_device *vdev,
				 enum vfio_device_mig_state new_state)
{
	struct gim_mig_vf_ctx *vf_ctx = vdev_to_vf_ctx(vdev);
	enum vfio_device_mig_state next_state;
	struct file *f = NULL;
	amdgv_dev_t adev = NULL;

	if (vf_ctx == NULL)
		return ERR_PTR(-EINVAL);

	adev = vf_ctx->gdev->pf_data->adev;

	mutex_lock(&vf_ctx->mig_state_lock);
	if (vf_ctx->curr_state == VFIO_DEVICE_STATE_DEFAULT_GIM &&
	    new_state == VFIO_DEVICE_STATE_RUNNING) {
		vf_ctx->curr_state = VFIO_DEVICE_STATE_RUNNING;
		goto out;
	}

	while (vf_ctx->curr_state != new_state) {
		if (vfio_mig_get_next_state(vdev, vf_ctx->curr_state, new_state,
					    &next_state)) {
			gim_warn("Failed to move state from %s to %s\n",
				 vfio_mig_state_name(vf_ctx->curr_state),
				 vfio_mig_state_name(new_state));
			break;
		}

		gim_info("migration state %s --> %s, next_state:%s\n",
			 vfio_mig_state_name(vf_ctx->curr_state),
			 vfio_mig_state_name(new_state), vfio_mig_state_name(next_state));

		f = gim_mig_set_device_state(vf_ctx, next_state);
		if (IS_ERR(f)) {
			gim_warn("Failed to set state to %s\n",
				 vfio_mig_state_name(next_state));
			break;
		}
		vf_ctx->curr_state = next_state;
	}

out:
	mutex_unlock(&vf_ctx->mig_state_lock);
	if (IS_ERR(f)) {
		f = NULL;
		amdgv_migration_set_abort(adev, vf_ctx->vf_idx);
	}
	return f;
}

static int gim_mig_get_state(struct vfio_device *vdev,
			enum vfio_device_mig_state *curr_state)
{
	struct gim_mig_vf_ctx *vf_ctx = vdev_to_vf_ctx(vdev);
	int ret = 0;

	if (vf_ctx == NULL)
		return -EINVAL;

	mutex_lock(&vf_ctx->mig_state_lock);
	if (vf_ctx->migf.enabled)
		*curr_state = vf_ctx->curr_state;
	else
		ret = -EINVAL;
	mutex_unlock(&vf_ctx->mig_state_lock);

	return ret;
}

/*
 * QEMU calls this function from vfio_query_stop_copy_size.
 * If a non-zero value is returned, QEMU will treat the data size as 100 GB.
 *
 * However, if we truly want to abort migration, we should make
 * gim_migf_save_read() fail instead of relying on any other migration interface.
 * Therefore, the return value of this function is not important.
 */
static int gim_mig_get_state_size(struct vfio_device *vdev,
			unsigned long *stop_copy_length)
{
	struct gim_mig_vf_ctx *vf_ctx = vdev_to_vf_ctx(vdev);
	struct gim_mig_device *gdev;
	struct gim_mig_file *migf;
	uint64_t shadow_dirty_fb_len = 0;
	int ret = 0;
	amdgv_dev_t adev = NULL;

	if (vf_ctx == NULL)
		return -EINVAL;
	adev = vf_ctx->gdev->pf_data->adev;
	*stop_copy_length = 0;
	mutex_lock(&vf_ctx->mig_state_lock);
	migf = &vf_ctx->migf;
	gdev = vf_ctx->gdev;

	if (!migf->enabled)
		*stop_copy_length = vf_ctx->fb_size;
	else {
		ret = gim_mig_validate_vf_sched_state(vf_ctx);
		if (ret <= 0) {
			goto exit;
		}

		ret = gim_mig_update_shadow_dirtybit(&vf_ctx->migf, &shadow_dirty_fb_len);
		if (ret) {
			gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_DATA_COPY_FAIL, 0);
			amdgv_migration_set_abort(adev, vf_ctx->vf_idx);
			goto exit;
		}

		*stop_copy_length = migf->left_bytes_fd;
		if (shadow_dirty_fb_len > 0) {
			*stop_copy_length += shadow_dirty_fb_len;
			*stop_copy_length += migf->msg.size;
		}
	}

exit:
	mutex_unlock(&vf_ctx->mig_state_lock);
	gim_dbg("stop_copy_length=0x%lx\n", *stop_copy_length);
	return ret;
}

static const struct vfio_migration_ops gim_mig_ops = {
	.migration_get_state = gim_mig_get_state,
	.migration_set_state = gim_mig_set_state,
	.migration_get_data_size = gim_mig_get_state_size,
};

static int gim_mig_get_vf_info(struct gim_mig_device *gdev, int vf_idx)
{
	struct gim_mig_vf_ctx *vf_ctx = &gdev->vf_ctx[vf_idx];

	if (amdgv_get_migration_data_size(gdev->pf_data->adev, vf_idx, &vf_ctx->fb_size,
		AMDGV_MIGRATION_CONTENT_VF_FB_DATA)) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_GET_MIG_INFO_FAIL, 0);
		return -EINVAL;
	}

	vf_ctx->fb_bitmap_bits = GIM_MIG_VF_FB_BITMAP_BITS(vf_idx);
	vf_ctx->fb_bitmap_size = GIM_MIG_VF_FB_BITMAP_SIZE(vf_idx);
	vf_ctx->vf_idx = vf_idx;
	vf_ctx->gdev = gdev;
	vf_ctx->inited = true;

	return 0;
}

static int gim_mig_update_mig_ctx(struct gim_mig_device *gdev, int vf_idx)
{
	struct amdgv_migration_ctx *ctx = &gdev->vf_ctx[vf_idx].ctx;

	if (amdgv_get_migration_ctx(gdev->pf_data->adev, vf_idx, ctx)) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_GET_MIG_INFO_FAIL, 0);
		return -EINVAL;
	}

	/* Currently, for xgmi_state
	 * true: XGMI sharing is enabled, indicating that multiple GPUs are assigned to a
	 * single VM, and these GPUs support XGMI sharing. P2P flag is needed by QEMU.
	 *
	 * false: XGMI sharing is disabled, indicating that VM is assigned either a single
	 * GPU or single VF without XGMI sharing enabled. P2P flag is not needed.
	 */
	if (ctx->gpu.xgmi_state)
		gdev->mig_flags = VFIO_MIGRATION_STOP_COPY | VFIO_MIGRATION_PRE_COPY |
				  VFIO_MIGRATION_P2P;
	else
		gdev->mig_flags = VFIO_MIGRATION_STOP_COPY | VFIO_MIGRATION_PRE_COPY;

	return 0;
}

/* Setup the migration environment for the VF device.
 *
 * @dev: device pointer of the PF device
 *
 * @return: 0 on success, otherwise error code.
 */
int gim_mig_init(struct pci_dev *pdev)
{
	struct gim_mig_device *gdev = NULL;
	struct gim_dev_data *pf_data;
	int i;
	int ret = 0;

	pf_data = pci_get_drvdata(pdev);
	if (IS_ERR(pf_data)) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_INIT_FAIL, 0);
		return -ENODEV;
	}

	gdev = gim_kzalloc(sizeof(struct gim_mig_device), GFP_KERNEL);
	if (!gdev) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_INIT_FAIL, 0);
		ret = -ENOMEM;
		goto err;
	}

	if (amdgv_migration_get_dirty_page_size(pf_data->adev, &gdev->fb_page_size)) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_INIT_FAIL, 0);
		ret = -EINVAL;
		goto err;
	}

	if (amdgv_get_migration_data_size(pf_data->adev, 0,
			&gdev->fw_s_size, AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA)) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_INIT_FAIL, 0);
		ret = -EINVAL;
		goto err;
	}

	if (amdgv_get_migration_data_size(pf_data->adev, 0,
			&gdev->fw_d_size, AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA)) {
		gim_put_error(AMDGV_ERROR_DRIVER_MIGRATION_INIT_FAIL, 0);
		ret = -EINVAL;
		goto err;
	}

	/* init the lock in case wrong sequence in userspace/other modules */
	for (i = 0; i < AMDGV_MAX_VF_NUM; i++) {
		mutex_init(&gdev->vf_ctx[i].mig_state_lock);
		gdev->vf_ctx[i].curr_state = VFIO_DEVICE_STATE_DEFAULT_GIM;
		gdev->vf_ctx[i].vf_sched_state = GIM_MIG_VF_MIGRATION_INVALID;
	}

	gdev->pf_data = pf_data;
	pf_data->pf_mig_dev = gdev;

	return ret;

err:
	gim_kfree(gdev);
	return ret;
}

void gim_mig_fini(struct pci_dev *pdev)
{
	struct gim_dev_data *pf_data = pci_get_drvdata(pdev);
	int i;

	if (pf_data == NULL || pf_data->pf_mig_dev == NULL)
		return;

	for (i = 0; i < AMDGV_MAX_VF_NUM; i++)
		mutex_destroy(&pf_data->pf_mig_dev->vf_ctx[i].mig_state_lock);

	gim_kfree(pf_data->pf_mig_dev);
	pf_data->pf_mig_dev = NULL;
}

/*
 * Get the migration information for the VF device.
 *
 * @dev: device pointer of the VF device
 * @mig_info: migration information
 *
 * @return: 0 on success, otherwise error code.
 */
int gim_get_mig_info(struct device *dev, struct gim_mig_info *mig_info)
{
	struct gim_mig_device *gdev = dev_to_gdev(dev);
	int vf_idx = -1;
	int ret = 0;

	if (mig_info == NULL || gdev == NULL)
		return -EINVAL;

	vf_idx = gim_get_vf_idx(to_pci_dev(dev), gdev->pf_data);
	if (vf_idx < 0 || vf_idx >= AMDGV_MAX_VF_NUM) {
		ret = -EINVAL;
		goto out;
	}

	if ((!gdev->vf_ctx[vf_idx].inited) &&
	    gim_mig_get_vf_info(gdev, vf_idx)) {
		ret = -EINVAL;
		goto out;
	}

	/* live migration is supported by libgv if update ctx successfully */
	if (gim_mig_update_mig_ctx(gdev, vf_idx)) {
		ret = -EINVAL;
		goto out;
	}

	mig_info->flags = gdev->mig_flags;
	mig_info->mig_ops = &gim_mig_ops;

out:
	return ret;
}

EXPORT_SYMBOL_GPL(gim_get_mig_info);
#endif
/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <smi_drv_oss.h>
#include <linux/types.h>

#include <smi_drv_cmd.h>
#include <smi_drv_event.h>

#include "amdgv_error.h"
#include "gim_gpumon.h"


#include <linux/version.h>
#include <linux/kref.h>

#include <linux/poll.h>
#include <linux/anon_inodes.h>
#include <linux/file.h>
#include <smi_drv_core.h>
#include <smi_drv_core_api.h>

#include "smi_drv_oss_wrapper.h"

#include "smi_drv_types.h"

#if defined(HAVE_POLL_T)
static __poll_t smi_event_poll(smi_process_handle filep,
#else
static unsigned smi_event_poll(smi_process_handle filep,
#endif
				struct poll_table_struct *wait);

static ssize_t smi_lnx_event_read(smi_process_handle filp, char __user *buf, size_t size, loff_t *off);

static int smi_event_release(struct inode *inode, smi_process_handle filp);
static void smi_event_free(struct kref *refcount);


struct smi_lnx_event_ctx {
	struct smi_ctx *smi;
	amdgv_dev_t *adev;
	smi_device_handle_t dev_id;
	wait_queue_head_t wait;
	struct kref refcount;
	struct amdgv_error_notifier *notifier;
	/* scratch buffer for entries */
	struct smi_event_entry event;
};

static const struct file_operations smi_event_fops = {
	.owner = THIS_MODULE,
	.release = smi_event_release,
	.read = smi_lnx_event_read,
	.poll = smi_event_poll,
};

static int smi_event_release(struct inode *inode, smi_process_handle filp)
{
	struct smi_lnx_event_ctx *ctx = filp->private_data;

	kref_put(&ctx->refcount, smi_event_free);
	return 0;
}

static ssize_t smi_lnx_event_read(smi_process_handle filp, char __user *buf, size_t size,
				loff_t *off)
{
	struct amdgv_error_entry *entry;
	struct smi_lnx_event_ctx *ctx = filp->private_data;
	ssize_t ret = 0;
	loff_t ptr;

	/* check the size requested is the size of an event */

	if ((size - *off) < sizeof(struct smi_event_entry))
		return -EINVAL;

	kref_get(&ctx->refcount);

	ptr = *off;

	while (sizeof(struct smi_event_entry) <= (size - ptr)) {
		if (!amdgv_error_get_error(ctx->adev, ctx->notifier, &entry)) {
			if (entry == NULL)
				break;

			ret += sizeof(struct smi_event_entry);

			ctx->event.timestamp = gim_gpumon_ktime_to_utc(entry->timestamp);
			smi_generate_date_string(ctx->event.date, entry->timestamp);
			ctx->event.category =
				AMDGV_ERROR_CATEGORY(entry->error_code);
			ctx->event.subcode =
				AMDGV_ERROR_SUBCODE(entry->error_code);
			ctx->event.level = entry->error_level;
			ctx->event.dev_id = ctx->dev_id.handle;

			if (entry->vf_idx == SMI_PF_INDEX)
				ctx->event.fcn_id.handle = ctx->dev_id.handle;
			else
				ctx->event.fcn_id.handle = smi_get_vf_handle(ctx->smi,
					&ctx->dev_id, entry->vf_idx);

			amdgv_error_get_error_text(entry->error_code,
				entry->error_data,
				ctx->event.message, SMI_EVENT_MSG_SIZE);

			if (copy_to_user(buf + ptr, &ctx->event,
					sizeof(struct smi_event_entry))) {
				ret = -EFAULT;
				break;
			}

			ptr += sizeof(struct smi_event_entry);
		} else
			break;
	}

	kref_put(&ctx->refcount, smi_event_free);

	return ret;
}

static void smi_event_free(struct kref *refcount)
{
	struct smi_lnx_event_ctx *set = container_of(refcount, struct smi_lnx_event_ctx,
				refcount);
	amdgv_dev_t adev = set->adev;

	amdgv_error_delete_notifier(adev, set->notifier);
#if !defined(HAVE_KFREE_SENSITIVE)
	kzfree(set);
#else
	kfree_sensitive(set);
#endif
}

int smi_create_event(struct smi_ctx *smi, amdgv_dev_t *adev, struct smi_event_set_config *config)
{
	struct smi_lnx_event_ctx *set;
	int fd;
	int ret = 0;
	smi_process_handle file;
	char *name;

	set = kzalloc(sizeof(struct smi_lnx_event_ctx), GFP_KERNEL);
	if (set == NULL)
		return -ENOMEM;

	fd = get_unused_fd_flags(O_RDONLY);
	if (fd < 0) {
		ret = -EMFILE;
		goto free_ctx;
	}

	name = kasprintf(GFP_KERNEL, "gim-evt-%d", fd);
	file = anon_inode_getfile(name, &smi_event_fops, set, O_RDONLY);
	kfree(name);

	if (IS_ERR(file)) {
		ret = PTR_ERR(file);
		goto free_fd;
	}

	kref_init(&set->refcount);
	init_waitqueue_head(&set->wait);
	set->smi = smi;
	set->adev = adev;
	set->dev_id.handle = config->dev_id.handle;
	if (amdgv_error_alloc_new_notifier(adev, config->event_mask, &set->wait,
							&set->notifier)) {
		ret = -EIO;
		goto free_mod;
	}

	fd_install(fd, file);

	config->event_set.fd = fd;
	return 0;

free_mod:
	/* fput triggers fop release (smi_event_release) on close,
	   so no need to free set here */
	fput(file);
	put_unused_fd(fd);
	return ret;
free_fd:
	put_unused_fd(fd);
free_ctx:
	kfree(set);

	return ret;
}

#if defined(HAVE_POLL_T)
static __poll_t smi_event_poll(smi_process_handle filep,
#else
static unsigned smi_event_poll(smi_process_handle filep,
#endif
				struct poll_table_struct *wait)
{
	struct smi_lnx_event_ctx *ctx = filep->private_data;
#if defined(HAVE_POLL_T)
	__poll_t events = 0;
#else
	unsigned events = 0;
#endif
	kref_get(&ctx->refcount);

	poll_wait(filep, &ctx->wait, wait);

	if (amdgv_error_is_pending(ctx->adev, ctx->notifier))
		events = POLLIN | POLLRDNORM;

	kref_put(&ctx->refcount, smi_event_free);

	return events;
}

int smi_read_event(struct smi_ctx *ctx, amdgv_dev_t *adev, uint64_t dev_id, struct smi_event_entry *event)
{
	return 0;
}
int smi_destroy_event(struct smi_ctx *ctx, amdgv_dev_t *adev, uint64_t dev_id)
{
	return 0;
}

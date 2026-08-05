/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <smi_drv_oss.h>
#include <linux/types.h>

#include <smi_drv_cmd.h>
#include <smi_drv_event.h>

#include "amdgv_log.h"
#include "gim_gpumon.h"
#include "gim.h"


#include <linux/version.h>
#include <linux/kref.h>
#include <linux/list.h>
#include <linux/mutex.h>

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
	struct amdgv_log_notifier *notifier;
	/* scratch buffer for entries */
	struct smi_event_entry event;
	/* revoke handle: lets the owner revoke this fd before the
	 * back-pointers above are freed */
	struct list_head node;
	/* set under smi_event_ctx_lock when the back-pointers go stale;
	 * gates poll()/read() */
	bool dead;
};

/* All live event fds, guarded by smi_event_ctx_lock; lets revoke clear the
 * raw back-pointers while the objects they point at are still valid. */
static LIST_HEAD(smi_event_ctx_list);
static DEFINE_MUTEX(smi_event_ctx_lock);

/* adevs in teardown but not yet freed, guarded by smi_event_ctx_lock; gates
 * create against a dying device. One slot per live GPU. */
static amdgv_dev_t *smi_dying_adevs[AMDGV_MAX_GPU_NUM];

/* Caller must hold smi_event_ctx_lock. */
static bool smi_adev_is_dying_locked(amdgv_dev_t *adev)
{
	int i;

	for (i = 0; i < AMDGV_MAX_GPU_NUM; i++)
		if (smi_dying_adevs[i] == adev)
			return true;
	return false;
}

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
	struct amdgv_log_entry *entry;
	struct smi_lnx_event_ctx *ctx = filp->private_data;
	ssize_t ret = 0;
	loff_t ptr;

	/* check the size requested is the size of an event */

	if ((size - *off) < sizeof(struct smi_event_entry))
		return -EINVAL;

	kref_get(&ctx->refcount);

	ptr = *off;

	while (sizeof(struct smi_event_entry) <= (size - ptr)) {
		mutex_lock(&smi_event_ctx_lock);

		/* smi_ctx/adev/notifier may have been revoked on /dev close;
		   stop before dereferencing the freed back-pointers */
		if (ctx->dead || ctx->notifier == NULL) {
			mutex_unlock(&smi_event_ctx_lock);
			break;
		}

		if (amdgv_log_get_entry(ctx->adev, ctx->notifier, &entry) ||
				entry == NULL) {
			mutex_unlock(&smi_event_ctx_lock);
			break;
		}

		ret += sizeof(struct smi_event_entry);

		ctx->event.timestamp = gim_gpumon_ktime_to_utc(entry->timestamp);
		smi_generate_date_string(ctx->event.date, entry->timestamp);
		ctx->event.category =
			AMDGV_LOG_CATEGORY(entry->log_code);
		ctx->event.subcode =
			AMDGV_LOG_SUBCODE(entry->log_code);
		ctx->event.level = entry->log_level;
		ctx->event.processor_handle.handle = ctx->dev_id.handle;

		if (entry->vf_idx == SMI_PF_INDEX)
			ctx->event.fcn_id.handle = ctx->dev_id.handle;
		else
			ctx->event.fcn_id.handle = smi_get_vf_handle(ctx->smi,
				&ctx->dev_id, entry->vf_idx);

		amdgv_log_get_text(entry->log_code,
			entry->log_data,
			ctx->event.message, SMI_EVENT_MSG_SIZE);
		ctx->event.data = entry->log_data;

		/* ctx->event is the wrapper's own scratch (kref-protected), so
		   the user copy is safe to do without the revoke lock held */
		mutex_unlock(&smi_event_ctx_lock);

		if (copy_to_user(buf + ptr, &ctx->event,
				sizeof(struct smi_event_entry))) {
			ret = -EFAULT;
			break;
		}

		ptr += sizeof(struct smi_event_entry);
	}

	kref_put(&ctx->refcount, smi_event_free);

	return ret;
}

static void smi_event_free(struct kref *refcount)
{
	struct smi_lnx_event_ctx *set = container_of(refcount, struct smi_lnx_event_ctx,
				refcount);

	mutex_lock(&smi_event_ctx_lock);
	list_del_init(&set->node);
	/* Delete under the lock so this is ordered against a concurrent
	   revoke; notifier is NULL if already revoked. */
	if (set->notifier != NULL) {
		amdgv_log_delete_notifier(set->adev, set->notifier);
		set->notifier = NULL;
	}
	mutex_unlock(&smi_event_ctx_lock);

#if !defined(HAVE_KFREE_SENSITIVE)
	gim_kzfree(set);
#else
	gim_kfree_sensitive(set);
#endif
}

int smi_create_event(struct smi_ctx *smi, amdgv_dev_t *adev, struct smi_event_set_config *config)
{
	struct smi_lnx_event_ctx *set;
	int fd;
	int ret = 0;
	smi_process_handle file;
	char *name;

	set = gim_kzalloc(sizeof(struct smi_lnx_event_ctx), GFP_KERNEL);
	if (set == NULL)
		return -ENOMEM;

	/* keep node self-linked so list_del_init() is safe on every error path */
	INIT_LIST_HEAD(&set->node);

	fd = get_unused_fd_flags(O_RDONLY);
	if (fd < 0) {
		ret = -EMFILE;
		goto free_ctx;
	}

	name = kasprintf(GFP_KERNEL, "gim-evt-%d", fd);
	file = anon_inode_getfile(name, &smi_event_fops, set, O_RDONLY);
	gim_kfree(name);

	if (IS_ERR(file)) {
		ret = PTR_ERR(file);
		goto free_fd;
	}

	kref_init(&set->refcount);
	init_waitqueue_head(&set->wait);
	set->smi = smi;
	set->adev = adev;
	set->dev_id.handle = config->dev_id.handle;

	/* Allocate and publish under the lock, ordered against revoke. Refuse if
	   the ctx is closing or the device is dying, else the sweep would revoke
	   this notifier while the adapter is still alive. */
	mutex_lock(&smi_event_ctx_lock);
	if (smi->releasing || smi_adev_is_dying_locked(adev)) {
		mutex_unlock(&smi_event_ctx_lock);
		ret = -ENODEV;
		goto free_mod;
	}
	if (amdgv_log_alloc_new_notifier(adev, config->event_mask, &set->wait,
							&set->notifier)) {
		mutex_unlock(&smi_event_ctx_lock);
		ret = -EIO;
		goto free_mod;
	}
	list_add(&set->node, &smi_event_ctx_list);
	mutex_unlock(&smi_event_ctx_lock);

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
	gim_kfree(set);

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

	mutex_lock(&smi_event_ctx_lock);
	/* A revoked fd must report a terminal condition: the revoke woke this
	   waiter once, and without POLLHUP it would see no event and sleep again
	   forever instead of observing the device/context teardown. */
	if (ctx->dead || ctx->notifier == NULL)
		events = POLLHUP | POLLERR;
	else if (amdgv_log_is_pending(ctx->adev, ctx->notifier))
		events = POLLIN | POLLRDNORM;
	mutex_unlock(&smi_event_ctx_lock);

	kref_put(&ctx->refcount, smi_event_free);

	return events;
}

int smi_read_event(struct smi_ctx *ctx, amdgv_dev_t *adev, uint64_t dev_id, struct smi_event_entry *event, int64_t timeout_usec)
{
	(void)timeout_usec;
	return 0;
}

/* Mark one event fd dead and drop its back-pointers. Caller must hold
 * smi_event_ctx_lock. adev/notifier are still valid when this runs (revoke
 * always happens before the device/context they point at is freed), so the
 * notifier is deleted here rather than later in smi_event_free(). */
static void smi_event_revoke_locked(struct smi_lnx_event_ctx *set)
{
	set->dead = true;
	if (set->notifier != NULL) {
		amdgv_log_delete_notifier(set->adev, set->notifier);
		set->notifier = NULL;
	}
	set->adev = NULL;
	set->smi = NULL;
	wake_up_interruptible(&set->wait);
}

int smi_destroy_event(struct smi_ctx *ctx, amdgv_dev_t *adev, uint64_t dev_id)
{
	struct smi_lnx_event_ctx *set;
	/* adev == NULL is the /dev-close sentinel: revoke every fd on this ctx.
	 * A real adev comes from the per-GPU DESTROY_EVENT ioctl, which must
	 * only revoke fds opened against that one device - matched on dev_id. */
	bool revoke_all = (adev == NULL);

	/* The smi_ctx (and its vf_map) is about to be freed on /dev close while
	 * event fds opened against it may still be held. Revoke the matching
	 * fds: delete their notifier (adev is still alive at this point) and
	 * clear the back-pointers under the lock so any later poll()/read()/
	 * close() can no longer dereference the freed smi_ctx or notifier. */
	mutex_lock(&smi_event_ctx_lock);
	/* Block a concurrent CREATE_EVENT (which runs without ioctl_mutex) from
	 * publishing a new fd onto a context being torn down on /dev close. */
	if (revoke_all)
		ctx->releasing = true;
	list_for_each_entry(set, &smi_event_ctx_list, node) {
		if (set->smi != ctx || set->dead)
			continue;
		if (!revoke_all && set->dev_id.handle != dev_id)
			continue;
		smi_event_revoke_locked(set);
	}
	mutex_unlock(&smi_event_ctx_lock);

	return 0;
}

void smi_revoke_device_events(amdgv_dev_t *adev)
{
	struct smi_lnx_event_ctx *set;
	int i;
	int free_slot = -1;

	if (adev == NULL)
		return;

	/* Revoke every event fd bound to this device, across all /dev contexts,
	 * before the adapter and its notifier state are freed. Must be called
	 * BEFORE amdgv_device_fini_ex(). */
	mutex_lock(&smi_event_ctx_lock);
	/* Mark the adev dying so an in-flight CREATE_EVENT not yet listed refuses
	 * to publish; the sweep below revokes already-listed fds. */
	for (i = 0; i < AMDGV_MAX_GPU_NUM; i++) {
		if (smi_dying_adevs[i] == adev)
			break;
		if (free_slot < 0 && smi_dying_adevs[i] == NULL)
			free_slot = i;
	}
	if (i == AMDGV_MAX_GPU_NUM && free_slot >= 0)
		smi_dying_adevs[free_slot] = adev;
	list_for_each_entry(set, &smi_event_ctx_list, node) {
		if (set->adev != adev || set->dead)
			continue;
		smi_event_revoke_locked(set);
	}
	mutex_unlock(&smi_event_ctx_lock);
}

void smi_clear_device_teardown(amdgv_dev_t *adev)
{
	int i;

	if (adev == NULL)
		return;

	/* Drop the adev-teardown marker once the adapter has been freed
	 * (amdgv_device_fini_ex returned). A subsequent probe that reuses the
	 * same address must not inherit a stale "dying" state, and the slot is
	 * reclaimed for the next teardown. Must be called AFTER
	 * amdgv_device_fini_ex(). */
	mutex_lock(&smi_event_ctx_lock);
	for (i = 0; i < AMDGV_MAX_GPU_NUM; i++) {
		if (smi_dying_adevs[i] == adev) {
			smi_dying_adevs[i] = NULL;
			break;
		}
	}
	mutex_unlock(&smi_event_ctx_lock);
}

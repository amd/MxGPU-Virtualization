/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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

#include <linux/kobject.h>
#include <linux/cdev.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/vfio.h>
#include <linux/version.h>
#if defined(HAVE_DCORE_IOVA_VM_CTX_VFIO_DEVICE)
#include <linux/vfio_pci_core.h>
#endif
#include <linux/pci.h>
#include <linux/debugfs.h>

#include <linux/sched/task.h>
#include <linux/sched/signal.h>
#include <linux/vmalloc.h>

#include "gim_config.h"
#include "gim.h"
#include "dcore_drv.h"
#include "dcore_ioctl.h"
#include "amdgv_api.h"

MODULE_SOFTDEP("pre:vfio-pci");

#define AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_END (5)
#define AMDGV_DIAG_DATA_CRIT_ERR_LOG_SIZE (3*1024*1024)
#define GIM_DIAG_DATA_DEFAULT_MEM_SIZE ((AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_END - 1) * AMDGV_DIAG_DATA_CRIT_ERR_LOG_SIZE)

static struct dcore {
	struct class *dcore_class;
	struct cdev dcore_cdev;
	struct device *dcore_dev;
	dev_t devid;
	struct list_head iova_list;
	spinlock_t lock_iova_list;
} dcore;

static LIST_HEAD(trap_ctx_list);
static DEFINE_SPINLOCK(lock_ctx_list);

/* device functions */
static int dcore_drv_open(struct inode *, struct file *);
static int dcore_drv_release(struct inode *, struct file *);
static long dcore_ioctl_handler(struct file *, unsigned int, unsigned long);
/* iova node functions */
static int dcore_iova_node_init(void);
static void dcore_iova_node_fini(void);
/* ioctl node functions */
static int dcore_trap_gpu_ctx_init(void);
static void dcore_trap_gpu_ctx_fini(void);
static int dcore_reset_ctx_event_output(struct dbglib_trap_gpu_ctx *ctx);
static int dcore_reset_ctx_status_proc(struct dbglib_trap_gpu_ctx *ctx);

static const struct file_operations dcore_file_ops = {
	.owner                  = THIS_MODULE,
	.unlocked_ioctl         = dcore_ioctl_handler,
	.open                   = dcore_drv_open,
	.release                = dcore_drv_release,
};

/* Prototype for dcore iova node */
struct iova_node {
	struct cdev		iova_cdev;
	struct device		*dev;
	struct pci_dev		*pdev;
	struct list_head	list;
};

/*
 * Module/class support
 */
#if defined(HAVE_DEVNODE_CONST)
static char *dcore_devnode(const struct device *dev, umode_t *mode)
#else
static char *dcore_devnode(struct device *dev, umode_t *mode)
#endif
{
	return kasprintf(GFP_KERNEL, "amdgpu_rdl/%s", dev_name(dev));
}

int dcore_init(void)
{
	int res = 0;

	res = alloc_chrdev_region(&dcore.devid, 0, MINOR_COUNT, DCORE_DEVICE_NAME);
	if (res < 0)
		goto failed;
#if !defined(HAVE_CLASS_CREATE_1_ARG)
	dcore.dcore_class = class_create(THIS_MODULE, DCORE_DEVICE_NAME);
#else
	dcore.dcore_class = class_create(DCORE_DEVICE_NAME);
#endif
	if (IS_ERR(dcore.dcore_class)) {
		res = PTR_ERR(dcore.dcore_class);
		goto err_class;
	}

	dcore.dcore_class->devnode = dcore_devnode;

	dcore.dcore_dev = device_create(dcore.dcore_class, NULL,
		MKDEV(MAJOR(dcore.devid), 0), NULL, DCORE_DEVICE_NAME);

	cdev_init(&dcore.dcore_cdev, &dcore_file_ops);
	res = cdev_add(&dcore.dcore_cdev, dcore.devid, 1);
	if (res < 0)
		goto err_cdev;

	kobject_uevent(&dcore.dcore_cdev.kobj, KOBJ_ADD);

	res = dcore_iova_node_init();
	if (res < 0)
		goto err_iova;

	res = dcore_trap_gpu_ctx_init();
	if (res < 0)
		goto err_iova;

	return 0;

err_iova:
	cdev_del(&dcore.dcore_cdev);
err_cdev:
	device_destroy(dcore.dcore_class, dcore.dcore_dev->devt);
	class_destroy(dcore.dcore_class);
err_class:
	unregister_chrdev_region(dcore.devid, MINOR_COUNT);
failed:
	return res;
}

void dcore_cleanup(void)
{
	dcore_trap_gpu_ctx_fini();
	dcore_iova_node_fini();
	kobject_uevent(&dcore.dcore_cdev.kobj, KOBJ_REMOVE);
	cdev_del(&dcore.dcore_cdev);
	device_destroy(dcore.dcore_class, dcore.dcore_dev->devt);
	class_destroy(dcore.dcore_class);
	unregister_chrdev_region(dcore.devid, MINOR_COUNT);
};

static int dcore_drv_open(struct inode *inode, struct file *filp)
{
	struct dbglib_private *private;

	private = kzalloc(sizeof(struct dbglib_private), GFP_KERNEL);
	if (!private)
		return -ENOMEM;

	filp->private_data = private;
	private->ctx = NULL;

	return 0;
}

static int dcore_drv_release(struct inode *inode, struct file *filp)
{
	struct dbglib_private *private = NULL;
	struct dbglib_trap_gpu_ctx *trap_ctx = NULL;

	if (!filp) {
		return 0;
	}

	private = (struct dbglib_private *)filp->private_data;

	/* Recover the context data from the file descriptor */
	if (private) {
		trap_ctx = private->ctx;
	}

	/* When the node is closed, reset the trap context */
	if (trap_ctx) {
		mutex_lock(&trap_ctx->mutex);
		dcore_reset_ctx_event_output(trap_ctx);
		dcore_reset_ctx_status_proc(trap_ctx);
		mutex_unlock(&trap_ctx->mutex);
	}

	if (private)
		kfree(private);

	return 0;
}

static struct dbglib_trap_gpu_ctx *get_trap_ctx_by_bdf(uint32_t bdf)
{
	struct dbglib_trap_gpu_ctx *ctx = NULL;
	struct dbglib_trap_gpu_ctx *tmp = NULL;

	spin_lock(&lock_ctx_list);
	list_for_each_entry(tmp, &trap_ctx_list, node) {
		if (tmp && tmp->pf_dbsf == bdf) {
			ctx = tmp;
			break;
		}
	}
	spin_unlock(&lock_ctx_list);

	return ctx;
}

static struct dbglib_trap_gpu_ctx *get_trap_ctx_by_dev(amdgv_dev_t dev)
{
	struct dbglib_trap_gpu_ctx *ctx = NULL;
	struct dbglib_trap_gpu_ctx *tmp = NULL;

	if (!dev)
		return NULL;

	spin_lock(&lock_ctx_list);
	list_for_each_entry(tmp, &trap_ctx_list, node) {
		if (tmp && tmp->adev == dev) {
			ctx = tmp;
			break;
		}
	}
	spin_unlock(&lock_ctx_list);

	return ctx;
}

static int get_vf_bdf_by_idx(amdgv_dev_t dev, uint32_t idx_vf, uint32_t *vf_bdf)
{
	union amdgv_vf_info *vf_info = NULL;
	int ret = 0;

	if (!dev || !vf_bdf)
		return -EINVAL;

	vf_info = kzalloc(sizeof(union amdgv_vf_info), GFP_KERNEL);
	if (!vf_info)
		return -ENOMEM;

	if (amdgv_get_vf_info(dev, idx_vf, AMDGV_GET_VF_BDF, vf_info)) {
		ret = -EINVAL;
		goto end;
	}

	*vf_bdf = vf_info->id.bdf;

end:
	if (vf_info)
		kfree(vf_info);

	return ret;
}

/* Reset the event and output field of the context */
static int dcore_reset_ctx_event_output(struct dbglib_trap_gpu_ctx *ctx)
{
	if (!ctx)
		return -EINVAL;

	/* Reset output field */
	ctx->dbsf = 0;
	ctx->event = DBGLIB_TRAP_EVENT_EXIT;
	ctx->error_code = 0;
	ctx->cookie = 0;

	/* Reset event */
	reinit_completion(&ctx->trap_event);
	reinit_completion(&ctx->untrap_event);
	reinit_completion(&ctx->data_ready_event);
	complete(&ctx->data_ready_event);

	return 0;
}

/* Reset the status and the status field and process info of the context */
static int dcore_reset_ctx_status_proc(struct dbglib_trap_gpu_ctx *ctx)
{
	if (!ctx)
		return -EINVAL;

	/* Reset status */
	atomic_set(&ctx->status, DBGLIB_TRAP_DISABLED);

	if (!ctx->proc_info)
		return -EINVAL;

	/* Reset process info */
	if (ctx->proc_info->task)
		put_task_struct(ctx->proc_info->task);
	ctx->proc_info->task = NULL;
	ctx->proc_info->pid = 0;
	ctx->proc_info->tgid = 0;

	return 0;
}

/* Fill the process data with current thread and task info */
static int dcore_fill_trap_ctx_proc(struct dbglib_process_info *proc_info)
{
	if (!proc_info)
		return -EINVAL;

	if (proc_info->task)
		put_task_struct(proc_info->task);
	get_task_struct(current);
	proc_info->task = current;
	proc_info->tgid = current->tgid;
	proc_info->pid = current->pid;
	get_task_comm(proc_info->process_name, current->group_leader);

	return 0;
}

/*
 * Checks the current task,
 * compare its task structure with the existing process data
 */
static bool dcore_check_trap_ctx_active(struct dbglib_process_info *proc_info)
{
	struct task_struct *ref;
	struct pid *pid;

	if (!proc_info)
		return false;

	if (proc_info->task == NULL || proc_info->pid == 0)
		return false;

	pid = find_get_pid(proc_info->pid);
	ref = pid_task(pid, PIDTYPE_PID);

	return ref == proc_info->task;
}

static int dcore_get_ffbm_data(struct file *file, void *user_buf)
{
	int max_num = 512;
	int ffbm_struct_size = 128;
	struct gim_dev_data *dev_data;
	struct dbglib_ffbm_block ffbm_block;
	int len = 0;
	int ret = 0;
	int dbsf = 0;
	void *buffer = vmalloc(max_num * ffbm_struct_size);

	if (!buffer) {
		ret = -ENOMEM;
		goto end;
	}

	if (copy_from_user(&ffbm_block, user_buf, sizeof(struct dbglib_ffbm_block))) {
		ret = -EINVAL;
		goto end;
	}

	list_for_each_entry(dev_data, &gim_device_list, list) {
		dbsf = (dev_data->vf_info.id.bdf >> 8) << 8;
		if (ffbm_block.dbsf != dbsf) {
			continue;
		}
		amdgv_ffbm_copy_spa_data(dev_data->adev, buffer, max_num, &len);
	}

	if (copy_to_user(ffbm_block.buffer, buffer, len)) {
		ret = -EINVAL;
	}

end:
	if (buffer)
		vfree(buffer);
	return ret;
}
/*
 * Callback function for libgv to :
 * 1. notify dcore that reset happened
 * 2. wait until host done the hang dump
 */
void dcore_signal_reset_happened(amdgv_dev_t dev, uint32_t idx_vf)
{
	struct dbglib_trap_gpu_ctx *ctx = NULL;
	uint64_t time_stamp = 0;
	uint32_t dbdf, out_bdf = 0;
	long timeout_ret = 0;

	if (!dev)
		return;

	ctx = get_trap_ctx_by_dev(dev);
	if (!ctx) {
		DCORE_ERROR("Can't find trap context\n");
		return;
	}

	dbdf = ctx->pf_dbsf;

	if (!ctx->proc_info)
		return;

	if (!dcore_check_trap_ctx_active(ctx->proc_info))
		return;

	/* Get the vf bdf by the index */
	if (idx_vf == AMDGV_PF_IDX)
		out_bdf = ctx->pf_dbsf;
	else if (get_vf_bdf_by_idx(ctx->adev, idx_vf, &out_bdf))
		return;

	if (atomic_read(&ctx->status) == DBGLIB_TRAP_WAITING) {
		atomic_set(&ctx->status, DBGLIB_TRAP_DUMPING);
		DCORE_DEVICE_INFO(ctx->dev, "Reset event triggered by [%x:%x:%x]\n",
							((out_bdf >> 8) & (0xff)), ((out_bdf >> 3) & (0x1f)), (out_bdf & (0x7)));
		time_stamp = (uint64_t)ktime_to_us(ktime_get());
		mutex_lock(&ctx->mutex);
		ctx->event = DBGLIB_TRAP_EVENT_RESET;
		/* Generate random cookies */
		gim_calc_hash_ext("crc32", &ctx->cookie, (void *)&time_stamp, sizeof(time_stamp));
		ctx->dbsf = out_bdf;
		ctx->idx_vf = idx_vf;
		reinit_completion(&ctx->untrap_event);
		complete(&ctx->trap_event);
		mutex_unlock(&ctx->mutex);
		DCORE_DEVICE_INFO(ctx->dev, "Create a new cookie for this hang dump: %u\n", ctx->cookie);
		/* wait host finised the hang dump */
		DCORE_DEVICE_INFO(ctx->dev, "libgv waits for the hang dump to finish\n");
		timeout_ret = wait_for_completion_interruptible_timeout(&ctx->untrap_event,
				msecs_to_jiffies(ctx->untrap_timeout));
		if (timeout_ret == 0) {
			if (ctx->proc_info->task && !IS_ERR(ctx->proc_info->task)) {
				task_lock(ctx->proc_info->task);
				kill_pid(find_get_pid(ctx->proc_info->pid), SIGIO, 1);
				task_unlock(ctx->proc_info->task);
			}
			DCORE_DEVICE_ERROR(ctx->dev, "Host hang dump timed out\n");
		} else if (timeout_ret > 0) {
			DCORE_DEVICE_INFO(ctx->dev, "libgv waked up from waiting\n");
		} else if (timeout_ret == -ERESTARTSYS) {
			DCORE_DEVICE_ERROR(ctx->dev, "Host threads exit unexpectedly\n");
		}
	}

	return;
}

void dcore_signal_manual_dump_happened(amdgv_dev_t dev, uint32_t idx_vf)
{
	struct dbglib_trap_gpu_ctx *ctx = NULL;
	uint64_t time_stamp = 0;
	uint32_t dbdf, out_bdf = 0;
	long timeout_ret = 0;
	uint32_t debug_data_size = GIM_DIAG_DATA_DEFAULT_MEM_SIZE;
	void *debug_data = NULL;

	if (!dev)
		return;

	ctx = get_trap_ctx_by_dev(dev);
	if (!ctx) {
		DCORE_ERROR("Can't find trap context\n");
		return;
	}

	dbdf = ctx->pf_dbsf;

	if (!ctx->proc_info)
		return;

	if (!dcore_check_trap_ctx_active(ctx->proc_info))
		return;

	/* Get the vf bdf by the index */
	if (idx_vf == AMDGV_PF_IDX)
		out_bdf = ctx->pf_dbsf;
	else if (get_vf_bdf_by_idx(ctx->adev, idx_vf, &out_bdf))
		return;

	if (atomic_read(&ctx->status) == DBGLIB_TRAP_WAITING) {

		atomic_set(&ctx->status, DBGLIB_TRAP_DUMPING);
		DCORE_DEVICE_INFO(ctx->dev, "manual dump event triggered by [%x:%x:%x]\n",
							((out_bdf >> 8) & (0xff)), ((out_bdf >> 3) & (0x1f)), (out_bdf & (0x7)));
		time_stamp = (uint64_t)ktime_to_us(ktime_get());
		mutex_lock(&ctx->mutex);
		ctx->event = DBGLIB_TRAP_EVENT_MANUAL_DUMP;
		/* Generate random cookies */
		gim_calc_hash_ext("crc32", &ctx->cookie, (void *)&time_stamp, sizeof(time_stamp));
		ctx->dbsf = out_bdf;
		ctx->idx_vf = idx_vf;
		reinit_completion(&ctx->untrap_event);
		complete(&ctx->trap_event);
		mutex_unlock(&ctx->mutex);
		DCORE_DEVICE_INFO(ctx->dev, "Create a new cookie for this manual dump: %u\n", ctx->cookie);
		/* wait host finised the manual dump */
		DCORE_DEVICE_INFO(ctx->dev, "libgv waits for the manual dump to finish\n");
		timeout_ret = wait_for_completion_interruptible_timeout(&ctx->untrap_event,
				msecs_to_jiffies(ctx->untrap_timeout));
		if (timeout_ret == 0) {
			if (ctx->proc_info->task && !IS_ERR(ctx->proc_info->task)) {
				task_lock(ctx->proc_info->task);
				kill_pid(find_get_pid(ctx->proc_info->pid), SIGIO, 1);
				task_unlock(ctx->proc_info->task);
			}
			DCORE_DEVICE_ERROR(ctx->dev, "Host manual dump timed out\n");
		} else if (timeout_ret > 0) {
			DCORE_DEVICE_INFO(ctx->dev, "libgv waked up from waiting\n");
		} else if (timeout_ret == -ERESTARTSYS) {
			DCORE_DEVICE_ERROR(ctx->dev, "Host threads exit unexpectedly\n");
		}
	}

	debug_data = vmalloc(debug_data_size);
	if (!debug_data) {
		return;
	}

	if (!amdgv_get_diag_data(dev, dbdf, debug_data, &debug_data_size)) {
		if (debug_data_size > GIM_DIAG_DATA_DEFAULT_MEM_SIZE) {
			debug_data_size = GIM_DIAG_DATA_DEFAULT_MEM_SIZE;
		}
		ctx->manual_dump_buffer = debug_data;
		ctx->manual_dump_size = debug_data_size;
	}
	return;
}

/* Callback function for libgv to notify host that diagnosis data is ready for collecting */
void dcore_signal_diag_data_ready(amdgv_dev_t dev)
{
	struct dbglib_trap_gpu_ctx *ctx = NULL;
	uint32_t dbdf = 0;

	if (!dev)
		return;

	ctx = get_trap_ctx_by_dev(dev);
	if (!ctx) {
		DCORE_ERROR("Can't find trap context\n");
		return;
	}

	dbdf = ctx->pf_dbsf;
	DCORE_DEVICE_INFO(ctx->dev, "libgv signal diagnosis data is ready\n");

	if (!ctx->proc_info)
		return;

	if (!dcore_check_trap_ctx_active(ctx->proc_info))
		return;

	if (atomic_read(&ctx->status) == DBGLIB_TRAP_DUMPING) {
		atomic_set(&ctx->status, DBGLIB_TRAP_DISABLED);
		mutex_lock(&ctx->mutex);
		complete(&ctx->data_ready_event);
		mutex_unlock(&ctx->mutex);
	}

	return;
}

/* Callback function for libgv to check if function:
 * amdgv_get_diag_data() allowed to return the diagnosis data.
 * When ctx status in waiting or dumping status,
 * legacy diagnosis data collect should be disabled
 */
bool dcore_diag_data_collect_disabled(amdgv_dev_t dev, uint32_t bdf)
{
	struct dbglib_trap_gpu_ctx *ctx = NULL;

	if (!dev && bdf == 0)
		return false;

	if (dev)
		ctx = get_trap_ctx_by_dev(dev);
	else
		ctx = get_trap_ctx_by_bdf(bdf);

	if (!ctx)
		return false;

	if (ctx->event == DBGLIB_TRAP_EVENT_MANUAL_DUMP)
		return false;

	if (atomic_read(&ctx->status) == DBGLIB_TRAP_DUMPING ||
				atomic_read(&ctx->status) == DBGLIB_TRAP_WAITING) {
		DCORE_DEVICE_INFO(ctx->dev,
					"Device is being trapped, diagnosis data collection is temporarily disabled\n");
		return true;
	}

	return false;
}

/* Dcore ioctl function for command RDL_CMD_START_TRAP_GPU_HANG */
static int dcore_start_trap_gpu_hang(struct file *filp, void *arg)
{
	struct dbglib_trap_gpu_info *data_in = (struct dbglib_trap_gpu_info *)arg;
	struct dbglib_private *private = (struct dbglib_private *)filp->private_data;
	struct dbglib_trap_gpu_info gpu_info;
	struct dbglib_trap_gpu_ctx *ctx = NULL;
	enum dbglib_trap_event event = DBGLIB_TRAP_EVENT_ERROR;
	uint32_t bdf_in = 0;
	uint32_t bdf_out = 0;
	uint32_t cookie = 0;
	int ret = 0;

	if (!data_in)
		return -EINVAL;

	/* Copy data from user */
	if (copy_from_user(&gpu_info, data_in,
				sizeof(struct dbglib_trap_gpu_info))) {
		return -EINVAL;
	}

	bdf_in = gpu_info.in.dbsf;

	ctx = private->ctx;
	if (!ctx) {
		ctx = get_trap_ctx_by_bdf(bdf_in);
		if (!ctx) {
			DCORE_ERROR("Can't find trap context\n");
			ret = -EINVAL;
			goto end;
		}
	}

	if (!ctx->proc_info) {
		ret = -EINVAL;
		goto end;
	}

	if (!dcore_check_trap_ctx_active(ctx->proc_info)) {
		if (ctx->proc_info->task == NULL || ctx->proc_info->pid == 0) {
			mutex_lock(&ctx->mutex);
			ret = dcore_fill_trap_ctx_proc(ctx->proc_info);
			mutex_unlock(&ctx->mutex);
			if (ret)
				goto end;
			private->ctx = ctx;
			DCORE_DEVICE_INFO(ctx->dev,
					"Successfully trapped by %s pid=%u\n", ctx->proc_info->process_name, ctx->proc_info->pid);
		} else {
			ret = -EACCES;
			goto end;
		}
	}

	if (atomic_read(&ctx->status) == DBGLIB_TRAP_DISABLED) {
		/* Reinit the events & output field before start trap hang*/
		mutex_lock(&ctx->mutex);
		ret = dcore_reset_ctx_event_output(ctx);
		mutex_unlock(&ctx->mutex);

		atomic_set(&ctx->status, DBGLIB_TRAP_WAITING);

		if (wait_for_completion_interruptible(&ctx->trap_event)) {
			ret = -EIO;
			goto end;
		}
		/* Save output value */
		cookie = ctx->cookie;
		bdf_out = ctx->dbsf;
		event = ctx->event;
	} else {
		ret = -EINVAL;
	}

end:
	/* Fill dbglib_dcore_trap out info*/
	gpu_info.out.dbsf = bdf_out;
	gpu_info.out.event = event;
	gpu_info.out.error_code = abs(ret);
	gpu_info.out.cookie = cookie;

	if (copy_to_user(data_in, &gpu_info,
					  sizeof(struct dbglib_trap_gpu_info))) {
		ret = -EINVAL;
	}

	return ret;
}

/* Dcore ioctl function for command RDL_CMD_NOTIFY_DUMP_DONE */
static int dcore_signal_hang_dump_done(struct file *filp, void *arg)
{
	struct dbglib_notify_done_info *data_in = (struct dbglib_notify_done_info *)arg;
	struct dbglib_private *private = (struct dbglib_private *)filp->private_data;
	struct dbglib_notify_done_info gpu_info;
	struct dbglib_trap_gpu_ctx *ctx = NULL;
	uint32_t bdf = 0;
	int ret = 0;

	if (!data_in)
		return -EINVAL;

	/* Copy data from user */
	if (copy_from_user(&gpu_info, data_in,
				sizeof(struct dbglib_notify_done_info))) {
		return -EINVAL;
	}

	bdf = gpu_info.in.dbsf;

	ctx = private->ctx;
	if (!ctx) {
		ctx = get_trap_ctx_by_bdf(bdf);
		if (!ctx) {
			DCORE_ERROR("Can't find trap context\n");
			ret = -EINVAL;
			goto end;
		}
	}

	DCORE_DEVICE_INFO(ctx->dev, "Host signal hang dump done\n");

	if (!ctx->proc_info) {
		ret = -EINVAL;
		goto end;
	}

	if (!dcore_check_trap_ctx_active(ctx->proc_info)) {
		ret = -EACCES;
		goto end;
	}

	if (atomic_read(&ctx->status) == DBGLIB_TRAP_DUMPING ||
				atomic_read(&ctx->status) == DBGLIB_TRAP_EXIT) {
		mutex_lock(&ctx->mutex);
		complete_all(&ctx->untrap_event);
		reinit_completion(&ctx->data_ready_event);
		mutex_unlock(&ctx->mutex);
	} else if (atomic_read(&ctx->status) != DBGLIB_TRAP_DISABLED) {
		ret = -EINVAL;
		goto end;
	}

end:
	return ret;
}


/* Dcore ioctl function for command RDL_CMD_GET_DIAG_DATA */
static int dcore_get_diag_data(struct file *filp, void *arg)
{
	struct dbglib_diag_data *data_in = (struct dbglib_diag_data *)arg;
	struct dbglib_private *private = (struct dbglib_private *)filp->private_data;
	struct dbglib_diag_data debug_data;
	struct dbglib_trap_gpu_ctx *ctx = NULL;
	int ret = 0;
	void *buffer = NULL;
	uint32_t bdf = 0;

	if (!data_in)
		return -EINVAL;

	/* Copy data from user */
	if (copy_from_user(&debug_data, data_in,
				sizeof(struct dbglib_diag_data))) {
		return -EINVAL;
	}

	bdf = debug_data.in.dbsf;

	ctx = private->ctx;
	if (!ctx) {
		ctx = get_trap_ctx_by_bdf(bdf);
		if (!ctx) {
			DCORE_ERROR("Can't find trap context\n");
			ret = -EINVAL;
			goto end;
		}
	}

	DCORE_DEVICE_INFO(ctx->dev, "Host try to get data from diagnosis data cache\n");

	if (!ctx->proc_info) {
		ret = -EINVAL;
		goto end;
	}

	if (!dcore_check_trap_ctx_active(ctx->proc_info)) {
		if (ctx->proc_info->task == NULL || ctx->proc_info->pid == 0) {
			goto dump;
		} else {
			ret = -EACCES;
			goto end;
		}
	}

	if (wait_for_completion_interruptible(&ctx->data_ready_event)) {
		ret = -EIO;
		goto end;
	}

dump:

	if (ctx->manual_dump_size) {
		debug_data.in.buffer_size = ctx->manual_dump_size;
		ctx->manual_dump_size = 0;
		buffer = ctx->manual_dump_buffer;
		goto copy;
	}

	if (debug_data.in.buffer_size > GIM_DIAG_DATA_DEFAULT_MEM_SIZE) {
		debug_data.in.buffer_size = GIM_DIAG_DATA_DEFAULT_MEM_SIZE;
	}
	buffer = vmalloc(debug_data.in.buffer_size);
	if (!buffer) {
		ret = -ENOMEM;
		goto end;
	}

	ret = amdgv_get_diag_data(NULL, ctx->pf_dbsf,
			buffer, &debug_data.in.buffer_size);
	if (ret) {
		ret = -EINVAL;
		goto end;
	}

copy:
	DCORE_DEVICE_INFO(ctx->dev, "libgv return the cached data size is: %u\n", debug_data.in.buffer_size);

	if (debug_data.in.buffer_size) {
		ret = copy_to_user(debug_data.in.buffer, buffer,
					  debug_data.in.buffer_size);
		if (ret) {
			DCORE_DEVICE_ERROR(ctx->dev, "copy diagnosis data to user failed\n");
			ret = -EINVAL;
			goto end;
		}
		debug_data.out.data_size = debug_data.in.buffer_size;
		debug_data.out.cookie = ctx->cookie;
	} else {
		debug_data.out.data_size = 0;
		debug_data.out.cookie = 0;
	}

end:
	debug_data.out.error_code = abs(ret);

	if (copy_to_user(data_in, &debug_data,
					  sizeof(struct dbglib_diag_data))) {
		ret = -EINVAL;
	}

	if (buffer)
		vfree(buffer);

	return ret;
}

/* Dcore ioctl function for command RDL_CMD_STOP_TRAP_GPU_HANG */
static int dcore_stop_trap_gpu_hang(struct file *filp, void *arg)
{
	struct dbglib_stop_trap_gpu_hang_info *data_in = (struct dbglib_stop_trap_gpu_hang_info *)arg;
	struct dbglib_private *private = (struct dbglib_private *)filp->private_data;
	struct dbglib_stop_trap_gpu_hang_info gpu_info;
	struct dbglib_trap_gpu_ctx *ctx = NULL;
	uint32_t bdf = 0;
	int ret = 0;
	long timeout_ret = 0;

	if (!data_in)
		return -EINVAL;

	/* Copy data from user */
	if (copy_from_user(&gpu_info, data_in,
				sizeof(struct dbglib_stop_trap_gpu_hang_info))) {
		return -EINVAL;
	}

	bdf = gpu_info.in.dbsf;

	ctx = private->ctx;
	if (!ctx) {
		ctx = get_trap_ctx_by_bdf(bdf);
		if (!ctx) {
			DCORE_ERROR("Can't find trap context\n");
			ret = -EINVAL;
			goto end;
		}
	}

	DCORE_DEVICE_INFO(ctx->dev, "Host stop trapping gpu hang\n");

	if (!ctx->proc_info) {
		ret = -EINVAL;
		goto end;
	}

	if (atomic_read(&ctx->status) == DBGLIB_TRAP_DISABLED ||
				atomic_read(&ctx->status) == DBGLIB_TRAP_EXIT) {
		goto reset;
	} else if (atomic_read(&ctx->status) == DBGLIB_TRAP_WAITING) {
		atomic_set(&ctx->status, DBGLIB_TRAP_EXIT);
		mutex_lock(&ctx->mutex);
		ctx->event = DBGLIB_TRAP_EVENT_EXIT;
		ctx->cookie = 0;
		ctx->dbsf = bdf;
		complete(&ctx->trap_event);
		mutex_unlock(&ctx->mutex);
		goto reset;
	} else if (atomic_read(&ctx->status) == DBGLIB_TRAP_DUMPING) {
		if (completion_done(&ctx->untrap_event)) {
			/* If dumping is end, set status to exit */
			atomic_set(&ctx->status, DBGLIB_TRAP_EXIT);
			goto reset;
		} else {
			timeout_ret = wait_for_completion_interruptible_timeout(&ctx->untrap_event,
					msecs_to_jiffies(ctx->untrap_timeout));
			if (timeout_ret == 0) {
				if (ctx->proc_info->task && !IS_ERR(ctx->proc_info->task)) {
					task_lock(ctx->proc_info->task);
					kill_pid(find_get_pid(ctx->proc_info->pid), SIGIO, 1);
					task_unlock(ctx->proc_info->task);
				}
				DCORE_DEVICE_ERROR(ctx->dev, "Host hang dump timed out\n");
				ret = -ETIMEDOUT;
				goto end;
			} else if (timeout_ret > 0) {
				DCORE_DEVICE_INFO(ctx->dev, "libgv waked up from waiting\n");
				goto end;
			} else if (timeout_ret == -ERESTARTSYS) {
				DCORE_DEVICE_ERROR(ctx->dev, "Host threads exit unexpectedly\n");
				ret = -EIO;
				goto end;
			}
		}
	} else {
		ret = -EINVAL;
		goto end;
	}

reset:
	if (atomic_read(&ctx->status) == DBGLIB_TRAP_EXIT) {
		mutex_lock(&ctx->mutex);
		ret = dcore_reset_ctx_status_proc(ctx);  /* Reset this trap context */
		mutex_unlock(&ctx->mutex);
	}

end:
	return ret;
}

static int dcore_get_mes_dbg_info(struct file *filp, void *user_buf)
{
	struct dbglib_mes_dbg_info_block mes_dbg_block = {0};
	int ret = 0;
	struct amd_sriov_msg_vf2pf_info *vf2pf_msg = NULL;
	struct gim_dev_data *dev_data;
	amdgv_dev_t adev = NULL;
	int idx_vf = -1;

	if (copy_from_user(&mes_dbg_block, user_buf, sizeof(struct dbglib_mes_dbg_info_block))) {
		ret = -EINVAL;
		goto out;
	}

	vf2pf_msg = (struct amd_sriov_msg_vf2pf_info *)vmalloc(sizeof(struct amd_sriov_msg_vf2pf_info));
	if (vf2pf_msg == NULL) {
		ret = AMDGV_FAILURE;
		goto out;
	}

	mutex_lock(&gim_device_list_lock);
	list_for_each_entry(dev_data, &gim_device_list, list) {
		idx_vf = gim_dbdf_to_vf_idx(mes_dbg_block.dbsf, dev_data);
		if (-1 != idx_vf) {
			adev = dev_data->adev;
			break;
		}
	}

	if (!adev) {
		DCORE_WARN("can't find device by the bdf\n");
		ret = -EINVAL;
		mutex_unlock(&gim_device_list_lock);
		goto out;
	}

	ret = amdgv_get_vf2pf_info(adev, (uint32_t)idx_vf, vf2pf_msg);
	if (ret == 0 && vf2pf_msg->mes_info_addr) {
		mes_dbg_block.dbg_addr = vf2pf_msg->mes_info_addr;
		mes_dbg_block.dbg_size = vf2pf_msg->mes_info_size;
	}
	mutex_unlock(&gim_device_list_lock);

out:
	if (copy_to_user(user_buf, &mes_dbg_block, sizeof(struct dbglib_mes_dbg_info_block)))
		ret = -EINVAL;
	if (vf2pf_msg)
		vfree(vf2pf_msg);
	if (ret)
		DCORE_WARN("Failed to get mes_dbg_info\n");

	return ret;
}

static long dcore_ioctl_handler(struct file *filp, unsigned int ioctl_num,
							unsigned long arg)
{
	long ret = 0;

	/* Check that the file descriptor is correct */
	if (filp->f_op != &dcore_file_ops)
		return -EINVAL;

	switch (ioctl_num) {
	case GIM_IOC_START_TRAP_GPU_HANG:
		ret = dcore_start_trap_gpu_hang(filp, (void *)arg);
		break;
	case GIM_IOC_NOTIFY_DUMP_DONE:
		ret = dcore_signal_hang_dump_done(filp, (void *)arg);
		break;
	case GIM_IOC_GET_DIAG_DATA:
		ret = dcore_get_diag_data(filp, (void *)arg);
		break;
	case GIM_IOC_GET_FFBM_DATA:
		ret = dcore_get_ffbm_data(filp, (void *)arg);
		break;
	case GIM_IOC_GET_MES_DBG_INFO:
		ret = dcore_get_mes_dbg_info(filp, (void *)arg);
		break;
	case GIM_IOC_STOP_TRAP_GPU_HANG:
		ret = dcore_stop_trap_gpu_hang(filp, (void *)arg);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int dcore_create_trap_ctx(struct gim_dev_data *dev_data)
{
	struct dbglib_trap_gpu_ctx *ctx = NULL;
	int ret = 0;

	ctx = kzalloc(sizeof(struct dbglib_trap_gpu_ctx), GFP_KERNEL);
	if (!ctx) {
		ret = -ENOMEM;
		goto failed;
	}
	ctx->proc_info = kzalloc(sizeof(struct dbglib_process_info), GFP_KERNEL);
	if (!ctx->proc_info) {
		ret = -ENOMEM;
		goto failed;
	}

	/* Init event */
	init_completion(&ctx->trap_event);
	init_completion(&ctx->untrap_event);
	init_completion(&ctx->data_ready_event);
	complete(&ctx->data_ready_event);
	/* Init timeout value */
	ctx->untrap_timeout = gim_conf_get_hangdump_timeout_opt(dev_data->gpu_index);
	/* Init status */
	atomic_set(&ctx->status, DBGLIB_TRAP_DISABLED);
	/* Save the adapter */
	ctx->adev = dev_data->adev;
	ctx->dev = &dev_data->pdev->dev;
	/* Init the mutex */
	mutex_init(&ctx->mutex);
	/* Save the pf's dbsf */
	ctx->pf_dbsf = dev_data->init_data.info.bdf;
	/* Init process info field */
	ctx->proc_info->task = NULL;
	ctx->proc_info->pid = 0;
	ctx->proc_info->tgid = 0;
	/* Init output field */
	ctx->dbsf = 0;
	ctx->event = DBGLIB_TRAP_EVENT_EXIT;
	ctx->error_code = 0;
	ctx->cookie = 0;

	/* Add the ctx to list */
	spin_lock(&lock_ctx_list);
	list_add_tail(&ctx->node, &trap_ctx_list);
	spin_unlock(&lock_ctx_list);

	return ret;

failed:
	if (ctx && ctx->proc_info)
		kfree(ctx->proc_info);
	if (ctx)
		kfree(ctx);

	return ret;
}

static int dcore_trap_gpu_ctx_init(void)
{
	int res = 0;
	struct gim_dev_data *dev_data;

	mutex_lock(&gim_device_list_lock);
	/* Create trap gpu hang ctx for all GPUs */
	list_for_each_entry(dev_data, &gim_device_list, list) {
		res = dcore_create_trap_ctx(dev_data);
		if (res < 0)
			goto err_trap;
	}
	mutex_unlock(&gim_device_list_lock);

	return 0;

err_trap:
	mutex_unlock(&gim_device_list_lock);
	dcore_trap_gpu_ctx_fini();

	return res;
}

static void dcore_trap_gpu_ctx_fini(void)
{
	struct dbglib_trap_gpu_ctx *ctx, *tmp = NULL;

	spin_lock(&lock_ctx_list);
	list_for_each_entry_safe(ctx, tmp, &trap_ctx_list, node) {
		list_del(&ctx->node);
		kfree(ctx->proc_info);
		kfree(ctx);
	}
	spin_unlock(&lock_ctx_list);
}

#if defined(HAVE_DCORE_IOVA_VM_CTX_PAGE_ARRAY)
int dcore_iova_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct iommu_domain *dom;
	struct pci_dev *pdev = (struct pci_dev *)filp->private_data;
	unsigned long pfn;
	unsigned long size;
	unsigned long page_cnt = 0;
	int ret = 0;
	size = vma->vm_end - vma->vm_start;
	if ((size & (PAGE_SIZE - 1)) != 0) {
		DCORE_ERROR(
			"dCore: mapping size needs to be page aligned. skip mapping\n");
		return -EINVAL;
	}

	page_cnt = size >> PAGE_SHIFT;
	if (!page_cnt) {
		DCORE_ERROR("dCore: size should not be 0. skip mapping\n");
		return -EINVAL;
	}

	if (!(vma->vm_flags & VM_SHARED)) {
		DCORE_ERROR("dCore: VM need to be shared. skip mapping\n");
		return -EINVAL;
	}
	dom = iommu_get_domain_for_dev(&pdev->dev);
	if (dom != NULL)
		pfn = iommu_iova_to_phys(dom, (vma->vm_pgoff << PAGE_SHIFT)) >> PAGE_SHIFT;
	else
		pfn = vma->vm_pgoff;

	if (!pfn_valid(pfn)) {
		DCORE_ERROR("dCore: PFN invalid. skip mapping\n");
		return -EPERM;
	}

	ret = remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);
	if (ret) {
		DCORE_ERROR("dCore: failed mmap iova_pfn=0x%lx, ret=0x%x.\n",
			    vma->vm_pgoff, ret);
		return -ENOMEM;
	}
	return 0;
}
#else
static void dcore_iova_vma_close(struct vm_area_struct *vma)
{
	struct dcore_iova_vm_ctx *ctx = (struct dcore_iova_vm_ctx *)vma->vm_private_data;
	unsigned long page_cnt = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
#if defined(HAVE_DCORE_IOVA_VM_CTX_VFIO_DEVICE)
	/* unpin pages */
	vfio_unpin_pages(ctx->vdev, ctx->guest_pfn, page_cnt);

	/* unregister notifier */
	vfio_unregister_notifier(ctx->vdev, VFIO_IOMMU_NOTIFY,
			&ctx->vfio_notifier);
#else
	/* unpin pages */
	vfio_unpin_pages(&ctx->pdev->dev, ctx->guest_pfn, page_cnt);

	/* unregister notifier */
	vfio_unregister_notifier(&ctx->pdev->dev, VFIO_IOMMU_NOTIFY,
			&ctx->vfio_notifier);
#endif

	/* free vm context */
	vfree(ctx->host_pfn);
	vfree(ctx->guest_pfn);
	vfree(ctx);
}

static vm_fault_t dcore_iova_vma_fault(struct vm_fault *vmf)
{
	struct dcore_iova_vm_ctx *ctx = (struct dcore_iova_vm_ctx *)vmf->vma->vm_private_data;
	unsigned long page_cnt = (vmf->vma->vm_end - vmf->vma->vm_start) >> PAGE_SHIFT;
	unsigned long index = vmf->pgoff - ctx->guest_pfn[0];
	unsigned long pfn;

	if (vmf->pgoff < ctx->guest_pfn[0] || vmf->pgoff > ctx->guest_pfn[page_cnt - 1]) {
		DCORE_ERROR("Page offset 0x%lx out of range\n", vmf->pgoff);
		goto failed;
	}

	pfn = ctx->host_pfn[index];
	return vmf_insert_pfn(vmf->vma, vmf->address, pfn);
failed:
	return VM_FAULT_SIGSEGV;
}

static struct vm_operations_struct dcore_vma_ops = {
	.close = dcore_iova_vma_close,
	.fault = dcore_iova_vma_fault
};
static int dcore_vfio_iommu_notifier(struct notifier_block *nb,
		unsigned long action, void *opaque)
{
	return NOTIFY_OK;
}

int dcore_iova_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct pci_dev *pdev = (struct pci_dev *)filp->private_data;
	struct dcore_iova_vm_ctx *vm_ctx;
	unsigned long *guest_pfn, *host_pfn;
	unsigned long page_cnt = 0;
	int ret;
	int i;
#if defined(HAVE_DCORE_IOVA_VM_CTX_VFIO_DEVICE)
	struct vfio_pci_core_device *vpdev = dev_get_drvdata(&pdev->dev);

	if (!vpdev || vpdev->pdev != pdev) {
		DCORE_ERROR("Can not get vfio device\n");
		return -EINVAL;
	}
#endif

	if (((vma->vm_end - vma->vm_start) & (PAGE_SIZE - 1)) != 0) {
		DCORE_ERROR("Mapping size needs to be page aligned. skip mapping\n");
		return -EINVAL;
	}

	page_cnt = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;

	if (!page_cnt) {
		DCORE_ERROR("Size should not be 0. skip mapping\n");
		return -EINVAL;
	}

	if (!(vma->vm_flags & VM_SHARED)) {
		DCORE_ERROR("VM need to be shared. skip mapping\n");
		return -EINVAL;
	}

	vm_ctx = vmalloc(sizeof(struct dcore_iova_vm_ctx));
	guest_pfn = vmalloc(sizeof(unsigned long) * page_cnt);
	host_pfn = vmalloc(sizeof(unsigned long) * page_cnt);

	if (!vm_ctx || !guest_pfn || !host_pfn) {
		DCORE_ERROR("Can't allocate memory. probably out of memory\n");
		ret = -ENOMEM;
		goto failed;
	}

	for (i = 0; i < page_cnt; i++) {
		guest_pfn[i] = vma->vm_pgoff + i;
	}

#if defined(HAVE_DCORE_IOVA_VM_CTX_VFIO_DEVICE)
	vm_ctx->vdev = &vpdev->vdev;
	vm_ctx->vfio_events = VFIO_IOMMU_NOTIFY_DMA_UNMAP;
#else
	vm_ctx->vfio_events = VFIO_GROUP_NOTIFY_SET_KVM;
#endif
	vm_ctx->host_pfn = host_pfn;
	vm_ctx->vfio_notifier.notifier_call = dcore_vfio_iommu_notifier;
	vm_ctx->pdev = pdev;
	vm_ctx->guest_pfn = guest_pfn;

#if defined(HAVE_DCORE_IOVA_VM_CTX_VFIO_DEVICE)
	/* we need to register a dummy notifier, otherwise the pin pages won't work */
	ret = vfio_register_notifier(vm_ctx->vdev, VFIO_IOMMU_NOTIFY,
			&vm_ctx->vfio_events, &vm_ctx->vfio_notifier);

	/* pin pages */
	ret = vfio_pin_pages(vm_ctx->vdev, guest_pfn, page_cnt, IOMMU_READ, host_pfn);
#else
	/* we need to register a dummy notifier, otherwise the pin pages won't work */
	ret = vfio_register_notifier(&vm_ctx->pdev->dev, VFIO_IOMMU_NOTIFY,
			&vm_ctx->vfio_events, &vm_ctx->vfio_notifier);

	/* pin pages */
	ret = vfio_pin_pages(&pdev->dev, guest_pfn, page_cnt, IOMMU_READ, host_pfn);
#endif
	if (ret != page_cnt) {
		DCORE_ERROR("Failed pin iova_pfn=0x%lx, cnt=0x%lx ,ret=0x%x.\n", vma->vm_pgoff, page_cnt, ret);
		ret = -EBUSY;
		goto unreg;
	}

	vma->vm_ops = &dcore_vma_ops;
	vma->vm_private_data = (void *)vm_ctx;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP | VM_PFNMAP;

	return 0;

unreg:
#if defined(HAVE_DCORE_IOVA_VM_CTX_VFIO_DEVICE)
	vfio_unregister_notifier(vm_ctx->vdev, VFIO_IOMMU_NOTIFY,
			&vm_ctx->vfio_notifier);
#else
	vfio_unregister_notifier(&vm_ctx->pdev->dev, VFIO_IOMMU_NOTIFY,
			&vm_ctx->vfio_notifier);
#endif

failed:
	if (vm_ctx)
		vfree(vm_ctx);

	if (guest_pfn)
		vfree(guest_pfn);

	if (host_pfn)
		vfree(host_pfn);

	return -ENOMEM;
}
#endif

static int dcore_iova_open(struct inode *inode, struct file *filp)
{
	struct iova_node *node = container_of(inode->i_cdev,
				struct iova_node, iova_cdev);
	filp->private_data = node->pdev;
	return 0;
}

static const struct file_operations iova_file_ops = {
	.owner                  = THIS_MODULE,
	.open                   = dcore_iova_open,
	.mmap                   = dcore_iova_mmap
};

static int dcore_iova_node_create(struct pci_dev *pdev, int minor)
{
	int res;
	int devno;
	struct iova_node *node;

	node = kzalloc(sizeof(struct iova_node), GFP_KERNEL);
	if (!node) {
		res = -ENOMEM;
		goto failed;
	}
	devno = MKDEV(MAJOR(dcore.devid), minor);

	node->dev = device_create(dcore.dcore_class, NULL, devno, NULL, pci_name(pdev));

	cdev_init(&node->iova_cdev, &iova_file_ops);
	res = cdev_add(&node->iova_cdev, devno, 1);
	if (res < 0)
		goto err_cdev;

	kobject_uevent(&node->iova_cdev.kobj, KOBJ_ADD);

	node->pdev = pdev;
	spin_lock(&dcore.lock_iova_list);
	list_add(&node->list, &dcore.iova_list);
	spin_unlock(&dcore.lock_iova_list);

	return 0;

err_cdev:
	device_destroy(dcore.dcore_class, node->dev->devt);
failed:
	if (node)
		kfree(node);
	return res;
}

static int dcore_iova_node_init(void)
{
	int res = 0;
	int minor = 0;
	int i;
	struct pci_dev *pdev_vf;
	struct pci_dev *pdev_pf;
	struct gim_dev_data *dev_data;

	minor = DCORE_IOVA_MINOR_OFFSET;

	INIT_LIST_HEAD(&dcore.iova_list);
	spin_lock_init(&dcore.lock_iova_list);

	/* iova_node for all GPUs */
	list_for_each_entry(dev_data, &gim_device_list, list) {
		pdev_pf = dev_data->pdev;
		/* PF */
		res = dcore_iova_node_create(pdev_pf, minor++);
		if (res < 0)
			goto err_iova;

		for (i = 0; i < dev_data->vf_num; i++) {
			pdev_vf = dev_data->vf_map[i].pdev;
			/* VF */
			res = dcore_iova_node_create(pdev_vf, minor++);
			if (res < 0)
				goto err_iova;
		}
	}

	return 0;

err_iova:
	dcore_iova_node_fini();
	return res;
}

static void dcore_iova_node_fini(void)
{
	struct iova_node *node, *tmp;

	list_for_each_entry_safe(node, tmp, &dcore.iova_list, list) {
		kobject_uevent(&node->iova_cdev.kobj, KOBJ_REMOVE);
		cdev_del(&node->iova_cdev);
		device_destroy(dcore.dcore_class, node->dev->devt);
		kfree(node);
	}
}


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

#ifndef __DCORE_DRV_H__
#define __DCORE_DRV_H__

#include <amdgv_api.h>

#define DCORE_DEVICE_NAME "debug"

#define DCORE_IOVA_MINOR_OFFSET		   1
#define DCORE_COMM_LEN 16

/* log header */
#define DCORE_INFO_HEADER   "gim info dcore: "
#define DCORE_WARN_HEADER   "gim warning dcore: "
#define DCORE_ERR_HEADER    "gim error dcore: "

#define DCORE_ERROR(fmt, ...)                                                 \
	do {                                                                  \
		printk(KERN_ERR DCORE_ERR_HEADER "[%s:%d] " fmt,              \
			  __func__, __LINE__, ##__VA_ARGS__);                 \
	} while (0)

#define DCORE_WARN(fmt, ...)                                                  \
	do {                                                                  \
		printk(KERN_WARNING DCORE_WARN_HEADER "[%s:%d] " fmt,         \
			  __func__, __LINE__, ##__VA_ARGS__);                 \
	} while (0)

#define DCORE_INFO(fmt, ...)                                                  \
	do {                                                                  \
		printk(KERN_INFO DCORE_INFO_HEADER "[%s:%d] " fmt,            \
			  __func__, __LINE__, ##__VA_ARGS__);                 \
	} while (0)

#define DCORE_DEVICE_INFO(dev, fmt, ...)                                          \
	do {                                                                      \
		if (dev)                                                          \
			dev_info(dev, "[%s:%d] " fmt,                             \
			__func__, __LINE__, ##__VA_ARGS__);                   \
	} while (0)

#define DCORE_DEVICE_ERROR(dev, fmt, ...)                                         \
	do {                                                                      \
		if (dev)                                                          \
			dev_err(dev, "[%s:%d] " fmt,                              \
			__func__, __LINE__, ##__VA_ARGS__);                   \
	} while (0)

/* MINOR_COUNT cannot be 0, otherwise unregister cannot remove chrdev */
#define MINOR_COUNT (MINORMASK + 1)

struct dcore_iova_vm_ctx {
	struct pci_dev *pdev;
#if defined(HAVE_DCORE_IOVA_VM_CTX_VFIO_DEVICE)
	struct vfio_device *vdev;
#endif
#if defined(HAVE_DCORE_IOVA_VM_CTX_PAGE_ARRAY)
	struct page **page_array;
#else
	struct notifier_block vfio_notifier;
	unsigned long vfio_events;
	unsigned long *host_pfn;
#endif
	unsigned long *guest_pfn;
};

struct dbglib_trap_gpu_ctx;
struct dbglib_private {
	struct dbglib_trap_gpu_ctx *ctx;
};

struct dbglib_process_info {
	struct task_struct *task;
	pid_t tgid;
	pid_t pid;
	char process_name[DCORE_COMM_LEN];
};

enum dbglib_trap_event {
	DBGLIB_TRAP_EVENT_ERROR,
	DBGLIB_TRAP_EVENT_RESET,
	DBGLIB_TRAP_EVENT_MANUAL_DUMP,
	DBGLIB_TRAP_EVENT_EXIT
};

enum dbglib_trap_status {
	DBGLIB_TRAP_DISABLED,      /* trap is temporary disabled*/
	DBGLIB_TRAP_WAITING,       /* trap is waiting for hang */
	DBGLIB_TRAP_DUMPING,       /* trap is waiting for dcore to finish dump */
	DBGLIB_TRAP_EXIT           /* trap has exited */
};

struct dbglib_trap_gpu_ctx {
	void *adev;             /* amdgv adapter */
	struct device *dev;  /* pci adapter */
	uint32_t pf_dbsf;       /* PF's dbsf of this adapter */

	struct completion trap_event;       /* completion for user thread to wait, kernel to wake up upon hang/exit/error happens */
	struct completion untrap_event;     /* completion for kernel thread to wait, user to wake up upon dcore’s dump done*/
	struct completion data_ready_event; /* completion for user thread to wait, kernel to wake up upon host/fw dump done */
	uint32_t untrap_timeout;    /* timeout for untrap_event */
	atomic_t status;    /* current trap context status */
	struct mutex mutex;  /* mutex for this structure */
	struct dbglib_process_info *proc_info;  /* process data form shim layer, normally store some process information */

	/* output for notification */
	uint32_t dbsf;      /* device id who triggered the notification */
	uint32_t idx_vf;    /* vf index */
	enum dbglib_trap_event event;     /* event id */
	uint32_t error_code;
	uint32_t cookie;    /* cookie to sync with diagnosis data */

	struct list_head node;    /* list node is used to find the existing context */
	void *manual_dump_buffer; /* manual dump log buffer */
	uint32_t manual_dump_size; /* manual dump log size */
	struct list_head manual_dump_node;    /* this node is used to record the manual dump ctx */

};

int dcore_init(void);
void dcore_cleanup(void);
int dcore_iova_mmap(struct file *filp, struct vm_area_struct *vma);
void dcore_signal_reset_happened(amdgv_dev_t dev, uint32_t idx_vf);  /* oss_interface - libgv signal the reset happened and wait until dcore done the hang dump */
void dcore_signal_manual_dump_happened(amdgv_dev_t dev, uint32_t idx_vf); /* oss_interface - libgv signal the manual dump and wait until dcore done the manual dump*/
void dcore_signal_diag_data_ready(amdgv_dev_t dev);  /* oss_interface - libgv signal the that diagnosis data are ready */
bool dcore_diag_data_collect_disabled(amdgv_dev_t dev, uint32_t bdf);  /* oss_interface - libgv to check if legacy diagnosis data collect should be disabled*/

#endif

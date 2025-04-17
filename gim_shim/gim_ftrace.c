/*
 * Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved.
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

#include <linux/module.h>
#include <linux/utsname.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/random.h>
#include <linux/kernel.h>
#include <linux/ftrace.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/kthread.h>

#include "amdgv_oss.h"
#include "gim_debug.h"
#include "gim_ftrace.h"
#include "gim.h"

#define GIM_DIAG_DATA_RB_INDEX(wr_count, total_entries) \
	((total_entries & (total_entries - 1)) ? \
		wr_count % total_entries : \
		((wr_count) & (total_entries - 1)))

#define GIM_FTRACE_GET_BUFFER(index, ftrace_buff, base, size, entry_size) { \
	ftrace_buff.buff = base + index; \
	ftrace_buff.w_count = 0; \
	index += size; \
	ftrace_buff.total_entries = \
	       (size/entry_size); \
}

struct gim_ftrace *ftrace_ctx;
struct task_struct *ftrace_exclude_thread;
DECLARE_WAIT_QUEUE_HEAD(ftrace_thread_stop_event);

char *gim_exclude_list[] = {
	"gim_spin_lock",
	"gim_spin_unlock",
	"gim_spin_lock_irq",
	"gim_spin_unlock_irq",
	"gim_strncmp",
	"gim_strlen",
	"gim_strnlen",
	"gim_vsnprintf",
	"gim_print",
	"gim_get_utc_time_stamp",
	"gim_get_time_stamp",
	"gim_yield",
	"gim_msleep",
	"gim_usleep",
	"gim_udelay",
	"gim_pci_find_next_ext_cap",
	"gim_pci_find_ext_cap",
	"gim_pci_find_cap",
	"gim_pci_write_config_dword",
	"gim_pci_write_config_word",
	"gim_pci_write_config_byte",
	"gim_pci_read_config_dword",
	"gim_pci_read_config_word",
	"gim_pci_read_config_byte",
	"gim_io_writel",
	"gim_io_writew",
	"gim_io_writeb",
	"gim_io_readl",
	"gim_io_readw",
	"gim_io_readb",
	"gim_mm_readl",
	"gim_mm_writel",
	"gim_thread_should_stop",
	"gim_atomic_init",
	"gim_atomic_inc_return",
	"gim_atomic_set",
	"mutex_unlock",
	"gim_thread_should_stop",
	"ktime_get",
	"atom_op_eot",
	"atom_iio_execute",
	"atom_get_src_int",
	"atom_put_dst",
	"atom_op_beep",
	"atom_op_repeat",
	"atom_op_div32",
	"atom_op_mul32",
	"atom_op_processds",
	"atom_op_debug",
	"atom_op_shr",
	"atom_op_shl",
	"atom_op_xor",
	"atom_op_setdatablock",
	"atom_op_postcard",
	"atom_op_clear",
	"atom_op_calltable",
	"atom_op_delay",
	"atom_op_test",
	"atom_op_jump",
	"atom_op_switch",
	"atom_op_compare",
	"atom_op_setfbbase",
	"atom_op_sub",
	"atom_op_add",
	"atom_op_div",
	"atom_op_mul",
	"atom_op_or",
	"atom_op_and",
	"atom_get_src_direct",
	"atom_op_mask",
	"atom_op_shift_right",
	"atom_op_shift_left",
	"atom_op_setregblock",
	"atom_op_setport",
	"atom_op_savereg",
	"atom_op_nop",
	"atom_op_move",
	"atom_op_restorereg"
};

static void notrace gim_ftrace_callback(unsigned long ip,
		unsigned long parent_ip, struct ftrace_ops *ops,
#if !defined(HAVE_FTRACE_REGS_OPS)
		struct pt_regs *regs)
#else
		struct ftrace_regs *regs)
#endif
{
	uint32_t index;
	uint32_t w_count;
	struct gim_ftrace_log_entry *entry;
	struct gim_ftrace *g_ftrace;

	g_ftrace = ops->private;
	if (g_ftrace->trace_suspend)
		return;

	/* Read at the beginning to avoid lock, this will make w_count
	 * one ahead of the slot which is adjusted later in local
	 * variable w_count -= 1
	 */
	w_count = atomic_inc_return(&g_ftrace->ftrace_entries.w_count_atomic);

	/* Slight probability of race condition when wr_count wraps around
	 * But in my opinion that risk of corrupting one entry (in case of
	 * corner case i.e., u64 overflow) is worth rather than introducing
	 * lock for every function call in host driver as ftrace CB will
	 * be called from all the function replacing mcount i.e. gcc -pg
	 */
	if (w_count == 0) {
		atomic_add(g_ftrace->ftrace_entries.total_entries,
				&g_ftrace->ftrace_entries.w_count_atomic);
		w_count = g_ftrace->ftrace_entries.total_entries;
	}
	w_count -= 1;

	/* Get the index atomically */
	index = ((w_count) & (g_ftrace->ftrace_entries.total_entries - 1));

	/* Fill int the entry */
	entry = (struct gim_ftrace_log_entry *)
		g_ftrace->ftrace_entries.buff;
	entry += index;
	entry->trace_entry.pid = current->pid;
	entry->trace_entry.tgid = current->tgid;
	entry->trace_entry.func_addr = ip;
	entry->trace_entry.parent_func_addr = parent_ip;
	entry->trace_entry.time_stamp = ktime_to_us(ktime_get());
}

static void notrace gim_ftrace_add_module_functions(
		struct ftrace_ops *ops, uint8_t remove)
{
	int i;
	struct gim_ftrace_func_map *func_entry;
	struct module *mod;
	struct mod_kallsyms *kallsyms;
	int ret = 0;
	/* reset filter is set to 1 unless the entry is been removed */
	uint32_t reset_filter = !remove;

	if (ftrace_ctx->gen_func_map && remove == 0)
		return;

	mod = THIS_MODULE;
	if (mod->state == MODULE_STATE_UNFORMED)
		return;


	func_entry = (struct gim_ftrace_func_map *)
		ftrace_ctx->func_map_entries.buff;
	kallsyms = mod->kallsyms;
	for (i = 0; i < kallsyms->num_symtab; i++) {
		const Elf_Sym *sym = &kallsyms->symtab[i];

		if (sym->st_shndx == SHN_UNDEF)
			continue;

		ret = ftrace_set_filter_ip(ops, sym->st_value, remove,
				reset_filter);

		/* Check for the conditions
		 * ret = 0 means it was a function and we are able to trace it
		 * remove = 0 means the call is to add filter not to remove
		 */
		if (ret == 0 && remove == 0) {
			/* Fill in the entries */
			func_entry->func_addr = sym->st_value;
			strscpy(func_entry->func_name,
				kallsyms->strtab + kallsyms->symtab[i].st_name,
				GIM_FTRACE_FUNC_NAME_LENGTH);
			ftrace_ctx->func_map_entries.w_count++;
			func_entry += 1;

			/* Set the reset filter to 0 so the rest entries are apended */
			if (reset_filter)
				reset_filter = 0;
		}

		/* breaks out of the loop once all the entries are filled */
		if (ftrace_ctx->func_map_entries.w_count >=
				ftrace_ctx->func_map_entries.total_entries)
			break;

	}

	if (ftrace_ctx->func_map_entries.w_count)
		ftrace_ctx->gen_func_map = 1;
}

static void notrace gim_ftrace_exclude(char **exclude_list,
		uint32_t exclude_len)
{
	int err = 0;
	int i = 0;

	if (!ftrace_ctx)
		return;

	if (exclude_len && exclude_list) {
		for (i = 0; i < exclude_len; i++) {
			if (kthread_should_stop())
				return;
			/* Set filter which functions to exclude */
			err = ftrace_set_notrace(&ftrace_ctx->ops,
					exclude_list[i],
					strlen(exclude_list[i]), 0);
			if (err)
				gim_dbg("Set ftrace no filter failed %d %s\n",
						err, exclude_list[i]);
		}
	}
}

static int notrace gim_ftrace_exclude_list(void *context)
{
	uint32_t gim_exclude_len;
	char **exclude_list;
	uint32_t exclude_len;
	struct gim_ftrace *ftrace_ctx;
	int ret_val = 0;

	ftrace_ctx = (struct gim_ftrace *)context;
	if (!ftrace_ctx) {
		ret_val = -1;
		goto stop;
	}

	/* GIM list to exclude from trace */
	gim_exclude_len =
		(sizeof (gim_exclude_list) / sizeof (const char *));
	gim_ftrace_exclude(gim_exclude_list, gim_exclude_len);

	if (kthread_should_stop())
		goto stop;

	/* Host driver/asic list to exclude */
	amdgv_device_exclude_list(&exclude_list, &exclude_len);
	gim_ftrace_exclude(exclude_list, exclude_len);

stop:
	wait_event_interruptible(ftrace_thread_stop_event, kthread_should_stop());
	return ret_val;

}

static int notrace gim_ftrace_start(void)
{
	/* Init ftrace ops */
	ftrace_ctx->ops.func = gim_ftrace_callback;
	ftrace_ctx->ops.private = ftrace_ctx;

	/* Set filter to trace all gim functions */
	gim_ftrace_add_module_functions(&ftrace_ctx->ops, 0);

	ftrace_ctx->trace_suspend = false;

	/* enable the ftrace callback */
	register_ftrace_function(&ftrace_ctx->ops);

	ftrace_exclude_thread = kthread_run(gim_ftrace_exclude_list,
			(void *)ftrace_ctx, "ftrace_exclude_thread");

	if (IS_ERR(ftrace_exclude_thread))
		gim_warn("failed to create ftrace exclude thread!\n");

	return 0;
}

int gim_ftrace_init(amdgv_dev_t *adapt_list)
{
	uint32_t total_entries = 0;
	uint32_t buf_idx = 0;

	/* Allocate ftrace context */
	ftrace_ctx = kzalloc(sizeof(struct gim_ftrace), GFP_KERNEL);
	if (!ftrace_ctx) {
		gim_warn("Unable to init ftrace context allocation failed\n");
		return -1;
	}

	/* Allocate ftrace buffer -- large buffer and does not have to be contig */
	ftrace_ctx->mem_blk = vzalloc(GIM_FTRACE_LOG_SIZE);
	if (ftrace_ctx->mem_blk == NULL) {
		gim_warn("Unable to get memory for ftrace block\n");
		kfree(ftrace_ctx);
		ftrace_ctx = NULL;
		return -1;
	}

	/* Assign region for init task entries */
	GIM_FTRACE_GET_BUFFER(buf_idx, ftrace_ctx->init_task_entries,
			ftrace_ctx->mem_blk, GIM_FTRACE_INIT_TASK_SIZE,
			sizeof(struct gim_ftrace_task_entry));

	/* Assign region for usr task entries */
	GIM_FTRACE_GET_BUFFER(buf_idx, ftrace_ctx->usr_task_entries,
			ftrace_ctx->mem_blk, GIM_FTRACE_USR_TASK_SIZE,
			sizeof(struct gim_ftrace_task_entry));

	/* Assign region for function entries */
	GIM_FTRACE_GET_BUFFER(buf_idx, ftrace_ctx->func_map_entries,
			ftrace_ctx->mem_blk, GIM_FTRACE_FUNC_MAP_SIZE,
			sizeof(struct gim_ftrace_func_map));

	/* Assign region for ftrace entries */
	GIM_FTRACE_GET_BUFFER(buf_idx, ftrace_ctx->ftrace_entries,
			ftrace_ctx->mem_blk, GIM_FTRACE_LOG_BUF_SIZE,
			sizeof(struct gim_ftrace_trace_entry));
	atomic_set(&ftrace_ctx->ftrace_entries.w_count_atomic, 0);

	/* Convert total entries to power of 2
	 * this is done so that to calculate the
	 * write index of ftrace ring buffer we do not have
	 * to use modulus since it is expensive and done
	 * from the ftrace CB
	 */

	total_entries = ftrace_ctx->ftrace_entries.total_entries;
	ftrace_ctx->ftrace_entries.total_entries =
			rounddown_pow_of_two(total_entries);

	/* Add current task to task entry */
	gim_ftrace_add_task_entry(current, 0, 0);

	ftrace_ctx->adapt_list = adapt_list;
	ftrace_ctx->gen_func_map = 0;
	if (gim_ftrace_start()) {
		gim_warn("Unable to start ftrace\n");
		if (ftrace_ctx->mem_blk) {
			vfree(ftrace_ctx->mem_blk);
			ftrace_ctx->mem_blk = NULL;
		}

		kfree(ftrace_ctx);
		ftrace_ctx = NULL;
		return -1;
	}

	return 0;
}

void notrace gim_ftrace_fini(void)
{
	if (ftrace_exclude_thread != NULL && !IS_ERR(ftrace_exclude_thread))
		kthread_stop(ftrace_exclude_thread);

	if (!ftrace_ctx)
		return;

	/* Stop the callback */
	if (unregister_ftrace_function(&ftrace_ctx->ops))
		gim_warn("Unregister ftrace failed\n");

	/* Remove all function from the filter */
	gim_ftrace_add_module_functions(&ftrace_ctx->ops, 1);

	if (ftrace_ctx->mem_blk) {
		vfree(ftrace_ctx->mem_blk);
		ftrace_ctx->mem_blk = NULL;
	}

	kfree(ftrace_ctx);
	ftrace_ctx = NULL;
}

void gim_ftrace_add_task_entry(void *task, unsigned int gpu_bdf,
		int usr_level_thread)
{
	struct gim_ftrace_task_entry *t_entry;
	struct task_struct *mod_task = (struct task_struct *)task;
	uint32_t index = 0;
	struct ftrace_buff *task_buff;

	if (!ftrace_ctx)
		return;

	if (usr_level_thread)
		task_buff = &ftrace_ctx->usr_task_entries;
	else
		task_buff = &ftrace_ctx->init_task_entries;

	/* Get the slot in respective task entry region for the new entry */
	index = GIM_DIAG_DATA_RB_INDEX(task_buff->w_count,
			task_buff->total_entries);
	t_entry = (struct gim_ftrace_task_entry *)task_buff->buff;
	t_entry += index;
	task_buff->w_count++;

	/* Fill in the contents */
	t_entry->pid = mod_task->pid;
	t_entry->tgid = mod_task->tgid;
	t_entry->gpu_bdf = gpu_bdf;
	strscpy(t_entry->task_name, mod_task->comm,
			GIM_FTRACE_TASK_NAME_LENGTH);
}

static uint32_t gim_ftrace_get_task_list(void *buf,
		amdgv_dev_t *adapt_list)
{
	int count;
	int gpu_idx;
	int i;
	struct task_struct *task;
	void *task_list[AMDGV_GPU_THREAD_COUNT];
	static amdgv_dev_t adapt;
	struct gim_ftrace_log_entry *entry;
	uint32_t entries = 0;
	uint32_t bdf;

	if (!buf || !adapt_list)
		return 0;

	entry = buf;
	for (gpu_idx = 0; gpu_idx < AMDGV_MAX_GPU_NUM; gpu_idx++) {
		adapt = adapt_list[gpu_idx];
		if (adapt) {
			count = amdgv_list_gpu_threads(adapt, task_list,
					AMDGV_GPU_THREAD_COUNT, &bdf);

			/* Get libgv threads */
			for (i = 0; i < count; i++) {
				task = (struct task_struct *)task_list[i];
				if (!task) {
					gim_dbg("Task not created/Died %d\n", i);
					continue;
				}
				entry->task_entry.pid = task->pid;
				entry->task_entry.tgid = task->tgid;
				entry->task_entry.gpu_bdf = bdf;
				strscpy(entry->task_entry.task_name, task->comm,
						GIM_FTRACE_TASK_NAME_LENGTH);
				entry++;
				entries++;
			}
		}
	}

	/* Get module init threads */
	if (ftrace_ctx->init_task_entries.w_count >
			ftrace_ctx->init_task_entries.total_entries)
		count = ftrace_ctx->init_task_entries.total_entries;
	else
		count = ftrace_ctx->init_task_entries.w_count;
	if (count) {
		memcpy(entry, ftrace_ctx->init_task_entries.buff,
			sizeof(struct gim_ftrace_task_entry) * count);
		entries += count;
		entry += count;
	}

	/* Get usr level threads */
	if (ftrace_ctx->usr_task_entries.w_count >
			ftrace_ctx->usr_task_entries.total_entries)
		count = ftrace_ctx->usr_task_entries.total_entries;
	else
		count = ftrace_ctx->usr_task_entries.w_count;
	if (count) {
		memcpy(entry, ftrace_ctx->usr_task_entries.buff,
			sizeof(struct gim_ftrace_task_entry) * count);
		entries += count;
	}

	return entries;
}

static void gim_ftrace_copy_data(void *trace_buf,
		uint32_t buf_len, void *entries,
		uint32_t entry_size, uint32_t log_entries, uint32_t w_count)
{
	uint8_t *src_addr;
	uint32_t copy_len;
	uint32_t w_idx;

	if (!ftrace_ctx || !trace_buf || !buf_len)
		return;

	if (w_count < log_entries) {
		src_addr = (uint8_t *)entries;
		copy_len = w_count * entry_size;
		if (copy_len > buf_len)
			copy_len = buf_len;
		memcpy(trace_buf, src_addr, copy_len);
	} else {
		w_idx = GIM_DIAG_DATA_RB_INDEX(w_count, log_entries);
		/* Copy the oldest entries i.e, w_count to size */
		src_addr = (uint8_t *)entries + (w_idx * entry_size);
		copy_len = (log_entries - w_idx) * entry_size;
		if (copy_len > buf_len)
			copy_len = buf_len;
		memcpy(trace_buf, src_addr, copy_len);
		trace_buf = (uint8_t *)trace_buf + copy_len;
		buf_len -= copy_len;

		/* return if no space left */
		if (!buf_len)
			return;

		/* Copy rest i.e., between first entry/keep and wr_count */
		src_addr = (uint8_t *)entries;
		copy_len = w_idx * entry_size;
		if (copy_len > buf_len)
			copy_len = buf_len;
		memcpy(trace_buf, src_addr, copy_len);
		trace_buf += copy_len;
	}
}

int notrace gim_ftrace_get_buffer(void *trace_buf,
		uint32_t trace_buf_len)
{
	int copy_len = 0;
	int data_len;
	uint32_t task_entries_size;
	uint32_t task_entries;
	uint32_t func_entries_size;
	uint32_t func_entries;
	uint32_t cur_index;
	uint32_t entry_size;
	uint32_t used_size = 0;
	uint32_t keep_entries = 0;
	struct gim_ftrace_header_entry *hdr_entry;

	entry_size = sizeof(struct gim_ftrace_log_entry);

	if (!ftrace_ctx || !trace_buf || !trace_buf_len)
		return 0;

	/* Stop the ftracing for data collection */
	ftrace_ctx->trace_suspend = true;

	cur_index = atomic_read(&ftrace_ctx->ftrace_entries.w_count_atomic);
	if (cur_index == 0)
		goto ftrace_get_buf_ret;

	/* First entry is header */
	if (trace_buf_len < entry_size) {
		gim_warn("Buffer is not big enough to hold header entry\n");
		goto ftrace_get_buf_ret;
	}
	hdr_entry = trace_buf;
	trace_buf += sizeof(*hdr_entry);
	trace_buf_len -= sizeof(*hdr_entry);

	/* The buffer passed should be big enough to hold task entries */
	task_entries_size =
		AMDGV_MAX_GPU_NUM * AMDGV_GPU_THREAD_COUNT * entry_size;
	if (trace_buf_len < task_entries_size) {
		gim_warn("Buffer is not big enough to hold task entries\n");
		goto ftrace_get_buf_ret;
	}

	/* Copy the task entries and update the size*/
	task_entries = gim_ftrace_get_task_list(trace_buf,
			ftrace_ctx->adapt_list);
	task_entries_size = task_entries * entry_size;

	/* Update the buffer pointer and remaining length */
	trace_buf = trace_buf + task_entries_size;
	trace_buf_len = trace_buf_len - task_entries_size;

	/* Copy the function entries */
	func_entries = ftrace_ctx->func_map_entries.w_count;
	func_entries_size = ftrace_ctx->func_map_entries.w_count *
				sizeof(struct gim_ftrace_func_map);
	if (trace_buf_len < func_entries_size) {
		gim_warn("Buffer is not big enough to hold function mapping\n");
		goto ftrace_get_buf_ret;
	}
	memcpy(trace_buf, ftrace_ctx->func_map_entries.buff, func_entries_size);

	/* Update the buffer pointer and remaining length */
	trace_buf = trace_buf + func_entries_size;
	trace_buf_len = trace_buf_len - func_entries_size;

	/* Get the length of total available data */
	if (cur_index >= ftrace_ctx->ftrace_entries.total_entries)
		data_len = (ftrace_ctx->ftrace_entries.total_entries * entry_size);
	else
		data_len = (cur_index * entry_size);

	/* Update the length to copy depending on the room in buffer */
	if (data_len > trace_buf_len)
		copy_len = trace_buf_len;
	else
		copy_len = data_len;
	gim_ftrace_copy_data(trace_buf,
		copy_len, ftrace_ctx->ftrace_entries.buff,
		entry_size, ftrace_ctx->ftrace_entries.total_entries,
		cur_index);

	/* Fill header */
	hdr_entry->task_entries = task_entries;
	hdr_entry->func_entries = func_entries;
	/* Add the number of trace entries
	 * copy_len contains the data that can be copied
	 * to the provided buffer. The passed buffer might
	 * not have the size that is exact multiple of entry_size
	 * so we keep the trace entries until upto the last
	 * full entry.
	 */
	hdr_entry->trace_entries = copy_len / entry_size;

	/* (func_entries * 2) is because each function
	 * entry is equal to 2 slots
	 * + 1 is for the single header entry
	 */
	keep_entries = task_entries + (func_entries * 2) + 1;
	used_size = copy_len + (keep_entries * entry_size);

ftrace_get_buf_ret:
	/* Start the ftracing for data collection */
	ftrace_ctx->trace_suspend = false;

	return used_size;
}


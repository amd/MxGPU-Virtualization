/*
 * Copyright (c) 2014-2019 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef _GPU_IOV_MODULE__FTRACE_H
#define _GPU_IOV_MODULE__FTRACE_H

/* FTRACE block size of 1M */
#define GIM_FTRACE_LOG_SIZE	(1024 * 1024)

#define GIM_FTRACE_GPU_TASK_ENTRIES	4
#define GIM_FTRACE_TASK_NAME_LENGTH	20
#define GIM_FTRACE_FUNC_NAME_LENGTH	56

#define GIM_FTRACE_FUNC_MAP_SIZE		(256 * 1024)
#define GIM_FTRACE_USR_TASK_SIZE		(8 * 1024)
#define GIM_FTRACE_INIT_TASK_SIZE		(8 * 1024)
#define GIM_FTRACE_LOG_BUF_SIZE			(752 * 1024)

/* Compile time check tom make sure that buffer is large enough to hold all sections */
#define GIM_FTRACE_BUF_TOTAL_SIZE	(GIM_FTRACE_FUNC_MAP_SIZE + \
					GIM_FTRACE_USR_TASK_SIZE + \
					GIM_FTRACE_INIT_TASK_SIZE + \
					GIM_FTRACE_LOG_BUF_SIZE)
#if (GIM_FTRACE_BUF_TOTAL_SIZE > GIM_FTRACE_LOG_SIZE)
#error The GIM_FTRACE_LOG_SIZE is too small to contain all blocks
#endif

typedef void *amdgv_dev_t;

struct gim_ftrace_func_map {
	uint64_t func_addr;
	char func_name[GIM_FTRACE_FUNC_NAME_LENGTH];
};

struct gim_ftrace_header_entry {
	uint32_t task_entries;
	uint32_t func_entries;
	uint32_t trace_entries;
	uint8_t reserved[20];
};

struct gim_ftrace_trace_entry {
	uint64_t time_stamp;
	uint32_t pid;
	uint32_t tgid;
	uint64_t func_addr;
	uint64_t parent_func_addr;
};

struct gim_ftrace_task_entry {
	uint32_t pid;
	uint32_t tgid;
	uint32_t gpu_bdf;
	char task_name[GIM_FTRACE_TASK_NAME_LENGTH];
};

struct gim_ftrace_log_entry {
	union{
		struct gim_ftrace_trace_entry trace_entry;
		struct gim_ftrace_task_entry task_entry;
	};
};

struct ftrace_buff {
	void *buff;
	uint32_t total_entries;
	union {
		atomic_t w_count_atomic;
		uint32_t w_count;
	};
};

struct gim_ftrace {
	struct ftrace_ops ops;
	amdgv_dev_t *adapt_list;
	uint32_t trace_suspend;
	uint32_t gen_func_map;
	void *mem_blk;

	/* Function trace information */
	struct ftrace_buff ftrace_entries;
	/* Function mapping information */
	struct ftrace_buff func_map_entries;
	/* User task information */
	struct ftrace_buff usr_task_entries;
	/* Init task information */
	struct ftrace_buff init_task_entries;
};

/*
 * gim_ftrace_init - Init ftrace. The API intializez the ftrace
 *				  context for the host driver.
 *
 * @adapt_list	List of GPUs
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int gim_ftrace_init(amdgv_dev_t *adapt_list);

/*
 * gim_ftrace_fini - De-init ftrace. The API deintializes ftrace
 *				  and free the context allocated in init.
 *
 */
void notrace gim_ftrace_fini(void);

/*
 * gim_ftrace_add_task_entry - Add task entry to list of tasks
 *
 * @task		the task the needs to be added to the list
 * @gpu_bdf		GPU bus ID
 * @usr_level_thread	flag to indicate if the task is user level
 *
 */
void gim_ftrace_add_task_entry(void *task, unsigned int gpu_bdf,
		int usr_level_thread);

/*
 * gim_ftrace_get_buffer - Get the contents of ftrace ring buffer.
 * 					The API copies the following:
 * 					1) Task list (User and Kernel)
 * 					2) Functions list that are been traced
 * 					3) Tracing list, list functions called
 *
 * @trace_buf		The buffer where the ftrace data will be copied
 * @trace_buf_len	Length of the trace_buf
 *
 * Returns:
 * size of data copied for success, 0 for failure
 *
 */
int notrace gim_ftrace_get_buffer(void *trace_buf,
		uint32_t trace_buf_len);

#endif

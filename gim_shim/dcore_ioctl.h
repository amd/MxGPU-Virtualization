/*
 * Copyright (c) 2014-2024 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef DCORE_IOCTL_H
#define DCORE_IOCTL_H

#define GIM_IOCTL_NR(n) _IOC_NR(n)
#define GIM_IOC_VOID _IOC_NONE
#define GIM_IOC_READ _IOC_READ
#define GIM_IOC_WRITE _IOC_WRITE
#define GIM_IOC_READWRITE (_IOC_READ | _IOC_WRITE)
#define GIM_IOC(dir, group, nr, size) _IOC(dir, group, nr, size)

#define GIM_IOC_BASE 'g'

#define GIM_COMMAND_BASE 0x10
#define GIM_COMMAND_END 0x50

/* command code for dcore_drv ioctl */
enum rdl_cmd_code {
	RDL_CMD_START_TRAP_GPU_HANG = 0x00000001,	  /* Used when host thread starts to wait for a reset event from kernel */
	RDL_CMD_NOTIFY_DUMP_DONE = 0x00000002,		  /* Used to notify kernel that the data dumping of host thread has been finished */
	RDL_CMD_GET_DIAG_DATA = 0x00000003,	  /* Used when host try to clean or get the existing host/fw part legacy data */
	RDL_CMD_STOP_TRAP_GPU_HANG = 0x00000004,	  /* Used when host try to stop the monitor thread */
	RDL_CMD_GET_FFBM_DATA = 0x00000005,			  /* Used when copy ffbm spa data */
	RDL_CMD_GET_MES_DBG_INFO = 0x00000006,		/* used to get mes debug ram address and size */
	RDL_CMD_CODE_MAX
};

/* args for RDL_CMD_START_TRAP_GPU_HANG */
struct dbglib_trap_gpu_info {
	struct {
		uint32_t dbsf; /* PF dbsf about which device to trap */
	} in;
	struct {
		uint32_t dbsf;		      /* which device woke up the thread, can be vf/pf's dbsf*/
		uint32_t idx_vf;		  /* vf index */
		enum dbglib_trap_event event; /* what event happened */
		uint32_t error_code;	      /* detail error code*/
		uint32_t cookie;	      /* sync up cookie */
	} out;
};

/* args for RDL_CMD_NOTIFY_DUMP_DONE */
struct dbglib_notify_done_info {
	struct {
		uint32_t dbsf; /* PF dbsf */
	} in;
};

/* args for RDL_CMD_STOP_TRAP_GPU_HANG */
struct dbglib_stop_trap_gpu_hang_info {
	struct {
		uint32_t dbsf; /* target device list */
	} in;
};

/* args for RDL_CMD_GET_DIAG_DATA */
struct dbglib_diag_data {
	struct {
		uint32_t dbsf;	      /* target device */
		uint32_t buffer_size; /* the pre-allocated buffer size */
		void *buffer;	      /* pre-allocated buffer to save data */
	} in;

	struct {
		uint32_t data_size;  /* the real data size returned */
		uint32_t error_code; /* if data_size = 0, error_code shows the details*/
		uint32_t cookie;	      /* sync up cookie */
	} out;
};

struct dbglib_ffbm_block{
	uint64_t dbsf;
	void *buffer;
};

struct dbglib_mes_dbg_info_block {
	uint32_t dbsf;
	uint32_t dbg_size;
	uint64_t dbg_addr;
};

#define GIM_IOC_START_TRAP_GPU_HANG                                                                                    \
	GIM_IOC(GIM_IOC_READWRITE, GIM_IOC_BASE, (GIM_COMMAND_BASE + RDL_CMD_START_TRAP_GPU_HANG),                     \
		sizeof(struct dbglib_trap_gpu_info))
#define GIM_IOC_NOTIFY_DUMP_DONE                                                                                       \
	GIM_IOC(GIM_IOC_READWRITE, GIM_IOC_BASE, (GIM_COMMAND_BASE + RDL_CMD_NOTIFY_DUMP_DONE),                        \
		sizeof(struct dbglib_notify_done_info))

#define GIM_IOC_GET_DIAG_DATA                                                                                  \
	GIM_IOC(GIM_IOC_READWRITE, GIM_IOC_BASE, (GIM_COMMAND_BASE + RDL_CMD_GET_DIAG_DATA),                   \
		sizeof(struct dbglib_diag_data))

#define GIM_IOC_STOP_TRAP_GPU_HANG                                                                                     \
	GIM_IOC(GIM_IOC_READWRITE, GIM_IOC_BASE, (GIM_COMMAND_BASE + RDL_CMD_STOP_TRAP_GPU_HANG),                      \
		sizeof(struct dbglib_stop_trap_gpu_hang_info))

#define GIM_IOC_GET_FFBM_DATA                                                                                     \
	GIM_IOC(GIM_IOC_READWRITE, GIM_IOC_BASE, (GIM_COMMAND_BASE + RDL_CMD_GET_FFBM_DATA), 0)

#define GIM_IOC_GET_MES_DBG_INFO                                                              				\
	GIM_IOC(GIM_IOC_READWRITE, GIM_IOC_BASE, (GIM_COMMAND_BASE + RDL_CMD_GET_MES_DBG_INFO), 				\
		sizeof(struct dbglib_mes_dbg_info_block))

#endif

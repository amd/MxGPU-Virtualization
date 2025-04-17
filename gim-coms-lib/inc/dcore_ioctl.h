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

#include "amdgv_cmd_def.h"
#include <sys/ioctl.h>

#define GIM_IOCTL_NR(n) _IOC_NR(n)
#define GIM_IOC_VOID _IOC_NONE
#define GIM_IOC_READ _IOC_READ
#define GIM_IOC_WRITE _IOC_WRITE
#define GIM_IOC_READWRITE (_IOC_READ | _IOC_WRITE)
#define GIM_IOC(dir, group, nr, size) _IOC(dir, group, nr, size)

#define GIM_IOC_BASE 'g'

#define GIM_COMMAND_BASE 0x10
#define GIM_COMMAND_END 0x50

#define DCORE_IOCTL_NUM_MASK 0x000000ff
#define DCORE_IOCTL_GET_NUM(cmd) ((*(uint32_t *)cmd) & DCORE_IOCTL_NUM_MASK)

enum gim_ioctl_flags {
	DCORE_GIM_IOCTL,
	DCORE_GIM_IOVA,
};

struct dbglib_file_operations;
struct dbglib_file {
	const struct dbglib_file_operations	*f_op;

	/*
	 * Protects f_ep, f_flags.
	 * Must not be taken from IRQ context.
	 */
	unsigned int 		f_flags;

	unsigned int			f_version;
	/* needed for tty driver, and maybe others */
	void			*private_data;
} __attribute__((aligned(4)));

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
	} in;

	struct {
		struct amdgv_cmd_shm_info shm_info;  /* pre-allocated buffer using shared fd */
		uint32_t data_size;  /* the real data size returned */
		uint32_t error_code; /* if data_size = 0, error_code shows the details*/
		uint32_t cookie;	      /* sync up cookie */
	} out;
};

/* args for RDL_CMD_GET_FFBM_DATA */
struct dbglib_ffbm_data {
	struct {
		uint32_t dbsf;	      /* target device */
	} in;

	struct {
		struct amdgv_cmd_shm_info shm_info;  /* pre-allocated buffer using shared fd */
	} out;
};

#endif //DCORE_IOCTL_H

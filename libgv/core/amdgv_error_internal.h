/*
 * Copyright (c) 2014-2021 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE.
 */

#ifndef __AMDGV_ERROR_INTERNAL_H__
#define __AMDGV_ERROR_INTERNAL_H__


/* Avoid using remainder %, set the size as 2^x */
#define AMDGV_ERROR_BUF_ENTRY_SIZE  128

/*
 * Get index of ring buffer according write_count/read_count.
 * Note: shift 7 bit for AMDGV_ERROR_BUF_SIZE is 128.
 */
#define AMDGV_ERROR_INDEX(c) ((c) & (AMDGV_ERROR_BUF_ENTRY_SIZE - 1))

/* TODO: below Macro are outdated, need to update them */
#define AMDGV_ERROR_CODE_FROM_MAILBOX(x)	((uint16_t)(((x) >> 16) & 0xFFFF))
#define AMDGV_ERROR_FLAGS_FROM_MAILBOX(x)	((uint16_t)((x)&0xFFFF))
#define AMDGV_ERROR_CODE_FLAGS_TO_MAILBOX(c, f) ((((c)&0xFFFF) << 16) | ((f)&0xFFFF))

enum ERROR_DATA_TYPE {
	ERROR_DATA_ARG_NONE = 0, // No error data
	ERROR_DATA_ARG_64,	 // 64-bit
	ERROR_DATA_ARG_32_32,	 // 32bit 32bit
	ERROR_DATA_ARG_16_16_32, // 16bit 16bit 32bit
	ERROR_DATA_ARG_16_16_16_16,
	ERROR_DATA_ARG_THREE_64_EXT,
	ERROR_DATA_ARG_FIVE_64_EXT,
};

struct error_text {
	const uint32_t code; /* full error code */
	const char	  *code_string;
	const uint8_t  arg_type;
	const uint8_t  severity;
	const char    *text;
	const uint32_t flags;
};

struct amdgv_error_info {
	const uint8_t		 category;
	const struct error_text *error_msg;
	const int		 count;
};

/**
 * Error Capture Ring Buffer for AMD SRIOV GPU Virtualization
 *
 * @ write_count: The total error entries already written in the Error Capture
 *		  Ring Buffer.
 * @ read_count: Read pointers array to save entries already read for each
 *		 outputting.
 * @ error_entry_buffer: The error entries array point.
 */
struct amdgv_error_ring_buffer {
	uint32_t write_count;
	uint32_t read_count;

	spin_lock_t error_lock;
	struct amdgv_error_entry error_entry_buffer[AMDGV_ERROR_BUF_ENTRY_SIZE];
};

struct amdgv_error_notifier {
	struct amdgv_list_head		head;
	uint64_t			event_mask;
	struct amdgv_error_ring_buffer *error_ring_buffer;
	void			       *priv;
};

int  amdgv_error_init(struct amdgv_adapter *adapt);
void amdgv_error_fini(struct amdgv_adapter *adapt);

bool amdgv_error_is_valid_vf_code(uint16_t error_code);

#endif

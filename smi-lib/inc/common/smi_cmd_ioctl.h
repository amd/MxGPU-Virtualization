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
 * THE SOFTWARE.
 */

#ifndef __SMI_CMD_IOCTL_H__
#define __SMI_CMD_IOCTL_H__

#ifndef __linux__
#ifdef _WIN64
#pragma pack(push, 8)
#else
#pragma pack(push, 1)
#endif
#endif

#define SMI_MAX_PAYLOAD		   1024 // 1024 DWORDS

#ifdef _WIN64
#define SMI_CMD_FD_CLI2SER_MASK 0
#else
/* When the in_hdr's code of smi_ioctl_cmd has MASK
 * SMI_CMD_FD_CLI2SER_MASK, then means this command
 * has a fd need to be sent with sendmsg.
 * The FD_OFFSET is the fd position in payload of smi_ioctl_cmd.
 */
#define SMI_CMD_FD_CLI2SER_MASK (1 << 20)
#define FD_OFFSET 16
#define SMI_CMD_FD_CLI2SER(cmd) \
	((GIM_IOCTL_GET_TYPE(cmd) == SMI_IOCTL) && ((*(uint32_t *)cmd) & SMI_CMD_FD_CLI2SER_MASK))
#endif

struct smi_in_hdr {
	uint32_t code;	  // Command code
	int16_t	 in_len;  // input buffer size;
	int16_t	 out_len; // output buffer size;
};

struct smi_out_hdr {
	int status; // smi response code
};

struct smi_in_command {
	struct smi_in_hdr hdr;			    // Input header
	uint32_t	  payload[SMI_MAX_PAYLOAD]; // variable size payload input
};

struct smi_out_response {
	struct smi_out_hdr hdr;			     // Output header
	uint32_t	   payload[SMI_MAX_PAYLOAD]; // variable size payload output
};

typedef struct smi_ioctl_cmd {
	struct smi_in_hdr  in_hdr;
	struct smi_out_hdr out_hdr;
	uint32_t	   payload[SMI_MAX_PAYLOAD];
} smi_ioctl_cmd;

#ifndef __linux__
#pragma pack(pop)
#endif

#endif // __SMI_CMD_IOCTL_H__

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

#ifndef _SMI_CMD_H_
#define _SMI_CMD_H_

/* NOTE: structures defined here should be up to date
 * with SMI IOCTL implementation and first entry
 * should be always a 32-bit command code.
 */

#define SMI_MAX_PAYLOAD 1024

/* When the in_hdr's code of smi_ioctl_cmd has MASK
 * SMI_CMD_FD_CLI2SER_MASK, then means this command
 * has a fd need to be sent with sendmsg.
 * The FD_OFFSET is the fd position in payload of smi_ioctl_cmd.
 */
#define SMI_CMD_FD_CLI2SER_MASK (1 << 20)
#define FD_OFFSET 16
#define SMI_CMD_FD_CLI2SER(cmd) \
	((GIM_IOCTL_GET_TYPE(cmd) == SMI_IOCTL) && ((*(uint32_t *)cmd) & SMI_CMD_FD_CLI2SER_MASK))

struct smi_in_hdr {
	uint32_t code;
	int16_t in_len;
	int16_t out_len;
};

struct smi_out_hdr {
	int status;
};

struct smi_ioctl_cmd {
	struct smi_in_hdr  in_hdr;
	struct smi_out_hdr out_hdr;
	uint32_t payload[SMI_MAX_PAYLOAD];
};

#endif

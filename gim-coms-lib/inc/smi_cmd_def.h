/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

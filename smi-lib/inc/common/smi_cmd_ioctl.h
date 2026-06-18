/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <smi_vcs.h>

#define SMI_PRIVATE_COMMAND(code) (code & (1U << 31))

static void smi_get_max_supported_cmds(int api_version, unsigned *max_public)
{
	switch (api_version) {
	case SMI_UNKNOWN_VERSION:
		*max_public = SMI_CMD_CODE_HANDSHAKE;
		break;
	case SMI_VERSION_ALPHA_0:
		*max_public = 0x00000013;
		break;
	case SMI_VERSION_BETA_0:
	case SMI_VERSION_BETA_1:
		*max_public = 0x0000001A;
		break;
	case SMI_VERSION_BETA_2:
		*max_public = 0x00000020;
		break;
	case SMI_VERSION_BETA_3:
	case SMI_VERSION_BETA_4:
		*max_public = SMI_CMD_CODE__MAX - 1;
		break;
	default:
		*max_public = SMI_CMD_CODE_INVALID;
		break;
	}
}

bool smi_is_supported(enum smi_cmd_code command_code, int api_version)
{
	unsigned max_public = SMI_CMD_CODE_INVALID;

	smi_get_max_supported_cmds(api_version, &max_public);

	return (unsigned)command_code <= max_public;
}

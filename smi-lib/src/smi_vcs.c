/*
 * Copyright (c) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

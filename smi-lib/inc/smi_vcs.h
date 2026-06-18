/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_VCS_H__
#define __SMI_VCS_H__

#include <stdbool.h>
#include "common/smi_cmd.h"

#define SMI_UNKNOWN_VERSION 0
#define SMI_CMD_CODE_INVALID    0

/**
 *  \brief  Returns whether the specified command is compatible
 *          with the negotiated api version
 *
 *  \param [in]  command_code  Command code to test
 *
 *  \param [in]  api_version   Negotiated api version
 *
 *  \return SMI_RET_CODE indicating result.
 */
bool smi_is_supported(enum smi_cmd_code command_code, int api_version);

#endif // __SMI_VCS_H__

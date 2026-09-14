/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_UCODE_MI350P_H
#define AMDGV_UCODE_MI350P_H

#include "ucode/mi350p/psp_drv_had_bin_mi350.h"
#include "ucode/mi350p/psp_drv_intf_bin_mi350.h"
#include "ucode/mi350p/psp_drv_ras_bin_mi350.h"
#include "ucode/mi350p/psp_drv_soc_bin_mi350.h"
#include "ucode/mi350p/psp_drv_sys_bin_mi350.h"
#include "ucode/mi350p/psp_keydb_mi350.h"
#include "ucode/mi350p/psp_sos_bin_mi350.h"
#include "ucode/mi350p/psp_tos_wl_bin_mi350.h"

int mi350p_ucode_load(struct amdgv_adapter *adapt,
		      enum amdgv_firmware_id *ucode_id_list,
		      uint32_t ucode_id_count);

#endif

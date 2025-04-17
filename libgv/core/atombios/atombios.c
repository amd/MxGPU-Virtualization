/*
 * Copyright (c) 2007-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#include "atom.h"
#include "atom-bits.h"
#include "atombios.h"
#include "amdgv_device.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

int amdgv_atombios_get_fw_usage_fb(struct amdgv_adapter *adapt)
{
	struct atom_context *ctx = adapt->vbios.atom_context;
	int index = GetIndexIntoMasterTable(DATA, VRAM_UsageByFirmware);
	uint16_t data_offset;
	struct _ATOM_VRAM_USAGE_BY_FIRMWARE *fw_usage;
	struct _ATOM_FIRMWARE_VRAM_RESERVE_INFO *resv_info;

	if (amdgv_atom_parse_data_header(ctx, index, NULL, NULL, NULL, &data_offset)) {
		fw_usage = (struct _ATOM_VRAM_USAGE_BY_FIRMWARE *)((uint8_t *)ctx->bios +
								   data_offset);

		resv_info = &fw_usage->asFirmwareVramReserveInfo[0];

		AMDGV_INFO("atom firmware requested %08x %dkb\n",
			   resv_info->ulStartAddrUsedByFirmware, resv_info->usFirmwareUseInKb);

		adapt->vbios.vram_usage_start_addr =
			(uint64_t)resv_info->ulStartAddrUsedByFirmware;

		AMDGV_INFO("vram used by fw start addr=0x%llx\n",
			   adapt->vbios.vram_usage_start_addr);
	}

	return 0;
}

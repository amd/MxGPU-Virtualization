/*
 * Copyright (c) 2017-2020 Advanced Micro Devices, Inc. All rights reserved.
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
#include "atombios/atom.h"
#include "atombios/atomfirmware.h"
#include "amdgv_powerplay_swsmu.h"
#include "amdgv.h"

#define BITS_64 64

int smu_get_atom_data_table(struct amdgv_adapter *adapt, uint32_t table, uint16_t *size,
			    uint8_t *frev, uint8_t *crev, uint8_t **addr)
{
	uint16_t data_start;
	struct atom_context *ctxt = adapt->vbios.atom_context;

	if (!amdgv_atom_parse_data_header(ctxt, table, size, frev, crev, &data_start))
		return AMDGV_FAILURE;

	*addr = (uint8_t *)ctxt->bios + data_start;

	return 0;
}

uint64_t smu_pp_throttler_event_convert(struct amdgv_adapter *adapt,
		const uint8_t *event_map, int count, uint64_t hw_status)
{
	uint64_t throttler_event;
	int i;

	if (!event_map || !count || !hw_status)
		return 0ULL;

	throttler_event = 0ULL;
	for (i = 0; i < BITS_64; i++) {
		if (hw_status & (1ULL << i)) {
			if (i < count)
				throttler_event |= 1ULL << event_map[i];
		}
	}

	return throttler_event;
}

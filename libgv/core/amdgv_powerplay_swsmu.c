/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

int amdgv_smu_send_ras_msg(struct amdgv_adapter *adapt, enum pp_smu_ras_msg msg,
	uint32_t *params, uint32_t num_params, uint32_t *read_args, uint32_t num_read_args)
{
if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->smu_send_ras_msg)
	return adapt->pp.pp_funcs->smu_send_ras_msg(adapt, msg,
				params, num_params, read_args, num_read_args);

return AMDGV_FAILURE;
}
/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "smuio_v13_0_3.h"
#include "mi300/SMUIO/smuio_13_0_3_offset.h"
#include "mi300/SMUIO/smuio_13_0_3_sh_mask.h"

 /**
  * smuio_v13_0_3_get_die_id - query die id from FCH.
  *
  * @adapt: amdgv device pointer
  *
  * Returns die id
  */
static uint32_t smuio_v13_0_3_get_die_id(struct amdgv_adapter *adapt)
{
	uint32_t data, die_id;

	data = RREG32_SOC15(SMUIO, 0, regSMUIO_MCM_CONFIG);
	die_id = REG_GET_FIELD(data, SMUIO_MCM_CONFIG, DIE_ID);

	return die_id;
}

/**
 * smuio_v13_0_3_get_socket_id - query socket id from FCH
 *
 * @adapt: amdgv device pointer
 *
 * Returns socket id
 */
static uint32_t smuio_v13_0_3_get_socket_id(struct amdgv_adapter *adapt)
{
	uint32_t data, socket_id;

	data = RREG32_SOC15(SMUIO, 0, regSMUIO_MCM_CONFIG);
	socket_id = REG_GET_FIELD(data, SMUIO_MCM_CONFIG, SOCKET_ID);

	return socket_id;
}

const struct amdgv_smuio_funcs smuio_v13_0_3_funcs = {
	.get_die_id = smuio_v13_0_3_get_die_id,
	.get_socket_id = smuio_v13_0_3_get_socket_id,
};

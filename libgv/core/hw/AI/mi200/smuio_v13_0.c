/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "smuio_v13_0.h"
#include "mi200/SMUIO/smuio_13_0_2_offset.h"
#include "mi200/SMUIO/smuio_13_0_2_sh_mask.h"

#define SMUIO_MCM_CONFIG__HOST_GPU_XGMI_MASK	0x00000001L

/**
 * smuio_v13_0_get_die_id - query die id from FCH.
 *
 * @adapt: amdgv device pointer
 *
 * Returns die id
 */
static uint32_t smuio_v13_0_get_die_id(struct amdgv_adapter *adapt)
{
	uint32_t data, die_id;

	data = RREG32_SOC15(SMUIO, 0, regSMUIO_MCM_CONFIG);
	die_id = REG_GET_FIELD(data, SMUIO_MCM_CONFIG, DIE_ID);

	return die_id;
}

/**
 * smuio_v13_0_get_socket_id - query socket id from FCH
 *
 * @adapt: amdgv device pointer
 *
 * Returns socket id
 */
static uint32_t smuio_v13_0_get_socket_id(struct amdgv_adapter *adapt)
{
	uint32_t data, socket_id;

	data = RREG32_SOC15(SMUIO, 0, regSMUIO_MCM_CONFIG);
	socket_id = REG_GET_FIELD(data, SMUIO_MCM_CONFIG, SOCKET_ID);

	return socket_id;
}

/**
 * smuio_v13_0_is_host_gpu_xgmi_supported - detect xgmi interface between cpu and gpu/s.
 *
 * @adapt: amdgv device pointer
 *
 * Returns true on success or false otherwise.
 */
static bool smuio_v13_0_is_host_gpu_xgmi_supported(struct amdgv_adapter *adapt)
{
	uint32_t data;

	data = RREG32_SOC15(SMUIO, 0, regSMUIO_MCM_CONFIG);
	data = REG_GET_FIELD(data, SMUIO_MCM_CONFIG, TOPOLOGY_ID);
	/* data[4:0]
	 * bit 0 == 0 host-gpu interface is PCIE
	 * bit 0 == 1 host-gpu interface is Alternate Protocal
	 * for AMD, this is XGMI
	 */
	data &= SMUIO_MCM_CONFIG__HOST_GPU_XGMI_MASK;

	return data ? true : false;
}

const struct amdgv_smuio_funcs smuio_v13_0_funcs = {
	.get_die_id = smuio_v13_0_get_die_id,
	.get_socket_id = smuio_v13_0_get_socket_id,
	.is_host_gpu_xgmi_supported = smuio_v13_0_is_host_gpu_xgmi_supported,
};

/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "smuio_v15_0_8.h"
#include "asic_reg/SMUIO/smuio_15_0_8_offset.h"
#include "asic_reg/SMUIO/smuio_15_0_8_sh_mask.h"

static const uint32_t this_block = AMDGV_MANAGEMENT_BLOCK;

#define SMUIO_MCM_CONFIG__HOST_GPU_XGMI_MASK		0x00000001L
#define SMUIO_MCM_CONFIG__LINK_TYPE_MASK		0x00000018L

/**
 * smuio_v15_0_8_get_die_id - query die id from FCH.
 *
 * @adapt: amdgv device pointer
 *
 * Returns die id
 */
static uint32_t smuio_v15_0_8_get_die_id(struct amdgv_adapter *adapt)
{
	uint32_t data, die_id;

	data = RREG32_SOC15(SMUIO, 0, regSMUIO_MCM_CONFIG);
	die_id = REG_GET_FIELD(data, SMUIO_MCM_CONFIG, DIE_ID);

	return die_id;
}

/**
 * smuio_v15_0_8_get_socket_id - query socket id from FCH
 *
 * @adapt: amdgv device pointer
 *
 * Returns socket id
 */
static uint32_t smuio_v15_0_8_get_socket_id(struct amdgv_adapter *adapt)
{
	uint32_t data, socket_id;

	data = RREG32_SOC15(SMUIO, 0, regSMUIO_MCM_CONFIG);
	socket_id = REG_GET_FIELD(data, SMUIO_MCM_CONFIG, SOCKET_ID);

	return socket_id;
}

/**
 * smuio_v15_0_8_is_host_gpu_xgmi_supported - detect xgmi interface between cpu and gpu/s.
 *
 * @adapt: amdgv device pointer
 *
 * Returns true on success or false otherwise.
 */
static bool smuio_v15_0_8_is_host_gpu_xgmi_supported(struct amdgv_adapter *adapt)
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

/**
 * smuio_v15_0_8_get_link_type - gets link type
 *
 * @adapt: amdgv device pointer
 *
 * Returns link type
 */
static enum amdgv_ual_link_type smuio_v15_0_8_get_link_type(struct amdgv_adapter *adapt)
{
	uint32_t data, link_type;
	data = RREG32_SOC15(SMUIO, 0, regSMUIO_MCM_CONFIG);
	data = REG_GET_FIELD(data, SMUIO_MCM_CONFIG, TOPOLOGY_ID);

	/* data[4:3]
	 * 0 - UALOE connected A+A system
	 * 1 - UALINK connected A+A system
	 * etc.
	 */
	link_type = ((data & SMUIO_MCM_CONFIG__LINK_TYPE_MASK) >> 3);

	switch (link_type) {
	case 0:
		return AMDGV_UALOE;
	case 1:
		return AMDGV_UALINK;
	default:
		AMDGV_ERROR("Unknown Link type: %d\n", link_type);
		return AMDGV_UAL_NONE;
	}
}

const struct amdgv_smuio_funcs smuio_v15_0_8_funcs = {
	.get_die_id = smuio_v15_0_8_get_die_id,
	.get_socket_id = smuio_v15_0_8_get_socket_id,
	.is_host_gpu_xgmi_supported = smuio_v15_0_8_is_host_gpu_xgmi_supported,
	.get_link_type = smuio_v15_0_8_get_link_type,
};

/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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
#include "amdgv.h"
#include "amdgv_device.h"
#include "smuio_v13_0_3.h"
#include "mi300/SMUIO/smuio_13_0_3_offset.h"
#include "mi300/SMUIO/smuio_13_0_3_sh_mask.h"

 /**
  * smuio_v13_0_3_get_die_id - query die id from FCH.
  *
  * @adev: amdgpu device pointer
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
 * @adev: amdgpu device pointer
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

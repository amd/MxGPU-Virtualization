/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AMDGV_SMUIO_H__
#define __AMDGV_SMUIO_H__

#include "amdgv_ual.h"

struct amdgv_smuio_funcs {
	uint32_t (*get_die_id)(struct amdgv_adapter *adapt);
	uint32_t (*get_socket_id)(struct amdgv_adapter *adapt);
	bool (*is_host_gpu_xgmi_supported)(struct amdgv_adapter *adapt);
	enum amdgv_ual_link_type (*get_link_type)(struct amdgv_adapter *adapt);
};

struct amdgv_smuio {
	const struct amdgv_smuio_funcs		*funcs;
};

#endif /* __AMDGV_SMUIO_H__ */

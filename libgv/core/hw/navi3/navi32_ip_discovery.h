/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAV2_IP_DISCOVERY_H
#define NAV2_IP_DISCOVERY_H

int navi32_ip_discovery_init(struct amdgv_adapter *adapt);
void navi32_ip_discovery_fini(struct amdgv_adapter *adapt);
int navi32_discover_ip(struct amdgv_adapter *adapt);
int navi32_copy_ip_data_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);

#endif

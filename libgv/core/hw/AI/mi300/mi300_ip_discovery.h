/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI300_IP_DISCOVERY_H
#define MI300_IP_DISCOVERY_H

int mi300_discover_ip(struct amdgv_adapter *adapt);
int mi300_discover_ip_nps_table(struct amdgv_adapter *adapt);

int mi300_copy_ip_data_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);

#endif

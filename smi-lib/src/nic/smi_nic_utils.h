/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_NIC_UTILS_H__
#define __SMI_NIC_UTILS_H__

#include "amdsmi.h"
#include "smi_nic_interface.h"

/**
 * @brief Maps an enum representing a NIC library status to an enum representing AMDSMI library status.
 *
 * This function takes an enum value representing a NIC library status and maps it to an
 * equivalent enum value representing an AMDSMI library status. The mapping is performed based on
 * predefined set of rules. If the provided NIC library status does not match any of the
 * predefined values, it is mapped to AMDSMI_STATUS_MAP_ERROR.
 *
 * @param[in] unit The enum value representing NIC library status.
 * @return The enum value representing the corresponding mapped AMDSMI library status.
 *
 * @note This function assumes that the enum values for both enums (amdsmi_status_t and smi_nic_status_t)
 * are compatible and represent similar concepts.
*/
amdsmi_status_t smi_map_nic_status(smi_nic_status_t status);

/**
 * @brief Maps a NIC link type enum value to the unified AMDSMI link type enum.
 *
 * Translates an ::smi_nic_link_type_t value coming from the NIC subsystem into
 * the corresponding ::amdsmi_link_type_t value exposed by the public API.
 * Unknown or unmapped values are translated to ::AMDSMI_LINK_TYPE_UNKNOWN.
 *
 * @param[in] link_type NIC subsystem link type.
 * @return Corresponding ::amdsmi_link_type_t value.
 */
amdsmi_link_type_t smi_map_nic_link_type(smi_nic_link_type_t link_type);

#endif // __SMI_NIC_UTILS_H__

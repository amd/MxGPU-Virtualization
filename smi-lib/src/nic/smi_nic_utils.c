/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_nic_utils.h"


amdsmi_status_t smi_map_nic_status(smi_nic_status_t status)
{
	int status_code = AMDSMI_STATUS_MAP_ERROR;

	switch(status) {
	case SMI_NIC_STATUS_SUCCESS:
		status_code = AMDSMI_STATUS_SUCCESS;
		break;
	case SMI_NIC_STATUS_ERROR:
		status_code = AMDSMI_STATUS_API_FAILED;
		break;
	case SMI_NIC_STATUS_WRONG_PARAM:
		status_code = AMDSMI_STATUS_INVAL;
		break;
	case SMI_NIC_STATUS_NOT_FOUND:
		status_code = AMDSMI_STATUS_NOT_FOUND;
		break;
	case SMI_NIC_STATUS_NO_RESOURCE:
		status_code = AMDSMI_STATUS_OUT_OF_RESOURCES;
		break;
	case SMI_NIC_STATUS_NOT_SUPPORTED:
		status_code = AMDSMI_STATUS_NOT_SUPPORTED;
		break;
	case SMI_NIC_STATUS_NOT_INIT:
		status_code = AMDSMI_STATUS_NOT_INIT;
		break;
	case SMI_NIC_STATUS_NO_DATA:
		status_code = AMDSMI_STATUS_NO_DATA;
		break;
	case SMI_NIC_STATUS_DRIVER_NOT_LOADED:
		status_code = AMDSMI_STATUS_DRIVER_NOT_LOADED;
		break;
	}

	return status_code;
}

amdsmi_link_type_t smi_map_nic_link_type(smi_nic_link_type_t link_type)
{
	switch (link_type) {
	case SMI_NIC_LINK_TYPE_PCIE:
		return AMDSMI_LINK_TYPE_PCIE;
	case SMI_NIC_LINK_TYPE_NUMA:
		return AMDSMI_LINK_TYPE_NUMA;
	case SMI_NIC_LINK_TYPE_XNUMA:
		return AMDSMI_LINK_TYPE_XNUMA;
	case SMI_NIC_LINK_TYPE_UNKNOWN:
	default:
		return AMDSMI_LINK_TYPE_UNKNOWN;
	}
}

/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @file amdsmi_nic_stub.c
 *
 * AMD SMI NIC Stub Implementation
 *
 * This file contains stub implementations for all NIC functions when
 * NIC is not supported in the library. All functions return
 * AMDSMI_STATUS_NOT_SUPPORTED to indicate that NIC functionality is
 * not available.
 */

#include "amdsmi.h"
#include "smi_os_defines.h"

#ifdef __linux__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

/**
 * @brief Stub implementation for amdsmi_get_nic_driver_info
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_nic_driver_info(amdsmi_processor_handle processor_handle, amdsmi_nic_driver_info_t *info)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)info;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_get_nic_fw_info
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_nic_fw_info(amdsmi_processor_handle processor_handle, amdsmi_nic_fw_info_t *info)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)info;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_get_nic_asic_info
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_nic_asic_info(amdsmi_processor_handle processor_handle, amdsmi_nic_asic_info_t *info)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)info;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_get_nic_bus_info
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_nic_bus_info(amdsmi_processor_handle processor_handle, amdsmi_nic_bus_info_t *info)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)info;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_get_nic_numa_info
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_nic_numa_info(amdsmi_processor_handle processor_handle, amdsmi_nic_numa_info_t *info)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)info;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_get_nic_port_info
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_nic_port_info(amdsmi_processor_handle processor_handle, amdsmi_nic_port_info_t *info)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)info;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_get_nic_rdma_dev_info
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_nic_rdma_dev_info(amdsmi_processor_handle processor_handle, amdsmi_nic_rdma_devices_info_t *info)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)info;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_get_nic_port_statistics
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_nic_port_statistics(amdsmi_processor_handle processor_handle, uint32_t port_index,
                                               uint32_t *num_stats, amdsmi_nic_stat_t *stats)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)port_index;
	(void)num_stats;
	(void)stats;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_get_nic_vendor_statistics
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_nic_vendor_statistics(amdsmi_processor_handle processor_handle, uint32_t port_index,
                                                 uint32_t *num_stats, amdsmi_nic_stat_t *stats)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)port_index;
	(void)num_stats;
	(void)stats;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Stub implementation for amdsmi_get_nic_rdma_port_statistics
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_get_nic_rdma_port_statistics(amdsmi_processor_handle processor_handle, uint32_t rdma_port_index,
                                                    uint32_t *num_stats, amdsmi_nic_stat_t *stats)
{
	#pragma SMI_EXPORT
	(void)processor_handle;
	(void)rdma_port_index;
	(void)num_stats;
	(void)stats;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

#ifdef __linux__
#pragma GCC diagnostic pop
#endif

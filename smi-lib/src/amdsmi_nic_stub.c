/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

/**
 * @brief Stub implementation for amdsmi_topo_get_nic_link_type
 * @return AMDSMI_STATUS_NOT_SUPPORTED
 */
amdsmi_status_t amdsmi_topo_get_nic_link_type(amdsmi_processor_handle nic_handle,
                                              amdsmi_processor_handle processor_handle,
                                              amdsmi_nic_link_type_t *type)
{
	#pragma SMI_EXPORT
	(void)nic_handle;
	(void)processor_handle;
	(void)type;
	return AMDSMI_STATUS_NOT_SUPPORTED;
}

#ifdef __linux__
#pragma GCC diagnostic pop
#endif

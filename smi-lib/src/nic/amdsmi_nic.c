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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "amdsmi.h"
#include "smi_nic_interface.h"
#include "smi_nic_utils.h"
#include "smi_utils.h"
#include "smi_defines.h"
#include "common/smi_device_handle.h"
#include "smi_debug.h"
#include "smi_os_defines.h"
#include "smi_processor_handle.h"
#include "smi_sys_wrapper.h"

#ifdef __linux__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

amdsmi_status_t amdsmi_get_nic_driver_info(amdsmi_processor_handle processor_handle, amdsmi_nic_driver_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_handle_type type;
	smi_nic_driver_info_t driver_info;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	memset(info, 0, sizeof(amdsmi_nic_driver_info_t));
	nic = ((struct smi_nic_handle *)processor_handle);

	ret = smi_get_nic_driver_info(smi_req.thread->nic_ctx, nic->bdf.as_uint, &driver_info);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get NIC driver info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	sys_wrapper->smi_strncpy(info->name, sizeof(info->name), driver_info.name, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->version, sizeof(info->version), driver_info.version, AMDSMI_MAX_STRING_LENGTH);

	return AMDSMI_STATUS_SUCCESS;
}


amdsmi_status_t amdsmi_get_nic_asic_info(amdsmi_processor_handle processor_handle, amdsmi_nic_asic_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_handle_type type;
	smi_nic_asic_info_t asic_info;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	memset(info, 0, sizeof(amdsmi_nic_asic_info_t));

	nic = ((struct smi_nic_handle *)processor_handle);

	ret = smi_get_nic_asic_info(smi_req.thread->nic_ctx, nic->bdf.as_uint, &asic_info);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get NIC ASIC info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	info->vendor_id = asic_info.vendor_id;
	info->subvendor_id = asic_info.subvendor_id;
	info->device_id = asic_info.device_id;
	info->subsystem_id = asic_info.subsystem_id;
	info->revision = asic_info.revision;
	sys_wrapper->smi_strncpy(info->permanent_address, sizeof(info->permanent_address), asic_info.permanent_address, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->product_name, sizeof(info->product_name), asic_info.product_name, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->vendor_name, sizeof(info->vendor_name), asic_info.vendor_name, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->part_number, sizeof(info->part_number), asic_info.part_number, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->serial_number, sizeof(info->serial_number), asic_info.serial_number, AMDSMI_MAX_STRING_LENGTH);

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_bus_info(amdsmi_processor_handle processor_handle, amdsmi_nic_bus_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_handle_type type;
	smi_nic_bus_info_t bus_info;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	nic = ((struct smi_nic_handle *)processor_handle);
	memset(info, 0, sizeof(amdsmi_nic_bus_info_t));

	ret = smi_get_nic_bus_info(smi_req.thread->nic_ctx, nic->bdf.as_uint, &bus_info);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get NIC bus info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	info->bdf.as_uint = bus_info.bdf;
	info->max_pcie_speed = bus_info.max_pcie_speed;
	info->max_pcie_width = bus_info.max_pcie_width;
	sys_wrapper->smi_strncpy(info->pcie_interface_version, sizeof(info->pcie_interface_version), bus_info.pcie_interface_version, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->slot_type, sizeof(info->slot_type), bus_info.slot_type, AMDSMI_MAX_STRING_LENGTH);

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_numa_info(amdsmi_processor_handle processor_handle, amdsmi_nic_numa_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_handle_type type;
	smi_nic_numa_info_t numa_info;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	nic = ((struct smi_nic_handle *)processor_handle);
	memset(info, 0, sizeof(amdsmi_nic_numa_info_t));

	ret = smi_get_nic_numa_info(smi_req.thread->nic_ctx, nic->bdf.as_uint, &numa_info);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get NIC NUMA info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	info->node = numa_info.node;
	sys_wrapper->smi_strncpy(info->affinity, sizeof(info->affinity), numa_info.affinity, AMDSMI_MAX_STRING_LENGTH);

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_port_info(amdsmi_processor_handle processor_handle, amdsmi_nic_port_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_handle_type type;
	smi_nic_port_info_t port_info;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	nic = ((struct smi_nic_handle *)processor_handle);
	memset(info, 0, sizeof(amdsmi_nic_port_info_t));

	ret = smi_get_nic_port_info(smi_req.thread->nic_ctx, nic->bdf.as_uint, &port_info);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get NIC port info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	info->num_ports = port_info.num_ports;
	for (uint32_t i = 0; i < info->num_ports; i++) {
		const smi_nic_port_t *port = &port_info.ports[i];

		info->ports[i].bdf.as_uint = port->bdf;
		info->ports[i].port_num = port->port_num;
		sys_wrapper->smi_strncpy(info->ports[i].type, sizeof(info->ports[i].type), port->type, AMDSMI_MAX_STRING_LENGTH);
		sys_wrapper->smi_strncpy(info->ports[i].flavour, sizeof(info->ports[i].flavour), port->flavour, AMDSMI_MAX_STRING_LENGTH);
		sys_wrapper->smi_strncpy(info->ports[i].netdev, sizeof(info->ports[i].netdev), port->netdev, AMDSMI_MAX_STRING_LENGTH);
		info->ports[i].ifindex = port->ifindex;
		sys_wrapper->smi_strncpy(info->ports[i].mac_address, sizeof(info->ports[i].mac_address), port->mac_address, AMDSMI_MAX_STRING_LENGTH);
		info->ports[i].carrier = port->carrier;
		info->ports[i].mtu = port->mtu;

		sys_wrapper->smi_strncpy(info->ports[i].link_state, sizeof(info->ports[i].link_state),
			((strcmp(port->link_state, "yes") == 0) ? "UP" : "DOWN"), AMDSMI_MAX_STRING_LENGTH);

		info->ports[i].link_speed = port->link_speed;
		info->ports[i].active_fec = port->active_fec;

		sys_wrapper->smi_strncpy(info->ports[i].autoneg, sizeof(info->ports[i].autoneg),
			port->autoneg, AMDSMI_MAX_STRING_LENGTH);
		sys_wrapper->smi_strncpy(info->ports[i].pause_autoneg, sizeof(info->ports[i].pause_autoneg),
			port->pause_autoneg, AMDSMI_MAX_STRING_LENGTH);
		sys_wrapper->smi_strncpy(info->ports[i].pause_rx, sizeof(info->ports[i].pause_rx),
			port->pause_rx, AMDSMI_MAX_STRING_LENGTH);
		sys_wrapper->smi_strncpy(info->ports[i].pause_tx, sizeof(info->ports[i].pause_tx),
			port->pause_tx, AMDSMI_MAX_STRING_LENGTH);
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_vendor_statistics(amdsmi_processor_handle processor_handle, uint32_t port_index,
						uint32_t *num_stats, amdsmi_nic_stat_t *stats)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_handle_type type;
	smi_nic_stat_info_t vendor_stats;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || num_stats == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	nic = ((struct smi_nic_handle *)processor_handle);
	if (stats == NULL) {
		ret = smi_get_nic_vendor_statistics_count(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_index, num_stats);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			SMI_ERROR("Failed to get nic vendor statistics count. Return code: %d", ret);
			return smi_map_nic_status(ret);
		}
	} else {
		ret = smi_get_nic_vendor_statistics_list(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_index, &vendor_stats);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			SMI_ERROR("Failed to get nic vendor statistics list. Return code: %d", ret);
			return smi_map_nic_status(ret);
		}

		if (*num_stats >= vendor_stats.count) {
			*num_stats = vendor_stats.count;
		}

		for (uint32_t i = 0; i < *num_stats; i++) {
			sys_wrapper->smi_strncpy(stats[i].name, sizeof(stats[i].name), vendor_stats.stats[i].name, AMDSMI_MAX_STRING_LENGTH);
			stats[i].value = vendor_stats.stats[i].value;
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_port_statistics(amdsmi_processor_handle processor_handle, uint32_t port_index,
					uint32_t *num_stats, amdsmi_nic_stat_t *stats)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_handle_type type;
	smi_nic_stat_info_t port_stats;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || num_stats == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	nic = ((struct smi_nic_handle *)processor_handle);
	if (stats == NULL) {
		ret = smi_get_nic_port_statistics_count(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_index, num_stats);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			SMI_ERROR("Failed to get nic port statistics count. Return code: %d", ret);
			return smi_map_nic_status(ret);
		}
	} else {
		ret = smi_get_nic_port_statistics_list(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_index, &port_stats);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			SMI_ERROR("Failed to get nic port statistics list. Return code: %d", ret);
			return smi_map_nic_status(ret);
		}

		if (*num_stats >= port_stats.count) {
			*num_stats = port_stats.count;
		}

		for (uint32_t i = 0; i < *num_stats; i++) {
			sys_wrapper->smi_strncpy(stats[i].name, sizeof(stats[i].name), port_stats.stats[i].name, AMDSMI_MAX_STRING_LENGTH);
			stats[i].value = port_stats.stats[i].value;
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_rdma_dev_info(amdsmi_processor_handle processor_handle, amdsmi_nic_rdma_devices_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_handle_type type;
	smi_nic_rdma_devices_info_t rdma_info;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	nic = ((struct smi_nic_handle *)processor_handle);
	memset(info, 0, sizeof(amdsmi_nic_rdma_devices_info_t));

	ret = smi_get_nic_rdma_dev_info(smi_req.thread->nic_ctx, nic->bdf.as_uint, &rdma_info);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get NIC RDMA device info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	info->num_rdma_dev = rdma_info.num_rdma_dev;
	for (uint8_t i = 0; i < info->num_rdma_dev; i++) {
		const smi_nic_rdma_dev_info_t *rdma = &rdma_info.rdma_dev_info[i];
		amdsmi_nic_rdma_dev_info_t *out = &info->rdma_dev_info[i];

		sys_wrapper->smi_strncpy(out->rdma_dev, sizeof(out->rdma_dev),
			rdma->rdma_dev, AMDSMI_MAX_STRING_LENGTH);
		sys_wrapper->smi_strncpy(out->node_guid, sizeof(out->node_guid),
			rdma->node_guid, AMDSMI_MAX_STRING_LENGTH);
		sys_wrapper->smi_strncpy(out->node_type, sizeof(out->node_type),
			rdma->node_type, AMDSMI_MAX_STRING_LENGTH);
		sys_wrapper->smi_strncpy(out->sys_image_guid, sizeof(out->sys_image_guid),
			rdma->sys_image_guid, AMDSMI_MAX_STRING_LENGTH);
		sys_wrapper->smi_strncpy(out->fw_ver, sizeof(out->fw_ver),
			rdma->fw_ver, AMDSMI_MAX_STRING_LENGTH);
		out->num_rdma_ports = rdma->num_rdma_ports;

		for (uint8_t j = 0; j < rdma->num_rdma_ports; j++) {
			const smi_nic_rdma_port_info_t *rdma_port = &rdma->rdma_port_info[j];

			sys_wrapper->smi_strncpy(out->rdma_port_info[j].netdev, sizeof(out->rdma_port_info[j].netdev),
				rdma_port->netdev, AMDSMI_MAX_STRING_LENGTH);
			sys_wrapper->smi_strncpy(out->rdma_port_info[j].state, sizeof(out->rdma_port_info[j].state),
				rdma_port->state, AMDSMI_MAX_STRING_LENGTH);
			out->rdma_port_info[j].rdma_port = rdma_port->rdma_port;
			out->rdma_port_info[j].max_mtu = rdma_port->max_mtu;
			out->rdma_port_info[j].active_mtu = rdma_port->active_mtu;
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_rdma_port_statistics(amdsmi_processor_handle processor_handle, uint32_t rdma_port_index,
						uint32_t *num_stats, amdsmi_nic_stat_t *stats)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_handle_type type;
	smi_nic_stat_info_t rdma_stats;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || num_stats == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_handle_type *)processor_handle);
	if (type != SMI_HANDLE_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	nic = ((struct smi_nic_handle *)processor_handle);
	if (stats == NULL) {
		ret = smi_get_nic_rdma_port_statistics_count(smi_req.thread->nic_ctx, nic->bdf.as_uint, 0, 0, rdma_port_index, num_stats);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			SMI_ERROR("Failed to get nic rdma port statistics count. Return code: %d", ret);
			return smi_map_nic_status(ret);
		}
	} else {
		ret = smi_get_nic_rdma_port_statistics_list(smi_req.thread->nic_ctx, nic->bdf.as_uint, 0, 0, rdma_port_index, &rdma_stats);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			SMI_ERROR("Failed to get nic rdma port statistics list. Return code: %d", ret);
			return smi_map_nic_status(ret);
		}

		if (*num_stats >= rdma_stats.count) {
			*num_stats = rdma_stats.count;
		}

		for (uint32_t i = 0; i < *num_stats; i++) {
			sys_wrapper->smi_strncpy(stats[i].name, sizeof(stats[i].name), rdma_stats.stats[i].name, AMDSMI_MAX_STRING_LENGTH);
			stats[i].value = rdma_stats.stats[i].value;
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

#ifdef __linux__
#pragma GCC diagnostic pop
#endif

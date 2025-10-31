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
	enum smi_processor_type type;
	struct ethtool_drvinfo drvinfo;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_processor_type *)processor_handle);
	if (type != SMI_PROCESSOR_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	memset(info, 0, sizeof(amdsmi_nic_driver_info_t));
	nic = ((struct smi_nic_handle *)processor_handle);

	ret = smi_get_nic_driver_info(smi_req.thread->nic_ctx, nic->bdf.as_uint, &drvinfo);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get NIC driver info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	sys_wrapper->smi_strncpy(info->name, sizeof(info->name), drvinfo.driver, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->version, sizeof(info->version), drvinfo.version, AMDSMI_MAX_STRING_LENGTH);

	return smi_map_nic_status(ret);
}


amdsmi_status_t amdsmi_get_nic_asic_info(amdsmi_processor_handle processor_handle, amdsmi_nic_asic_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_processor_type type;
	uint16_t vendor_id = 0, subvendor_id = 0, device_id = 0, subsystem_id = 0;
	uint8_t revision = 0;
	char permanent_address[AMDSMI_MAX_STRING_LENGTH] = {0};
	char product_name[AMDSMI_MAX_STRING_LENGTH];
	char vendor_name[AMDSMI_MAX_STRING_LENGTH];
	char part_number[AMDSMI_MAX_STRING_LENGTH];
	char serial_number[AMDSMI_MAX_STRING_LENGTH];
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_processor_type *)processor_handle);
	if (type != SMI_PROCESSOR_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	memset(info, 0, sizeof(amdsmi_nic_asic_info_t));

	nic = ((struct smi_nic_handle *)processor_handle);

	ret = smi_get_nic_vendor_id(smi_req.thread->nic_ctx, nic->bdf.as_uint, &vendor_id);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get vendor ID. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	ret = smi_get_nic_subvendor_id(smi_req.thread->nic_ctx, nic->bdf.as_uint, &subvendor_id);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get subvendor ID. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	ret = smi_get_nic_device_id(smi_req.thread->nic_ctx, nic->bdf.as_uint, &device_id);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get device ID. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	ret = smi_get_nic_subsystem_id(smi_req.thread->nic_ctx, nic->bdf.as_uint, &subsystem_id);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get subsystem ID. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	ret = smi_get_nic_revision(smi_req.thread->nic_ctx, nic->bdf.as_uint, &revision);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get revision. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	ret = smi_get_nic_perm_address(smi_req.thread->nic_ctx, nic->bdf.as_uint, permanent_address, sizeof(permanent_address));
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get permanent mac address. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	ret = smi_get_nic_product_name(smi_req.thread->nic_ctx, nic->bdf.as_uint, product_name, sizeof(product_name));
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get product name. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	ret = smi_get_nic_vendor_name(smi_req.thread->nic_ctx, nic->bdf.as_uint, vendor_name, sizeof(vendor_name));
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get vendor name. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	ret = smi_get_nic_part_number(smi_req.thread->nic_ctx, nic->bdf.as_uint, part_number, sizeof(part_number));
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get part number. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	ret = smi_get_nic_serial_number(smi_req.thread->nic_ctx, nic->bdf.as_uint, serial_number, sizeof(serial_number));
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get serial number. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	info->vendor_id = vendor_id;
	info->subvendor_id = subvendor_id;
	info->device_id = device_id;
	info->subsystem_id = subsystem_id;
	info->revision = revision;
	sys_wrapper->smi_strncpy(info->permanent_address, sizeof(info->permanent_address), permanent_address, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->product_name, sizeof(info->product_name), product_name, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->vendor_name, sizeof(info->vendor_name), vendor_name, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->part_number, sizeof(info->part_number), part_number, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->serial_number, sizeof(info->serial_number), serial_number, AMDSMI_MAX_STRING_LENGTH);

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_bus_info(amdsmi_processor_handle processor_handle, amdsmi_nic_bus_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_processor_type type;

	char pcie_interface_version[AMDSMI_MAX_STRING_LENGTH] = {0};
	char slot_type[AMDSMI_MAX_STRING_LENGTH] = {0};
	uint8_t max_pcie_width;
	uint32_t max_pcie_speed;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_processor_type *)processor_handle);
	if (type != SMI_PROCESSOR_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	nic = ((struct smi_nic_handle *)processor_handle);
	memset(info, 0, sizeof(amdsmi_nic_bus_info_t));

	info->bdf = nic->bdf;

	ret = smi_get_nic_max_pcie_width(smi_req.thread->nic_ctx, nic->bdf.as_uint, &max_pcie_width);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get max_pcie_width info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}
	ret = smi_get_nic_max_pcie_speed(smi_req.thread->nic_ctx, nic->bdf.as_uint, &max_pcie_speed);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get max_pcie_speed info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}
	ret = smi_get_nic_pcie_interface_version(smi_req.thread->nic_ctx, nic->bdf.as_uint, pcie_interface_version, sizeof(pcie_interface_version));
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get pcie_interface_version info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}
	ret = smi_get_nic_slot_type(smi_req.thread->nic_ctx, nic->bdf.as_uint, slot_type, sizeof(slot_type));
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get slot_type info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	info->max_pcie_speed = max_pcie_speed;
	info->max_pcie_width = max_pcie_width;
	sys_wrapper->smi_strncpy(info->pcie_interface_version, sizeof(info->pcie_interface_version), pcie_interface_version, AMDSMI_MAX_STRING_LENGTH);
	sys_wrapper->smi_strncpy(info->slot_type, sizeof(info->slot_type), slot_type, AMDSMI_MAX_STRING_LENGTH);


	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_numa_info(amdsmi_processor_handle processor_handle, amdsmi_nic_numa_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_processor_type type;
	uint8_t numa_node;
	char affinity[AMDSMI_MAX_STRING_LENGTH] = {0};
	int ret = 0;
	system_wrapper *sys_wrapper = get_system_wrapper();

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_processor_type *)processor_handle);
	if (type != SMI_PROCESSOR_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	nic = ((struct smi_nic_handle *)processor_handle);
	memset(info, 0, sizeof(amdsmi_nic_numa_info_t));

	ret = smi_get_nic_numa_node(smi_req.thread->nic_ctx, nic->bdf.as_uint, &numa_node);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get nic numa info. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}
	ret = smi_get_nic_numa_affinity(smi_req.thread->nic_ctx, nic->bdf.as_uint, numa_node, affinity, sizeof(affinity));
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get nic numa affinity. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	info->node = numa_node;
	sys_wrapper->smi_strncpy(info->affinity, sizeof(info->affinity), affinity, AMDSMI_MAX_STRING_LENGTH);

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_port_info(amdsmi_processor_handle processor_handle, amdsmi_nic_port_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_processor_type type;
	uint32_t num_ports = 0;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_processor_type *)processor_handle);
	if (type != SMI_PROCESSOR_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	nic = ((struct smi_nic_handle *)processor_handle);
	memset(info, 0, sizeof(amdsmi_nic_port_info_t));

	ret = smi_get_nic_ports_num(smi_req.thread->nic_ctx, nic->bdf.as_uint, &num_ports);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get number of ports. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	info->num_ports = num_ports;
	if (num_ports > AMDSMI_MAX_NIC_PORTS) {
		info->num_ports = AMDSMI_MAX_NIC_PORTS;
		num_ports = AMDSMI_MAX_NIC_PORTS;
	}

	for (uint32_t port_idx = 0; port_idx < num_ports; port_idx++) {
		amdsmi_nic_port_t *port_info = &info->ports[port_idx];
		struct ethtool_pauseparam pause_info;
		struct ethtool_fecparam fecparam_info;
		struct ethtool_link_settings link_settings;
		uint8_t ifindex, carrier;
		uint16_t mtu;
		uint32_t link_speed;
		uint32_t port_num;
		char port_type[AMDSMI_MAX_STRING_LENGTH] = {0};
		char mac_address[AMDSMI_MAX_STRING_LENGTH] = {0};
		char link_state[AMDSMI_MAX_STRING_LENGTH] = {0};
		char flavour[AMDSMI_MAX_STRING_LENGTH] = {0};
		char port_interface[AMDSMI_MAX_STRING_LENGTH] = {0};

		ret = smi_get_nic_port_interface(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, port_interface, sizeof(port_interface));
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			SMI_ERROR("Failed to get port %d interface name. Return code: %d", port_idx, ret);
			continue;
		}

		uint64_t port_bdf;
		ret = smi_get_nic_port_bdf(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, &port_bdf);
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			port_info->bdf.as_uint = port_bdf;
		} else {
			port_info->bdf = nic->bdf;
		}
		sys_wrapper->smi_strncpy(port_info->netdev, sizeof(port_info->netdev), port_interface, AMDSMI_MAX_STRING_LENGTH);

		ret = smi_get_nic_port_num_by_index(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, &port_num);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			SMI_ERROR("Failed to get port %d number. Return code: %d", port_idx, ret);
			port_num = port_idx;
		}
		port_info->port_num = port_num;

		ret = smi_get_nic_flavour(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, flavour, sizeof(flavour));
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			sys_wrapper->smi_strncpy(port_info->flavour, sizeof(port_info->flavour), flavour, AMDSMI_MAX_STRING_LENGTH);
		}

		ret = smi_get_nic_port_type(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, port_type, sizeof(port_type));
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			sys_wrapper->smi_strncpy(port_info->type, sizeof(port_info->type), port_type, AMDSMI_MAX_STRING_LENGTH);
		}

		ret = smi_get_nic_ifindex(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, &ifindex);
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			port_info->ifindex = ifindex;
		}

		ret = smi_get_nic_carrier(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, &carrier);
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			port_info->carrier = carrier;
		}

		ret = smi_get_nic_mtu(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, &mtu);
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			port_info->mtu = mtu;
		}

		ret = smi_get_nic_mac_address(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, mac_address, sizeof(mac_address));
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			sys_wrapper->smi_strncpy(port_info->mac_address, sizeof(port_info->mac_address), mac_address, AMDSMI_MAX_STRING_LENGTH);
		}

		ret = smi_get_nic_link_state(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, link_state, sizeof(link_state));
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			sys_wrapper->smi_strncpy(port_info->link_state, sizeof(port_info->link_state),
				((strcmp(link_state, "yes") == 0) ? "UP" : "DOWN"), AMDSMI_MAX_STRING_LENGTH);
		}

		ret = smi_get_nic_link_speed(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, &link_speed);
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			port_info->link_speed = link_speed;
		}

		ret = smi_get_nic_fecparam_info(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, &fecparam_info);
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			port_info->active_fec = fecparam_info.active_fec;
		}

		ret = smi_get_nic_link_settings_info(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, &link_settings);
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			sys_wrapper->smi_strncpy(port_info->autoneg, sizeof(port_info->autoneg),
				(link_settings.autoneg ? "ENABLED" : "DISABLED"), AMDSMI_MAX_STRING_LENGTH);
		}

		ret = smi_get_nic_pause_info(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_idx, &pause_info);
		if (ret == SMI_NIC_STATUS_SUCCESS) {
			sys_wrapper->smi_strncpy(port_info->pause_autoneg, sizeof(port_info->pause_autoneg),
				(pause_info.autoneg ? "ON" : "OFF"), AMDSMI_MAX_STRING_LENGTH);
			sys_wrapper->smi_strncpy(port_info->pause_rx, sizeof(port_info->pause_rx),
				(pause_info.rx_pause ? "ON" : "OFF"), AMDSMI_MAX_STRING_LENGTH);
			sys_wrapper->smi_strncpy(port_info->pause_tx, sizeof(port_info->pause_tx),
				(pause_info.tx_pause ? "ON" : "OFF"), AMDSMI_MAX_STRING_LENGTH);
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_vendor_statistics(amdsmi_processor_handle processor_handle, uint32_t port_index,
                                                 uint32_t *num_stats, amdsmi_nic_stat_t *stats)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_processor_type type;
	smi_nic_stat_list smi_stats;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || num_stats == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_processor_type *)processor_handle);
	if (type != SMI_PROCESSOR_TYPE_AMD_NIC) {
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
		ret = smi_get_nic_vendor_statistics_list(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_index, &smi_stats);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			SMI_ERROR("Failed to get nic vendor statistics list. Return code: %d", ret);
			return smi_map_nic_status(ret);
		}

		if (*num_stats >= smi_stats.count) {
			*num_stats = smi_stats.count;
		}

		for (uint32_t i = 0; i < *num_stats; i++) {
			sys_wrapper->smi_strncpy(stats[i].name, sizeof(stats[i].name), smi_stats.stats[i].name, AMDSMI_MAX_STRING_LENGTH);
			stats[i].value = smi_stats.stats[i].value;
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
	enum smi_processor_type type;
	smi_nic_stat_list smi_stats;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || num_stats == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_processor_type *)processor_handle);
	if (type != SMI_PROCESSOR_TYPE_AMD_NIC) {
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
		ret = smi_get_nic_port_statistics_list(smi_req.thread->nic_ctx, nic->bdf.as_uint, port_index, &smi_stats);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			SMI_ERROR("Failed to get nic port statistics list. Return code: %d", ret);
			return smi_map_nic_status(ret);
		}

		if (*num_stats >= smi_stats.count) {
			*num_stats = smi_stats.count;
		}

		for (uint32_t i = 0; i < *num_stats; i++) {
			sys_wrapper->smi_strncpy(stats[i].name, sizeof(stats[i].name), smi_stats.stats[i].name, AMDSMI_MAX_STRING_LENGTH);
			stats[i].value = smi_stats.stats[i].value;
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_rdma_dev_info(amdsmi_processor_handle processor_handle, amdsmi_nic_rdma_devices_info_t *info)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_processor_type type;
	char rdma_dev[AMDSMI_MAX_STRING_LENGTH] = {0};
	char node_guid[AMDSMI_MAX_STRING_LENGTH] = {0};
	char node_type[AMDSMI_MAX_STRING_LENGTH] = {0};
	char sys_image_guid[AMDSMI_MAX_STRING_LENGTH] = {0};
	char fw_ver[AMDSMI_MAX_STRING_LENGTH] = {0};
	char state[AMDSMI_MAX_STRING_LENGTH] = {0};
	uint8_t ib_count;
	uint16_t max_mtu;
	uint16_t active_mtu;
	uint32_t num_nic_ports = 0;
	uint8_t total_rdma_dev_count = 0;
	int ret = AMDSMI_STATUS_SUCCESS;
	system_wrapper *sys_wrapper = get_system_wrapper();
	uint8_t driver_not_loaded = 0;
	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || info == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_processor_type *)processor_handle);
	if (type != SMI_PROCESSOR_TYPE_AMD_NIC) {
		SMI_ERROR("Wrong processor handle. Return code: %d", AMDSMI_STATUS_INVAL);
		return AMDSMI_STATUS_INVAL;
	}

	nic = ((struct smi_nic_handle *)processor_handle);
	memset(info, 0, sizeof(amdsmi_nic_rdma_devices_info_t));

	ret = smi_get_nic_ports_num(smi_req.thread->nic_ctx, nic->bdf.as_uint, &num_nic_ports);
	if (ret != SMI_NIC_STATUS_SUCCESS) {
		SMI_ERROR("Failed to get number of NIC ports. Return code: %d", ret);
		return smi_map_nic_status(ret);
	}

	for (uint32_t nic_idx = 0; nic_idx < num_nic_ports; nic_idx++) {
		ret = smi_get_nic_infiniband_num(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, &ib_count);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			if (ret == SMI_NIC_STATUS_DRIVER_NOT_LOADED) {
				driver_not_loaded = 1;
			}
			SMI_ERROR("Failed to get number of InfiniBand devices on NIC port %d. Return code: %d", nic_idx, ret);
			continue;
		}

		for (uint8_t ib_idx = 0; ib_idx < ib_count; ib_idx++) {
			if (total_rdma_dev_count >= AMDSMI_MAX_NIC_RDMA_DEV) {
				SMI_ERROR("Maximum number of RDMA devices (%d) exceeded", AMDSMI_MAX_NIC_RDMA_DEV);
				break;
			}

			ret = smi_get_infiniband_rdma_dev(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, ib_idx, rdma_dev, sizeof(rdma_dev));
			if (ret != SMI_NIC_STATUS_SUCCESS) {
				SMI_ERROR("Failed to get InfiniBand[%d:%d] rdma dev. Return code: %d", nic_idx, ib_idx, ret);
				continue;
			}
			sys_wrapper->smi_strncpy(info->rdma_dev_info[total_rdma_dev_count].rdma_dev, sizeof(info->rdma_dev_info[total_rdma_dev_count].rdma_dev), rdma_dev, AMDSMI_MAX_STRING_LENGTH);

			ret = smi_get_infiniband_node_guid(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, ib_idx, node_guid, sizeof(node_guid));
			if (ret != SMI_NIC_STATUS_SUCCESS) {
				SMI_ERROR("Failed to get InfiniBand[%d:%d] node guid. Return code: %d", nic_idx, ib_idx, ret);
			}
			sys_wrapper->smi_strncpy(info->rdma_dev_info[total_rdma_dev_count].node_guid, sizeof(info->rdma_dev_info[total_rdma_dev_count].node_guid), node_guid, AMDSMI_MAX_STRING_LENGTH);

			ret = smi_get_infiniband_node_type(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, ib_idx, node_type, sizeof(node_type));
			if (ret != SMI_NIC_STATUS_SUCCESS) {
				SMI_ERROR("Failed to get InfiniBand[%d:%d] node type. Return code: %d", nic_idx, ib_idx, ret);
			}
			sys_wrapper->smi_strncpy(info->rdma_dev_info[total_rdma_dev_count].node_type, sizeof(info->rdma_dev_info[total_rdma_dev_count].node_type), node_type, AMDSMI_MAX_STRING_LENGTH);

			ret = smi_get_infiniband_sys_image_guid(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, ib_idx, sys_image_guid, sizeof(sys_image_guid));
			if (ret != SMI_NIC_STATUS_SUCCESS) {
				SMI_ERROR("Failed to get InfiniBand[%d:%d] sys image guid. Return code: %d", nic_idx, ib_idx, ret);
			}
			sys_wrapper->smi_strncpy(info->rdma_dev_info[total_rdma_dev_count].sys_image_guid, sizeof(info->rdma_dev_info[total_rdma_dev_count].sys_image_guid), sys_image_guid, AMDSMI_MAX_STRING_LENGTH);

			ret = smi_get_infiniband_fw_ver(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, ib_idx, fw_ver, sizeof(fw_ver));
			if (ret != SMI_NIC_STATUS_SUCCESS) {
				SMI_ERROR("Failed to get InfiniBand[%d:%d] firmware version. Return code: %d", nic_idx, ib_idx, ret);
			}
			sys_wrapper->smi_strncpy(info->rdma_dev_info[total_rdma_dev_count].fw_ver, sizeof(info->rdma_dev_info[total_rdma_dev_count].fw_ver), fw_ver, AMDSMI_MAX_STRING_LENGTH);

			uint8_t num_ports = 0;
			ret = smi_get_infiniband_num_ports(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, ib_idx, &num_ports);
			if (ret != SMI_NIC_STATUS_SUCCESS) {
				SMI_ERROR("Failed to get InfiniBand[%d:%d] number of ports. Return code: %d", nic_idx, ib_idx, ret);
				num_ports = 0;
			}
			info->rdma_dev_info[total_rdma_dev_count].num_rdma_ports = num_ports;

			for (uint8_t port = 0; port < num_ports && port < AMDSMI_MAX_NIC_PORTS; port++) {
				uint8_t port_num = 0;
				ret = smi_get_infiniband_port_num(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, ib_idx, port, &port_num);
				info->rdma_dev_info[total_rdma_dev_count].rdma_port_info[port].rdma_port = port_num;

				char port_interface[AMDSMI_MAX_STRING_LENGTH] = {0};
				ret = smi_get_nic_port_interface(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, port_interface, sizeof(port_interface));
				sys_wrapper->smi_strncpy(info->rdma_dev_info[total_rdma_dev_count].rdma_port_info[port].netdev, sizeof(info->rdma_dev_info[total_rdma_dev_count].rdma_port_info[port].netdev), port_interface, AMDSMI_MAX_STRING_LENGTH);

				ret = smi_get_infiniband_port_state(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, ib_idx, port, state, sizeof(state));
				if (ret != SMI_NIC_STATUS_SUCCESS) {
					SMI_ERROR("Failed to get InfiniBand[%d:%d] port[%d] state. Return code: %d", nic_idx, ib_idx, port, ret);
				}
				sys_wrapper->smi_strncpy(info->rdma_dev_info[total_rdma_dev_count].rdma_port_info[port].state, sizeof(info->rdma_dev_info[total_rdma_dev_count].rdma_port_info[port].state), state, AMDSMI_MAX_STRING_LENGTH);

				ret = smi_get_infiniband_port_max_mtu(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, ib_idx, port, &max_mtu);
				if (ret != SMI_NIC_STATUS_SUCCESS) {
					SMI_ERROR("Failed to get InfiniBand[%d:%d] port[%d] max mtu. Return code: %d", nic_idx, ib_idx, port, ret);
					max_mtu = 0;
				}
				info->rdma_dev_info[total_rdma_dev_count].rdma_port_info[port].max_mtu = max_mtu;

				ret = smi_get_infiniband_port_active_mtu(smi_req.thread->nic_ctx, nic->bdf.as_uint, nic_idx, ib_idx, port, &active_mtu);
				if (ret != SMI_NIC_STATUS_SUCCESS) {
					SMI_ERROR("Failed to get InfiniBand[%d:%d] port[%d] active mtu. Return code: %d", nic_idx, ib_idx, port, ret);
					active_mtu = 0;
				}
				info->rdma_dev_info[total_rdma_dev_count].rdma_port_info[port].active_mtu = active_mtu;
			}

			total_rdma_dev_count++;
		}
	}

	info->num_rdma_dev = total_rdma_dev_count;

	if (driver_not_loaded == 1 && total_rdma_dev_count == 0) {
		return AMDSMI_STATUS_DRIVER_NOT_LOADED;
	}
	if (total_rdma_dev_count == 0) {
		return AMDSMI_STATUS_NO_DATA;
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t amdsmi_get_nic_rdma_port_statistics(amdsmi_processor_handle processor_handle, uint32_t rdma_port_index,
                                                    uint32_t *num_stats, amdsmi_nic_stat_t *stats)
{
	#pragma SMI_EXPORT
	smi_req_ctx smi_req;
	struct smi_nic_handle *nic = NULL;
	enum smi_processor_type type;
	smi_nic_stat_list smi_stats;
	system_wrapper *sys_wrapper = get_system_wrapper();
	int ret = 0;

	AMDSMI_ESCAPE_IF_NOT_INIT;

	if (processor_handle == NULL || num_stats == NULL) {
		return AMDSMI_STATUS_INVAL;
	}

	type = *((enum smi_processor_type *)processor_handle);
	if (type != SMI_PROCESSOR_TYPE_AMD_NIC) {
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
		ret = smi_get_nic_rdma_port_statistics_list(smi_req.thread->nic_ctx, nic->bdf.as_uint, 0, 0, rdma_port_index, &smi_stats);
		if (ret != SMI_NIC_STATUS_SUCCESS) {
			SMI_ERROR("Failed to get nic rdma port statistics list. Return code: %d", ret);
			return smi_map_nic_status(ret);
		}

		if (*num_stats >= smi_stats.count) {
			*num_stats = smi_stats.count;
		}

		for (uint32_t i = 0; i < *num_stats; i++) {
			sys_wrapper->smi_strncpy(stats[i].name, sizeof(stats[i].name), smi_stats.stats[i].name, AMDSMI_MAX_STRING_LENGTH);
			stats[i].value = smi_stats.stats[i].value;
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

#ifdef __linux__
#pragma GCC diagnostic pop
#endif

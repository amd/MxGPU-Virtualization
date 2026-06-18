/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "amdsmi.h"

int main(void)
{
	amdsmi_nic_driver_info_t driver_info;
	amdsmi_nic_fw_info_t fw_info;
	amdsmi_nic_asic_info_t asic_info;
	amdsmi_nic_port_info_t port_info;
	amdsmi_nic_bus_info_t bus_info;
	amdsmi_nic_numa_info_t numa_info;
	amdsmi_nic_rdma_devices_info_t nic_rdma_devices_info;
	amdsmi_link_type_t nic_link_type;
	amdsmi_socket_handle socket_handle = NULL;
	uint32_t nic_count = 0;
	uint32_t gpu_count = 0;
	amdsmi_processor_handle *gpu_handles = NULL;
	uint32_t amd_nic_count = 0;
	uint32_t brcm_nic_count = 0;
	int ret = 0;

	ret = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fprintf(stderr, "Failed to initialize amdsmi: %d\n", ret);
		return -1;
	}

	ret = amdsmi_get_processor_handles_by_type(socket_handle, AMDSMI_PROCESSOR_TYPE_AMD_NIC, NULL, &amd_nic_count);
	if (ret != AMDSMI_STATUS_SUCCESS && ret != AMDSMI_STATUS_NOT_FOUND) {
		fprintf(stderr, "Failed to get the number of AMD NICs: %d\n", ret);
		amdsmi_shut_down();
		return -1;
	}

	ret = amdsmi_get_processor_handles_by_type(socket_handle, AMDSMI_PROCESSOR_TYPE_BRCM_NIC, NULL, &brcm_nic_count);
	if (ret != AMDSMI_STATUS_SUCCESS && ret != AMDSMI_STATUS_NOT_FOUND) {
		fprintf(stderr, "Failed to get the number of Broadcom NICs: %d\n", ret);
		amdsmi_shut_down();
		return -1;
	}

	nic_count = amd_nic_count + brcm_nic_count;
	amdsmi_processor_handle *nic_handles = (amdsmi_processor_handle *)malloc(nic_count * sizeof(amdsmi_processor_handle));
	if (nic_handles == NULL) {
		fprintf(stderr, "Failed to allocate memory for NIC handles\n");
		return -1;
	}

	uint32_t total_count = 0;
	if (amd_nic_count > 0) {
		uint32_t amd_count = amd_nic_count;
		ret = amdsmi_get_processor_handles_by_type(socket_handle, AMDSMI_PROCESSOR_TYPE_AMD_NIC, nic_handles, &amd_count);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to get AMD NIC handles: %d\n", ret);
			free(nic_handles);
			amdsmi_shut_down();
			return -1;
		}
		total_count += amd_count;
	}

	if (brcm_nic_count > 0) {
		uint32_t brcm_count = brcm_nic_count;
		ret = amdsmi_get_processor_handles_by_type(socket_handle, AMDSMI_PROCESSOR_TYPE_BRCM_NIC, &nic_handles[total_count], &brcm_count);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to get Broadcom NIC handles: %d\n", ret);
			free(nic_handles);
			amdsmi_shut_down();
			return -1;
		}
		total_count += brcm_count;
	}

	nic_count = total_count;
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fprintf(stderr, "Failed to get NIC handles: %d\n", ret);
		free(nic_handles);
		amdsmi_shut_down();
		return -1;
	}

	// Get GPU handles for NIC-GPU topology
	ret = amdsmi_get_processor_handles_by_type(socket_handle, AMDSMI_PROCESSOR_TYPE_AMD_GPU, NULL, &gpu_count);
	if (ret == AMDSMI_STATUS_SUCCESS && gpu_count > 0) {
		gpu_handles = (amdsmi_processor_handle *)malloc(gpu_count * sizeof(amdsmi_processor_handle));
		if (gpu_handles != NULL) {
			ret = amdsmi_get_processor_handles_by_type(socket_handle, AMDSMI_PROCESSOR_TYPE_AMD_GPU, gpu_handles, &gpu_count);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				free(gpu_handles);
				gpu_handles = NULL;
				gpu_count = 0;
			}
		}
	}

	printf("Number of NICs: %u\n", nic_count);
	printf("Number of GPUs: %u\n", gpu_count);
	for (uint32_t i = 0; i < nic_count; i++) {
		printf("NIC %u:\n", i);
		ret = amdsmi_get_nic_driver_info(nic_handles[i], &driver_info);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to get NIC driver info for NIC %u: %d\n", i, ret);
			continue;
		}
		printf("Driver info:\n\t VERSION: %s\n",
			driver_info.version
		);
		ret = amdsmi_get_nic_asic_info(nic_handles[i], &asic_info);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to get NIC driver info for NIC %u: %d\n", i, ret);
			continue;
		}
		printf("ASIC:\n\t VENDOR ID: 0x%04x\n\t SUBVENDOR ID: 0x%04x\n\t"
			" DEVICE ID: 0x%04x\n\t SUBSYSTEM ID: 0x%04x\n\t REVISION: 0x%02x\n\t PERMANENT_ADDRESS: %s\n\t "
			"PRODUCT_NAME: %s\n\t PART_NUMBER: %s\n\t SERIAL_NUMBER: %s\n\t VENDOR_NAME: %s\n",
			asic_info.vendor_id,
			asic_info.subvendor_id,
			asic_info.device_id,
			asic_info.subsystem_id,
			asic_info.revision,
			asic_info.permanent_address,
			asic_info.product_name,
			asic_info.part_number,
			asic_info.serial_number,
			asic_info.vendor_name
		);

		ret = amdsmi_get_nic_fw_info(nic_handles[i], &fw_info);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to get NIC firmware info for NIC %u: %d\n", i, ret);
			continue;
		}
		printf("Firmware (%u entries):\n", fw_info.num_fw);
		for (uint32_t j = 0; j < fw_info.num_fw; j++) {
			const char *type_str = "unknown";
			switch (fw_info.fw[j].type) {
			case AMDSMI_NIC_FW_VERSION_TYPE_FIXED:   type_str = "fixed";   break;
			case AMDSMI_NIC_FW_VERSION_TYPE_RUNNING: type_str = "running"; break;
			case AMDSMI_NIC_FW_VERSION_TYPE_STORED:  type_str = "stored";  break;
			}
			printf("\t [%s] %s: %s\n", type_str, fw_info.fw[j].name, fw_info.fw[j].version);
		}

		ret = amdsmi_get_nic_bus_info(nic_handles[i], &bus_info);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to get NIC bus info for NIC %u: %d\n", i, ret);
			continue;
		}
		printf("BUS:\n\t BDF: %04x:%02x:%02x.%1x\n\t MAX_PCIE_WIDTH: %u\n\t MAX_PCIE_SPEED: %u\n",
			(unsigned int)bus_info.bdf.bdf.domain_number, bus_info.bdf.bdf.bus_number,
			bus_info.bdf.bdf.device_number, bus_info.bdf.bdf.function_number,
			bus_info.max_pcie_width,
			bus_info.max_pcie_speed
       		);
			printf("Driver:\n\t NAME: %s\n",
			driver_info.name
		);

		ret = amdsmi_get_nic_numa_info(nic_handles[i], &numa_info);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to get NIC numa info for NIC %u: %d\n", i, ret);
			continue;
		}
		printf("NUMA:\n\t NODE: %u\n", numa_info.node);
		printf("\t AFFINITY: %s\n", numa_info.affinity);

		// NIC-GPU Link Topology
		if (gpu_handles != NULL && gpu_count > 0) {
			printf("NIC-GPU Link Topology:\n");
			for (uint32_t j = 0; j < gpu_count; j++) {
				ret = amdsmi_topo_get_link_type(nic_handles[i], gpu_handles[j], NULL, &nic_link_type);
				if (ret == AMDSMI_STATUS_SUCCESS) {
					const char *link_type_str;
					switch (nic_link_type) {
					case AMDSMI_LINK_TYPE_PCIE:
						link_type_str = "PCIE";
						break;
					case AMDSMI_LINK_TYPE_NUMA:
						link_type_str = "NUMA";
						break;
					case AMDSMI_LINK_TYPE_XNUMA:
						link_type_str = "XNUMA";
						break;
					default:
						link_type_str = "UNKNOWN";
						break;
					}
					printf("\t NIC %u -> GPU %u: %s\n", i, j, link_type_str);
				} else {
					printf("\t NIC %u -> GPU %u: Failed to get link type (%d)\n", i, j, ret);
				}
			}
		}

		ret = amdsmi_get_nic_port_info(nic_handles[i], &port_info);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to get NIC port info for NIC %u: %d\n", i, ret);
			continue;
		}
		printf("Number of ports: %u\n", port_info.num_ports);
		for (uint32_t port_idx = 0; port_idx < port_info.num_ports; port_idx++) {
			printf("Port %u: %u\n\t Type: %s\n\t NETDEV: %s\n\t Ifindex: %u\n\t Mac address: %s\n\t Carrier: %u\n\t Mtu: %u\n\t "
				"Link state: %s\n\t Link speed: %u\n\t Active_fec: %u\n\t Autoneg: %s\n\t "
				"Pause_autoneg: %s\n\t RX Pause: %s\n\t TX Pause: %s\n",
				port_idx,
				port_info.ports[port_idx].port_num,
				port_info.ports[port_idx].type,
				port_info.ports[port_idx].netdev,
				port_info.ports[port_idx].ifindex,
				port_info.ports[port_idx].mac_address,
				port_info.ports[port_idx].carrier,
				port_info.ports[port_idx].mtu,
				port_info.ports[port_idx].link_state,
				port_info.ports[port_idx].link_speed,
				port_info.ports[port_idx].active_fec,
				port_info.ports[port_idx].autoneg,
				port_info.ports[port_idx].pause_autoneg,
				port_info.ports[port_idx].pause_rx,
				port_info.ports[port_idx].pause_tx
			);
		}
		ret = amdsmi_get_nic_rdma_dev_info(nic_handles[i], &nic_rdma_devices_info);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to get NIC port info for NIC %u: %d\n", i, ret);
			continue;
		}
		for (uint8_t i = 0; i < nic_rdma_devices_info.num_rdma_dev; ++i) {
			printf("RDMA_DEV:%s\n\t NODE_GUID: %s\n\t NODE_TYPE: %s\n\t SYS_IMAGE_GUID: %s\n\t FW_VER: %s\n\t",
				nic_rdma_devices_info.rdma_dev_info[i].rdma_dev,
				nic_rdma_devices_info.rdma_dev_info[i].node_guid,
				nic_rdma_devices_info.rdma_dev_info[i].node_type,
				nic_rdma_devices_info.rdma_dev_info[i].sys_image_guid,
				nic_rdma_devices_info.rdma_dev_info[i].fw_ver
			);
			for (uint8_t j = 0; j < nic_rdma_devices_info.rdma_dev_info[i].num_rdma_ports; ++j) {
				printf(" RDMA_PORT: %u\n\t\t NETDEV: %s\n\t\t STATE: %s\n\t\t"
					" MAX_MTU: %u\n\t\t ACTIVE_MTU: %u\n",
					nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].rdma_port,
					nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].netdev,
					nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].state,
					nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].max_mtu,
					nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].active_mtu
				);
			}
		}
		// METRIC

		for (uint32_t port_idx = 0; port_idx < port_info.num_ports; ++port_idx) {
			printf("NETDEV[%u]: %s\n\t STATISTICS:\n", port_idx,
				(port_info.ports[port_idx].netdev[0] != '\0') ? port_info.ports[port_idx].netdev : "N/A");
			uint32_t port_stats_count;
			ret = amdsmi_get_nic_port_statistics(nic_handles[i], port_idx, &port_stats_count, NULL);
			if (ret == AMDSMI_STATUS_SUCCESS && port_stats_count > 0) {
				amdsmi_nic_stat_t *port_stats = malloc(port_stats_count * sizeof(amdsmi_nic_stat_t));
				ret = amdsmi_get_nic_port_statistics(nic_handles[i], port_idx, &port_stats_count, port_stats);
				if (ret == AMDSMI_STATUS_SUCCESS) {
					for (uint32_t s = 0; s < port_stats_count; s++) {
						printf("\t\t%s: %lu\n", port_stats[s].name, port_stats[s].value);
					}
				}
				free(port_stats);
			}

			printf("\t VENDOR_STATISTICS:\n");
			uint32_t vendor_stats_count;
			ret = amdsmi_get_nic_vendor_statistics(nic_handles[i], port_idx, &vendor_stats_count, NULL);
			if (ret == AMDSMI_STATUS_SUCCESS && vendor_stats_count > 0) {
				amdsmi_nic_stat_t *vendor_stats = malloc(vendor_stats_count * sizeof(amdsmi_nic_stat_t));
				ret = amdsmi_get_nic_vendor_statistics(nic_handles[i], port_idx, &vendor_stats_count, vendor_stats);
				if (ret == AMDSMI_STATUS_SUCCESS) {
					for (uint32_t s = 0; s < vendor_stats_count; s++) {
						printf("\t\t%s: %lu\n", vendor_stats[s].name, vendor_stats[s].value);
					}
				}
				free(vendor_stats);
			}
		}
		for (uint8_t k = 0; k < nic_rdma_devices_info.num_rdma_dev; ++k) {
			printf("RDMA_DEV: %s\n", nic_rdma_devices_info.rdma_dev_info[k].rdma_dev);
			for (uint8_t j = 0; j < nic_rdma_devices_info.rdma_dev_info[k].num_rdma_ports; ++j) {
				printf("\t RDMA_PORT: %u\n", nic_rdma_devices_info.rdma_dev_info[k].rdma_port_info[j].rdma_port);
				uint32_t rdma_stats_count;
				ret = amdsmi_get_nic_rdma_port_statistics(nic_handles[i], nic_rdma_devices_info.rdma_dev_info[k].rdma_port_info[j].rdma_port, &rdma_stats_count, NULL);
				if (ret == AMDSMI_STATUS_SUCCESS && rdma_stats_count > 0) {
					amdsmi_nic_stat_t *rdma_stats = malloc(rdma_stats_count * sizeof(amdsmi_nic_stat_t));
					ret = amdsmi_get_nic_rdma_port_statistics(nic_handles[i], nic_rdma_devices_info.rdma_dev_info[k].rdma_port_info[j].rdma_port, &rdma_stats_count, rdma_stats);
					if (ret == AMDSMI_STATUS_SUCCESS) {
						for (uint32_t s = 0; s < rdma_stats_count; s++) {
							printf("\t\t%s: %lu\n", rdma_stats[s].name, rdma_stats[s].value);
						}
					}
					free(rdma_stats);
				}
			}
		}
		printf("\n");
	}

	if (nic_handles != NULL) {
		free(nic_handles);
	}
	if (gpu_handles != NULL) {
		free(gpu_handles);
	}
	ret = amdsmi_shut_down();
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fprintf(stderr, "Failed to shut down amdsmi: %d\n", ret);
		return -1;
	}

	return 0;
}

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "amdsmi.h"

int main(void)
{
	amdsmi_nic_driver_info_t driver_info;
	amdsmi_nic_asic_info_t asic_info;
	amdsmi_nic_port_info_t port_info;
	amdsmi_nic_bus_info_t bus_info;
	amdsmi_nic_numa_info_t numa_info;
	amdsmi_nic_rdma_devices_info_t nic_rdma_devices_info;
	amdsmi_socket_handle socket_handle = NULL;
	uint32_t processor_count = 0;
	int ret = 0;

	ret = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fprintf(stderr, "Failed to initialize amdsmi: %d\n", ret);
		return -1;
	}

	ret = amdsmi_get_processor_handles_by_type(socket_handle, AMDSMI_PROCESSOR_TYPE_AMD_NIC, NULL, &processor_count);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fprintf(stderr, "Failed to get the number of NICs: %d\n", ret);
		return -1;
	}

	amdsmi_processor_handle *processor_handles = (amdsmi_processor_handle *)malloc(processor_count * sizeof(amdsmi_processor_handle));
	if (processor_handles == NULL) {
		fprintf(stderr, "Failed to allocate memory for NIC handles\n");
		return -1;
	}

	ret = amdsmi_get_processor_handles_by_type(socket_handle, AMDSMI_PROCESSOR_TYPE_AMD_NIC, processor_handles, &processor_count);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fprintf(stderr, "Failed to get NIC handles: %d\n", ret);
		free(processor_handles);
		amdsmi_shut_down();
		return -1;
	}

	printf("Number of NICs: %u\n", processor_count);
	for (uint32_t i = 0; i < processor_count; i++) {
		printf("NIC %u:\n", i);
		ret = amdsmi_get_nic_driver_info(processor_handles[i], &driver_info);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to get NIC driver info for NIC %u: %d\n", i, ret);
			continue;
		}
		printf("Driver info:\n\t VERSION: %s\n",
			driver_info.version
		);
		ret = amdsmi_get_nic_asic_info(processor_handles[i], &asic_info);
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

		ret = amdsmi_get_nic_bus_info(processor_handles[i], &bus_info);
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

		ret = amdsmi_get_nic_numa_info(processor_handles[i], &numa_info);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to get NIC numa info for NIC %u: %d\n", i, ret);
			continue;
		}
		printf("NUMA:\n\t NODE: %u\n", numa_info.node);
		printf("\t AFFINITY: %s\n", numa_info.affinity);

		ret = amdsmi_get_nic_port_info(processor_handles[i], &port_info);
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
		ret = amdsmi_get_nic_rdma_dev_info(processor_handles[i], &nic_rdma_devices_info);
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
			ret = amdsmi_get_nic_port_statistics(processor_handles[i], port_idx, &port_stats_count, NULL);
			if (ret == AMDSMI_STATUS_SUCCESS && port_stats_count > 0) {
				amdsmi_nic_stat_t *port_stats = malloc(port_stats_count * sizeof(amdsmi_nic_stat_t));
				ret = amdsmi_get_nic_port_statistics(processor_handles[i], port_idx, &port_stats_count, port_stats);
				if (ret == AMDSMI_STATUS_SUCCESS) {
					for (uint32_t s = 0; s < port_stats_count; s++) {
						printf("\t\t%s: %lu\n", port_stats[s].name, port_stats[s].value);
					}
				}
				free(port_stats);
			}

			printf("\t VENDOR_STATISTICS:\n");
			uint32_t vendor_stats_count;
			ret = amdsmi_get_nic_vendor_statistics(processor_handles[i], port_idx, &vendor_stats_count, NULL);
			if (ret == AMDSMI_STATUS_SUCCESS && vendor_stats_count > 0) {
				amdsmi_nic_stat_t *vendor_stats = malloc(vendor_stats_count * sizeof(amdsmi_nic_stat_t));
				ret = amdsmi_get_nic_vendor_statistics(processor_handles[i], port_idx, &vendor_stats_count, vendor_stats);
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
				ret = amdsmi_get_nic_rdma_port_statistics(processor_handles[i], nic_rdma_devices_info.rdma_dev_info[k].rdma_port_info[j].rdma_port, &rdma_stats_count, NULL);
				if (ret == AMDSMI_STATUS_SUCCESS && rdma_stats_count > 0) {
					amdsmi_nic_stat_t *rdma_stats = malloc(rdma_stats_count * sizeof(amdsmi_nic_stat_t));
					ret = amdsmi_get_nic_rdma_port_statistics(processor_handles[i], nic_rdma_devices_info.rdma_dev_info[k].rdma_port_info[j].rdma_port, &rdma_stats_count, rdma_stats);
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

	free(processor_handles);
	ret = amdsmi_shut_down();
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fprintf(stderr, "Failed to shut down amdsmi: %d\n", ret);
		return -1;
	}

	return 0;
}

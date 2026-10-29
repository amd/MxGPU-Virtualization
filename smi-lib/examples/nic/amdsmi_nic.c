/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ethtool.h>
#include "amdsmi.h"

typedef amdsmi_status_t (*nic_stats_getter_t)(amdsmi_processor_handle processor_handle,
					      uint32_t index,
					      uint32_t *num_stats,
					      amdsmi_nic_stat_t *stats);

static const char *status_str(amdsmi_status_t status)
{
	const char *str = NULL;

	if (amdsmi_status_code_to_string(status, &str) != AMDSMI_STATUS_SUCCESS || str == NULL) {
		return "AMDSMI_STATUS_UNKNOWN_ERROR";
	}
	return str;
}

static void print_unavailable(const char *indent, const char *label, amdsmi_status_t status)
{
	printf("%s%s: N/A (%s)\n", indent, label, status_str(status));
}

/* The NIC library reports a numeric field it cannot read as the maximum value of its type. */
static void print_uint(
    const char *indent, const char *label, uint64_t value, uint64_t unavailable, const char *unit)
{
	if (value == unavailable) {
		printf("%s%s: N/A\n", indent, label);
	} else if (unit != NULL) {
		printf("%s%s: %" PRIu64 " %s\n", indent, label, value, unit);
	} else {
		printf("%s%s: %" PRIu64 "\n", indent, label, value);
	}
}

static void
print_hex(const char *indent, const char *label, uint64_t value, uint64_t unavailable, int width)
{
	if (value == unavailable) {
		printf("%s%s: N/A\n", indent, label);
	} else {
		printf("%s%s: 0x%0*" PRIx64 "\n", indent, label, width, value);
	}
}

static void format_bdf(char *buf, size_t len, amdsmi_bdf_t bdf)
{
	snprintf(buf,
		 len,
		 "%04x:%02x:%02x.%x",
		 (unsigned int)bdf.bdf.domain_number,
		 (unsigned int)bdf.bdf.bus_number,
		 (unsigned int)bdf.bdf.device_number,
		 (unsigned int)bdf.bdf.function_number);
}

static const char *nic_vendor_str(amdsmi_processor_handle nic)
{
	amdsmi_processor_type_t type = AMDSMI_PROCESSOR_TYPE_UNKNOWN;

	if (amdsmi_get_processor_type(nic, &type) != AMDSMI_STATUS_SUCCESS) {
		return "UNKNOWN";
	}
	switch (type) {
	case AMDSMI_PROCESSOR_TYPE_AMD_NIC:
		return "AMD";
	case AMDSMI_PROCESSOR_TYPE_BRCM_NIC:
		return "BROADCOM";
	default:
		return "UNKNOWN";
	}
}

static const char *fw_type_str(amdsmi_nic_fw_version_type_t type)
{
	switch (type) {
	case AMDSMI_NIC_FW_VERSION_TYPE_FIXED:
		return "fixed";
	case AMDSMI_NIC_FW_VERSION_TYPE_RUNNING:
		return "running";
	case AMDSMI_NIC_FW_VERSION_TYPE_STORED:
		return "stored";
	}
	return "unknown";
}

static const char *link_type_str(amdsmi_link_type_t type)
{
	switch (type) {
	case AMDSMI_LINK_TYPE_PCIE:
		return "PCIE";
	case AMDSMI_LINK_TYPE_NUMA:
		return "NUMA";
	case AMDSMI_LINK_TYPE_XNUMA:
		return "XNUMA";
	default:
		return "UNKNOWN";
	}
}

static void print_active_fec(const char *indent, uint32_t active_fec)
{
	static const struct {
		uint32_t mask;
		const char *name;
	} fec_modes[] = {
	    {ETHTOOL_FEC_NONE, "NONE"},
	    {ETHTOOL_FEC_AUTO, "AUTO"},
	    {ETHTOOL_FEC_OFF, "OFF"},
	    {ETHTOOL_FEC_RS, "RS"},
	    {ETHTOOL_FEC_BASER, "BASER"},
	    {ETHTOOL_FEC_LLRS, "LLRS"},
	};

	if (active_fec == UINT32_MAX) {
		printf("%sACTIVE_FEC: N/A\n", indent);
		return;
	}
	printf("%sACTIVE_FEC:", indent);
	for (size_t i = 0; i < sizeof(fec_modes) / sizeof(fec_modes[0]); i++) {
		if (active_fec & fec_modes[i].mask) {
			printf(" %s", fec_modes[i].name);
		}
	}
	printf(" (0x%x)\n", active_fec);
}

static void print_statistics(const char *indent,
			     const char *title,
			     nic_stats_getter_t get_stats,
			     amdsmi_processor_handle nic,
			     uint32_t index)
{
	amdsmi_nic_stat_t *stats = NULL;
	uint32_t count		 = 0;
	amdsmi_status_t ret;

	ret = get_stats(nic, index, &count, NULL);
	if (ret == AMDSMI_STATUS_SUCCESS && count > 0) {
		stats = calloc(count, sizeof(*stats));
		if (stats == NULL) {
			ret = AMDSMI_STATUS_OUT_OF_RESOURCES;
		} else {
			ret = get_stats(nic, index, &count, stats);
		}
	}

	if (ret != AMDSMI_STATUS_SUCCESS) {
		print_unavailable(indent, title, ret);
	} else if (count == 0) {
		printf("%s%s: none\n", indent, title);
	} else {
		printf("%s%s:\n", indent, title);
		for (uint32_t i = 0; i < count; i++) {
			printf("%s\t%s: %" PRIu64 "\n", indent, stats[i].name, stats[i].value);
		}
	}
	free(stats);
}

static void print_nic_header(uint32_t index, amdsmi_processor_handle nic)
{
	amdsmi_bdf_t bdf;
	char bdf_str[32] = "N/A";

	if (amdsmi_get_nic_device_bdf(nic, &bdf) == AMDSMI_STATUS_SUCCESS) {
		format_bdf(bdf_str, sizeof(bdf_str), bdf);
	}
	printf("NIC %u: %s %s\n", index, nic_vendor_str(nic), bdf_str);
}

static void print_driver_info(amdsmi_processor_handle nic)
{
	amdsmi_nic_driver_info_t info;
	amdsmi_status_t ret = amdsmi_get_nic_driver_info(nic, &info);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		print_unavailable("\t", "DRIVER", ret);
		return;
	}
	printf("\tDRIVER:\n");
	printf("\t\tNAME: %s\n", info.name);
	printf("\t\tVERSION: %s\n", info.version);
}

static void print_asic_info(amdsmi_processor_handle nic)
{
	amdsmi_nic_asic_info_t info;
	amdsmi_status_t ret = amdsmi_get_nic_asic_info(nic, &info);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		print_unavailable("\t", "ASIC", ret);
		return;
	}
	printf("\tASIC:\n");
	print_hex("\t\t", "VENDOR_ID", info.vendor_id, UINT16_MAX, 4);
	print_hex("\t\t", "SUBVENDOR_ID", info.subvendor_id, UINT16_MAX, 4);
	print_hex("\t\t", "DEVICE_ID", info.device_id, UINT16_MAX, 4);
	print_hex("\t\t", "SUBSYSTEM_ID", info.subsystem_id, UINT16_MAX, 4);
	print_hex("\t\t", "REVISION", info.revision, UINT8_MAX, 2);
	printf("\t\tPERMANENT_ADDRESS: %s\n", info.permanent_address);
	printf("\t\tPRODUCT_NAME: %s\n", info.product_name);
	printf("\t\tPART_NUMBER: %s\n", info.part_number);
	printf("\t\tSERIAL_NUMBER: %s\n", info.serial_number);
	printf("\t\tVENDOR_NAME: %s\n", info.vendor_name);
}

static void print_fw_info(amdsmi_processor_handle nic)
{
	amdsmi_nic_fw_info_t info;
	amdsmi_status_t ret = amdsmi_get_nic_fw_info(nic, &info);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		print_unavailable("\t", "FIRMWARE", ret);
		return;
	}
	if (info.num_fw == 0) {
		printf("\tFIRMWARE: none\n");
		return;
	}
	printf("\tFIRMWARE:\n");
	for (uint32_t i = 0; i < info.num_fw; i++) {
		printf("\t\t[%s] %s: %s\n",
		       fw_type_str(info.fw[i].type),
		       info.fw[i].fw.name,
		       info.fw[i].fw.version);
	}
}

static void print_bus_info(amdsmi_processor_handle nic)
{
	amdsmi_nic_bus_info_t info;
	char bdf_str[32];
	amdsmi_status_t ret = amdsmi_get_nic_bus_info(nic, &info);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		print_unavailable("\t", "BUS", ret);
		return;
	}
	format_bdf(bdf_str, sizeof(bdf_str), info.bdf);
	printf("\tBUS:\n");
	printf("\t\tBDF: %s\n", bdf_str);
	print_uint("\t\t", "MAX_PCIE_WIDTH", info.max_pcie_width, UINT8_MAX, NULL);
	print_uint("\t\t", "MAX_PCIE_SPEED", info.max_pcie_speed, UINT32_MAX, "GT/s");
	printf("\t\tPCIE_INTERFACE_VERSION: %s\n", info.pcie_interface_version);
	printf("\t\tSLOT_TYPE: %s\n", info.slot_type);
}

static void print_numa_info(amdsmi_processor_handle nic)
{
	amdsmi_nic_numa_info_t info;
	amdsmi_status_t ret = amdsmi_get_nic_numa_info(nic, &info);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		print_unavailable("\t", "NUMA", ret);
		return;
	}
	printf("\tNUMA:\n");
	print_uint("\t\t", "NODE", info.node, UINT8_MAX, NULL);
	printf("\t\tAFFINITY: %s\n", info.affinity);
}

static void print_link_type(amdsmi_processor_handle nic,
			    const char *peer_kind,
			    uint32_t peer_index,
			    amdsmi_processor_handle peer)
{
	amdsmi_link_type_t type = AMDSMI_LINK_TYPE_UNKNOWN;
	amdsmi_status_t ret	= amdsmi_topo_get_link_type(nic, peer, NULL, &type);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		printf("\t\t%s %u: N/A (%s)\n", peer_kind, peer_index, status_str(ret));
		return;
	}
	printf("\t\t%s %u: %s\n", peer_kind, peer_index, link_type_str(type));
}

static void print_topology(uint32_t nic_index,
			   amdsmi_processor_handle *nics,
			   uint32_t nic_count,
			   amdsmi_processor_handle *gpus,
			   uint32_t gpu_count)
{
	if (gpu_count == 0 && nic_count < 2) {
		return;
	}
	printf("\tTOPOLOGY:\n");
	for (uint32_t i = 0; i < gpu_count; i++) {
		print_link_type(nics[nic_index], "GPU", i, gpus[i]);
	}
	for (uint32_t i = 0; i < nic_count; i++) {
		if (i != nic_index) {
			print_link_type(nics[nic_index], "NIC", i, nics[i]);
		}
	}
}

static void
print_port(amdsmi_processor_handle nic, uint32_t port_index, const amdsmi_nic_port_t *port)
{
	char bdf_str[32];

	format_bdf(bdf_str, sizeof(bdf_str), port->bdf);
	printf("\tPORT %u:\n", port_index);
	printf("\t\tBDF: %s\n", bdf_str);
	print_uint("\t\t", "PORT_NUM", port->port_num, UINT32_MAX, NULL);
	printf("\t\tTYPE: %s\n", port->type);
	printf("\t\tFLAVOUR: %s\n", port->flavour);
	printf("\t\tNETDEV: %s\n", port->netdev);
	print_uint("\t\t", "IFINDEX", port->ifindex, UINT8_MAX, NULL);
	printf("\t\tMAC_ADDRESS: %s\n", port->mac_address);
	print_uint("\t\t", "CARRIER", port->carrier, UINT8_MAX, NULL);
	print_uint("\t\t", "MTU", port->mtu, UINT16_MAX, "B");
	printf("\t\tLINK_STATE: %s\n", port->link_state);
	print_uint("\t\t", "LINK_SPEED", port->link_speed, UINT32_MAX, "Mb/s");
	print_active_fec("\t\t", port->active_fec);
	printf("\t\tAUTONEG: %s\n", port->autoneg);
	printf("\t\tPAUSE_AUTONEG: %s\n", port->pause_autoneg);
	printf("\t\tPAUSE_RX: %s\n", port->pause_rx);
	printf("\t\tPAUSE_TX: %s\n", port->pause_tx);
	print_statistics("\t\t", "STATISTICS", amdsmi_get_nic_port_statistics, nic, port_index);
	print_statistics(
	    "\t\t", "VENDOR_STATISTICS", amdsmi_get_nic_vendor_statistics, nic, port_index);
}

static void print_ports(amdsmi_processor_handle nic)
{
	amdsmi_nic_port_info_t info;
	amdsmi_status_t ret = amdsmi_get_nic_port_info(nic, &info);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		print_unavailable("\t", "PORTS", ret);
		return;
	}
	for (uint32_t i = 0; i < info.num_ports; i++) {
		print_port(nic, i, &info.ports[i]);
	}
}

static void print_rdma_devices(amdsmi_processor_handle nic)
{
	amdsmi_nic_rdma_devices_info_t *info = calloc(1, sizeof(*info));
	amdsmi_status_t ret;

	if (info == NULL) {
		print_unavailable("\t", "RDMA_DEVICES", AMDSMI_STATUS_OUT_OF_RESOURCES);
		return;
	}
	ret = amdsmi_get_nic_rdma_dev_info(nic, info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		print_unavailable("\t", "RDMA_DEVICES", ret);
		free(info);
		return;
	}

	uint32_t rdma_port_index = 0;

	for (uint32_t i = 0; i < info->num_rdma_dev; i++) {
		const amdsmi_nic_rdma_dev_info_t *dev = &info->rdma_dev_info[i];

		printf("\tRDMA_DEVICE %u: %s\n", i, dev->rdma_dev);
		printf("\t\tNODE_GUID: %s\n", dev->node_guid);
		printf("\t\tNODE_TYPE: %s\n", dev->node_type);
		printf("\t\tSYS_IMAGE_GUID: %s\n", dev->sys_image_guid);
		printf("\t\tFW_VER: %s\n", dev->fw_ver);
		for (uint32_t j = 0; j < dev->num_rdma_ports; j++) {
			const amdsmi_nic_rdma_port_info_t *port = &dev->rdma_port_info[j];

			printf("\t\tRDMA_PORT %u:\n", j);
			print_uint("\t\t\t", "PORT_NUM", port->rdma_port, UINT8_MAX, NULL);
			printf("\t\t\tNETDEV: %s\n", port->netdev);
			printf("\t\t\tSTATE: %s\n", port->state);
			print_uint("\t\t\t", "MAX_MTU", port->max_mtu, UINT16_MAX, "B");
			print_uint("\t\t\t", "ACTIVE_MTU", port->active_mtu, UINT16_MAX, "B");
			/* Position of this port in amdsmi_get_nic_rdma_dev_info() order. */
			print_statistics("\t\t\t",
					 "STATISTICS",
					 amdsmi_get_nic_rdma_port_statistics,
					 nic,
					 rdma_port_index);
			rdma_port_index++;
		}
	}
	free(info);
}

static void print_nic(uint32_t index,
		      amdsmi_processor_handle *nics,
		      uint32_t nic_count,
		      amdsmi_processor_handle *gpus,
		      uint32_t gpu_count)
{
	amdsmi_processor_handle nic = nics[index];

	print_nic_header(index, nic);
	print_driver_info(nic);
	print_asic_info(nic);
	print_fw_info(nic);
	print_bus_info(nic);
	print_numa_info(nic);
	print_topology(index, nics, nic_count, gpus, gpu_count);
	print_ports(nic);
	print_rdma_devices(nic);
	printf("\n");
}

static void print_gpus(amdsmi_processor_handle *gpus, uint32_t gpu_count)
{
	printf("Number of GPUs: %u\n", gpu_count);
	for (uint32_t i = 0; i < gpu_count; i++) {
		amdsmi_bdf_t bdf;
		char bdf_str[32] = "N/A";

		if (amdsmi_get_gpu_device_bdf(gpus[i], &bdf) == AMDSMI_STATUS_SUCCESS) {
			format_bdf(bdf_str, sizeof(bdf_str), bdf);
		}
		printf("\tGPU %u: %s\n", i, bdf_str);
	}
}

static amdsmi_status_t get_nic_handles(amdsmi_processor_handle **handles, uint32_t *count)
{
	amdsmi_status_t ret;

	*handles = NULL;
	*count	 = 0;
	ret	 = amdsmi_get_nic_processor_handles(NULL, count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS || *count == 0) {
		return ret;
	}

	*handles = calloc(*count, sizeof(**handles));
	if (*handles == NULL) {
		*count = 0;
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}
	ret = amdsmi_get_nic_processor_handles(NULL, count, *handles);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		*count = 0;
	}
	return ret;
}

static amdsmi_status_t get_gpu_handles(amdsmi_processor_handle **handles, uint32_t *count)
{
	amdsmi_status_t ret;

	*handles = NULL;
	*count	 = 0;
	ret =
	    amdsmi_get_processor_handles_by_type(NULL, AMDSMI_PROCESSOR_TYPE_AMD_GPU, NULL, count);
	if (ret != AMDSMI_STATUS_SUCCESS || *count == 0) {
		return ret;
	}

	*handles = calloc(*count, sizeof(**handles));
	if (*handles == NULL) {
		*count = 0;
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}
	ret = amdsmi_get_processor_handles_by_type(
	    NULL, AMDSMI_PROCESSOR_TYPE_AMD_GPU, *handles, count);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		*count = 0;
	}
	return ret;
}

int main(void)
{
	amdsmi_processor_handle *nics = NULL;
	amdsmi_processor_handle *gpus = NULL;
	uint32_t nic_count	      = 0;
	uint32_t gpu_count	      = 0;
	int exit_code		      = EXIT_FAILURE;
	amdsmi_status_t ret;

	ret = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fprintf(stderr, "Failed to initialize amdsmi: %s\n", status_str(ret));
		return EXIT_FAILURE;
	}

	ret = get_nic_handles(&nics, &nic_count);
	if (ret == AMDSMI_STATUS_DRIVER_NOT_LOADED) {
		printf("No NIC driver (ionic or bnxt_en) is loaded\n");
		exit_code = EXIT_SUCCESS;
		goto out;
	}
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fprintf(stderr, "Failed to get NIC handles: %s\n", status_str(ret));
		goto out;
	}

	ret = get_gpu_handles(&gpus, &gpu_count);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		printf("GPU handles unavailable, skipping NIC-GPU topology: %s\n", status_str(ret));
	}

	print_gpus(gpus, gpu_count);
	printf("Number of NICs: %u\n\n", nic_count);
	for (uint32_t i = 0; i < nic_count; i++) {
		print_nic(i, nics, nic_count, gpus, gpu_count);
	}
	exit_code = EXIT_SUCCESS;

out:
	free(gpus);
	free(nics);
	ret = amdsmi_shut_down();
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fprintf(stderr, "Failed to shut down amdsmi: %s\n", status_str(ret));
		exit_code = EXIT_FAILURE;
	}
	return exit_code;
}

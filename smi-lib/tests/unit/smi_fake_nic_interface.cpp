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

extern "C" {
#include "smi_nic_interface.h"
}

#include <cstring>
#include <cstdlib>
#include <string>
#include "smi_fake_nic_interface.h"

static bool g_nic_init_flag = true;
static bool g_nic_cleanup_flag = true;
static bool g_nic_discovery_flag = true;
static bool g_use_long_interface_name = false;
static smi_nic_status_t g_nic_api_status_code = SMI_NIC_STATUS_SUCCESS;

extern "C" {

bool get_nic_init()
{
	return g_nic_init_flag;
}

void set_nic_init(bool flag)
{
	g_nic_init_flag = flag;
}

bool get_nic_cleanup()
{
	return g_nic_cleanup_flag;
}

void set_nic_cleanup(bool flag)
{
	g_nic_cleanup_flag = flag;
}

bool get_nic_discovery()
{
	return g_nic_discovery_flag;
}

void set_nic_discovery(bool flag)
{
	g_nic_discovery_flag = flag;
}

void set_nic_long_interface_name(bool flag)
{
	g_use_long_interface_name = flag;
}

smi_nic_status_t get_nic_api_status()
{
	return g_nic_api_status_code;
}

void set_nic_api_status(smi_nic_status_t status)
{
	g_nic_api_status_code = status;
}

smi_nic_status_t smi_nic_create_context(smi_nic_ctx_t *ctx)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!g_nic_init_flag) {
		return SMI_NIC_STATUS_ERROR;
	}

	*ctx = reinterpret_cast<smi_nic_ctx_t>(malloc(1));
	if (!*ctx) {
		return SMI_NIC_STATUS_NO_RESOURCE;
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status_t smi_nic_destroy_context(smi_nic_ctx_t ctx)
{
	if (ctx) {
		free(ctx);
	}

	if (!g_nic_cleanup_flag) {
		return SMI_NIC_STATUS_ERROR;
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status_t smi_nic_driver_loaded(smi_nic_vendor_t vendor)
{
	(void)vendor;
	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status_t smi_discover_nics(smi_nic_ctx_t ctx, smi_nic_discovery_t *discovery)
{
	(void)ctx;
	if (!g_nic_discovery_flag) {
		return SMI_NIC_STATUS_ERROR;
	}

	if (!discovery) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	memset(discovery, 0, sizeof(smi_nic_discovery_t));
	discovery->count = 2;

	const char* mock_bdf_amd = "0001:02:03.4";
	const char* mock_bdf_broadcom = "0001:02:04.6";

	std::snprintf(discovery->devices[0].bdf, SMI_NIC_MAX_STRING_LENGTH,
		"%s", mock_bdf_amd);
	discovery->devices[0].vendor = SMI_NIC_VENDOR_AMD;

	std::snprintf(discovery->devices[1].bdf, SMI_NIC_MAX_STRING_LENGTH,
		"%s", mock_bdf_broadcom);
	discovery->devices[1].vendor = SMI_NIC_VENDOR_BROADCOM;

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status_t smi_get_nic_driver_info(smi_nic_ctx_t ctx, uint64_t device, smi_nic_driver_info_t *info)
{
	(void)ctx;
	(void)device;
	if (info) {
		std::snprintf(info->name, SMI_NIC_MAX_STRING_LENGTH, "%s", "driver_mock");
		std::snprintf(info->version, SMI_NIC_MAX_STRING_LENGTH, "%s", "1.0.0");
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_get_nic_asic_info(smi_nic_ctx_t ctx, uint64_t device, smi_nic_asic_info_t *info)
{
	(void)ctx;
	(void)device;
	if (info) {
		info->vendor_id = 0x1002;
		info->subvendor_id = 0x1234;
		info->device_id = 0x5678;
		info->subsystem_id = 0x9ABC;
		info->revision = 0x01;
		std::snprintf(info->permanent_address, SMI_NIC_MAX_STRING_LENGTH, "%s", "aa:bb:cc:dd:ee:ff");
		std::snprintf(info->product_name, SMI_NIC_MAX_STRING_LENGTH, "%s", "AMD NIC");
		std::snprintf(info->vendor_name, SMI_NIC_MAX_STRING_LENGTH, "%s", "AMD");
		std::snprintf(info->part_number, SMI_NIC_MAX_STRING_LENGTH, "%s", "AMD-NIC-1234");
		std::snprintf(info->serial_number, SMI_NIC_MAX_STRING_LENGTH, "%s", "SN0123456789");
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_get_nic_bus_info(smi_nic_ctx_t ctx, uint64_t device, smi_nic_bus_info_t *info)
{
	(void)ctx;
	(void)device;
	if (info) {
		info->bdf = device;
		info->max_pcie_width = 16;
		info->max_pcie_speed = 16;
		std::snprintf(info->pcie_interface_version, SMI_NIC_MAX_STRING_LENGTH, "%s", "Gen 4");
		std::snprintf(info->slot_type, SMI_NIC_MAX_STRING_LENGTH, "%s", "OAM");
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_get_nic_numa_info(smi_nic_ctx_t ctx, uint64_t device, smi_nic_numa_info_t *info)
{
	(void)ctx;
	(void)device;
	if (info) {
		info->node = 0;
		std::snprintf(info->affinity, SMI_NIC_MAX_STRING_LENGTH, "%s", "0,1,2,3");
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_get_nic_port_info(smi_nic_ctx_t ctx, uint64_t device, smi_nic_port_info_t *info)
{
	(void)ctx;
	(void)device;
	if (info) {
		info->num_ports = 1;

		smi_nic_port_t *port = &info->ports[0];
		port->bdf = (0x0000 << 16) | (0x41 << 8) | (0x00 << 3) | 0x0;
		port->port_num = 0;
		std::snprintf(port->type, SMI_NIC_MAX_STRING_LENGTH, "%s", "Ethernet");
		std::snprintf(port->flavour, SMI_NIC_MAX_STRING_LENGTH, "%s", "physical");
		std::snprintf(port->netdev, SMI_NIC_MAX_STRING_LENGTH, "%s", "eth0");
		port->ifindex = 2;
		std::snprintf(port->mac_address, SMI_NIC_MAX_STRING_LENGTH, "%s", "aa:bb:cc:dd:ee:ff");
		port->carrier = 1;
		port->mtu = 1500;
		std::snprintf(port->link_state, SMI_NIC_MAX_STRING_LENGTH, "%s", "up");
		port->link_speed = 10000;
		port->active_fec = 1;
		std::snprintf(port->autoneg, SMI_NIC_MAX_STRING_LENGTH, "%s", "ON");
		std::snprintf(port->pause_autoneg, SMI_NIC_MAX_STRING_LENGTH, "%s", "ON");
		std::snprintf(port->pause_rx, SMI_NIC_MAX_STRING_LENGTH, "%s", "ON");
		std::snprintf(port->pause_tx, SMI_NIC_MAX_STRING_LENGTH, "%s", "ON");
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_get_nic_rdma_dev_info(smi_nic_ctx_t ctx, uint64_t device, smi_nic_rdma_devices_info_t *info)
{
	(void)ctx;
	(void)device;
	if (info) {
		memset(info, 0, sizeof(smi_nic_rdma_devices_info_t));
		info->num_rdma_dev = 1;


		std::snprintf(info->rdma_dev_info[0].rdma_dev, SMI_NIC_MAX_STRING_LENGTH, "%s", "ionic_0");
		std::snprintf(info->rdma_dev_info[0].node_guid, SMI_NIC_MAX_STRING_LENGTH, "%s", "0x1234567890abcdef");
		std::snprintf(info->rdma_dev_info[0].node_type, SMI_NIC_MAX_STRING_LENGTH, "%s", "CA");
		std::snprintf(info->rdma_dev_info[0].sys_image_guid, SMI_NIC_MAX_STRING_LENGTH, "%s", "0xfedcba0987654321");
		std::snprintf(info->rdma_dev_info[0].fw_ver, SMI_NIC_MAX_STRING_LENGTH, "%s", "20.32.1010");
		info->rdma_dev_info[0].num_rdma_ports = 2;

		for (uint8_t i = 0; i < 2; i++) {
			std::snprintf(info->rdma_dev_info[0].rdma_port_info[i].netdev, SMI_NIC_MAX_STRING_LENGTH, "%s", "eth0");
			std::snprintf(info->rdma_dev_info[0].rdma_port_info[i].state, SMI_NIC_MAX_STRING_LENGTH, "%s", "ACTIVE");
			info->rdma_dev_info[0].rdma_port_info[i].rdma_port = i;
			info->rdma_dev_info[0].rdma_port_info[i].max_mtu = 4096;
			info->rdma_dev_info[0].rdma_port_info[i].active_mtu = 4096;
		}
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_get_nic_port_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *count)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (count) {
		*count = 7;
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_get_nic_port_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, smi_nic_stat_info_t *stats)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (stats) {
		stats->count = 7;
		std::snprintf(stats->stats[0].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "port_stat1");
		stats->stats[0].value = 300;
		std::snprintf(stats->stats[1].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "port_stat2");
		stats->stats[1].value = 301;
		std::snprintf(stats->stats[2].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "port_stat3");
		stats->stats[2].value = 302;
		std::snprintf(stats->stats[3].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "port_stat4");
		stats->stats[3].value = 303;
		std::snprintf(stats->stats[4].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "port_stat5");
		stats->stats[4].value = 304;
		std::snprintf(stats->stats[5].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "port_stat6");
		stats->stats[5].value = 305;
		std::snprintf(stats->stats[6].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "port_stat7");
		stats->stats[6].value = 306;
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_get_nic_vendor_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *count)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (count) {
		*count = 7;
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_get_nic_vendor_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, smi_nic_stat_info_t *stats)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (stats) {
		stats->count = 7;
		std::snprintf(stats->stats[0].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "vendor_stat1");
		stats->stats[0].value = 300;
		std::snprintf(stats->stats[1].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "vendor_stat2");
		stats->stats[1].value = 301;
		std::snprintf(stats->stats[2].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "vendor_stat3");
		stats->stats[2].value = 302;
		std::snprintf(stats->stats[3].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "vendor_stat4");
		stats->stats[3].value = 303;
		std::snprintf(stats->stats[4].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "vendor_stat5");
		stats->stats[4].value = 304;
		std::snprintf(stats->stats[5].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "vendor_stat6");
		stats->stats[5].value = 305;
		std::snprintf(stats->stats[6].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "vendor_stat7");
		stats->stats[6].value = 306;
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_get_nic_rdma_port_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t rdma_port_index, uint32_t *count)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)ib_index;
	(void)rdma_port_index;
	if (count) {
		*count = 7;
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_get_nic_rdma_port_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t rdma_port_index, smi_nic_stat_info_t *stats)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)ib_index;
	(void)rdma_port_index;
	if (stats) {
		stats->count = 7;
		std::snprintf(stats->stats[0].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "rdma_stat1");
		stats->stats[0].value = 300;
		std::snprintf(stats->stats[1].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "rdma_stat2");
		stats->stats[1].value = 301;
		std::snprintf(stats->stats[2].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "rdma_stat3");
		stats->stats[2].value = 302;
		std::snprintf(stats->stats[3].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "rdma_stat4");
		stats->stats[3].value = 303;
		std::snprintf(stats->stats[4].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "rdma_stat5");
		stats->stats[4].value = 304;
		std::snprintf(stats->stats[5].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "rdma_stat6");
		stats->stats[5].value = 305;
		std::snprintf(stats->stats[6].name, SMI_NIC_MAX_STRING_LENGTH, "%s", "rdma_stat7");
		stats->stats[6].value = 306;
	}
	return g_nic_api_status_code;
}

smi_nic_status_t smi_topo_get_nic_link_type(smi_nic_ctx_t ctx, uint64_t device_src, uint64_t device_dst, smi_nic_link_type_t *type)
{
	(void)ctx;
	(void)device_src;
	(void)device_dst;
	if (type) {
		*type = SMI_NIC_LINK_TYPE_PCIE; // Return PCIE link type by default
	}
	return g_nic_api_status_code;
}

} // extern "C"

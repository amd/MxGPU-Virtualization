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
static smi_nic_status g_nic_api_status_code = SMI_NIC_STATUS_SUCCESS;

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

smi_nic_status get_nic_api_status()
{
	return g_nic_api_status_code;
}

void set_nic_api_status(smi_nic_status status)
{
	g_nic_api_status_code = status;
}

smi_nic_status smi_nic_create_context(smi_nic_ctx_t *ctx)
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

smi_nic_status smi_nic_destroy_context(smi_nic_ctx_t ctx)
{
	if (ctx) {
		free(ctx);
	}

	if (!g_nic_cleanup_flag) {
		return SMI_NIC_STATUS_ERROR;
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_discover_nics(smi_nic_ctx_t ctx, smi_nic_discovery_t *discovery)
{
	(void)ctx;
	if (!g_nic_discovery_flag) {
		return SMI_NIC_STATUS_ERROR;
	}

	if (!discovery) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	memset(discovery, 0, sizeof(smi_nic_discovery_t));
	discovery->count = 1;

	const char* mock_bdf = "0001:02:03.4";

		std::snprintf(discovery->devices[0].bdf, SMI_NIC_MAX_STRING_LENGTH,
			"%s", mock_bdf);

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_driver_info(smi_nic_ctx_t ctx, uint64_t device, struct ethtool_drvinfo *drvinfo)
{
	(void)ctx;
	(void)device;
	if (drvinfo) {
		strncpy(drvinfo->driver, "driver_mock", sizeof(drvinfo->driver) - 1);
		drvinfo->driver[sizeof(drvinfo->driver) - 1] = '\0';
		strncpy(drvinfo->version, "1.0.0", sizeof(drvinfo->version) - 1);
		drvinfo->version[sizeof(drvinfo->version) - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_link_status(smi_nic_ctx_t ctx, uint64_t device, struct ethtool_stats *stats)
{
	(void)ctx;
	(void)device;
	(void)stats;
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_vendor_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t *vendor_id)
{
	(void)ctx;
	(void)device;
	if (vendor_id) {
		*vendor_id = 0x1002;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_subvendor_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t *subvendor_id)
{
	(void)ctx;
	(void)device;
	if (subvendor_id) {
		*subvendor_id = 0x1234;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_device_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t *device_id)
{
	(void)ctx;
	(void)device;
	if (device_id) {
		*device_id = 0x5678;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_subsystem_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t *subsystem_id)
{
	(void)ctx;
	(void)device;
	if (subsystem_id) {
		*subsystem_id = 0x9ABC;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_revision(smi_nic_ctx_t ctx, uint64_t device, uint8_t *revision)
{
	(void)ctx;
	(void)device;
	if (revision) {
		*revision = 0x01;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_mac_address(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *mac, size_t mac_len)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (mac && mac_len > 17) {
		strncpy(mac, "aa:bb:cc:dd:ee:ff", mac_len - 1);
		mac[mac_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_perm_address(smi_nic_ctx_t ctx, uint64_t device, char *perm_mac, size_t perm_mac_len)
{
	(void)ctx;
	(void)device;
	if (perm_mac && perm_mac_len > 17) {
		strncpy(perm_mac, "aa:bb:cc:dd:ee:ff", perm_mac_len - 1);
		perm_mac[perm_mac_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_max_pcie_width(smi_nic_ctx_t ctx, uint64_t device, uint8_t *max_pcie_width)
{
	(void)ctx;
	(void)device;
	if (max_pcie_width) {
		*max_pcie_width = 16;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_max_pcie_speed(smi_nic_ctx_t ctx, uint64_t device, uint32_t *max_pcie_speed)
{
	(void)ctx;
	(void)device;
	if (max_pcie_speed) {
		*max_pcie_speed = 16;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_pcie_interface_version(smi_nic_ctx_t ctx, uint64_t device, char *pcie_interface_version, size_t pcie_interface_version_len)
{
	(void)ctx;
	(void)device;
	if (pcie_interface_version && pcie_interface_version_len > 5) {
		strncpy(pcie_interface_version, "Gen 4", pcie_interface_version_len - 1);
		pcie_interface_version[pcie_interface_version_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_slot_type(smi_nic_ctx_t ctx, uint64_t device, char *slot_type, size_t slot_type_len)
{
	(void)ctx;
	(void)device;
	if (slot_type && slot_type_len > 3) {
		strncpy(slot_type, "OAM", slot_type_len - 1);
		slot_type[slot_type_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_numa_node(smi_nic_ctx_t ctx, uint64_t device, uint8_t *numa_node)
{
	(void)ctx;
	(void)device;
	if (numa_node) {
		*numa_node = 0;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_numa_affinity(smi_nic_ctx_t ctx, uint64_t device, uint8_t node, char *affinity, size_t affinity_len)
{
	(void)ctx;
	(void)device;
	(void)node;
	if (affinity && affinity_len > 7) {
		strncpy(affinity, "0,1,2,3", affinity_len - 1);
		affinity[affinity_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_port_num_by_index(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *port_num)
{
	(void)ctx;
	(void)device;
	if (port_num) {
		*port_num = port_index;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_ports_num(smi_nic_ctx_t ctx, uint64_t device, uint32_t *num_ports)
{
	(void)ctx;
	(void)device;
	if (num_ports) {
		*num_ports = 1;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_port_interface(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *interface_name, size_t interface_name_len)
{
	(void)ctx;
	(void)device;
	if (interface_name && interface_name_len > 0) {
		std::snprintf(interface_name, interface_name_len, "eth%u", port_index);
		interface_name[interface_name_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_port_bdf(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint64_t *port_bdf)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (port_bdf) {
		*port_bdf = (0x0000 << 16) | (0x41 << 8) | (0x00 << 3) | 0x0;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_flavour(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *flavour, size_t flavour_len)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (flavour && flavour_len > 8) {
		strncpy(flavour, "physical", flavour_len - 1);
		flavour[flavour_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_port_type(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *port_type, size_t port_type_len)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (port_type && port_type_len > 8) {
		strncpy(port_type, "Ethernet", port_type_len - 1);
		port_type[port_type_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_ifindex(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint8_t *ifindex)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (ifindex) {
		*ifindex = 2;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_carrier(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint8_t *carrier)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (carrier) {
		*carrier = 1;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_mtu(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint16_t *mtu)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (mtu) {
		*mtu = 1500;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_link_state(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *link_state, size_t link_state_len)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (link_state && link_state_len > 3) {
		strncpy(link_state, "yes", link_state_len - 1);
		link_state[link_state_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_link_speed(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *link_speed)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (link_speed) {
		*link_speed = 10000;
	}
	return g_nic_api_status_code;
}


smi_nic_status smi_get_nic_pause_info(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, struct ethtool_pauseparam *pause_info)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (pause_info) {
		pause_info->autoneg = 1;
		pause_info->rx_pause = 1;
		pause_info->tx_pause = 1;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_fecparam_info(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, struct ethtool_fecparam *fecparam_info)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (fecparam_info) {
		fecparam_info->active_fec = 1;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_link_settings_info(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, struct ethtool_link_settings *link_settings)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (link_settings) {
		link_settings->autoneg = 1;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_infiniband_num(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint8_t *num_infiniband)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (num_infiniband) {
		*num_infiniband = 1;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_infiniband_rdma_dev(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t index, char *rdma_dev, size_t rdma_dev_len)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)index;
	if (rdma_dev && rdma_dev_len > 6) {
		strncpy(rdma_dev, "mlx5_0", rdma_dev_len - 1);
		rdma_dev[rdma_dev_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_infiniband_node_guid(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t index, char *guid, size_t guid_len)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)index;
	if (guid && guid_len > 18) {
		strncpy(guid, "0x1234567890abcdef", guid_len - 1);
		guid[guid_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_infiniband_node_type(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t index, char *type, size_t type_len)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)index;
	if (type && type_len > 2) {
		strncpy(type, "CA", type_len - 1);
		type[type_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_infiniband_sys_image_guid(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t index, char *guid, size_t guid_len)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)index;
	if (guid && guid_len > 18) {
		strncpy(guid, "0xfedcba0987654321", guid_len - 1);
		guid[guid_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_infiniband_fw_ver(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t index, char *fw, size_t fw_len)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)index;
	if (fw && fw_len > 10) {
		strncpy(fw, "20.32.1010", fw_len - 1);
		fw[fw_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_infiniband_num_ports(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t index, uint8_t *num_ports)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)index;
	if (num_ports) {
		*num_ports = 2;
	}
	return g_nic_api_status_code;
}


smi_nic_status smi_get_infiniband_port_netdev(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, char *netdev, size_t netdev_len)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)ib_index;
	(void)ib_port_index;
	(void)netdev;
	(void)netdev_len;
	return g_nic_api_status_code;
}

smi_nic_status smi_get_infiniband_port_num(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, uint8_t *port_num)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)ib_index;
	if (port_num) {
		*port_num = static_cast<uint8_t>(ib_port_index);
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_infiniband_port_state(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, char *state, size_t state_len)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)ib_index;
	(void)ib_port_index;
	if (state && state_len > 6) {
		strncpy(state, "ACTIVE", state_len - 1);
		state[state_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_infiniband_port_max_mtu(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, uint16_t *max_mtu)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)ib_index;
	(void)ib_port_index;
	if (max_mtu) {
		*max_mtu = 4096;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_infiniband_port_active_mtu(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, uint16_t *active_mtu)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)ib_index;
	(void)ib_port_index;
	if (active_mtu) {
		*active_mtu = 4096;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_port_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *count)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (count) {
		*count = 7;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_port_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, smi_nic_stat_list *stats)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (stats) {
		stats->count = 7;
		strncpy(stats->stats[0].name, "port_stat1", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[0].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[0].value = 300;
		strncpy(stats->stats[1].name, "port_stat2", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[1].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[1].value = 301;
		strncpy(stats->stats[2].name, "port_stat3", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[2].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[2].value = 302;
		strncpy(stats->stats[3].name, "port_stat4", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[3].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[3].value = 303;
		strncpy(stats->stats[4].name, "port_stat5", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[4].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[4].value = 304;
		strncpy(stats->stats[5].name, "port_stat6", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[5].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[5].value = 305;
		strncpy(stats->stats[6].name, "port_stat7", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[6].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[6].value = 306;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_vendor_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *count)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (count) {
		*count = 7;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_vendor_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, smi_nic_stat_list *stats)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	if (stats) {
		stats->count = 7;
		strncpy(stats->stats[0].name, "vendor_stat1", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[0].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[0].value = 300;
		strncpy(stats->stats[1].name, "vendor_stat2", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[1].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[1].value = 301;
		strncpy(stats->stats[2].name, "vendor_stat3", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[2].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[2].value = 302;
		strncpy(stats->stats[3].name, "vendor_stat4", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[3].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[3].value = 303;
		strncpy(stats->stats[4].name, "vendor_stat5", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[4].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[4].value = 304;
		strncpy(stats->stats[5].name, "vendor_stat6", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[5].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[5].value = 305;
		strncpy(stats->stats[6].name, "vendor_stat7", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[6].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[6].value = 306;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_rdma_port_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t rdma_port_index, uint32_t *count)
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

smi_nic_status smi_get_nic_rdma_port_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t rdma_port_index, smi_nic_stat_list *stats)
{
	(void)ctx;
	(void)device;
	(void)port_index;
	(void)ib_index;
	(void)rdma_port_index;
	if (stats) {
		stats->count = 7;
		strncpy(stats->stats[0].name, "rdma_stat1", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[0].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[0].value = 300;
		strncpy(stats->stats[1].name, "rdma_stat2", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[1].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[1].value = 301;
		strncpy(stats->stats[2].name, "rdma_stat3", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[2].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[2].value = 302;
		strncpy(stats->stats[3].name, "rdma_stat4", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[3].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[3].value = 303;
		strncpy(stats->stats[4].name, "rdma_stat5", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[4].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[4].value = 304;
		strncpy(stats->stats[5].name, "rdma_stat6", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[5].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[5].value = 305;
		strncpy(stats->stats[6].name, "rdma_stat7", SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[6].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[6].value = 306;
	}
	return g_nic_api_status_code;
}


smi_nic_status smi_get_nic_topo_get_nic_link_type(smi_nic_ctx_t ctx, uint64_t device, uint8_t nic_numa_info, uint8_t processor_numa_info, int *type)
{
	(void)ctx;
	(void)device;
	(void)nic_numa_info;
	(void)processor_numa_info;
	if (type) {
		*type = 1;
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_product_name(smi_nic_ctx_t ctx, uint64_t device, char *product_name, size_t product_len)
{
	(void)ctx;
	(void)device;
	if (product_name && product_len > 7) {
		strncpy(product_name, "AMD NIC", product_len - 1);
		product_name[product_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_vendor_name(smi_nic_ctx_t ctx, uint64_t device, char *vendor_name, size_t vendor_len)
{
	(void)ctx;
	(void)device;
	if (vendor_name && vendor_len > 3) {
		strncpy(vendor_name, "AMD", vendor_len - 1);
		vendor_name[vendor_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_part_number(smi_nic_ctx_t ctx, uint64_t device, char *part_number, size_t part_len)
{
	(void)ctx;
	(void)device;
	if (part_number && part_len > 9) {
		strncpy(part_number, "AMD-NIC-1234", part_len - 1);
		part_number[part_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

smi_nic_status smi_get_nic_serial_number(smi_nic_ctx_t ctx, uint64_t device, char *serial_number, size_t serial_len)
{
	(void)ctx;
	(void)device;
	if (serial_number && serial_len > 12) {
		strncpy(serial_number, "SN0123456789", serial_len - 1);
		serial_number[serial_len - 1] = '\0';
	}
	return g_nic_api_status_code;
}

} // extern "C"

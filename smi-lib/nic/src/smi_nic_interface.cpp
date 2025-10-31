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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstring>
#include <cstdlib>
#include <vector>
#include <variant>
#include <string>
#include <iostream>
#include <memory>
#include <mutex>
#include <atomic>

#include "smi_sysfs.h"
#include "smi_ethtool_ioctl.h"
#include "smi_nic_interface.h"
#include "smi_nic_system.h"

struct smi_nic_ctx {
	std::unique_ptr<SmiNicSystem> nic_system;
	std::mutex ctx_mutex;
	std::atomic<bool> init;

	smi_nic_ctx() : init(false) {}
};

static SmiNicSystem* get_nic_system_from_context(smi_nic_ctx *ctx)
{
	if (!ctx || !ctx->init || !ctx->nic_system) {
		return nullptr;
	}
	return ctx->nic_system.get();
}

extern "C" {
smi_nic_status smi_nic_create_context(smi_nic_ctx_t *ctx)
{
	try {
		if (!ctx) {
			return SMI_NIC_STATUS_WRONG_PARAM;
		}

		auto context = std::make_unique<smi_nic_ctx>();
		context->nic_system = std::make_unique<SmiNicSystem>();
		context->init = true;
		*ctx = context.release();

		(*ctx)->nic_system->discover_nics();

		return SMI_NIC_STATUS_SUCCESS;

	} catch (const std::bad_alloc&) {
		return SMI_NIC_STATUS_NO_RESOURCE;
	} catch (...) {
		return SMI_NIC_STATUS_ERROR;
	}
}

smi_nic_status smi_nic_destroy_context(smi_nic_ctx_t ctx)
{
	try {
		if (!ctx) {
			return SMI_NIC_STATUS_WRONG_PARAM;
		}

		{
			std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
			ctx->init = false;
			ctx->nic_system.reset();
		}

		delete ctx;
		return SMI_NIC_STATUS_SUCCESS;
	} catch (...) {
		return SMI_NIC_STATUS_ERROR;
	}
}

smi_nic_status smi_discover_nics(smi_nic_ctx_t ctx, smi_nic_discovery_t *discovery)
{
	if (!ctx || !discovery) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	discovery->count = 0;
	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);

	try {
		const auto& nics = nic_system->get_nics();
		if (nics.empty()) {
			return SMI_NIC_STATUS_NO_DATA;
		}

		if (nics.size() > SMI_NIC_MAX_DEVICES) {
			return SMI_NIC_STATUS_NO_RESOURCE;
		}

		uint32_t index = 0;
		for (const auto& nic : nics) {
			std::snprintf(discovery->devices[index].bdf, SMI_NIC_MAX_STRING_LENGTH,
						"%s", nic.bdf().c_str());
			index++;
		}

		discovery->count = static_cast<uint32_t>(nics.size());
		return SMI_NIC_STATUS_SUCCESS;

	} catch (const std::exception&) {
		discovery->count = 0;
		return SMI_NIC_STATUS_ERROR;
	}
}




smi_nic_status smi_get_nic_driver_info(smi_nic_ctx_t ctx, uint64_t device, struct ethtool_drvinfo *drvinfo)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (drvinfo == nullptr) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	memset(drvinfo, 0, sizeof(struct ethtool_drvinfo));
	drvinfo->cmd = ETHTOOL_GDRVINFO;

	return smi_ethtool_ioctl(ports[0].interface(), drvinfo) == 0 ? SMI_NIC_STATUS_SUCCESS : SMI_NIC_STATUS_ERROR;
}

smi_nic_status smi_get_nic_link_status(smi_nic_ctx_t ctx, uint64_t device, struct ethtool_stats *stats)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (stats == nullptr) {
	    return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	memset(stats, 0, sizeof(struct ethtool_stats));
	stats->cmd = ETHTOOL_GSSET_INFO;

	return smi_ethtool_ioctl(ports[0].interface(), stats) == 0 ? SMI_NIC_STATUS_SUCCESS : SMI_NIC_STATUS_ERROR;
}

smi_nic_status smi_get_nic_link_settings_info(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, struct ethtool_link_settings *link_settings)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (link_settings == nullptr) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	memset(link_settings, 0, sizeof(struct ethtool_link_settings));
	link_settings->cmd = ETHTOOL_GLINKSETTINGS;

	return smi_ethtool_ioctl(ports[port_index].interface(), link_settings) == 0 ? SMI_NIC_STATUS_SUCCESS : SMI_NIC_STATUS_ERROR;
}

smi_nic_status smi_get_nic_pause_info(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, struct ethtool_pauseparam *pause_info)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (pause_info == nullptr) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	memset(pause_info, 0, sizeof(struct ethtool_pauseparam));
	pause_info->cmd = ETHTOOL_GPAUSEPARAM;

	return smi_ethtool_ioctl(ports[port_index].interface(), pause_info) == 0 ? SMI_NIC_STATUS_SUCCESS : SMI_NIC_STATUS_ERROR;
}

smi_nic_status smi_get_nic_fecparam_info(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, struct ethtool_fecparam *fecparam_info)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (fecparam_info == nullptr) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	memset(fecparam_info, 0, sizeof(struct ethtool_fecparam));
	fecparam_info->cmd = ETHTOOL_GFECPARAM;

	return smi_ethtool_ioctl(ports[port_index].interface(), fecparam_info) == 0 ? SMI_NIC_STATUS_SUCCESS : SMI_NIC_STATUS_ERROR;
}

smi_nic_status smi_get_nic_vendor_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t *vendor_id)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!vendor_id) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->vendor_id();
	if (!val.has_value()) {
		*vendor_id = (uint16_t)-1;
	} else {
		*vendor_id = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_max_pcie_width(smi_nic_ctx_t ctx, uint64_t device, uint8_t *max_pcie_width)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!max_pcie_width) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);

	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->max_pcie_width();
	if (!val.has_value()) {
		*max_pcie_width = (uint8_t)-1;
	} else {
		*max_pcie_width = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_max_pcie_speed(smi_nic_ctx_t ctx, uint64_t device, uint32_t *max_pcie_speed)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!max_pcie_speed) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);

	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->max_pcie_speed();
	if (!val.has_value()) {
		*max_pcie_speed = (uint32_t)-1;
	} else {
		*max_pcie_speed = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_pcie_interface_version(smi_nic_ctx_t ctx, uint64_t device, char *pcie_interface_version, size_t pcie_interface_version_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!pcie_interface_version || pcie_interface_version_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	strncpy(pcie_interface_version, "N/A", pcie_interface_version_len - 1);

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_slot_type(smi_nic_ctx_t ctx, uint64_t device, char *slot_type, size_t slot_type_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!slot_type || slot_type_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	strncpy(slot_type, "N/A", slot_type_len - 1);

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_numa_node(smi_nic_ctx_t ctx, uint64_t device, uint8_t *numa_node)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!numa_node) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);

	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->numa_node();
	if (!val.has_value()) {
		*numa_node = (uint8_t)-1;
	} else {
		*numa_node = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_numa_affinity(smi_nic_ctx_t ctx, uint64_t device, uint8_t node, char *affinity, size_t affinity_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!affinity || affinity_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->numa_affinity(node);
	if (!val.has_value()) {
		strncpy(affinity, "N/A", affinity_len - 1);
		affinity[affinity_len - 1] = '\0';
	} else {
		strncpy(affinity, val.value().c_str(), affinity_len - 1);
		affinity[affinity_len - 1] = '\0';
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_flavour(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *flavour, size_t flavour_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!flavour || flavour_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	std::string flavour_str = ports[port_index].flavour();
	if (flavour_str.length() >= flavour_len) {
		return SMI_NIC_STATUS_NO_RESOURCE;
	}

	strncpy(flavour, flavour_str.c_str(), flavour_len - 1);
	flavour[flavour_len - 1] = '\0';

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_port_num_by_index(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *port_num)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!port_num) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	auto val = ports[port_index].port_num();
	if (!val.has_value()) {
		*port_num = (uint32_t)-1;
	} else {
		*port_num = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_port_type(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *port_type, size_t port_type_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!port_type || port_type_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = ports[port_index].port_type();
	strncpy(port_type, val.c_str(), port_type_len - 1);
	port_type[port_type_len - 1] = '\0';

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_ifindex(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint8_t *ifindex)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!ifindex) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = ports[port_index].ifindex();
	if (!val.has_value()) {
		*ifindex = (uint8_t)-1;
	} else {
		*ifindex = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_carrier(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint8_t *carrier)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!carrier) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = ports[port_index].carrier();
	if (!val.has_value()) {
		*carrier = (uint8_t)-1;
	} else {
		*carrier = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_mtu(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint16_t *mtu)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!mtu) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = ports[port_index].mtu();
	if (!val.has_value()) {
		*mtu = (uint16_t)-1;
	} else {
		*mtu = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_link_state(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *link_state, size_t link_state_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!link_state || link_state_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = ports[port_index].link_state();
	if (!val.has_value()) {
		strncpy(link_state, "N/A", link_state_len - 1);
		link_state[link_state_len - 1] = '\0';
	} else {
		strncpy(link_state, val.value().c_str(), link_state_len - 1);
		link_state[link_state_len - 1] = '\0';
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_link_speed(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *link_speed)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!link_speed) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = ports[port_index].link_speed();
	if (!val.has_value()) {
		*link_speed = (uint32_t)-1;
	} else {
		*link_speed = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_subvendor_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t* subvendor_id)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!subvendor_id) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->subvendor_id();
	if (!val.has_value()) {
		*subvendor_id = (uint16_t)-1;
	} else {
		*subvendor_id = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_device_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t* device_id)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!device_id) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->device_id();
	if (!val.has_value()) {
		*device_id = (uint16_t)-1;
	} else {
		*device_id = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_subsystem_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t* subsystem_id)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!subsystem_id) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->subsystem_id();
	if (!val.has_value()) {
		*subsystem_id = (uint16_t)-1;
	} else {
		*subsystem_id = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_revision(smi_nic_ctx_t ctx, uint64_t device, uint8_t* revision)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!revision) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->revision();
	if (!val.has_value()) {
		*revision = (uint8_t)-1;
	} else {
		*revision = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_mac_address(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char* mac, size_t mac_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!mac || mac_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto mac_str = ports[port_index].mac_address();
	if (!mac_str.has_value()) {
		strncpy(mac, "N/A", mac_len - 1);
		mac[mac_len - 1] = '\0';
		return SMI_NIC_STATUS_SUCCESS;
	}

	if (mac_str.value().length() >= mac_len) {
		return SMI_NIC_STATUS_NO_RESOURCE;
	}

	strncpy(mac, mac_str.value().c_str(), mac_len - 1);
	mac[mac_len - 1] = '\0';

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_perm_address(smi_nic_ctx_t ctx, uint64_t device, char* perm_mac, size_t perm_mac_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!perm_mac || perm_mac_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto perm_addr = nic->perm_address();
	if (!perm_addr.has_value()) {
		strncpy(perm_mac, "N/A", perm_mac_len - 1);
		perm_mac[perm_mac_len - 1] = '\0';
		return SMI_NIC_STATUS_SUCCESS;
	}

	if (perm_addr.value().length() >= perm_mac_len) {
		return SMI_NIC_STATUS_NO_RESOURCE;
	}

	strncpy(perm_mac, perm_addr.value().c_str(), perm_mac_len - 1);
	perm_mac[perm_mac_len - 1] = '\0';

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_infiniband_num(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint8_t* num_infiniband)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!num_infiniband) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	*num_infiniband = ports[port_index].infiniband_num();
	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_infiniband_rdma_dev(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, char* rdma_dev, size_t rdma_dev_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!rdma_dev || rdma_dev_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const std::string& rdma_dev_str = ibs[ib_index].rdma_dev();
	strncpy(rdma_dev, rdma_dev_str.c_str(), rdma_dev_len - 1);
	rdma_dev[rdma_dev_len - 1] = '\0';

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_infiniband_node_guid(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, char* guid, size_t guid_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!guid || guid_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	auto val = ibs[ib_index].node_guid();
	std::string guid_str = "N/A";
	if (val.has_value()) {
		guid_str = val.value();
	}
	strncpy(guid, guid_str.c_str(), guid_len - 1);
	guid[guid_len - 1] = '\0';

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_infiniband_node_type(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, char* type, size_t type_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!type || type_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	auto val = ibs[ib_index].node_type();
	if (!val.has_value()) {
		strncpy(type, "N/A", type_len - 1);
		type[type_len - 1] = '\0';
	} else {
		strncpy(type, val.value().c_str(), type_len - 1);
		type[type_len - 1] = '\0';
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_infiniband_sys_image_guid(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, char* guid, size_t guid_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!guid || guid_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	auto val = ibs[ib_index].sys_image_guid();
	if (!val.has_value()) {
		strncpy(guid, "N/A", guid_len - 1);
		guid[guid_len - 1] = '\0';
	} else {
		strncpy(guid, val.value().c_str(), guid_len - 1);
		guid[guid_len - 1] = '\0';
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_infiniband_fw_ver(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, char* fw, size_t fw_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!fw || fw_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	auto val = ibs[ib_index].fw_ver();
	if (!val.has_value()) {
		strncpy(fw, "N/A", fw_len - 1);
		fw[fw_len - 1] = '\0';
	} else {
		strncpy(fw, val.value().c_str(), fw_len - 1);
		fw[fw_len - 1] = '\0';
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_ports_num(smi_nic_ctx_t ctx, uint64_t device, uint32_t *num_ports)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!num_ports) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	*num_ports = static_cast<uint32_t>(nic->nic_ports_num());

	if (*num_ports == 0) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_port_interface(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *interface_name, size_t interface_name_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!interface_name || interface_name_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const std::string& port_iface = ports[port_index].interface();
	std::string interface = port_iface.empty() ? "N/A" : port_iface;
	strncpy(interface_name, interface.c_str(), interface_name_len - 1);
	interface_name[interface_name_len - 1] = '\0';

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_port_bdf(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint64_t *port_bdf)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!port_bdf) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const std::string& bdf_str = ports[port_index].bdf();
	uint64_t domain, bus, device_num, function;
	if (sscanf(bdf_str.c_str(), "%04lx:%02lx:%02lx.%1lx", &domain, &bus, &device_num, &function) == 4) {
		*port_bdf = (domain << 16) | (bus << 8) | (device_num << 3) | function;
	} else {
		return SMI_NIC_STATUS_ERROR;
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_infiniband_num_ports(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint8_t* num_ports)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!num_ports) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	*num_ports = ibs[ib_index].ports_num();

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_infiniband_port_netdev(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, char* netdev, size_t netdev_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!netdev || netdev_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	const auto& ib_ports = ibs[ib_index].ports();
	if (ib_port_index >= ib_ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	const std::string& netdev_str = ib_ports[ib_port_index].netdev();
	strncpy(netdev, netdev_str.c_str(), netdev_len - 1);
	netdev[netdev_len - 1] = '\0';

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_infiniband_port_num(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, uint8_t* port_num)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!port_num) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	const auto& ib_ports = ibs[ib_index].ports();
	if (ib_port_index >= ib_ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	auto port_number = ib_ports[ib_port_index].port_num();
	if (port_number.has_value()) {
		*port_num = port_number.value();
		return SMI_NIC_STATUS_SUCCESS;
	} else {
		*port_num = static_cast<uint8_t>(-1);
		return SMI_NIC_STATUS_SUCCESS;
	}
}

smi_nic_status smi_get_infiniband_port_state(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, char* state, size_t state_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!state || state_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	const auto& ib_ports = ibs[ib_index].ports();
	if (ib_port_index >= ib_ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	auto val = ib_ports[ib_port_index].state();
	std::string state_str = "N/A";
	if (val.has_value()) {
		const std::string& raw_state = val.value();
		auto pos = raw_state.find(": ");
		if (pos != std::string::npos) {
			state_str = raw_state.substr(pos + 2);
		}
	}

	strncpy(state, state_str.c_str(), state_len - 1);
	state[state_len - 1] = '\0';

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_infiniband_port_max_mtu(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, uint16_t* max_mtu)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!max_mtu) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	const auto& ib_ports = ibs[ib_index].ports();
	if (ib_port_index >= ib_ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	auto val = ib_ports[ib_port_index].max_mtu();
	if (!val.has_value()) {
		*max_mtu = (uint16_t)-1;
	} else {
		*max_mtu = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_infiniband_port_active_mtu(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, uint16_t* active_mtu)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!active_mtu) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	const auto& ib_ports = ibs[ib_index].ports();
	if (ib_port_index >= ib_ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}
	auto val = ib_ports[ib_port_index].active_mtu();
	if (!val.has_value()) {
		*active_mtu = (uint16_t)-1;
	} else {
		*active_mtu = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_topo_get_nic_link_type(smi_nic_ctx_t ctx, uint64_t device, uint8_t nic_numa_info, uint8_t processor_numa_info, int *type)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!type) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->topo_get_nic_link_type(nic_numa_info, processor_numa_info);
	if (!val.has_value()) {
		*type = -1;
	} else {
		*type = val.value();
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_port_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *count)
{
	if (!ctx || !count) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& port = ports[port_index];
	const auto& stats_map = port.get_standard_stats_map();
	*count = static_cast<uint32_t>(stats_map.size());

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_port_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, smi_nic_stat_list *stats)
{
	if (!ctx || !stats) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& port = ports[port_index];
	const auto& stats_map = port.get_standard_stats_map();

	if (stats_map.empty()) {
		return SMI_NIC_STATUS_NO_DATA;
	}

	stats->count = static_cast<uint32_t>(std::min(stats_map.size(), (size_t)SMI_NIC_MAX_STATISTICS));
	uint32_t i = 0;
	for (const auto& stat_pair : stats_map) {
		if (i >= stats->count) {
			break;
		}
		strncpy(stats->stats[i].name, stat_pair.first.c_str(), SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[i].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[i].value = stat_pair.second;
		i++;
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_vendor_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *count)
{
	if (!ctx || !count) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}


	const auto& port = ports[port_index];
	const auto& stats_map = port.get_vendor_stats_map();
	*count = static_cast<uint32_t>(stats_map.size());

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_vendor_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, smi_nic_stat_list *stats)
{
	if (!ctx || !stats) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& port = ports[port_index];
	const auto& stats_map = port.get_vendor_stats_map();

	if (stats_map.empty()) {
		return SMI_NIC_STATUS_NO_DATA;
	}

	stats->count = static_cast<uint32_t>(std::min(stats_map.size(), (size_t)SMI_NIC_MAX_STATISTICS));
	uint32_t i = 0;
	for (const auto& stat_pair : stats_map) {
		if (i >= stats->count) {
			break;
		}
		strncpy(stats->stats[i].name, stat_pair.first.c_str(), SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[i].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[i].value = stat_pair.second;
		i++;
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_rdma_port_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t rdma_port_index, uint32_t *count)
{
	if (!ctx || !count) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= (uint32_t)ibs.size()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ib_ports = ibs[ib_index].ports();
	if (rdma_port_index >= (uint32_t)ib_ports.size()) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	const auto& ib_port = ib_ports[rdma_port_index];
	const auto& stats_map = ib_port.get_hw_counters_map();
	*count = static_cast<uint32_t>(stats_map.size());

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_rdma_port_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t rdma_port_index, smi_nic_stat_list *stats)
{
	if (!ctx || !stats) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (port_index >= ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	if (!nic_system->driver_loaded(ports[port_index].bdf(), DriverType::IONIC_RDMA)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	const auto& ibs = ports[port_index].infiniband();
	if (ib_index >= (uint32_t)ibs.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ib_ports = ibs[ib_index].ports();
	if (rdma_port_index >= (uint32_t)ib_ports.size()) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ib_port = ib_ports[rdma_port_index];
	const auto& stats_map = ib_port.get_hw_counters_map();

	if (stats_map.empty()) {
		return SMI_NIC_STATUS_NO_DATA;
	}

	stats->count = static_cast<uint32_t>(std::min(stats_map.size(), (size_t)SMI_NIC_MAX_STATISTICS));
	uint32_t i = 0;
	for (const auto& stat_pair : stats_map) {
		if (i >= stats->count) {
			break;
		}
		strncpy(stats->stats[i].name, stat_pair.first.c_str(), SMI_NIC_MAX_STRING_LENGTH - 1);
		stats->stats[i].name[SMI_NIC_MAX_STRING_LENGTH - 1] = '\0';
		stats->stats[i].value = stat_pair.second;
		i++;
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_product_name(smi_nic_ctx_t ctx, uint64_t device, char *product_name, size_t product_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!product_name || product_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->product_name();
	if (!val.has_value()) {
		strncpy(product_name, "N/A", product_len - 1);
		product_name[product_len - 1] = '\0';
	} else {
		strncpy(product_name, val.value().c_str(), product_len - 1);
		product_name[product_len - 1] = '\0';
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_vendor_name(smi_nic_ctx_t ctx, uint64_t device, char *vendor_name, size_t vendor_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!vendor_name || vendor_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->vendor_name();
	if (!val.has_value()) {
		strncpy(vendor_name, "N/A", vendor_len - 1);
		vendor_name[vendor_len - 1] = '\0';
	} else {
		strncpy(vendor_name, val.value().c_str(), vendor_len - 1);
		vendor_name[vendor_len - 1] = '\0';
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_part_number(smi_nic_ctx_t ctx, uint64_t device, char* part_number, size_t part_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!part_number || part_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->part_number();
	if (!val.has_value()) {
		strncpy(part_number, "N/A", part_len - 1);
		part_number[part_len - 1] = '\0';
	} else {
		strncpy(part_number, val.value().c_str(), part_len - 1);
		part_number[part_len - 1] = '\0';
	}

	return SMI_NIC_STATUS_SUCCESS;
}

smi_nic_status smi_get_nic_serial_number(smi_nic_ctx_t ctx, uint64_t device, char* serial_number, size_t serial_len)
{
	if (!ctx) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	if (!serial_number || serial_len == 0) {
		return SMI_NIC_STATUS_WRONG_PARAM;
	}

	auto* nic_system = get_nic_system_from_context(ctx);
	if (!nic_system) {
		return SMI_NIC_STATUS_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(ctx->ctx_mutex);
	const SmiNic *nic = nic_system->get_nic_by_bdf(device);
	if (!nic) {
		return SMI_NIC_STATUS_NOT_FOUND;
	}

	const auto& ports = nic->nic_ports();
	if (ports.empty()) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	if (!nic_system->driver_loaded(ports[0].bdf(), DriverType::IONIC)) {
		return SMI_NIC_STATUS_DRIVER_NOT_LOADED;
	}

	auto val = nic->serial_number();
	if (!val.has_value()) {
		strncpy(serial_number, "N/A", serial_len - 1);
		serial_number[serial_len - 1] = '\0';
	} else {
		strncpy(serial_number, val.value().c_str(), serial_len - 1);
		serial_number[serial_len - 1] = '\0';
	}

	return SMI_NIC_STATUS_SUCCESS;
}

} // extern "C"

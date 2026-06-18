/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_utils.h"

#include <regex>
#include <filesystem>
#include <sstream>
#include <iomanip>

#ifdef LIBMNL_INSTALLED
#include <linux/devlink.h>
#endif

static const std::string SYSFS_PCI_BUS_PATH = "/sys/bus/pci/devices/";

namespace smi_utils {

bool is_valid_bdf(const std::string& bdf)
{
	static const std::regex bdf_pattern("^[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\\.[0-9a-fA-F]$");
	return std::regex_match(bdf, bdf_pattern);
}

uint64_t parse_bdf(const std::string& bdf)
{
	if (!is_valid_bdf(bdf)) return 0;

	try {
		uint64_t domain = std::stoul(bdf.substr(0, 4), nullptr, 16);
		uint64_t bus = std::stoul(bdf.substr(5, 2), nullptr, 16);
		uint64_t device = std::stoul(bdf.substr(8, 2), nullptr, 16);
		uint64_t function = std::stoul(bdf.substr(11, 1), nullptr, 16);

		return (domain << 16) | (bus << 8) | (device << 3) | function;
	} catch (const std::exception&) {
		return 0;
	}
}

std::string format_bdf(uint64_t bdf)
{
	uint64_t function = bdf & 0x7;
	uint64_t device = (bdf >> 3) & 0x1F;
	uint64_t bus = (bdf >> 8) & 0xFF;
	uint64_t domain = (bdf >> 16) & 0xFFFF;

	std::ostringstream oss;
	oss << std::hex << std::setfill('0')
	    << std::setw(4) << domain << ":"
	    << std::setw(2) << bus << ":"
	    << std::setw(2) << device << "."
	    << std::setw(1) << function;
	return oss.str();
}

std::optional<int> get_numa_node_from_bdf(const std::string& bdf)
{
	if (!is_valid_bdf(bdf)) {
		return std::nullopt;
	}

	return get_sysfs_data<int>(SYSFS_PCI_BUS_PATH + bdf + "/numa_node");
}

std::optional<std::string> get_pcie_parent_bdf(const std::string& bdf)
{
	if (!is_valid_bdf(bdf)) {
		return std::nullopt;
	}

	std::string sysfsPath = SYSFS_PCI_BUS_PATH + bdf;
	std::error_code ec;

	// Read the symlink target - points to actual device path in /sys/devices
	// e.g., ../../../devices/pci0000:00/0000:00:01.0/0000:01:00.0
	auto linkTarget = std::filesystem::read_symlink(sysfsPath, ec);
	if (ec) return std::nullopt;

	// Parent BDF is the second-to-last path component
	std::string parentName = linkTarget.parent_path().filename().string();

	if (is_valid_bdf(parentName)) {
		return parentName;
	}

	return std::nullopt;
}

std::optional<uint8_t> get_pcie_link_gen(const std::string& bdf)
{
	static constexpr uint8_t PCI_CAP_LIST_PTR = 0x34;
	static constexpr uint8_t PCI_CAP_ID_PCIE = 0x10;
	static constexpr uint8_t PCIE_LNKSTA = 0x12;
	static constexpr uint8_t LINK_SPEED_MASK = 0x0F;
	static constexpr uint8_t CAP_PTR_MASK = 0xFC;
	static constexpr int MAX_CAP_WALK = 48;

	if (!is_valid_bdf(bdf)) {
		return std::nullopt;
	}

	const std::string config_path = SYSFS_PCI_BUS_PATH + bdf + "/config";

	uint8_t offset = 0;
	if (SmiSysfsReader::readBytes(config_path, PCI_CAP_LIST_PTR, &offset, sizeof(offset)) !=
	    SmiSysfsReader::SysfsStatus::Success) {
		return std::nullopt;
	}
	offset &= CAP_PTR_MASK;

	for (int i = 0; offset >= 0x40 && i < MAX_CAP_WALK; i++) {
		uint8_t cap_header[2] = { 0, 0 };
		if (SmiSysfsReader::readBytes(config_path, offset, cap_header, sizeof(cap_header)) !=
		    SmiSysfsReader::SysfsStatus::Success) {
			return std::nullopt;
		}

		if (cap_header[0] == PCI_CAP_ID_PCIE) {
			if ((static_cast<uint16_t>(offset) + PCIE_LNKSTA) > 0xFF) {
				return std::nullopt;
			}
			const auto link_off = static_cast<uint16_t>(offset) + PCIE_LNKSTA;
			uint8_t link_val = 0;
			if (SmiSysfsReader::readBytes(config_path, link_off, &link_val, sizeof(link_val)) !=
			    SmiSysfsReader::SysfsStatus::Success) {
				return std::nullopt;
			}

			// Current Link Speed is encoded
			// as an index into the Supported Link Speeds Vector. For Gen 1-6 that
			// index equals the generation number (1=2.5, 2=5.0, ... 6=64.0 GT/s),
			// so we return it directly. If a future generation breaks this 1:1
			// mapping, this must be replaced with an explicit lookup. 0 = reserved.
			const uint8_t gen = link_val & LINK_SPEED_MASK;
			if (gen == 0) {
				return std::nullopt;
			}
			return gen;
		}

		offset = static_cast<uint8_t>(cap_header[1] & CAP_PTR_MASK);
	}

	return std::nullopt;
}

std::string nic_type_to_string(NicType type)
{
	switch (type) {
	case NicType::PCIBridge:
		return "PCI Bridge";
	case NicType::Ethernet:
		return "Ethernet";
	case NicType::InfiniBand:
		return "InfiniBand";
	default:
		return "";
	}
}

#ifdef LIBMNL_INSTALLED
const char *flavour_to_string(uint16_t flavour)
{
	switch (flavour) {
	case DEVLINK_PORT_FLAVOUR_PHYSICAL:
		return "physical";
	case DEVLINK_PORT_FLAVOUR_CPU:
		return "cpu";
	case DEVLINK_PORT_FLAVOUR_DSA:
		return "dsa";
	case DEVLINK_PORT_FLAVOUR_PCI_PF:
		return "pci_pf";
	case DEVLINK_PORT_FLAVOUR_PCI_VF:
		return "pci_vf";
	case DEVLINK_PORT_FLAVOUR_VIRTUAL:
		return "virtual";
	case DEVLINK_PORT_FLAVOUR_UNUSED:
		return "unused";
	case DEVLINK_PORT_FLAVOUR_PCI_SF:
		return "pci_sf";
	default:
		return "";
	}
}
#endif

} // namespace smi_utils



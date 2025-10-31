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

#include "smi_nic_system.h"
#include "smi_sysfs.h"
#include <cstdio>
#include <algorithm>
#include <set>
#include <regex>
#include <iomanip>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

SmiNicSystem::SmiNicSystem() : net_path_("/sys/class/net"), pci_path_("/sys/bus/pci/devices") {}

bool SmiNicSystem::interface_exists(const std::string& iface)
{
	std::error_code ec;
	return fs::exists(fs::path(net_path_) / fs::path(iface).string(), ec);
}

bool SmiNicSystem::resolve_bdf(const std::string& symlink, std::string& bdf) const
{
	char resolved_path[PATH_MAX];
	ssize_t len = readlink(symlink.c_str(), resolved_path, sizeof(resolved_path) - 1);

	if (len == -1) {
		return false;
	}
	resolved_path[len] = '\0';

	try {
		fs::path symlink_dir = fs::path(symlink).parent_path();
		fs::path target_path = symlink_dir / resolved_path;
		std::string full_path = fs::canonical(target_path);
		bdf = fs::path(full_path).filename();
		return true;
	} catch (const fs::filesystem_error&) {
		return false;
	}
}

std::pair<uint16_t, uint16_t> SmiNicSystem::read_pci_ids(const std::string& sysfs_bus_path) const
{
	uint16_t vendor_id = 0, device_id = 0;

	std::string vendor_path = sysfs_bus_path + "/vendor";
	std::string device_path = sysfs_bus_path + "/device";

	SmiSysfsReader::SysfsValue vendor_val, device_val;
	if (SmiSysfsReader::readLine(vendor_path, vendor_val) == SmiSysfsReader::SysfsStatus::Success &&
		SmiSysfsReader::readLine(device_path, device_val) == SmiSysfsReader::SysfsStatus::Success) {

		if (std::holds_alternative<int>(vendor_val)) {
			vendor_id = static_cast<uint16_t>(std::get<int>(vendor_val));
		} else if (std::holds_alternative<std::string>(vendor_val)) {
			vendor_id = static_cast<uint16_t>(std::stoul(std::get<std::string>(vendor_val), nullptr, 0));
		}

		if (std::holds_alternative<int>(device_val)) {
			device_id = static_cast<uint16_t>(std::get<int>(device_val));
		} else if (std::holds_alternative<std::string>(device_val)) {
			device_id = static_cast<uint16_t>(std::stoul(std::get<std::string>(device_val), nullptr, 0));
		}
	}

	return {vendor_id, device_id};
}

bool SmiNicSystem::downstream_port(const std::string& port_bdf, const std::string& bridge_bdf) const
{
	std::error_code ec;
	std::string port_path = pci_path_ + "/" + port_bdf;

	if (!fs::exists(port_path, ec) || !fs::is_symlink(port_path, ec)) {
		return false;
	}

	try {
		std::string port_canon_path = fs::canonical(port_path, ec).string();
		if (ec) {
			return false;
		}

		std::string bridge = "/" + bridge_bdf + "/";
		return port_canon_path.find(bridge) != std::string::npos;

	} catch (const fs::filesystem_error&) {
		return false;
	}
}

void SmiNicSystem::discover_nics()
{
	std::error_code ec;

	if (!fs::exists(pci_path_, ec) || !fs::is_directory(pci_path_, ec)) {
		return;
	}

	nics_.clear();

	for (const auto& entry : fs::directory_iterator(pci_path_, ec)) {
		if (ec) {
			continue;
		}

		std::string bdf = entry.path().filename().string();
		std::string sysfs_bus_path = entry.path().string();
		auto [vendor_id, device_id] = read_pci_ids(sysfs_bus_path);

		if (vendor_id == 0x1dd8 && device_id == 0x0008) {
			nics_.emplace_back("", bdf, NicType::PCIBridge, "", sysfs_bus_path);
			SmiNic& nic = nics_.back();

			for (const auto& net_entry : fs::directory_iterator(net_path_, ec)) {
				if (ec) {
					continue;
				}

				const std::string iface_name = net_entry.path().filename().string();
				std::string device_symlink = net_entry.path().string() + "/device";
				std::string sysfs_class_path = net_entry.path().string();

				if (fs::exists(device_symlink, ec) && fs::is_symlink(device_symlink, ec)) {
					std::string port_bdf;
					if (resolve_bdf(device_symlink, port_bdf)) {
						std::string port_sysfs_bus_path = pci_path_ + "/" + port_bdf;
						auto [port_vendor_id, port_device_id] = read_pci_ids(port_sysfs_bus_path);

						if (port_vendor_id == 0x1dd8 && port_device_id == 0x1002) {
							if (downstream_port(port_bdf, bdf)) {
								SmiNicPort port(iface_name, port_bdf, sysfs_class_path, port_sysfs_bus_path);
								port.discover_infiniband();
								port.collect_vendor_statistics();
								port.collect_standard_statistics();
								nic.add_nic_port(port);
							}
						}
					}
				}
			}
		}
	}


	auto parse_bdf = [](const std::string& bdf) -> std::tuple<int, int, int, int> {
		if (bdf.length() != 12) return {0, 0, 0, 0};

		if (bdf[4] != ':' || bdf[7] != ':' || bdf[10] != '.') {
			return {0, 0, 0, 0};
		}

		try {
			int domain = std::stoi(bdf.substr(0, 4), nullptr, 16);
			int bus = std::stoi(bdf.substr(5, 2), nullptr, 16);
			int device = std::stoi(bdf.substr(8, 2), nullptr, 16);
			int function = std::stoi(bdf.substr(11, 1), nullptr, 16);

			return {domain, bus, device, function};
		} catch (const std::exception&) {
			return {0, 0, 0, 0};
		}
	};

	// Sort NICs by BDF
	std::sort(nics_.begin(), nics_.end(), [&parse_bdf](const SmiNic& x, const SmiNic& y) {
		return parse_bdf(x.bdf()) < parse_bdf(y.bdf());
	});

	// Sort ports within each NIC by BDF for consistency
	for (auto& nic : nics_) {
		auto& ports = const_cast<std::vector<SmiNicPort>&>(nic.nic_ports());
		std::sort(ports.begin(), ports.end(), [&parse_bdf](const SmiNicPort& x, const SmiNicPort& y) {
			return parse_bdf(x.bdf()) < parse_bdf(y.bdf());
		});
	}
}

const std::vector<SmiNic>& SmiNicSystem::get_nics() const
{
	return nics_;
}

std::vector<std::string> SmiNicSystem::list_bdfs()
{
	std::vector<std::string> bdfs;
	for (const auto& nic : nics_) {
		bdfs.push_back(nic.bdf());
	}
	return bdfs;
}

const SmiNic* SmiNicSystem::get_nic_by_interface(const std::string& iface) const
{
	for (const auto& nic : nics_) {
		if (nic.interface() == iface) {
			return &nic;
		}
	}

	return nullptr;
}

const SmiNic* SmiNicSystem::get_nic_by_bdf(const std::string& bdf) const
{
	for (const auto& nic : nics_) {
		if (nic.bdf() == bdf) {
			return &nic;
		}
	}

	return nullptr;
}

const SmiNic* SmiNicSystem::get_nic_by_bdf(uint64_t bdf) const
{
	uint64_t function_number = bdf & 0x7;
	uint64_t device_number = (bdf >> 3) & 0x1F;
	uint64_t bus_number = (bdf >> 8) & 0xFF;
	uint64_t domain_number = (bdf >> 16) & 0xFFFFFFFFFFFF;
	std::ostringstream oss;

	oss << std::hex << std::setfill('0')
	    << std::setw(4) << domain_number << ":"
	    << std::setw(2) << bus_number << ":"
	    << std::setw(2) << device_number << "."
	    << std::setw(1) << function_number;

	return get_nic_by_bdf(oss.str());
}

bool SmiNicSystem::driver_loaded(const std::string& bdf, DriverType driver_type) const
{
	std::error_code ec;
	std::string driver_dir;

	switch (driver_type) {
		case DriverType::IONIC:
			driver_dir = "/sys/bus/pci/drivers/ionic";
			break;
		case DriverType::IONIC_RDMA:
			driver_dir = "/sys/bus/auxiliary/drivers/ionic_rdma.rdma";
			break;
		default:
			return false;
	}

	if (!fs::exists(driver_dir, ec) || !fs::is_directory(driver_dir, ec)) {
		return false;
	}

	try {
		for (const auto& entry : fs::directory_iterator(driver_dir, ec)) {
			if (ec) {
				continue;
			}

			if (!fs::is_symlink(entry, ec)) {
				continue;
			}

			std::string symlink_target = fs::read_symlink(entry.path(), ec).string();
			if (ec) {
				continue;
			}

			if (driver_type == DriverType::IONIC) {
				if (entry.path().filename().string() == bdf) {
					return true;
				}
			}
			else if (driver_type == DriverType::IONIC_RDMA) {
				fs::path full_target_path = entry.path().parent_path() / symlink_target;
				std::string canonical_target = fs::canonical(full_target_path, ec).string();
				if (ec) {
					continue;
				}

				if (canonical_target.find("/" + bdf + "/") != std::string::npos) {
					return true;
				}
			}
		}
	} catch (const fs::filesystem_error&) {
		return false;
	}

	return false;
}

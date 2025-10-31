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

#ifndef __SMI_NIC_SYSTEM_H__
#define __SMI_NIC_SYSTEM_H__

#include <iostream>
#include <filesystem>
#include <string>
#include <map>
#include <unistd.h>
#include <climits>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <cstdint>
#include <utility>
#include "smi_nic.h"

enum class DriverType {
	IONIC,
	IONIC_RDMA,
};

class SmiNicSystem {
public:
	SmiNicSystem();
	~SmiNicSystem() = default;

	void discover_nics();
	std::vector<std::string> list_bdfs();
	bool interface_exists(const std::string& iface);
	const std::vector<SmiNic>& get_nics() const;
	const SmiNic* get_nic_by_interface(const std::string& iface) const;
	const SmiNic* get_nic_by_bdf(const std::string& bdf) const;
	const SmiNic* get_nic_by_bdf(uint64_t bdf) const;

	bool driver_loaded(const std::string& bdf, DriverType driver_type) const;

	SmiNicSystem(const SmiNicSystem &) = delete;
	SmiNicSystem & operator = (const SmiNicSystem &) = delete;
	SmiNicSystem(SmiNicSystem &&) = delete;
	SmiNicSystem & operator = (SmiNicSystem &&) = delete;

private:
	std::string net_path_;
	std::string pci_path_;
	std::map<std::string, std::string> iface_bdf_map_;
	std::vector<SmiNic> nics_;

	bool resolve_bdf(const std::string& symlink, std::string& bdf) const;
	std::pair<uint16_t, uint16_t> read_pci_ids(const std::string& sysfs_bus_path) const;
	bool downstream_port(const std::string& port_bdf, const std::string& bridge_bdf) const;
};

#endif // __SMI_NIC_SYSTEM_H__

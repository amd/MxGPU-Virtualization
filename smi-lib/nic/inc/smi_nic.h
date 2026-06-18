/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_NIC_H__
#define __SMI_NIC_H__

#include <string>
#include <optional>
#include <cstdint>
#include <vector>
#include <map>

#include "smi_ethtool_ioctl.h"

enum class NicType {
	Unknown,
	PCIBridge,
	Ethernet,
	InfiniBand,
};

enum class NicVendor {
	Unknown,
	AMD,
	Broadcom
};

enum class NicProduct {
	Unknown,
	Pollara, //!< AMD Pensando Pollara
	Thor2    //!< Broadcom Thor2
};

enum class NicLinkType {
	UNKNOWN,  //!< unknown type.
	PCIE,     //!< two processors connect via same PCIe switch
	NUMA,     //!< two processors connect via different PCIe switches but on the same CPU
	XNUMA     //!< two processors connect via different PCIe switches but on different CPUs
};

class SmiInfiniBandPort {
public:
	SmiInfiniBandPort(const std::string& netdev, const std::string& rdma_dev,
			  const std::string& name, const std::string& sysfs_path);

	const std::string& netdev() const;
	const std::string& name() const;
	std::optional<uint8_t> port_num() const;
	std::optional<std::string> state() const;
	std::optional<uint16_t> max_mtu() const;
	std::optional<uint16_t> active_mtu() const;
	void collect_hw_counters();
	const std::map<std::string, uint64_t>& get_hw_counters_map() const;

private:
	std::string netdev_;
	std::string rdma_dev_;
	std::string name_;
	std::string sysfs_path_;
	std::map<std::string, uint64_t> hw_counters_map_;
	std::optional<uint16_t> max_mtu_;
	std::optional<uint16_t> active_mtu_;
};

class SmiInfiniBand {
public:
	SmiInfiniBand(const std::string& name, const std::string& sysfs_path);

	std::string rdma_dev() const;
	std::optional<std::string> node_guid() const;
	std::optional<std::string> node_type() const;
	std::optional<std::string> sys_image_guid() const;
	std::optional<std::string> fw_ver() const;

	void add_port(const SmiInfiniBandPort& port);
	const std::vector<SmiInfiniBandPort>& ports() const;
	uint8_t ports_num() const;

private:
	std::string name_;
	std::string sysfs_path_;
	NicType type_ = NicType::InfiniBand;
	std::vector<SmiInfiniBandPort> ports_;
};

class SmiNicPort {
public:
	SmiNicPort(const std::string& iface, const std::string& bdf, const std::string& sysfs_class_path, const std::string& sysfs_bus_path,
		   NicVendor vendor = NicVendor::Unknown);

	const std::string& interface() const;
	const std::string& bdf() const;
	const std::string& sysfs_class_path() const;
	const std::string& sysfs_bus_path() const;

	std::optional<std::string> mac_address() const;
	std::optional<uint32_t> port_num() const;
	std::optional<uint8_t> ifindex() const;
	std::optional<uint8_t> carrier() const;
	std::optional<uint16_t> mtu() const;
	std::optional<std::string> link_state() const;
	std::optional<uint32_t> link_speed() const;

	const std::string port_type() const;
	const std::string& flavour() const;
	void set_flavour(const std::string& flavour);

	std::optional<uint32_t> active_fec() const;
	std::optional<std::string> autoneg() const;
	std::optional<std::string> pause_autoneg() const;
	std::optional<std::string> pause_rx() const;
	std::optional<std::string> pause_tx() const;

	void discover_infiniband();
	void add_infiniband(const SmiInfiniBand& infiniband);
	const std::vector<SmiInfiniBand>& infiniband() const;
	uint8_t infiniband_num() const;
	void collect_vendor_statistics();
	void add_vendor_statistic(struct ethtool_gstrings *strings, struct ethtool_stats *stats);
	const std::map<std::string, uint64_t>& get_vendor_stats_map() const;
	void collect_standard_statistics();
	const std::map<std::string, uint64_t>& get_standard_stats_map() const;
	std::optional<std::string> read_vpd_content() const;

private:
	bool vendor_stat_supported(const std::string& stat_name) const;

	std::string iface_;
	std::string bdf_;
	std::string flavour_;
	NicVendor vendor_;
	NicType type_;
	std::string sysfs_class_path_;
	std::string sysfs_bus_path_;
	std::optional<uint32_t> port_num_;
	std::vector<SmiInfiniBand> infiniband_;
	std::map<std::string, uint64_t> vendor_stats_map_;
	std::map<std::string, uint64_t> standard_stats_map_;
};

class SmiNic {
public:
	SmiNic(const std::string& iface, const std::string& bdf, NicType type = NicType::Unknown,
	       const std::string& sysfs_class_path = "", const std::string& sysfs_bus_path = "",
	       NicVendor vendor = NicVendor::Unknown, NicProduct product = NicProduct::Unknown);
	virtual ~SmiNic() = default;

	const std::string& interface() const;
	const std::string& bdf() const;
	NicType type() const;
	NicVendor vendor() const;
	NicProduct product() const;
	const std::string port_type() const;
	const std::string& sysfs_class_path() const;
	const std::string& sysfs_bus_path() const;

	void add_nic_port(const SmiNicPort& port);
	const std::vector<SmiNicPort>& nic_ports() const;
	uint8_t nic_ports_num() const;

	std::optional<uint16_t> vendor_id() const;
	std::optional<uint16_t> subvendor_id() const;
	std::optional<uint16_t> device_id() const;
	std::optional<uint16_t> subsystem_id() const;
	std::optional<uint8_t> revision() const;
	std::optional<std::string> perm_address() const;
	std::optional<uint32_t> pcie_class() const;
	std::optional<uint8_t> max_pcie_width() const;
	std::optional<uint32_t> max_pcie_speed() const;
	std::optional<uint8_t> numa_node() const;
	std::optional<std::string> numa_affinity(uint8_t node) const;
	// Returns the link type between NIC and the device identified by the given BDF.
	std::optional<NicLinkType> link_type(uint64_t bdf) const;

	// PCIe parent BDF for this NIC
	std::optional<std::string> pcie_parent_bdf() const;

	// Check if this NIC shares the same immediate PCIe parent with another device
	bool share_same_pcie_parent(uint64_t bdf) const;

	// Vendor specific
	virtual std::optional<std::string> product_name() const = 0;
	virtual std::optional<std::string> vendor_name() const = 0;
	virtual std::optional<std::string> part_number() const = 0;
	virtual std::optional<std::string> serial_number() const = 0;

protected:
	std::string iface_;
	std::string bdf_;
	NicType type_;
	NicVendor vendor_;
	NicProduct product_;
	std::string sysfs_class_path_;
	std::string sysfs_bus_path_;
	std::vector<SmiNicPort> ports_;
};

class SmiNicPensando : public SmiNic {
public:
	SmiNicPensando(const std::string& iface, const std::string& bdf, NicType type = NicType::Unknown,
		       const std::string& sysfs_class_path = "", const std::string& sysfs_bus_path = "",
		       NicVendor vendor = NicVendor::AMD, NicProduct product = NicProduct::Pollara);

	std::optional<std::string> vendor_name() const override;
	std::optional<std::string> product_name() const override;
	std::optional<std::string> part_number() const override;
	std::optional<std::string> serial_number() const override;
};

class SmiNicBroadcom : public SmiNic {
public:
	SmiNicBroadcom(const std::string& iface, const std::string& bdf, NicType type = NicType::Unknown,
		       const std::string& sysfs_class_path = "", const std::string& sysfs_bus_path = "",
		       NicVendor vendor = NicVendor::Broadcom, NicProduct product = NicProduct::Thor2);

	std::optional<std::string> vendor_name() const override;
	std::optional<std::string> product_name() const override;
	std::optional<std::string> part_number() const override;
	std::optional<std::string> serial_number() const override;

private:
	std::optional<std::string> read_vpd_content() const;
};

#endif // __SMI_NIC_H__

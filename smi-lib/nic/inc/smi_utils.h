/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_UTILS_H__
#define __SMI_UTILS_H__

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <type_traits>
#include <charconv>
#include <system_error>
#include <limits>

#include "smi_nic.h"
#include "smi_sysfs.h"

namespace smi_utils {

/**
 * @brief Validate BDF address format
 *
 * Validates that a string matches the BDF format: DDDD:BB:DD.F
 * where D=domain (hex), B=bus (hex), D=device (hex), F=function (hex)
 *
 * @param bdf String to validate
 * @return true if valid BDF format, false otherwise
 */
bool is_valid_bdf(const std::string& bdf);

/**
 * @brief Convert BDF string format to uint64_t
 *
 * Converts a BDF string to uint64_t format:
 * (domain << 16) | (bus << 8) | (device << 3) | function
 *
 * @param bdf BDF string
 * @return uint64_t BDF value, or 0 if parsing fails
 */
uint64_t parse_bdf(const std::string& bdf);

/**
 * @brief Convert uint64_t BDF to string format
 *
 * Converts a uint64_t BDF value to string format: DDDD:BB:DD.F
 * where D=domain (hex), B=bus (hex), D=device (hex), F=function (hex)
 *
 * @param bdf uint64_t BDF value
 * @return BDF string in format DDDD:BB:DD.F
 */
std::string format_bdf(uint64_t bdf);

/**
 * @brief Get NUMA node from BDF using sysfs
 *
 * Reads the NUMA node information from sysfs for a given PCI device BDF.
 *
 * @param bdf BDF string of the PCI device
 * @return NUMA node number, or nullopt if not available
 */
std::optional<int> get_numa_node_from_bdf(const std::string& bdf);

/**
 * @brief Get the immediate PCIe parent BDF from sysfs
 *
 * Reads the symlink target from sysfs to determine the parent BDF
 * of a given PCI device.
 *
 * @param bdf BDF string of the PCI device
 * @return Parent BDF string, or nullopt if not available
 */
std::optional<std::string> get_pcie_parent_bdf(const std::string& bdf);

/**
 * @brief Get the current PCIe link generation directly from config space
 *
 * Reads the negotiated Current Link Speed from the PCIe Capability Link Status
 * register, which is encoded as the PCIe generation. This is the currently
 * negotiated generation, not the maximum capability of the device or slot.
 *
 * @param bdf BDF string of the PCI device
 * @return PCIe generation number, or nullopt if not available
 */
std::optional<uint8_t> get_pcie_link_gen(const std::string& bdf);

/**
 * @brief Get the PCIe ancestors of a device
 *
 * Walks up the PCIe hierarchy from the given device and returns its ancestor
 * BDFs ordered from nearest to farthest (the device itself is excluded). The
 * walk is bounded to guard against malformed/cyclic topologies.
 *
 * @param bdf BDF string of the PCI device
 * @return Ancestor BDF strings, nearest first (empty if the device has no parent)
 */
[[nodiscard]] std::vector<std::string> get_pcie_ancestors(const std::string& bdf);

/**
 * @brief Check if two devices share the same PCIe switch
 *
 * Walks up the PCIe tree for both devices to find if they share
 * a common PCIe switch ancestor (not just immediate parent).
 * A shared switch means both devices connect through the same
 * PCIe switch before reaching the root complex.
 *
 * @param bdf1 BDF string of the first PCI device
 * @param bdf2 BDF string of the second PCI device
 * @return true if both devices share a common PCIe switch, false otherwise
 */
[[nodiscard]] bool share_same_pcie_switch(const std::string& bdf1, const std::string& bdf2);

/**
 * @brief Convert NicType enum to string representation
 *
 * @param type NicType enum value
 * @return String representation of the NIC type
 */
std::string nic_type_to_string(NicType type);

#ifdef LIBMNL_INSTALLED
/**
 * @brief Convert devlink port flavour to string representation
 *
 * @param flavour Devlink port flavour value
 * @return String representation of the port flavour
 */
const char* flavour_to_string(uint16_t flavour);
#endif

/**
 * @brief Read data from sysfs path and return as specified type
 *
 * Template function that reads a sysfs file and converts the value
 * to the requested type. Supports string and numeric types.
 * Numbers are parsed as hex only with a 0x/0X prefix, otherwise as decimal.
 *
 * @tparam T The type to return (std::string or numeric type)
 * @param path Path to the sysfs file
 * @return Value of type T, or nullopt if the read fails or the value does not fit in T
 */
template <typename T> std::optional<T> get_sysfs_data(const std::string& path)
{
	SmiSysfsReader::SysfsValue val;
	if (SmiSysfsReader::readLine(path, val) == SmiSysfsReader::SysfsStatus::Success) {
		if constexpr (std::is_same_v<T, std::string>) {
			if (std::holds_alternative<std::string>(val)) {
				return std::get<std::string>(val);
			}
			if (std::holds_alternative<int>(val)) {
				return std::to_string(std::get<int>(val));
			}
		} else {
			if (std::holds_alternative<int>(val)) {
				const int raw = std::get<int>(val);
				if (raw < 0 || static_cast<unsigned long long>(raw) >
						   static_cast<unsigned long long>(
						       std::numeric_limits<T>::max())) {
					return std::nullopt;
				}
				return static_cast<T>(raw);
			}

			const std::string& str = std::get<std::string>(val);
			const bool hex	       = str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0;
			T value {};
			const auto result =
			    std::from_chars(str.data() + (hex ? 2 : 0), str.data() + str.size(),
					    value, hex ? 16 : 10);
			if (result.ec != std::errc()) {
				return std::nullopt;
			}
			return value;
		}
	}

	return std::nullopt;
}

} // namespace smi_utils

#endif // __SMI_UTILS_H__

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

#ifndef __SMI_NIC_INTERFACE_H__
#define __SMI_NIC_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <linux/ethtool.h>

#define SMI_NIC_MAX_STRING_LENGTH 256
#define SMI_NIC_MAX_DEVICES 64
#define SMI_NIC_MAX_STATISTICS 64

typedef enum {
	SMI_NIC_STATUS_SUCCESS = 0,		/**< API completed successfully */
	SMI_NIC_STATUS_ERROR = 1,		/**< Generic error */
	SMI_NIC_STATUS_WRONG_PARAM = 2,		/**< Wrong parameter provided */
	SMI_NIC_STATUS_NOT_FOUND = 3,		/**< NIC not found */
	SMI_NIC_STATUS_NO_RESOURCE = 4,		/**< Memory allocation failed */
	SMI_NIC_STATUS_NOT_SUPPORTED = 5,	/**< API not supported */
	SMI_NIC_STATUS_NOT_INIT = 6,		/**< Not initialized */
	SMI_NIC_STATUS_NO_DATA = 7,		/**< Requested data not found */
	SMI_NIC_STATUS_DRIVER_NOT_LOADED = 8	/**< Required driver not loaded */
} smi_nic_status;

/**
 * @struct smi_nic_discovery_t
 * @brief Structure about discovered NIC devices
 *
 * Contains information about detected network interface cards, including their count
 * and details for each device such as PCI BDF.
 *
 * @var smi_nic_discovery_t::count
 * Number of NIC devices discovered
 * @var smi_nic_discovery_t::devices
 * Array containing details for each discovered NIC device
 */
typedef struct {
	uint32_t count;
	struct {
		char bdf[SMI_NIC_MAX_STRING_LENGTH];		/**< PCI BDF */
	} devices[SMI_NIC_MAX_DEVICES];
} smi_nic_discovery_t;

/**
 * @brief Opaque handle for thread-safe NIC context
 *
 * This handle represents a thread-safe NIC context.
 * Multiple contexts can be created and used concurrently from different threads.
 */
typedef struct smi_nic_ctx *smi_nic_ctx_t;


/**
 * @struct smi_nic_stat_item
 * @brief Structure representing a single statistic name-value pair
 *
 * Contains a statistic name and its corresponding 64-bit value.
 */
typedef struct {
	char name[SMI_NIC_MAX_STRING_LENGTH];
	uint64_t value;
} smi_nic_stat_item;

/**
 * @struct smi_nic_stat_list
 * @brief Structure containing an array of statistics
 *
 * Contains the count and array of statistic name-value pairs.
 */
typedef struct {
	uint32_t count;
	smi_nic_stat_item stats[SMI_NIC_MAX_STATISTICS];
} smi_nic_stat_list;

/**
 * @brief Create a new thread-safe NIC context
 *
 * Creates a new context handle. Each context maintains its own state and
 * can be used concurrently from different threads.
 *
 * @param[out] ctx Pointer to store the created context handle
 *
 * @return ::SMI_NIC_STATUS_SUCCESS if context created successfully
 * @return ::SMI_NIC_STATUS_WRONG_PARAM if ctx is NULL
 * @return ::SMI_NIC_STATUS_NO_RESOURCE if memory allocation failed
 *
 * @note This function is thread-safe
 * @note The context must be destroyed with smi_nic_destroy_context()
 *
 */
smi_nic_status smi_nic_create_context(smi_nic_ctx_t *ctx);

/**
 * @brief Destroy a NIC context and free its resources
 *
 * Destroys the specified context and frees all associated resources.
 *
 * @param[in] ctx Context handle to destroy
 *
 * @return ::SMI_NIC_STATUS_SUCCESS if context destroyed successfully
 * @return ::SMI_NIC_STATUS_WRONG_PARAM if ctx is NULL
 *
 * @note This function is thread-safe
 * @note Do not use the context handle after calling this function
 */
smi_nic_status smi_nic_destroy_context(smi_nic_ctx_t ctx);

/**
 * @brief Discover available NICs and their BDFs.
 *
 * Discovers all available network interface cards.
 *
 * @param ctx Context handle
 * @param discovery Pointer to structure that will be filled with discovered NIC info.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 *
 * @note This function is thread-safe when using separate contexts
 * @note Maximum of SMI_NIC_MAX_DEVICES devices can be discovered
 */
smi_nic_status smi_discover_nics(smi_nic_ctx_t ctx, smi_nic_discovery_t *discovery);

/**
 * @brief Retrieve NIC driver information.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param drvinfo Pointer to an ethtool_drvinfo structure to be filled.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_driver_info(smi_nic_ctx_t ctx, uint64_t device, struct ethtool_drvinfo *drvinfo);

/**
 * @brief Get NIC link status and statistics.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param stats Pointer to an ethtool_stats structure to be filled.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_link_status(smi_nic_ctx_t ctx, uint64_t device, struct ethtool_stats *stats);

/**
 * @brief Retrieve the vendor ID of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param vendor_id Pointer to a variable to store the vendor ID.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_vendor_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t *vendor_id);

/**
 * @brief Retrieve the subvendor ID of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param subvendor_id Pointer to a variable to store the subvendor ID.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_subvendor_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t *subvendor_id);

/**
 * @brief Retrieve the device ID of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param device_id Pointer to a variable to store the device ID.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_device_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t *device_id);

/**
 * @brief Retrieve the subsystem ID of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param subsystem_id Pointer to a variable to store the subsystem ID.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_subsystem_id(smi_nic_ctx_t ctx, uint64_t device, uint16_t *subsystem_id);

/**
 * @brief Retrieve the revision number of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param revision Pointer to a variable to store the revision number.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_revision(smi_nic_ctx_t ctx, uint64_t device, uint8_t *revision);

/**
 * @brief Retrieve the MAC address of a NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param mac Buffer to store the MAC address as a string.
 * @param mac_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_mac_address(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *mac, size_t mac_len);

/**
 * @brief Retrieve the permanent MAC address of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param perm_mac Buffer to store the permanent MAC address as a string.
 * @param perm_mac_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_perm_address(smi_nic_ctx_t ctx, uint64_t device, char *perm_mac, size_t perm_mac_len);
/**
 * @brief Retrieve the max pcie width of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param max_pcie_width Pointer to a variable to store the max pcie width.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_max_pcie_width(smi_nic_ctx_t ctx, uint64_t device, uint8_t *max_pcie_width);
/**
 * @brief Retrieve the max pcie speed of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param max_pcie_speed Pointer to a variable to store the max pcie speed.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_max_pcie_speed(smi_nic_ctx_t ctx, uint64_t device, uint32_t *max_pcie_speed);
/**
 * @brief Retrieve the pcie interface version of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param pcie_interface_version Buffer to store the pcie interface version as a string.
 * @param pcie_interface_version_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_pcie_interface_version(smi_nic_ctx_t ctx, uint64_t device, char *pcie_interface_version, size_t pcie_interface_version_len);
/**
 * @brief Retrieve the slot type of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param slot_type Buffer to store the slot type as a string.
 * @param slot_type_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_slot_type(smi_nic_ctx_t ctx, uint64_t device, char *slot_type, size_t slot_type_len);
/**
 * @brief Retrieve the numa node of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param numa_node Pointer to a variable to store the numa node.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_numa_node(smi_nic_ctx_t ctx, uint64_t device, uint8_t *numa_node);
/**
 * @brief Retrieve the numa affinity of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param affinity Buffer to store the affinity as a string.
 * @param affinity_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_numa_affinity(smi_nic_ctx_t ctx, uint64_t device, uint8_t node, char *affinity, size_t affinity_len);
/**
 * @brief Retrieve the PORT number for a specific port index of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param port_num Pointer to a variable to store the port number.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_port_num_by_index(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *port_num);
/**
 * @brief Retrieve the total number of ports for a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param num_ports Pointer to a variable to store the total number of ports.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_ports_num(smi_nic_ctx_t ctx, uint64_t device, uint32_t *num_ports);
/**
 * @brief Retrieve the network interface name for a specific port of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param interface_name Buffer to store the interface name (e.g., "eth0").
 * @param interface_name_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_port_interface(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *interface_name, size_t interface_name_len);

/**
 * @brief Retrieve the BDF of a specific NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param port_bdf Pointer to store the port BDF as uint64_t.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_port_bdf(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint64_t *port_bdf);
/**
 * @brief Retrieve the type of a specific NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param port_type Buffer to store the port type (e.g. "Ethernet").
 * @param port_type_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_port_type(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *port_type, size_t port_type_len);
/**
 * @brief Retrieve the flavour of a NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param flavour Buffer to store the flavour as a string.
 * @param flavour_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_flavour(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *flavour, size_t flavour_len);
/**
 * @brief Retrieve the ifindex of a NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param ifindex Pointer to a variable to store the ifindex.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_ifindex(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint8_t *ifindex);
/**
 * @brief Retrieve the carrier of a NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param carrier Pointer to a variable to store the carrier.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_carrier(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint8_t *carrier);
/**
 * @brief Retrieve the mtu of a NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param mtu Pointer to a variable to store the mtu.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_mtu(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint16_t *mtu);
/**
 * @brief Retrieve the link state of a NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param link_state Buffer to store the link state as a string.
 * @param link_state_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_link_state(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, char *link_state, size_t link_state_len);
/**
 * @brief Retrieve the link speed of a NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param link_speed Pointer to a variable to store the link speed.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_link_speed(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *link_speed);
/**
 * @brief Retrieve NIC Ethernet pause (flow control) parameters information.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param pause_info Pointer to an ethtool_pauseparam structure to be filled.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_pause_info(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, struct ethtool_pauseparam *pause_info);
/**
 * @brief Retrieve NIC Ethernet Forward Error Correction parameters information.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param fecparam_info Pointer to an ethtool_fecparam structure to be filled.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_fecparam_info(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, struct ethtool_fecparam *fecparam_info);
/**
 * @brief Retrieve NIC link settings information.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the port (0-based)
 * @param link_settings Pointer to an ethtool_link_settings structure to be filled.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on fail
 */
smi_nic_status smi_get_nic_link_settings_info(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, struct ethtool_link_settings *link_settings);

/**
 * @brief Retrieve the number of InfiniBand devices for a specific NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param num_infiniband Pointer to an integer to store the number of InfiniBand devices.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_infiniband_num(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint8_t *num_infiniband);

/**
 * @brief Retrieve the RDMA device name for a given InfiniBand device on a specific port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param rdma_dev Buffer to store the RDMA device name.
 * @param rdma_dev_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_infiniband_rdma_dev(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, char *rdma_dev, size_t rdma_dev_len);

/**
 * @brief Retrieve the node GUID for a given InfiniBand device on a specific port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param guid Buffer to store the node GUID as a string.
 * @param guid_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_infiniband_node_guid(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, char *guid, size_t guid_len);

/**
 * @brief Retrieve the node type for a given InfiniBand device on a specific port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param type Buffer to store the node type as a string.
 * @param type_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_infiniband_node_type(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, char *type, size_t type_len);

/**
 * @brief Retrieve the system image GUID for a given InfiniBand device on a specific port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param guid Buffer to store the system image GUID as a string.
 * @param guid_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_infiniband_sys_image_guid(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, char *guid, size_t guid_len);

/**
 * @brief Retrieve the firmware version for a given InfiniBand device on a specific port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param fw Buffer to store the firmware version as a string.
 * @param fw_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_infiniband_fw_ver(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, char *fw, size_t fw_len);

/**
 * @brief Retrieve the number of ports for a given InfiniBand device on a specific port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param num_ports Pointer to an integer to store the number of ports.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_infiniband_num_ports(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint8_t *num_ports);

/**
 * @brief Retrieve the netdev name for a given InfiniBand port on a specific NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param ib_port_index Index of the InfiniBand port (0-based).
 * @param netdev Buffer to store the netdev name.
 * @param netdev_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_infiniband_port_netdev(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, char *netdev, size_t netdev_len);

/**
 * @brief Retrieve the port number for a given InfiniBand port on a specific NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param ib_port_index Index of the InfiniBand port (0-based).
 * @param port_num Pointer to store the port number.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_infiniband_port_num(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, uint8_t *port_num);

/**
 * @brief Retrieve the state for a given InfiniBand port on a specific NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param ib_port_index Index of the InfiniBand port (0-based).
 * @param state Buffer to store the port state as a string.
 * @param state_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_infiniband_port_state(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, char *state, size_t state_len);

/**
 * @brief Retrieve the maximum MTU for a given InfiniBand port on a specific NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param ib_port_index Index of the InfiniBand port (0-based).
 * @param max_mtu Pointer to a uint32_t to store the maximum MTU.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_infiniband_port_max_mtu(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, uint16_t *max_mtu);

/**
 * @brief Retrieve the active MTU for a given InfiniBand port on a specific NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param ib_port_index Index of the InfiniBand port (0-based).
 * @param active_mtu Pointer to a uint32_t to store the active MTU.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_infiniband_port_active_mtu(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t ib_port_index, uint16_t *active_mtu);


/**
 * @brief Get the count of available standard port statistics for a specified NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param count Pointer to uint32_t to store the number of available statistics.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_port_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *count);

/**
 * @brief Retrieve standard port statistics list for a specified NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param stats Pointer to smi_nic_stat_list structure to be filled.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_port_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, smi_nic_stat_list *stats);

/**
 * @brief Get the count of available vendor statistics for a specified NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param count Pointer to uint32_t to store the number of available statistics.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_vendor_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t *count);

/**
 * @brief Retrieve vendor statistics list for a specified NIC port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param stats Pointer to smi_nic_stat_list structure to be filled.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_vendor_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, smi_nic_stat_list *stats);

/**
 * @brief Get the count of available RDMA hardware counters for a specified InfiniBand port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param rdma_port_index Index of the RDMA port (0-based).
 * @param count Pointer to uint32_t to store the number of available counters.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_rdma_port_statistics_count(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t rdma_port_index, uint32_t *count);

/**
 * @brief Retrieve RDMA hardware counters list for a specified InfiniBand port.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param port_index Index of the NIC port (0-based).
 * @param ib_index Index of the InfiniBand device (0-based).
 * @param rdma_port_index Index of the RDMA port (0-based).
 * @param stats Pointer to smi_nic_stat_list structure to be filled.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_rdma_port_statistics_list(smi_nic_ctx_t ctx, uint64_t device, uint32_t port_index, uint32_t ib_index, uint32_t rdma_port_index, smi_nic_stat_list *stats);


/**
 * @brief Retrieve the link topology type information between nic and processor using their numa nodes.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param nic_numa_info NIC numa node.
 * @param processor_numa_info Processor numa node.
 * @param type Pointer to a int to store the link topology type.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_topo_get_nic_link_type(smi_nic_ctx_t ctx, uint64_t device, uint8_t nic_numa_info, uint8_t processor_numa_info, int *type);

/**
 * @brief Retrieve the product name of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param product_name Buffer to store the product name as a string.
 * @param product_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_product_name(smi_nic_ctx_t ctx, uint64_t device, char *product_name, size_t product_len);

/**
 * @brief Retrieve the vendor name of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param vendor_name Buffer to store the vendor name as a string.
 * @param vendor_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_vendor_name(smi_nic_ctx_t ctx, uint64_t device, char *vendor_name, size_t vendor_len);

/**
 * @brief Retrieve the part number of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param part_number Buffer to store the part number as a string.
 * @param part_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_part_number(smi_nic_ctx_t ctx, uint64_t device, char *part_number, size_t part_len);

/**
 * @brief Retrieve the serial number of a NIC.
 *
 * @param ctx Context handle
 * @param device BDF of the network device.
 * @param serial_number Buffer to store the serial number as a string.
 * @param serial_len Length of the buffer.
 * @return ::smi_nic_status | ::SMI_NIC_STATUS_SUCCESS on success, non-zero on failure.
 */
smi_nic_status smi_get_nic_serial_number(smi_nic_ctx_t ctx, uint64_t device, char *serial_number, size_t serial_len);

#ifdef __cplusplus
}
#endif

#endif // __SMI_NIC_INTERFACE_H__

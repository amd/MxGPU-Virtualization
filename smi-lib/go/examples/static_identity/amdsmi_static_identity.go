// Copyright Advanced Micro Devices, Inc.
//
// SPDX-License-Identifier: MIT

package main

import (
	"fmt"
	"os"

	"smi-lib/go/amdsmi"
	example_utils "smi-lib/go/examples/example_utils"
)

func formatPciIdHex(id uint64) string {
	if id == 0 {
		return "N/A"
	}
	return fmt.Sprintf("0x%x", id)
}

func main() {
	err := runStaticIdentity()
	example_utils.HandleExampleError(err)
}

func runStaticIdentity() error {
	// INIT_ALL_PROCESSORS is needed to discover both GPUs and optional NICs.
	initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS

	fmt.Println("###Test amdsmi.Init")
	fmt.Println("  Initializing with AMDSMI_INIT_ALL_PROCESSORS")
	if err := amdsmi.Init(initFlags); err != nil {
		return err
	}
	fmt.Println("  Initialization complete")

	// Always release AMD SMI resources, including after an API error.
	defer func() {
		fmt.Println("###Test amdsmi.ShutDown")
		if err := amdsmi.ShutDown(); err != nil {
			fmt.Println(err)
			return
		}
		fmt.Println("Shutdown complete")
	}()

	// Processor discovery is the shared prelude for AMD SMI examples.
	fmt.Println("###Test amdsmi.GetProcessorHandles")
	processors, err := amdsmi.GetProcessorHandles()
	if err != nil {
		return err
	}
	fmt.Printf("  number of GPUs is: %d\n\n", len(processors))

	if len(processors) == 0 {
		fmt.Println("  No GPUs found\n")
	}

	// Query each GPU independently because every handle represents one
	// physical device and can report different identity information.
	for processorNumber, processor := range processors {
		if err := printGpuStaticIdentity(processorNumber, processor); err != nil {
			return err
		}
	}

	return printNicStaticIdentity()
}

// Print the ASIC properties that identify the GPU hardware. These values are
// returned as one struct by amdsmi.GetGpuAsicInfo().
func printGpuAsicInfo(asicInfo amdsmi.AsicInfo) {
	// Device and revision IDs can be used to distinguish ASIC variants.
	fmt.Printf("  asic_info['market_name'] is: %s\n", asicInfo.MarketName)
	fmt.Printf("  asic_info['vendor_id'] is: %s\n", formatPciIdHex(uint64(asicInfo.VendorID)))
	fmt.Printf("  asic_info['vendor_name'] is: %s\n", asicInfo.VendorName)
	fmt.Printf("  asic_info['subvendor_id'] is: %s\n", formatPciIdHex(uint64(asicInfo.SubvendorID)))
	fmt.Printf("  asic_info['device_id'] is: %s\n", formatPciIdHex(uint64(asicInfo.DeviceID)))
	fmt.Printf("  asic_info['rev_id'] is: %s\n", fmt.Sprintf("0x%x", asicInfo.RevID))

	// The remaining printed fields combine silicon characteristics, module
	// identifiers, PCI subsystem identity, and capability flags.
	fmt.Printf("  asic_info['asic_serial'] is: %s\n", asicInfo.AsicSerial)
	fmt.Printf("  asic_info['oam_id'] is: %d\n", asicInfo.OamID)
	fmt.Printf("  asic_info['num_of_compute_units'] is: %d\n", asicInfo.NumComputeUnits)
	fmt.Printf(
		"  asic_info['target_graphics_version'] is: %d\n",
		asicInfo.TargetGraphicsVersion,
	)
	fmt.Printf("  asic_info['subsystem_id'] is: %s\n", formatPciIdHex(uint64(asicInfo.SubsystemID)))
	fmt.Printf("  asic_info['flags'] is: %d\n", asicInfo.Flags)
}

// Print manufacturing and product information for the GPU board.
func printGpuBoardInfo(boardInfo amdsmi.BoardInfo) {
	fmt.Printf("  board_info['model_number'] is: %s\n", boardInfo.ModelNumber)
	fmt.Printf("  board_info['product_serial'] is: %s\n", boardInfo.ProductSerial)
	fmt.Printf("  board_info['fru_id'] is: %s\n", boardInfo.FruID)
	fmt.Printf("  board_info['product_name'] is: %s\n", boardInfo.ProductName)
	fmt.Printf("  board_info['manufacturer_name'] is: %s\n", boardInfo.ManufacturerName)
}

// Print the installed VRAM's type, vendor, capacity, interface width, and
// maximum bandwidth.
func printGpuVramInfo(vramInfo amdsmi.VramInfo) {
	// VramType is an enum; its String() method produces a readable label.
	fmt.Printf("  vram_info['vram_type'] is: %s\n", vramInfo.VramType)
	fmt.Printf("  vram_info['vram_vendor'] is: %s\n", vramInfo.VramVendor)
	fmt.Printf("  vram_info['vram_size'] is: %d\n", vramInfo.VramSize)
	fmt.Printf("  vram_info['vram_bit_width'] is: %d\n", vramInfo.VramBitWidth)
	fmt.Printf("  vram_info['vram_max_bandwidth'] is: %d\n", vramInfo.VramMaxBandwidth)
}

// Cache information contains a list because a GPU can expose multiple cache
// levels or cache types.
func printGpuCacheInfo(cacheInfo amdsmi.GpuCacheInfo) {
	cacheEntries := cacheInfo.Cache
	fmt.Printf("  number of cache types is: %d\n", len(cacheEntries))
	if len(cacheEntries) == 0 {
		fmt.Println("  No cache entries found")
		return
	}

	// Print each cache configuration separately so its level, size, sharing,
	// and instance count remain grouped together.
	for cacheNumber, cache := range cacheEntries {
		fmt.Printf("  Cache %d:\n", cacheNumber)
		fmt.Printf("    cache_properties is: %s\n", cache.CacheProperties)
		fmt.Printf("    cache_size is: %d\n", cache.CacheSize)
		fmt.Printf("    cache_level is: %d\n", cache.CacheLevel)
		fmt.Printf("    max_num_cu_shared is: %d\n", cache.MaxNumCuShared)
		fmt.Printf("    num_cache_instance is: %d\n", cache.NumCacheInstance)
	}
}

// Print the GPU driver identity reported for this device.
func printGpuDriverInfo(driverInfo amdsmi.DriverInfo) {
	fmt.Printf("  driver_info['driver_version'] is: %s\n", driverInfo.DriverVersion)
	fmt.Printf("  driver_info['driver_date'] is: %s\n", driverInfo.DriverDate)
	fmt.Printf("  driver_info['driver_name'] is: %s\n", driverInfo.DriverName)
}

// RDMA information is nested by device and then by port. A NIC can expose more
// than one RDMA device, and each RDMA device can expose multiple ports.
func printNicRdmaDevInfo(rdmaDevInfo amdsmi.NicRdmaDevicesInfo) {
	rdmaDevices := rdmaDevInfo.RdmaDevInfo
	fmt.Printf("  number of RDMA devices is: %d\n", len(rdmaDevices))
	if len(rdmaDevices) == 0 {
		fmt.Println("  No RDMA devices found")
		return
	}

	for devNumber, rdmaDev := range rdmaDevices {
		fmt.Printf("  RDMA device %d:\n", devNumber)
		fmt.Printf("    rdma_dev is: %s\n", rdmaDev.RdmaDev)
		fmt.Printf("    node_guid is: %s\n", rdmaDev.NodeGuid)
		fmt.Printf("    node_type is: %s\n", rdmaDev.NodeType)
		fmt.Printf("    sys_image_guid is: %s\n", rdmaDev.SysImageGuid)
		fmt.Printf("    fw_ver is: %s\n", rdmaDev.FwVer)

		rdmaPorts := rdmaDev.RdmaPortInfo
		fmt.Printf("    number of RDMA ports is: %d\n", len(rdmaPorts))

		// Port fields identify the associated network interface and report the RDMA
		// port state, port number, and MTU values.
		for portNumber, rdmaPort := range rdmaPorts {
			fmt.Printf("    RDMA port %d:\n", portNumber)
			fmt.Printf("      netdev is: %s\n", rdmaPort.Netdev)
			fmt.Printf("      state is: %s\n", rdmaPort.State)
			fmt.Printf("      rdma_port is: %d\n", rdmaPort.RdmaPort)
			fmt.Printf("      max_mtu is: %d\n", rdmaPort.MaxMtu)
			fmt.Printf("      active_mtu is: %d\n", rdmaPort.ActiveMtu)
		}
	}
}

// Query and print the static identity fields for one discovered GPU handle.
func printGpuStaticIdentity(
	processorNumber int,
	processor amdsmi.ProcessorHandle,
) error {
	// BDF identifies the GPU's location on the PCI bus and helps correlate
	// this handle with the device shown by operating-system tools.
	fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
	bdf, err := amdsmi.GetGpuDeviceBdf(processor)
	if err != nil {
		return err
	}
	fmt.Printf("  Processor number: %d bdf: %s\n\n", processorNumber, bdf)

	// The following queries report mostly stable GPU identification, hardware,
	// driver, and virtualization-context information.
	fmt.Println("###Test amdsmi.GetGpuAsicInfo")
	asicInfo, err := amdsmi.GetGpuAsicInfo(processor)
	if err != nil {
		return err
	}
	printGpuAsicInfo(asicInfo)
	fmt.Println()

	// Board information identifies the manufactured accelerator product.
	fmt.Println("###Test amdsmi.GetGpuBoardInfo")
	boardInfo, err := amdsmi.GetGpuBoardInfo(processor)
	if err != nil {
		return err
	}
	printGpuBoardInfo(boardInfo)
	fmt.Println()

	// VRAM information describes the memory installed on the board.
	fmt.Println("###Test amdsmi.GetGpuVramInfo")
	vramInfo, err := amdsmi.GetGpuVramInfo(processor)
	if err != nil {
		return err
	}
	printGpuVramInfo(vramInfo)
	fmt.Println()

	// Cache information may contain several entries for different levels.
	fmt.Println("###Test amdsmi.GetGpuCacheInfo")
	cacheInfo, err := amdsmi.GetGpuCacheInfo(processor)
	if err != nil {
		return err
	}
	printGpuCacheInfo(cacheInfo)
	fmt.Println()

	// Driver information identifies the software controlling the GPU.
	fmt.Println("###Test amdsmi.GetGpuDriverInfo")
	driverInfo, err := amdsmi.GetGpuDriverInfo(processor)
	if err != nil {
		return err
	}
	printGpuDriverInfo(driverInfo)
	fmt.Println()

	// Virtualization mode reports how the GPU is currently configured for
	// physical-function or virtual-function use.
	fmt.Println("###Test amdsmi.GetGpuVirtualizationMode")
	virtualizationMode, err := amdsmi.GetGpuVirtualizationMode(processor)
	if err != nil {
		return err
	}
	fmt.Printf("  virtualization_mode is: %s\n", virtualizationMode)
	fmt.Println()

	return nil
}

// Discover NICs and print the static identity fields available for each one.
func printNicStaticIdentity() error {
	// NIC support is optional and may require a supported NIC driver.
	fmt.Println("###Test amdsmi.GetNicProcessorHandles")
	nicProcessors, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetNicProcessorHandles",
		amdsmi.GetNicProcessorHandles,
	)
	if err != nil {
		fmt.Println()
		return nil
	}

	fmt.Printf("  number of NICs is: %d\n", len(nicProcessors))
	if len(nicProcessors) == 0 {
		fmt.Println("  No NICs found")
		fmt.Println()
		return nil
	}

	for nicNumber, nic := range nicProcessors {
		// As with GPUs, the BDF correlates the AMD SMI handle with a PCI
		// device visible to the operating system.
		fmt.Println("###Test amdsmi.GetNicDeviceBdf")
		nicBdf, err := amdsmi.GetNicDeviceBdf(nic)
		if err != nil {
			return err
		}
		fmt.Printf("  NIC number: %d bdf: %s\n\n", nicNumber, nicBdf)

		// GetOptionalAPI() reports an unavailable call and allows the
		// rest of the example to continue instead of terminating.
		fmt.Println("###Test amdsmi.GetNicAsicInfo")
		asicInfo, err := example_utils.GetOptionalAPI(
			os.Stdout,
			fmt.Sprintf("amdsmi.GetNicAsicInfo for NIC %d", nicNumber),
			func() (amdsmi.NicAsicInfo, error) {
				return amdsmi.GetNicAsicInfo(nic)
			},
		)
		if err == nil {
			fmt.Printf("  asic_info['vendor_id'] is: %s\n", formatPciIdHex(uint64(asicInfo.VendorID)))
			fmt.Printf("  asic_info['subvendor_id'] is: %s\n", formatPciIdHex(uint64(asicInfo.SubvendorID)))
			fmt.Printf("  asic_info['device_id'] is: %s\n", formatPciIdHex(uint64(asicInfo.DeviceID)))
			fmt.Printf("  asic_info['subsystem_id'] is: %s\n", formatPciIdHex(uint64(asicInfo.SubsystemID)))
			fmt.Printf("  asic_info['revision'] is: %s\n", fmt.Sprintf("0x%x", asicInfo.Revision))
			fmt.Printf("  asic_info['permanent_address'] is: %s\n", asicInfo.PermanentAddress)
			fmt.Printf("  asic_info['product_name'] is: %s\n", asicInfo.ProductName)
			fmt.Printf("  asic_info['part_number'] is: %s\n", asicInfo.PartNumber)
			fmt.Printf("  asic_info['serial_number'] is: %s\n", asicInfo.SerialNumber)
			fmt.Printf("  asic_info['vendor_name'] is: %s\n", asicInfo.VendorName)
		}
		fmt.Println()

		// Bus information describes the NIC's PCIe location and capabilities.
		fmt.Println("###Test amdsmi.GetNicBusInfo")
		busInfo, err := example_utils.GetOptionalAPI(
			os.Stdout,
			fmt.Sprintf("amdsmi.GetNicBusInfo for NIC %d", nicNumber),
			func() (amdsmi.NicBusInfo, error) {
				return amdsmi.GetNicBusInfo(nic)
			},
		)
		if err == nil {
			fmt.Printf("  bus_info['bdf'] is: %s\n", busInfo.Bdf)
			fmt.Printf("  bus_info['max_pcie_width'] is: %d\n", busInfo.MaxPcieWidth)
			fmt.Printf("  bus_info['max_pcie_speed'] is: %d\n", busInfo.MaxPcieSpeed)
			fmt.Printf(
				"  bus_info['pcie_interface_version'] is: %s\n",
				busInfo.PcieInterfaceVersion,
			)
			fmt.Printf("  bus_info['slot_type'] is: %s\n", busInfo.SlotType)
		}
		fmt.Println()

		// Driver information identifies the host software managing the NIC.
		fmt.Println("###Test amdsmi.GetNicDriverInfo")
		driverInfo, err := example_utils.GetOptionalAPI(
			os.Stdout,
			fmt.Sprintf("amdsmi.GetNicDriverInfo for NIC %d", nicNumber),
			func() (amdsmi.NicDriverInfo, error) {
				return amdsmi.GetNicDriverInfo(nic)
			},
		)
		if err == nil {
			fmt.Printf("  driver_info['name'] is: %s\n", driverInfo.Name)
			fmt.Printf("  driver_info['version'] is: %s\n", driverInfo.Version)
		}
		fmt.Println()

		// RDMA device details may be incomplete without libibverbs installed.
		fmt.Println("###Test amdsmi.GetNicRdmaDevInfo")
		rdmaDevInfo, err := example_utils.GetOptionalAPI(
			os.Stdout,
			fmt.Sprintf("amdsmi.GetNicRdmaDevInfo for NIC %d", nicNumber),
			func() (amdsmi.NicRdmaDevicesInfo, error) {
				return amdsmi.GetNicRdmaDevInfo(nic)
			},
		)
		if err == nil {
			printNicRdmaDevInfo(rdmaDevInfo)
		}
		fmt.Println()
	}

	return nil
}

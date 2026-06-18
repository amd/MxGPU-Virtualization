// This file contains usage examples for the APIs exposed by
// smi-lib/go/amdsmi (see go/amdsmi/amdsmi_interface.go). The intent is to
// keep this file in sync with that package: when a new API is added there,
// add a corresponding example call here.
//
// Some calls are commented out because they are slow or destructive
// (e.g. GPU reset, set num-VF, partition setters, event monitor).
// Uncomment them when you want to exercise that path.
package main

import (
	"fmt"
	"log"
	"os"
	"smi-lib/go/amdsmi"
	"strings"
)

var skippedFunctions []string

const (
	colorYellow = "\033[93m"
	colorReset  = "\033[0m"
)

func main() {
	if err := amdsmi.Init(amdsmi.AMDSMI_INIT_AMD_GPUS); err != nil {
		log.Fatalf("Init failed: %v", err)
	}
	defer amdsmi.ShutDown()

	ver, err := amdsmi.GetLibVersion()
	if err != nil {
		fmt.Fprintf(os.Stderr, "GetLibVersion: %v\n", err)
	} else {
		fmt.Printf("Library version: %d.%d.%d\n", ver.Major, ver.Minor, ver.Release)
	}

	handles, err := amdsmi.GetProcessorHandles()
	if err != nil {
		log.Fatalf("GetProcessorHandles failed: %v", err)
	}
	fmt.Printf("Found %d processor(s)\n\n", len(handles))

	for i, ph := range handles {
		fmt.Printf("========== GPU %d ==========\n", i)

		ptype, err := amdsmi.GetProcessorType(ph)
		if err != nil {
			fmt.Fprintf(os.Stderr, "  [skip] GetProcessorType: %v\n", err)
		} else {
			fmt.Printf("  Processor Type:   %s (%d)\n", ptype, ptype)
		}

		printAsicInfo(ph)
		printGPUActivity(ph)
		printPowerCapInfo(ph)
		printPcieInfo(ph)
		printVramInfo(ph)
		printBoardInfo(ph)
		printEccInfo(ph)
		printVfInfo(ph)
		printPartitionProfileInfo(ph)
		printGpuPhBdf(ph)
		printHandleFromBdf(ph)
		printPhBdf(ph)
		printUuidFromGpuPh(ph)
		printPhFromUuid(ph)
		//printResetGpu(ph)  // disabled as it takes long time to execute. enable if required
		//printSetNumVf(ph)  // disabled for quick check. enable if required
		//printClearVfFb(ph) // disabled as it takes long time to execute. enable if required
		printMemoryPartitionConfig(ph)
		//printSetMemoryPartitionMode(ph) // disabled for quick check. enable if required
		printAcceleratorPartitionProfileConfig(ph)
		printAcceleratorPartitionProfileConfigGlobal(ph)
		printAcceleratorPartitionProfile(ph)
		//printSetAcceleratorPartitionProfile(ph) // disabled for quick check. enable if required
		printGpuVirtualizationMode(ph)
		printGetCpuAffinityWithScope(ph, amdsmi.AMDSMI_AFFINITY_SCOPE_NODE)
		//printGetNodeHandle(ph) // disabled for quick check. enable if required
		printGetVfBdf(ph)
		printGetVfHandleFromBdf(ph)
		printGetVfUuid(ph)
		printGetVfHandleFromUuid(ph)
		printGetVfHandleFromVfIndex(ph)
		printGetProcessorHandlesByType(ph)

		printVfInfoDetails(ph)
		printVfDataDetails(ph)
		printBadPageInfo(ph)
		printBadPageThreshold(ph)
		printRasFeatureInfo(ph)
		printRasPolicyInfo(ph)
		printGpuCacheInfo(ph)
		printLinkTopologyNearest(ph)
		printNumaNodeNumber(ph)
		printLinkMetrics(ph)
		printXgmiFbSharingCaps(ph)
		printSocPstate(ph)
		printXgmiPlpd(ph)
		//printSetSocPstate(ph) // disabled for quick check. enable if required
		//printSetXgmiPlpd(ph) // disabled for quick check. enable if required
		printStatusCodeToString(ph)
		printGetGpuDriverInfo(ph)
		printGetGpuVbiosInfo(ph)
		printGetGpuDriverModel(ph)
		printGetFbLayout(ph)
		printGetFwInfo(ph)
		printGetFwErrorRecords(ph)
		printGetDfcFwTable(ph)
		printGetPowerInfo(ph)
		printIsGpuPowerManagementEnabled(ph)
		printGetClockInfo(ph)
		printGetTempMetric(ph)
		printGetIndexFromProcessorHandle(ph)
		printGetGpuPciBandwidth(ph)
		printGetSupportedPowerCap(ph)
		printGetNpmInfo(ph)
		printGetGpuPtlState(ph)
		printGetGpuPtlFormats(ph)
		printSetGpuPtlState(ph)
		printSetGpuPtlFormats(ph)
		printGetGuestData(ph)
		printGetVfFwInfo(ph)
		printGetVfHbmInfo(ph)
		printGetTdiState(ph)
		printGetCcMode(ph)
		//printSetCcMode(ph) // disabled for quick check. enable if required
		printGetGpuFabricInfo(ph)
		printGetFabricTelemetry(ph)
		printAllocFabricTelemetry(ph)
		printGpuMetrics(ph)
		//printGPUCperEntries(ph) // disabled for quick check. enable if required

		fmt.Println()
	}

	if len(handles) > 1 {
		printLinkTopology(handles[0], handles[1])
		printTopoLinkType(handles[0], handles[1])
		printP2pStatus(handles[0], handles[1])
		printXgmiFbSharingModeInfo(handles[0], handles[1])
		//printSetXgmiFbSharingMode(handles[0]) // disabled for quick check. enable if required
		//printSetXgmiFbSharingModeV2(handles) // disabled for quick check. enable if required
	}

	if len(handles) > 0 {
		// Demonstrate SetPowerCap (read-modify-write round-trip, restoring original value)
		demonstrateSetPowerCap(handles[0])

		// Demonstrate event monitoring (create/read/destroy)
		//demonstrateEventMonitor(handles) // disabled for quick check. enable if required
	}

	printNicInfo(handles)
	printSkippedFunctionsSummary()
}

func printAsicInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetGpuAsicInfo(ph)
	if err != nil {
		logSkip("GetGpuAsicInfo", err)
		return
	}
	fmt.Printf("  ASIC Info:\n")
	fmt.Printf("    Market Name:     %s\n", info.MarketName)
	fmt.Printf("    Vendor Name:     %s\n", info.VendorName)
	fmt.Printf("    Vendor ID:       0x%04X\n", info.VendorID)
	fmt.Printf("    Subvendor ID:    0x%04X\n", info.SubvendorID)
	fmt.Printf("    Device ID:       0x%04X\n", info.DeviceID)
	fmt.Printf("    Rev ID:          0x%02X\n", info.RevID)
	fmt.Printf("    ASIC Serial:     %s\n", info.AsicSerial)
	fmt.Printf("    OAM ID:          %d\n", info.OamID)
	fmt.Printf("    Compute Units:   %d\n", info.NumComputeUnits)
	fmt.Printf("    GFX Version:     %d\n", info.TargetGraphicsVersion)
	fmt.Printf("    Subsystem ID:    0x%04X\n", info.SubsystemID)
	fmt.Printf("    Flags:           0x%X\n", info.Flags)
}

func printGPUActivity(ph amdsmi.ProcessorHandle) {
	activity, err := amdsmi.GetGpuActivity(ph)
	if err != nil {
		logSkip("GetGpuActivity", err)
		return
	}
	fmt.Printf("  GPU Activity:\n")
	fmt.Printf("    GFX:             %d%%\n", activity.GfxActivity)
	fmt.Printf("    Memory:          %d%%\n", activity.UmcActivity)
	fmt.Printf("    Multimedia:      %d%%\n", activity.MmActivity)
}

func printPowerCapInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetPowerCapInfo(ph, 0)
	if err != nil {
		logSkip("GetPowerCapInfo", err)
		return
	}
	fmt.Printf("  Power Cap Info:\n")
	fmt.Printf("    Current:         %d\n", info.PowerCap)
	fmt.Printf("    Default:         %d\n", info.DefaultPowerCap)
	fmt.Printf("    DPM Cap:         %d\n", info.DpmCap)
	fmt.Printf("    Min:             %d\n", info.MinPowerCap)
	fmt.Printf("    Max:             %d\n", info.MaxPowerCap)
}

func printPcieInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetPcieInfo(ph)
	if err != nil {
		logSkip("GetPcieInfo", err)
		return
	}
	fmt.Printf("  PCIe Static:\n")
	fmt.Printf("    Max Width:       x%d\n", info.Static.MaxPcieWidth)
	fmt.Printf("    Max Speed:       %d GT/s\n", info.Static.MaxPcieSpeed)
	fmt.Printf("    Interface Ver:   Gen%d\n", info.Static.PcieInterfaceVersion)
	fmt.Printf("    Slot Type:       %d\n", info.Static.SlotType)
	fmt.Printf("    Max Interface:   Gen%d\n", info.Static.MaxPcieInterfaceVersion)
	fmt.Printf("  PCIe Metric:\n")
	fmt.Printf("    Width:           x%d\n", info.Metric.PcieWidth)
	fmt.Printf("    Speed:           %d MT/s\n", info.Metric.PcieSpeed)
	fmt.Printf("    Bandwidth:       %d Mb/s\n", info.Metric.PcieBandwidth)
	fmt.Printf("    Replay Count:    %d\n", info.Metric.PcieReplayCount)
	fmt.Printf("    L0->Recovery:    %d\n", info.Metric.PcieL0ToRecoveryCount)
	fmt.Printf("    NAK Sent:        %d\n", info.Metric.PcieNakSentCount)
	fmt.Printf("    NAK Received:    %d\n", info.Metric.PcieNakReceivedCount)
	fmt.Printf("    LC other-end recovery: %d\n", info.Metric.PcieLcPerfOtherEndRecoveryCount)
}

func printVramInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetGpuVramInfo(ph)
	if err != nil {
		logSkip("GetGpuVramInfo", err)
		return
	}
	fmt.Printf("  VRAM Info:\n")
	fmt.Printf("    Type:            %s\n", info.VramType)
	fmt.Printf("    Vendor:          %s\n", info.VramVendor)
	fmt.Printf("    Size:            %d MB\n", info.VramSize)
	fmt.Printf("    Bit Width:       %d bits\n", info.VramBitWidth)
	fmt.Printf("    Max Bandwidth:   %d GB/s\n", info.VramMaxBandwidth)
}

func printBoardInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetGpuBoardInfo(ph)
	if err != nil {
		logSkip("GetGpuBoardInfo", err)
		return
	}
	fmt.Printf("  Board Info:\n")
	fmt.Printf("    Model Number:    %s\n", info.ModelNumber)
	fmt.Printf("    Product Serial:  %s\n", info.ProductSerial)
	fmt.Printf("    FRU ID:          %s\n", info.FruID)
	fmt.Printf("    Product Name:    %s\n", info.ProductName)
	fmt.Printf("    Manufacturer:    %s\n", info.ManufacturerName)
}

func printVfInfo(ph amdsmi.ProcessorHandle) {
	numEnabled, numSupported, err := amdsmi.GetNumVf(ph)
	if err != nil {
		logSkip("GetNumVf", err)
		return
	}
	fmt.Printf("  VF Count:          %d enabled / %d supported\n", numEnabled, numSupported)

	partitions, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetVfPartitionInfo", err)
		return
	}
	for j, p := range partitions {
		fmt.Printf("    VF[%d]: handle=0x%X  offset=%d MB  size=%d MB\n",
			j, p.VfHandle, p.Fb.FbOffset, p.Fb.FbSize)
	}
}

func eccBlockLabel(b amdsmi.GpuBlock) string {
	switch b {
	case amdsmi.AMDSMI_GPU_BLOCK_UMC:
		return "UMC"
	case amdsmi.AMDSMI_GPU_BLOCK_SDMA:
		return "SDMA"
	case amdsmi.AMDSMI_GPU_BLOCK_GFX:
		return "GFX"
	case amdsmi.AMDSMI_GPU_BLOCK_MMHUB:
		return "MMHUB"
	case amdsmi.AMDSMI_GPU_BLOCK_ATHUB:
		return "ATHUB"
	case amdsmi.AMDSMI_GPU_BLOCK_PCIE_BIF:
		return "PCIE_BIF"
	case amdsmi.AMDSMI_GPU_BLOCK_HDP:
		return "HDP"
	case amdsmi.AMDSMI_GPU_BLOCK_XGMI_WAFL:
		return "XGMI_WAFL"
	case amdsmi.AMDSMI_GPU_BLOCK_DF:
		return "DF"
	case amdsmi.AMDSMI_GPU_BLOCK_SMN:
		return "SMN"
	case amdsmi.AMDSMI_GPU_BLOCK_SEM:
		return "SEM"
	case amdsmi.AMDSMI_GPU_BLOCK_MP0:
		return "MP0"
	case amdsmi.AMDSMI_GPU_BLOCK_MP1:
		return "MP1"
	case amdsmi.AMDSMI_GPU_BLOCK_FUSE:
		return "FUSE"
	case amdsmi.AMDSMI_GPU_BLOCK_MCA:
		return "MCA"
	case amdsmi.AMDSMI_GPU_BLOCK_VCN:
		return "VCN"
	case amdsmi.AMDSMI_GPU_BLOCK_JPEG:
		return "JPEG"
	case amdsmi.AMDSMI_GPU_BLOCK_IH:
		return "IH"
	case amdsmi.AMDSMI_GPU_BLOCK_MPIO:
		return "MPIO"
	default:
		return fmt.Sprintf("0x%x", uint64(b))
	}
}

// printEccInfo demonstrates GetGpuTotalEccCount, GetGpuEccEnabled, and GetGpuEccCount.
// Per-device ECC APIs may be unavailable on VM guests or without host privileges.
func printEccInfo(ph amdsmi.ProcessorHandle) {
	total, err := amdsmi.GetGpuTotalEccCount(ph)
	if err != nil {
		logSkip("GetGpuTotalEccCount", err)
	} else {
		fmt.Printf("  ECC (total):\n")
		fmt.Printf("    Correctable:     %d\n", total.CorrectableCount)
		fmt.Printf("    Uncorrectable:   %d\n", total.UncorrectableCount)
		fmt.Printf("    Deferred:        %d\n", total.DeferredCount)
	}

	byBlock, err := amdsmi.GetGpuEccEnabled(ph)
	if err != nil {
		logSkip("GetGpuEccEnabled", err)
	} else {
		fmt.Printf("  ECC enabled by block:\n")
		for b := amdsmi.AMDSMI_GPU_BLOCK_FIRST; b <= amdsmi.AMDSMI_GPU_BLOCK_LAST; b <<= 1 {
			fmt.Printf("    %-12s %v\n", eccBlockLabel(b), byBlock[b])
		}
	}

	umc, err := amdsmi.GetGpuEccCount(ph, amdsmi.AMDSMI_GPU_BLOCK_UMC)
	if err != nil {
		logSkip("GetGpuEccCount(UMC)", err)
	} else {
		fmt.Printf("  ECC per-block (UMC):\n")
		fmt.Printf("    Correctable:     %d\n", umc.CorrectableCount)
		fmt.Printf("    Uncorrectable:   %d\n", umc.UncorrectableCount)
		fmt.Printf("    Deferred:        %d\n", umc.DeferredCount)
	}
}

var profileCapLabels = []string{"MEMORY", "ENCODE", "DECODE", "COMPUTE"}

// printPartitionProfileInfo demonstrates GetPartitionProfileInfo (host SR-IOV profiling).
func printPartitionProfileInfo(ph amdsmi.ProcessorHandle) {
	pi, err := amdsmi.GetPartitionProfileInfo(ph)
	if err != nil {
		logSkip("GetPartitionProfileInfo", err)
		return
	}
	fmt.Printf("  Partition profile info:\n")
	fmt.Printf("    Profile count:       %d\n", pi.ProfileCount)
	fmt.Printf("    Current profile idx: %d\n", pi.CurrentProfileIndex)
	for i, prof := range pi.Profiles {
		fmt.Printf("    Profile[%d] vf_count: %d\n", i, prof.VfCount)
		for ct := amdsmi.AMDSMI_PROFILE_CAPABILITY_MEMORY; ct < amdsmi.AMDSMI_PROFILE_CAPABILITY__MAX; ct++ {
			c := prof.ProfileCaps[ct]
			fmt.Printf("      %-7s total=%d available=%d optimal=%d min=%d max=%d\n",
				profileCapLabels[ct], c.Total, c.Available, c.Optimal, c.MinValue, c.MaxValue)
		}
	}
}

func printGpuPhBdf(ph amdsmi.ProcessorHandle) {
	bdfResult, err := amdsmi.GetGpuDeviceBdf(ph)
	if err != nil {
		logSkip("GetGpuDeviceBdf", err)
		return
	}
	fmt.Printf("  GPU device BDF: %s\n", bdfResult)
}

func printHandleFromBdf(ph amdsmi.ProcessorHandle) {
	bdfResult, err := amdsmi.GetGpuDeviceBdf(ph)
	if err != nil {
		logSkip("GetGpuDeviceBdf", err)
		return
	}

	handle, err := amdsmi.GetProcessorHandleFromBdf(bdfResult)

	if err != nil {
		logSkip("GetProcessorHandleFromBdf", err)
		return
	}
	fmt.Printf("  GetProcessorHandleFromBdf:\n")
	fmt.Printf("    original processor handle: %d\n", ph)
	fmt.Printf("    from bdf processor handle: %d\n", handle)
}

func printPhBdf(ph amdsmi.ProcessorHandle) {
	bdfResult, err := amdsmi.GetProcessorBdf(ph)
	if err != nil {
		logSkip("GetProcessorBdf", err)
		return
	}

	bdfResult2, err := amdsmi.GetGpuDeviceBdf(ph)
	if err != nil {
		logSkip("GetGpuDeviceBdf", err)
		return
	}

	fmt.Printf("  From ph BDF information:\n")
	fmt.Printf("     ph bdf.bus() = %d \n", bdfResult.Bus())
	fmt.Printf("     gpu  bdf.bus() = %d \n", bdfResult2.Bus())

}

func printUuidFromGpuPh(ph amdsmi.ProcessorHandle) {
	uuid, err := amdsmi.GetGpuDeviceUuid(ph)
	if err != nil {
		logSkip("GetGpuDeviceUuid", err)
		return
	}

	fmt.Printf("  From gpu ph uuid: %s\n", uuid)
}

func printPhFromUuid(ph amdsmi.ProcessorHandle) {
	uuid, err := amdsmi.GetGpuDeviceUuid(ph)
	if err != nil {
		logSkip("GetGpuDeviceUuid", err)
		return
	}

	phRes, err := amdsmi.GetProcessorHandleFromUuid(uuid)
	if err != nil {
		logSkip("GetProcessorHandleFromUuid", err)
		return
	}
	fmt.Printf("  Get ph from uuid:\n")
	fmt.Printf("     ph original = %d \n", ph)
	fmt.Printf("     ph new      = %d \n", phRes)
}

func printResetGpu(ph amdsmi.ProcessorHandle) {
	err := amdsmi.ResetGpu(ph)
	if err != nil {
		logSkip("ResetGpu", err)
		return
	}
	fmt.Printf("  ResetGpu: success\n")
}

func printSetNumVf(ph amdsmi.ProcessorHandle) {
	numEnabled, _, err := amdsmi.GetNumVf(ph)
	if err != nil {
		logSkip("GetNumVf (for SetNumVf demo)", err)
		return
	}

	err = amdsmi.SetNumVf(ph, numEnabled)
	if err != nil {
		logSkip("SetNumVf", err)
		return
	}
	fmt.Printf("  SetNumVf(%d): success\n", numEnabled)
}

func printClearVfFb(ph amdsmi.ProcessorHandle) {
	partitions, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetVfPartitionInfo (for ClearVfFb demo)", err)
		return
	}
	if len(partitions) == 0 {
		fmt.Printf("  ClearVfFb: no VFs to clear\n")
		return
	}

	err = amdsmi.ClearVfFb(partitions[0].VfHandle)
	if err != nil {
		logSkip("ClearVfFb", err)
		return
	}
	fmt.Printf("  ClearVfFb(VF[0]): success\n")
}

func printMemoryPartitionConfig(ph amdsmi.ProcessorHandle) {
	config, err := amdsmi.GetGpuMemoryPartitionConfig(ph)
	if err != nil {
		logSkip("GetGpuMemoryPartitionConfig", err)
		return
	}
	fmt.Printf("  Memory Partition Config:\n")
	fmt.Printf("    Mode:            %s (%d)\n", config.Mode, config.Mode)
	fmt.Printf("    NPS caps:        %s\n", config.PartitionCaps)
	fmt.Printf("    NUMA ranges:     %d\n", config.NumNumaRanges)
	for i := uint32(0); i < config.NumNumaRanges; i++ {
		r := config.NumaRanges[i]
		fmt.Printf("      [%d] type=%s start=0x%X end=0x%X\n", i, r.MemoryType, r.Start, r.End)
	}
}

func printSetMemoryPartitionMode(ph amdsmi.ProcessorHandle) {
	err := amdsmi.SetGpuMemoryPartitionMode(ph, amdsmi.AMDSMI_MEMORY_PARTITION_NPS4)
	if err != nil {
		logSkip("SetGpuMemoryPartitionMode(NPS4)", err)
		return
	}
	fmt.Printf("  SetGpuMemoryPartitionMode(NPS4): success\n")

	config, err := amdsmi.GetGpuMemoryPartitionConfig(ph)
	if err != nil {
		logSkip("GetGpuMemoryPartitionConfig (verify)", err)
		return
	}
	fmt.Printf("  Current memory partition mode: %s (%d)\n", config.Mode, config.Mode)
}

func printAcceleratorPartitionProfileConfig(ph amdsmi.ProcessorHandle) {
	config, err := amdsmi.GetGpuAcceleratorPartitionProfileConfig(ph)
	if err != nil {
		logSkip("GetGpuAcceleratorPartitionProfileConfig", err)
		return
	}
	fmt.Printf("  Accelerator Partition Profile Config:\n")
	fmt.Printf("    Profiles:          %d\n", config.NumProfiles)
	fmt.Printf("    Resource profiles: %d\n", config.NumResourceProfiles)
	fmt.Printf("    Default index:     %d\n", config.DefaultProfileIndex)
	for i := uint32(0); i < config.NumResourceProfiles; i++ {
		rp := config.ResourceProfiles[i]
		fmt.Printf("    ResourceProfile[%d]: index=%d type=%s partition_resource=%d shared_by=%d\n",
			i, rp.ProfileIndex, rp.ResourceType, rp.PartitionResource, rp.NumPartitionsShareResource)
	}
	for i := uint32(0); i < config.NumProfiles; i++ {
		p := config.Profiles[i]
		fmt.Printf("    Profile[%d]: type=%s partitions=%d index=%d num_resources=%d\n",
			i, p.ProfileType, p.NumPartitions, p.ProfileIndex, p.NumResources)
		fmt.Printf("      NPS caps:   %s\n", p.MemoryCaps)
		fmt.Printf("      Resources:  %v\n", p.Resources)
	}
}

func printAcceleratorPartitionProfile(ph amdsmi.ProcessorHandle) {
	profile, partitionIDs, err := amdsmi.GetGpuAcceleratorPartitionProfile(ph)
	if err != nil {
		logSkip("GetGpuAcceleratorPartitionProfile", err)
		return
	}
	fmt.Printf("  Current Accelerator Partition Profile:\n")
	fmt.Printf("    Type:          %s\n", profile.ProfileType)
	fmt.Printf("    Partitions:    %d\n", profile.NumPartitions)
	fmt.Printf("    Memory caps:   %s\n", profile.MemoryCaps)
	fmt.Printf("    Profile index: %d\n", profile.ProfileIndex)
	fmt.Printf("    Num resources:  %d\n", profile.NumResources)
	fmt.Printf("    Resources:     %v\n", profile.Resources)
	fmt.Printf("    Partition IDs: %v\n", partitionIDs)
}

func printSetAcceleratorPartitionProfile(ph amdsmi.ProcessorHandle) {
	err := amdsmi.SetGpuAcceleratorPartitionProfile(ph, 0)
	if err != nil {
		logSkip("SetGpuAcceleratorPartitionProfile(0)", err)
		return
	}
	fmt.Printf("  SetGpuAcceleratorPartitionProfile(0): success\n")

	profile, _, err := amdsmi.GetGpuAcceleratorPartitionProfile(ph)
	if err != nil {
		logSkip("GetGpuAcceleratorPartitionProfile (verify)", err)
		return
	}
	fmt.Printf("  Current profile index: %d (%s)\n", profile.ProfileIndex, profile.ProfileType)
}

func printGpuVirtualizationMode(ph amdsmi.ProcessorHandle) {
	mode, err := amdsmi.GetGpuVirtualizationMode(ph)
	if err != nil {
		logSkip("GetGpuVirtualizationMode", err)
		return
	}

	fmt.Printf("  Get GPU virtualization mode: %s\n", mode.String())
}

func printGetCpuAffinityWithScope(ph amdsmi.ProcessorHandle, scope amdsmi.AffinityScope) {
	mode, err := amdsmi.GetCpuAffinityWithScope(ph, scope)
	if err != nil {
		logSkip("GetCpuAffinityWithScope", err)
		return
	}
	fmt.Printf("  Get CPU Affinity With Scope: \n")
	fmt.Printf("  	GetCpuAffinityWithScope mode[0]: %d\n", mode[0])
	fmt.Printf("  	GetCpuAffinityWithScope mode[1]: %d\n", mode[1])
	fmt.Printf("  	GetCpuAffinityWithScope mode[2]: %d\n", mode[2])
	fmt.Printf("  	GetCpuAffinityWithScope mode[2]: %d\n", mode[3])

}

// not supported on asrock
func printGetNodeHandle(ph amdsmi.ProcessorHandle) {
	nh, err := amdsmi.GetNodeHandle(ph)
	if err != nil {
		logSkip("GetNodeHandle", err)
		return
	}
	fmt.Printf("  GetNodeHandle: %d\n", &nh)
}

func printGetVfBdf(ph amdsmi.ProcessorHandle) {
	partitionsInfo, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetVfBdf a)", err)
		return
	}

	vfbdf, err := amdsmi.GetVfBdf(partitionsInfo[0].VfHandle)
	if err != nil {
		logSkip("GetVfBdf b)", err)
		return
	}

	fmt.Printf("  GetVfBdf:\n")
	fmt.Printf("  	BDF Function: %v\n", vfbdf.Function())
	fmt.Printf("  	BDF Device: %v\n", vfbdf.Device())
	fmt.Printf("  	BDF Bus: %v\n", vfbdf.Bus())

}

func printGetVfHandleFromBdf(ph amdsmi.ProcessorHandle) {
	partitionsInfo, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetVfHandleFromBdf a)", err)
		return
	}

	vfbdf, err := amdsmi.GetVfBdf(partitionsInfo[0].VfHandle)
	if err != nil {
		logSkip("GetVfHandleFromBdf b)", err)
		return
	}

	newVfHandle, err := amdsmi.GetVfHandleFromBdf(vfbdf)
	if err != nil {
		logSkip("GetVfHandleFromBdf c)", err)
		return
	}

	fmt.Printf("  GetVfHandleFromBdf:\n")
	fmt.Printf("  	reference VfHandle: %v\n", partitionsInfo[0].VfHandle)
	fmt.Printf("  	new Vfhandle      : %v\n", newVfHandle)
}

func printGetVfUuid(ph amdsmi.ProcessorHandle) {
	partitionsInfo, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("printGetVfUuid a)", err)
		return
	}

	vfUUID, err := amdsmi.GetVfUuid(partitionsInfo[0].VfHandle)
	if err != nil {
		logSkip("printGetVfUuid b)", err)
		return
	}

	fmt.Printf("  GetVfUuid: %s\n", vfUUID)
}

func printGetVfHandleFromUuid(ph amdsmi.ProcessorHandle) {
	partitionsInfo, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetVfHandleFromUuid a)", err)
		return
	}

	vfUUID, err := amdsmi.GetVfUuid(partitionsInfo[0].VfHandle)
	if err != nil {
		logSkip("GetVfHandleFromUuid b)", err)
		return
	}

	newVfHandle, err := amdsmi.GetVfHandleFromUuid(vfUUID)
	if err != nil {
		logSkip("GetVfHandleFromUuid c)", err)
		return
	}

	fmt.Printf("  GetVfHandleFromUuid:\n")
	fmt.Printf("  	reference VfHandle: %v\n", partitionsInfo[0].VfHandle)
	fmt.Printf("  	new Vfhandle      : %v\n", newVfHandle)
}

func printGetVfHandleFromVfIndex(ph amdsmi.ProcessorHandle) {
	partitionsInfo, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetVfHandleFromVfIndex a)", err)
		return
	}

	newVfHandle, err := amdsmi.GetVfHandleFromVfIndex(ph, 0)
	if err != nil {
		logSkip("GetVfHandleFromVfIndex b)", err)
		return
	}

	fmt.Printf("  GetVfHandleFromVfIndex:\n")
	fmt.Printf("  	reference VfHandle: %v\n", partitionsInfo[0].VfHandle)
	fmt.Printf("  	new Vfhandle      : %v\n", newVfHandle)
}

func printGetProcessorHandlesByType(ph amdsmi.ProcessorHandle) {
	handles, err := amdsmi.GetProcessorHandlesByType(amdsmi.AMDSMI_PROCESSOR_TYPE_AMD_GPU)
	if err != nil {
		logSkip("GetProcessorHandlesByType", err)
		return
	}

	fmt.Printf("  GetProcessorHandlesByType:\n")
	fmt.Printf("  	reference VfHandle: %v\n", ph)
	fmt.Printf("  	new Vfhandle      : %v\n", handles[0])
}

func printStatusCodeToString(ph amdsmi.ProcessorHandle) {
	s0, err := amdsmi.StatusCodeToString(amdsmi.AMDSMI_STATUS_SUCCESS)

	if err != nil {
		logSkip("StatusCodeToString (SUCCESS)", err)
		return
	}

	s1, err := amdsmi.StatusCodeToString(amdsmi.AMDSMI_STATUS_INVAL)
	if err != nil {
		logSkip("StatusCodeToString (INVAL)", err)
		return
	}

	fmt.Printf("  StatusCodeToString:\n")
	fmt.Printf("  	SUCCESS (0)       : %q\n", s0)
	fmt.Printf("  	INVAL (1)         : %q\n", s1)
}

func printGetGpuDriverInfo(ph amdsmi.ProcessorHandle) {
	GPUDrvInfo, err := amdsmi.GetGpuDriverInfo(ph)
	if err != nil {
		logSkip("GetGpuDriverInfo", err)
		return
	}

	fmt.Printf("  GetGpuDriverInfo:\n")
	fmt.Printf("  	Driver version    : %s\n", GPUDrvInfo.DriverVersion)
	fmt.Printf("  	Driver date       : %s\n", GPUDrvInfo.DriverDate)
	fmt.Printf("  	Driver name       : %s\n", GPUDrvInfo.DriverName)
}

func printGetGpuVbiosInfo(ph amdsmi.ProcessorHandle) {
	GPUVBIOSInfo, err := amdsmi.GetGpuVbiosInfo(ph)
	if err != nil {
		logSkip("GetGpuVbiosInfo", err)
		return
	}

	fmt.Printf("  GetGpuVbiosInfo:\n")
	fmt.Printf("  	Name              : %s\n", GPUVBIOSInfo.Name)
	fmt.Printf("  	Build date        : %s\n", GPUVBIOSInfo.BuildDate)
	fmt.Printf("  	Part number       : %s\n", GPUVBIOSInfo.PartNumber)
	fmt.Printf("  	Version           : %s\n", GPUVBIOSInfo.Version)
	fmt.Printf("  	Boot firmware     : %s\n", GPUVBIOSInfo.BootFirmware)
}

func printGetGpuDriverModel(ph amdsmi.ProcessorHandle) {
	dm, err := amdsmi.GetGpuDriverModel(ph)
	if err != nil {
		logSkip("GetGpuDriverModel", err)
		return
	}

	fmt.Printf("  GetGpuDriverModel:\n")
	fmt.Printf("  	Driver model      : %s (%d)\n", dm.String(), dm)
}

func printGetFbLayout(ph amdsmi.ProcessorHandle) {
	FbLayout, err := amdsmi.GetFbLayout(ph)
	if err != nil {
		logSkip("GetFbLayout", err)
		return
	}

	fmt.Printf("  GetFbLayout:\n")
	fmt.Printf("  	TotalFbSize     : %d\n", FbLayout.TotalFbSize)
	fmt.Printf("  	PfFbReserved    : %d\n", FbLayout.PfFbReserved)
	fmt.Printf("  	PfFbOffset      : %d\n", FbLayout.PfFbOffset)
	fmt.Printf("  	FbAlignment     : %d\n", FbLayout.FbAlignment)
	fmt.Printf("  	MaxVfFbUsable   : %d\n", FbLayout.MaxVfFbUsable)
	fmt.Printf("  	MinVfFbUsable   : %d\n", FbLayout.MinVfFbUsable)
}

func printGetFwInfo(ph amdsmi.ProcessorHandle) {
	FwInfo, err := amdsmi.GetFwInfo(ph)
	if err != nil {
		logSkip("GetFwInfo", err)
		return
	}

	fmt.Printf("  GetFwInfo:\n")
	fmt.Printf("  	NumFwInfo       : %d\n", FwInfo.NumFwInfo)

	const maxFwSlots = 64
	printed := 0
	for i := 0; i < len(FwInfo.FwList); i++ {
		e := FwInfo.FwList[i]
		if e.FwVersion == 0 {
			continue
		}
		fmt.Printf("  	[%3d] FwID %s  FwVersion %d\n", i, e.FwID.String(), e.FwVersion)
		printed++
		if printed >= maxFwSlots {
			fmt.Printf("  	... (truncated at %d non-zero entries)\n", maxFwSlots)
			break
		}
	}
	if printed == 0 {
		fmt.Printf("  	(no slots with non-zero FwVersion)\n")
	}
}

func printGetFwErrorRecords(ph amdsmi.ProcessorHandle) {
	rec, err := amdsmi.GetFwErrorRecords(ph)
	if err != nil {
		logSkip("GetFwErrorRecords", err)
		return
	}

	fmt.Printf("  GetFwErrorRecords:\n")
	fmt.Printf("  	NumErrRecords   : %d\n", rec.NumErrRecords)

	n := int(rec.NumErrRecords)
	if n > amdsmi.AMDSMI_MAX_ERR_RECORDS {
		n = amdsmi.AMDSMI_MAX_ERR_RECORDS
	}
	for i := 0; i < n; i++ {
		e := rec.ErrRecords[i]
		fmt.Printf("  	[%d] ts=%d vf_idx=%d fw_id=%d status=%s\n",
			i, e.Timestamp, e.VfIdx, e.FwID, e.Status.String())
	}
}

func printGetDfcFwTable(ph amdsmi.ProcessorHandle) {
	t, err := amdsmi.GetDfcFwTable(ph)
	if err != nil {
		logSkip("GetDfcFwTable", err)
		return
	}

	fmt.Printf("  GetDfcFwTable:\n")
	fmt.Printf("  	DfcFwVersion       : %d\n", t.Header.DfcFwVersion)
	fmt.Printf("  	DfcFwTotalEntries  : %d\n", t.Header.DfcFwTotalEntries)
	fmt.Printf("  	DfcGartWrGuestMin  : %d\n", t.Header.DfcGartWrGuestMin)
	fmt.Printf("  	DfcGartWrGuestMax  : %d\n", t.Header.DfcGartWrGuestMax)

	n := int(t.Header.DfcFwTotalEntries)
	if n > amdsmi.AMDSMI_DFC_FW_NUMBER_OF_ENTRIES {
		n = amdsmi.AMDSMI_DFC_FW_NUMBER_OF_ENTRIES
	}
	for i := 0; i < n; i++ {
		d := t.Data[i]
		fmt.Printf("  	data[%d] type=%d verification_enabled=%d customer_ordinal=%d\n",
			i, d.DfcFwType, d.VerificationEnabled, d.CustomerOrdinal)
		for j := 0; j < 4 && j < amdsmi.AMDSMI_MAX_WHITE_LIST_ELEMENTS; j++ {
			w := d.WhiteList[j]
			if w.Oldest == 0 && w.Latest == 0 {
				continue
			}
			fmt.Printf("  	  white_list[%d] oldest=%d latest=%d\n", j, w.Oldest, w.Latest)
		}
		nb := 0
		for z := 0; z < amdsmi.AMDSMI_MAX_BLACK_LIST_ELEMENTS && nb < 8; z++ {
			if d.BlackList[z] == 0 {
				continue
			}
			fmt.Printf("  	  black_list[%d]=%d\n", z, d.BlackList[z])
			nb++
		}
	}
}

func printGetPowerInfo(ph amdsmi.ProcessorHandle) {
	PowerInfo, err := amdsmi.GetPowerInfo(ph)
	if err != nil {
		logSkip("GetPowerInfo", err)
		return
	}

	fmt.Printf("  GetPowerInfo:\n")
	fmt.Printf("  	SocketPower        : %d\n", PowerInfo.SocketPower)
	fmt.Printf("  	CurrentSocketPower : %d\n", PowerInfo.CurrentSocketPower)
	fmt.Printf("  	AverageSocketPower : %d\n", PowerInfo.AverageSocketPower)
	fmt.Printf("  	GfxVoltage         : %d\n", PowerInfo.GfxVoltage)
	fmt.Printf("  	SocVoltage         : %d\n", PowerInfo.SocVoltage)
	fmt.Printf("  	MemVoltage         : %d\n", PowerInfo.MemVoltage)
	fmt.Printf("  	PowerLimit         : %d\n", PowerInfo.PowerLimit)
}

func printIsGpuPowerManagementEnabled(ph amdsmi.ProcessorHandle) {
	enabled, err := amdsmi.IsGpuPowerManagementEnabled(ph)
	if err != nil {
		logSkip("IsGpuPowerManagementEnabled", err)
		return
	}

	fmt.Printf("  IsGpuPowerManagementEnabled:\n")
	fmt.Printf("  	Enabled            : %v\n", enabled)
}

func printGetClockInfo(ph amdsmi.ProcessorHandle) {
	fmt.Printf("  GetClockInfo:\n")
	clockTypes := []struct {
		label string
		typ   amdsmi.ClkType
	}{
		{"GFX", amdsmi.AMDSMI_CLK_TYPE_GFX},
		{"MEM", amdsmi.AMDSMI_CLK_TYPE_MEM},
		{"SOC", amdsmi.AMDSMI_CLK_TYPE_SOC},
	}
	for _, ct := range clockTypes {
		info, err := amdsmi.GetClockInfo(ph, ct.typ)
		if err != nil {
			logSkip(fmt.Sprintf("GetClockInfo(%s)", ct.label), err)
			continue
		}
		fmt.Printf("  	%-3s MHz clk=%d min=%d max=%d locked=%v deep_sleep=%v\n",
			ct.label, info.Clk, info.MinClk, info.MaxClk, info.ClkLocked, info.ClkDeepSleep)
	}
}

func printGetTempMetric(ph amdsmi.ProcessorHandle) {
	fmt.Printf("  GetTempMetric:\n")
	temp, err := amdsmi.GetTempMetric(ph, amdsmi.AMDSMI_TEMPERATURE_TYPE_EDGE, amdsmi.AMDSMI_TEMP_CURRENT)
	if err != nil {
		logSkip("GetTempMetric(EDGE,CURRENT)", err)
		return
	}
	fmt.Printf("  	EDGE / CURRENT (C): %d\n", temp)

	temp2, err := amdsmi.GetTempMetric(ph, amdsmi.AMDSMI_TEMPERATURE_TYPE_HOTSPOT, amdsmi.AMDSMI_TEMP_CURRENT)
	if err != nil {
		logSkip("GetTempMetric(HOTSPOT,CURRENT)", err)
	} else {
		fmt.Printf("  	HOTSPOT / CURRENT (C): %d\n", temp2)
	}
}

func printGetIndexFromProcessorHandle(ph amdsmi.ProcessorHandle) {
	idx, err := amdsmi.GetIndexFromProcessorHandle(ph)
	if err != nil {
		logSkip("GetIndexFromProcessorHandle", err)
		return
	}

	fmt.Printf("  GetIndexFromProcessorHandle: %d\n", idx)
}

func printGetGpuPciBandwidth(ph amdsmi.ProcessorHandle) {
	bw, err := amdsmi.GetGpuPciBandwidth(ph)
	if err != nil {
		logSkip("GetGpuPciBandwidth", err)
		return
	}

	tr := bw.TransferRate
	fmt.Printf("  GetGpuPciBandwidth:\n")
	fmt.Printf("  	Has deep sleep:    %v\n", tr.HasDeepSleep)
	fmt.Printf("  	Num supported:     %d\n", tr.NumSupported)
	fmt.Printf("  	Current index:     %d\n", tr.Current)
	for i := uint32(0); i < tr.NumSupported && i < uint32(len(tr.Values)); i++ {
		fmt.Printf("  	[%d] Transfer_rate=%d Lanes=x%d\n", i, tr.Values[i], bw.Lanes[i])
	}
}

func printGetSupportedPowerCap(ph amdsmi.ProcessorHandle) {
	entries, err := amdsmi.GetSupportedPowerCap(ph)
	if err != nil {
		logSkip("GetSupportedPowerCap", err)
		return
	}

	fmt.Printf("  GetSupportedPowerCap:\n")
	fmt.Printf("  	Count: %d\n", len(entries))
	for i, e := range entries {
		fmt.Printf("  	[%d] sensor_index=%d type=%s (%d)\n", i, e.SensorIndex, e.Type.String(), e.Type)
	}
}

func printGetNpmInfo(ph amdsmi.ProcessorHandle) {
	nh, err := amdsmi.GetNodeHandle(ph)
	if err != nil {
		logSkip("GetNpmInfo (GetNodeHandle)", err)
		return
	}

	info, err := amdsmi.GetNpmInfo(nh)
	if err != nil {
		logSkip("GetNpmInfo", err)
		return
	}

	fmt.Printf("  GetNpmInfo:\n")
	fmt.Printf("  	Status:     %s (%d)\n", info.Status.String(), info.Status)
	fmt.Printf("  	Limit (W):  %d\n", info.Limit)
}

func printGetGpuPtlState(ph amdsmi.ProcessorHandle) {
	enabled, err := amdsmi.GetGpuPtlState(ph)
	if err != nil {
		logSkip("GetGpuPtlState", err)
		return
	}

	fmt.Printf("  GetGpuPtlState:\n")
	fmt.Printf("  	PTL enabled: %v\n", enabled)
}

func printGetGpuPtlFormats(ph amdsmi.ProcessorHandle) {
	f1, f2, err := amdsmi.GetGpuPtlFormats(ph)
	if err != nil {
		logSkip("GetGpuPtlFormats", err)
		return
	}

	fmt.Printf("  GetGpuPtlFormats:\n")
	fmt.Printf("  	Format1: %s (%d)\n", f1.String(), f1)
	fmt.Printf("  	Format2: %s (%d)\n", f2.String(), f2)
}

func printSetGpuPtlState(ph amdsmi.ProcessorHandle) {
	cur, err := amdsmi.GetGpuPtlState(ph)
	if err != nil {
		logSkip("SetGpuPtlState (GetGpuPtlState)", err)
		return
	}

	if err := amdsmi.SetGpuPtlState(ph, cur); err != nil {
		logSkip("SetGpuPtlState", err)
		return
	}

	fmt.Printf("  SetGpuPtlState:\n")
	fmt.Printf("  	Round-trip current state (%v): ok\n", cur)
}

func printSetGpuPtlFormats(ph amdsmi.ProcessorHandle) {
	enabled, err := amdsmi.GetGpuPtlState(ph)
	if err != nil {
		logSkip("SetGpuPtlFormats (GetGpuPtlState)", err)
		return
	}
	if !enabled {
		logSkip("SetGpuPtlFormats - PTL not enabled", err)
		return
	}

	f1, f2, err := amdsmi.GetGpuPtlFormats(ph)
	if err != nil {
		logSkip("SetGpuPtlFormats (GetGpuPtlFormats)", err)
		return
	}

	if err := amdsmi.SetGpuPtlFormats(ph, f1, f2); err != nil {
		logSkip("SetGpuPtlFormats", err)
		return
	}

	fmt.Printf("  SetGpuPtlFormats:\n")
	fmt.Printf("  	Round-trip formats %s / %s: ok\n", f1.String(), f2.String())
}

func printGetGuestData(ph amdsmi.ProcessorHandle) {
	partitionsInfo, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetGuestData a)", err)
		return
	}
	if len(partitionsInfo) == 0 {
		logSkip("GetGuestData b) - no VF partitions", err)
		return
	}

	guest, err := amdsmi.GetGuestData(partitionsInfo[0].VfHandle)
	if err != nil {
		logSkip("GetGuestData c)", err)
		return
	}

	fmt.Printf("  GetGuestData:\n")
	fmt.Printf("  	Driver version: %s\n", guest.DriverVersion)
	fmt.Printf("  	FB usage (MB):  %d\n", guest.FbUsage)
}

func printGetVfFwInfo(ph amdsmi.ProcessorHandle) {
	partitionsInfo, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetVfFwInfo a)", err)
		return
	}
	if len(partitionsInfo) == 0 {
		logSkip("GetVfFwInfo b) - no VF partitions", err)
		return
	}

	fw, err := amdsmi.GetVfFwInfo(partitionsInfo[0].VfHandle)
	if err != nil {
		logSkip("GetVfFwInfo c)", err)
		return
	}

	fmt.Printf("  GetVfFwInfo:\n")
	fmt.Printf("  	NumFwInfo: %d\n", fw.NumFwInfo)
	for i := 0; i < int(fw.NumFwInfo); i++ {
		e := fw.FwList[i]
		fmt.Printf("  	[%d] FwID %s (%d)  FwVersion %d\n", i, e.FwID.String(), e.FwID, e.FwVersion)
	}
}

func printAcceleratorPartitionProfileConfigGlobal(ph amdsmi.ProcessorHandle) {
	config, err := amdsmi.GetGpuAcceleratorPartitionProfileConfigGlobal(ph)
	if err != nil {
		logSkip("GetGpuAcceleratorPartitionProfileConfigGlobal", err)
		return
	}
	fmt.Printf("  Accelerator Partition Profile Config Global:\n")
	fmt.Printf("    Profiles:          %d\n", config.NumProfiles)
	fmt.Printf("    Resource profiles: %d\n", config.NumResourceProfiles)
	fmt.Printf("    Default index:     %d\n", config.DefaultProfileIndex)
	for i := uint32(0); i < config.NumResourceProfiles; i++ {
		rp := config.ResourceProfiles[i]
		fmt.Printf("    ResourceProfile[%d]: index=%d type=%s partition_resource=%d shared_by=%d\n",
			i, rp.ProfileIndex, rp.ResourceType, rp.PartitionResource, rp.NumPartitionsShareResource)
	}
	for i := uint32(0); i < config.NumProfiles; i++ {
		p := config.Profiles[i]
		fmt.Printf("    Profile[%d]: type=%s partitions=%d index=%d num_resources=%d\n",
			i, p.Profile.ProfileType, p.Profile.NumPartitions, p.Profile.ProfileIndex, p.Profile.NumResources)
		fmt.Printf("      NPS caps:   %s\n", p.Profile.MemoryCaps)
		fmt.Printf("      Resources:  %v\n", p.Profile.Resources)
		fmt.Printf("      VF modes:   %v\n", p.VfMode)
	}
}

func printVfInfoDetails(ph amdsmi.ProcessorHandle) {
	partitions, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetVfPartitionInfo (for VfInfo)", err)
		return
	}
	for j, p := range partitions {
		info, err := amdsmi.GetVfInfo(p.VfHandle)
		if err != nil {
			logSkip(fmt.Sprintf("GetVfInfo(VF[%d])", j), err)
			continue
		}
		fmt.Printf("  VfInfo[%d]:\n", j)
		fmt.Printf("    FB offset:       %d MB\n", info.Fb.FbOffset)
		fmt.Printf("    FB size:         %d MB\n", info.Fb.FbSize)
		fmt.Printf("    GFX timeslice:   %d us\n", info.GfxTimeslice)
	}
}

func printVfDataDetails(ph amdsmi.ProcessorHandle) {
	partitions, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetVfPartitionInfo (for VfData)", err)
		return
	}
	for j, p := range partitions {
		data, err := amdsmi.GetVfData(p.VfHandle)
		if err != nil {
			logSkip(fmt.Sprintf("GetVfData(VF[%d])", j), err)
			continue
		}
		fmt.Printf("  VfData[%d]:\n", j)
		fmt.Printf("    Sched:\n")
		fmt.Printf("      FLR count:           %d\n", data.Sched.FlrCount)
		fmt.Printf("      Boot-up time:        %d us\n", data.Sched.BootUpTime)
		fmt.Printf("      Shutdown time:       %d\n", data.Sched.ShutdownTime)
		fmt.Printf("      Reset time:          %d\n", data.Sched.ResetTime)
		fmt.Printf("      State:               %s\n", data.Sched.State)
		fmt.Printf("      Last boot start:     %s\n", data.Sched.LastBootStart)
		fmt.Printf("      Last boot end:       %s\n", data.Sched.LastBootEnd)
		fmt.Printf("      Last shutdown start: %s\n", data.Sched.LastShutdownStart)
		fmt.Printf("      Last shutdown end:   %s\n", data.Sched.LastShutdownEnd)
		fmt.Printf("      Last reset start:    %s\n", data.Sched.LastResetStart)
		fmt.Printf("      Last reset end:      %s\n", data.Sched.LastResetEnd)
		fmt.Printf("      Current active time: %s\n", data.Sched.CurrentActiveTime)
		fmt.Printf("      Current running time:%s\n", data.Sched.CurrentRunningTime)
		fmt.Printf("      Total active time:   %s\n", data.Sched.TotalActiveTime)
		fmt.Printf("      Total running time:  %s\n", data.Sched.TotalRunningTime)
		fmt.Printf("    Guard:\n")
		fmt.Printf("      Enabled:             %v\n", data.Guard.Enabled)
		for k, g := range data.Guard.Guard {
			fmt.Printf("      %s: state=%s amount=%d interval=%d threshold=%d active=%d\n",
				amdsmi.GuardEventType(k), g.State, g.Amount, g.Interval, g.Threshold, g.Active)
		}
	}
}

func printBadPageInfo(ph amdsmi.ProcessorHandle) {
	pages, err := amdsmi.GetGpuBadPageInfo(ph)
	if err != nil {
		logSkip("GetGpuBadPageInfo", err)
		return
	}
	fmt.Printf("  Bad Page Info: %d record(s)\n", len(pages))
	for i, p := range pages {
		fmt.Printf("    [%d] retired_page=0x%X ts=%d mem_channel=%d mcumc_id=%d\n",
			i, p.RetiredPage, p.Ts, p.MemChannel, p.McumcID)
	}
}

func printBadPageThreshold(ph amdsmi.ProcessorHandle) {
	threshold, err := amdsmi.GetBadPageThreshold(ph)
	if err != nil {
		logSkip("GetBadPageThreshold", err)
		return
	}
	fmt.Printf("  Bad Page Threshold: %d\n", threshold)
}

func printRasFeatureInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetGpuRasFeatureInfo(ph)
	if err != nil {
		logSkip("GetGpuRasFeatureInfo", err)
		return
	}
	fmt.Printf("  RAS Feature Info:\n")
	fmt.Printf("    EEPROM version:          %d\n", info.RasEepromVersion)
	fmt.Printf("    ECC correction schema:   %d\n", info.EccCorrectionSchemaFlag)
}

func printRasPolicyInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetGpuRasPolicyInfo(ph)
	if err != nil {
		logSkip("GetGpuRasPolicyInfo", err)
		return
	}
	fmt.Printf("  RAS Policy Info:\n")
	fmt.Printf("    Version:                     %d.%d\n", info.MajorVersion, info.MinorVersion)
	if info.V4_0 != nil {
		fmt.Printf("    DRAM non-critical threshold: %d\n", info.V4_0.DramNonCriticalRegionThreshold)
		fmt.Printf("    DRAM critical threshold:     %d\n", info.V4_0.DramCriticalRegionThreshold)
	}
}

func printGpuCacheInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetGpuCacheInfo(ph)
	if err != nil {
		logSkip("GetGpuCacheInfo", err)
		return
	}
	fmt.Printf("  GPU Cache Info: %d type(s)\n", info.NumCacheTypes)
	for i, c := range info.Cache {
		fmt.Printf("    [%d] level=%d size=%d KB properties=%s cu_shared=%d instances=%d\n",
			i, c.CacheLevel, c.CacheSize, c.CacheProperties, c.MaxNumCuShared, c.NumCacheInstance)
	}
}

func printLinkTopology(phSrc, phDst amdsmi.ProcessorHandle) {
	topo, err := amdsmi.GetLinkTopology(phSrc, phDst)
	if err != nil {
		logSkip("GetLinkTopology", err)
		return
	}
	fmt.Printf("  Link Topology (GPU 0 -> GPU 1):\n")
	fmt.Printf("    Weight:      %d\n", topo.Weight)
	fmt.Printf("    Link status: %s\n", topo.LinkStatus)
	fmt.Printf("    Link type:   %s\n", topo.LinkType)
	fmt.Printf("    Num hops:    %d\n", topo.NumHops)
	fmt.Printf("    FB sharing:  %d\n", topo.FbSharing)
}

func printTopoLinkType(phSrc, phDst amdsmi.ProcessorHandle) {
	hops, linkType, err := amdsmi.TopoGetLinkType(phSrc, phDst)
	if err != nil {
		logSkip("TopoGetLinkType", err)
		return
	}
	fmt.Printf("  TopoGetLinkType:\n")
	fmt.Printf("    Link type: %s (%d)\n", linkType, linkType)
	if hops == ^uint64(0) {
		fmt.Printf("    Hops:      N/A (NIC query)\n")
	} else {
		fmt.Printf("    Hops:      %d\n", hops)
	}
}

func printLinkTopologyNearest(ph amdsmi.ProcessorHandle) {
	linkTypes := []struct {
		name string
		val  amdsmi.LinkType
	}{
		{"XGMI", amdsmi.AMDSMI_LINK_TYPE_XGMI},
		{"PCIE", amdsmi.AMDSMI_LINK_TYPE_PCIE},
	}
	for _, lt := range linkTypes {
		nearest, err := amdsmi.GetLinkTopologyNearest(ph, lt.val)
		if err != nil {
			logSkip(fmt.Sprintf("GetLinkTopologyNearest(%s)", lt.name), err)
			continue
		}
		fmt.Printf("  Nearest GPUs (%s): %d found\n", lt.name, nearest.Count)
		for j, p := range nearest.ProcessorList {
			bdf, _ := amdsmi.GetGpuDeviceBdf(p)
			fmt.Printf("    [%d] BDF=%s\n", j, bdf)
		}
	}
}

func printP2pStatus(phSrc, phDst amdsmi.ProcessorHandle) {
	linkType, cap, err := amdsmi.TopoGetP2pStatus(phSrc, phDst)
	if err != nil {
		logSkip("TopoGetP2pStatus", err)
		return
	}
	fmt.Printf("  P2P Status (GPU 0 -> GPU 1):\n")
	fmt.Printf("    Link type:        %s\n", linkType)
	fmt.Printf("    Coherent:         %d\n", cap.IsIolinkCoherent)
	fmt.Printf("    Atomics 32-bit:   %d\n", cap.IsIolinkAtomics32bit)
	fmt.Printf("    Atomics 64-bit:   %d\n", cap.IsIolinkAtomics64bit)
	fmt.Printf("    DMA:              %d\n", cap.IsIolinkDma)
	fmt.Printf("    Bi-directional:   %d\n", cap.IsIolinkBiDirectional)
}

func printNumaNodeNumber(ph amdsmi.ProcessorHandle) {
	numaNode, err := amdsmi.TopoGetNumaNodeNumber(ph)
	if err != nil {
		logSkip("TopoGetNumaNodeNumber", err)
		return
	}
	fmt.Printf("  NUMA Node Number:  %d\n", numaNode)
}

func demonstrateSetPowerCap(ph amdsmi.ProcessorHandle) {
	fmt.Println("\n--- SetPowerCap demo ---")

	info, err := amdsmi.GetPowerCapInfo(ph, 0)
	if err != nil {
		logSkip("GetPowerCapInfo (for SetPowerCap demo)", err)
		return
	}
	original := info.PowerCap
	fmt.Printf("  Current power cap: %d\n", original)

	var target uint64 = 700
	fmt.Printf("  Setting power cap to %d...\n", target)
	if err := amdsmi.SetPowerCap(ph, 0, target); err != nil {
		fmt.Fprintf(os.Stderr, "  SetPowerCap: %v (may require elevated privileges)\n", err)
		return
	}

	info2, err := amdsmi.GetPowerCapInfo(ph, 0)
	if err != nil {
		logSkip("GetPowerCapInfo (verify)", err)
		return
	}
	fmt.Printf("  Power cap after set: %d\n", info2.PowerCap)

	fmt.Printf("  Restoring original power cap %d...\n", original)
	if err := amdsmi.SetPowerCap(ph, 0, original); err != nil {
		fmt.Fprintf(os.Stderr, "  Restore failed: %v\n", err)
		return
	}
	fmt.Println("  Restored OK")
}

func printLinkMetrics(ph amdsmi.ProcessorHandle) {
	metrics, err := amdsmi.GetLinkMetrics(ph)
	if err != nil {
		logSkip("GetLinkMetrics", err)
		return
	}
	fmt.Printf("  Link Metrics: %d link(s)\n", metrics.NumLinks)
	for i, l := range metrics.Links {
		fmt.Printf("    [%d] bdf=%s bit_rate=%d max_bandwidth=%d link_type=%s read=%d write=%d link_status=%s\n",
			i, l.Bdf, l.BitRate, l.MaxBandwidth, l.LinkType, l.Read, l.Write, l.LinkStatus)
	}
}

func printXgmiFbSharingCaps(ph amdsmi.ProcessorHandle) {
	caps, err := amdsmi.GetXgmiFbSharingCaps(ph)
	if err != nil {
		logSkip("GetXgmiFbSharingCaps", err)
		return
	}
	fmt.Printf("  XGMI FB Sharing Caps:\n")
	fmt.Printf("    Custom: %d\n", caps.ModeCustomCap)
	fmt.Printf("    Mode 1: %d\n", caps.Mode1Cap)
	fmt.Printf("    Mode 2: %d\n", caps.Mode2Cap)
	fmt.Printf("    Mode 4: %d\n", caps.Mode4Cap)
	fmt.Printf("    Mode 8: %d\n", caps.Mode8Cap)
}

func printXgmiFbSharingModeInfo(phSrc, phDst amdsmi.ProcessorHandle) {
	modes := []struct {
		name string
		val  amdsmi.XgmiFbSharingMode
	}{
		{"CUSTOM", amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM},
		{"MODE_1", amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_1},
		{"MODE_2", amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_2},
		{"MODE_4", amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_4},
		{"MODE_8", amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_8},
	}
	fmt.Printf("  XGMI FB Sharing Mode Info (GPU 0 -> GPU 1):\n")
	for _, m := range modes {
		fbSharing, err := amdsmi.GetXgmiFbSharingModeInfo(phSrc, phDst, m.val)
		if err != nil {
			logSkip(fmt.Sprintf("GetXgmiFbSharingModeInfo(%s)", m.name), err)
			continue
		}
		fmt.Printf("    %s: fb_sharing=%d\n", m.name, fbSharing)
	}
}

func printSocPstate(ph amdsmi.ProcessorHandle) {
	policy, err := amdsmi.GetSocPstate(ph)
	if err != nil {
		logSkip("GetSocPstate", err)
		return
	}
	fmt.Printf("  Soc Pstate Policy:\n")
	fmt.Printf("    Current:         %d\n", policy.Current)
	fmt.Printf("    Num supported:   %d\n", policy.NumSupported)
	for i, p := range policy.Policies {
		fmt.Printf("    [%d] id=%d description=%s\n", i, p.PolicyID, p.PolicyDescription)
	}
}

func printXgmiPlpd(ph amdsmi.ProcessorHandle) {
	policy, err := amdsmi.GetXgmiPlpd(ph)
	if err != nil {
		logSkip("GetXgmiPlpd", err)
		return
	}
	fmt.Printf("  XGMI PLPD Policy:\n")
	fmt.Printf("    Current:         %d\n", policy.Current)
	fmt.Printf("    Num supported:   %d\n", policy.NumSupported)
	for i, p := range policy.Policies {
		fmt.Printf("    [%d] id=%d description=%s\n", i, p.PolicyID, p.PolicyDescription)
	}
}

func printSetSocPstate(ph amdsmi.ProcessorHandle) {
	err := amdsmi.SetSocPstate(ph, 0)
	if err != nil {
		logSkip("SetSocPstate", err)
		return
	}
	fmt.Printf("  SetSocPstate(0): success\n")
}

func printSetXgmiPlpd(ph amdsmi.ProcessorHandle) {
	err := amdsmi.SetXgmiPlpd(ph, 0)
	if err != nil {
		logSkip("SetXgmiPlpd", err)
		return
	}
	fmt.Printf("  SetXgmiPlpd(0): success\n")
}

func printSetXgmiFbSharingMode(ph amdsmi.ProcessorHandle) {
	err := amdsmi.SetXgmiFbSharingMode(ph, amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_4)
	if err != nil {
		logSkip("SetXgmiFbSharingMode", err)
		return
	}
	fmt.Printf("  SetXgmiFbSharingMode(MODE_4): success\n")
}

func printSetXgmiFbSharingModeV2(handles []amdsmi.ProcessorHandle) {
	if len(handles) < 3 {
		logSkip("SetXgmiFbSharingModeV2", fmt.Errorf("need at least 3 processors, got %d", len(handles)))
		return
	}
	customGroup := []amdsmi.ProcessorHandle{handles[0], handles[2]}
	err := amdsmi.SetXgmiFbSharingModeV2(customGroup, amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM)
	if err != nil {
		logSkip("SetXgmiFbSharingModeV2", err)
		return
	}
	fmt.Printf("  SetXgmiFbSharingModeV2(CUSTOM, [processors[0], processors[2]]): success\n")
}

func printNicInfo(gpuHandles []amdsmi.ProcessorHandle) {
	nicHandles, err := amdsmi.GetNicProcessorHandles()
	if err != nil {
		logSkip("GetNicProcessorHandles", err)
		return
	}
	fmt.Printf("\nFound %d NIC(s)\n\n", len(nicHandles))

	for i, ph := range nicHandles {
		fmt.Printf("========== NIC %d ==========\n", i)

		printNicDeviceBdf(ph)
		printNicDriverInfo(ph)
		printNicAsicInfo(ph)
		printNicBusInfo(ph)
		printNicNumaInfo(ph)
		printNicFwInfo(ph)
		printNicPortInfo(ph)
		printNicRdmaDevInfo(ph)
		printNicPortStatistics(ph, 0)
		printNicVendorStatistics(ph, 0)
		printNicRdmaPortStatistics(ph, 0)

		if len(gpuHandles) > 0 {
			printTopoLinkType(ph, gpuHandles[0])
		}

		fmt.Println()
	}
}

func printNicDeviceBdf(ph amdsmi.ProcessorHandle) {
	bdf, err := amdsmi.GetNicDeviceBdf(ph)
	if err != nil {
		logSkip("GetNicDeviceBdf", err)
		return
	}
	fmt.Printf("  NIC BDF: %s\n", bdf)
}

func printNicDriverInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetNicDriverInfo(ph)
	if err != nil {
		logSkip("GetNicDriverInfo", err)
		return
	}
	fmt.Printf("  NIC Driver Info:\n")
	fmt.Printf("    Name:            %s\n", info.Name)
	fmt.Printf("    Version:         %s\n", info.Version)
}

func printNicAsicInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetNicAsicInfo(ph)
	if err != nil {
		logSkip("GetNicAsicInfo", err)
		return
	}
	fmt.Printf("  NIC ASIC Info:\n")
	fmt.Printf("    Vendor ID:       0x%04X\n", info.VendorID)
	fmt.Printf("    Vendor Name:     %s\n", info.VendorName)
	fmt.Printf("    Subvendor ID:    0x%04X\n", info.SubvendorID)
	fmt.Printf("    Device ID:       0x%04X\n", info.DeviceID)
	fmt.Printf("    Subsystem ID:    0x%04X\n", info.SubsystemID)
	fmt.Printf("    Revision:        0x%02X\n", info.Revision)
	fmt.Printf("    Permanent Addr:  %s\n", info.PermanentAddress)
	fmt.Printf("    Product Name:    %s\n", info.ProductName)
	fmt.Printf("    Part Number:     %s\n", info.PartNumber)
	fmt.Printf("    Serial Number:   %s\n", info.SerialNumber)
}

func printNicBusInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetNicBusInfo(ph)
	if err != nil {
		logSkip("GetNicBusInfo", err)
		return
	}
	fmt.Printf("  NIC Bus Info:\n")
	fmt.Printf("    BDF:             %s\n", info.Bdf)
	fmt.Printf("    Max PCIe Width:  x%d\n", info.MaxPcieWidth)
	fmt.Printf("    Max PCIe Speed:  %d GT/s\n", info.MaxPcieSpeed)
	fmt.Printf("    PCIe Interface:  %s\n", info.PcieInterfaceVersion)
	fmt.Printf("    Slot Type:       %s\n", info.SlotType)
}

func printNicNumaInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetNicNumaInfo(ph)
	if err != nil {
		logSkip("GetNicNumaInfo", err)
		return
	}
	fmt.Printf("  NIC NUMA Info:\n")
	fmt.Printf("    Node:            %d\n", info.Node)
	fmt.Printf("    Affinity:        %s\n", info.Affinity)
}

func printNicFwInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetNicFwInfo(ph)
	if err != nil {
		logSkip("GetNicFwInfo", err)
		return
	}
	fmt.Printf("  NIC FW Info (num_fw=%d):\n", info.NumFw)
	for i, fw := range info.Fw {
		fmt.Printf("    FW %d:\n", i)
		fmt.Printf("      Type:           %s\n", fw.Type)
		fmt.Printf("      Name:           %s\n", fw.Fw.Name)
		fmt.Printf("      Version:        %s\n", fw.Fw.Version)
	}
}

func printNicPortInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetNicPortInfo(ph)
	if err != nil {
		logSkip("GetNicPortInfo", err)
		return
	}
	fmt.Printf("  NIC Port Info (num_ports=%d):\n", info.NumPorts)
	for i, p := range info.Ports {
		fmt.Printf("    Port %d:\n", i)
		fmt.Printf("      BDF:            %s\n", p.Bdf)
		fmt.Printf("      Port Num:       %d\n", p.PortNum)
		fmt.Printf("      Type:           %s\n", p.Type)
		fmt.Printf("      Flavour:        %s\n", p.Flavour)
		fmt.Printf("      Netdev:         %s\n", p.Netdev)
		fmt.Printf("      IfIndex:        %d\n", p.Ifindex)
		fmt.Printf("      MAC Address:    %s\n", p.MacAddress)
		fmt.Printf("      Carrier:        %d\n", p.Carrier)
		fmt.Printf("      MTU:            %d\n", p.Mtu)
		fmt.Printf("      Link State:     %s\n", p.LinkState)
		fmt.Printf("      Link Speed:     %d Mb/s\n", p.LinkSpeed)
		fmt.Printf("      Active FEC:     0x%X\n", p.ActiveFec)
		fmt.Printf("      Autoneg:        %s\n", p.Autoneg)
		fmt.Printf("      Pause Autoneg:  %s\n", p.PauseAutoneg)
		fmt.Printf("      Pause RX:       %s\n", p.PauseRx)
		fmt.Printf("      Pause TX:       %s\n", p.PauseTx)
	}
}

func printNicRdmaDevInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetNicRdmaDevInfo(ph)
	if err != nil {
		logSkip("GetNicRdmaDevInfo", err)
		return
	}
	fmt.Printf("  NIC RDMA Devices Info (num_rdma_dev=%d):\n", info.NumRdmaDev)
	for i, d := range info.RdmaDevInfo {
		fmt.Printf("    RDMA Dev %d:\n", i)
		fmt.Printf("      RDMA Dev:       %s\n", d.RdmaDev)
		fmt.Printf("      Node GUID:      %s\n", d.NodeGuid)
		fmt.Printf("      Node Type:      %s\n", d.NodeType)
		fmt.Printf("      Sys Image GUID: %s\n", d.SysImageGuid)
		fmt.Printf("      FW Version:     %s\n", d.FwVer)
		fmt.Printf("      Num RDMA Ports: %d\n", d.NumRdmaPorts)
		for j, p := range d.RdmaPortInfo {
			fmt.Printf("      RDMA Port %d:\n", j)
			fmt.Printf("        Netdev:       %s\n", p.Netdev)
			fmt.Printf("        State:        %s\n", p.State)
			fmt.Printf("        RDMA Port:    %d\n", p.RdmaPort)
			fmt.Printf("        Max MTU:      %d\n", p.MaxMtu)
			fmt.Printf("        Active MTU:   %d\n", p.ActiveMtu)
		}
	}
}

func printNicStats(label string, stats []amdsmi.NicStat) {
	fmt.Printf("  %s (count=%d):\n", label, len(stats))
	for _, s := range stats {
		fmt.Printf("    %-40s %d\n", s.Name+":", s.Value)
	}
}

func printNicPortStatistics(ph amdsmi.ProcessorHandle, portIndex uint32) {
	stats, err := amdsmi.GetNicPortStatistics(ph, portIndex)
	if err != nil {
		logSkip("GetNicPortStatistics", err)
		return
	}
	printNicStats(fmt.Sprintf("NIC Port Statistics (port %d)", portIndex), stats)
}

func printNicVendorStatistics(ph amdsmi.ProcessorHandle, portIndex uint32) {
	stats, err := amdsmi.GetNicVendorStatistics(ph, portIndex)
	if err != nil {
		logSkip("GetNicVendorStatistics", err)
		return
	}
	printNicStats(fmt.Sprintf("NIC Vendor Statistics (port %d)", portIndex), stats)
}

func printNicRdmaPortStatistics(ph amdsmi.ProcessorHandle, rdmaPortIndex uint32) {
	stats, err := amdsmi.GetNicRdmaPortStatistics(ph, rdmaPortIndex)
	if err != nil {
		logSkip("GetNicRdmaPortStatistics", err)
		return
	}
	printNicStats(fmt.Sprintf("NIC RDMA Port Statistics (rdma_port %d)", rdmaPortIndex), stats)
}

// demonstrateEventMonitor opens an EventReader on every processor, polls a few
// times with a short timeout, and prints any received events. The reader is
// torn down automatically via defer reader.Close().
func demonstrateEventMonitor(handles []amdsmi.ProcessorHandle) {
	fmt.Println("========== Event Monitoring ==========")

	reader, err := amdsmi.NewEventReaderMask(handles, amdsmi.AMDSMI_MASK_ALL)
	if err != nil {
		logSkip("NewEventReaderMask", err)
		return
	}
	defer func() {
		if err := reader.Close(); err != nil {
			fmt.Fprintf(os.Stderr, "  [warn] EventReader.Close: %v\n", err)
		}
	}()

	const (
		pollAttempts = 60
		timeoutUsec  = int64(1_000_000) // 1 s per attempt -> 60 s window
	)

	fmt.Printf("  Listening for events on %d processor(s)\n", len(handles))
	for i := 0; i < pollAttempts; i++ {
		entry, err := reader.Read(timeoutUsec)
		if err != nil {
			logSkip(fmt.Sprintf("EventReader.Read [%d/%d]", i+1, pollAttempts), err)
			continue
		}
		fmt.Printf("  Event %d:\n", i+1)
		fmt.Printf("    Date:        %s\n", entry.Date)
		fmt.Printf("    Timestamp:   %d\n", entry.Timestamp)
		fmt.Printf("    Category:    %s (%d)\n", entry.Category, uint32(entry.Category))
		fmt.Printf("    Subcode:     %s (%d)\n", amdsmi.SubcodeName(entry.Category, entry.Subcode), entry.Subcode)
		fmt.Printf("    Severity:    %s (%d)\n", entry.Level, uint32(entry.Level))
		fmt.Printf("    Dev ID:      0x%X\n", entry.DevId)
		fmt.Printf("    Data:        0x%X\n", entry.Data)
		fmt.Printf("    Message:     %s\n", entry.Message)
	}
	fmt.Println()
}

// printGPUCperEntries fetches CPER entries cached in the driver and prints a
// summary for each. It also demonstrates GetAfidsFromCper on the first record.
func printGPUCperEntries(ph amdsmi.ProcessorHandle) {
	const severityMask = 0xFFFFFFFF // all severities
	entries, cursor, err := amdsmi.GetGpuCperEntries(ph, severityMask, 0, 0)
	if err != nil {
		logSkip("GetGpuCperEntries", err)
		return
	}
	if len(entries) == 0 {
		fmt.Println("  CPER Entries: (none cached)")
		return
	}
	fmt.Printf("  CPER Entries (%d, next cursor=%d):\n", len(entries), cursor)
	for i, e := range entries {
		fmt.Printf("    Entry %d:\n", i)
		fmt.Printf("      Signature:    %q\n", e.Header.Signature)
		fmt.Printf("      Revision:     %d\n", e.Header.Revision)
		fmt.Printf("      Severity:     %s (%d)\n", e.Header.ErrorSeverity, uint32(e.Header.ErrorSeverity))
		fmt.Printf("      Notify Type:  %s\n", e.Header.NotifyType.NotifyType())
		fmt.Printf("      Timestamp:    %s\n", e.Header.Timestamp)
		fmt.Printf("      Record Len:   %d bytes\n", e.Header.RecordLength)
		fmt.Printf("      Sec Count:    %d\n", e.Header.SecCnt)
		fmt.Printf("      Flags:        0x%08X\n", e.Header.Flags)
		fmt.Printf("      Persistence:  0x%016X\n", e.Header.PersistenceInfo)
		fmt.Printf("      Record ID:    %q (raw=% X)\n",
			strings.TrimRight(string(e.Header.RecordID[:]), "\x00"),
			e.Header.RecordID[:])
		fmt.Printf("      Raw size:     %d bytes\n", len(e.Bytes))
	}

	afids, err := amdsmi.GetAfidsFromCper(entries[0].Bytes)
	if err != nil {
		logSkip("GetAfidsFromCper", err)
		return
	}
	fmt.Printf("  AFIDs from entry 0 (%d): %v\n", len(afids), afids)
}

func printGpuMetrics(ph amdsmi.ProcessorHandle) {
	metrics, err := amdsmi.GetGpuMetrics(ph)
	if err != nil {
		logSkip("GetGpuMetrics", err)
		return
	}

	fmt.Printf("  GetGpuMetrics: %d metric(s)\n", len(metrics))

	for i, m := range metrics {
		flagNames := make([]string, 0, 4)
		for _, f := range m.Flags.Flags() {
			flagNames = append(flagNames, f.String())
		}
		flagStr := "-"
		if len(flagNames) > 0 {
			flagStr = strings.Join(flagNames, "|")
		}

		fmt.Printf("  	[%3d] name=%-28s unit=%-10s cat=%-10s val=%-20d flags=%-20s vf_mask=0x%08X res=%s/%s inst=%d\n",
			i, m.Name.String(), m.Unit.String(), m.Category.String(),
			m.Val, flagStr, m.VfMask,
			m.ResGroup.String(), m.ResSubgroup.String(), m.ResInstance)
	}
}

func printGetVfHbmInfo(ph amdsmi.ProcessorHandle) {
	partitions, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetVfHbmInfo (GetVfPartitionInfo)", err)
		return
	}
	if len(partitions) == 0 {
		logSkip("GetVfHbmInfo - no VF partitions", nil)
		return
	}
	fmt.Printf("  GetVfHbmInfo:\n")
	for j, p := range partitions {
		info, err := amdsmi.GetVfHbmInfo(p.VfHandle)
		if err != nil {
			logSkip(fmt.Sprintf("GetVfHbmInfo(VF[%d])", j), err)
			continue
		}
		fmt.Printf("  	VF[%d]: phy_addr=0x%X phy_size=%d numa_id=%d name=%s\n",
			j, info.PhyAddr, info.PhySize, info.NumaID, info.Name)
	}
}

func printGetTdiState(ph amdsmi.ProcessorHandle) {
	partitions, err := amdsmi.GetVfPartitionInfo(ph)
	if err != nil {
		logSkip("GetTdiState (GetVfPartitionInfo)", err)
		return
	}
	if len(partitions) == 0 {
		logSkip("GetTdiState - no VF partitions", nil)
		return
	}
	fmt.Printf("  GetTdiState:\n")
	for j, p := range partitions {
		state, err := amdsmi.GetTdiState(p.VfHandle)
		if err != nil {
			logSkip(fmt.Sprintf("GetTdiState(VF[%d])", j), err)
			continue
		}
		fmt.Printf("  	VF[%d]: %s (%d)\n", j, state, state)
	}
}

func printGetCcMode(ph amdsmi.ProcessorHandle) {
	mode, err := amdsmi.GetCcMode(ph)
	if err != nil {
		logSkip("GetCcMode", err)
		return
	}
	fmt.Printf("  GetCcMode: %s (%d)\n", mode, mode)
}

func printSetCcMode(ph amdsmi.ProcessorHandle) {
	cur, err := amdsmi.GetCcMode(ph)
	if err != nil {
		logSkip("SetCcMode (GetCcMode)", err)
		return
	}
	if err := amdsmi.SetCcMode(ph, cur); err != nil {
		logSkip("SetCcMode", err)
		return
	}
	fmt.Printf("  SetCcMode:\n")
	fmt.Printf("  	Round-trip current mode (%s): ok\n", cur)
}

func printGetGpuFabricInfo(ph amdsmi.ProcessorHandle) {
	info, err := amdsmi.GetGpuFabricInfo(ph)
	if err != nil {
		logSkip("GetGpuFabricInfo", err)
		return
	}
	fmt.Printf("  GetGpuFabricInfo:\n")
	fmt.Printf("  	BDF:           %s\n", info.Bdf)
	fmt.Printf("  	Version:       %d\n", info.Version)
	if info.Version != 1 {
		return
	}
	v1 := info.InfoV1
	fmt.Printf("  	InfoV1:\n")
	fmt.Printf("  	  Accelerator ID: %d\n", v1.AcceleratorID)
	fmt.Printf("  	  Fabric Type:    %s (%d)\n", v1.FabricType, v1.FabricType)
	fmt.Printf("  	  Bandwidth:      %d Mb/s\n", v1.Bandwidth)
	fmt.Printf("  	  Latency:        %d ns\n", v1.Latency)
	fmt.Printf("  	  pPoD:           id=%d size=%d\n", v1.PpodID, v1.PpodSize)
	fmt.Printf("  	  vPoD:           id=%d size=%d state=%s\n",
		v1.VpodID, v1.VpodSize, v1.AccelState)
	fmt.Printf("  	  Addr Mode:      %s\n", v1.AddrMode)

	activeAccels := make([]int, 0)
	for word, bits := range v1.VpodActiveAccelerators {
		if bits == 0 {
			continue
		}
		for bit := 0; bit < 32; bit++ {
			if bits&(1<<uint(bit)) != 0 {
				activeAccels = append(activeAccels, word*32+bit)
			}
		}
	}
	fmt.Printf("  	  Active accels:  %v\n", activeAccels)

	localAccels := make([]uint32, 0)
	for _, a := range v1.LocalAccelerators {
		if a != 0 {
			localAccels = append(localAccels, a)
		}
	}
	fmt.Printf("  	  Local accels:   %v\n", localAccels)
}

func printFabricTelemetryDatasets(snap amdsmi.FabricTelemetryData, indent string) {
	for cat, ds := range snap.Datasets {
		fmt.Printf("%s[%s] instances=%d gen=%d ts=%d.%09d\n",
			indent, cat, ds.InstanceCount, ds.GenerationCount,
			ds.TimestampSec, ds.TimestampNsec)
		for _, inst := range ds.Instances {
			fmt.Printf("%s  inst name=%s logical=%d items=%d\n",
				indent, inst.Name, inst.LogicalIdx, inst.ItemCount)
			for _, it := range inst.Items {
				fmt.Printf("%s    id=0x%X value=%d\n", indent, it.ID, it.Value)
			}
		}
	}
}

func printGetFabricTelemetry(ph amdsmi.ProcessorHandle) {
	snap, err := amdsmi.GetFabricTelemetry(ph)
	if err != nil {
		logSkip("GetFabricTelemetry", err)
		return
	}
	fmt.Printf("  GetFabricTelemetry: %d dataset(s)\n", len(snap.Datasets))
	printFabricTelemetryDatasets(snap, "  	")
}

// printAllocFabricTelemetry exercises the alloc/get/close lifecycle directly
// (mirrors the Python AmdSmiFabricTelemetry context-manager usage). It only
// requests UALOE and NETPORT to keep output short.
func printAllocFabricTelemetry(ph amdsmi.ProcessorHandle) {
	tel, err := amdsmi.AllocFabricTelemetry(ph,
		amdsmi.AMDSMI_FABRIC_TELEMETRY_CATEGORY_UALOE,
		amdsmi.AMDSMI_FABRIC_TELEMETRY_CATEGORY_NETPORT)
	if err != nil {
		logSkip("AllocFabricTelemetry", err)
		return
	}
	defer tel.Close()

	snap, err := tel.Get()
	if err != nil {
		logSkip("AllocFabricTelemetry (Get)", err)
		return
	}
	fmt.Printf("  AllocFabricTelemetry: requested %v\n", tel.Categories())
	printFabricTelemetryDatasets(snap, "  	")
}

func logSkip(name string, err error) {
	skippedFunctions = append(skippedFunctions, name)
	fmt.Fprintf(os.Stderr, "  [skip] %s: %v\n", name, err)
}

func printSkippedFunctionsSummary() {
	total := len(skippedFunctions)
	fmt.Println()
	fmt.Printf("%s========== Skipped functions (total: %d) ==========%s\n", colorYellow, total, colorReset)
	if len(os.Args) > 1 {
		if os.Args[1] == "verbose" {
			if total == 0 {
				fmt.Printf("%s  (none)%s\n", colorYellow, colorReset)
				return
			}
			for _, fn := range skippedFunctions {
				fmt.Printf("%s  %s%s\n", colorYellow, fn, colorReset)
			}
		}
	}

}

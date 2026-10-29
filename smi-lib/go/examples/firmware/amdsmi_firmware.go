// Copyright Advanced Micro Devices, Inc.
//
// SPDX-License-Identifier: MIT

package main

import (
	"context"
	"errors"
	"fmt"
	"os"
	"os/signal"
	"strings"
	"syscall"

	"smi-lib/go/amdsmi"
	example_utils "smi-lib/go/examples/example_utils"
)

var pspHexFormatFwBlocks = map[amdsmi.FwBlock]bool{
	amdsmi.AMDSMI_FW_ID_PSP_SYSDRV:           true,
	amdsmi.AMDSMI_FW_ID_PSP_SOSDRV:           true,
	amdsmi.AMDSMI_FW_ID_PSP_KEYDB:            true,
	amdsmi.AMDSMI_FW_ID_DFC:                  true,
	amdsmi.AMDSMI_FW_ID_PSP_SPL:              true,
	amdsmi.AMDSMI_FW_ID_PSP_BL:               true,
	amdsmi.AMDSMI_FW_ID_REG_ACCESS_WHITELIST: true,
	amdsmi.AMDSMI_FW_ID_PSP_SOC:              true,
	amdsmi.AMDSMI_FW_ID_PSP_DBG:              true,
	amdsmi.AMDSMI_FW_ID_PSP_INTF:             true,
	amdsmi.AMDSMI_FW_ID_TA_RAS:               true,
	amdsmi.AMDSMI_FW_ID_TA_XGMI:              true,
	amdsmi.AMDSMI_FW_ID_PSP_RAS:              true,
	amdsmi.AMDSMI_FW_ID_PSP_IPKEYMGR:         true,
	amdsmi.AMDSMI_FW_ID_PSP_IOVM:             true,
	amdsmi.AMDSMI_FW_ID_PSP_SPDM:             true,
	amdsmi.AMDSMI_FW_ID_PSP_DPE:              true,
}

var imuPldmFwBlocks = map[amdsmi.FwBlock]bool{
	amdsmi.AMDSMI_FW_ID_IMU_DRAM:    true,
	amdsmi.AMDSMI_FW_ID_IMU_IRAM:    true,
	amdsmi.AMDSMI_FW_ID_PLDM_BUNDLE: true,
}

var rlcRestoreListFwBlocks = map[amdsmi.FwBlock]bool{
	amdsmi.AMDSMI_FW_ID_RLC_RESTORE_LIST_GPM_MEM: true,
	amdsmi.AMDSMI_FW_ID_RLC_RESTORE_LIST_SRM_MEM: true,
	amdsmi.AMDSMI_FW_ID_RLC_RESTORE_LIST_CNTL:    true,
}

var mesFwBlocks = map[amdsmi.FwBlock]bool{
	amdsmi.AMDSMI_FW_ID_CP_MES:            true,
	amdsmi.AMDSMI_FW_ID_MES_STACK:         true,
	amdsmi.AMDSMI_FW_ID_MES_THREAD1:       true,
	amdsmi.AMDSMI_FW_ID_MES_THREAD1_STACK: true,
}

func main() {
	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	err := runFirmware(ctx)
	stop()

	example_utils.HandleExampleError(err)
}

func runFirmware(ctx context.Context) (err error) {
	prompter := example_utils.NewPrompter(os.Stdin, os.Stdout)

	// INIT_ALL_PROCESSORS is needed to discover both GPUs and optional NICs.
	initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS

	fmt.Println("###Test amdsmi.Init")
	fmt.Println("  Initializing with AMDSMI_INIT_ALL_PROCESSORS")
	if err = amdsmi.Init(initFlags); err != nil {
		return fmt.Errorf("initialize AMD SMI: %w", err)
	}
	fmt.Println("  Initialization complete")

	// Always release AMD SMI resources, including after an API error
	// or keyboard interrupt.
	defer func() {
		fmt.Println("###Test amdsmi.ShutDown")
		if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
			wrapped := fmt.Errorf("shut down AMD SMI: %w", shutdownErr)
			if err != nil && !errors.Is(err, context.Canceled) {
				fmt.Fprintf(os.Stderr, "shut down AMD SMI: %v\n", shutdownErr)
			} else {
				err = wrapped
			}
			return
		}
		fmt.Println("Shutdown complete")
	}()

	// Discover GPU handles. Each handle is passed to the firmware APIs
	// below and remains valid until amdsmi.ShutDown is called.
	fmt.Println("###Test amdsmi.GetProcessorHandles")
	processors, err := amdsmi.GetProcessorHandles()
	if err != nil {
		return fmt.Errorf("get processor handles: %w", err)
	}
	fmt.Printf("  number of GPUs is: %d\n\n", len(processors))

	if len(processors) == 0 {
		fmt.Println("  No GPUs found\n")
	} else {
		fmt.Println("  Available GPUs:")
		for gpuIndex, processor := range processors {
			bdf, bdfErr := amdsmi.GetGpuDeviceBdf(processor)
			if bdfErr != nil {
				return fmt.Errorf("get BDF for GPU %d: %w", gpuIndex, bdfErr)
			}
			fmt.Printf("  GPU %d bdf: %s\n", gpuIndex, bdf)
		}

		allGpuIndex := uint64(len(processors))
		fmt.Printf("  %d: All GPUs\n", allGpuIndex)

		// The index immediately after the last GPU is used as the
		// "All GPUs" menu choice.
		pickGpuIndex, promptErr := prompter.PromptForInput(
			ctx,
			"  Enter the GPU index to inspect: ",
			example_utils.InputOptions{
				DefaultValue: 0,
				UpperBound:   &allGpuIndex,
				ErrorMessage: fmt.Sprintf(
					"  GPU index must be between 0 and %d (%d for all GPUs)",
					len(processors)-1,
					allGpuIndex,
				),
			},
		)
		if promptErr != nil {
			return fmt.Errorf("select GPU index: %w", promptErr)
		}

		selectedGpuIndexes := make([]int, 0, len(processors))
		if pickGpuIndex == allGpuIndex {
			for gpuIndex := range processors {
				selectedGpuIndexes = append(selectedGpuIndexes, gpuIndex)
			}
		} else {
			selectedGpuIndexes = append(selectedGpuIndexes, int(pickGpuIndex))
		}

		for _, gpuIndex := range selectedGpuIndexes {
			if err := ctx.Err(); err != nil {
				return err
			}
			if err := printGpuFirmware(gpuIndex, processors[gpuIndex]); err != nil {
				return err
			}
		}
	}

	// NIC firmware discovery is independent of whether GPUs were found.
	return printNicFirmware(ctx, prompter)
}

func printGpuFirmware(gpuIndex int, processor amdsmi.ProcessorHandle) error {
	fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
	bdf, err := amdsmi.GetGpuDeviceBdf(processor)
	if err != nil {
		return fmt.Errorf("get BDF for selected GPU %d: %w", gpuIndex, err)
	}
	fmt.Printf("  Selected GPU %d bdf: %s\n\n", gpuIndex, bdf)

	// VBIOS metadata identifies the board firmware image installed
	// on the selected GPU.
	fmt.Println("###Test amdsmi.GetGpuVbiosInfo")
	vbiosInfo, err := amdsmi.GetGpuVbiosInfo(processor)
	if err != nil {
		return fmt.Errorf("get vBIOS info for GPU %d: %w", gpuIndex, err)
	}
	fmt.Printf("  vbios_info['name'] is: %s\n", vbiosInfo.Name)
	fmt.Printf("  vbios_info['build_date'] is: %s\n", vbiosInfo.BuildDate)
	fmt.Printf("  vbios_info['part_number'] is: %s\n", vbiosInfo.PartNumber)
	fmt.Printf("  vbios_info['version'] is: %s\n", vbiosInfo.Version)
	fmt.Printf("  vbios_info['boot_firmware'] is: %s\n", vbiosInfo.BootFirmware)
	fmt.Println()

	// The firmware inventory contains versions for the individual
	// firmware blocks running on the GPU.
	fmt.Println("###Test amdsmi.GetFwInfo")
	fwInfo, err := amdsmi.GetFwInfo(processor)
	if err != nil {
		return fmt.Errorf("get firmware info for GPU %d: %w", gpuIndex, err)
	}
	printFirmwareInfo(fwInfo, "  ")
	fmt.Println()

	// Firmware error records are an optional feature, so a missing
	// capability does not stop the example.
	fmt.Println("###Test amdsmi.GetFwErrorRecords")
	errorRecords, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetFwErrorRecords",
		func() (amdsmi.FwErrorRecord, error) {
			return amdsmi.GetFwErrorRecords(processor)
		},
	)
	if err == nil {
		printFwErrorRecords(errorRecords)
	}
	fmt.Println()

	printVfFirmware(processor)
	return nil
}

// formatFirmwareVersion converts a raw firmware version to the format used by
// amd-smi.
func formatFirmwareVersion(fwID amdsmi.FwBlock, fwVersion uint64) string {
	// Firmware versions are returned as packed 32-bit values. Each firmware
	// block defines how those bits map to human-readable version components.
	fwVersion &= 0xFFFFFFFF

	switch fwID {
	// SMU uses four decimal bytes: major.minor.debug.test.
	case amdsmi.AMDSMI_FW_ID_SMU:
		return fmt.Sprintf(
			"%d.%d.%d.%d",
			(fwVersion>>24)&0xFF,
			(fwVersion>>16)&0xFF,
			(fwVersion>>8)&0xFF,
			fwVersion&0xFF,
		)
	// MMSCH reserves the lower 16 bits for the final version component.
	case amdsmi.AMDSMI_FW_ID_MMSCH:
		return fmt.Sprintf(
			"%d.%d.%d",
			(fwVersion>>24)&0xFF,
			(fwVersion>>16)&0xFF,
			fwVersion&0xFFFF,
		)
	// UVD uses fields of 2, 6, 8, and 8 bits; unused bits are ignored.
	case amdsmi.AMDSMI_FW_ID_UVD:
		return fmt.Sprintf(
			"%d.%d.%d.%d",
			(fwVersion>>30)&0x3,
			(fwVersion>>24)&0x3F,
			(fwVersion>>8)&0xFF,
			fwVersion&0xFF,
		)
	// VCE encodes two 12-bit components followed by one 8-bit component.
	case amdsmi.AMDSMI_FW_ID_VCE:
		return fmt.Sprintf(
			"%d.%d.%d",
			(fwVersion>>20)&0xFFF,
			(fwVersion>>8)&0xFFF,
			fwVersion&0xFF,
		)
	// VCN uses two 4-bit, one 8-bit, and one 12-bit decimal component.
	case amdsmi.AMDSMI_FW_ID_VCN:
		return fmt.Sprintf(
			"%d.%d.%d.%d",
			(fwVersion>>24)&0xF,
			(fwVersion>>20)&0xF,
			(fwVersion>>12)&0xFF,
			fwVersion&0xFFF,
		)
	}

	// PSP-related firmware reports four hexadecimal byte components.
	if pspHexFormatFwBlocks[fwID] {
		return fmt.Sprintf(
			"%x.%x.%x.%x",
			(fwVersion>>24)&0xFF,
			(fwVersion>>16)&0xFF,
			(fwVersion>>8)&0xFF,
			fwVersion&0xFF,
		)
	}
	// IMU and PLDM versions use the same four-byte decimal layout as SMU.
	if imuPldmFwBlocks[fwID] {
		return fmt.Sprintf(
			"%d.%d.%d.%d",
			(fwVersion>>24)&0xFF,
			(fwVersion>>16)&0xFF,
			(fwVersion>>8)&0xFF,
			fwVersion&0xFF,
		)
	}
	// RLC restore-list versions are displayed as unpadded hexadecimal values.
	if rlcRestoreListFwBlocks[fwID] {
		return fmt.Sprintf("0x%x", fwVersion)
	}
	// MES versions are always padded to the full eight hexadecimal digits.
	if mesFwBlocks[fwID] {
		return fmt.Sprintf("0x%08x", fwVersion)
	}
	// Preserve unknown or unlisted firmware versions as raw hexadecimal.
	return fmt.Sprintf("0x%x", fwVersion)
}

func fwBlockDisplayName(fwID amdsmi.FwBlock) string {
	return strings.TrimPrefix(fwID.String(), "FW_ID_")
}

// printFirmwareInfo prints nonzero entries from a GPU or VF firmware inventory.
func printFirmwareInfo(fwInfo amdsmi.FwInfo, indent string) {
	if fwInfo.NumFwInfo == 0 {
		fmt.Printf("%sNo firmware blocks found\n", indent)
		return
	}

	fmt.Printf("%sFW_LIST:\n", indent)
	firmwareNumber := 0
	for i := 0; i < int(fwInfo.NumFwInfo); i++ {
		firmware := fwInfo.FwList[i]
		// A zero version marks a firmware block that is not present.
		if firmware.FwVersion == 0 {
			continue
		}

		// Remove the enum prefix to make the output easier to scan.
		fwName := fwBlockDisplayName(firmware.FwID)
		fmt.Printf("%s  FW_%d:\n", indent, firmwareNumber)
		fmt.Printf("%s    FW_ID: %s\n", indent, fwName)
		fmt.Printf(
			"%s    FW_VERSION: %s\n",
			indent,
			formatFirmwareVersion(firmware.FwID, firmware.FwVersion),
		)
		firmwareNumber++
	}
}

// printFwErrorRecords prints firmware errors recorded by the GPU driver.
func printFwErrorRecords(errorRecords amdsmi.FwErrorRecord) {
	records := int(errorRecords.NumErrRecords)
	fmt.Printf("  number of firmware error records is: %d\n", records)
	if records == 0 {
		fmt.Println("  No firmware error records found")
		return
	}

	for recordNumber := 0; recordNumber < records; recordNumber++ {
		record := errorRecords.ErrRecords[recordNumber]
		fmt.Printf("  Firmware error record %d:\n", recordNumber)
		fmt.Printf("    timestamp is: %d\n", record.Timestamp)
		fmt.Printf("    vf_idx is: %d\n", record.VfIdx)
		fmt.Printf("    fw_id is: %s\n", amdsmi.FwBlock(record.FwID).String())
		fmt.Printf("    status is: %s\n", record.Status.String())
	}
}

// printVfFirmware shows firmware information for every enabled VF on a GPU.
func printVfFirmware(processor amdsmi.ProcessorHandle) {
	// VF firmware information requires SR-IOV VFs to be enabled.
	fmt.Println("###Test amdsmi.GetVfPartitionInfo")
	partitions, partitionErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetVfPartitionInfo",
		func() ([]amdsmi.PartitionInfo, error) {
			return amdsmi.GetVfPartitionInfo(processor)
		},
	)
	if partitionErr != nil {
		fmt.Println()
		return
	}

	fmt.Printf("  number of enabled VFs is: %d\n", len(partitions))
	if len(partitions) == 0 {
		fmt.Println("  No enabled VFs found; VF firmware queries require an enabled VF")
		fmt.Println()
		return
	}

	// A partition contains the VF handle required by the VF firmware API.
	for vfNumber, partition := range partitions {
		fmt.Println("###Test amdsmi.GetVfFwInfo")
		fwInfo, vfErr := example_utils.GetOptionalAPI(
			os.Stdout,
			fmt.Sprintf("amdsmi.GetVfFwInfo for VF %d", vfNumber),
			func() (amdsmi.FwInfo, error) {
				return amdsmi.GetVfFwInfo(partition.VfHandle)
			},
		)
		if vfErr == nil {
			fmt.Printf("  VF number: %d\n", vfNumber)
			if fwInfo.NumFwInfo == 0 {
				fmt.Println("  No firmware blocks found; the VM may not be started on this VF")
			} else {
				printFirmwareInfo(fwInfo, "  ")
			}
		}
		fmt.Println()
	}
}

// printNicFirmware selects one or more optional NICs and shows their firmware
// entries.
func printNicFirmware(ctx context.Context, prompter *example_utils.Prompter) error {
	// NIC support is optional and may require a supported NIC driver.
	fmt.Println("###Test amdsmi.GetNicProcessorHandles")
	nicProcessors, nicErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetNicProcessorHandles",
		amdsmi.GetNicProcessorHandles,
	)
	if nicErr != nil {
		fmt.Println()
		return nil
	}

	fmt.Printf("  number of NICs is: %d\n", len(nicProcessors))
	if len(nicProcessors) == 0 {
		fmt.Println("  No NICs found")
		fmt.Println()
		return nil
	}

	fmt.Println("  Available NICs:")
	for nicIndex, nic := range nicProcessors {
		nicBdf, err := amdsmi.GetNicDeviceBdf(nic)
		if err != nil {
			return fmt.Errorf("get BDF for NIC %d: %w", nicIndex, err)
		}
		fmt.Printf("  NIC %d bdf: %s\n", nicIndex, nicBdf)
	}

	allNicIndex := uint64(len(nicProcessors))
	fmt.Printf("  %d: All NICs\n", allNicIndex)

	// The index immediately after the last NIC is used as the "All NICs"
	// menu choice.
	pickNicIndex, err := prompter.PromptForInput(
		ctx,
		"  Enter the NIC index to inspect: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &allNicIndex,
			ErrorMessage: fmt.Sprintf(
				"  NIC index must be between 0 and %d (%d for all NICs)",
				len(nicProcessors)-1,
				allNicIndex,
			),
		},
	)
	if err != nil {
		return fmt.Errorf("select NIC index: %w", err)
	}

	selectedNicIndexes := make([]int, 0, len(nicProcessors))
	if pickNicIndex == allNicIndex {
		for nicIndex := range nicProcessors {
			selectedNicIndexes = append(selectedNicIndexes, nicIndex)
		}
	} else {
		selectedNicIndexes = append(selectedNicIndexes, int(pickNicIndex))
	}

	for _, nicIndex := range selectedNicIndexes {
		if err := ctx.Err(); err != nil {
			return err
		}
		if err := printSingleNicFirmware(nicIndex, nicProcessors[nicIndex]); err != nil {
			return err
		}
	}

	return nil
}

func printSingleNicFirmware(
	nicIndex int,
	nic amdsmi.ProcessorHandle,
) error {
	// Print the BDF so users can associate the firmware with a physical
	// NIC in the system.
	fmt.Println("###Test amdsmi.GetNicDeviceBdf")
	nicBdf, err := amdsmi.GetNicDeviceBdf(nic)
	if err != nil {
		return fmt.Errorf("get BDF for selected NIC %d: %w", nicIndex, err)
	}
	fmt.Printf("  Selected NIC %d bdf: %s\n\n", nicIndex, nicBdf)

	fmt.Println("###Test amdsmi.GetNicFwInfo")
	fwInfo, nicErr := example_utils.GetOptionalAPI(
		os.Stdout,
		fmt.Sprintf("amdsmi.GetNicFwInfo for NIC %d", nicIndex),
		func() (amdsmi.NicFwInfo, error) {
			return amdsmi.GetNicFwInfo(nic)
		},
	)
	if nicErr == nil {
		fmt.Printf("  NIC number: %d\n", nicIndex)
		if len(fwInfo.Fw) == 0 {
			fmt.Println("  No NIC firmware entries found")
		}
		for _, firmware := range fwInfo.Fw {
			fmt.Printf("  type is: %s\n", firmware.Type)
			fmt.Printf("  name is: %s\n", firmware.Fw.Name)
			fmt.Printf("  version is: %s\n", firmware.Fw.Version)
		}
	}
	fmt.Println()
	return nil
}

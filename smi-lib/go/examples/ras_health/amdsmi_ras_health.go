// Copyright Advanced Micro Devices, Inc.
//
// SPDX-License-Identifier: MIT

package main

import (
	"errors"
	"fmt"
	"math"
	"os"
	"strings"
	"time"

	"smi-lib/go/amdsmi"
	example_utils "smi-lib/go/examples/example_utils"
)

type eccCorrectionSchema struct {
	name string
	flag uint32
}

var eccCorrectionSchemas = []eccCorrectionSchema{
	{name: "PARITY", flag: 1 << 0},
	{name: "CORRECTABLE", flag: 1 << 1},
	{name: "UNCORRECTABLE", flag: 1 << 2},
	{name: "POISON", flag: 1 << 3},
}

func main() {
	err := runRasHealth()
	example_utils.HandleExampleError(err)
}

func runRasHealth() error {
	// AMDSMI_INIT_ALL_PROCESSORS is the default. Choose a different
	// InitFlags value when only a specific processor type is needed.
	initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS

	fmt.Println("###Test amdsmi.Init")
	fmt.Println("  Initializing with AMDSMI_INIT_ALL_PROCESSORS")
	if err := amdsmi.Init(initFlags); err != nil {
		return err
	}
	fmt.Println("  Initialization complete")

	// Always release AMD SMI resources, including after an API error or
	// keyboard interrupt.
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
		fmt.Println("  No GPUs found")
	}

	// RAS health is reported independently for every discovered GPU.
	for processorNumber, processor := range processors {
		if err := printGpuRasHealth(processorNumber, processor); err != nil {
			return err
		}
	}

	return nil
}

// ECC APIs return the same three counter types. Keep their formatting in one
// helper so total and per-block counts are presented consistently.
func printEccCount(errorCount amdsmi.ErrorCount) {
	fmt.Printf("  correctable_count is: %d\n", errorCount.CorrectableCount)
	fmt.Printf("  uncorrectable_count is: %d\n", errorCount.UncorrectableCount)
	fmt.Printf("  deferred_count is: %d\n", errorCount.DeferredCount)
}

// formatTimestamp converts the bad-page record's Unix timestamp to a readable
// local time.
func formatTimestamp(ts uint64) string {
	// A RAS record can contain a value the local platform cannot represent,
	// so preserve that raw value instead of terminating the example.
	if ts > uint64(math.MaxInt64) {
		return fmt.Sprintf("unavailable (raw value %d)", ts)
	}

	parsed := time.Unix(int64(ts), 0)
	if parsed.Year() < 1970 || parsed.Year() > 9999 {
		return fmt.Sprintf("unavailable (raw value %d)", ts)
	}

	return parsed.Format("2006/01/02 15:04:05")
}

func gpuBlockName(block amdsmi.GpuBlock) string {
	switch block {
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
	case amdsmi.AMDSMI_GPU_BLOCK_MMSCH:
		return "MMSCH"
	case amdsmi.AMDSMI_GPU_BLOCK_MP5:
		return "MP5"
	case amdsmi.AMDSMI_GPU_BLOCK_ATU:
		return "ATU"
	case amdsmi.AMDSMI_GPU_BLOCK_DACC_BE:
		return "DACC_BE"
	case amdsmi.AMDSMI_GPU_BLOCK_ECLR:
		return "ECLR"
	case amdsmi.AMDSMI_GPU_BLOCK_KPX_SERDES:
		return "KPX_SERDES"
	case amdsmi.AMDSMI_GPU_BLOCK_LSDMA:
		return "LSDMA"
	case amdsmi.AMDSMI_GPU_BLOCK_MPART:
		return "MPART"
	case amdsmi.AMDSMI_GPU_BLOCK_MPIFOE:
		return "MPIFOE"
	case amdsmi.AMDSMI_GPU_BLOCK_MPRAS:
		return "MPRAS"
	case amdsmi.AMDSMI_GPU_BLOCK_NBIF:
		return "NBIF"
	case amdsmi.AMDSMI_GPU_BLOCK_NBIO:
		return "NBIO"
	case amdsmi.AMDSMI_GPU_BLOCK_OXRP:
		return "OXRP"
	case amdsmi.AMDSMI_GPU_BLOCK_PCIE_PL:
		return "PCIE_PL"
	case amdsmi.AMDSMI_GPU_BLOCK_PCS_XGMI:
		return "PCS_XGMI"
	case amdsmi.AMDSMI_GPU_BLOCK_PIE:
		return "PIE"
	case amdsmi.AMDSMI_GPU_BLOCK_CS:
		return "CS"
	case amdsmi.AMDSMI_GPU_BLOCK_SHUB:
		return "SHUB"
	case amdsmi.AMDSMI_GPU_BLOCK_SSBDCI:
		return "SSBDCI"
	case amdsmi.AMDSMI_GPU_BLOCK_UCIE_PCS:
		return "UCIE_PCS"
	default:
		return fmt.Sprintf("0x%x", uint64(block))
	}
}

func apiErrorInfo(err error) string {
	var statusErr *amdsmi.StatusError
	if errors.As(err, &statusErr) {
		description, decodeErr := amdsmi.StatusCodeToString(statusErr.Code)
		if decodeErr == nil {
			return description
		}
		if statusErr.Name != "" {
			return statusErr.Name
		}
	}
	return err.Error()
}

func printGpuRasHealth(
	processorNumber int,
	processor amdsmi.ProcessorHandle,
) error {
	// Display the PCI BDF to identify the GPU associated with the
	// health information that follows.
	fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
	bdf, err := amdsmi.GetGpuDeviceBdf(processor)
	if err != nil {
		return err
	}
	fmt.Printf("  Processor number: %d bdf: %s\n\n", processorNumber, bdf)

	// Read the aggregate ECC counters across all GPU blocks.
	// Correctable errors were repaired, uncorrectable errors were not,
	// and deferred errors are pending further handling.
	fmt.Println("###Test amdsmi.GetGpuTotalEccCount")
	totalEccCount, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuTotalEccCount",
		func() (amdsmi.ErrorCount, error) {
			return amdsmi.GetGpuTotalEccCount(processor)
		},
	)
	if err == nil {
		printEccCount(totalEccCount)
	}
	fmt.Println()

	// Determine which hardware blocks have ECC reporting enabled.
	// This result also controls the per-block queries below.
	fmt.Println("###Test amdsmi.GetGpuEccEnabled")
	eccEnabled, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuEccEnabled",
		func() (map[amdsmi.GpuBlock]bool, error) {
			return amdsmi.GetGpuEccEnabled(processor)
		},
	)
	if err == nil {
		for block := amdsmi.AMDSMI_GPU_BLOCK_FIRST; block <= amdsmi.AMDSMI_GPU_BLOCK_LAST; block <<= 1 {
			state := "disabled"
			if eccEnabled[block] {
				state = "enabled"
			}
			fmt.Printf("  ECC for %s is: %s\n", gpuBlockName(block), state)
		}
	}
	fmt.Println()

	// Query individual block counters only where ECC is enabled.
	// This helps identify the source of errors included in the total count.
	fmt.Println("###Test amdsmi.GetGpuEccCount")
	if err == nil {
		enabledBlocks := make([]amdsmi.GpuBlock, 0)
		for block := amdsmi.AMDSMI_GPU_BLOCK_FIRST; block <= amdsmi.AMDSMI_GPU_BLOCK_LAST; block <<= 1 {
			if eccEnabled[block] {
				enabledBlocks = append(enabledBlocks, block)
			}
		}
		if len(enabledBlocks) == 0 {
			fmt.Println("  ECC is not enabled for any GPU block")
		}
		// A block can have ECC enabled without supporting a counter
		// read, so group the failures and report them after the loop
		// instead of once per block. This improves the readability of the output.
		failedBlocks := make(map[string][]string)
		failedOrder := make([]string, 0)
		for _, block := range enabledBlocks {
			blockEccCount, blockErr := amdsmi.GetGpuEccCount(processor, block)
			if blockErr != nil {
				errInfo := apiErrorInfo(blockErr)
				if _, exists := failedBlocks[errInfo]; !exists {
					failedOrder = append(failedOrder, errInfo)
				}
				failedBlocks[errInfo] = append(failedBlocks[errInfo], gpuBlockName(block))
				continue
			}
			fmt.Printf("  %s ECC errors:\n", gpuBlockName(block))
			printEccCount(blockEccCount)
		}
		for _, errInfo := range failedOrder {
			fmt.Printf(
				"  amdsmi.GetGpuEccCount failed for %s: %s\n",
				strings.Join(failedBlocks[errInfo], ", "),
				errInfo,
			)
		}
	}
	fmt.Println()

	// Read the RAS EEPROM version and the correction mechanisms the
	// device advertises. ecc_correction_schema is a bitmask, so test
	// each schema flag to produce a readable list of supported schemas.
	fmt.Println("###Test amdsmi.GetGpuRasFeatureInfo")
	rasFeature, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuRasFeatureInfo",
		func() (amdsmi.RasFeatureInfo, error) {
			return amdsmi.GetGpuRasFeatureInfo(processor)
		},
	)
	if err == nil {
		fmt.Printf("  ras_eeprom_version is: %d\n", rasFeature.RasEepromVersion)
		fmt.Printf("  ecc_correction_schema is: %d\n", rasFeature.EccCorrectionSchemaFlag)
		supportedSchemas := make([]string, 0)
		for _, schema := range eccCorrectionSchemas {
			if rasFeature.EccCorrectionSchemaFlag&schema.flag != 0 {
				supportedSchemas = append(supportedSchemas, schema.name)
			}
		}
		fmt.Printf("  supported ECC correction schemas are: %v\n\n", supportedSchemas)
	}
	fmt.Println()

	// Read the versioned RAS policy stored by supported PMFW-managed
	// RAS EEPROM implementations.
	fmt.Println("###Test amdsmi.GetGpuRasPolicyInfo")
	rasPolicy, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuRasPolicyInfo",
		func() (amdsmi.RasPolicyInfo, error) {
			return amdsmi.GetGpuRasPolicyInfo(processor)
		},
	)
	if err == nil {
		fmt.Printf("  major_version is: %d\n", rasPolicy.MajorVersion)
		fmt.Printf("  minor_version is: %d\n", rasPolicy.MinorVersion)
		if rasPolicy.V4_0 != nil {
			fmt.Printf(
				"  dram_non_critical_region_threshold is: %d\n",
				rasPolicy.V4_0.DramNonCriticalRegionThreshold,
			)
			fmt.Printf(
				"  dram_critical_region_threshold is: %d\n",
				rasPolicy.V4_0.DramCriticalRegionThreshold,
			)
		}
	}
	fmt.Println()

	// Read the number of retired memory pages at which the device is
	// considered unhealthy. A retired page is no longer available for
	// GPU memory allocations.
	fmt.Println("###Test amdsmi.GetBadPageThreshold")
	badPageThreshold, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetBadPageThreshold",
		func() (uint32, error) {
			return amdsmi.GetBadPageThreshold(processor)
		},
	)
	if err == nil {
		fmt.Printf("  bad page threshold is: %d\n", badPageThreshold)
	}
	fmt.Println()

	// List every retired memory page. In addition to its address, each
	// record identifies when and where the page failure was recorded.
	fmt.Println("###Test amdsmi.GetGpuBadPageInfo")
	badPages, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuBadPageInfo",
		func() ([]amdsmi.EepromTableRecord, error) {
			return amdsmi.GetGpuBadPageInfo(processor)
		},
	)
	if err == nil {
		if len(badPages) == 0 {
			fmt.Println("  No bad pages found")
		}
		for _, badPage := range badPages {
			fmt.Printf("  retired_page is: %#x\n", badPage.RetiredPage)
			fmt.Printf("  timestamp is: %s\n", formatTimestamp(badPage.Ts))
			fmt.Printf("  mem_channel is: %d\n", badPage.MemChannel)
			fmt.Printf("  mcumc_id is: %d\n", badPage.McumcID)
		}
	}
	fmt.Println()

	return nil
}

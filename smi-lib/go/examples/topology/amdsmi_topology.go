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
	"strconv"
	"strings"
	"syscall"

	"smi-lib/go/amdsmi"
	example_utils "smi-lib/go/examples/example_utils"
)

var xgmiFbSharingModes = []amdsmi.XgmiFbSharingMode{
	amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM,
	amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_1,
	amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_2,
	amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_4,
	amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_8,
}

func main() {
	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	err := runTopology(ctx)
	stop()

	example_utils.HandleExampleError(err)
}

func runTopology(ctx context.Context) (err error) {
	prompter := example_utils.NewPrompter(os.Stdin, os.Stdout)

	// INIT_ALL_PROCESSORS is needed to discover GPUs and optional NICs.
	initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS

	fmt.Println("###Test amdsmi.Init")
	fmt.Println("  Initializing with AMDSMI_INIT_ALL_PROCESSORS")
	if err = amdsmi.Init(initFlags); err != nil {
		return fmt.Errorf("initialize AMD SMI: %w", err)
	}
	fmt.Println("  Initialization complete")

	// Always release AMD SMI resources, including after an API error or
	// keyboard interrupt.
	defer func() {
		fmt.Println("###Test amdsmi.ShutDown")
		if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
			if err != nil && !errors.Is(err, context.Canceled) {
				fmt.Println(shutdownErr)
			} else {
				err = fmt.Errorf("shut down AMD SMI: %w", shutdownErr)
			}
			return
		}
		fmt.Println("Shutdown complete")
	}()

	// Discover GPU handles. Each handle is passed to the topology APIs
	// below and remains valid until amdsmi.ShutDown is called.
	fmt.Println("###Test amdsmi.GetProcessorHandles")
	processors, err := amdsmi.GetProcessorHandles()
	if err != nil {
		return fmt.Errorf("get processor handles: %w", err)
	}
	fmt.Printf("  number of GPUs is: %d\n\n", len(processors))

	if len(processors) == 0 {
		fmt.Println("  No GPUs found")
		return nil
	}

	// Operation 0 only reads information. Operations 1 and 2 change
	// GPU configuration and ask for confirmation before applying.
	fmt.Println("  Available operations:")
	fmt.Println("  0: Display topology and interconnect information")
	fmt.Println("  1: Set XGMI framebuffer sharing mode v2")
	fmt.Println("  2: Set XGMI per-link power down policy")
	fmt.Println("  3: Exit")

	operationUpperBound := uint64(3)
	operationNumber, promptErr := prompter.PromptForInput(
		ctx,
		"  Enter the operation to perform: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &operationUpperBound,
			ErrorMessage: "  Operation index must be between 0 and 3",
		},
	)
	if promptErr != nil {
		return fmt.Errorf("select operation: %w", promptErr)
	}

	switch operationNumber {
	case 0:
		// Read-only queries can run on one GPU or on all GPUs.
		selectedGpuIndexes, selectErr := promptGpuIndexes(
			ctx,
			prompter,
			processors,
			true,
		)
		if selectErr != nil {
			return selectErr
		}
		if displayErr := displayTopology(
			processors,
			selectedGpuIndexes,
		); displayErr != nil {
			return displayErr
		}
	case 3:
		fmt.Println("  No operation was performed.")
	default:
		// Set operations act on a single GPU, so "All GPUs" is not
		// offered.
		selectedGpuIndexes, selectErr := promptGpuIndexes(
			ctx,
			prompter,
			processors,
			false,
		)
		if selectErr != nil {
			return selectErr
		}
		processorNumber := selectedGpuIndexes[0]
		processor := processors[processorNumber]

		fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
		bdf, bdfErr := amdsmi.GetGpuDeviceBdf(processor)
		if bdfErr != nil {
			return fmt.Errorf("get BDF for selected GPU %d: %w", processorNumber, bdfErr)
		}
		fmt.Printf("  Selected GPU %d bdf: %s\n\n", processorNumber, bdf)

		switch operationNumber {
		case 1:
			setXgmiFbSharingModeV2(ctx, prompter, processors, processorNumber)
		case 2:
			setXgmiPlpd(ctx, prompter, processor)
		}
	}

	return nil
}

// printLinkMetrics prints per-link metrics (type, status, speed, traffic) for
// one GPU.
func printLinkMetrics(processor amdsmi.ProcessorHandle, processorNumber int) {
	fmt.Println("###Test amdsmi.GetLinkMetrics")
	// Link metrics are optional, so an unsupported system prints a message
	// instead of stopping the example.
	linkMetrics, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetLinkMetrics",
		func() (amdsmi.LinkMetrics, error) {
			return amdsmi.GetLinkMetrics(processor)
		},
	)
	if err == nil {
		links := linkMetrics.Links
		fmt.Printf("  GPU %d has %d link(s)\n", processorNumber, len(links))
		if len(links) == 0 {
			fmt.Println("  No link metrics found")
		}

		// Each entry describes one link from this GPU to a peer, identified
		// by the peer's BDF. Read and Write are cumulative traffic counters.
		for linkNumber, link := range links {
			fmt.Printf("  Link %d:\n", linkNumber)
			fmt.Printf("    bdf is: %s\n", link.Bdf)
			fmt.Printf("    link_type is: %s\n", link.LinkType)
			fmt.Printf("    link_status is: %s\n", link.LinkStatus)
			fmt.Printf("    bit_rate is: %d Gb/s\n", link.BitRate)
			fmt.Printf("    max_bandwidth is: %d\n", link.MaxBandwidth)
			fmt.Printf("    read is: %d KB\n", link.Read)
			fmt.Printf("    write is: %d KB\n", link.Write)
		}
	}
	fmt.Println()
}

// printNearestProcessors prints the GPUs closest to this GPU over XGMI and
// over PCIe.
func printNearestProcessors(processor amdsmi.ProcessorHandle) {
	fmt.Println("###Test amdsmi.GetLinkTopologyNearest")
	// The link type selects which interconnect to search. The result is
	// the GPUs of that type that share the smallest hop count.
	// XGMI peers can be more than one hop away.
	// The PCIe result is only the closest PCIe peers.
	linkTypes := []amdsmi.LinkType{
		amdsmi.AMDSMI_LINK_TYPE_XGMI,
		amdsmi.AMDSMI_LINK_TYPE_PCIE,
	}
	for _, linkType := range linkTypes {
		nearest, err := amdsmi.GetLinkTopologyNearest(processor, linkType)
		if err != nil {
			// Report the failure for this link type and continue with the
			// next one.
			example_utils.PrintOptionalAPIError(
				os.Stdout,
				fmt.Sprintf("amdsmi.GetLinkTopologyNearest for %s", linkType),
				err,
			)
			continue
		}

		// The API returns processor handles. Convert them to BDFs so the
		// output identifies physical devices.
		nearestBdfs := make([]amdsmi.Bdf, 0, len(nearest.ProcessorList))
		bdfLookupFailed := false
		for _, nearestProcessor := range nearest.ProcessorList {
			bdf, bdfErr := amdsmi.GetGpuDeviceBdf(nearestProcessor)
			if bdfErr != nil {
				example_utils.PrintOptionalAPIError(
					os.Stdout,
					fmt.Sprintf("amdsmi.GetLinkTopologyNearest for %s", linkType),
					bdfErr,
				)
				bdfLookupFailed = true
				break
			}
			nearestBdfs = append(nearestBdfs, bdf)
		}
		if bdfLookupFailed {
			continue
		}

		displayBdfs := "none"
		if len(nearestBdfs) > 0 {
			displayBdfs = fmt.Sprintf("%v", nearestBdfs)
		}
		fmt.Printf("  Nearest %s GPU BDFs are: %s\n", linkType, displayBdfs)
	}
	fmt.Println()
}

// printHostAffinity prints the NUMA node and host CPU cores closest to this
// GPU.
func printHostAffinity(processor amdsmi.ProcessorHandle) {
	// The NUMA node identifies the host memory domain the GPU is attached
	// to. Placing work on CPUs and memory in the same node avoids
	// cross-node traffic.
	fmt.Println("###Test amdsmi.TopoGetNumaNodeNumber")
	numaNode, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.TopoGetNumaNodeNumber",
		func() (uint32, error) {
			return amdsmi.TopoGetNumaNodeNumber(processor)
		},
	)
	if err == nil {
		fmt.Printf("  NUMA node is: %d\n", numaNode)
	}
	fmt.Println()

	// CPU affinity is a list of bitmask words.
	// Each set bit is a logical CPU from the GPU's NUMA-node local_cpu_list.
	fmt.Println("###Test amdsmi.GetCpuAffinityWithScope")
	affinityScopes := []struct {
		name  string
		scope amdsmi.AffinityScope
	}{
		{"NUMA_SCOPE", amdsmi.AMDSMI_AFFINITY_SCOPE_NODE},
		{"SOCKET_SCOPE", amdsmi.AMDSMI_AFFINITY_SCOPE_SOCKET},
	}
	for _, affinityScope := range affinityScopes {
		affinity, err := example_utils.GetOptionalAPI(
			os.Stdout,
			fmt.Sprintf(
				"amdsmi.GetCpuAffinityWithScope for %s",
				affinityScope.name,
			),
			func() ([]uint64, error) {
				return amdsmi.GetCpuAffinityWithScope(processor, affinityScope.scope)
			},
		)
		if err == nil {
			hexWords := make([]string, len(affinity))
			for i, word := range affinity {
				hexWords[i] = fmt.Sprintf("0x%x", word)
			}
			fmt.Printf(
				"  %s CPU affinity bitmask is: %v\n",
				affinityScope.name,
				hexWords,
			)
		}
	}
	fmt.Println()
}

// getXgmiModes returns the XGMI framebuffer sharing modes reported as
// supported.
func getXgmiModes(caps amdsmi.XgmiFbSharingCaps) []amdsmi.XgmiFbSharingMode {
	// Map each capability flag to the mode enum it describes. CUSTOM is left
	// out because it cannot be queried by amdsmi.GetXgmiFbSharingModeInfo.
	modes := make([]amdsmi.XgmiFbSharingMode, 0, 4)
	if caps.Mode1Cap != 0 {
		modes = append(modes, amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_1)
	}
	if caps.Mode2Cap != 0 {
		modes = append(modes, amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_2)
	}
	if caps.Mode4Cap != 0 {
		modes = append(modes, amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_4)
	}
	if caps.Mode8Cap != 0 {
		modes = append(modes, amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_8)
	}
	return modes
}

// printXgmiFbSharingCaps prints XGMI framebuffer sharing capabilities and
// returns supported modes.
func printXgmiFbSharingCaps(
	processor amdsmi.ProcessorHandle,
) []amdsmi.XgmiFbSharingMode {
	// XGMI framebuffer sharing lets GPUs connected over XGMI access each
	// other's framebuffer memory. Each capability flag reports whether the
	// GPU supports the corresponding sharing mode.
	fmt.Println("###Test amdsmi.GetXgmiFbSharingCaps")
	caps, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetXgmiFbSharingCaps",
		func() (amdsmi.XgmiFbSharingCaps, error) {
			return amdsmi.GetXgmiFbSharingCaps(processor)
		},
	)
	if err != nil {
		fmt.Println()
		// An empty list tells callers that there are no modes to query.
		return nil
	}

	fmt.Printf("  mode_custom_cap is: %d\n", caps.ModeCustomCap)
	fmt.Printf("  mode_1_cap is: %d\n", caps.Mode1Cap)
	fmt.Printf("  mode_2_cap is: %d\n", caps.Mode2Cap)
	fmt.Printf("  mode_4_cap is: %d\n", caps.Mode4Cap)
	fmt.Printf("  mode_8_cap is: %d\n", caps.Mode8Cap)
	fmt.Println()
	// Return the supported modes so the pair-topology section only queries
	// sharing state for modes this GPU supports.
	return getXgmiModes(caps)
}

// printXgmiPlpd prints the XGMI per-link power down (PLPD) policies for a GPU.
func printXgmiPlpd(processor amdsmi.ProcessorHandle) (amdsmi.DpmPolicy, bool) {
	// PLPD controls when individual XGMI links may be powered down to save
	// power. The API returns the list of available policies and the index of
	// the policy that is currently active.
	// Every GPU on the same socket reports the same policy.
	fmt.Println("###Test amdsmi.GetXgmiPlpd")
	plpd, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetXgmiPlpd",
		func() (amdsmi.DpmPolicy, error) {
			return amdsmi.GetXgmiPlpd(processor)
		},
	)
	if err == nil {
		// Current is an index into the Policies list, not a policy ID.
		fmt.Printf("  Current XGMI PLPD policy index is: %d\n", plpd.Current)
		if len(plpd.Policies) == 0 {
			fmt.Println("  No XGMI PLPD policies found")
		}
		for policyIndex, policy := range plpd.Policies {
			current := ""
			if uint32(policyIndex) == plpd.Current {
				current = " (current)"
			}
			fmt.Printf(
				"  Policy index %d (ID %d) is: %s%s\n",
				policyIndex,
				policy.PolicyID,
				policy.PolicyDescription,
				current,
			)
		}
	}
	fmt.Println()
	// Return the result so setXgmiPlpd() can offer the same policy list.
	return plpd, err != nil
}

// promptGpuIndexes asks the user to pick one GPU, or all GPUs when includeAll
// is true.
func promptGpuIndexes(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processors []amdsmi.ProcessorHandle,
	includeAll bool,
) ([]int, error) {
	// Display each GPU's BDF so the user can identify the physical device
	// before choosing it.
	fmt.Println("  Available GPUs:")
	for gpuIndex, processor := range processors {
		bdf, bdfErr := amdsmi.GetGpuDeviceBdf(processor)
		if bdfErr != nil {
			return nil, fmt.Errorf("get BDF for GPU %d: %w", gpuIndex, bdfErr)
		}
		fmt.Printf("  GPU %d bdf: %s\n", gpuIndex, bdf)
	}

	gpuUpperBound := uint64(len(processors) - 1)
	errorMessage := fmt.Sprintf(
		"  GPU index must be between 0 and %d",
		len(processors)-1,
	)
	if includeAll {
		// The index immediately after the last GPU is used as the
		// "All GPUs" menu choice.
		gpuUpperBound = uint64(len(processors))
		fmt.Printf("  %d: All GPUs\n", gpuUpperBound)
		errorMessage = fmt.Sprintf(
			"  GPU index must be between 0 and %d (%d for all GPUs)",
			len(processors)-1,
			gpuUpperBound,
		)
	}

	pickGpuIndex, promptErr := prompter.PromptForInput(
		ctx,
		"  Enter the GPU index to operate on: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &gpuUpperBound,
			ErrorMessage: errorMessage,
		},
	)
	if promptErr != nil {
		return nil, fmt.Errorf("select GPU index: %w", promptErr)
	}

	// Always return a list so callers handle one GPU and all GPUs the
	// same way.
	if includeAll && pickGpuIndex == gpuUpperBound {
		selectedGpuIndexes := make([]int, 0, len(processors))
		for gpuIndex := range processors {
			selectedGpuIndexes = append(selectedGpuIndexes, gpuIndex)
		}
		return selectedGpuIndexes, nil
	}

	return []int{int(pickGpuIndex)}, nil
}

// printGpuTopologyInfo prints per-GPU topology information for each selected
// GPU.
func printGpuTopologyInfo(
	processors []amdsmi.ProcessorHandle,
	selectedGpuIndexes []int,
) (map[int][]amdsmi.XgmiFbSharingMode, error) {
	selected := make(map[int]bool, len(selectedGpuIndexes))
	for _, gpuIndex := range selectedGpuIndexes {
		selected[gpuIndex] = true
	}

	// Supported XGMI sharing modes, keyed by GPU index. They are collected
	// here and reused later by printPairTopology().
	supportedXgmiModes := make(map[int][]amdsmi.XgmiFbSharingMode)
	for processorNumber, processor := range processors {
		if !selected[processorNumber] {
			continue
		}

		// Print the BDF so the following output can be associated with a
		// physical GPU.
		fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
		bdf, err := amdsmi.GetGpuDeviceBdf(processor)
		if err != nil {
			return nil, fmt.Errorf("get BDF for GPU %d: %w", processorNumber, err)
		}
		fmt.Printf("  Processor number: %d bdf: %s\n\n", processorNumber, bdf)

		printLinkMetrics(processor, processorNumber)
		printNearestProcessors(processor)
		printHostAffinity(processor)
		supportedXgmiModes[processorNumber] = printXgmiFbSharingCaps(processor)
		printXgmiPlpd(processor)
	}
	return supportedXgmiModes, nil
}

// getTopologyCell returns a short summary of the link between two GPUs for
// the matrix.
func getTopologyCell(
	processorSrc amdsmi.ProcessorHandle,
	processorDst amdsmi.ProcessorHandle,
) string {
	topology, err := amdsmi.GetLinkTopology(processorSrc, processorDst)
	if err != nil {
		// If a pair cannot be queried, mark the cell instead of aborting the
		// whole matrix.
		return "unavailable"
	}

	// Each cell is formatted as type:status/hops:fb=on|off,
	// for example XGMI:UP/1hop:fb=on.
	fbSharing := "off"
	if topology.FbSharing != 0 {
		fbSharing = "on"
	}
	return fmt.Sprintf(
		"%s:%s/%dhop:fb=%s",
		topology.LinkType,
		topology.LinkStatus,
		topology.NumHops,
		fbSharing,
	)
}

// printLinkTopologyMatrix prints an NxN matrix of link topology between GPUs.
func printLinkTopologyMatrix(
	processors []amdsmi.ProcessorHandle,
	selectedGpuIndexes []int,
) {
	selected := make(map[int]bool, len(selectedGpuIndexes))
	for _, gpuIndex := range selectedGpuIndexes {
		selected[gpuIndex] = true
	}

	fmt.Println("###Test amdsmi.GetLinkTopology")
	fmt.Println("  NxN link matrix (type:status/hops:framebuffer-sharing):")
	// Columns always include every GPU, including the source.
	// The diagonal cell is that GPU queried against itself.
	headers := make([]string, len(processors))
	for i := range processors {
		headers[i] = fmt.Sprintf("GPU %d", i)
	}
	fmt.Printf("  Source\\Destination: %s\n", strings.Join(headers, " | "))

	// Rows are printed only for the GPUs the user selected.
	for sourceNumber, sourceProcessor := range processors {
		if !selected[sourceNumber] {
			continue
		}
		cells := make([]string, len(processors))
		for destinationNumber, destinationProcessor := range processors {
			cells[destinationNumber] = getTopologyCell(
				sourceProcessor,
				destinationProcessor,
			)
		}
		fmt.Printf("  GPU %d: %s\n", sourceNumber, strings.Join(cells, " | "))
	}
	fmt.Println()
}

// printPairTopology prints link type, P2P capabilities, and XGMI sharing for
// GPU pairs.
func printPairTopology(
	processors []amdsmi.ProcessorHandle,
	supportedXgmiModes map[int][]amdsmi.XgmiFbSharingMode,
	selectedGpuIndexes []int,
) {
	// Every query in this section needs a source and a different
	// destination GPU.
	if len(processors) < 2 {
		fmt.Println("  GPU-to-GPU topology queries require at least two GPUs.\n")
		return
	}

	selected := make(map[int]bool, len(selectedGpuIndexes))
	for _, gpuIndex := range selectedGpuIndexes {
		selected[gpuIndex] = true
	}

	// Link type reports how two GPUs are connected (for example XGMI or
	// PCIe) and how many hops separate them.
	fmt.Println("###Test amdsmi.TopoGetLinkType")
	for sourceNumber, sourceProcessor := range processors {
		if !selected[sourceNumber] {
			continue
		}
		for destinationNumber, destinationProcessor := range processors {
			// Skip querying a GPU against itself.
			if sourceNumber == destinationNumber {
				continue
			}
			linkInfo, linkInfoErr := example_utils.GetOptionalAPI(
				os.Stdout,
				fmt.Sprintf(
					"amdsmi.TopoGetLinkType from GPU %d to GPU %d",
					sourceNumber,
					destinationNumber,
				),
				func() (struct {
					hops     uint64
					linkType amdsmi.LinkType
				}, error) {
					hops, linkType, err := amdsmi.TopoGetLinkType(
						sourceProcessor,
						destinationProcessor,
					)
					return struct {
						hops     uint64
						linkType amdsmi.LinkType
					}{hops: hops, linkType: linkType}, err
				},
			)
			if linkInfoErr == nil {
				fmt.Printf(
					"  GPU %d to GPU %d: type=%s hops=%d\n",
					sourceNumber,
					destinationNumber,
					linkInfo.linkType,
					linkInfo.hops,
				)
			}
		}
	}
	fmt.Println()

	// P2P status reports the link type and the IO-link capabilities:
	// coherency, 32-bit and 64-bit atomics, DMA, and whether the link is
	// bidirectional.
	fmt.Println("###Test amdsmi.TopoGetP2pStatus")
	for sourceNumber, sourceProcessor := range processors {
		if !selected[sourceNumber] {
			continue
		}
		for destinationNumber, destinationProcessor := range processors {
			if sourceNumber == destinationNumber {
				continue
			}
			p2pStatus, p2pStatusErr := example_utils.GetOptionalAPI(
				os.Stdout,
				fmt.Sprintf(
					"amdsmi.TopoGetP2pStatus from GPU %d to GPU %d",
					sourceNumber,
					destinationNumber,
				),
				func() (struct {
					linkType amdsmi.LinkType
					cap      amdsmi.P2pCapability
				}, error) {
					linkType, cap, err := amdsmi.TopoGetP2pStatus(
						sourceProcessor,
						destinationProcessor,
					)
					return struct {
						linkType amdsmi.LinkType
						cap      amdsmi.P2pCapability
					}{linkType: linkType, cap: cap}, err
				},
			)
			if p2pStatusErr == nil {
				fmt.Printf(
					"  GPU %d to GPU %d: type=%s\n",
					sourceNumber,
					destinationNumber,
					p2pStatus.linkType,
				)
				fmt.Printf(
					"    coherent=%d atomics_32bit=%d atomics_64bit=%d dma=%d bi_directional=%d\n",
					p2pStatus.cap.IsIolinkCoherent,
					p2pStatus.cap.IsIolinkAtomics32bit,
					p2pStatus.cap.IsIolinkAtomics64bit,
					p2pStatus.cap.IsIolinkDma,
					p2pStatus.cap.IsIolinkBiDirectional,
				)
			}
		}
	}
	fmt.Println()

	// For each supported sharing mode, report whether the source GPU shares
	// its framebuffer with the destination GPU in that mode.
	fmt.Println("###Test amdsmi.GetXgmiFbSharingModeInfo")
	for sourceNumber, sourceProcessor := range processors {
		if !selected[sourceNumber] {
			continue
		}
		// Only query modes the source GPU reported as supported in
		// printXgmiFbSharingCaps().
		modes := supportedXgmiModes[sourceNumber]
		if len(modes) == 0 {
			fmt.Printf(
				"  GPU %d reports no queryable XGMI framebuffer sharing modes\n",
				sourceNumber,
			)
			continue
		}

		for destinationNumber, destinationProcessor := range processors {
			if sourceNumber == destinationNumber {
				continue
			}
			for _, mode := range modes {
				fbSharing, fbSharingErr := example_utils.GetOptionalAPI(
					os.Stdout,
					fmt.Sprintf(
						"amdsmi.GetXgmiFbSharingModeInfo from GPU %d to GPU %d with %s",
						sourceNumber,
						destinationNumber,
						mode,
					),
					func() (uint8, error) {
						return amdsmi.GetXgmiFbSharingModeInfo(
							sourceProcessor,
							destinationProcessor,
							mode,
						)
					},
				)
				if fbSharingErr == nil {
					state := "disabled"
					if fbSharing != 0 {
						state = "enabled"
					}
					fmt.Printf(
						"  GPU %d to GPU %d with %s: %s\n",
						sourceNumber,
						destinationNumber,
						mode,
						state,
					)
				}
			}
		}
	}
	fmt.Println()
}

// printNicTopology prints each NIC's BDF, NUMA placement, and link type to
// every GPU.
func printNicTopology(processors []amdsmi.ProcessorHandle) {
	// NIC discovery and topology are optional. A lack of NIC support does not
	// prevent the GPU-only topology section from being useful.
	fmt.Println("###Test amdsmi.GetNicProcessorHandles")
	nicProcessors, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetNicProcessorHandles",
		amdsmi.GetNicProcessorHandles,
	)
	if err != nil {
		fmt.Println()
		return
	}

	fmt.Printf("  number of NICs is: %d\n", len(nicProcessors))
	if len(nicProcessors) == 0 {
		fmt.Println("  No NICs found\n")
		return
	}

	for nicNumber, nicProcessor := range nicProcessors {
		// Print the BDF so users can associate the output with a physical
		// NIC in the system.
		fmt.Println("###Test amdsmi.GetNicDeviceBdf")
		bdf, bdfErr := example_utils.GetOptionalAPI(
			os.Stdout,
			"amdsmi.GetNicDeviceBdf",
			func() (amdsmi.Bdf, error) {
				return amdsmi.GetNicDeviceBdf(nicProcessor)
			},
		)
		if bdfErr == nil {
			fmt.Printf("  NIC number: %d bdf: %s\n", nicNumber, bdf)
		}

		// NUMA info gives the NIC's NUMA node and the mask of CPU cores that
		// are local to it.
		fmt.Println("###Test amdsmi.GetNicNumaInfo")
		numaInfo, numaInfoErr := example_utils.GetOptionalAPI(
			os.Stdout,
			"amdsmi.GetNicNumaInfo",
			func() (amdsmi.NicNumaInfo, error) {
				return amdsmi.GetNicNumaInfo(nicProcessor)
			},
		)
		if numaInfoErr == nil {
			fmt.Printf("  node is: %d\n", numaInfo.Node)
			fmt.Printf("  affinity is: %s\n", numaInfo.Affinity)
		}

		// For NIC-to-GPU queries the link type is PCIE (same PCIe switch),
		// NUMA (same NUMA node, different switch), XNUMA (different NUMA
		// nodes), or UNKNOWN. The hops value is not meaningful for NICs and
		// is reported as UINT64_MAX.
		fmt.Println("###Test amdsmi.TopoGetLinkType")
		for gpuNumber, gpuProcessor := range processors {
			linkInfo, linkInfoErr := example_utils.GetOptionalAPI(
				os.Stdout,
				fmt.Sprintf(
					"amdsmi.TopoGetLinkType from NIC %d to GPU %d",
					nicNumber,
					gpuNumber,
				),
				func() (struct {
					hops     uint64
					linkType amdsmi.LinkType
				}, error) {
					hops, linkType, err := amdsmi.TopoGetLinkType(
						nicProcessor,
						gpuProcessor,
					)
					return struct {
						hops     uint64
						linkType amdsmi.LinkType
					}{hops: hops, linkType: linkType}, err
				},
			)
			if linkInfoErr == nil {
				fmt.Printf(
					"  NIC %d to GPU %d: type=%s hops=%d\n",
					nicNumber,
					gpuNumber,
					linkInfo.linkType,
					linkInfo.hops,
				)
			}
		}
		fmt.Println()
	}
}

// displayTopology runs every read-only topology query for the selected GPUs.
func displayTopology(
	processors []amdsmi.ProcessorHandle,
	selectedGpuIndexes []int,
) error {
	// Per-GPU information comes first because it also collects the
	// supported XGMI sharing modes used by the pair-topology section.
	supportedXgmiModes, err := printGpuTopologyInfo(processors, selectedGpuIndexes)
	if err != nil {
		return err
	}
	printLinkTopologyMatrix(processors, selectedGpuIndexes)
	printPairTopology(processors, supportedXgmiModes, selectedGpuIndexes)
	printNicTopology(processors)
	return nil
}

// promptXgmiMode asks the user to pick an XGMI framebuffer sharing mode.
func promptXgmiMode(
	ctx context.Context,
	prompter *example_utils.Prompter,
) (amdsmi.XgmiFbSharingMode, error) {
	// Offer every mode defined by the enum. Use the capabilities printed by
	// operation 0 to see which modes the GPU supports.
	fmt.Println("  Available XGMI framebuffer sharing modes:")
	for modeNumber, mode := range xgmiFbSharingModes {
		fmt.Printf("  %d: %s\n", modeNumber, mode)
	}

	modeUpperBound := uint64(len(xgmiFbSharingModes) - 1)
	modeNumber, err := prompter.PromptForInput(
		ctx,
		"  Enter the XGMI framebuffer sharing mode: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &modeUpperBound,
			ErrorMessage: fmt.Sprintf(
				"  Mode index must be between 0 and %d",
				len(xgmiFbSharingModes)-1,
			),
		},
	)
	if err != nil {
		return amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_UNKNOWN, err
	}
	return xgmiFbSharingModes[modeNumber], nil
}

// printXgmiSharingVerification reads back XGMI sharing state from one GPU to
// every other GPU.
func printXgmiSharingVerification(
	processors []amdsmi.ProcessorHandle,
	sourceNumber int,
	mode amdsmi.XgmiFbSharingMode,
) {
	sourceProcessor := processors[sourceNumber]
	// Sharing mode info can only be queried for the fixed MODE_X values,
	// so skip this step for CUSTOM.
	if mode != amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM {
		fmt.Println("###Test amdsmi.GetXgmiFbSharingModeInfo")
		for destinationNumber, destinationProcessor := range processors {
			if sourceNumber == destinationNumber {
				continue
			}
			fbSharing, fbSharingErr := example_utils.GetOptionalAPI(
				os.Stdout,
				fmt.Sprintf(
					"amdsmi.GetXgmiFbSharingModeInfo from GPU %d to GPU %d",
					sourceNumber,
					destinationNumber,
				),
				func() (uint8, error) {
					return amdsmi.GetXgmiFbSharingModeInfo(
						sourceProcessor,
						destinationProcessor,
						mode,
					)
				},
			)
			if fbSharingErr == nil {
				state := "disabled"
				if fbSharing != 0 {
					state = "enabled"
				}
				fmt.Printf(
					"  GPU %d to GPU %d with %s: %s\n",
					sourceNumber,
					destinationNumber,
					mode,
					state,
				)
			}
		}
	}

	// Link topology includes an FbSharing field, which shows the effective
	// sharing state for every mode, including CUSTOM.
	fmt.Println("###Test amdsmi.GetLinkTopology")
	for destinationNumber, destinationProcessor := range processors {
		if sourceNumber == destinationNumber {
			continue
		}
		topology, topologyErr := example_utils.GetOptionalAPI(
			os.Stdout,
			fmt.Sprintf(
				"amdsmi.GetLinkTopology from GPU %d to GPU %d",
				sourceNumber,
				destinationNumber,
			),
			func() (amdsmi.LinkTopology, error) {
				return amdsmi.GetLinkTopology(sourceProcessor, destinationProcessor)
			},
		)
		if topologyErr == nil {
			fmt.Printf(
				"  GPU %d to GPU %d: link_type=%s link_status=%s num_hops=%d fb_sharing=%d\n",
				sourceNumber,
				destinationNumber,
				topology.LinkType,
				topology.LinkStatus,
				topology.NumHops,
				topology.FbSharing,
			)
		}
	}
	fmt.Println()
}

// promptProcessorIndexes asks the user for a comma-separated list of unique
// GPU indexes.
func promptProcessorIndexes(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processors []amdsmi.ProcessorHandle,
) ([]int, error) {
	prompt := "  Enter comma-separated GPU indexes for the custom group: "
	maxIndex := len(processors) - 1
	errorMessage := fmt.Sprintf(
		"  Enter unique GPU indexes between 0 and %d, separated by commas",
		maxIndex,
	)

	// Keep asking until every value is a valid GPU index and no index is
	// repeated.
	for {
		line, err := prompter.PromptForLine(ctx, prompt)
		if err != nil {
			return nil, fmt.Errorf("read GPU indexes: %w", err)
		}
		if line == "" {
			fmt.Println(errorMessage)
			continue
		}

		values := strings.Split(line, ",")
		indexes := make([]int, 0, len(values))
		seen := make(map[int]bool, len(values))
		valid := true
		for _, value := range values {
			value = strings.TrimSpace(value)
			index, parseErr := strconv.Atoi(value)
			if parseErr != nil || index < 0 || index > maxIndex || seen[index] {
				valid = false
				break
			}
			seen[index] = true
			indexes = append(indexes, index)
		}
		if valid {
			return indexes, nil
		}

		fmt.Println(errorMessage)
	}
}

// setXgmiFbSharingModeV2 changes the XGMI framebuffer sharing mode and
// verifies the result.
func setXgmiFbSharingModeV2(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processors []amdsmi.ProcessorHandle,
	processorNumber int,
) {
	fmt.Println("  Changing XGMI framebuffer sharing requires no running guest VMs.")
	mode, err := promptXgmiMode(ctx, prompter)
	if err != nil {
		return
	}

	var processorIndexes []int
	if mode == amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM {
		// In custom mode the user builds the sharing group explicitly. All
		// GPUs in the group must be on the same NUMA node, and any GPU that
		// already belongs to a group is released from it first.
		processorIndexes, err = promptProcessorIndexes(ctx, prompter, processors)
		if err != nil {
			return
		}
	} else {
		// In auto modes, the v2 API uses only the first processor in the list
		// and applies the selected mode within that GPU's NUMA node. It is
		// equivalent to amdsmi.SetXgmiFbSharingMode() for a single GPU.
		processorIndexes = []int{processorNumber}
	}

	// Require explicit confirmation because changing the sharing mode
	// reconfigures how the selected GPUs share framebuffer memory.
	fmt.Printf("  Selected GPUs are: %v\n", processorIndexes)
	confirmed, err := prompter.PromptConfirmation(
		ctx,
		fmt.Sprintf("  Are you sure you want to set %s with the v2 API?", mode),
	)
	if err != nil {
		return
	}
	if !confirmed {
		fmt.Println("  No operation was performed.\n")
		return
	}

	// The v2 API takes processor handles, not indexes.
	selectedProcessors := make([]amdsmi.ProcessorHandle, len(processorIndexes))
	for i, index := range processorIndexes {
		selectedProcessors[i] = processors[index]
	}

	fmt.Println("###Test amdsmi.SetXgmiFbSharingModeV2")
	if example_utils.SetOptionalAPI(
		os.Stdout,
		"amdsmi.SetXgmiFbSharingModeV2",
		func() error {
			return amdsmi.SetXgmiFbSharingModeV2(selectedProcessors, mode)
		},
	) != amdsmi.AMDSMI_STATUS_SUCCESS {
		fmt.Println()
		return
	}
	fmt.Println("  XGMI framebuffer sharing v2 mode change request completed")

	// Read the sharing state back from each affected GPU to confirm that the
	// new mode took effect.
	for _, sourceNumber := range processorIndexes {
		printXgmiSharingVerification(processors, sourceNumber, mode)
	}
}

// setXgmiPlpd changes the XGMI per-link power down policy and verifies the
// result.
func setXgmiPlpd(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processor amdsmi.ProcessorHandle,
) {
	// Show the available policies first so the user knows which IDs are
	// valid.
	plpd, failed := printXgmiPlpd(processor)
	if failed || len(plpd.Policies) == 0 {
		return
	}

	// The set API takes a policy ID, while Current is an index into the
	// Policies list. Look up the current policy's ID to use as the default
	// input.
	policyIDs := make([]uint64, len(plpd.Policies))
	for i, policy := range plpd.Policies {
		policyIDs[i] = uint64(policy.PolicyID)
	}

	currentPolicyIndex := int(plpd.Current)
	var defaultPolicyID uint64
	if currentPolicyIndex < 0 || currentPolicyIndex >= len(plpd.Policies) {
		fmt.Println(
			"  The current policy index does not identify a supported " +
				"policy; defaulting to the first supported policy.\n",
		)
		defaultPolicyID = policyIDs[0]
	} else {
		defaultPolicyID = uint64(plpd.Policies[currentPolicyIndex].PolicyID)
	}
	// ValidValues limits the input to IDs that appear in the policy list.
	policyID, err := prompter.PromptForInput(
		ctx,
		"  Enter the XGMI PLPD policy ID to set: ",
		example_utils.InputOptions{
			DefaultValue: defaultPolicyID,
			ValidValues:  policyIDs,
			ErrorMessage: fmt.Sprintf(
				"  Policy ID must be one of: %v",
				policyIDs,
			),
		},
	)
	if err != nil {
		return
	}

	confirmed, err := prompter.PromptConfirmation(
		ctx,
		fmt.Sprintf(
			"  Are you sure you want to set XGMI PLPD policy %d?",
			policyID,
		),
	)
	if err != nil {
		return
	}
	if !confirmed {
		fmt.Println("  No operation was performed.\n")
		return
	}

	fmt.Println("###Test amdsmi.SetXgmiPlpd")
	if example_utils.SetOptionalAPI(
		os.Stdout,
		"amdsmi.SetXgmiPlpd",
		func() error {
			return amdsmi.SetXgmiPlpd(processor, uint32(policyID))
		},
	) != amdsmi.AMDSMI_STATUS_SUCCESS {
		fmt.Println()
		return
	}
	fmt.Println("  XGMI PLPD policy change request completed\n")

	// Read the policies again to confirm the new current policy.
	// The setter applies that policy to every GPU on the same socket.
	printXgmiPlpd(processor)
}

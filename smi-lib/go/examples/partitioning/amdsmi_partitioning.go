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

type currentAcceleratorProfile struct {
	profile      amdsmi.AcceleratorPartitionProfile
	partitionIDs []uint32
}

func main() {
	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	err := runPartitioning(ctx)
	stop()

	example_utils.HandleExampleError(err)
}

func runPartitioning(ctx context.Context) (err error) {
	prompter := example_utils.NewPrompter(os.Stdin, os.Stdout)

	// INIT_ALL_PROCESSORS is the default. Choose a different InitFlags value
	// when only a specific processor type is needed.
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

	// Processor discovery is the shared prelude for AMD SMI examples.
	fmt.Println("###Test amdsmi.GetProcessorHandles")
	processors, err := amdsmi.GetProcessorHandles()
	if err != nil {
		return fmt.Errorf("get processor handles: %w", err)
	}

	if len(processors) == 0 {
		fmt.Println("  No GPUs found")
		return nil
	}

	// Information queries are read-only. Both setter operations are
	// hive-wide and therefore call out their idle-device requirements.
	fmt.Println("  Available operations:")
	fmt.Println(
		"  0: Display memory and accelerator partitioning information for all GPUs",
	)
	fmt.Println(
		"  1: Set GPU memory partition mode (XGMI hive-wide; all GPUs in the " +
			"hive must be idle.)",
	)
	fmt.Println(
		"  2: Set accelerator partition profile (XGMI hive-wide; all GPUs " +
			"in the hive must be idle with no active VFs.)",
	)
	fmt.Println("  3: Exit")

	operationUpperBound := uint64(3)
	pickOperation, promptErr := prompter.PromptForInput(
		ctx,
		"  Enter the operation to perform: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &operationUpperBound,
			ErrorMessage: "  Operation must be between 0 and 3",
		},
	)
	if promptErr != nil {
		return fmt.Errorf("select operation: %w", promptErr)
	}

	switch pickOperation {
	case 3:
		fmt.Println("  No operation was performed. Exiting...")
	case 0:
		printPartitioningInfoForAllProcessors(processors)
	case 1:
		processor, selectErr := selectProcessor(ctx, prompter, processors)
		if selectErr != nil {
			return selectErr
		}
		setMemoryPartitionMode(ctx, prompter, processor)
	default:
		processor, selectErr := selectProcessor(ctx, prompter, processors)
		if selectErr != nil {
			return selectErr
		}
		setAcceleratorPartitionProfile(ctx, prompter, processor)
	}

	return nil
}

func formatMemoryPartitionModes(modes []amdsmi.MemoryPartitionType) string {
	if len(modes) == 0 {
		return "None"
	}

	parts := make([]string, len(modes))
	for i, mode := range modes {
		parts[i] = mode.String()
	}
	return strings.Join(parts, ", ")
}

func formatVfModes(modes []amdsmi.VfMode) string {
	if len(modes) == 0 {
		return "None"
	}

	parts := make([]string, len(modes))
	for i, mode := range modes {
		parts[i] = mode.String()
	}
	return strings.Join(parts, ", ")
}

func formatUint32List(values []uint32) string {
	parts := make([]string, len(values))
	for i, value := range values {
		parts[i] = fmt.Sprintf("%d", value)
	}
	return strings.Join(parts, ", ")
}

func modeInSupportedModes(
	mode amdsmi.MemoryPartitionType,
	modes []amdsmi.MemoryPartitionType,
) bool {
	for _, candidate := range modes {
		if candidate == mode {
			return true
		}
	}
	return false
}

func indexOfMemoryPartitionMode(
	modes []amdsmi.MemoryPartitionType,
	mode amdsmi.MemoryPartitionType,
) int {
	for i, candidate := range modes {
		if candidate == mode {
			return i
		}
	}
	return 0
}

func activeAcceleratorProfiles(
	config amdsmi.AcceleratorPartitionProfileConfig,
) []amdsmi.AcceleratorPartitionProfile {
	profiles := make([]amdsmi.AcceleratorPartitionProfile, 0, config.NumProfiles)
	for i := uint32(0); i < config.NumProfiles; i++ {
		profiles = append(profiles, config.Profiles[i])
	}
	return profiles
}

func activeResourceProfiles(
	config amdsmi.AcceleratorPartitionProfileConfig,
) []amdsmi.AcceleratorPartitionResourceProfile {
	profiles := make(
		[]amdsmi.AcceleratorPartitionResourceProfile,
		0,
		config.NumResourceProfiles,
	)
	for i := uint32(0); i < config.NumResourceProfiles; i++ {
		profiles = append(profiles, config.ResourceProfiles[i])
	}
	return profiles
}

func activeGlobalAcceleratorProfiles(
	config amdsmi.AcceleratorPartitionProfileConfigGlobal,
) []amdsmi.AcceleratorPartitionProfileGlobal {
	profiles := make(
		[]amdsmi.AcceleratorPartitionProfileGlobal,
		0,
		config.NumProfiles,
	)
	for i := uint32(0); i < config.NumProfiles; i++ {
		profiles = append(profiles, config.Profiles[i])
	}
	return profiles
}

func activeGlobalResourceProfiles(
	config amdsmi.AcceleratorPartitionProfileConfigGlobal,
) []amdsmi.AcceleratorPartitionResourceProfile {
	profiles := make(
		[]amdsmi.AcceleratorPartitionResourceProfile,
		0,
		config.NumResourceProfiles,
	)
	for i := uint32(0); i < config.NumResourceProfiles; i++ {
		profiles = append(profiles, config.ResourceProfiles[i])
	}
	return profiles
}

func printMemoryPartitionConfig(
	processor amdsmi.ProcessorHandle,
) (amdsmi.MemoryPartitionConfig, bool) {
	// This query reports the active memory partition mode, all modes supported
	// by the GPU, and the NUMA address ranges created by the active mode.
	fmt.Println("###Test amdsmi.GetGpuMemoryPartitionConfig")
	config, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuMemoryPartitionConfig",
		func() (amdsmi.MemoryPartitionConfig, error) {
			return amdsmi.GetGpuMemoryPartitionConfig(processor)
		},
	)
	// Partitioning APIs are not available on every ASIC. GetOptionalAPI
	// reports that condition and lets the example continue with other queries.
	if err != nil {
		fmt.Println("")
		return amdsmi.MemoryPartitionConfig{}, true
	}

	// Mode is the mode currently active on the physical GPU, while
	// PartitionCaps lists every mode that may be requested on this GPU.
	fmt.Printf(
		"  Current memory partition mode is: %s\n",
		config.Mode,
	)
	fmt.Printf(
		"  Supported memory partition modes are: %s\n",
		formatMemoryPartitionModes(config.PartitionCaps.Supported()),
	)
	fmt.Printf("  number of NUMA ranges is: %d\n", config.NumNumaRanges)
	// Each range describes one memory region exposed by the current partition
	// mode, including its memory type and start and end addresses.
	for rangeIndex := uint32(0); rangeIndex < config.NumNumaRanges; rangeIndex++ {
		numaRange := config.NumaRanges[rangeIndex]
		fmt.Printf(
			"  NUMA range %d memory_type is: %s\n",
			rangeIndex,
			numaRange.MemoryType,
		)
		fmt.Printf("  NUMA range %d start is: 0x%x\n", rangeIndex, numaRange.Start)
		fmt.Printf("  NUMA range %d end is: 0x%x\n", rangeIndex, numaRange.End)
	}
	fmt.Println("")
	return config, false
}

func printResourceProfiles(
	resourceProfiles []amdsmi.AcceleratorPartitionResourceProfile,
	indent string,
) {
	// Resource profiles describe how a hardware resource is divided and how
	// many accelerator partitions share each resulting resource partition.
	if len(resourceProfiles) == 0 {
		fmt.Printf("%sNo resource profiles found\n", indent)
		return
	}

	for _, resourceProfile := range resourceProfiles {
		fmt.Printf(
			"%sProfile %d %s resource: partition_resource=%d "+
				"num_partitions_share_resource=%d\n",
			indent,
			resourceProfile.ProfileIndex,
			resourceProfile.ResourceType,
			resourceProfile.PartitionResource,
			resourceProfile.NumPartitionsShareResource,
		)
	}
}

func printAcceleratorProfile(
	profile amdsmi.AcceleratorPartitionProfile,
	indent string,
	vfModes []amdsmi.VfMode,
	showVfModes bool,
) {
	// An accelerator profile describes one complete accelerator-partition
	// layout. Its profile_index is the value accepted by the setter API.
	fmt.Printf("%sprofile_index is: %d\n", indent, profile.ProfileIndex)
	fmt.Printf(
		"%sprofile_type is: %s\n",
		indent,
		profile.ProfileType,
	)
	fmt.Printf("%snum_partitions is: %d\n", indent, profile.NumPartitions)
	fmt.Printf(
		"%smemory_caps are: %s\n",
		indent,
		formatMemoryPartitionModes(profile.MemoryCaps.Supported()),
	)

	if showVfModes {
		// VfMode is reported only by the global accelerator configuration.
		// The per-GPU configuration does not include supported VF modes.
		fmt.Printf("%ssupported VF modes are: %s\n", indent, formatVfModes(vfModes))
	}

	// Each entry corresponds to one accelerator partition. The values are
	// indices into the resource profiles printed with this configuration.
	for partitionIndex, resources := range profile.Resources {
		fmt.Printf(
			"%spartition %d resource profile indices are: %s\n",
			indent,
			partitionIndex,
			formatUint32List(resources),
		)
	}
}

func printAcceleratorPartitionProfileConfig(
	processor amdsmi.ProcessorHandle,
) (amdsmi.AcceleratorPartitionProfileConfig, bool) {
	// This per-GPU query returns the accelerator layouts currently configured
	// on the device and the resource profiles those layouts reference.
	fmt.Println("###Test amdsmi.GetGpuAcceleratorPartitionProfileConfig")
	config, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuAcceleratorPartitionProfileConfig",
		func() (amdsmi.AcceleratorPartitionProfileConfig, error) {
			return amdsmi.GetGpuAcceleratorPartitionProfileConfig(processor)
		},
	)
	if err != nil {
		fmt.Println("")
		return amdsmi.AcceleratorPartitionProfileConfig{}, true
	}

	fmt.Printf("  default_profiles_index is: %d\n", config.DefaultProfileIndex)
	fmt.Println("  Resource profiles:")
	printResourceProfiles(activeResourceProfiles(config), "    ")
	fmt.Println("  Accelerator partition profiles:")
	profiles := activeAcceleratorProfiles(config)
	if len(profiles) == 0 {
		fmt.Println("    No accelerator partition profiles found")
	}
	for _, profile := range profiles {
		printAcceleratorProfile(profile, "    ", nil, false)
	}
	fmt.Println("")
	return config, false
}

func printGlobalAcceleratorPartitionProfileConfig(processor amdsmi.ProcessorHandle) {
	// The global query describes hive-wide accelerator layouts. This can
	// differ from the per-GPU configuration printed by the previous query.
	fmt.Println("###Test amdsmi.GetGpuAcceleratorPartitionProfileConfigGlobal")
	config, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuAcceleratorPartitionProfileConfigGlobal",
		func() (amdsmi.AcceleratorPartitionProfileConfigGlobal, error) {
			return amdsmi.GetGpuAcceleratorPartitionProfileConfigGlobal(processor)
		},
	)
	if err != nil {
		fmt.Println("")
		return
	}

	fmt.Printf("  default_profiles_index is: %d\n", config.DefaultProfileIndex)
	fmt.Println("  Global resource profiles:")
	printResourceProfiles(activeGlobalResourceProfiles(config), "    ")
	fmt.Println("  Global accelerator partition profiles:")
	profiles := activeGlobalAcceleratorProfiles(config)
	if len(profiles) == 0 {
		fmt.Println("    No global accelerator partition profiles found")
	}
	for _, profile := range profiles {
		printAcceleratorProfile(profile.Profile, "    ", profile.VfMode, true)
	}
	fmt.Println("")
}

func currentAcceleratorProfileIndex(
	processor amdsmi.ProcessorHandle,
) *uint32 {
	// Same profile lookup as the display path, without printing it. The
	// setter flows use the index only to mark the active profile.
	profileIndex, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuAcceleratorPartitionProfile",
		func() (uint32, error) {
			profile, _, err := amdsmi.GetGpuAcceleratorPartitionProfile(processor)
			if err != nil {
				return 0, err
			}
			return profile.ProfileIndex, nil
		},
	)
	if err != nil {
		return nil
	}
	return &profileIndex
}

func printCurrentAcceleratorPartitionProfile(
	processor amdsmi.ProcessorHandle,
) (uint32, bool) {
	// Read back the active profile and the partition identifiers assigned
	// under that profile.
	fmt.Println("###Test amdsmi.GetGpuAcceleratorPartitionProfile")
	currentProfile, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuAcceleratorPartitionProfile",
		func() (currentAcceleratorProfile, error) {
			profile, partitionIDs, err := amdsmi.GetGpuAcceleratorPartitionProfile(processor)
			if err != nil {
				return currentAcceleratorProfile{}, err
			}
			return currentAcceleratorProfile{
				profile:      profile,
				partitionIDs: partitionIDs,
			}, nil
		},
	)
	if err != nil {
		fmt.Println("")
		return 0, false
	}

	fmt.Println("  Current accelerator partition profile:")
	printAcceleratorProfile(currentProfile.profile, "    ", nil, false)
	fmt.Printf(
		"  partition_id values are: %s\n",
		formatUint32List(currentProfile.partitionIDs),
	)
	fmt.Println("")
	return currentProfile.profile.ProfileIndex, true
}

func printValidCombinations(
	memoryConfig amdsmi.MemoryPartitionConfig,
	acceleratorConfig amdsmi.AcceleratorPartitionProfileConfig,
	currentProfileIndex *uint32,
) {
	// An accelerator partition profile can only be set while the memory
	// partition mode is one of the modes the profile reports in memory_caps.
	profiles := activeAcceleratorProfiles(acceleratorConfig)
	if len(profiles) == 0 {
		return
	}

	currentMode := memoryConfig.Mode
	fmt.Println(
		"  Valid memory partition mode and accelerator partition profile combinations:",
	)
	for _, profile := range profiles {
		memoryModes := profile.MemoryCaps.Supported()
		// The active profile is marked current even when it is also compatible
		// with the current memory partition mode. Other compatible profiles
		// are marked settable now.
		marker := ""
		if currentProfileIndex != nil && profile.ProfileIndex == *currentProfileIndex {
			marker = " <- current"
		} else if modeInSupportedModes(currentMode, memoryModes) {
			marker = " <- settable now"
		}
		fmt.Printf(
			"    profile %d (%s, %d partition(s)) valid memory partition modes: %s%s\n",
			profile.ProfileIndex,
			profile.ProfileType,
			profile.NumPartitions,
			formatMemoryPartitionModes(memoryModes),
			marker,
		)
	}
	fmt.Printf(
		"  Current memory partition mode is %s. A profile that does not "+
			"list it requires a memory partition mode change first.\n",
		currentMode,
	)
	fmt.Println("")
}

func printPartitioningInfo(processor amdsmi.ProcessorHandle) {
	// Partitioning support is ASIC-specific. Each query is independent so
	// supported information is still shown when another query is unavailable.
	memoryConfig, memoryFailed := printMemoryPartitionConfig(processor)
	acceleratorConfig, acceleratorFailed := printAcceleratorPartitionProfileConfig(processor)
	printGlobalAcceleratorPartitionProfileConfig(processor)
	currentProfileIndex, currentOk := printCurrentAcceleratorPartitionProfile(processor)
	if !memoryFailed && !acceleratorFailed {
		var currentIndex *uint32
		if currentOk {
			currentIndex = &currentProfileIndex
		}
		printValidCombinations(memoryConfig, acceleratorConfig, currentIndex)
	}
}

func printPartitioningInfoForAllProcessors(processors []amdsmi.ProcessorHandle) {
	// Print the BDF before each result block so output from a multi-GPU system
	// can be associated with the correct physical device.
	for processorIndex, processor := range processors {
		fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
		bdf, err := amdsmi.GetGpuDeviceBdf(processor)
		if err != nil {
			fmt.Fprintf(
				os.Stderr,
				"  get BDF for GPU %d: %v\n",
				processorIndex,
				err,
			)
			continue
		}
		fmt.Printf("  GPU %d bdf: %s\n\n", processorIndex, bdf)
		printPartitioningInfo(processor)
	}
}

func selectProcessor(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processors []amdsmi.ProcessorHandle,
) (amdsmi.ProcessorHandle, error) {
	var tmpNilProcessorHandle amdsmi.ProcessorHandle
	// Display stable PCI BDFs alongside list indices so the user can identify
	// the physical GPU before selecting a hive-wide operation.
	fmt.Println("  Available GPUs:")
	for processorIndex, processor := range processors {
		bdf, err := amdsmi.GetGpuDeviceBdf(processor)
		if err != nil {
			return tmpNilProcessorHandle, fmt.Errorf("get BDF for GPU %d: %w", processorIndex, err)
		}
		fmt.Printf("  GPU %d bdf: %s\n", processorIndex, bdf)
	}

	gpuUpperBound := uint64(len(processors) - 1)
	pickGpuIndex, err := prompter.PromptForInput(
		ctx,
		"  Enter the GPU index to operate on: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &gpuUpperBound,
			ErrorMessage: fmt.Sprintf(
				"  GPU index must be between 0 and %d",
				len(processors)-1,
			),
		},
	)
	if err != nil {
		return tmpNilProcessorHandle, fmt.Errorf("select GPU index: %w", err)
	}

	processor := processors[pickGpuIndex]
	// Repeat the BDF after selection to make the operation target explicit.
	fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
	bdf, err := amdsmi.GetGpuDeviceBdf(processor)
	if err != nil {
		return tmpNilProcessorHandle, fmt.Errorf("get BDF for selected GPU %d: %w", pickGpuIndex, err)
	}
	fmt.Printf("  Selected GPU %d bdf: %s\n\n", pickGpuIndex, bdf)
	return processor, nil
}

func changeMemoryPartitionMode(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processor amdsmi.ProcessorHandle,
	requestedMode amdsmi.MemoryPartitionType,
) bool {
	// The change applies to every GPU in the selected GPU's XGMI hive. All
	// GPUs in the hive must be idle and have no workloads.
	confirmed, err := prompter.PromptConfirmation(
		ctx,
		fmt.Sprintf(
			"  Are you sure you want to set memory partition mode to %s? "+
				"This operation applies to all GPUs in the selected GPU's XGMI "+
				"hive. All GPUs in the hive must be idle with no workloads.",
			requestedMode,
		),
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return false
	}
	if !confirmed {
		fmt.Println("  No operation was performed. Exiting...")
		return false
	}

	fmt.Println("###Test amdsmi.SetGpuMemoryPartitionMode")
	fmt.Printf(
		"  Setting memory partition mode to %s\n",
		requestedMode,
	)
	if example_utils.SetOptionalAPI(
		os.Stdout,
		"amdsmi.SetGpuMemoryPartitionMode",
		func() error {
			return amdsmi.SetGpuMemoryPartitionMode(processor, requestedMode)
		},
	) != amdsmi.AMDSMI_STATUS_SUCCESS {
		return false
	}
	// A successful return means the request was accepted. Read the current
	// configuration back below so the user can see the resulting state.
	fmt.Println("  Memory partition mode change request completed")
	fmt.Println(
		"  Changing the memory partition mode also changes the accelerator " +
			"partition profile to the default profile of the new memory " +
			"partition mode",
	)

	printMemoryPartitionConfig(processor)
	printCurrentAcceleratorPartitionProfile(processor)
	return true
}

func setMemoryPartitionMode(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processor amdsmi.ProcessorHandle,
) {
	// Fetch configs without the verbose dumps from operation 0 (NUMA ranges,
	// resource profiles, full profile listings). Those live under "display";
	// here only the mode/profile combination summary is needed to choose.
	config, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuMemoryPartitionConfig",
		func() (amdsmi.MemoryPartitionConfig, error) {
			return amdsmi.GetGpuMemoryPartitionConfig(processor)
		},
	)
	if err != nil {
		return
	}

	supportedModes := config.PartitionCaps.Supported()
	if len(supportedModes) == 0 {
		fmt.Println("  No supported memory partition modes were reported")
		return
	}

	// Accelerator profiles that can be set after a mode change depend on the
	// memory partition mode; show which profiles each mode allows.
	acceleratorConfig, acceleratorErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuAcceleratorPartitionProfileConfig",
		func() (amdsmi.AcceleratorPartitionProfileConfig, error) {
			return amdsmi.GetGpuAcceleratorPartitionProfileConfig(processor)
		},
	)
	if acceleratorErr == nil {
		printValidCombinations(
			config,
			acceleratorConfig,
			currentAcceleratorProfileIndex(processor),
		)
	}

	fmt.Println("  Available memory partition modes:")
	for modeIndex, mode := range supportedModes {
		fmt.Printf("  %d: %s\n", modeIndex, mode.String())
	}

	defaultModeIndex := indexOfMemoryPartitionMode(supportedModes, config.Mode)
	if !modeInSupportedModes(config.Mode, supportedModes) {
		defaultModeIndex = 0
	}

	// Default the prompt to the active mode so that choice is suggested.
	// Confirming it still runs the hive-wide setter.
	modeUpperBound := uint64(len(supportedModes) - 1)
	pickModeIndex, err := prompter.PromptForInput(
		ctx,
		"  Enter the memory partition mode to set: ",
		example_utils.InputOptions{
			DefaultValue: uint64(defaultModeIndex),
			UpperBound:   &modeUpperBound,
			ErrorMessage: fmt.Sprintf(
				"  Mode index must be between 0 and %d",
				len(supportedModes)-1,
			),
		},
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return
	}

	changeMemoryPartitionMode(ctx, prompter, processor, supportedModes[pickModeIndex])
}

func printBlockedProfiles(
	blockedProfiles []amdsmi.AcceleratorPartitionProfile,
	currentMode amdsmi.MemoryPartitionType,
) {
	// Keep incompatible profiles visible instead of silently filtering them;
	// their memory_caps explain which prerequisite mode each profile needs.
	fmt.Printf(
		"  These accelerator partition profiles cannot be set while the "+
			"memory partition mode is %s:\n",
		currentMode,
	)
	for _, profile := range blockedProfiles {
		fmt.Printf(
			"    profile %d (%s) valid memory partition modes: %s\n",
			profile.ProfileIndex,
			profile.ProfileType,
			formatMemoryPartitionModes(profile.MemoryCaps.Supported()),
		)
	}
}

func offerMemoryPartitionModeChange(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processor amdsmi.ProcessorHandle,
	memoryConfig amdsmi.MemoryPartitionConfig,
	blockedProfiles []amdsmi.AcceleratorPartitionProfile,
) bool {
	// Lead the user through the memory partition mode change that the blocked
	// profiles need. The profile itself is set on a later run, once the new
	// memory partition mode is in place.
	supportedModes := memoryConfig.PartitionCaps.Supported()
	requiredModes := make([]amdsmi.MemoryPartitionType, 0)
	// Preserve the GPU's advertised mode order and include only modes that
	// make at least one currently blocked accelerator profile available.
	for _, mode := range supportedModes {
		for _, profile := range blockedProfiles {
			if modeInSupportedModes(mode, profile.MemoryCaps.Supported()) {
				requiredModes = append(requiredModes, mode)
				break
			}
		}
	}
	if len(requiredModes) == 0 {
		fmt.Println(
			"  This GPU does not support any of the memory partition modes " +
				"those profiles require",
		)
		return false
	}

	fmt.Println(
		"  To use one of those profiles, the memory partition mode must be " +
			"changed first.",
	)
	confirmed, err := prompter.PromptConfirmation(
		ctx,
		"  Do you want to change the memory partition mode now?",
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return false
	}
	if !confirmed {
		fmt.Println("  The memory partition mode was not changed")
		return false
	}

	fmt.Println("  Memory partition modes that make those profiles available:")
	for modeIndex, mode := range requiredModes {
		fmt.Printf("  %d: %s\n", modeIndex, mode.String())
	}

	modeUpperBound := uint64(len(requiredModes) - 1)
	pickModeIndex, err := prompter.PromptForInput(
		ctx,
		"  Enter the memory partition mode to set: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &modeUpperBound,
			ErrorMessage: fmt.Sprintf(
				"  Mode index must be between 0 and %d",
				len(requiredModes)-1,
			),
		},
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return false
	}

	if !changeMemoryPartitionMode(ctx, prompter, processor, requiredModes[pickModeIndex]) {
		return false
	}

	// Changing memory mode selects that mode's default accelerator profile.
	// Ask the user to rerun the operation before applying a different profile.
	fmt.Println(
		"  Run operation 2 again to set the accelerator partition profile " +
			"that requires this memory partition mode",
	)
	return true
}

func setAcceleratorPartitionProfile(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processor amdsmi.ProcessorHandle,
) {
	// Changing an accelerator partition profile can disrupt workloads. Only a
	// profile that lists the current memory partition mode in its memory_caps
	// can be set; the others need a memory partition mode change first.
	memoryConfig, memoryFailed := printMemoryPartitionConfig(processor)
	config, configFailed := printAcceleratorPartitionProfileConfig(processor)
	if configFailed {
		return
	}

	profiles := activeAcceleratorProfiles(config)
	if len(profiles) == 0 {
		fmt.Println("  No accelerator partition profiles were reported")
		return
	}

	var settableProfiles []amdsmi.AcceleratorPartitionProfile
	var blockedProfiles []amdsmi.AcceleratorPartitionProfile
	if memoryFailed {
		// Without the memory partition configuration the valid combinations
		// are unknown, so every reported profile is offered.
		settableProfiles = profiles
	} else {
		printValidCombinations(
			memoryConfig,
			config,
			currentAcceleratorProfileIndex(processor),
		)
		currentMode := memoryConfig.Mode
		// The setter accepts only profiles whose memory_caps contain the
		// currently active memory partition mode.
		for _, profile := range profiles {
			if modeInSupportedModes(currentMode, profile.MemoryCaps.Supported()) {
				settableProfiles = append(settableProfiles, profile)
			} else {
				blockedProfiles = append(blockedProfiles, profile)
			}
		}
	}

	if len(blockedProfiles) > 0 {
		printBlockedProfiles(blockedProfiles, memoryConfig.Mode)
		if len(settableProfiles) == 0 {
			fmt.Printf(
				"  No accelerator partition profile can be set while the "+
					"memory partition mode is %s\n",
				memoryConfig.Mode,
			)
		}
		if offerMemoryPartitionModeChange(
			ctx,
			prompter,
			processor,
			memoryConfig,
			blockedProfiles,
		) {
			return
		}
		if len(settableProfiles) == 0 {
			fmt.Println("  No operation was performed. Exiting...")
			return
		}
	}

	profileIndices := make([]uint64, len(settableProfiles))
	for i, profile := range settableProfiles {
		profileIndices[i] = uint64(profile.ProfileIndex)
	}
	// Profile indices are API-defined identifiers and may not be contiguous,
	// so validate against the advertised values rather than a numeric range.
	fmt.Printf(
		"  Accelerator partition profile indices that can be set now: %s\n",
		strings.Join(uint64SliceToStrings(profileIndices), ", "),
	)

	pickProfileIndex, err := prompter.PromptForInput(
		ctx,
		"  Enter the accelerator partition profile index to set: ",
		example_utils.InputOptions{
			DefaultValue: profileIndices[0],
			ValidValues:  profileIndices,
			ErrorMessage: fmt.Sprintf(
				"  Profile index must be one of: %s",
				strings.Join(uint64SliceToStrings(profileIndices), ", "),
			),
		},
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return
	}

	confirmed, err := prompter.PromptConfirmation(
		ctx,
		fmt.Sprintf(
			"  Are you sure you want to set accelerator partition profile %d? "+
				"This operation applies to all GPUs in the selected GPU's XGMI "+
				"hive. All GPUs in the hive must be idle with no active VFs.",
			pickProfileIndex,
		),
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return
	}
	if !confirmed {
		fmt.Println("  No operation was performed. Exiting...")
		return
	}

	fmt.Println("###Test amdsmi.SetGpuAcceleratorPartitionProfile")
	fmt.Printf("  Setting accelerator partition profile to %d\n", pickProfileIndex)
	if example_utils.SetOptionalAPI(
		os.Stdout,
		"amdsmi.SetGpuAcceleratorPartitionProfile",
		func() error {
			return amdsmi.SetGpuAcceleratorPartitionProfile(
				processor,
				uint32(pickProfileIndex),
			)
		},
	) != amdsmi.AMDSMI_STATUS_SUCCESS {
		return
	}
	// Read the profile back after the request so its layout and assigned
	// partition identifiers are visible in the example output.
	fmt.Println("  Accelerator partition profile change request completed")

	printCurrentAcceleratorPartitionProfile(processor)
}

func uint64SliceToStrings(values []uint64) []string {
	result := make([]string, len(values))
	for i, value := range values {
		result[i] = fmt.Sprintf("%d", value)
	}
	return result
}

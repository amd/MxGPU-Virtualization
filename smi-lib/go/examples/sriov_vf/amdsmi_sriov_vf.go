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
	"syscall"

	"smi-lib/go/amdsmi"
	example_utils "smi-lib/go/examples/example_utils"
)

type vfCountInfo struct {
	numEnabled   uint32
	numSupported uint32
}

func main() {
	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	err := runSriovVf(ctx)
	stop()
	example_utils.HandleExampleError(err)
}

func runSriovVf(ctx context.Context) (err error) {
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
	fmt.Printf("  number of GPUs is: %d\n\n", len(processors))

	if len(processors) == 0 {
		fmt.Println("  No GPUs found.")
		return nil
	}

	// Displaying information is read-only and runs on every GPU.
	// Setting the VF count changes the selected GPU's configuration.
	fmt.Println("  Available operations:")
	fmt.Println("  0: Display SR-IOV and VF information")
	fmt.Println(
		"  1: Set enabled VF count (Changing the VF count can disrupt workloads.)",
	)
	fmt.Println("  2: Exit")

	operationUpperBound := uint64(2)
	pickOperation, promptErr := prompter.PromptForInput(
		ctx,
		"  Enter the operation to perform: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &operationUpperBound,
			ErrorMessage: "  Operation must be between 0 and 2",
		},
	)
	if promptErr != nil {
		return fmt.Errorf("select operation: %w", promptErr)
	}

	switch pickOperation {
	case 2:
		fmt.Println("  No operation was performed.")
	case 0:
		for processorNumber, processor := range processors {
			if sriovErr := printSriovInfo(processorNumber, processor); sriovErr != nil {
				return sriovErr
			}
		}
	default:
		// Display each GPU's BDF so the user can identify the physical
		// device before changing its VF count.
		fmt.Println("  Available GPUs:")
		for gpuIndex, processor := range processors {
			bdf, bdfErr := amdsmi.GetGpuDeviceBdf(processor)
			if bdfErr != nil {
				return fmt.Errorf("get BDF for GPU %d: %w", gpuIndex, bdfErr)
			}
			fmt.Printf("  GPU %d bdf: %s\n", gpuIndex, bdf)
		}

		// Prompt the user to select a GPU, and validate that the
		// selected index belongs to one of the discovered GPU handles.
		gpuUpperBound := uint64(len(processors) - 1)
		pickGpuIndex, gpuPromptErr := prompter.PromptForInput(
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
		if gpuPromptErr != nil {
			return fmt.Errorf("select GPU index: %w", gpuPromptErr)
		}

		processor := processors[pickGpuIndex]
		fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
		bdf, bdfErr := amdsmi.GetGpuDeviceBdf(processor)
		if bdfErr != nil {
			return fmt.Errorf("get BDF for selected GPU %d: %w", pickGpuIndex, bdfErr)
		}
		fmt.Printf("  Selected GPU %d bdf: %s\n\n", pickGpuIndex, bdf)
		setNumVf(ctx, prompter, processor)
	}

	return nil
}

func printFramebufferLayout(processor amdsmi.ProcessorHandle) {
	// The framebuffer layout describes how the GPU's memory is split between
	// the PF and its VFs. The usable VF range bounds how much framebuffer each
	// VF can be given.
	fmt.Println("###Test amdsmi.GetFbLayout")
	fbLayout, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetFbLayout",
		func() (amdsmi.PfFbInfo, error) {
			return amdsmi.GetFbLayout(processor)
		},
	)
	if err == nil {
		fmt.Printf("  total_fb_size is: %d MB\n", fbLayout.TotalFbSize)
		fmt.Printf("  pf_fb_reserved is: %d MB\n", fbLayout.PfFbReserved)
		fmt.Printf("  pf_fb_offset is: %d MB\n", fbLayout.PfFbOffset)
		fmt.Printf("  fb_alignment is: %d MB\n", fbLayout.FbAlignment)
		fmt.Printf("  max_vf_fb_usable is: %d MB\n", fbLayout.MaxVfFbUsable)
		fmt.Printf("  min_vf_fb_usable is: %d MB\n", fbLayout.MinVfFbUsable)
	}
	fmt.Println()
}

func printVfInfo(vfIndex int, vfHandle amdsmi.VfHandle) {
	// VF info is the configuration the host gave the VF: where its
	// framebuffer starts, how large it is, and its GFX scheduling timeslice.
	fmt.Println("###Test amdsmi.GetVfInfo")
	vfInfo, err  := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetVfInfo",
		func() (amdsmi.VfInfo, error) {
			return amdsmi.GetVfInfo(vfHandle)
		},
	)
	if err == nil {
		fmt.Printf("  VF %d fb_offset is: %d MB\n", vfIndex, vfInfo.Fb.FbOffset)
		fmt.Printf("  VF %d fb_size is: %d MB\n", vfIndex, vfInfo.Fb.FbSize)
		fmt.Printf("  VF %d gfx_timeslice is: %d us\n", vfIndex, vfInfo.GfxTimeslice)
	}
	fmt.Println()
}

func printVfData(vfIndex int, vfHandle amdsmi.VfHandle) {
	// VF data is runtime state reported by the host scheduler, unlike the
	// static configuration returned by amdsmi.GetVfInfo.
	fmt.Println("###Test amdsmi.GetVfData")
	vfData, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetVfData",
		func() (amdsmi.VfData, error) {
			return amdsmi.GetVfData(vfHandle)
		},
	)
	if err != nil {
		fmt.Println()
		return
	}

	// Scheduler statistics: FLR (function level reset) count, how long the
	// last boot, shutdown, and reset took, the VF's current scheduling
	// state, and timestamps for the most recent lifecycle events.
	sched := vfData.Sched
	fmt.Printf("  VF %d sched['flr_count'] is: %d\n", vfIndex, sched.FlrCount)
	fmt.Printf("  VF %d sched['boot_up_time'] is: %d us\n", vfIndex, sched.BootUpTime)
	fmt.Printf("  VF %d sched['shutdown_time'] is: %d us\n", vfIndex, sched.ShutdownTime)
	fmt.Printf("  VF %d sched['reset_time'] is: %d us\n", vfIndex, sched.ResetTime)
	fmt.Printf(
		"  VF %d sched['state'] is: %s\n",
		vfIndex,
		sched.State.String(),
	)
	fmt.Printf("  VF %d sched['last_boot_start'] is: %s\n", vfIndex, sched.LastBootStart)
	fmt.Printf("  VF %d sched['last_boot_end'] is: %s\n", vfIndex, sched.LastBootEnd)
	fmt.Printf(
		"  VF %d sched['last_shutdown_start'] is: %s\n",
		vfIndex,
		sched.LastShutdownStart,
	)
	fmt.Printf("  VF %d sched['last_shutdown_end'] is: %s\n", vfIndex, sched.LastShutdownEnd)
	fmt.Printf("  VF %d sched['last_reset_start'] is: %s\n", vfIndex, sched.LastResetStart)
	fmt.Printf("  VF %d sched['last_reset_end'] is: %s\n", vfIndex, sched.LastResetEnd)
	fmt.Printf(
		"  VF %d sched['current_active_time'] is: %s\n",
		vfIndex,
		sched.CurrentActiveTime,
	)
	fmt.Printf(
		"  VF %d sched['current_running_time'] is: %s\n",
		vfIndex,
		sched.CurrentRunningTime,
	)
	fmt.Printf("  VF %d sched['total_active_time'] is: %s\n", vfIndex, sched.TotalActiveTime)
	fmt.Printf("  VF %d sched['total_running_time'] is: %s\n", vfIndex, sched.TotalRunningTime)

	// Guards limit how often a VF can trigger an event type (for example
	// FLRs) within an interval. guard.Guard holds each event type's
	// current state, event count, interval, and threshold.
	guard := vfData.Guard
	fmt.Printf("  VF %d guard['enabled'] is: %v\n", vfIndex, guard.Enabled)
	for i := 0; i < amdsmi.AMDSMI_GUARD_EVENT__MAX; i++ {
		guardType := amdsmi.GuardEventType(i)
		guardInfo := guard.Guard[i]
		fmt.Printf("  VF %d %s guard:\n", vfIndex, guardType.String())
		fmt.Printf(
			"    state is: %s\n",
			guardInfo.State.String(),
		)
		fmt.Printf("    amount is: %d\n", guardInfo.Amount)
		fmt.Printf("    interval is: %d seconds\n", guardInfo.Interval)
		fmt.Printf("    threshold is: %d\n", guardInfo.Threshold)
		fmt.Printf("    active is: %d\n", guardInfo.Active)
	}
	fmt.Println()
}

func printGuestData(vfIndex int, vfHandle amdsmi.VfHandle) {
	// Guest data comes from the driver running inside the VM, so it is only
	// available while a guest driver is loaded on the VF.
	fmt.Println("###Test amdsmi.GetGuestData")
	guestData, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGuestData",
		func() (amdsmi.GuestData, error) {
			return amdsmi.GetGuestData(vfHandle)
		},
	)
	if err == nil {
		fmt.Printf("  VF %d driver_version is: %s\n", vfIndex, guestData.DriverVersion)
		fmt.Printf("  VF %d fb_usage is: %d MB\n", vfIndex, guestData.FbUsage)
	}
	fmt.Println()
}

func printVfHbmInfo(vfIndex int, vfHandle amdsmi.VfHandle) {
	// HBM info reports the physical memory range backing the VF and the
	// NUMA node it belongs to.
	fmt.Println("###Test amdsmi.GetVfHbmInfo")
	hbmInfo, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetVfHbmInfo",
		func() (amdsmi.VfHbmInfo, error) {
			return amdsmi.GetVfHbmInfo(vfHandle)
		},
	)
	if err == nil {
		fmt.Printf("  VF %d phy_addr is: %#x\n", vfIndex, hbmInfo.PhyAddr)
		fmt.Printf("  VF %d phy_size is: %d bytes\n", vfIndex, hbmInfo.PhySize)
		fmt.Printf("  VF %d numa_id is: %d\n", vfIndex, hbmInfo.NumaID)
		fmt.Printf("  VF %d HBM name is: %s\n", vfIndex, hbmInfo.Name)
	}
	fmt.Println()
}

func printVfPartitions(processor amdsmi.ProcessorHandle) {
	// The partition list has one entry per enabled VF. Each entry holds the
	// VF handle that all the per-VF APIs below take as input.
	fmt.Println("###Test amdsmi.GetVfPartitionInfo")
	partitions, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetVfPartitionInfo",
		func() ([]amdsmi.PartitionInfo, error) {
			return amdsmi.GetVfPartitionInfo(processor)
		},
	)
	if err != nil {
		fmt.Println()
		return
	}

	fmt.Printf("  number of enabled VFs is: %d\n", len(partitions))
	if len(partitions) == 0 {
		fmt.Println("  No enabled VFs found; per-VF APIs require an enabled VF")
		fmt.Println()
		return
	}

	// Print each VF's framebuffer slice, then query every per-VF API with
	// that VF's handle.
	for vfIndex, partition := range partitions {
		vfHandle := partition.VfHandle
		fmt.Printf(
			"  VF %d partition fb_offset is: %d MB\n",
			vfIndex,
			partition.Fb.FbOffset,
		)
		fmt.Printf(
			"  VF %d partition fb_size is: %d MB\n\n",
			vfIndex,
			partition.Fb.FbSize,
		)

		printVfInfo(vfIndex, vfHandle)
		printVfData(vfIndex, vfHandle)
		printGuestData(vfIndex, vfHandle)
		printVfHbmInfo(vfIndex, vfHandle)
	}
}

func printSriovInfo(processorNumber int, processor amdsmi.ProcessorHandle) error {
	// Print the GPU's BDF first so the output below can be matched to a
	// physical device.
	fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
	bdf, err := amdsmi.GetGpuDeviceBdf(processor)
	if err != nil {
		return fmt.Errorf("get GPU BDF for processor %d: %w", processorNumber, err)
	}
	fmt.Printf("  Processor number: %d bdf: %s\n\n", processorNumber, bdf)

	// The enabled VF count tells us whether per-VF APIs can be queried.
	fmt.Println("###Test amdsmi.GetNumVf")
	vfCount, vfCountErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetNumVf",
		func() (vfCountInfo, error) {
			numEnabled, numSupported, getErr := amdsmi.GetNumVf(processor)
			if getErr != nil {
				return vfCountInfo{}, getErr
			}
			return vfCountInfo{
				numEnabled:   numEnabled,
				numSupported: numSupported,
			}, nil
		},
	)
	if vfCountErr == nil {
		fmt.Printf("  Number of enabled VFs is: %d\n", vfCount.numEnabled)
		fmt.Printf("  Number of supported VFs is: %d\n\n", vfCount.numSupported)
	} else {
		fmt.Println()
	}

	// GPU-level SR-IOV information can be shown even when no VFs are
	// enabled. Per-VF information is skipped if the VF count is unavailable.
	printFramebufferLayout(processor)
	if vfCountErr == nil {
		printVfPartitions(processor)
	}

	return nil
}

func setNumVf(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processor amdsmi.ProcessorHandle,
) {
	// Changing the VF count persists beyond this script and can disrupt
	// workloads, so it requires a validated count and confirmation.
	fmt.Println("###Test amdsmi.GetNumVf")
	vfCount, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetNumVf",
		func() (vfCountInfo, error) {
			numEnabled, numSupported, getErr := amdsmi.GetNumVf(processor)
			if getErr != nil {
				return vfCountInfo{}, getErr
			}
			return vfCountInfo{
				numEnabled:   numEnabled,
				numSupported: numSupported,
			}, nil
		},
	)
	if err != nil {
		return
	}
	fmt.Printf("  Number of enabled VFs is: %d\n", vfCount.numEnabled)
	fmt.Printf("  Number of supported VFs is: %d\n", vfCount.numSupported)

	// Read the current and supported VF counts first.
	// The supported count is the upper bound for the new value.
	fmt.Printf(
		"  Note: You cannot set the enabled VF count higher than the supported VF count (%d).\n",
		vfCount.numSupported,
	)
	// Prompt for the new VF count, defaulting to the current count.
	// Reject values above the supported count.
	supportedUpperBound := uint64(vfCount.numSupported)
	pickNumVf, err := prompter.PromptForInput(
		ctx,
		"  Enter the enabled VF count to set: ",
		example_utils.InputOptions{
			DefaultValue: uint64(vfCount.numEnabled),
			UpperBound:   &supportedUpperBound,
			ErrorMessage: fmt.Sprintf(
				"  VF count must be between 0 and %d",
				vfCount.numSupported,
			),
		},
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return
	}

	// Require explicit confirmation because changing the VF count can
	// disrupt VMs that are using the existing VFs.
	confirmed, confirmErr := prompter.PromptConfirmation(
		ctx,
		fmt.Sprintf(
			"  Are you sure you want to change enabled VFs from %d to %d?",
			vfCount.numEnabled,
			pickNumVf,
		),
	)
	if confirmErr != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", confirmErr)
		return
	}
	if !confirmed {
		fmt.Println("  No operation was performed. Exiting...")
		return
	}

	// Submit the change request.
	fmt.Println("###Test amdsmi.SetNumVf")
	fmt.Printf(
		"  Changing enabled VFs from %d to %d\n",
		vfCount.numEnabled,
		pickNumVf,
	)
	status := example_utils.SetOptionalAPI(
		os.Stdout,
		"amdsmi.SetNumVf",
		func() error {
			return amdsmi.SetNumVf(processor, uint32(pickNumVf))
		},
	)
	if status != amdsmi.AMDSMI_STATUS_SUCCESS {
		return
	}
	fmt.Println("  VF count change request completed")

	// Read the VF count again to verify the requested count is now active.
	fmt.Println("###Test amdsmi.GetNumVf")
	updatedVfCount, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetNumVf",
		func() (vfCountInfo, error) {
			numEnabled, numSupported, getErr := amdsmi.GetNumVf(processor)
			if getErr != nil {
				return vfCountInfo{}, getErr
			}
			return vfCountInfo{
				numEnabled:   numEnabled,
				numSupported: numSupported,
			}, nil
		},
	)
	if err == nil {
		fmt.Printf("  Number of enabled VFs is: %d\n", updatedVfCount.numEnabled)
		fmt.Printf("  Number of supported VFs is: %d\n", updatedVfCount.numSupported)
	}
}

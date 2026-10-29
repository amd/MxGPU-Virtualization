# Run the AMD SMI topology example

The topology example is an interactive program for inspecting GPU topology
and interconnect information. Its display operation is read-only and reports
information for every discovered GPU. It also includes separate, confirmed
operations to change XGMI framebuffer sharing or the XGMI per-link power-down
(PLPD) policy on a selected GPU.

`go/examples/topology/amdsmi_topology.go` is the Go implementation.

Changing an XGMI setting can affect GPU connectivity and virtual-machine
configuration. Run a setter operation only when there are no running guest VMs
and it is safe to change the selected GPU's configuration.

## Prerequisites

Review the [shared prerequisites for AMD SMI Go examples](../../../docs/how_to/amdsmi_go_lib.md#prerequisites)
before running this program. At least one AMD GPU must be available.

The read-only display operation is useful on a single-GPU system. GPU-to-GPU
topology, P2P status, nearest-GPU, and XGMI framebuffer-sharing output require
two or more GPUs, and XGMI-specific output requires a supported
XGMI-connected configuration. NIC output is optional and depends on the
associated driver and build support.

For a setter operation, ensure that no guest VM is running on the affected
GPUs. The XGMI APIs return an error when their platform, topology, or selected
mode is unsupported.

## Run the example

From `smi-lib/go/`, run:

```shell
sudo go run examples/topology/amdsmi_topology.go
```

If no GPUs are discovered, the example prints `No GPUs found` and does not
offer an operation.

## Use the interactive prompts

After initialization and GPU discovery, the example presents this menu before
listing GPUs:

| Input | Operation | Result |
| --- | --- | --- |
| `0` | Display topology and interconnect information | Lists GPUs (plus an All GPUs option), prints read-only per-GPU topology for the selection, then prints the link matrix and pairwise queries with each selected GPU as the source, plus optional NIC information. |
| `1` | Set XGMI framebuffer sharing mode v2 | Lists GPUs, prompts for one GPU and an XGMI sharing mode, then optionally prompts for a custom GPU group and confirms before calling the v2 setter. |
| `2` | Set XGMI per-link power-down policy | Lists GPUs, prompts for one GPU and one of its reported PLPD policy IDs, then confirms before changing the policy. |
| `3` | Exit | Makes no change and shuts down AMD SMI. |

Enter an integer from `0` through `3`. The example repeats the prompt for an
invalid or out-of-range operation. It performs at most one operation per run.

For operations `0`, `1`, and `2`, the example lists the discovered GPUs with
their indexes and BDF addresses before prompting for a GPU. Operation `0`
also offers an `All GPUs` choice whose index is one past the last GPU:

```text
Available GPUs:
GPU 0 bdf: 0000:41:00.0
GPU 1 bdf: 0000:81:00.0
2: All GPUs
Enter the GPU index to operate on: 1
```

The GPU index must be a non-negative integer in the displayed range. The
example repeats the prompt for an invalid or out-of-range index. Operations
`1` and `2` then display the BDF of the selected GPU.

### Display topology and interconnect information

Choose `0`, then a GPU index or `All GPUs`. The example starts each selected
GPU section with its BDF address, then displays these read-only categories:

- Link metrics from `amdsmi.GetLinkMetrics()`, including the link BDF, type,
  status, rate, maximum bandwidth, and read/write counters.
- Nearest GPUs for XGMI and PCIe from `amdsmi.GetLinkTopologyNearest()`.
- The NUMA node and CPU affinity bitmasks for both `NUMA_SCOPE` and
  `SOCKET_SCOPE`.
- XGMI framebuffer-sharing capabilities and the current XGMI PLPD policy list.

After that per-GPU dump, the example prints the link matrix and pairwise
topology queries using each selected GPU as the source and every discovered
GPU as a destination. Choosing one GPU prints that GPU's matrix row and the
pairs that start from it. Choosing `All GPUs` prints every source row and
every ordered pair. Optional NIC information is printed for every discovered
NIC regardless of the GPU selection.

For example, CPU affinity is queried with an affinity-scope value:

```go
affinityScopes := []amdsmi.AffinityScope{
	amdsmi.AMDSMI_AFFINITY_SCOPE_NODE,
	amdsmi.AMDSMI_AFFINITY_SCOPE_SOCKET,
}
for _, scope := range affinityScopes {
	affinity, err := amdsmi.GetCpuAffinityWithScope(processor, scope)
}
```

`AMDSMI_AFFINITY_SCOPE_NODE` is the NUMA-node scope. The Go example labels
that query `NUMA_SCOPE` in its output to match the Python example.

The example creates a link matrix by querying `amdsmi.GetLinkTopology()` for
each selected source GPU and every destination GPU, including the source
itself.

Each cell reports the link type, link status, hop count, and
framebuffer-sharing state.
The destination header still lists every discovered GPU:

```text
NxN link matrix (type:status/hops:framebuffer-sharing):
Source\Destination: GPU 0 | GPU 1
GPU 0: INTERNAL:ENABLED/0hop:fb=off | XGMI:ENABLED/1hop:fb=on
GPU 1: XGMI:ENABLED/1hop:fb=on | INTERNAL:ENABLED/0hop:fb=off
```

The exact link types, statuses, hop counts, and framebuffer-sharing states
depend on the installed hardware and driver.

When two or more GPUs are available, the example additionally calls these APIs
for each selected source GPU and every other GPU:

```go
hops, linkType, err := amdsmi.TopoGetLinkType(
	sourceProcessor, destinationProcessor)
linkType, cap, err := amdsmi.TopoGetP2pStatus(
	sourceProcessor, destinationProcessor)
fbSharing, err := amdsmi.GetXgmiFbSharingModeInfo(
	sourceProcessor, destinationProcessor, mode)
```

`amdsmi.TopoGetP2pStatus()` reports the link type and its coherent, atomic,
DMA, and bidirectional P2P capabilities. Framebuffer-sharing state is queried
only for the automatic modes (`MODE_1`, `MODE_2`, `MODE_4`, and `MODE_8`)
reported as supported by `amdsmi.GetXgmiFbSharingCaps()`. The example reports
the `CUSTOM` capability but does not pass `CUSTOM` to
`amdsmi.GetXgmiFbSharingModeInfo()`.

On a single-GPU system, the example prints that GPU-to-GPU queries require at
least two GPUs.

### Optional NIC information

After the GPU sections, the example attempts to discover NIC processors. For
each NIC it prints the BDF, NUMA node, CPU affinity mask, and the link type
between that NIC and every discovered GPU:

```go
nicProcessors, err := amdsmi.GetNicProcessorHandles()
numaInfo, err := amdsmi.GetNicNumaInfo(nicProcessor)
hops, linkType, err := amdsmi.TopoGetLinkType(nicProcessor, gpuProcessor)
```

### Set XGMI framebuffer sharing mode v2

Choose `1` to use `amdsmi.SetXgmiFbSharingModeV2()` for the selected GPU.

The example first displays the available mode choices:

```text
Available XGMI framebuffer sharing modes:
0: CUSTOM
1: MODE_1
2: MODE_2
3: MODE_4
4: MODE_8
Enter the XGMI framebuffer sharing mode: 3
```

The mode selection is an index into the displayed list; it is not a command
line argument. The requested mode must be supported by the selected system.

For an automatic mode (`MODE_1`, `MODE_2`, `MODE_4`, or `MODE_8`), the
selected GPU is the only processor passed to the API, and the requested mode
applies within that GPU's NUMA node. This is equivalent to calling
`amdsmi.SetXgmiFbSharingMode()`, which the v2 setter uses internally for a
single GPU.

For `CUSTOM`, which only this v2 setter supports and which requires the
selected GPUs to share a NUMA node, the example additionally asks for a
comma-separated list of unique GPU indexes:

```text
Enter comma-separated GPU indexes for the custom group: 0,2
```

Every index must identify a discovered GPU, and the custom group must meet the
API's topology requirement that all selected processors are in the same NUMA
node.

Note that `amdsmi.GetXgmiFbSharingModeInfo()` does not accept `CUSTOM`.

The example shows the selected indexes and asks for explicit confirmation
before it changes the configuration:

```text
Are you sure you want to set MODE_4 with the v2 API? (YES/no)
```

Enter `yes` to continue or `no` to cancel; either value is case-insensitive.
Any other response repeats the confirmation prompt. On confirmation, the
example calls:

```go
selectedProcessors := make([]amdsmi.ProcessorHandle, len(processorIndexes))
for i, index := range processorIndexes {
	selectedProcessors[i] = processors[index]
}
amdsmi.SetXgmiFbSharingModeV2(selectedProcessors, mode)
```

The setter requires no running guest VMs. Once an automatic-mode request
succeeds, the example re-queries framebuffer-sharing information and link
topology from each selected GPU to every other discovered GPU.

After a `CUSTOM` request, it queries only link topology because
`amdsmi.GetXgmiFbSharingModeInfo()` does not accept `CUSTOM`.

### Set the XGMI PLPD policy

Choose `2` to change the XGMI per-link power-down policy on the selected GPU.
The example first calls `amdsmi.GetXgmiPlpd()` and displays the current policy
and every supported policy ID:

```text
Current XGMI PLPD policy index is: 0
Policy index 0 (ID 1) is: <device-specific policy description> (current)
Enter the XGMI PLPD policy ID to set: 1
```

The current value returned by `amdsmi.GetXgmiPlpd()` is an index into the
policy list, while the setter accepts a policy ID. The available policy IDs
and descriptions are device-specific. Enter one of the IDs printed by the
example. It repeats the prompt for any other value, requests confirmation,
calls `amdsmi.SetXgmiPlpd()`, and reads the policy again to display the
resulting configuration:

```go
amdsmi.SetXgmiPlpd(processor, uint32(policyID))
printXgmiPlpd(processor)
```

### Handle errors and cleanup

For optional topology and NIC APIs, the example reports unsupported,
missing-driver, no-data, and not-found conditions without aborting the run.

AMD SMI Go APIs return an error when an operation fails. `amdsmi.StatusError`
carries the status code; `amdsmi.StatusCodeToString()` converts it to text.
Optional topology and NIC getters use helpers in `go/examples/example_utils`
so unsupported or missing-data conditions are reported without aborting the
run.

The program shuts down AMD SMI from `defer` after a successful
`amdsmi.Init()`. Press `Ctrl+C` to cancel; a `signal.NotifyContext` reports
the interrupt and then still runs that cleanup.

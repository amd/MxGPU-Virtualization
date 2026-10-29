---

myst:
  html_meta:
    "description lang=en": "Choose and run AMD SMI Python and Go examples for lifecycle management, discovery, monitoring, topology, virtualization, and GPU configuration."
    "keywords": "amd, smi, python, go, examples, GPU, NIC, SR-IOV, monitoring, configuration"

---

# AMD SMI examples

AMD SMI includes matching topic examples in Python and Go. Each how-to describes
one workflow. Pages that have been converted show Python and Go side by side in
language tabs.

- Python examples live in `py/examples/<example>/amdsmi_<example>.py`. Build and
  run instructions are in [Python library usage](amdsmi_py_lib.md).
- Go examples live in `go/examples/<example>/amdsmi_<example>.go`. Setup and
  run instructions are in [Go library usage](amdsmi_go_lib.md).

## Choose an example

### Get started

- [Library lifecycle](examples/amdsmi_lifecycle_example.md) initializes and shuts down
AMD SMI, displays the library version, and decodes status codes.
- [Processor discovery](examples/amdsmi_discovery_example.md) identifies GPU processor
handles and optionally discovers nodes, VFs, and NICs.
- [Static identity](examples/amdsmi_static_identity_example.md) displays GPU and
optional NIC hardware and driver identity information.

### Monitor hardware and firmware

- [Telemetry](examples/amdsmi_telemetry_example.md) uses metrics-table
telemetry when supported, focused compatibility APIs otherwise, and optionally
samples NIC counters.
- [Event monitoring](examples/amdsmi_events_example.md) subscribes to GPU events and
reads event details with a timeout.
- [Firmware](examples/amdsmi_firmware_example.md) displays GPU VBIOS and firmware
inventories, plus optional VF and NIC firmware information.
- [RAS health](examples/amdsmi_ras_health_example.md) inspects ECC, RAS policy and bad
pages.

### Inspect topology and platform settings

- [Topology](examples/amdsmi_topology_example.md) displays GPU, NIC, NUMA, XGMI, and
fabric topology information.
- [Node power](examples/amdsmi_node_power_example.md) reads node power management
status and limits.

### Configure GPUs and virtualization

- [Partitioning](examples/amdsmi_partitioning_example.md) displays or changes memory
and accelerator partitioning.
- [Power and performance control](examples/amdsmi_power_perf_control_example.md)
displays power and performance settings or temporarily changes and restores
selected settings.
- [SR-IOV virtual functions](examples/amdsmi_sriov_vf_example.md) displays VF
information or changes the enabled VF count.
- [GPU reset](examples/amdsmi_reset_example.md) resets a GPU or clears an inactive VF
framebuffer.

## Safety

The lifecycle, discovery, static identity, telemetry, event monitoring,
firmware, RAS health, and node power examples are read-only. The topology
example is read-only when displaying information, but its XGMI operations
change configuration.

The partitioning, power and performance, SR-IOV VF, and reset examples can
modify GPU or virtualization state. Read each example's
prerequisites and operation-specific warnings before selecting a setter or
reset operation. Stop or migrate affected workloads when required, and do not
assume a setting is restored if an API request reports an error.

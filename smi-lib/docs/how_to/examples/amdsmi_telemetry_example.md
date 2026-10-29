---

myst:
  html_meta:
    "description lang=en": "Run the AMD SMI telemetry example with metrics-table telemetry on supported MI GPUs, compatibility telemetry on Navi32 GPUs, and optional NIC counters. Python and Go examples are shown side by side."
    "keywords": "amd, smi, python, go, golang, telemetry, monitoring, GPU, power, temperature, clock, PCIe, NIC, RDMA, example"

---

# Run the AMD SMI telemetry example

::::{tab-set}

:::{tab-item} Python
:sync: python

```{include} ../../../py/examples/telemetry/README.md
:start-line: 2
```

:::

:::{tab-item} Go
:sync: go

```{include} ../../../go/examples/telemetry/README.md
:start-line: 2
```

:::

::::
